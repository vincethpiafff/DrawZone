# check_logic_full.ps1 - full test of the Logic Graph (v0.7.2 brick 7) with the Augmenta simulator
#
#   powershell -ExecutionPolicy Bypass -File check_logic_full.ps1
#
# check_logic.ps1 tests the event nodes ; this one tests every other node, through variables : each value node feeds a
# Set node without In (live), so its value is read back in Values > Variables through MCP (read only), at known moments.
# Copies of the session (make_psn_session.ps1 -NoPsn, the original is not touched), then reopens the session.
#
# Run A (slot release Next Person Takes Slot) :
#   0 s     77 at the centre of ZONE RECT (Capacity 2), height 1.62, for 5 s
#   1.5 s   78 at 0.2 m below it, for 2 s (the zone is full)
#   6 s     80 where ZONE RECT and a zone "overlap" added on its right cross, 81 at the centre of ZONE ELIPSE, for 2.5 s
#   values : Zone Presence Time, Person Age, Zone Occupancy, Person Position, Person Height, Slot Position,
#   Zone Average Distance To Center, Zone Person Count, Person Relative Position (zone overlap) ;
#   logic : Compare, In Range, And, Or, Xor, Not, Branch + Timer ; variables : Set Add / Toggle / Set, Get Number,
#   Get On-Off, Reset Variables (all at start, one when a person is lost), a disabled node, a collapsed group.
# Run B (slot release Hold Spot To Reclaim, Hold Time 3 s) :
#   0 s     77 at the centre of ZONE RECT for 3 s, then lost : slot 1 held
#   4.5 s   79 at the same spot for 2 s : takes slot 1 back ; then lost : held again, nobody comes, released after 3 s
#   Slot Held, Slot Hold Remaining, Slot Person, Slot Relative Position while held, Slot Joined / Left.

param([string]$Session = "D:\0-DRAWZONE\ChataigneProject\TEST_ZONES.noisette")

$ErrorActionPreference = "Stop"
$sim = Join-Path $PSScriptRoot "augmenta_sim.ps1"
. (Join-Path $PSScriptRoot "common.ps1")
$dir = Join-Path $env:TEMP "drawzone_checks"
[void](New-Item -ItemType Directory -Force $dir)

# --- graph and session helpers

function P([string]$address, $value) { return @{ controlAddress = $address; value = $value } }
function NewGraph
{
	$script:nodes = New-Object System.Collections.Generic.List[object]
	$script:links = New-Object System.Collections.Generic.List[object]
	$script:vars = New-Object System.Collections.Generic.List[object]
	$script:groups = New-Object System.Collections.Generic.List[object]
	$script:y = 0
}
function Node([string]$uid, [string]$type, [object[]]$params, [float]$x = 0, [switch]$Disabled)
{
	$ps = @((P "/uid" $uid), (P "/viewUIPosition" @($x, $script:y))) + $params
	if ($Disabled) { $ps += (P "/enabled" $false) }
	$script:nodes.Add(@{ niceName = "$type $uid"; type = $type; parameters = $ps })
	$script:y += 70
}
function Link([string]$a, [string]$pa, [string]$b, [string]$pb) { $script:links.Add(@($a, $pa, $b, $pb)) }
#a variable of the Cockpit ; default : its Default Value
function AddVar([string]$name, [string]$type, $default = $null)
{
	$v = @{ niceName = $name; type = $type }
	if ($null -ne $default) { $v.parameters = @(P "/defaultValue" $default) }
	$script:vars.Add($v)
}
#the value of src.pin kept live in a new variable (Set <type> without In)
function Probe([string]$src, [string]$pin, [string]$var, [string]$type)
{
	AddVar $var $type
	Node "set_$var" "Set $type" @(P "/variable" "/$var") 400
	Link $src $pin "set_$var" "Value"
}
#an exec output written in the logs : word + the data pin
function LogOn([string]$src, [string]$pin, [string]$word, [string]$dataPin)
{
	$uid = "log_$word"
	Node $uid "Log" @(P "/message" $word) 400
	Link $src $pin $uid "In"
	if ($dataPin) { Link $src $dataPin $uid "Value" }
}

#the Variables block of the copy, balanced braces (it may hold the variables of the session)
function ReplaceBlock([string]$t, [string]$key, [string]$json, [string]$before)
{
	$from = 0
	while (($i = $t.IndexOf('"' + $key + '": {', $from)) -ge 0)
	{
		$start = $t.IndexOf('{', $i); $depth = 0; $inStr = $false
		for ($k = $start; $k -lt $t.Length; $k++)
		{
			$c = $t[$k]
			if ($inStr) { if ($c -eq '\') { $k++ } elseif ($c -eq '"') { $inStr = $false }; continue }
			if ($c -eq '"') { $inStr = $true } elseif ($c -eq '{') { $depth++ }
			elseif ($c -eq '}') { $depth--; if ($depth -eq 0) { break } }
		}
		if ($t.Substring($start, $k - $start).Contains('"items"')) { return $t.Substring(0, $start) + $json + $t.Substring($k + 1) }
		$from = $k
	}
	return $t.Replace($before, '"' + $key + '": ' + $json + ', ' + $before)
}

#copy of the session with the graph, the variables, and optional text replacements (zone added, slot release...)
function WriteSession([string]$copy, [hashtable]$replace = @{}, [string]$zoneJson = "")
{
	& (Join-Path $PSScriptRoot "make_psn_session.ps1") -From $Session -To $copy -NoPsn | Out-Null
	$text = [System.IO.File]::ReadAllText($copy)
	$graph = @{ niceName = "Test"; type = "Graph"; containers = @{ nodes = @{ items = $script:nodes }; groups = @{ items = $script:groups } }; links = $script:links }
	$graphJson = @{ items = @($graph) } | ConvertTo-Json -Depth 20 -Compress
	$pattern = '(?s)"logic": \{.*?\}, "scene": \{'
	if (-not [regex]::IsMatch($text, $pattern)) { throw "Logic block not found in $copy" }
	$text = [regex]::Replace($text, $pattern, { param($m) '"logic": ' + $graphJson + ', "scene": {' })
	$varsJson = @{ hideInEditor = $true; items = $script:vars } | ConvertTo-Json -Depth 10 -Compress
	$text = ReplaceBlock $text "variables" $varsJson '"logic": '
	foreach ($k in $replace.Keys)
	{
		if (-not $text.Contains($k)) { throw "not found in the session : $k" }
		$i = $text.IndexOf($k)
		$text = $text.Substring(0, $i) + $replace[$k] + $text.Substring($i + $k.Length)	# first one only (surface SOL)
	}
	if ($zoneJson)
	{
		$zonesHead = '"zones": {"editorIsCollapsed": true, "items": ['
		if (-not $text.Contains($zonesHead)) { throw "zones block not found" }
		$text = $text.Replace($zonesHead, $zonesHead + $zoneJson + ', ')
	}
	[System.IO.File]::WriteAllText($copy, $text)
}

function V([string]$name) { return Value "spatial/values/variables/$name/value" }
#read now, kept by name : $R.R1.zpt...
function ReadVars([string]$tag, [string[]]$names)
{
	$r = @{}
	foreach ($n in $names) { $r[$n] = V $n }
	$script:R[$tag] = $r
	"  $tag : " + (($names | ForEach-Object { "$_=$($r[$_])" }) -join ", ")
}
function Elapsed { return ((Get-Date) - $script:t0).TotalSeconds }
function WaitUntil([float]$s) { $w = $s - (Elapsed); if ($w -gt 0) { Start-Sleep -Milliseconds ([int]($w * 1000)) } }
function NearOrNull($v, $target, [float]$tol) { return $null -ne $v -and (Near $v $target $tol) }

# ZONE RECT and ZONE ELIPSE of the session
$doc = Get-Content $Session -Raw | ConvertFrom-Json
$zonesItems = @($doc.modules.items | Where-Object { $_.type -eq "Cockpit" })[0].params.containers.zones.items
function ZoneParam([string]$zoneName, [string]$address) { return ($zonesItems | Where-Object { $_.niceName -eq $zoneName }).parameters | Where-Object { $_.controlAddress -eq $address } | ForEach-Object { $_.value } }
$rc = @(ZoneParam "ZONE RECT" "/center"); $rs = @(ZoneParam "ZONE RECT" "/size")
$ec = @(ZoneParam "ZONE ELIPSE" "/center")
if ($rc.Count -lt 2 -or $rs.Count -lt 2 -or $ec.Count -lt 2) { "STOP : ZONE RECT / ZONE ELIPSE not found in the session"; exit 99 }
$x = [math]::Round($rc[0], 3); $yy = [math]::Round($rc[1], 3)
$script:R = @{}
$zone = P "/zone" "/zoneRECT"

# ================= Run A =================

NewGraph
#values -> variables (live)
Node "zpt" "Zone Presence Time" @($zone); Probe "zpt" "Time" "zpt" "Time"
Node "age" "Person Age" @(P "/personId" 77); Probe "age" "Time" "age" "Time"
Node "occ" "Zone Occupancy" @($zone); Probe "occ" "Percentage" "occ" "Percentage"
Node "pos" "Person Position" @(P "/personId" 77); Probe "pos" "X" "posx" "Meters"; Probe "pos" "Y" "posy" "Meters"
Node "hgt" "Person Height" @(P "/personId" 77); Probe "hgt" "Height" "height" "Meters"
Node "spos" "Slot Position" @(P "/slot" 1); Probe "spos" "X" "slotx" "Meters"
Node "dist" "Zone Average Distance To Center" @($zone); Probe "dist" "Distance" "dist" "Meters"
Node "zpc" "Zone Person Count" @($zone); Probe "zpc" "Count" "zpc" "Number"
#compare
Node "ge2" "Compare" @((P "/operator" ">="), (P "/b" 2)); Link "zpc" "Count" "ge2" "A"; Probe "ge2" "Result" "full2" "On-Off"
Node "inr" "In Range" @((P "/min" ($x - 0.5)), (P "/max" ($x + 0.5)), (P "/minMeters" ($x - 0.5)), (P "/maxMeters" ($x + 0.5))); Link "pos" "X" "inr" "Value"; Probe "inr" "Result" "inrange" "On-Off"
Node "pres" "Zone Presence" @($zone); Node "full" "Zone Full" @($zone)
foreach ($g in @(@("and", "And"), @("or", "Or"), @("xor", "Xor")))
{
	Node $g[0] $g[1] @(); Link "pres" "State" $g[0] "A"; Link "full" "State" $g[0] "B"; Probe $g[0] "Result" "$($g[0])v" "On-Off"
}
Node "not" "Not" @(); Link "full" "State" "not" "A"; Probe "not" "Result" "notv" "On-Off"
#Branch : every second, inside or outside
Node "tick" "Timer" @(P "/interval" 1.0)
Node "br" "Branch" @(); Link "tick" "Then" "br" "In"; Link "pres" "State" "br" "Condition"
AddVar "ticksin" "Number"; AddVar "ticksout" "Number"
Node "set_ticksin" "Set Number" @((P "/variable" "/ticksin"), (P "/operation" "Add"), (P "/value" 1)) 400; Link "br" "True" "set_ticksin" "In"
Node "set_ticksout" "Set Number" @((P "/variable" "/ticksout"), (P "/operation" "Add"), (P "/value" 1)) 400; Link "br" "False" "set_ticksout" "In"
#each entry : entries + 1, toggle, resettest = 3, then a disabled node (nothing, and no Then)
AddVar "entries" "Number"; AddVar "toggle" "On-Off"; AddVar "resettest" "Number" 7; AddVar "disabled" "Number"; AddVar "exits" "Number"
Node "zin" "Zone Entered" @($zone)
Node "set_entries" "Set Number" @((P "/variable" "/entries"), (P "/operation" "Add"), (P "/value" 1)) 400; Link "zin" "Then" "set_entries" "In"
Node "set_toggle" "Set On-Off" @((P "/variable" "/toggle"), (P "/operation" "Toggle")) 400; Link "set_entries" "Then" "set_toggle" "In"
Node "set_reset3" "Set Number" @((P "/variable" "/resettest"), (P "/operation" "Set"), (P "/value" 3)) 400; Link "set_toggle" "Then" "set_reset3" "In"
Node "set_disabled" "Set Number" @((P "/variable" "/disabled"), (P "/operation" "Add"), (P "/value" 1)) 400 -Disabled; Link "set_reset3" "Then" "set_disabled" "In"
Node "zout" "Zone Exited" @($zone)
Node "set_exits" "Set Number" @((P "/variable" "/exits"), (P "/operation" "Add"), (P "/value" 1)) 400; Link "zout" "Then" "set_exits" "In"
#a person lost : resettest back to 7 (Reset Variables on one variable)
Node "plost" "Person Lost" @()
Node "reset1" "Reset Variables" @(P "/variable" "/resettest") 400; Link "plost" "Then" "reset1" "In"
#On Start : Reset Variables (all), then started + 1
AddVar "started" "Number"
Node "start" "On Start" @()
Node "resetall" "Reset Variables" @() 400; Link "start" "Then" "resetall" "In"
Node "set_started" "Set Number" @((P "/variable" "/started"), (P "/operation" "Add"), (P "/value" 1)) 400; Link "resetall" "Then" "set_started" "In"
#Get : Changed of entries, On / Off of toggle
Node "getent" "Get Number" @(P "/variable" "/entries"); LogOn "getent" "Changed" "GET_ENTRIES" "Value"
Node "gettog" "Get On-Off" @(P "/variable" "/toggle"); LogOn "gettog" "On" "TOG_ON" $null; LogOn "gettog" "Off" "TOG_OFF" $null
#a collapsed group : its nodes keep running
AddVar "grouped" "Number"
Node "gzin" "Zone Entered" @($zone) 1200
Node "set_grouped" "Set Number" @((P "/variable" "/grouped"), (P "/operation" "Add"), (P "/value" 1)) 1500; Link "gzin" "Then" "set_grouped" "In"
$script:groups.Add(@{ niceName = "Hidden"; type = "Group"; parameters = @((P "/collapsed" $true), (P "/members" "gzin,set_grouped"), (P "/viewUIPosition" @(1180, 0)), (P "/viewUISize" @(600, 300))) })
#phase 2 : zones crossing, ellipse
Node "rel80" "Person Relative Position" @(P "/personId" 80); LogOn "rel80" "Changed" "P80_ZONE" "Zone"; Probe "rel80" "Zone X" "ovx" "Percentage"
Node "ovin" "Zone Entered" @(P "/zone" "/overlap"); LogOn "ovin" "Then" "OVL_IN" "Person Id"
Node "eliin" "Zone Entered" @(P "/zone" "/zoneELIPSE"); LogOn "eliin" "Then" "ELI_IN" "Person Id"

#zone "overlap" : same size as ZONE RECT, 1.5 m to its right ; they cross from x + w/2 - ... : 80 at the middle of the crossing
$ow = [double]$rs[0]; $oh = [double]$rs[1]
$ox = $x + 1.5
$crossLeft = $ox - $ow / 2; $crossRight = $x + $ow / 2
$x80 = [math]::Round(($crossLeft + $crossRight) / 2, 3)
$overlapJson = '{"parameters": [{"value": "", "controlAddress": "/surface"}, {"value": "Rectangle", "controlAddress": "/shape"}, {"value": [' + $ox + ', ' + $yy + '], "controlAddress": "/center"}, {"value": [' + $ow + ', ' + $oh + '], "controlAddress": "/size"}, {"value": 4, "hexMode": false, "controlAddress": "/capacity"}], "niceName": "overlap", "type": "Zone"}'
$copyA = "$dir\logic_full_a.noisette"
WriteSession $copyA @{} $overlapJson

"Run A"
OpenSession $copyA
$project = ConnectMcp
$startTime = (Get-Date).AddSeconds(-30)
$names = @("zpt", "age", "occ", "posx", "posy", "height", "slotx", "dist", "zpc", "full2", "inrange", "andv", "orv", "xorv", "notv",
	"ticksin", "ticksout", "entries", "toggle", "resettest", "disabled", "exits", "started", "grouped", "ovx")
ReadVars "R0" $names
$script:t0 = Get-Date
$p77 = Sim $sim @("-Id", "77", "-X", "$x", "-Y", "$yy", "-Height", "1.62", "-Seconds", "5")
WaitUntil 1.5
ReadVars "R1" $names
$p78 = Sim $sim @("-Id", "78", "-X", "$x", "-Y", "$([math]::Round($yy + 0.2, 3))", "-Seconds", "2.5")
WaitUntil 3.4
ReadVars "R2" $names
$p78.WaitForExit(); $p77.WaitForExit()
WaitUntil 6.2
ReadVars "R3" $names
$p80 = Sim $sim @("-Id", "80", "-X", "$x80", "-Y", "$yy", "-Seconds", "2.5")
$t80 = Elapsed
$p81 = Sim $sim @("-Id", "81", "-X", "$([math]::Round($ec[0], 3))", "-Y", "$([math]::Round($ec[1], 3))", "-Seconds", "2.5")
WaitUntil ($t80 + 1.6)
ReadVars "R4" $names
$p80.WaitForExit(); $p81.WaitForExit()
WaitUntil 10.5
ReadVars "R5" $names

$diag = Tool "chataigne_diagnostics" @{ limit = 200 }
$logsA = @($diag.logs | Where-Object { [datetime]$_.time -ge $startTime } | Sort-Object { [datetime]$_.time })
$testA = @($logsA | Where-Object { $_.source -eq "Test" } | ForEach-Object { $_.message.Trim() })
"Logs of the graph :"; $testA | ForEach-Object { "  $_" }
function CountA([string]$m) { return @($testA | Where-Object { $_ -eq $m }).Count }

$R0 = $script:R.R0; $R1 = $script:R.R1; $R2 = $script:R.R2; $R3 = $script:R.R3; $R4 = $script:R.R4; $R5 = $script:R.R5
#start
Check "A start : On Start > Reset Variables > started 1" ($R1.started -eq 1) "$($R1.started)"
Check "A start : resettest at its default 7" ($R0.resettest -eq 7) "$($R0.resettest)"
#77 alone
Check "A 77 : Zone Presence Time 0.2-2.5 s" ($R1.zpt -ge 0.2 -and $R1.zpt -le 2.5) "$($R1.zpt)"
Check "A 77 : Person Age 0.2-2.5 s" ($R1.age -ge 0.2 -and $R1.age -le 2.5) "$($R1.age)"
Check "A 77 : Zone Occupancy 50 % (1 / 2)" (NearOrNull $R1.occ 0.5 0.01) "$($R1.occ)"
Check "A 77 : Person Position = sent position" ((NearOrNull $R1.posx $x 0.05) -and (NearOrNull $R1.posy $yy 0.05)) "$($R1.posx), $($R1.posy)"
Check "A 77 : Person Height 1.62" (NearOrNull $R1.height 1.62 0.01) "$($R1.height)"
Check "A 77 : Slot Position X = its position" (NearOrNull $R1.slotx $x 0.05) "$($R1.slotx)"
Check "A 77 : Zone Average Distance To Center 0" (NearOrNull $R1.dist 0 0.05) "$($R1.dist)"
Check "A 77 : Zone Person Count 1" ($R1.zpc -eq 1) "$($R1.zpc)"
Check "A 77 : Compare 1 >= 2 Off" ($R1.full2 -eq $false) "$($R1.full2)"
Check "A 77 : In Range (x +- 0.5) On" ($R1.inrange -eq $true) "$($R1.inrange)"
Check "A 77 : And (presence, full) Off" ($R1.andv -eq $false) "$($R1.andv)"
Check "A 77 : Or On" ($R1.orv -eq $true) "$($R1.orv)"
Check "A 77 : Xor On" ($R1.xorv -eq $true) "$($R1.xorv)"
Check "A 77 : Not (full) On" ($R1.notv -eq $true) "$($R1.notv)"
Check "A 77 : Set Add, entries 1" ($R1.entries -eq 1) "$($R1.entries)"
Check "A 77 : Set Toggle, toggle On" ($R1.toggle -eq $true) "$($R1.toggle)"
Check "A 77 : Set Set, resettest 3" ($R1.resettest -eq 3) "$($R1.resettest)"
Check "A 77 : collapsed group still runs, grouped 1" ($R1.grouped -eq 1) "$($R1.grouped)"
#77 + 78
Check "A 77+78 : Zone Occupancy 100 %" (NearOrNull $R2.occ 1 0.01) "$($R2.occ)"
Check "A 77+78 : Zone Person Count 2" ($R2.zpc -eq 2) "$($R2.zpc)"
Check "A 77+78 : Zone Average Distance To Center 0.1" (NearOrNull $R2.dist 0.1 0.03) "$($R2.dist)"
Check "A 77+78 : Compare 2 >= 2 On" ($R2.full2 -eq $true) "$($R2.full2)"
Check "A 77+78 : And On" ($R2.andv -eq $true) "$($R2.andv)"
Check "A 77+78 : Or On" ($R2.orv -eq $true) "$($R2.orv)"
Check "A 77+78 : Xor Off" ($R2.xorv -eq $false) "$($R2.xorv)"
Check "A 77+78 : Not Off" ($R2.notv -eq $false) "$($R2.notv)"
Check "A 77+78 : entries 2, toggle Off" ($R2.entries -eq 2 -and $R2.toggle -eq $false) "$($R2.entries), $($R2.toggle)"
Check "A 77+78 : grouped 2" ($R2.grouped -eq 2) "$($R2.grouped)"
Check "A 77+78 : Person Age grows" ($R2.age -gt $R1.age) "$($R1.age) -> $($R2.age)"
Check "A 77+78 : Zone Presence Time grows" ($R2.zpt -gt $R1.zpt) "$($R1.zpt) -> $($R2.zpt)"
#both gone
Check "A gone : exits 2" ($R3.exits -eq 2) "$($R3.exits)"
Check "A gone : Person Lost > Reset Variables (resettest) 7" ($R3.resettest -eq 7) "$($R3.resettest)"
Check "A gone : Branch True each second inside (3-8)" ($R3.ticksin - $R0.ticksin -ge 3 -and $R3.ticksin - $R0.ticksin -le 8) "$($R3.ticksin)"
Check "A gone : variables keep their last value (posx)" (NearOrNull $R3.posx $x 0.05) "$($R3.posx)"
Check "A disabled node : nothing, disabled 0" ($R5.disabled -eq 0) "$($R5.disabled)"
Check "A end : Branch False outside (>= 1)" ($R5.ticksout -ge 1) "$($R5.ticksout)"
#crossing zones
Check "A 80 : Zone X in the crossing of 2 zones 0.5" (NearOrNull $R4.ovx 0.5 0.05) "$($R4.ovx)"
$z80 = @($testA | Where-Object { $_ -like "P80_ZONE *" })
Check "A 80 : Zone = both zones (A + B)" (@($z80 | Where-Object { $_ -like "*overlap*" -and $_ -like "*ZONE RECT*" -and $_ -like "* + *" }).Count -ge 1) "$($z80 -join ' | ')"
Check "A 80 : Zone Entered (overlap) 80 once" ((CountA "OVL_IN 80") -eq 1) "$(CountA 'OVL_IN 80')"
Check "A 81 : Zone Entered (ellipse) 81 once" ((CountA "ELI_IN 81") -eq 1) "$(CountA 'ELI_IN 81')"
Check "A 80 : entries 3 (80 enters ZONE RECT, 81 does not)" ($R5.entries -eq 3) "$($R5.entries)"
#Get
Check "A Get Number : Changed 1, then 2" ((CountA "GET_ENTRIES 1.0") -eq 1 -and (CountA "GET_ENTRIES 2.0") -eq 1) "$(($testA | Where-Object { $_ -like 'GET_ENTRIES*' }) -join ', ')"
Check "A Get On-Off : On then Off (2 entries), On (3rd)" ((CountA "TOG_ON") -eq 2 -and (CountA "TOG_OFF") -eq 1) "$(CountA 'TOG_ON') On, $(CountA 'TOG_OFF') Off"
$badA = @($logsA | Where-Object { $_.severity -ne "info" -and $_.source -notin @("CommunityModuleManager", "HelpBox") })
Check "A no warning / error in logs" ($badA.Count -eq 0) "$($badA.Count) messages"
$badA | ForEach-Object { "FAIL log, $($_.severity): $($_.message)" }

# ================= Run B =================

NewGraph
Node "sh" "Slot Held" @(P "/slot" 1); LogOn "sh" "On" "SHELD_ON" $null; LogOn "sh" "Off" "SHELD_OFF" $null
Node "shr" "Slot Hold Remaining" @(P "/slot" 1); Probe "shr" "Time" "hold" "Time"
Node "sper" "Slot Person" @(P "/slot" 1); Probe "sper" "Person Id" "sper" "Number"
Node "srel" "Slot Relative Position" @(P "/slot" 1); Probe "srel" "Zone X" "srelzx" "Percentage"; Probe "srel" "Surface X" "srelsx" "Percentage"
Node "socc" "Slot Occupied" @(P "/slot" 1); Probe "socc" "State" "occupied" "On-Off"
Node "sj" "Slot Joined" @(P "/slot" 1); LogOn "sj" "Then" "SJOIN" "Person Id"
Node "sl" "Slot Left" @(P "/slot" 1); LogOn "sl" "Then" "SLEFT" "Person Id"
$copyB = "$dir\logic_full_b.noisette"
WriteSession $copyB @{ '{"value": "Next Person Takes Slot", "controlAddress": "/slotRelease"}' = '{"value": "Hold Spot To Reclaim", "controlAddress": "/slotRelease"}, {"value": 3.0, "controlAddress": "/holdTime"}' }	# make_psn_session.ps1 writes the slots of SOL without Hold Time

"Run B"
OpenSession $copyB
$project = ConnectMcp
$startTime = (Get-Date).AddSeconds(-30)
$names = @("hold", "sper", "srelzx", "srelsx", "occupied")
$script:t0 = Get-Date
$p77 = Sim $sim @("-Id", "77", "-X", "$x", "-Y", "$yy", "-Seconds", "3")
WaitUntil 1.5
ReadVars "B1" $names
$p77.WaitForExit()
$lost77 = Elapsed
WaitUntil ($lost77 + 1.0)
ReadVars "B2" $names
WaitUntil ($lost77 + 1.6)
$p79 = Sim $sim @("-Id", "79", "-X", "$x", "-Y", "$yy", "-Seconds", "2")
WaitUntil ($lost77 + 2.9)
ReadVars "B3" $names
$p79.WaitForExit()
$lost79 = Elapsed
WaitUntil ($lost79 + 1.5)
ReadVars "B4" $names
WaitUntil ($lost79 + 4.5)
ReadVars "B5" $names

$diag = Tool "chataigne_diagnostics" @{ limit = 200 }
$logsB = @($diag.logs | Where-Object { [datetime]$_.time -ge $startTime } | Sort-Object { [datetime]$_.time })
$testB = @($logsB | Where-Object { $_.source -eq "Test" } | ForEach-Object { $_.message.Trim() })
"Logs of the graph :"; $testB | ForEach-Object { "  $_" }
function CountB([string]$m) { return @($testB | Where-Object { $_ -eq $m }).Count }
function IndexB([string]$m) { return [array]::IndexOf($testB, $m) }

$B1 = $script:R.B1; $B2 = $script:R.B2; $B3 = $script:R.B3; $B4 = $script:R.B4; $B5 = $script:R.B5
$sx = $x / 8	# Size Mode Auto : the scene of the simulator, 8 m wide
Check "B 77 : Slot Person 77, Occupied" ($B1.sper -eq 77 -and $B1.occupied -eq $true) "$($B1.sper), $($B1.occupied)"
Check "B 77 : Slot Relative Position zone 0.5, surface x / 8" ((NearOrNull $B1.srelzx 0.5 0.03) -and (NearOrNull $B1.srelsx $sx 0.03)) "$($B1.srelzx), $($B1.srelsx)"
Check "B held : Slot Held On" ((CountB "SHELD_ON") -ge 1) "$(CountB 'SHELD_ON')"
Check "B held : Slot Hold Remaining 1.2-2.6 s, 1 s after the loss" ($B2.hold -ge 1.2 -and $B2.hold -le 2.6) "$($B2.hold)"
Check "B held : Slot Person -1 (nobody), still Occupied" ($B2.sper -eq -1 -and $B2.occupied -eq $true) "$($B2.sper), $($B2.occupied)"
Check "B held : Zone X -1, Surface X frozen at x / 8" ((NearOrNull $B2.srelzx -1 0.01) -and (NearOrNull $B2.srelsx $sx 0.03)) "$($B2.srelzx), $($B2.srelsx)"
Check "B reclaim : no Joined, no Left (same player back, Cockpit rule)" ((CountB "SJOIN 79") -eq 0 -and @($testB | Where-Object { $_ -like "SLEFT 77" }).Count -eq 0) "$(($testB | Where-Object { $_ -like 'S*' }) -join ', ')"
Check "B reclaim : Slot Person 79" ($B3.sper -eq 79) "$($B3.sper)"
Check "B reclaim : Slot Held Off when 79 takes it" ((CountB "SHELD_OFF") -ge 1) "$(CountB 'SHELD_OFF')"
Check "B 2nd hold : Slot Hold Remaining 0.8-2.2 s, 1.5 s after the loss" ($B4.hold -ge 0.8 -and $B4.hold -le 2.2) "$($B4.hold)"
Check "B timeout : released after Hold Time, free (-1), not occupied" ($B5.sper -eq -1 -and $B5.occupied -eq $false) "$($B5.sper), $($B5.occupied)"
Check "B timeout : Slot Held On twice, Off twice" ((CountB "SHELD_ON") -eq 2 -and (CountB "SHELD_OFF") -eq 2) "$(CountB 'SHELD_ON') On, $(CountB 'SHELD_OFF') Off"
Check "B Slot Joined 77 once" ((CountB "SJOIN 77") -eq 1) "$(CountB 'SJOIN 77')"
Check "B Slot Left when released (>= 1)" (@($testB | Where-Object { $_ -like "SLEFT*" }).Count -ge 1) "$(($testB | Where-Object { $_ -like 'SLEFT*' }) -join ', ')"
$badB = @($logsB | Where-Object { $_.severity -ne "info" -and $_.source -notin @("CommunityModuleManager", "HelpBox") })
Check "B no warning / error in logs" ($badB.Count -eq 0) "$($badB.Count) messages"
$badB | ForEach-Object { "FAIL log, $($_.severity): $($_.message)" }

ShowResults
$failed = $script:failedChecks
OpenSession $Session
exit $failed
