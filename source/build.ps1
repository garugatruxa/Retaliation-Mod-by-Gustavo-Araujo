# Uso: powershell -ExecutionPolicy Bypass -File build.ps1 -Version 1.03
#   gera Mod\Builds\Retaliation <versao>\  (pasta pronta pro modloader)
#   e guarda o codigo dessa versao em Mod\source\history\<versao>\
param(
    [Parameter(Mandatory = $true)][string]$Version,
    [string]$OutName = "Retaliation $Version",
    [string]$SrcDir = ""
)
$ErrorActionPreference = "Stop"
if (-not $SrcDir) { $SrcDir = $PSScriptRoot }
$root = Split-Path $PSScriptRoot
$out = Join-Path $root "Builds\$OutName"
New-Item -ItemType Directory -Force $out | Out-Null

& i686-w64-mingw32-g++ -std=c++17 -O2 -s -shared -static -static-libgcc -static-libstdc++ `
    -fno-exceptions -fno-rtti -Wall -Wno-unused-function `
    -o "$out\Retaliation.asi" "$SrcDir\Retaliation.cpp" -luser32 -lkernel32
if ($LASTEXITCODE -ne 0) { throw "compile failed" }

& python "$SrcDir\make_cs.py" "$out\Retaliation.cs"
if ($LASTEXITCODE -ne 0) { throw "cs build failed" }
if (Test-Path "$SrcDir\make_txd.py") {
    & python "$SrcDir\make_txd.py" "$out\retstar.txd"
    if ($LASTEXITCODE -ne 0) { throw "txd build failed" }
}

Copy-Item "$SrcDir\Retaliation.ini" "$out\Retaliation.ini" -Force
Copy-Item "$SrcDir\LEIAME.txt" "$out\LEIAME.txt" -Force

# preset folders (difficulty / performance) shipped next to the mod folder
if (Test-Path "$SrcDir\make_presets.py") {
    $presets = Join-Path $root "Builds\$OutName - Presets"
    if (Test-Path $presets) { Remove-Item -Recurse -Force $presets }
    & python "$SrcDir\make_presets.py" $presets
    if ($LASTEXITCODE -ne 0) { throw "presets build failed" }
}

# snapshot of the source used for this version
$hist = Join-Path $PSScriptRoot "history\$Version"
if ((Resolve-Path $SrcDir).Path -ne (New-Item -ItemType Directory -Force $hist).FullName) {
    Copy-Item "$SrcDir\Retaliation.cpp", "$SrcDir\make_cs.py", "$SrcDir\Retaliation.ini", "$SrcDir\LEIAME.txt" $hist -Force
    if (Test-Path "$SrcDir\make_txd.py") { Copy-Item "$SrcDir\make_txd.py" $hist -Force }
    if (Test-Path "$SrcDir\make_presets.py") { Copy-Item "$SrcDir\make_presets.py" $hist -Force }
}
Write-Host "Build OK -> $out"
