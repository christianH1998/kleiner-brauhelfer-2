@echo off
rem Baut die Core-Lib (eigene Kopie) und kbh-dump.
rem kbh-dump.exe landet in build-win64\bin (dort liegen die Qt-DLLs von build_win64.cmd).
rem Voraussetzung: build_win64.cmd wurde mindestens einmal ausgefuehrt.
rem Aufruf: tools\regression\build.cmd C:\Qt\6.11.0\msvc2022_64\bin

set QT_DIR=%1
set QT_DIR=%QT_DIR:"=%
if "%QT_DIR%" equ "" (
  echo Usage: build.cmd ^<qt-bin-directory^>
  exit /b -1
)

set ROOT=%~dp0..\..
set BUILD_DIR=%ROOT%\build-win64\tools-regression

for /f "usebackq delims=#" %%a in (`"%PROGRAMFILES(x86)%\Microsoft Visual Studio\Installer\vswhere" -products * -latest -property installationPath`) do set VSPATH=%%a
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1

if not exist "%BUILD_DIR%\core" mkdir "%BUILD_DIR%\core"
if not exist "%BUILD_DIR%\dump" mkdir "%BUILD_DIR%\dump"

pushd "%BUILD_DIR%\core"
"%QT_DIR%\qmake.exe" "%ROOT%\kleiner-brauhelfer-core\kleiner-brauhelfer-core.pro" -config release || (popd & exit /b 1)
nmake || (popd & exit /b 1)
popd

pushd "%BUILD_DIR%\dump"
"%QT_DIR%\qmake.exe" "%~dp0kbh-dump.pro" -config release || (popd & exit /b 1)
nmake || (popd & exit /b 1)
popd
