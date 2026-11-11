@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
cl /nologo /std:c++17 /O2 /MT /EHsc /W4 test_gameplay_compile.cpp /Fe:test_gameplay.exe