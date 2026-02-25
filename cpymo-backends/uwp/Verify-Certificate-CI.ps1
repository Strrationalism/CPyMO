# Verify-Certificate-CI.ps1
# Certificate verification script for CI environments
# Returns 0 if certificate is valid, 1 if invalid (will attempt to regenerate)
param(
    [string]$PfxPath = "cert.pfx",
    [string]$Password = "123456"
)

$ErrorActionPreference = "Stop"
Write-Host "Verifying certificate for CI build..." -ForegroundColor Cyan

# 1. Check if certificate file exists
if (-not (Test-Path $PfxPath)) {
    Write-Host "[ERROR] Certificate file not found: $PfxPath" -ForegroundColor Red
    exit 1
}

# 2. Try to load certificate
$securePassword = ConvertTo-SecureString -String $Password -AsPlainText -Force
try {
    $cert = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2
    $cert.Import($PfxPath, $securePassword, [System.Security.Cryptography.X509Certificates.X509KeyStorageFlags]::Exportable)
    Write-Host "[OK] Certificate loaded" -ForegroundColor Green
    Write-Host "  Thumbprint: $($cert.Thumbprint)"
    Write-Host "  Subject: $($cert.Subject)"
    Write-Host "  Valid until: $($cert.NotAfter.ToString('yyyy-MM-dd'))"
} catch {
    Write-Host "[ERROR] Failed to load certificate: $_" -ForegroundColor Red
    exit 1
}

# 3. Check validity dates
$now = Get-Date
$isExpired = $cert.NotAfter -lt $now
$isNotYetValid = $cert.NotBefore -gt $now

if ($isExpired) {
    Write-Host "[ERROR] Certificate expired on $($cert.NotAfter.ToString('yyyy-MM-dd'))" -ForegroundColor Red
    exit 1
}

if ($isNotYetValid) {
    Write-Host "[ERROR] Certificate not yet valid (starts $($cert.NotBefore.ToString('yyyy-MM-dd')))" -ForegroundColor Red
    exit 1
}

Write-Host "[OK] Certificate is within validity period" -ForegroundColor Green

# 4. Check for code signing extension (simplified check)
$hasCodeSigning = $false
foreach ($extension in $cert.Extensions) {
    $oid = $extension.Oid.Value
    if ($oid -eq "2.5.29.37") {  # Enhanced Key Usage
        $formatted = $extension.Format($true)
        if ($formatted -like "*1.3.6.1.5.5.7.3.3*") {
            $hasCodeSigning = $true
            Write-Host "[OK] Certificate has code signing extension" -ForegroundColor Green
            break
        }
    }
}

if (-not $hasCodeSigning) {
    Write-Host "[WARNING] Certificate may be missing code signing extension" -ForegroundColor Yellow
    # Continue anyway, let msbuild fail with specific error
}

# 5. Check manifest match
$manifestPath = "Package.appxmanifest"
if (Test-Path $manifestPath) {
    try {
        $manifest = [xml](Get-Content $manifestPath)
        $manifestPublisher = $manifest.Package.Identity.Publisher
        Write-Host "  Manifest publisher: $manifestPublisher"

        # Simple check: compare CN parts
        if ($cert.Subject -match "CN=([^,]+)") {
            $certCN = $matches[1].Trim()
            if ($manifestPublisher -eq "CN=$certCN" -or $manifestPublisher -eq $cert.Subject) {
                Write-Host "[OK] Publisher matches" -ForegroundColor Green
            } else {
                Write-Host "[WARNING] Publisher may not match exactly" -ForegroundColor Yellow
                Write-Host "  Manifest: $manifestPublisher"
                Write-Host "  Certificate: $($cert.Subject)"
            }
        }
    } catch {
        Write-Host "[WARNING] Could not verify manifest match: $_" -ForegroundColor Yellow
    }
}

Write-Host "[SUCCESS] Certificate verification passed" -ForegroundColor Green
exit 0