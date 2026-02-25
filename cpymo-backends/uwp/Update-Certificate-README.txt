UWP 证书更新说明
====================

## 背景
UWP 应用包需要使用证书进行签名。当前证书文件 `cert.pfx` 可能已过期或即将过期。本目录提供了 PowerShell 脚本来检查和更新证书。

## 文件说明
- `cert.pfx` - 当前的证书文件（密码：123456）
- `Check-Certificate.ps1` - 检查证书信息的脚本
- `Update-Certificate.ps1` - 更新证书的脚本

## 使用方法

### 1. 检查当前证书状态
打开 PowerShell，切换到当前目录，运行：

```powershell
.\Check-Certificate.ps1
```

这将显示证书的详细信息，包括过期时间。

### 2. 更新证书
如果证书已过期或需要更新，运行：

```powershell
.\Update-Certificate.ps1
```

如果证书仍在有效期内但需要强制更新，使用 `-Force` 参数：

```powershell
.\Update-Certificate.ps1 -Force
```

### 3. 脚本参数
`Update-Certificate.ps1` 支持以下参数：

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `-PfxPath` | `cert.pfx` | PFX 文件路径 |
| `-Password` | `123456` | 证书密码 |
| `-Subject` | `CN=CPyMO` | 证书主题名称 |
| `-ValidYears` | `5` | 有效期（年） |
| `-ProjectFile` | `CPyMO.vcxproj` | 项目文件路径 |
| `-Force` | `$false` | 强制更新（即使证书未过期） |

### 4. 更新过程
脚本将执行以下操作：

1. 检查现有证书（如果存在）
2. 生成新的自签名证书（SHA256，2048位密钥，有效期5年）
3. 导出为 PFX 文件（使用相同密码）
4. 更新 `CPyMO.vcxproj` 中的证书指纹
5. 清理临时证书

## 注意事项

### 证书密码
- 当前密码为 `123456`（在 CI 配置中硬编码）
- 如需更改密码，需要同时更新：
  - `Update-Certificate.ps1` 的 `-Password` 参数
  - `.github/workflows/ci.yml` 中的 `PackageCertificatePassword` 值

### 证书主题
- 主题名称必须为 `CN=CPyMO` 以匹配应用清单中的发布者信息
- 如需更改主题，需要同时更新：
  - `Update-Certificate.ps1` 的 `-Subject` 参数
  - `Package.appxmanifest` 中的 `<Identity Publisher="..." />` 属性

### 构建影响
- 更新证书后，需要重新构建 UWP 项目
- CI/CD 流水线将自动使用新证书，因为 `cert.pfx` 文件已被替换
- 已签名的旧版应用包将无法更新到新证书签名的版本（需要卸载重装）

## CI/CD 集成
GitHub Actions 构建 UWP 时使用以下命令：

```bash
msbuild -m -p:UapAppxPackageBuildMode=SideLoadOnly \
  -p:AppxBundlePlatforms="x86|x64|arm64" \
  -p:AppxPackageSigningEnabled=true \
  -p:Configuration=Release \
  -p:PackageCertificateKeyFile=cert.pfx \
  -p:PackageCertificatePassword="123456" \
  CPyMO.vcxproj
```

证书更新后，构建将自动使用新证书。

## 故障排除

### PowerShell 执行策略
如果脚本无法运行，可能需要调整执行策略：

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
```

### New-SelfSignedCertificate 不可用
某些 Windows Server 版本可能没有 `New-SelfSignedCertificate` cmdlet。此时需要：

1. 使用其他方式生成证书（如 OpenSSL）
2. 或使用开发机器生成证书后复制到仓库

### 项目文件更新失败
如果脚本无法更新项目文件，请手动编辑 `CPyMO.vcxproj`，查找并替换：

```xml
<PackageCertificateThumbprint>C6A81DDDAEBB7CAECC0136559395525FE6A5147E</PackageCertificateThumbprint>
```

将指纹替换为新证书的指纹（可通过 `Check-Certificate.ps1` 查看）。

## 技术支持
如有问题，请参考：
- UWP 应用打包文档：https://docs.microsoft.com/en-us/windows/msix/package/create-certificate-package-signing
- PowerShell 证书管理：https://docs.microsoft.com/en-us/powershell/module/pki/

---
脚本生成时间: $(Get-Date)
