param(
  [int]$Port = 8765
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Path $PSScriptRoot -Parent
$uiDir = Join-Path $root "ui-tester"

if (-not (Test-Path $uiDir)) {
  throw "ui-tester directory not found: $uiDir"
}

Write-Host "Starting UI tester at http://localhost:$Port"
Write-Host "Press Ctrl+C to stop."
Set-Location $uiDir
python -m http.server $Port
