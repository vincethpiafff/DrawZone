# check_sim.ps1 - non-regression test of the Cockpit module with the Augmenta simulator
#
#   powershell -ExecutionPolicy Bypass -File check_sim.ps1
#
# Needs Chataigne running with TEST_ZONES.noisette, the MCP server enabled (Inspect profile)
# and the token in the user variable CHATAIGNE_MCP_TOKEN. Read only : it never changes the session.
# For each scenario, a fake person (id 77) is sent at a known position, then read back
# (position, zone, relative position, slot), then it must disappear after /object/leave.

param(
	[string]$Endpoint = "http://127.0.0.1:42002/mcp",
	[switch]$ProjectOnly	# only print "saved" or "modified" (used before closing Chataigne)
)

$ErrorActionPreference = "Stop"
$sim = Join-Path $PSScriptRoot "augmenta_sim.ps1"
$token = [Environment]::GetEnvironmentVariable("CHATAIGNE_MCP_TOKEN", "User")
if (-not $token) { throw "CHATAIGNE_MCP_TOKEN is not set" }

$script:session = $null
$script:nextId = 1

function ParseBody([string]$body)
{
	# JSON, or server-sent events : keep the last data line
	if ($body.TrimStart().StartsWith("{")) { return $body | ConvertFrom-Json }
	$data = ($body -split "`n" | Where-Object { $_ -like "data:*" } | Select-Object -Last 1)
	return $data.Substring(5) | ConvertFrom-Json
}

function Rpc([string]$method, $params)
{
	$headers = @{ "Authorization" = "Bearer $token"; "Accept" = "application/json, text/event-stream" }
	if ($script:session) { $headers["Mcp-Session-Id"] = $script:session; $headers["MCP-Protocol-Version"] = $script:protocol }
	$body = @{ jsonrpc = "2.0"; id = $script:nextId; method = $method; params = $params } | ConvertTo-Json -Depth 10 -Compress
	$script:nextId++
	try { $r = Invoke-WebRequest -Uri $Endpoint -Method Post -Headers $headers -ContentType "application/json" -Body $body -UseBasicParsing }
	catch
	{
		#JSON-RPC errors come with an HTTP error status (e.g. "child does not exist")
		if ($_.ErrorDetails -and $_.ErrorDetails.Message) { return ParseBody $_.ErrorDetails.Message }
		throw
	}
	if (-not $script:session -and $r.Headers["Mcp-Session-Id"]) { $script:session = $r.Headers["Mcp-Session-Id"] }
	return ParseBody ([System.Text.Encoding]::UTF8.GetString($r.RawContentStream.ToArray()))
}

function Tool([string]$name, $arguments)
{
	$r = Rpc "tools/call" @{ name = $name; arguments = $arguments }
	if ($r.error) { return $null }
	$text = $r.result.content[0].text
	if ($r.result.isError) { return $null }
	return $text | ConvertFrom-Json
}

$init = Rpc "initialize" @{ protocolVersion = "2025-06-18"; capabilities = @{}; clientInfo = @{ name = "drawzone-check"; version = "1" } }
$script:protocol = $init.result.protocolVersion
#required by the protocol before any other request (a notification : no id, no answer)
$headers = @{ "Authorization" = "Bearer $token"; "Accept" = "application/json, text/event-stream"; "Mcp-Session-Id" = $script:session; "MCP-Protocol-Version" = $script:protocol }
[void](Invoke-WebRequest -Uri $Endpoint -Method Post -Headers $headers -ContentType "application/json" -Body '{"jsonrpc":"2.0","method":"notifications/initialized"}' -UseBasicParsing)

$project = Tool "chataigne_project" @{}
$modules = $project.modules
if ($ProjectOnly) { if ($project.modified) { "modified" } else { "saved" }; exit 0 }

function Value([string]$path)
{
	$o = Tool "organic_inspect" @{ handle = $modules; path = $path }
	if ($null -eq $o) { return $null }
	return $o.value
}

$results = New-Object System.Collections.Generic.List[object]
function Check([string]$name, [bool]$ok, [string]$got)
{
	$results.Add([pscustomobject]@{ Result = $(if ($ok) { "PASS" } else { "FAIL" }); Check = $name; Got = $got })
}

function Near($a, $b, [float]$tol = 0.02) { return [math]::Abs([float]$a - [float]$b) -le $tol }

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
$spoutOn = Value "spatial/parameters/videoOutput/spout"
$ndiOn = Value "spatial/parameters/videoOutput/ndi"
if ($spoutOn -or $ndiOn)
{
	$res = Value "spatial/parameters/videoOutput/resolution"
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

#no warning or error in the logs during the test
$diag = Tool "chataigne_diagnostics" @{ limit = 200 }
$bad = @($diag.logs | Where-Object { $_.severity -ne "info" -and [datetime]$_.time -ge $startTime })
Check "no warning / error in logs" ($bad.Count -eq 0) $(($bad | ForEach-Object { "$($_.severity): $($_.message)" }) -join " | ")

$results | Format-Table -AutoSize | Out-String -Width 220
$failed = @($results | Where-Object { $_.Result -eq "FAIL" }).Count
"$($results.Count - $failed) / $($results.Count) passed"
exit $failed
