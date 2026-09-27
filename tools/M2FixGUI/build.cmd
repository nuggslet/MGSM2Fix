@echo off
setlocal
pushd "%~dp0\..\.."
if errorlevel 1 exit /b 1
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "M2FIX_VS=%%i"
if not defined M2FIX_VS (
    echo Visual Studio C++ Build Tools are required.
    set "M2FIXGUI_BUILD_ERROR=1"
    goto fail
)
call "%M2FIX_VS%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 (
    set "M2FIXGUI_BUILD_ERROR=1"
    goto fail
)
set "M2FIXGUI_OUTPUT_OPTION="
if defined M2FIXGUI_OUTPUT_DIR set M2FIXGUI_OUTPUT_OPTION=-DM2FIXGUI_OUTPUT_DIR="%M2FIXGUI_OUTPUT_DIR%"
cmake -S tools/M2FixGUI -B build/M2FixGUI-native -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release %M2FIXGUI_OUTPUT_OPTION%
if errorlevel 1 (
    set "M2FIXGUI_BUILD_ERROR=1"
    goto fail
)
cmake --build build/M2FixGUI-native
if errorlevel 1 (
    set "M2FIXGUI_BUILD_ERROR=1"
    goto fail
)
ctest --test-dir build/M2FixGUI-native -C Release --output-on-failure
if errorlevel 1 (
    set "M2FIXGUI_BUILD_ERROR=1"
    goto fail
)
copy /Y tools\integral-english\BUILDER.md dist\M2FixGUI-README.md >nul
if errorlevel 1 (
    set "M2FIXGUI_BUILD_ERROR=1"
    goto fail
)
copy /Y tools\integral-english\CREDITS.md dist\CREDITS.md >nul
if errorlevel 1 (
    set "M2FIXGUI_BUILD_ERROR=1"
    goto fail
)
popd
endlocal
exit /b 0

:fail
popd
endlocal & exit /b %M2FIXGUI_BUILD_ERROR%
