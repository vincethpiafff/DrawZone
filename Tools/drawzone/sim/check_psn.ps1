# check_psn.ps1 - test of the PSN source of the Cockpit module with psn_sim.ps1 and augmenta_sim.ps1
#
#   powershell -ExecutionPolicy Bypass -File check_psn.ps1
#
# Needs Chataigne running with a session whose Cockpit module has :
#   - source "PSN Y" : port 56575, Up Axis Y, Offset 1, 0.5
#   - source "PSN Z" : port 56576, Up Axis Z, Area Position -8, -2, Area Size 16, 12 (like Kratos)
#   - Scene Size Mode Auto, video output on, started without any Augmenta message
# (make_psn_session.ps1 makes it from a copy of TEST_ZONES.noisette). Read only through MCP.

# -NoChoice : session made with make_psn_session.ps1 -NoChoice, no source is marked Use As Scene

param([string]$Endpoint = "http://127.0.0.1:42002/mcp", [switch]$NoChoice)

$ErrorActionPreference = "Stop"
$psnSim = Join-Path $PSScriptRoot "psn_sim.ps1"
$augmentaSim = Join-Path $PSScriptRoot "augmenta_sim.ps1"
$token = [Environment]::GetEnvironmentVariable("CHATAIGNE_MCP_TOKEN", "User")
if (-not $token) { throw "CHATAIGNE_MCP_TOKEN is not set" }

$script:session = $null
$script:nextId = 1

function ParseBody([string]$body)
{
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
		if ($_.ErrorDetails -and $_.ErrorDetails.Message) { return ParseBody $_.ErrorDetails.Message }
		throw
	}
	if (-not $script:session -and $r.Headers["Mcp-Session-Id"]) { $script:session = $r.Headers["Mcp-Session-Id"] }
	return ParseBody ([System.Text.Encoding]::UTF8.GetString($r.RawContentStream.ToArray()))
}

function Tool([string]$name, $arguments)
{
	$r = Rpc "tools/call" @{ name = $name; arguments = $arguments }
	if ($r.error -or $r.result.isError) { return $null }
	return $r.result.content[0].text | ConvertFrom-Json
}

$init = Rpc "initialize" @{ protocolVersion = "2025-06-18"; capabilities = @{}; clientInfo = @{ name = "drawzone-check-psn"; version = "1" } }
$script:protocol = $init.result.protocolVersion
$headers = @{ "Authorization" = "Bearer $token"; "Accept" = "application/json, text/event-stream"; "Mcp-Session-Id" = $script:session; "MCP-Protocol-Version" = $script:protocol }
[void](Invoke-WebRequest -Uri $Endpoint -Method Post -Headers $headers -ContentType "application/json" -Body '{"jsonrpc":"2.0","method":"notifications/initialized"}' -UseBasicParsing)

$modules = (Tool "chataigne_project" @{}).modules

function Value([string]$path)
{
	$o = Tool "organic_inspect" @{ handle = $modules; path = $path }
	if ($null -eq $o) { return $null }
	return $o.value
}

$psnOutputFile = Join-Path $PSScriptRoot "psn_capture.ps1"
function Output([string]$via = "127.0.0.1") { return & $psnOutputFile -Via $via }

$results = New-Object System.Collections.Generic.List[object]
function Check([string]$name, [bool]$ok, [string]$got)
{
	$results.Add([pscustomobject]@{ Result = $(if ($ok) { "PASS" } else { "FAIL" }); Check = $name; Got = $got })
}
function Near($a, $b, [float]$tol = 0.02) { return [math]::Abs([float]$a - [float]$b) -le $tol }
function Sim([string]$file, [object[]]$simArgs) { return Start-Process powershell -ArgumentList (@("-ExecutionPolicy", "Bypass", "-File", $file) + $simArgs) -WindowStyle Hidden -PassThru }

$startTime = Get-Date
$sources = "spatial/parameters/sources"
$people = "spatial/values/people"

#Auto : no Augmenta scene yet, so the Area Size of PSN Z (the video output shows the scene size)
$res = Value "spatial/parameters/videoOutput/resolution"
Check "scene from PSN area (16 x 12)" ($res -match "^1600 x 1200 px") "'$res'"

#before anything is sent
Check "PSN Y connected, no data" ((Value "$sources/psnY/status") -match "^No data yet, 0 people, 236\.10\.10\.10:56575") "$(Value "$sources/psnY/status")"

#Y up : x = 2, z = 1.5 + offset (1, 0.5) -> scene (3, 2) ; the unused tracker at the origin must not appear
$p = Sim $psnSim @("-Port", 56575, "-Id", 77, "-X", 2, "-Y", 1.7, "-Z", 1.5, "-Vx", 0.8, "-Vz", -0.4, "-Seconds", 9)
Start-Sleep -Milliseconds 1500
$pos = Value "$people/person77/position"
Check "Y up : position (3, 2)" ($null -ne $pos -and (Near $pos[0] 3) -and (Near $pos[1] 2)) ($pos -join ", ")
Check "Y up : height 1.7" (Near (Value "$people/person77/height") 1.7) "$(Value "$people/person77/height")"
$v = Value "$people/person77/velocity"
Check "Y up : velocity (0.8, -0.4)" ($null -ne $v -and (Near $v[0] 0.8) -and (Near $v[1] -0.4)) ($v -join ", ")
$age = Value "$people/person77/age"
Check "Y up : age grows" ($age -gt 0.5 -and $age -lt 3) "$age"
Check "tracker at origin ignored" ($null -eq (Tool "organic_inspect" @{ handle = $modules; path = "$people/person500" })) ""
Check "person count 1" ((Value "spatial/values/personCount") -eq 1) "$(Value "spatial/values/personCount")"
$s = Value "$sources/psnY/status"
Check "status receiving" ($s -match "^Receiving, \d+ msg/s, 1 people, 236\.10\.10\.10:56575") "'$s'"
#PSN output : slot 1 -> tracker 1, scene meters, y up (height), z = scene y
$o = Output
Check "output : only tracker 1" (($o.ids -join ",") -eq "1") ($o.ids -join ",")
Check "output : position (3, 1.7, 2)" ($o.ids.Count -gt 0 -and (Near $o.pos[1][0] 3) -and (Near $o.pos[1][1] 1.7) -and (Near $o.pos[1][2] 2)) ($(if ($o.ids.Count -gt 0) { $o.pos[1] -join ", " }))
Check "output : speed (0.8, 0, -0.4)" ($o.ids.Count -gt 0 -and (Near $o.speed[1][0] 0.8) -and (Near $o.speed[1][1] 0) -and (Near $o.speed[1][2] -0.4)) ($(if ($o.ids.Count -gt 0) { $o.speed[1] -join ", " }))
Check "output : slot name in the info packet" ($o.info -match "JOUEUR 1") "'$($o.info)'"
#another card : a socket joined without a card would also hear Loopback, so a real card is named
$otherCard = (Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.IPAddress -notlike "127.*" -and $_.IPAddress -notlike "169.*" -and (Get-NetIPInterface -InterfaceIndex $_.InterfaceIndex -AddressFamily IPv4).ConnectionState -eq "Connected" } | Select-Object -First 1).IPAddress
$oOther = Output $otherCard
Check "output : chosen card only, nothing on $otherCard" ($oOther.packets -eq 0) "$($oOther.packets) packets"
Check "output : status" ((Value "spatial/parameters/psnOutput/status") -match "^Sending 1 tracker \(Player Slots\) to 236\.10\.10\.11:56565$") "$(Value "spatial/parameters/psnOutput/status")"
Check "source name" ((Value "$people/person77/source") -eq "PSN Y") "$(Value "$people/person77/source")"
$p.WaitForExit()

#no leave in PSN : gone after the timeout
Start-Sleep -Milliseconds 1500
$o = Output
Check "output : held slot still sent at its last position" (($o.ids -join ",") -eq "1" -and (Near $o.pos[1][0] 3) -and (Near $o.pos[1][2] 2) -and (Near $o.speed[1][0] 0)) ($o.ids -join ",")
Check "gone after silence" ($null -eq (Tool "organic_inspect" @{ handle = $modules; path = "$people/person77" })) ""
Start-Sleep -Milliseconds 500
$s = Value "$sources/psnY/status"
Check "status silent" ($s -match "^Silent, 0 people") "'$s'"

#Z up, other port, sent on Loopback like Kratos : area x -8..8, y -2..10 ; x = 1 -> 9, y = 1 -> 10 - 1 = 9 from the top
$p = Sim $psnSim @("-Via", "127.0.0.1", "-Port", 56576, "-Id", 12, "-X", 1, "-Y", 1, "-Z", 1.6, "-Seconds", 3)
Start-Sleep -Milliseconds 1500
$pos = Value "$people/person12/position"
Check "Z up, area : position (9, 9)" ($null -ne $pos -and (Near $pos[0] 9) -and (Near $pos[1] 9)) ($pos -join ", ")
Check "Z up : height 1.6" (Near (Value "$people/person12/height") 1.6) "$(Value "$people/person12/height")"
Check "other port : only PSN Z receives" ((Value "$sources/psnY/status") -notmatch "^Receiving") "$(Value "$sources/psnY/status")"
$p.WaitForExit()
Start-Sleep -Milliseconds 1500

#Augmenta and PSN together, same id 77 : one keeps 77, the other gets an id from 10000
$a = Sim $augmentaSim @("-Id", 77, "-X", 4, "-Y", 1, "-Seconds", 4)
Start-Sleep -Milliseconds 1000
$p = Sim $psnSim @("-Port", 56575, "-Id", 77, "-X", 2, "-Y", 1.7, "-Z", 1.5, "-Seconds", 2)
Start-Sleep -Milliseconds 1200
$first = Value "$people/person77/source"
Check "same id : first keeps 77" ($first -eq "Augmenta") "$first"
$second = Value "$people/person10000/source"
if ($null -eq $second)
{
	#the counter only goes up : find the remapped person by its count
	$second = (Tool "organic_inspect" @{ handle = $modules; path = "$people" } | ConvertTo-Json -Depth 6)
	Check "same id : other gets an id from 10000" ($second -match '"person1\d{4}"|person1\d{4}') "$second"
}
else { Check "same id : other gets an id from 10000" ($second -eq "PSN Y") "$second" }
$res = Value "spatial/parameters/videoOutput/resolution"
if ($NoChoice) { Check "2 scenes, none chosen : no scene" ($res -match "^Waiting for scene size") "'$res'" }
else { Check "2 scenes : the chosen source keeps the scene" ($res -match "^1600 x 1200 px") "'$res'" }
Check "same id : 2 people" ((Value "spatial/values/personCount") -eq 2) "$(Value "spatial/values/personCount")"
$p.WaitForExit()
$a.WaitForExit()

#no slot left for the 3rd person : not sent. First wait for the held slots to end (5 s in TEST_ZONES)
Start-Sleep -Seconds 7
$o = Output
Check "output : free slots not sent" ($o.ids.Count -eq 0) "ids $($o.ids -join ",")"
$a = Sim $augmentaSim @("-Id", 40, "-X", 4, "-Y", 1, "-Seconds", 3)
$p = Sim $psnSim @("-Port", 56575, "-Id", 41, "-X", 2, "-Y", 1.7, "-Z", 1.5, "-Seconds", 3)
$q = Sim $psnSim @("-Via", "127.0.0.1", "-Port", 56576, "-Id", 42, "-X", 1, "-Y", 1, "-Z", 1.6, "-Seconds", 3)
Start-Sleep -Milliseconds 1500
$people3 = Value "spatial/values/personCount"
$o = Output
Check "output : 3 people, 2 slots, trackers 1 and 2" ($people3 -eq 3 -and ($o.ids -join ",") -eq "1,2") "$people3 people, ids $($o.ids -join ",")"
$a.WaitForExit(); $p.WaitForExit(); $q.WaitForExit()

$diag = Tool "chataigne_diagnostics" @{ limit = 200 }
$bad = @($diag.logs | Where-Object { $_.severity -ne "info" -and [datetime]$_.time -ge $startTime })
Check "no warning / error in logs" ($bad.Count -eq 0) $(($bad | ForEach-Object { "$($_.severity): $($_.message)" }) -join " | ")

$results | Format-Table -AutoSize | Out-String -Width 220
$failed = @($results | Where-Object { $_.Result -eq "FAIL" }).Count
"$($results.Count - $failed) / $($results.Count) passed"
exit $failed
