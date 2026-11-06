# check_logic.ps1 - test of the Logic Graph event nodes (v0.7.2 brick 3) with the Augmenta simulator
#
#   powershell -ExecutionPolicy Bypass -File check_logic.ps1
#
# Writes a copy of the session (make_psn_session.ps1 -NoPsn) with a graph "Test" : each of the 15 event nodes is
# linked to a Log node writing a known word (ZIN, PRES_ON...) with a value. Opens it, sends 2 fake people (77 then 78)
# into ZONE RECT (Capacity 2), reads the logs back through MCP (read only), then reopens the session.

param([string]$Session = "D:\0-DRAWZONE\ChataigneProject\TEST_ZONES.noisette")

$ErrorActionPreference = "Stop"
$sim = Join-Path $PSScriptRoot "augmenta_sim.ps1"
. (Join-Path $PSScriptRoot "common.ps1")
$dir = Join-Path $env:TEMP "drawzone_checks"
[void](New-Item -ItemType Directory -Force $dir)
$copy = "$dir\logic.noisette"

# --- session : graph Test

function P([string]$address, $value) { return @{ controlAddress = $address; value = $value } }
$nodes = New-Object System.Collections.Generic.List[object]
$links = New-Object System.Collections.Generic.List[object]
$y = 0
function Node([string]$uid, [string]$type, [object[]]$params, [float]$x)
{
	$ps = @((P "/uid" $uid), (P "/viewUIPosition" @($x, $script:y))) + $params
	$script:nodes.Add(@{ niceName = "$type $uid"; type = $type; parameters = $ps })
}
#event node uid, with a Log on each exec output : @(exec pin, word, data pin [, word 2, data pin 2]) ; a word 2 adds a 2nd Log after the 1st
function Event([string]$uid, [string]$type, [object[]]$params, [object[]]$outs)
{
	Node $uid $type $params 0
	foreach ($o in $outs)
	{
		$log = "$uid-$($o[1])"
		Node $log "Log" @(P "/message" $o[1]) 360
		$script:links.Add(@($uid, $o[0], $log, "In"))
		if ($o[2]) { $script:links.Add(@($uid, $o[2], $log, "Value")) }
		if ($o.Count -gt 3)
		{
			$log2 = "$uid-$($o[3])"
			Node $log2 "Log" @(P "/message" $o[3]) 720
			$script:links.Add(@($log, "Then", $log2, "In"))
			$script:links.Add(@($uid, $o[4], $log2, "Value"))
		}
		$script:y += 90
	}
}
$zone = P "/zone" "/zoneRECT"
#Time
Event "start" "On Start" @() @(, @("Then", "START", $null))
Event "timer" "Timer" @(P "/interval" 1.0) @(, @("Then", "TICK", "Count"))
#Zones
Event "zin" "Zone Entered" @($zone) @(, @("Then", "ZIN", "Person Id"))
Event "zout" "Zone Exited" @($zone) @(, @("Then", "ZOUT", "Person Id"))
Event "pres" "Zone Presence" @($zone) @(@("On", "PRES_ON", "State"), @("Off", "PRES_OFF", "State"))
Event "full" "Zone Full" @($zone) @(@("On", "FULL_ON", $null), @("Off", "FULL_OFF", $null))
Event "zpc" "Zone Person Count" @($zone) @(, @("Changed", "ZPC", "Count"))
#Slots (first surface SOL : 2 slots, Next Person Takes Slot)
Event "sjoin" "Slot Joined" @() @(, @("Then", "SJOIN", "Slot", "SJOIN_ID", "Person Id"))
Event "sleft" "Slot Left" @() @(, @("Then", "SLEFT", "Slot", "SLEFT_ID", "Person Id"))
Event "socc" "Slot Occupied" @(P "/slot" 1) @(@("On", "SOCC_ON", $null), @("Off", "SOCC_OFF", $null))
Event "sheld" "Slot Held" @(P "/slot" 1) @(@("On", "SHELD_ON", $null), @("Off", "SHELD_OFF", $null))
Event "free" "Free Slots" @() @(, @("Changed", "FREE", "Count"))
#People
Event "pin" "Person Appeared" @() @(, @("Then", "PIN", "Person Id"))
Event "pout" "Person Lost" @() @(, @("Then", "POUT", "Person Id"))
Event "pc" "Person Count" @() @(, @("Changed", "PC", "Count"))
#relative positions (brick 4b) : slot 1 = person 77, at the centre of ZONE RECT
Event "sper" "Slot Person" @(P "/slot" 1) @(, @("Changed", "SPER", "Person Id"))
Event "srel" "Slot Relative Position" @(P "/slot" 1) @(, @("Changed", "SREL_ZX", "Zone X", "SREL_SX", "Surface X"))
Event "prel" "Person Relative Position" @(P "/personId" 77) @(, @("Changed", "PREL_ZY", "Zone Y", "PREL_SY", "Surface Y"))
Event "prelz" "Person Relative Position" @(P "/personId" 77) @(, @("Changed", "PREL_ZN", "Zone"))

$graph = @{ niceName = "Test"; type = "Graph"; containers = @{ nodes = @{ items = $nodes } }; links = $links }
$graphJson = @{ items = @($graph) } | ConvertTo-Json -Depth 20 -Compress

& (Join-Path $PSScriptRoot "make_psn_session.ps1") -From $Session -To $copy -NoPsn | Out-Null
$text = [System.IO.File]::ReadAllText($copy)
$pattern = '(?s)"logic": \{.*?\}, "scene": \{'
if (-not [regex]::IsMatch($text, $pattern)) { throw "Logic block not found in $copy" }
$text = [regex]::Replace($text, $pattern, { param($m) '"logic": ' + $graphJson + ', "scene": {' })
[System.IO.File]::WriteAllText($copy, $text)

# --- run

OpenSession $copy
$project = ConnectMcp
$startTime = (Get-Date).AddSeconds(-30)	# On Start fires at load, before the MCP connection

$rect = Value "spatial/parameters/zones/zoneRECT/center"
if ($null -eq $rect) { "STOP : ZONE RECT not found"; exit 99 }
$x = [math]::Round($rect[0], 3); $yy = [math]::Round($rect[1], 3)

#77 : 5 s in the zone ; 78 : from 1.5 s to 3.5 s, so the zone is full (Capacity 2) for 2 s
$p77 = Sim $sim @("-Id", "77", "-X", "$x", "-Y", "$yy", "-Seconds", "5")
Start-Sleep -Milliseconds 1500
$p78 = Sim $sim @("-Id", "78", "-X", "$x", "-Y", "$([math]::Round($yy + 0.2, 3))", "-Seconds", "2")
$p78.WaitForExit(); $p77.WaitForExit()
Start-Sleep -Milliseconds 1500

$diag = Tool "chataigne_diagnostics" @{ limit = 200 }
$logs = @($diag.logs | Where-Object { [datetime]$_.time -ge $startTime } | Sort-Object { [datetime]$_.time })
$test = @($logs | Where-Object { $_.source -eq "Test" } | ForEach-Object { $_.message.Trim() })
"Logs of the graph :"
$test | ForEach-Object { "  $_" }

function Count([string]$m) { return @($test | Where-Object { $_ -eq $m }).Count }
function Index([string]$m) { return [array]::IndexOf($test, $m) }

Check "On Start : once" ((Count "START") -eq 1) "$(Count 'START')"
$ticks = @($test | Where-Object { $_ -like "TICK *" })
Check "Timer : ticks, Count goes up" ($ticks.Count -ge 5 -and $ticks[-1] -eq "TICK $($ticks.Count)") "$($ticks.Count) ticks, last $($ticks[-1])"
#77 alone (0 s), 78 joins (1.5 s), 78 lost (3.5 s), 77 lost (5 s)
$once = @("ZIN 77", "ZIN 78", "ZOUT 78", "ZOUT 77", "PRES_ON 1", "PRES_OFF 0", "FULL_ON", "FULL_OFF", "ZPC 2", "ZPC 0",
	"SJOIN 1", "SJOIN_ID 77", "SJOIN 2", "SJOIN_ID 78", "SLEFT 2", "SLEFT_ID 78", "SLEFT 1", "SLEFT_ID 77", "SOCC_ON", "SOCC_OFF",
	"FREE 0", "FREE 2", "PIN 77", "PIN 78", "POUT 78", "POUT 77", "PC 2", "PC 0")
foreach ($m in $once) { Check "$m : once" ((Count $m) -eq 1) "$(Count $m)" }
foreach ($m in @("ZPC 1", "FREE 1", "PC 1")) { Check "$m : twice (77 in, 78 out)" ((Count $m) -eq 2) "$(Count $m)" }
Check "Slot Held : never (Next Person Takes Slot)" (@($test | Where-Object { $_ -like "SHELD*" }).Count -eq 0) ""
#events of one same update share their log time : only moments 1 s apart are compared
Check "Zone : 77 in < 78 in < 78 out < 77 out" ((Index "ZIN 77") -lt (Index "ZIN 78") -and (Index "ZIN 78") -lt (Index "ZOUT 78") -and (Index "ZOUT 78") -lt (Index "ZOUT 77")) ""
Check "Zone Presence : On before Off" ((Index "PRES_ON 1") -ge 0 -and (Index "PRES_ON 1") -lt (Index "PRES_OFF 0")) ""
Check "Zone Full : On with 78, Off when 78 leaves" ((Index "ZIN 77") -lt (Index "FULL_ON") -and (Index "FULL_ON") -lt (Index "FULL_OFF") -and (Index "FULL_OFF") -lt (Index "PRES_OFF 0")) ""
Check "Slots : 2 joined < 2 left < 1 left" ((Index "SJOIN 2") -ge 0 -and (Index "SJOIN 2") -lt (Index "SLEFT 2") -and (Index "SLEFT 2") -lt (Index "SLEFT 1")) ""
Check "People : 77 in < 78 in < 78 out < 77 out" ((Index "PIN 77") -lt (Index "PIN 78") -and (Index "PIN 78") -lt (Index "POUT 78") -and (Index "POUT 78") -lt (Index "POUT 77")) ""

#relative positions : last value logged, 77 at the centre of ZONE RECT, surface SOL
function LastValue([string]$word) { $l = @($test | Where-Object { $_ -like "$word *" }); if ($l.Count -eq 0) { return $null }; return [double]::Parse(($l[-1] -split " ")[1], [Globalization.CultureInfo]::InvariantCulture) }
#Size Mode Auto : SOL takes the /scene of the simulator (augmenta_sim.ps1 : 8 x 4.5 m), Manual : its Size
$sol = if ((Value "spatial/parameters/surfaces/sol/sizeMode") -eq "Manual") { Value "spatial/parameters/surfaces/sol/size" } else { @(8.0, 4.5) }
$sx = if ($sol) { $x / $sol[0] } else { -1 }
$sy = if ($sol) { $yy / $sol[1] } else { -1 }
Check "Slot Person : 77 then -1" ((Index "SPER 77") -ge 0 -and (Index "SPER 77") -lt [array]::LastIndexOf($test, "SPER -1")) ""
Check "Zone output : PREL_ZN ZONE RECT while 77 is inside" (@($test | Where-Object { $_ -eq "PREL_ZN ZONE RECT" }).Count -ge 1) "$(@($test | Where-Object { $_ -like 'PREL_ZN*' }) -join ', ')"
foreach ($c in @(@("SREL_ZX", 0.5), @("SREL_SX", $sx), @("PREL_ZY", 0.5), @("PREL_SY", $sy)))
{
	$v = LastValue $c[0]
	Check "$($c[0]) = $([math]::Round($c[1], 3))" ($null -ne $v -and (Near $v $c[1] 0.03)) "$v"
}

$bad = @($logs | Where-Object { $_.severity -ne "info" -and $_.source -notin @("CommunityModuleManager", "HelpBox") })
Check "no warning / error in logs" ($bad.Count -eq 0) "$($bad.Count) messages"
$bad | ForEach-Object { "FAIL log, $($_.severity): $($_.message)" }

ShowResults
$failed = $script:failedChecks
OpenSession $Session
exit $failed
