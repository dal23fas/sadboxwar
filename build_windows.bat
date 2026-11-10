@echo off
rem EARTH FRONTIER — builds ef_server.exe (and ef_headless.exe) on Windows with a portable GCC, no Visual Studio, no CMake.
rem
rem 1) Download w64devkit (portable, ~100 MB, no installer): https://github.com/skeeto/w64devkit/releases  (w64devkit-x64-*.zip)
rem 2) Unzip it anywhere, e.g. C:\w64devkit
rem 3) Double-click this file, or run it from any command prompt:  build_windows.bat C:\w64devkit
rem
setlocal enabledelayedexpansion
set KIT=%~1
if "%KIT%"=="" set KIT=C:\w64devkit
if exist "%KIT%\bin\g++.exe" set "PATH=%KIT%\bin;%PATH%"
where g++ >nul 2>&1
if errorlevel 1 (
  echo [build] g++ not found. Unzip w64devkit to C:\w64devkit or pass its folder: build_windows.bat C:\path\to\w64devkit
  pause
  exit /b 1
)
cd /d "%~dp0"
if not exist build mkdir build
echo [build] compiling EFWorldCore + ef_server (this takes about a minute)...
g++ -std=c++17 -O2 -fno-exceptions -fno-rtti -static -I Source\EFWorldCore\Public Source\EFWorldCore\Private\*.cpp Tools\ef_server\main.cpp -lws2_32 -o build\ef_server.exe
if errorlevel 1 (
  echo [build] FAILED — copy the text above and send it.
  pause
  exit /b 1
)
echo [build] compiling ef_headless (acceptance script)...
g++ -std=c++17 -O2 -fno-exceptions -fno-rtti -static -I Source\EFWorldCore\Public Source\EFWorldCore\Private\*.cpp Tools\ef_headless\main.cpp -o build\ef_headless.exe
echo.
echo [build] OK: build\ef_server.exe  build\ef_headless.exe
echo [build] Starting the server now. Keep this window open, then Play in Godot (F5 inside the game to reconnect).
echo.
build\ef_server.exe --port 7777 --save-dir Saved
pause
