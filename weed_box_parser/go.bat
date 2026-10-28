
@echo off
cd /d "%~dp0"

cmake -S . -B build
if errorlevel 1 goto error

cmake --build build --parallel
if errorlevel 1 goto error

echo.
echo === RUNNING PROGRAM ===
set "PATH=C:\Users\fang\Desktop\study\libraryCPP\opencv\build\x64\vc16\bin;%PATH%"
".\build\Debug\weed_box_parser.exe"

echo.
echo === PROGRAM FINISHED ===
pause
exit /b 0

:error
echo.
echo === BUILD FAILED ===
pause
exit /b 1
