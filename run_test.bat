@echo off
if "%1"=="" (
    echo Usage: run_test.bat ^<test_file.mt^>
    echo Example: run_test.bat mType/tests/testFiles/stringpool/string_pool_integration_test.mt
    exit /b 1
)

echo Running test file: %1
"bin\mType\Debug\x64\mType.exe" "%1"
rm "C:\Users\matan\source\repos\mType\nul"
echo.
echo Press any key to continue...
pause >nul