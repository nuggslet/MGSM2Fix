@echo off
setlocal
pushd "%~dp0"
if not exist "..\x64\tests" mkdir "..\x64\tests"
for %%T in (test_*.cpp) do (
    cl /nologo /std:c++20 /EHsc "%%T" /Fo"..\x64\tests\%%~nT.obj" /Fe"..\x64\tests\%%~nT.exe"
    if errorlevel 1 exit /b 1
    "..\x64\tests\%%~nT.exe"
    if errorlevel 1 exit /b 1
)
popd
