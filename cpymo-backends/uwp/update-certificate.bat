@echo off
REM update-certificate.bat
REM 批处理文件用于运行 PowerShell 脚本更新证书

echo UWP 证书更新工具
echo.

REM 检查 PowerShell 是否可用
where powershell >nul 2>nul
if %errorlevel% neq 0 (
    echo 错误: 未找到 PowerShell。
    pause
    exit /b 1
)

REM 默认运行更新脚本
if "%1"=="" (
    echo Running certificate update script...
    echo Use -Force parameter to force update (even if certificate is not expired).
    echo.
    powershell -NoProfile -ExecutionPolicy Bypass -File "Update-Certificate.ps1" %*
    if %errorlevel% neq 0 (
        echo 证书更新失败。
        pause
        exit /b 1
    )
) else (
    powershell -ExecutionPolicy Bypass -File "Update-Certificate.ps1" %*
)

echo.
echo 完成。
echo 请查看 Update-Certificate-README.txt 获取更多信息。
pause