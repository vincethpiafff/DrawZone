# common.ps1 - shared by the test scripts, dot-sourced :  . (Join-Path $PSScriptRoot "common.ps1")
#
# Results (Check, ShowResults), comparisons (Near, Near3), simulators started in the background (Sim),
# read-only MCP client (ConnectMcp, Tool, Value) and Chataigne closing / opening (CloseChataigne, OpenSession).

$results = New-Object System.Collections.Generic.List[object]
function Check([string]$name, [bool]$ok, [string]$got)
{
	$results.Add([pscustomobject]@{ Result = $(if ($ok) { "PASS" } else { "FAIL" }); Check = $name; Got = $got })
}

#table and "n / m passed" ; the caller ends with : exit $script:failedChecks
function ShowResults
{
	$results | Format-Table -AutoSize -Wrap | Out-String -Width 220
	$script:failedChecks = @($results | Where-Object { $_.Result -eq "FAIL" }).Count
	"$($results.Count - $script:failedChecks) / $($results.Count) passed"
}

function Near($a, $b, [float]$tol = 0.02) { return [math]::Abs([float]$a - [float]$b) -le $tol }
function Near3($v, $x, $y, $z) { return $null -ne $v -and (Near $v[0] $x) -and (Near $v[1] $y) -and (Near $v[2] $z) }
function Sim([string]$file, [object[]]$simArgs) { return Start-Process powershell -ArgumentList (@("-ExecutionPolicy", "Bypass", "-File", $file) + $simArgs) -WindowStyle Hidden -PassThru }

# --- MCP (Chataigne's server, token in the user variable CHATAIGNE_MCP_TOKEN, Inspect profile : read only)

function ParseBody([string]$body)
{
	# JSON, or server-sent events : keep the last data line
	if ($body.TrimStart().StartsWith("{")) { return $body | ConvertFrom-Json }
	$data = ($body -split "`n" | Where-Object { $_ -like "data:*" } | Select-Object -Last 1)
	return $data.Substring(5) | ConvertFrom-Json
}

function Rpc([string]$method, $params)
{
	$headers = @{ "Authorization" = "Bearer $script:mcpToken"; "Accept" = "application/json, text/event-stream" }
	if ($script:mcpSession) { $headers["Mcp-Session-Id"] = $script:mcpSession; $headers["MCP-Protocol-Version"] = $script:mcpProtocol }
	$body = @{ jsonrpc = "2.0"; id = $script:mcpNextId; method = $method; params = $params } | ConvertTo-Json -Depth 10 -Compress
	$script:mcpNextId++
	try { $r = Invoke-WebRequest -Uri $script:mcpEndpoint -Method Post -Headers $headers -ContentType "application/json" -Body $body -UseBasicParsing }
	catch
	{
		#JSON-RPC errors come with an HTTP error status (e.g. "child does not exist")
		if ($_.ErrorDetails -and $_.ErrorDetails.Message) { return ParseBody $_.ErrorDetails.Message }
		throw
	}
	if (-not $script:mcpSession -and $r.Headers["Mcp-Session-Id"]) { $script:mcpSession = $r.Headers["Mcp-Session-Id"] }
	return ParseBody ([System.Text.Encoding]::UTF8.GetString($r.RawContentStream.ToArray()))
}

function Tool([string]$name, $arguments)
{
	$r = Rpc "tools/call" @{ name = $name; arguments = $arguments }
	if ($r.error -or $r.result.isError) { return $null }
	return $r.result.content[0].text | ConvertFrom-Json
}

#opens the MCP session, returns chataigne_project ; Value reads under its modules
function ConnectMcp([string]$endpoint = "http://127.0.0.1:42002/mcp", [string]$client = "drawzone-check")
{
	$script:mcpEndpoint = $endpoint
	$script:mcpToken = [Environment]::GetEnvironmentVariable("CHATAIGNE_MCP_TOKEN", "User")
	if (-not $script:mcpToken) { throw "CHATAIGNE_MCP_TOKEN is not set" }
	$script:mcpSession = $null
	$script:mcpNextId = 1

	$init = Rpc "initialize" @{ protocolVersion = "2025-06-18"; capabilities = @{}; clientInfo = @{ name = $client; version = "1" } }
	$script:mcpProtocol = $init.result.protocolVersion
	#required by the protocol before any other request (a notification : no id, no answer)
	$headers = @{ "Authorization" = "Bearer $script:mcpToken"; "Accept" = "application/json, text/event-stream"; "Mcp-Session-Id" = $script:mcpSession; "MCP-Protocol-Version" = $script:mcpProtocol }
	[void](Invoke-WebRequest -Uri $endpoint -Method Post -Headers $headers -ContentType "application/json" -Body '{"jsonrpc":"2.0","method":"notifications/initialized"}' -UseBasicParsing)

	$project = Tool "chataigne_project" @{}
	$script:modules = $project.modules
	return $project
}

function Value([string]$path)
{
	$o = Tool "organic_inspect" @{ handle = $script:modules; path = $path }
	if ($null -eq $o) { return $null }
	return $o.value
}

# --- Chataigne

$chataigneExe = Join-Path (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")) "Binaries\App\Chataigne.exe"

#closed only when its session has no unsaved change (read through MCP) ; else the script stops
function CloseChataigne
{
	$running = Get-Process Chataigne -ErrorAction SilentlyContinue
	if (-not $running) { return }
	$state = & (Join-Path $PSScriptRoot "check_sim.ps1") -ProjectOnly
	if ($state -ne "saved") { "STOP : the session has unsaved changes ($state), Chataigne not closed"; exit 2 }
	[void]$running.CloseMainWindow()
	if (-not $running.WaitForExit(15000)) { "STOP : Chataigne did not close"; exit 2 }
	Start-Sleep 1
}

#waits for the MCP port, then a few seconds for the session to settle
function OpenSession([string]$file)
{
	CloseChataigne
	Start-Process -FilePath $chataigneExe -ArgumentList "`"$file`"" -WorkingDirectory (Split-Path $chataigneExe)
	$deadline = (Get-Date).AddSeconds(90)
	while ((Get-Date) -lt $deadline)
	{
		try { $c = New-Object System.Net.Sockets.TcpClient("127.0.0.1", 42002); $c.Close(); break } catch { Start-Sleep 1 }
	}
	Start-Sleep 6
}
