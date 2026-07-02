@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to load Visual Studio x64 Build Environment.
    exit /b %errorlevel%
)

cl.exe /EHsc positioner_test.cpp /Fe:positioner_test.exe
if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed.
    exit /b %errorlevel%
)

echo [SUCCESS] positioner_test.exe compiled successfully!
