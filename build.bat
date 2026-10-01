@echo off
setlocal
cd /d "%~dp0" || exit /b 1

REM Add one row per Python version: version|python.exe|Include directory|pythonXY.lib.
set "OPENSEES_PYTHON_311=3.11|D:\Python311\python.exe|D:\Python311\Include|D:\Python311\libs\python311.lib"
set "OPENSEES_PYTHON_312=3.12|D:\Python312\python.exe|D:\Python312\Include|D:\Python312\libs\python312.lib"
set "OPENSEES_PYTHON_313=3.13|D:\Python313\python.exe|D:\Python313\Include|D:\Python313\libs\python313.lib"
set "OPENSEES_PYTHON_314=3.14|D:\Python314\python.exe|D:\Python314\Include|D:\Python314\libs\python314.lib"

call "D:\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" || exit /b 1
call "D:\oneAPI\setvars.bat" intel64 mod || exit /b 1

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0multi_python_build.ps1" %*
exit /b %ERRORLEVEL%
