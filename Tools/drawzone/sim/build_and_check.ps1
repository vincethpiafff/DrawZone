# build_and_check.ps1 - close Chataigne, build, run run_checks.ps1 (which reopens the session)
#
#   powershell -ExecutionPolicy Bypass -File build_and_check.ps1
#
# Chataigne is closed only when its session has no unsaved change (read through MCP),
# so nothing is lost ; it reopens the last session by itself.

# -Rebuild : full rebuild, needed when a struct or class of a header changes its layout (fields added
# or removed) : the incremental build may not recompile every file using it (crash 0xc0000374, 29/10/2026)

param([switch]$Rebuild)

$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
$exe = Join-Path $root "Binaries\App\Chataigne.exe"
$msbuild = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"

$running = Get-Process Chataigne -ErrorAction SilentlyContinue
if ($running)
{
	$check = & (Join-Path $PSScriptRoot "check_sim.ps1") -ProjectOnly
	if ($check -ne "saved") { "STOP : the session has unsaved changes ($check), Chataigne not closed"; exit 2 }
	[void]$running.CloseMainWindow()
	if (-not $running.WaitForExit(15000)) { "STOP : Chataigne did not close"; exit 2 }
}

& $msbuild (Join-Path $root "Builds\VisualStudio2022\Chataigne.sln") /p:Configuration=Release /p:Platform=x64 /p:PreferredToolArchitecture=x64 /p:WholeProgramOptimization=false /p:LinkTimeCodeGeneration=Default /m /v:minimal /nologo $(if ($Rebuild) { "/t:Rebuild" }) | Select-String -Pattern "error|warning C|LNK"
if ($LASTEXITCODE -ne 0) { "STOP : build failed"; exit 1 }
"build OK"

& (Join-Path $PSScriptRoot "run_checks.ps1")
exit $LASTEXITCODE
