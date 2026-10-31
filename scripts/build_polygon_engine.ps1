$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $ProjectRoot

Write-Host ""
Write-Host "=== MAYHEM BUILDING FORGE | POLYGON ENGINE ===" -ForegroundColor Cyan
Write-Host "Project: $ProjectRoot" -ForegroundColor DarkGray

if (-not (Get-Command git -ErrorAction SilentlyContinue)) { throw "Git was not found. Install Git and reopen PowerShell." }
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { throw "CMake was not found. Install CMake 3.20+ and ensure it is on PATH." }

Write-Host "[1/5] Updating repository..." -ForegroundColor Yellow
git pull --ff-only
if ($LASTEXITCODE -ne 0) { throw "git pull failed." }

Write-Host "[2/5] Configuring CMake..." -ForegroundColor Yellow
cmake -S . -B build -A x64 -DMBF_BUILD_TESTS=ON -DMBF_BUILD_EXAMPLES=ON
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }

Write-Host "[3/5] Building..." -ForegroundColor Yellow
cmake --build build --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "Build failed." }

Write-Host "[4/5] Running automated tests..." -ForegroundColor Yellow
ctest --test-dir build -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "One or more tests failed." }

Write-Host "[5/5] Generating polygon gallery..." -ForegroundColor Yellow
$Demo = Join-Path $ProjectRoot "build\Release\mbf_polygon_demo.exe"
if (-not (Test-Path $Demo)) { throw "Polygon demo executable not found: $Demo" }
& $Demo
if ($LASTEXITCODE -ne 0) { throw "Polygon demo failed." }

$Output = Join-Path $ProjectRoot "mayhem_polygon_gallery.svg"
if (-not (Test-Path $Output)) { throw "Expected SVG output was not created." }
Write-Host "POLYGON ENGINE BUILD AND TEST COMPLETED." -ForegroundColor Green
Write-Host "SVG: $Output" -ForegroundColor Green
Start-Process $Output
