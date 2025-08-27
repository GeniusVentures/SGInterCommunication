#!/usr/bin/env powershell

Write-Host "Setting up development environment with MSYS2 tools..." -ForegroundColor Green

# Add MSYS2 UCRT64 to PATH
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH

Write-Host "Development environment configured successfully!" -ForegroundColor Green
Write-Host ""

Write-Host "Available tools:" -ForegroundColor Yellow
Write-Host "- GCC:" -ForegroundColor Cyan
& gcc --version | Select-Object -First 1

Write-Host ""
Write-Host "- Clang:" -ForegroundColor Cyan  
& clang --version | Select-Object -First 1

Write-Host ""
Write-Host "- Clang-format:" -ForegroundColor Cyan
& clang-format --version

Write-Host ""
Write-Host "- Clang-tidy:" -ForegroundColor Cyan
& clang-tidy --version | Select-Object -First 1

Write-Host ""
Write-Host "Environment is ready for development!" -ForegroundColor Green
