# check_sim.ps1 - non-regression test of the Cockpit module with the Augmenta simulator
#
#   powershell -ExecutionPolicy Bypass -File check_sim.ps1
#
# Needs Chataigne running with a copy of TEST_ZONES.noisette from make_psn_session.ps1 -NoPsn (see run_checks.ps1), the MCP server enabled (Inspect profile)
# and the token in the user variable CHATAIGNE_MCP_TOKEN. Read only : it never changes the session.
# For each scenario, a fake person (id 77) is sent at a known position, then read back
# (position, zone, relative position, slot), then it must disappear after /object/leave.

param(
	[string]$Endpoint = "http://127.0.0.1:42002/mcp",
	[switch]$ProjectOnly	# only print "saved" or "modified" (used before closing Chataigne)
)

$ErrorActionPreference = "Stop"
$sim = Join-Path $PSScriptRoot "augmenta_sim.ps1"
. (Join-Path $PSScriptRoot "common.ps1")
$project = ConnectMcp $Endpoint "drawzone-check"
if ($ProjectOnly) { if ($project.modified) { "modified" } else { "saved" }; exit 0 }

$person = "spatial/values/people/person77"

#the person is placed at the center of each zone of TEST_ZONES.noisette (read live, so moving a zone
#does not break the test) : relative position 0.5, 0.5 ; then outside any zone
$rect = Value "spatial/parameters/zones/zoneRECT/center"
$ellipse = Value "spatial/parameters/zones/zoneELIPSE/center"
$freeCenter = Value "spatial/parameters/zones/zone1/center"
$freeSize = Value "spatial/parameters/zones/zone1/size"
if ($null -eq $rect -or $null -eq $ellipse -or $null -eq $freeCenter) { "STOP : zones ZONE RECT / ZONE ELIPSE / Zone 1 not found (TEST_ZONES.noisette not loaded ?)"; exit 99 }
#free shape : a point well inside its 4 points (0.47, 0.5 in its box)
$freeX = [math]::Round($freeCenter[0] + (0.47 - 0.5) * $freeSize[0], 3)
$freeY = [math]::Round($freeCenter[1], 3)

# name, x, y, expected zone, expected relative position, zone values path (or "")
$scenarios = @(
	@("rect", [math]::Round($rect[0], 3), [math]::Round($rect[1], 3), "ZONE RECT", @(0.5, 0.5), "zoneRECT"),
	@("ellipse", [math]::Round($ellipse[0], 3), [math]::Round($ellipse[1], 3), "ZONE ELIPSE", @(0.5, 0.5), "zoneELIPSE"),
	@("free shape", $freeX, $freeY, "Zone 1", @(0.47, 0.5), "zone1"),
	@("outside", 4.0, 4.2, "", @(-1, -1), "")
)

$startTime = Get-Date

foreach ($s in $scenarios)
{
	$name, $x, $y, $zone, $rel, $zoneValues = $s
	$p = Start-Process powershell -ArgumentList "-ExecutionPolicy", "Bypass", "-File", $sim, "-X", $x, "-Y", $y, "-Seconds", 3 -WindowStyle Hidden -PassThru
	Start-Sleep -Milliseconds 1500

	$pos = Value "$person/position"
	Check "$name : position ($x, $y)" ($null -ne $pos -and (Near $pos[0] $x) -and (Near $pos[1] $y)) ($pos -join ", ")
	$z = Value "$person/zone"
	Check "$name : zone '$zone'" ($z -eq $zone) "'$z'"
	$r = Value "$person/relativePosition"
	Check "$name : relative position" ($null -ne $r -and (Near $r[0] $rel[0] 0.03) -and (Near $r[1] $rel[1] 0.03)) ($r -join ", ")
	$slot = Value "$person/slot"
	Check "$name : has a slot" ($slot -like "JOUEUR*") "'$slot'"
	if ($zoneValues)
	{
		#only the test person in the zone : its barycenter is the person
		$zc = Value "spatial/values/zones/$zoneValues/personCount"
		Check "$name : zone person count 1" ($zc -eq 1) "$zc"
		$avg = Value "spatial/values/zones/$zoneValues/averagePosition"
		Check "$name : zone average position" ($null -ne $avg -and (Near $avg[0] $rel[0] 0.03) -and (Near $avg[1] $rel[1] 0.03)) ($avg -join ", ")
	}

	$p.WaitForExit()
	Start-Sleep -Milliseconds 700
	$gone = Tool "organic_inspect" @{ handle = $modules; path = $person }
	Check "$name : removed after leave" ($null -eq $gone) $(if ($gone) { "still there" } else { "gone" })
}

#velocity with Flip off : as sent (m/s in scene axes)
$p = Start-Process powershell -ArgumentList "-ExecutionPolicy", "Bypass", "-File", $sim, "-X", 4, "-Y", 1, "-Vx", 0.8, "-Vy", -0.4, "-Seconds", 3 -WindowStyle Hidden -PassThru
Start-Sleep -Milliseconds 1500
$v = Value "$person/velocity"
Check "velocity (0.8, -0.4)" ($null -ne $v -and (Near $v[0] 0.8) -and (Near $v[1] -0.4)) ($v -join ", ")
$status = Value "spatial/parameters/sources/augmenta/status"
Check "source status receiving" ($status -match "^Receiving, \d+ msg/s, \d+ people, Augmenta scene") "'$status'"
#video output (Spout / NDI) : when one is on, frames are rendered and their size shown
$spoutOn = Value "spatial/parameters/surfaces/sol/videoOutput/spout"
$ndiOn = Value "spatial/parameters/surfaces/sol/videoOutput/ndi"
if ($spoutOn -or $ndiOn)
{
	$res = Value "spatial/parameters/surfaces/sol/videoOutput/resolution"
	Check "video output renders" ($res -match "^\d+ x \d+ px, 1 px = ") "'$res'"
}
$count = Value "spatial/values/personCount"
Check "person count >= 1" ($count -ge 1) "$count"
$p.WaitForExit()

#silent source : no leave, the person must go after the 1 s timeout
$p = Start-Process powershell -ArgumentList "-ExecutionPolicy", "Bypass", "-File", $sim, "-X", 4, "-Y", 1, "-Seconds", 1.5, "-NoLeave" -WindowStyle Hidden -PassThru
$p.WaitForExit()
Start-Sleep -Milliseconds 300
$still = Tool "organic_inspect" @{ handle = $modules; path = $person }
Check "no leave : still there right after" ($null -ne $still) $(if ($still) { "there" } else { "gone" })
Start-Sleep -Milliseconds 1500
$gone = Tool "organic_inspect" @{ handle = $modules; path = $person }
Check "no leave : removed after timeout" ($null -eq $gone) $(if ($gone) { "still there" } else { "gone" })

#a much smaller scene must not move or shrink the zones
$sizeBefore = Value "spatial/parameters/zones/zoneRECT/size"
$p = Start-Process powershell -ArgumentList "-ExecutionPolicy", "Bypass", "-File", $sim, "-X", 1, "-Y", 0.5, "-SceneW", 2, "-SceneH", 1, "-Seconds", 1.5 -WindowStyle Hidden -PassThru
$p.WaitForExit()
$c = Value "spatial/parameters/zones/zoneRECT/center"
$z = Value "spatial/parameters/zones/zoneRECT/size"
Check "small scene : zone keeps its place and size" ((Near $c[0] $rect[0] 0.001) -and (Near $c[1] $rect[1] 0.001) -and (Near $z[0] $sizeBefore[0] 0.001) -and (Near $z[1] $sizeBefore[1] 0.001)) (($c + $z) -join ", ")

#PSN output off by default (session without it) : a person is needed for it to have something to send
$p = Start-Process powershell -ArgumentList "-ExecutionPolicy", "Bypass", "-File", $sim, "-X", 4, "-Y", 1, "-Seconds", 2.5 -WindowStyle Hidden -PassThru
Start-Sleep -Milliseconds 800
$o = & (Join-Path $PSScriptRoot "psn_capture.ps1") -Via "127.0.0.1"
$p.WaitForExit()
$outStatus = Value "spatial/parameters/psnOutput/status"
Check "PSN output status Disabled" ($outStatus -eq "Disabled") "'$outStatus'"
Check "PSN output off by default : nothing sent" ($o.packets -eq 0) "$($o.packets) packets"

#2nd Fusion scene on its own port (make_psn_session.ps1 -NoPsn : Augmenta 2, port 12998, Id Offset 1000, Offset 8, 0)
if ($null -ne (Value "spatial/parameters/sources/augmenta2/port"))
{
	$p = Start-Process powershell -ArgumentList "-ExecutionPolicy", "Bypass", "-File", $sim, "-Port", 12998, "-Id", 5, "-X", 2, "-Y", 1, "-SceneW", 8, "-SceneH", 3, "-Seconds", 3 -WindowStyle Hidden -PassThru
	Start-Sleep -Milliseconds 1500
	$pos = Value "spatial/values/people/person1005/position"
	Check "2nd scene : person 1005 at (10, 1)" ($null -ne $pos -and (Near $pos[0] 10) -and (Near $pos[1] 1)) ($pos -join ", ")
	#Augmenta 2 feeds the surface MUR, which has its own slots (Slot 1...) and its own video output
	$surf = Value "spatial/values/people/person1005/surface"
	Check "2nd scene : person 1005 on surface MUR" ($surf -eq "MUR") "'$surf'"
	$slot = Value "spatial/values/people/person1005/slot"
	Check "2nd scene : slot of MUR (Slot 1)" ($slot -eq "Slot 1") "'$slot'"
	$slotId = Value "spatial/values/slots/mur/slot1/personId"
	Check "2nd scene : Values > Slots > MUR > Slot 1 = 1005" ($slotId -eq 1005) "$slotId"
	$solFree = Value "spatial/values/slots/sol/freeSlots"
	Check "2nd scene : SOL slots untouched (2 free)" ($solFree -eq 2) "$solFree"
	Check "2nd scene : MUR has its video output" ($null -ne (Value "spatial/parameters/surfaces/mur/videoOutput/spout")) ""
	$other = Tool "organic_inspect" @{ handle = $modules; path = "spatial/values/people/person5" }
	Check "2nd scene : not heard by the 1st source" ($null -eq $other) $(if ($other) { "person5 there" } else { "no person5" })
	$s2 = Value "spatial/parameters/sources/augmenta2/status"
	Check "2nd scene : status on port 12998" ($s2 -match "^Receiving, .*Augmenta scene 8.0 x 3.0 m, port 12998") "'$s2'"
	$s1 = Value "spatial/parameters/sources/augmenta/status"
	Check "1st scene : status module input" ($s1 -match "module input") "'$s1'"
	$p.WaitForExit()
	Start-Sleep -Milliseconds 700
	$gone = Tool "organic_inspect" @{ handle = $modules; path = "spatial/values/people/person1005" }
	Check "2nd scene : removed after leave" ($null -eq $gone) $(if ($gone) { "still there" } else { "gone" })
}
else { Check "2nd scene : source Augmenta 2 in the session" $false "not found" }

#no warning or error in the logs during the test
$diag = Tool "chataigne_diagnostics" @{ limit = 200 }
#the community module list and the online help come from the internet (and may fail to download), not from DrawZone
$bad = @($diag.logs | Where-Object { $_.severity -ne "info" -and [datetime]$_.time -ge $startTime -and $_.source -notin @("CommunityModuleManager", "HelpBox") })
Check "no warning / error in logs" ($bad.Count -eq 0) "$($bad.Count) messages"
$bad | ForEach-Object { "FAIL log, $($_.severity): $($_.message)" }

ShowResults
exit $script:failedChecks
