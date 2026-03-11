@echo off
setlocal
cd /d "%~dp0"

if not exist build (
    echo [FDE] No build directory. Run build_all.bat first.
    PAUSE
    exit /b 1
)

echo [FDE] Quick build (incremental)...
cmake --build build --config Debug
if errorlevel 1 (
    echo.
    echo [FDE] Build failed.
    PAUSE
    exit /b 1
)

echo.
echo [FDE] Build succeeded.
exit /b 0
