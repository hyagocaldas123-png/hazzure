@echo off
setlocal
python "%~dp0offset_dumper.py"
if errorlevel 1 (
    echo [ERROR] Offset synchronization failed.
    exit /b 1
)
echo [OK] Offsets up to date.
