@echo off
setlocal
cd /d "%~dp0"

echo [FDE] Full build - configure + build...
echo.

if not exist build mkdir build
cd build

echo [1/2] Configuring CMake...
cmake .. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo.
    echo [FDE] CMake configure failed.
    PAUSE
    exit /b 1
)

echo.
echo [2/2] Building Debug...
cmake --build . --config Debug
if errorlevel 1 (
    echo.
    echo [FDE] Build failed.
    PAUSE
    exit /b 1
)

echo.
echo [FDE] Build succeeded: bin\DEBUG\FordEngine.dll, bin\DEBUG\FordEditor.exe
exit /b 0
