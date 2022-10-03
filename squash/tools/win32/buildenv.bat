for /f "usebackq tokens=*" %%i in (`tools\win32\vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (set VSBaseInstallDir=%%i)
call "%VSBaseInstallDir%\Common7\Tools\VsMSBuildCmd.bat"
exit /B 0