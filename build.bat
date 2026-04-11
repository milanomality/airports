@echo off
setlocal enabledelayedexpansion

echo ========================================
echo   Airports — Qt6/C++ Build Script
echo ========================================
echo.

:: --- Настройки ---------------------------------------------------------------
set QT_DIR=D:\Qt\6.11.0\mingw_64
set MINGW_DIR=D:\Qt\Tools\mingw1310_64\bin
set CMAKE_BIN=C:\Program Files\CMake\bin

set PATH=%CMAKE_BIN%;%MINGW_DIR%;%QT_DIR%\bin;%PATH%

:: --- Проверки ----------------------------------------------------------------
where cmake  >nul 2>&1 || (echo [ERROR] cmake не найден    & pause & exit /b 1)
where g++    >nul 2>&1 || (echo [ERROR] g++ не найден      & pause & exit /b 1)
where windeployqt >nul 2>&1 || echo [WARN] windeployqt не найден

:: --- CMake -------------------------------------------------------------------
echo [1/4] CMake configure...
if not exist build rmdir /s /q build 2>nul
mkdir build
cd /d "%~dp0build"

cmake -G "MinGW Makefiles" ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DCMAKE_CXX_COMPILER="%MINGW_DIR%\g++.exe" ^
    -DCMAKE_MAKE_PROGRAM="%MINGW_DIR%\mingw32-make.exe" ^
    -DCMAKE_BUILD_TYPE=Release ^
    ..
if errorlevel 1 ( cd /d "%~dp0" & echo [ERROR] CMake configure failed & pause & exit /b 1 )

echo [2/4] Compiling...
mingw32-make -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 ( cd /d "%~dp0" & echo [ERROR] Compilation failed & pause & exit /b 1 )

cd /d "%~dp0"

:: --- Деплой ------------------------------------------------------------------
echo [3/4] Deploying Qt DLLs...
if exist "%QT_DIR%\bin\windeployqt.exe" (
    windeployqt --no-translations --no-opengl-sw --release "build\airports.exe" 2>nul
) else (
    for %%f in (Qt6Core Qt6Gui Qt6Widgets) do copy /y "%QT_DIR%\bin\%%f*.dll" "build\" >nul
    mkdir "build\platforms" 2>nul
    copy /y "%QT_DIR%\plugins\platforms\qwindows.dll" "build\platforms\" >nul
    for %%f in (libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll) do (
        copy /y "%MINGW_DIR%\%%f" "build\" >nul 2>nul
    )
)

copy /y "res\airports.csv" "build\" >nul

echo [4/4] Running tests...
cd /d "%~dp0build"
ctest --output-on-failure
set TEST_RESULT=%errorlevel%
cd /d "%~dp0"

echo.
echo ========================================
echo   Готово!
echo   Приложение:  build\airports.exe
echo   Тесты:       build\airports_tests.exe
echo ========================================
if %TEST_RESULT% neq 0 echo   ВНИМАНИЕ: Тесты не прошли!
pause
if %TEST_RESULT% neq 0 exit /b 1
