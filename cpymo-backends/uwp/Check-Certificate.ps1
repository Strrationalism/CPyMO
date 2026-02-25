# Check-Certificate.ps1
# Check certificate details including expiration

param(
    [string]$PfxPath = "cert.pfx",
    [string]$Password = "123456"
)

Write-Host "Checking certificate: $PfxPath" -ForegroundColor Cyan

# Convert password to SecureString
$securePassword = ConvertTo-SecureString -String $Password -AsPlainText -Force

try {
    # Import certificate
    $cert = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2
    $cert.Import($PfxPath, $securePassword, [System.Security.Cryptography.X509Certificates.X509KeyStorageFlags]::Exportable)

    Write-Host "Certificate Information:" -ForegroundColor Green
    Write-Host "  Subject: $($cert.Subject)"
    Write-Host "  Issuer: $($cert.Issuer)"
    Write-Host "  Thumbprint: $($cert.Thumbprint)"
    Write-Host "  Valid From: $($cert.NotBefore.ToString('yyyy-MM-dd'))"
    Write-Host "  Valid To: $($cert.NotAfter.ToString('yyyy-MM-dd'))"
    Write-Host "  Days Remaining: $((New-TimeSpan -Start (Get-Date) -End $cert.NotAfter).Days)"
    Write-Host "  Algorithm: $($cert.SignatureAlgorithm.FriendlyName)"
    Write-Host "  Version: $($cert.Version)"

    # Check if expired
    $isExpired = $cert.NotAfter -lt (Get-Date)
    if ($isExpired) {
        Write-Host "Certificate is EXPIRED!" -ForegroundColor Red
    } else {
        Write-Host "Certificate is valid." -ForegroundColor Green
    }

} catch {
    Write-Host "Error: $_" -ForegroundColor Red
    exit 1
}