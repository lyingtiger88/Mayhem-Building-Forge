$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

$exe = Join-Path $repoRoot "build\Release\mbf_build_house.exe"
$topologyExe = Join-Path $repoRoot "build\Release\mbf_topology_preview.exe"
if ((-not (Test-Path $exe)) -or (-not (Test-Path $topologyExe))) {
    Write-Host "Native generator not found. Configuring and building Mayhem..." -ForegroundColor Yellow
    cmake -S . -B build
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }
    cmake --build build --config Release --parallel
    if ($LASTEXITCODE -ne 0) { throw "Mayhem build failed." }
}

$python = Get-Command py -ErrorAction SilentlyContinue
if ($python) {
    & py -3 scripts\mbf_gui_server.py --exe "build\Release\mbf_build_house.exe" --topology-exe "build\Release\mbf_topology_preview.exe"
} else {
    $python = Get-Command python -ErrorAction SilentlyContinue
    if (-not $python) { throw "Python 3 was not found. Install Python 3 or add it to PATH." }
    & python scripts\mbf_gui_server.py --exe "build\Release\mbf_build_house.exe" --topology-exe "build\Release\mbf_topology_preview.exe"
}
