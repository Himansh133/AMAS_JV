@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to load Visual Studio x64 Build Environment.
    exit /b %errorlevel%
)

cl.exe /EHsc /I "C:\Program Files\IVI Foundation\VISA\Win64\Include" visa_test.cpp "C:\Program Files\IVI Foundation\VISA\Win64\Lib_x64\msc\visa64.lib" /Fe:visa_test_msvc.exe
if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed.
    exit /b %errorlevel%
)

echo [SUCCESS] visa_test_msvc.exe compiled successfully!
