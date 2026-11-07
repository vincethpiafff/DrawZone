# osc_capture.ps1 - reads the OSC messages sent by the Cockpit, for the checks (v0.7.2 brick 6, Send OSC of the variables)
#
#   powershell -ExecutionPolicy Bypass -File osc_capture.ps1 -Port 9000 -Seconds 8 -Out result.json
#
# Listens on 127.0.0.1:Port for Seconds and writes to Out : count, and for each address the values received in order
# (first argument of each message : float or int).

param(
	[int]$Port = 9000,
	[float]$Seconds = 8,
	[string]$Out = "$env:TEMP\drawzone_checks\osc_capture.json"
)

function ReadString([byte[]]$b, [ref]$o)
{
	$start = $o.Value
	while ($o.Value -lt $b.Length -and $b[$o.Value] -ne 0) { $o.Value++ }
	$s = [System.Text.Encoding]::ASCII.GetString($b, $start, $o.Value - $start)
	$o.Value = ($o.Value + 4) -band -4	# past the 0, padded to 4 bytes
	return $s
}

function BigEndian([byte[]]$b, [int]$o) { $x = $b[$o..($o + 3)]; [array]::Reverse($x); return ,$x }

$u = New-Object System.Net.Sockets.UdpClient
$u.ExclusiveAddressUse = $false
$u.Client.SetSocketOption('Socket', 'ReuseAddress', $true)
$u.Client.Bind((New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Any, $Port)))
$u.Client.ReceiveTimeout = 200

$result = @{ count = 0; values = @{} }
$sw = [Diagnostics.Stopwatch]::StartNew()
while ($sw.Elapsed.TotalSeconds -lt $Seconds)
{
	try
	{
		$ep = New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Any, 0)
		$b = $u.Receive([ref]$ep)
	}
	catch { continue }
	$o = 0
	$address = ReadString $b ([ref]$o)
	if (-not $address.StartsWith("/")) { continue }	# a bundle or something else
	$tags = ReadString $b ([ref]$o)
	if ($tags.Length -lt 2 -or $o + 4 -gt $b.Length) { continue }
	$v = if ($tags[1] -eq 'f') { [BitConverter]::ToSingle((BigEndian $b $o), 0) } elseif ($tags[1] -eq 'i') { [BitConverter]::ToInt32((BigEndian $b $o), 0) } else { $null }
	if (-not $result.values.ContainsKey($address)) { $result.values[$address] = New-Object System.Collections.ArrayList }
	[void]$result.values[$address].Add($v)
	$result.count++
}
$u.Close()
[void](New-Item -ItemType Directory -Force (Split-Path $Out))
$result | ConvertTo-Json -Depth 5 | Set-Content -Encoding UTF8 $Out
