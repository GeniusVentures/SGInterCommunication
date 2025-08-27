@echo off
echo Setting up development environment with MSYS2 tools...

REM Add MSYS2 UCRT64 to PATH
set PATH=C:\msys64\ucrt64\bin;%PATH%

echo Development environment configured successfully!
echo.
echo Available tools:
echo - GCC: 
gcc --version | head -1
echo.
echo - Clang:
clang --version | head -1
echo.
echo - Clang-format:
clang-format --version
echo.
echo - Clang-tidy:
clang-tidy --version | head -1
echo.
echo Environment is ready for development!
