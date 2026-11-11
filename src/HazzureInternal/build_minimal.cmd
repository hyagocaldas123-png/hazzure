@echo off
setlocal
set "TASK_ROOT=%~dp0"
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 >nul
if errorlevel 1 exit /b 1
set "TASK_OUTPUT=%TASK_ROOT%build"
if not "%~1"=="" set "TASK_OUTPUT=%~f1"
if not exist "%TASK_OUTPUT%" mkdir "%TASK_OUTPUT%"
pushd "%TASK_OUTPUT%"

echo Compiling MinHook...
cl /nologo /O2 /MT /W4 /D_WIN64 /c "%TASK_ROOT%references\Aspasia\Includes\kiero\minhook\src\hook.c" "%TASK_ROOT%references\Aspasia\Includes\kiero\minhook\src\buffer.c" "%TASK_ROOT%references\Aspasia\Includes\kiero\minhook\src\trampoline.c" "%TASK_ROOT%references\Aspasia\Includes\kiero\minhook\src\hde\hde64.c"
if errorlevel 1 (popd & exit /b 1)

echo Compiling core gameplay DLL (minimal)...
cl /I "%~dp0..\Compartilhado" /I "%TASK_ROOT%src" /I "%TASK_ROOT%references\Aspasia\Includes" /I "%TASK_ROOT%references\Aspasia\Includes\kiero\minhook\include" /nologo /std:c++17 /O2 /MT /EHsc /W4 /LD ^
  "%TASK_ROOT%src\Internal.cpp" ^
  "%TASK_ROOT%src\kiero.cpp" ^
  hook.obj buffer.obj trampoline.obj hde64.obj ^
  /Fe:Hazzure.Internal.dll ^
  /DUSE_MINIMAL_BUILD
if errorlevel 1 (popd & exit /b 1)

echo Compiling self-test executable...
cl /I "%~dp0..\Compartilhado" /I "%TASK_ROOT%src" /I "%TASK_ROOT%references\Aspasia\Includes" /I "%TASK_ROOT%references\Aspasia\Includes\kiero\minhook\include" /nologo /std:c++17 /O2 /MT /EHsc /W4 /DINTERNAL_SELF_TEST /DUSE_MINIMAL_BUILD ^
  "%TASK_ROOT%src\Internal.cpp" ^
  "%TASK_ROOT%src\kiero.cpp" ^
  hook.obj buffer.obj trampoline.obj hde64.obj ^
  /Fe:Hazzure.Internal.Test.exe
if errorlevel 1 (popd & exit /b 1)

echo Running self-tests...
Hazzure.Internal.Test.exe
if errorlevel 1 (popd & exit /b 1)

echo Build successful!
popd
exit /b 0