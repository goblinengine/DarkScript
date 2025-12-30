@echo off
echo DAScript Build Script (Windows)
echo ===================================
echo.

REM Directly call vcvarsall if we find VS
if exist "D:\Software\VisualStudio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    echo Setting up MSVC environment...
    call "D:\Software\VisualStudio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    goto build
)

if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    echo Setting up MSVC environment...
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    goto build
)

echo ERROR: Could not find Visual Studio
exit /b 1

:build
echo.
echo Verifying cl.exe...
where cl.exe
if errorlevel 1 (
    echo ERROR: cl.exe not found
    exit /b 1
)

echo.
echo Building with MSVC (use_mingw=no)
echo.

if "%1"=="" (
    scons platform=windows target=template_debug arch=x86_64 use_mingw=no
) else (
    scons platform=windows target=%1 arch=x86_64 use_mingw=no
)

if errorlevel 1 (
    echo Build FAILED
    exit /b 1
)

echo Build succeeded!
echo Output: addons\dascript\bin\
