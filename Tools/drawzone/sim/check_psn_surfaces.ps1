# check_psn_surfaces.ps1 - test of the PSN output (Player Slots) with 2 surfaces, with augmenta_sim.ps1
#
#   powershell -ExecutionPolicy Bypass -File check_psn_surfaces.ps1 [-MurSlotsOff]
#
# Needs Chataigne running with a session made by make_psn_session.ps1 -Surfaces [-MurSlotsOff].
# One person on SOL (Augmenta, id 3), one on MUR (Augmenta 2, port 12998, id 5 + Id Offset 1000, Offset 8, 0).
# Slots on : SOL slot 1 -> tracker 60001, MUR slot 1 -> tracker 60101. MUR slots off : its person keeps its id, 1005.
# Reads the PSN output only (no MCP).

param([switch]$MurSlotsOff)

$augmentaSim = Join-Path $PSScriptRoot "augmenta_sim.ps1"
$capture = Join-Path $PSScriptRoot "psn_capture.ps1"

. (Join-Path $PSScriptRoot "common.ps1")

$a = Sim $augmentaSim @("-Id", 3, "-X", 2, "-Y", 1, "-Seconds", 6)
$b = Sim $augmentaSim @("-Port", 12998, "-Id", 5, "-X", 3, "-Y", 2, "-SceneW", 8, "-SceneH", 3, "-Seconds", 6)
Start-Sleep -Milliseconds 2000
$o = & $capture -Via "127.0.0.1" -Seconds 2.5
$murId = if ($MurSlotsOff) { 1005 } else { 60101 }
$expected = if ($MurSlotsOff) { "1005,60001" } else { "60001,60101" }

Check "trackers $expected" (($o.ids -join ",") -eq $expected) ($o.ids -join ",")
Check "SOL : position (2, 1)" ($null -ne $o.pos[60001] -and (Near $o.pos[60001][0] 2) -and (Near $o.pos[60001][2] 1)) ($o.pos[60001] -join ", ")
Check "MUR : position in MUR meters (11, 2)" ($null -ne $o.pos[$murId] -and (Near $o.pos[$murId][0] 11) -and (Near $o.pos[$murId][2] 2)) ($o.pos[$murId] -join ", ")
Check "SOL : name JOUEUR 1 (SOL)" ($o.info -match "JOUEUR 1 \(SOL\)") "'$($o.info)'"
if ($MurSlotsOff) { Check "MUR : tracker name then (MUR), no slot" ($o.info -match "\(MUR\)" -and $o.info -notmatch "Slot 1 \(MUR\)") "'$($o.info)'" }
else { Check "MUR : name Slot 1 (MUR)" ($o.info -match "Slot 1 \(MUR\)") "'$($o.info)'" }

$a.WaitForExit(); $b.WaitForExit()

ShowResults
exit $script:failedChecks
