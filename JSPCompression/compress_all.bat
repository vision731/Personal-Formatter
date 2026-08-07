@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

:: ============================================
:: JSP 批量压缩脚本
:: 用法: compress_all.bat [目标目录] [输出目录]
::   - 目标目录: 要扫描的文件夹，默认为当前目录
::   - 输出目录: 压缩结果存放位置，默认为 目标目录\compressed
:: ============================================

set "EXE=%~dp0JSPCompressor.exe"
set "TARGET_DIR=%~1"
if "%TARGET_DIR%"=="" set "TARGET_DIR=%~dp0"
set "OUT_DIR=%~2"
if "%OUT_DIR%"=="" set "OUT_DIR=%TARGET_DIR%\compressed"

:: 去掉目标目录末尾的反斜杠
if "%TARGET_DIR:~-1%"=="\" set "TARGET_DIR=%TARGET_DIR:~0,-1%"
if "%OUT_DIR:~-1%"=="\" set "OUT_DIR=%OUT_DIR:~0,-1%"

:: 检查 exe 是否存在
if not exist "%EXE%" (
    echo [错误] 找不到 JSPCompressor.exe，请将其放在本脚本同目录下。
    pause
    exit /b 1
)

:: 检查目标目录是否存在
if not exist "%TARGET_DIR%\" (
    echo [错误] 目录不存在: %TARGET_DIR%
    pause
    exit /b 1
)

:: 创建输出目录
if not exist "%OUT_DIR%\" mkdir "%OUT_DIR%"

echo ============================================
echo   JSP 批量压缩
echo   源目录: %TARGET_DIR%
echo   输出目录: %OUT_DIR%
echo ============================================
echo.

set "COUNT=0"
set "FAIL=0"

for %%f in ("%TARGET_DIR%\*.jsp") do (
    set "IN_FILE=%%f"
    set "OUT_FILE=%OUT_DIR%\%%~nxf"

    echo 压缩中: %%~nxf
    "%EXE%" "%%f" "!OUT_FILE!"

    if !errorlevel! equ 0 (
        set /a COUNT+=1
    ) else (
        echo   [失败] %%~nxf
        set /a FAIL+=1
    )
)

echo.
echo ============================================
echo   完成: 成功 !COUNT! 个, 失败 !FAIL! 个
echo ============================================

endlocal
