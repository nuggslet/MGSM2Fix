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

MSBuild MGSM2Fix.sln /p:Configuration=Release /p:Platform=x86 /t:Clean
MSBuild MGSM2Fix.sln /p:Configuration=Release /p:Platform=x64 /t:Clean

MSBuild MGSM2Fix.sln /p:Configuration=Release /p:Platform=x86
MSBuild MGSM2Fix.sln /p:Configuration=Release /p:Platform=x64

rd /S /Q dist\
md dist\

xcopy /I /Y res\ dist\
xcopy /I /Y asiloader\ dist\
echo F | xcopy /Y MGSM2Fix.ini dist\MGSM2Fix.ini
echo F | xcopy /Y Release\MGSM2Fix.asi dist\MGSM2Fix32.asi
echo F | xcopy /Y x64\Release\MGSM2Fix.asi dist\MGSM2Fix64.asi

del /S /Q MGSM2Fix.zip

powershell -command "Compress-Archive -Path 'dist\*' -DestinationPath 'MGSM2Fix.zip'"
