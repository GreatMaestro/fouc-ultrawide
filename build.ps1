# Builds build/FOUCUltrawide.asi (32-bit DLL) with an i686 MinGW-w64 GCC toolchain.
# Example: .\build.ps1 -Gxx "C:\mingw32\bin\g++.exe"
param(
    [string]$Gxx = "g++"
)

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
New-Item -ItemType Directory -Force build | Out-Null

& $Gxx -std=c++20 -O2 -masm=intel -s -static -shared `
    -o build/FOUCUltrawide.asi src/main.cpp `
    "-Wl,--exclude-all-symbols"

if ($LASTEXITCODE -ne 0) { throw "Build failed (exit code $LASTEXITCODE)" }
Get-Item build/FOUCUltrawide.asi | Select-Object Name, Length
