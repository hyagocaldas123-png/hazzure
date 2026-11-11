@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 >nul
if errorlevel 1 exit /b 1
pushd "%~dp0"
if not exist out mkdir out
cl /nologo /std:c++17 /O2 /MT /EHsc /W4 /D_UNICODE /D_UNICODE /I "..\..\Compartilhado" OrbPreview.cpp /Fo:out\ /Fe:out\OrbPreview.exe
if errorlevel 1 (popd & exit /b 1)
"%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe" /nologo /optimize+ /platform:x64 /r:System.Drawing.dll /out:out\ToPng.exe ToPng.cs
if errorlevel 1 (popd & exit /b 1)
popd
pushd "%~dp0out"
OrbPreview.exe
if errorlevel 1 (popd & exit /b 1)
ToPng.exe orb-sky.bgra orb-spin.bgra orb-face.bgra orb-bite.bgra orb-edge.bgra orb-dim.bgra
set "TASK_RESULT=%ERRORLEVEL%"
popd
exit /b %TASK_RESULT%
