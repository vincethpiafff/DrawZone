# check_psn_modes.ps1 - test of the Source IDs mode of the PSN output, with psn_sim.ps1 and augmenta_sim.ps1
#
#   powershell -ExecutionPolicy Bypass -File check_psn_modes.ps1
#
# Needs Chataigne running with a session made by make_psn_session.ps1 -OutputMode "Source IDs".
# Sends one PSN tracker to source "PSN Z" (Area -8, -2 / 16, 12) and one Augmenta person with the same id,
# reads the PSN output only (no MCP).

$psnSim = Join-Path $PSScriptRoot "psn_sim.ps1"
$augmentaSim = Join-Path $PSScriptRoot "augmenta_sim.ps1"
$capture = Join-Path $PSScriptRoot "psn_capture.ps1"

$results = New-Object System.Collections.Generic.List[object]
function Check([string]$name, [bool]$ok, [string]$got)
{
	$results.Add([pscustomobject]@{ Result = $(if ($ok) { "PASS" } else { "FAIL" }); Check = $name; Got = $got })
}
function Near($a, $b, [float]$tol = 0.02) { return [math]::Abs([float]$a - [float]$b) -le $tol }
function Near3($v, $x, $y, $z) { return $null -ne $v -and (Near $v[0] $x) -and (Near $v[1] $y) -and (Near $v[2] $z) }
function Sim([string]$file, [object[]]$simArgs) { return Start-Process powershell -ArgumentList (@("-ExecutionPolicy", "Bypass", "-File", $file) + $simArgs) -WindowStyle Hidden -PassThru }

#PSN Z : x = 2, y = 3 -> scene (2 + 8, -2 + 12 - 3) = (10, 7), height 1.7
$p = Sim $psnSim @("-Via", "127.0.0.1", "-Port", 56576, "-Id", 88, "-X", 2, "-Y", 3, "-Z", 1.7, "-Vx", 0.5, "-Vy", -0.25, "-Seconds", 7)
Start-Sleep -Milliseconds 1500
$o = & $capture -Via "127.0.0.1" -Seconds 2.2
Check "one tracker with the id of the source" (($o.ids -join ",") -eq "88") ($o.ids -join ",")
Check "position in the scene (10, 1.7, 7)" (Near3 $o.pos[88] 10 1.7 7) ($o.pos[88] -join ", ")
Check "speed in the scene (0.5, 0, 0.25)" (Near3 $o.speed[88] 0.5 0 0.25) ($o.speed[88] -join ", ")
Check "system and tracker names in the info packet" ($o.info -match "DrawZone PSN sim Sim") "'$($o.info)'"

$a = Sim $augmentaSim @("-Id", 88, "-X", 4, "-Y", 1, "-Seconds", 3)
Start-Sleep -Milliseconds 1500
$o = & $capture -Via "127.0.0.1"
Check "2 sources with the same id : 2 trackers" ($o.ids.Count -eq 2 -and $o.ids[0] -eq 88 -and $o.ids[1] -ge 10000) ($o.ids -join ",")
$a.WaitForExit(); $p.WaitForExit()

Start-Sleep -Milliseconds 1500
$o = & $capture -Via "127.0.0.1"
Check "nothing sent once they are gone" ($o.ids.Count -eq 0) ($o.ids -join ",")

$results | Format-Table -AutoSize -Wrap
$failed = @($results | Where-Object Result -eq "FAIL").Count
"$($results.Count - $failed) / $($results.Count) passed"
exit $failed
