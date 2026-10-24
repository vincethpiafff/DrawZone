@echo off
rem ===========================================================================
rem  DrawZone - setup_windows.bat
rem
rem  Prepares a fresh clone for building and running on Windows:
rem    1. WebView2 NuGet package for the VS2022 project (needed to compile)
rem    2. Chataigne dependency pack (DLLs needed to run the app)
rem    3. Extracts the DLLs next to Binaries\App\Chataigne.exe
rem
rem  Safe to run several times: each step is skipped when already done.
rem  Everything it creates is ignored by Git.
rem  See docs\drawzone\BUILDING.md
rem ===========================================================================
setlocal

set "ROOT=%~dp0..\.."
cd /d "%ROOT%"

set "PKG_SRC=Builds\VisualStudio2022_CI\packages"
set "PKG_DST=Builds\VisualStudio2022\packages"
set "DEP_DIR=dependencies_zips"
set "DEP_ZIP=%DEP_DIR%\Chataigne-win-x64-release-dependencies.zip"
set "DEP_URL=https://benjamin.kuperberg.fr/chataigne/user/data/Chataigne-win-x64-release-dependencies.zip"
set "APP_DIR=Binaries\App"

echo.
echo [1/3] WebView2 package
if exist "%PKG_DST%\Microsoft.Web.WebView2.1.0.902.49\build\native\include\WebView2.h" (
    echo       already installed
) else (
    if not exist "%PKG_SRC%" (
        echo       ERROR: %PKG_SRC% not found
        exit /b 1
    )
    xcopy /E /I /Y /Q "%PKG_SRC%" "%PKG_DST%" >nul
    if errorlevel 1 (
        echo       ERROR: copy failed
        exit /b 1
    )
    echo       copied to %PKG_DST%
)

echo.
echo [2/3] Dependency pack
if exist "%DEP_ZIP%" (
    echo       already downloaded
) else (
    if not exist "%DEP_DIR%" mkdir "%DEP_DIR%"
    echo       downloading from %DEP_URL%
    curl -L --fail -o "%DEP_ZIP%" "%DEP_URL%"
    if errorlevel 1 (
        echo       ERROR: download failed
        if exist "%DEP_ZIP%" del "%DEP_ZIP%"
        exit /b 1
    )
)

echo.
echo [3/3] DLLs next to the app
if exist "%APP_DIR%\JoyShockLibrary.dll" (
    echo       already extracted
) else (
    if not exist "%APP_DIR%" mkdir "%APP_DIR%"
    tar -xf "%DEP_ZIP%" -C "%APP_DIR%"
    if errorlevel 1 (
        echo       ERROR: extraction failed
        exit /b 1
    )
    echo       extracted to %APP_DIR%
)

echo.
echo Setup done. Next: build_dev.bat
exit /b 0
