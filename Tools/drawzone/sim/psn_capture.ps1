# psn_capture.ps1 - reads what a PSN sender emits, for the checks
#
#   powershell -ExecutionPolicy Bypass -File psn_capture.ps1 -Via 127.0.0.1
#
# Listens for Seconds on one network card (-Via, empty = default one) and returns the last frame :
# packets, frames, ids (sorted), pos[id] and speed[id] as x, y, z, info (text of the first info packet).

param(
	[string]$Group = "236.10.10.11",
	[int]$Port = 56565,
	[string]$Via = "127.0.0.1",
	[float]$Seconds = 1.2
)

function Kids([byte[]]$b, [int]$o, [int]$l)
{
	$e = $o + $l
	$r = New-Object System.Collections.ArrayList
	while ($o + 4 -le $e)
	{
		$h = [BitConverter]::ToUInt32($b, $o)
		$len = ($h -shr 16) -band 0x7FFF
		[void]$r.Add(@(($h -band 0xFFFF), $len, ($o + 4)))
		$o += 4 + $len
	}
	return ,$r
}

function Float3([byte[]]$b, [int]$o) { return @([BitConverter]::ToSingle($b, $o), [BitConverter]::ToSingle($b, $o + 4), [BitConverter]::ToSingle($b, $o + 8)) }

$u = New-Object System.Net.Sockets.UdpClient
$u.ExclusiveAddressUse = $false
$u.Client.SetSocketOption('Socket', 'ReuseAddress', $true)
$u.Client.Bind((New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Any, $Port)))
if ($Via) { $u.JoinMulticastGroup([System.Net.IPAddress]::Parse($Group), [System.Net.IPAddress]::Parse($Via)) }
else { $u.JoinMulticastGroup([System.Net.IPAddress]::Parse($Group)) }
$u.Client.ReceiveTimeout = 200

$result = @{ packets = 0; frames = 0; ids = @(); pos = @{}; speed = @{}; info = "" }
$sw = [Diagnostics.Stopwatch]::StartNew()
while ($sw.Elapsed.TotalSeconds -lt $Seconds)
{
	$ep = New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Any, 0)
	try { [byte[]]$b = $u.Receive([ref]$ep) } catch { continue }
	$result.packets++
	$id = [BitConverter]::ToUInt16($b, 0)
	$len = ([BitConverter]::ToUInt32($b, 0) -shr 16) -band 0x7FFF
	if ($id -eq 0x6756)
	{
		if (-not $result.info) { $result.info = [System.Text.Encoding]::ASCII.GetString($b) -replace '[^\x20-\x7e]', '.' }
	}
	elseif ($id -eq 0x6755)
	{
		$result.frames++
		$ids = @(); $pos = @{}; $speed = @{}
		foreach ($c in (Kids $b 4 $len))
		{
			if ($c[0] -ne 1) { continue }
			foreach ($t in (Kids $b $c[2] $c[1]))
			{
				$ids += [int]$t[0]
				foreach ($f in (Kids $b $t[2] $t[1]))
				{
					if ($f[0] -eq 0) { $pos[[int]$t[0]] = Float3 $b $f[2] }
					elseif ($f[0] -eq 1) { $speed[[int]$t[0]] = Float3 $b $f[2] }
				}
			}
		}
		$result.ids = @($ids | Sort-Object); $result.pos = $pos; $result.speed = $speed
	}
}
$u.Close()
return $result
