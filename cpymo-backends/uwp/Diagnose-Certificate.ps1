# Diagnose-Certificate.ps1
# Comprehensive certificate diagnosis for UWP signing issues
param(
    [string]$PfxPath = "cert.pfx",
    [string]$Password = "123456"
)

$ErrorActionPreference = "Stop"
Write-Host "=== UWP Certificate Signing Issue Diagnosis ===" -ForegroundColor Cyan
Write-Host "Diagnosis Time: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" -ForegroundColor Cyan
Write-Host "Certificate File: $PfxPath" -ForegroundColor Cyan
Write-Host ""

# 1. Load certificate
Write-Host "=== 1. Certificate Basic Information ===" -ForegroundColor Yellow
$securePassword = ConvertTo-SecureString -String $Password -AsPlainText -Force
$cert = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2
try {
    $cert.Import($PfxPath, $securePassword, [System.Security.Cryptography.X509Certificates.X509KeyStorageFlags]::Exportable)
    Write-Host "[OK] Certificate loaded successfully" -ForegroundColor Green
} catch {
    Write-Host "[ERROR] Failed to load certificate: $_" -ForegroundColor Red
    exit 1
}

Write-Host "  Subject: $($cert.Subject)"
Write-Host "  Issuer: $($cert.Issuer)"
Write-Host "  Thumbprint: $($cert.Thumbprint)"
Write-Host "  Valid From: $($cert.NotBefore.ToString('yyyy-MM-dd')) to $($cert.NotAfter.ToString('yyyy-MM-dd'))"
Write-Host "  Algorithm: $($cert.SignatureAlgorithm.FriendlyName)"
Write-Host "  Version: $($cert.Version)"

# Validity check
$now = Get-Date
$isExpired = $cert.NotAfter -lt $now
$isNotYetValid = $cert.NotBefore -gt $now
$validityDays = ($cert.NotAfter - $now).Days

Write-Host ""
Write-Host "  Validity Status:" -ForegroundColor Cyan
if ($isExpired) {
    Write-Host "  [ERROR] Certificate expired on: $($cert.NotAfter.ToString('yyyy-MM-dd'))" -ForegroundColor Red
} elseif ($isNotYetValid) {
    Write-Host "  [ERROR] Certificate not yet valid (starts: $($cert.NotBefore.ToString('yyyy-MM-dd')))" -ForegroundColor Red
} else {
    Write-Host "  [OK] Certificate is valid (remaining: $validityDays days)" -ForegroundColor Green
}

# 2. Check extensions
Write-Host ""
Write-Host "=== 2. Certificate Extensions Check ===" -ForegroundColor Yellow
$hasCodeSigning = $false
$hasDigitalSignature = $false
$hasKeyEncipherment = $false

foreach ($extension in $cert.Extensions) {
    $oid = $extension.Oid.Value
    $friendlyName = $extension.Oid.FriendlyName
    $formatted = $extension.Format($true)

    switch ($oid) {
        "2.5.29.37" {  # Enhanced Key Usage
            Write-Host "  - Enhanced Key Usage (EKU): $formatted"
            if ($formatted -like "*Code Signing*") {
                $hasCodeSigning = $true
                Write-Host "    [OK] Contains Code Signing extension (OID 1.3.6.1.5.5.7.3.3)" -ForegroundColor Green
            } else {
                Write-Host "    [ERROR] Missing Code Signing extension" -ForegroundColor Red
            }
        }
        "2.5.29.15" {  # Key Usage
            Write-Host "  - Key Usage: $formatted"
            if ($formatted -like "*Digital Signature*") {
                $hasDigitalSignature = $true
                Write-Host "    [OK] Contains Digital Signature" -ForegroundColor Green
            }
            if ($formatted -like "*Key Encipherment*") {
                $hasKeyEncipherment = $true
                Write-Host "    [OK] Contains Key Encipherment" -ForegroundColor Green
            }
        }
        "2.5.29.19" {  # Basic Constraints
            Write-Host "  - Basic Constraints: $formatted"
        }
        "2.5.29.14" {  # Subject Key Identifier
            Write-Host "  - Subject Key Identifier: $formatted"
        }
        "2.5.29.35" {  # Authority Key Identifier
            Write-Host "  - Authority Key Identifier: $formatted"
        }
        default {
            Write-Host "  - $friendlyName ($oid): $formatted"
        }
    }
}

if (-not $hasCodeSigning) {
    Write-Host "  [WARNING] Certificate missing Code Signing extension (1.3.6.1.5.5.7.3.3)" -ForegroundColor Red
    Write-Host "    This is required for UWP app signing." -ForegroundColor Red
}

# 3. Manifest file match check
Write-Host ""
Write-Host "=== 3. Manifest File Match Check ===" -ForegroundColor Yellow
$manifestPath = "Package.appxmanifest"
if (Test-Path $manifestPath) {
    try {
        $manifest = [xml](Get-Content $manifestPath)
        $manifestPublisher = $manifest.Package.Identity.Publisher
        Write-Host "  Manifest Publisher: $manifestPublisher"

        # Extract CN from certificate subject
        $certCN = ""
        if ($cert.Subject -match "CN=([^,]+)") {
            $certCN = $matches[1].Trim()
        }

        Write-Host "  Certificate Subject CN: $certCN"
        Write-Host "  Full Certificate Subject: $($cert.Subject)"

        # Check match logic
        $isMatch = $false
        if ($manifestPublisher -eq "CN=$certCN") {
            $isMatch = $true
            Write-Host "  [OK] Publisher exact match: Certificate CN matches manifest publisher" -ForegroundColor Green
        } elseif ($manifestPublisher -eq $cert.Subject) {
            $isMatch = $true
            Write-Host "  [OK] Publisher exact match: Certificate subject matches manifest publisher" -ForegroundColor Green
        } elseif ($manifestPublisher -like "*$certCN*") {
            Write-Host "  [WARNING] Publisher partial match: Manifest publisher contains certificate CN" -ForegroundColor Yellow
            Write-Host "    Manifest: $manifestPublisher"
            Write-Host "    Certificate CN: $certCN"
        } else {
            Write-Host "  [ERROR] Publisher mismatch!" -ForegroundColor Red
            Write-Host "    Manifest expects: $manifestPublisher"
            Write-Host "    Certificate provides: CN=$certCN (full subject: $($cert.Subject))"

            # Suggested fixes
            Write-Host ""
            Write-Host "  Suggested fixes:" -ForegroundColor Cyan
            Write-Host "  A. Update manifest publisher to: CN=$certCN, O=CPyMO, C=US" -ForegroundColor Yellow
            Write-Host "  B. Regenerate certificate: .\Update-Certificate.ps1 -Force -Subject `"CN=CPyMO`"" -ForegroundColor Yellow
        }
    } catch {
        Write-Host "  [ERROR] Failed to parse manifest file: $_" -ForegroundColor Red
    }
} else {
    Write-Host "  [ERROR] Manifest file not found: $manifestPath" -ForegroundColor Red
}

# 4. Project file match check
Write-Host ""
Write-Host "=== 4. Project File Match Check ===" -ForegroundColor Yellow
$projectFile = "CPyMO.vcxproj"
if (Test-Path $projectFile) {
    $projectContent = Get-Content $projectFile -Raw
    # Find PackageCertificateThumbprint
    if ($projectContent -match '<PackageCertificateThumbprint>\s*([A-Fa-f0-9]{40})\s*</PackageCertificateThumbprint>') {
        $projectThumbprint = $matches[1]
        Write-Host "  Project file thumbprint: $projectThumbprint"
        Write-Host "  Actual certificate thumbprint: $($cert.Thumbprint)"

        if ($projectThumbprint -eq $cert.Thumbprint) {
            Write-Host "  [OK] Thumbprint matches" -ForegroundColor Green
        } else {
            Write-Host "  [ERROR] Thumbprint mismatch!" -ForegroundColor Red
            Write-Host "    Project expects: $projectThumbprint"
            Write-Host "    Certificate actual: $($cert.Thumbprint)"

            Write-Host ""
            Write-Host "  Suggested fix:" -ForegroundColor Cyan
            Write-Host "  Run: .\Update-Certificate.ps1 -Force" -ForegroundColor Yellow
            Write-Host "  This will automatically update thumbprint in project file." -ForegroundColor Yellow
        }
    } else {
        Write-Host "  [WARNING] PackageCertificateThumbprint node not found in project file" -ForegroundColor Yellow
        # Check if thumbprint exists elsewhere
        if ($projectContent -match 'PackageCertificateThumbprint') {
            Write-Host "  But found PackageCertificateThumbprint text, maybe different format" -ForegroundColor Yellow
        }
    }

    # Check AppxPackageSigningEnabled
    if ($projectContent -match '<AppxPackageSigningEnabled>(.*?)</AppxPackageSigningEnabled>') {
        $signingEnabled = $matches[1].Trim()
        Write-Host "  AppxPackageSigningEnabled: $signingEnabled"
        if ($signingEnabled -eq "true") {
            Write-Host "  [OK] Appx package signing enabled" -ForegroundColor Green
        } else {
            Write-Host "  [WARNING] Appx package signing not enabled" -ForegroundColor Yellow
        }
    }
} else {
    Write-Host "  [ERROR] Project file not found: $projectFile" -ForegroundColor Red
}

# 5. System environment check
Write-Host ""
Write-Host "=== 5. System Environment Check ===" -ForegroundColor Yellow
Write-Host "  PowerShell Version: $($PSVersionTable.PSVersion)"
Write-Host "  System Time: $now"
Write-Host "  Timezone: $(Get-TimeZone).Id"

# Check if New-SelfSignedCertificate cmdlet is available
try {
    $null = Get-Command New-SelfSignedCertificate -ErrorAction Stop
    Write-Host "  [OK] New-SelfSignedCertificate cmdlet available" -ForegroundColor Green
} catch {
    Write-Host "  [ERROR] New-SelfSignedCertificate cmdlet not available" -ForegroundColor Red
    Write-Host "    This may affect certificate generation." -ForegroundColor Yellow
}

# 6. Certificate validity summary
Write-Host ""
Write-Host "=== 6. Certificate Validity Summary ===" -ForegroundColor Cyan

$issues = @()

if ($isExpired) { $issues += "Certificate expired" }
if ($isNotYetValid) { $issues += "Certificate not yet valid" }
if (-not $hasCodeSigning) { $issues += "Missing code signing extension" }

# Check publisher match
if ($manifestPublisher -and $certCN) {
    if ($manifestPublisher -ne "CN=$certCN" -and $manifestPublisher -ne $cert.Subject) {
        $issues += "Publisher mismatch"
    }
}

# Check thumbprint match
if ($projectThumbprint -and $projectThumbprint -ne $cert.Thumbprint) {
    $issues += "Thumbprint mismatch"
}

if ($issues.Count -eq 0) {
    Write-Host "[OK] Certificate appears valid, all checks passed" -ForegroundColor Green
    Write-Host "  Possible issues:" -ForegroundColor Yellow
    Write-Host "  - CI environment certificate store access permissions" -ForegroundColor Yellow
    Write-Host "  - Windows SDK version compatibility" -ForegroundColor Yellow
    Write-Host "  - Build command parameter issues" -ForegroundColor Yellow
    exit 0
} else {
    Write-Host "[ISSUES] Found $($issues.Count) issue(s):" -ForegroundColor Red
    foreach ($issue in $issues) {
        Write-Host "  [X] $issue" -ForegroundColor Red
    }

    Write-Host ""
    Write-Host "=== Suggested Solutions ===" -ForegroundColor Cyan
    if ($issues -contains "Publisher mismatch") {
        Write-Host "1. Regenerate certificate:" -ForegroundColor Yellow
        Write-Host "   .\Update-Certificate.ps1 -Force -Subject `"CN=CPyMO`"" -ForegroundColor Green
        Write-Host ""
    }

    if ($issues -contains "Missing code signing extension") {
        Write-Host "2. Create certificate with code signing extension:" -ForegroundColor Yellow
        Write-Host "   .\Update-Certificate.ps1 -Force" -ForegroundColor Green
        Write-Host "   (Script includes correct extension configuration)" -ForegroundColor Yellow
        Write-Host ""
    }

    if ($issues -contains "Certificate expired" -or $issues -contains "Certificate not yet valid") {
        Write-Host "3. Regenerate certificate:" -ForegroundColor Yellow
        Write-Host "   .\Update-Certificate.ps1 -Force" -ForegroundColor Green
        Write-Host ""
    }

    if ($issues -contains "Thumbprint mismatch") {
        Write-Host "4. Update project file thumbprint:" -ForegroundColor Yellow
        Write-Host "   .\Update-Certificate.ps1 -Force" -ForegroundColor Green
        Write-Host "   (Script automatically updates thumbprint)" -ForegroundColor Yellow
        Write-Host ""
    }

    exit 1
}

Write-Host ""
Write-Host "Diagnosis complete. Review results above to determine root cause of certificate signing issue." -ForegroundColor Cyan