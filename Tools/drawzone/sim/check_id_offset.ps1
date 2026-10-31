# check_id_offset.ps1 - test of the Id Offset of the sources, with psn_sim.ps1 and augmenta_sim.ps1
#
#   powershell -ExecutionPolicy Bypass -File check_id_offset.ps1
#
# Needs Chataigne running with a session made by make_psn_session.ps1 -DefaultIds -OutputMode "Source IDs".
# Augmenta ids 0 and 1 (default Id Offset 101), PSN ids 1 and 2 (default 0), in both starting orders.
# Reads the PSN output only (no MCP).

$psnSim = Join-Path $PSScriptRoot "psn_sim.ps1"
$augmentaSim = Join-Path $PSScriptRoot "augmenta_sim.ps1"
$capture = Join-Path $PSScriptRoot "psn_capture.ps1"

$results = New-Object System.Collections.Generic.List[object]
function Check([string]$name, [bool]$ok, [string]$got)
{
	$results.Add([pscustomobject]@{ Result = $(if ($ok) { "PASS" } else { "FAIL" }); Check = $name; Got = $got })
}
function Sim([string]$file, [object[]]$simArgs) { return Start-Process powershell -ArgumentList (@("-ExecutionPolicy", "Bypass", "-File", $file) + $simArgs) -WindowStyle Hidden -PassThru }
function Augmenta([int]$id, [float]$seconds) { return Sim $augmentaSim @("-Id", $id, "-X", 4, "-Y", 1, "-Seconds", $seconds) }
#one sender per source : 2 senders on the same group and port would be mixed
function Psn([int]$id, [float]$seconds) { return Sim $psnSim @("-Port", (56574 + $id), "-Id", $id, "-X", 2, "-Y", 1.7, "-Z", 1.5, "-Seconds", $seconds) }

$expected = "1,2,101,102"

#Augmenta starts first
$a = Augmenta 0 7; $b = Augmenta 1 7
Start-Sleep -Milliseconds 1500
$o = & $capture -Via "127.0.0.1"
Check "Augmenta alone : 101, 102" (($o.ids -join ",") -eq "101,102") ($o.ids -join ",")
$c = Psn 1 3; $d = Psn 2 3
Start-Sleep -Milliseconds 1500
$o = & $capture -Via "127.0.0.1"
Check "Augmenta first : same ids" (($o.ids -join ",") -eq $expected) ($o.ids -join ",")
$a.WaitForExit(); $b.WaitForExit(); $c.WaitForExit(); $d.WaitForExit()
Start-Sleep -Milliseconds 1500

#PSN starts first
$c = Psn 1 7; $d = Psn 2 7
Start-Sleep -Milliseconds 1500
$a = Augmenta 0 3; $b = Augmenta 1 3
Start-Sleep -Milliseconds 1500
$o = & $capture -Via "127.0.0.1"
Check "PSN first : same ids" (($o.ids -join ",") -eq $expected) ($o.ids -join ",")
$a.WaitForExit(); $b.WaitForExit()

#the person comes back : same id, no counter going up
Start-Sleep -Milliseconds 1500
$a = Augmenta 1 3
Start-Sleep -Milliseconds 1500
$o = & $capture -Via "127.0.0.1"
Check "person back : same id" (($o.ids -join ",") -eq "1,2,102") ($o.ids -join ",")
$a.WaitForExit(); $c.WaitForExit(); $d.WaitForExit()

$results | Format-Table -AutoSize -Wrap
$failed = @($results | Where-Object Result -eq "FAIL").Count
"$($results.Count - $failed) / $($results.Count) passed"
exit $failed
