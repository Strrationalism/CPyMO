# Update-Certificate.ps1
# 生成新的自签名证书并更新项目文件

param(
    [string]$PfxPath = "cert.pfx",
    [string]$Password = "123456",
    [string]$Subject = "CN=CPyMO, O=CPyMO, C=US",
    [int]$ValidYears = 5,
    [string]$ProjectFile = "CPyMO.vcxproj",
    [switch]$Force = $false
)

Write-Host "Updating UWP certificate" -ForegroundColor Cyan

# 检查现有证书
if (Test-Path $PfxPath) {
    Write-Host "Found existing certificate: $PfxPath" -ForegroundColor Yellow
    $securePassword = ConvertTo-SecureString -String $Password -AsPlainText -Force
    try {
        $oldCert = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2
        $oldCert.Import($PfxPath, $securePassword, [System.Security.Cryptography.X509Certificates.X509KeyStorageFlags]::Exportable)
        Write-Host "Current certificate thumbprint: $($oldCert.Thumbprint)"
        Write-Host "Current certificate valid until: $($oldCert.NotAfter.ToString('yyyy-MM-dd'))"

        # 检查是否过期
        $isExpired = $oldCert.NotAfter -lt (Get-Date)
        if (-not $isExpired -and -not $Force) {
            Write-Host "Certificate is still valid. Use -Force parameter to force renewal." -ForegroundColor Yellow
            exit 0
        }
    } catch {
        Write-Host "Cannot read existing certificate, will generate new one." -ForegroundColor Yellow
    }
} else {
    Write-Host "No existing certificate found, will generate new one." -ForegroundColor Yellow
}

# 生成新证书
Write-Host "Generating new self-signed certificate..." -ForegroundColor Green
$certParams = @{
    Subject = $Subject
    CertStoreLocation = "Cert:\CurrentUser\My"
    KeyExportPolicy = "Exportable"
    KeySpec = "Signature"
    KeyLength = 2048
    HashAlgorithm = "SHA256"
    NotAfter = (Get-Date).AddYears($ValidYears)
    TextExtension = @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}")
}

try {
    $newCert = New-SelfSignedCertificate @certParams
    Write-Host "New certificate generated successfully!" -ForegroundColor Green
    Write-Host "  Subject: $($newCert.Subject)"
    Write-Host "  Thumbprint: $($newCert.Thumbprint)"
    Write-Host "  Valid until: $($newCert.NotAfter.ToString('yyyy-MM-dd'))"
} catch {
    Write-Host "Failed to generate certificate: $_" -ForegroundColor Red
    exit 1
}

# 导出为 PFX 文件
Write-Host "Exporting certificate to PFX file..." -ForegroundColor Green
$securePassword = ConvertTo-SecureString -String $Password -AsPlainText -Force
$pfxBytes = $newCert.Export([System.Security.Cryptography.X509Certificates.X509ContentType]::Pfx, $securePassword)
try {
    [System.IO.File]::WriteAllBytes((Resolve-Path .).Path + "\" + $PfxPath, $pfxBytes)
    Write-Host "Certificate exported to: $PfxPath" -ForegroundColor Green
} catch {
    Write-Host "Failed to export certificate: $_" -ForegroundColor Red
    exit 1
}

# 更新项目文件中的指纹
Write-Host "Updating certificate thumbprint in project file..." -ForegroundColor Green
if (Test-Path $ProjectFile) {
    $content = Get-Content $ProjectFile -Raw
    # 正则表达式匹配 <PackageCertificateThumbprint>...</PackageCertificateThumbprint>，允许前后有空格
    $pattern = '(?s)<PackageCertificateThumbprint>\s*[A-Fa-f0-9]{40}\s*</PackageCertificateThumbprint>'

    if ($content -match $pattern) {
        $newContent = $content -replace $pattern, "<PackageCertificateThumbprint>$($newCert.Thumbprint)</PackageCertificateThumbprint>"
        Set-Content -Path $ProjectFile -Value $newContent -Encoding UTF8
        Write-Host "Project file updated, new thumbprint: $($newCert.Thumbprint)" -ForegroundColor Green
    } else {
        Write-Host "PackageCertificateThumbprint node not found in project file, trying more flexible match..." -ForegroundColor Yellow
        # 尝试更宽松的匹配，忽略指纹长度
        $pattern2 = '(?s)<PackageCertificateThumbprint>.*?</PackageCertificateThumbprint>'
        if ($content -match $pattern2) {
            $newContent = $content -replace $pattern2, "<PackageCertificateThumbprint>$($newCert.Thumbprint)</PackageCertificateThumbprint>"
            Set-Content -Path $ProjectFile -Value $newContent -Encoding UTF8
            Write-Host "Project file updated (flexible match), new thumbprint: $($newCert.Thumbprint)" -ForegroundColor Green
        } else {
            Write-Host "Warning: Cannot find PackageCertificateThumbprint node, please update project file manually." -ForegroundColor Red
            Write-Host "New certificate thumbprint: $($newCert.Thumbprint)" -ForegroundColor Yellow
        }
    }
} else {
    Write-Host "Project file not found: $ProjectFile" -ForegroundColor Red
}

# 从证书存储中删除临时证书（可选）
Write-Host "Removing temporary certificate from certificate store..." -ForegroundColor Yellow
try {
    Get-ChildItem "Cert:\CurrentUser\My\$($newCert.Thumbprint)" | Remove-Item
    Write-Host "Temporary certificate removed." -ForegroundColor Green
} catch {
    Write-Host "Failed to remove temporary certificate (non-critical): $_" -ForegroundColor Yellow
}

Write-Host "Certificate update completed!" -ForegroundColor Cyan
Write-Host "Please rebuild UWP project to use the new certificate." -ForegroundColor Yellow