param(
  [int]$Port = 8765
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Path $PSScriptRoot -Parent
$uiDir = Join-Path $root "ui-tester"
$syncScript = Join-Path $PSScriptRoot "sync-ui-tester.py"

if (-not (Test-Path $uiDir)) {
  throw "ui-tester directory not found: $uiDir"
}

if (-not (Test-Path $syncScript)) {
  throw "UI tester sync script not found: $syncScript"
}

Write-Host "Syncing UI tester metadata from firmware renderer..."
& python $syncScript
if ($LASTEXITCODE -ne 0) {
  throw "UI tester sync failed with exit code $LASTEXITCODE"
}

Write-Host "Starting UI tester at http://localhost:$Port"
Write-Host "Press Ctrl+C to stop."
Set-Location $uiDir
python -m http.server $Port
