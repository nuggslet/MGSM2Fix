REM M2Package.bat: Release archive packager for M2Fix.

@echo off
pushd "%~dp0"

for /f "usebackq tokens=1* delims=: " %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere" -products * -latest -requires Microsoft.Component.MSBuild`) do (
	if /i "%%i"=="installationPath" (
		set vsdir=%%j
	)
)

echo %vsdir%

if exist "%vsdir%\Common7\Tools\VsMSBuildCmd.bat" (
	call "%vsdir%\Common7\Tools\VsMSBuildCmd"
	cd "%~dp0"
)

REM Clean builds
echo --------------------------------------------------------
echo [INFO] Cleaning previous Release builds…
MSBuild MGSM2Fix.sln /p:Configuration=Release /p:Platform=x86 /t:Clean /verbosity:minimal
MSBuild MGSM2Fix.sln /p:Configuration=Release /p:Platform=x64 /t:Clean /verbosity:minimal

REM Build
echo --------------------------------------------------------
echo [INFO] Building Release/x86…
MSBuild MGSM2Fix.sln /p:Configuration=Release /p:Platform=x86 /verbosity:minimal
if errorlevel 1 (
	echo [ERROR] Build failed for x86.
	exit /b 1
)

echo [INFO] Building Release/x64…
MSBuild MGSM2Fix.sln /p:Configuration=Release /p:Platform=x64 /verbosity:minimal
if errorlevel 1 (
	echo [ERROR] Build failed for x64.
	exit /b 1
)

echo --------------------------------------------------------
echo [INFO] Cleaning and preparing dist\ folder…
if exist dist\ (
	rd /S /Q dist\
)
md dist\

echo --------------------------------------------------------
echo [INFO] Copying resource files…
xcopy /I /Y /F res\ dist\
xcopy /I /Y /F asiloader\ dist\
xcopy /Y /F MGSM2Fix.ini dist\MGSM2Fix.ini
xcopy /Y /F Release\MGSM2Fix.asi dist\MGSM2Fix32.asi
xcopy /Y /F x64\Release\MGSM2Fix.asi dist\MGSM2Fix64.asi

REM Remove old archive
echo --------------------------------------------------------
echo [INFO] Removing old MGSM2Fix.zip if it exists…
if exist MGSM2Fix.zip (
	del /Q MGSM2Fix.zip
)

REM Create archive
echo --------------------------------------------------------
echo [INFO] Creating MGSM2Fix.zip archive…
powershell -command ^
    "Compress-Archive -Path 'dist\*' -DestinationPath 'MGSM2Fix.zip' -Force"

if exist MGSM2Fix.zip (
	echo [INFO] Archive created successfully: MGSM2Fix.zip
) else (
	echo [ERROR] Failed to create MGSM2Fix.zip
	exit /b 1
)

echo ========================================================
echo [INFO] M2Package.bat completed successfully.
echo ========================================================
popd
endlocal
exit /b 0
