@echo off
setlocal enabledelayedexpansion

set "MTYPE_EXE=bin\mType\Debug\x64\mType.exe"
set "TEST_DIR=mType\tests\testFiles\optimizer\pass"

echo ========================================
echo Running optimizer test files...
echo ========================================
echo.

set COUNT=0

for %%f in ("%TEST_DIR%\*.mt") do (
    set /a COUNT+=1
    echo ========================================
    echo [!COUNT!] Running: %%~nxf
    echo ========================================
    "%MTYPE_EXE%" --bytecode -release "%%f"
    echo.
    echo Exit code: !errorlevel!
    echo.
)

echo ========================================
echo Completed running %COUNT% optimizer test files
echo ========================================
pause