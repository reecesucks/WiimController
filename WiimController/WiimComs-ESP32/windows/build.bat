@echo off
setlocal

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe not found at "%VSWHERE%"
    exit /b 1
)

set "VSINSTALL="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do (
    set "VSINSTALL=%%i"
)
if not defined VSINSTALL (
    echo ERROR: no Visual Studio installation found via vswhere.
    exit /b 1
)

set "DEVCMD=%VSINSTALL%\Common7\Tools\VsDevCmd.bat"
if not exist "%DEVCMD%" (
    echo ERROR: VsDevCmd.bat not found at "%DEVCMD%".
    exit /b 1
)

call "%DEVCMD%" -arch=x64 -host_arch=x64 -no_logo
if errorlevel 1 exit /b 1

cd /d "%~dp0\.."

if not exist "build-win\CMakeCache.txt" (
    cmake -S windows -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Debug || exit /b 1
)

cmake --build build-win
exit /b %ERRORLEVEL%
