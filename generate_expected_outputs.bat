@echo off
setlocal enabledelayedexpansion

set "MTYPE_EXE=bin\mType\Debug\x64\mType.exe"
set "TEST_DIR=mType\tests\testFiles\optimizer\pass"

echo Generating .expected files for optimizer tests...
echo.

for %%f in ("%TEST_DIR%\*.mt") do (
    set "testfile=%%f"
    set "expectedfile=%%~dpnf.expected"

    echo Processing: %%~nxf
    "%MTYPE_EXE%" --bytecode -release "%%f" > "!expectedfile!" 2>&1

    if !errorlevel! equ 0 (
        echo   Created: %%~nf.expected
    ) else (
        echo   WARNING: Test failed for %%~nxf
    )
    echo.
)

echo Done! All .expected files have been generated.
pause
