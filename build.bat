@echo off
REM SGInterCommunication Build Script for Windows

echo SGInterCommunication Build Script
echo ==================================

REM Check if cmake is available
where cmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo Error: CMake not found. Please install CMake and add it to PATH.
    pause
    exit /b 1
)

REM Check if ninja is available
where ninja >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo Warning: Ninja not found. Using default generator.
    set GENERATOR="Visual Studio 16 2019"
) else (
    set GENERATOR="Ninja"
)

REM Set build configuration
set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Debug

echo Build Type: %BUILD_TYPE%
echo Generator: %GENERATOR%

REM Create build directory
if not exist "build\Windows" mkdir "build\Windows"
if not exist "build\Windows\%BUILD_TYPE%" mkdir "build\Windows\%BUILD_TYPE%"

REM Navigate to build directory
cd "build\Windows\%BUILD_TYPE%"

REM Configure with CMake
echo Configuring project...
cmake ..\..\.. -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -G %GENERATOR%
if %ERRORLEVEL% neq 0 (
    echo Error: CMake configuration failed.
    cd ..\..\..
    pause
    exit /b 1
)

REM Build the project
echo Building project...
if "%GENERATOR%"=="Ninja" (
    ninja
) else (
    cmake --build . --config %BUILD_TYPE%
)

if %ERRORLEVEL% neq 0 (
    echo Error: Build failed.
    cd ..\..\..
    pause
    exit /b 1
)

echo Build completed successfully!
echo.
echo Executables are in: build\Windows\%BUILD_TYPE%\
echo.
echo To run tests: .\sgipc_test.exe
echo To run examples: .\simple_ipc_example.exe
echo.

cd ..\..\..
pause
