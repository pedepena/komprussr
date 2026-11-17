@echo off
setlocal
title KOMPRUSSR - medir plugin
cd /d "%~dp0.."
echo ============================================
echo  KOMPRUSSR KU-01 - compilar y medir
echo  Carpeta del repo: %CD%
echo ============================================
echo.

if not exist "JUCE\CMakeLists.txt" goto nojuce

echo [1/5] Comprobando Python...
where python >nul 2>&1
if errorlevel 1 goto nopython

echo [2/5] Comprobando CMake...
where cmake >nul 2>&1
if not errorlevel 1 goto cmakeok
if exist "C:\Program Files\CMake\bin\cmake.exe" set "PATH=%PATH%;C:\Program Files\CMake\bin"
where cmake >nul 2>&1
if not errorlevel 1 goto cmakeok
echo CMake no esta instalado. Intentando instalarlo con winget...
winget install --id Kitware.CMake -e --accept-package-agreements --accept-source-agreements
if exist "C:\Program Files\CMake\bin\cmake.exe" set "PATH=%PATH%;C:\Program Files\CMake\bin"
where cmake >nul 2>&1
if errorlevel 1 goto nocmake
:cmakeok

echo [3/5] Comprobando herramientas de C++ de Visual Studio...
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSPATH="
if exist "%VSWHERE%" for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if defined VSPATH goto vsok
echo Faltan las herramientas de C++. Intentando instalarlas con winget...
echo (Pedira permiso de administrador y tarda bastante. Deja la ventana abierta.)
winget install --id Microsoft.VisualStudio.2022.BuildTools -e --accept-package-agreements --accept-source-agreements --override "--passive --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
if exist "%VSWHERE%" for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH goto novs
:vsok

echo [4/5] Compilando el plugin en modo Release (la primera vez tarda varios minutos)...
if not exist "build\CMakeCache.txt" cmake -B build
if errorlevel 1 goto builderror
cmake --build build --config Release
if errorlevel 1 goto builderror

echo [5/5] Instalando librerias de Python y midiendo...
python -m pip install --quiet --upgrade pedalboard numpy scipy matplotlib
if errorlevel 1 goto piperror
python tools\medir.py
if errorlevel 1 goto medirerror

echo.
echo ============================================
echo  TERMINADO. Resultados en: %CD%\resultados
echo ============================================
start "" "%CD%\resultados"
pause
exit /b 0

:nojuce
echo ERROR: no encuentro la carpeta JUCE dentro del repo (JUCE\CMakeLists.txt).
echo Copia o descarga JUCE en %CD%\JUCE y vuelve a ejecutar.
goto fin
:nopython
echo ERROR: Python no esta en el PATH. Reinstalalo marcando "Add Python to PATH".
goto fin
:nocmake
echo ERROR: no se pudo instalar CMake automaticamente.
echo Instalalo desde https://cmake.org/download marcando "Add CMake to the system PATH",
echo cierra esta ventana y vuelve a ejecutar el script.
goto fin
:novs
echo ERROR: no se pudieron instalar las herramientas de C++.
echo Instala "Build Tools para Visual Studio" con la carga "Desarrollo para el escritorio con C++",
echo cierra esta ventana y vuelve a ejecutar el script.
goto fin
:builderror
echo ERROR al compilar. Copia las ultimas 20 lineas de arriba y mandaselas a Claude.
goto fin
:piperror
echo ERROR instalando las librerias de Python. Mira el mensaje de arriba.
goto fin
:medirerror
echo ERROR midiendo el plugin. Copia las ultimas 20 lineas de arriba y mandaselas a Claude.
:fin
echo.
pause
exit /b 1