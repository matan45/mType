@echo off
setlocal enabledelayedexpansion

set "MTYPE_EXE=bin\mType\Debug\x64\mType.exe"
set "TEST_DIR=mType\tests\testFiles\optimizer\pass"

echo Comparing optimizer test outputs with expected files...
echo.

set PASSED=0
set FAILED=0
set TOTAL=0

for %%f in ("%TEST_DIR%\*.mt") do (
    set /a TOTAL+=1
    set "expectedfile=%%~dpnf.expected"

    echo [!TOTAL!] Testing: %%~nxf

    if not exist "!expectedfile!" (
        echo     SKIP: No expected file found
        echo.
    ) else (
        "%MTYPE_EXE%" --bytecode -release "%%f" > "%TEMP%\mtype_test.txt" 2>&1

        fc /b "%TEMP%\mtype_test.txt" "!expectedfile!" >nul 2>&1

        if !errorlevel! equ 0 (
            echo     PASS
            set /a PASSED+=1
        ) else (
            echo     FAIL - Output differs from expected
            set /a FAILED+=1
        )
    )
    echo.
)

del "%TEMP%\mtype_test.txt" 2>nul

echo ========================================
echo Results: %PASSED% passed, %FAILED% failed out of %TOTAL% tests
echo ========================================
pause