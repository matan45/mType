@echo off
echo ========================================
echo  mType VS Code Extension Build Script
echo ========================================
echo.

echo [1/3] Navigating to extension directory...
cd /d "%~dp0mtype-vscode-extension"
if %errorlevel% neq 0 (
    echo ERROR: Failed to navigate to extension directory
    pause
    exit /b 1
)

echo [2/3] Compiling TypeScript...
call npm run compile
if %errorlevel% neq 0 (
    echo ERROR: TypeScript compilation failed
    pause
    exit /b 1
)

echo [3/3] Packaging extension...
call vsce package
if %errorlevel% neq 0 (
    echo ERROR: Extension packaging failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo  Build completed successfully!
echo ========================================
echo Extension packaged as: mtype-language-support-1.0.0.vsix
echo.
pause