# run_checks.ps1 - runs check_psn.ps1 and check_sim.ps1 on copies of the session, then reopens the session
#
#   powershell -ExecutionPolicy Bypass -File run_checks.ps1
#
# The copies (make_psn_session.ps1) listen on ports no real system uses, so the checks stay valid
# while Kratos or Augmenta Fusion are sending. Chataigne is closed only when its session has no
# unsaved change.

param([string]$Session = "D:\0-DRAWZONE\ChataigneProject\TEST_ZONES.noisette")

$ErrorActionPreference = "Stop"
$checkSim = Join-Path $PSScriptRoot "check_sim.ps1"
$checkPsn = Join-Path $PSScriptRoot "check_psn.ps1"
$checkModes = Join-Path $PSScriptRoot "check_psn_modes.ps1"
$checkIds = Join-Path $PSScriptRoot "check_id_offset.ps1"
$checkSurfaces = Join-Path $PSScriptRoot "check_psn_surfaces.ps1"
$make = Join-Path $PSScriptRoot "make_psn_session.ps1"
$dir = Join-Path $env:TEMP "drawzone_checks"
[void](New-Item -ItemType Directory -Force $dir)

. (Join-Path $PSScriptRoot "common.ps1")	# CloseChataigne, OpenSession

function Summary([string]$output) { ($output -split "`n") | Where-Object { $_ -match "FAIL|passed|STOP" } }

$failed = 0
try
{
	& $make -From $Session -To "$dir\psn.noisette" | Out-Null
	& $make -From $Session -To "$dir\psn_no_choice.noisette" -NoChoice | Out-Null
	& $make -From $Session -To "$dir\no_psn.noisette" -NoPsn | Out-Null
	& $make -From $Session -To "$dir\default_ids.noisette" -DefaultIds -OutputMode "Source IDs" | Out-Null
	& $make -From $Session -To "$dir\source_ids.noisette" -OutputMode "Source IDs" | Out-Null
	& $make -From $Session -To "$dir\surfaces.noisette" -Surfaces | Out-Null
	& $make -From $Session -To "$dir\surfaces_mur_off.noisette" -Surfaces -MurSlotsOff | Out-Null

	OpenSession "$dir\psn.noisette"
	"== PSN, one source marked Use As Surface"
	Summary (& $checkPsn | Out-String -Width 220)
	$failed += $LASTEXITCODE

	OpenSession "$dir\psn_no_choice.noisette"
	"== PSN, no source marked Use As Surface"
	Summary (& $checkPsn -NoChoice | Out-String -Width 220)
	$failed += $LASTEXITCODE

	OpenSession "$dir\source_ids.noisette"
	"== PSN output, Source IDs"
	Summary (& $checkModes | Out-String -Width 220)
	$failed += $LASTEXITCODE

	OpenSession "$dir\default_ids.noisette"
	"== Id Offset, default values"
	Summary (& $checkIds | Out-String -Width 220)
	$failed += $LASTEXITCODE

	OpenSession "$dir\surfaces.noisette"
	"== PSN, 2 surfaces, slots on"
	Summary (& $checkSurfaces | Out-String -Width 220)
	$failed += $LASTEXITCODE

	OpenSession "$dir\surfaces_mur_off.noisette"
	"== PSN, 2 surfaces, MUR slots off"
	Summary (& $checkSurfaces -MurSlotsOff | Out-String -Width 220)
	$failed += $LASTEXITCODE

	OpenSession "$dir\no_psn.noisette"
	"== Augmenta"
	Summary (& $checkSim | Out-String -Width 220)
	$failed += $LASTEXITCODE
}
finally
{
	OpenSession $Session
	"== session reopened : $(& $checkSim -ProjectOnly)"
}
exit $failed
