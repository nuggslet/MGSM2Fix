@echo off
setlocal
pushd "%~dp0"
if not exist "..\..\x64\tests" mkdir "..\..\x64\tests"
cl /nologo /std:c++20 /EHsc /I "..\..\src\extern\json\single_include" test_patch_options.cpp /Fo"..\..\x64\tests\patch_options.obj" /Fe"..\..\x64\tests\patch_options.exe"
if errorlevel 1 exit /b 1
"..\..\x64\tests\patch_options.exe"
if errorlevel 1 exit /b 1
popd
