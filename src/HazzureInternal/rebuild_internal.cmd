@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
cl /I src /I ..\Compartilhado /I references\Aspasia\Includes /I references\Aspasia\Includes\kiero\minhook\include /nologo /std:c++17 /O2 /MT /EHsc /W4 /LD /DUSE_MINIMAL_BUILD src\Internal.cpp /Fo:build\Internal_new.obj