# psn_sim.ps1 - sends fake PosiStageNet trackers to test DrawZone
#
#   powershell -ExecutionPolicy Bypass -File psn_sim.ps1 -Id 77 -X 1 -Y 1.7 -Z 2 -Seconds 2
#
# Same chunks as External/posistagenet (little endian) : a data packet at Rate Hz, an info packet
# every second. Tracker Id at X, Y, Z (meters) with speed Vx, Vy, Vz, plus an unused tracker
# almost at 0, 0, 0 (id 500) like Kratos sends. No "leave" in PSN : the tracker just stops.

param(
	[string]$Group = "236.10.10.10",
	[string]$Via = "",	# network card to send from (127.0.0.1 like Kratos here), default one otherwise
	[int]$Port = 56565,
	[int]$Id = 77,
	[float]$X = 1,
	[float]$Y = 1.7,
	[float]$Z = 2,
	[float]$Vx = 0,
	[float]$Vy = 0,
	[float]$Vz = 0,
	[float]$Seconds = 2,
	[int]$Rate = 30
)

# id on 16 bits, data length on 15, "has subchunks" on the last bit
function Chunk([int]$id, [byte[]]$data, [bool]$sub)
{
	$h = [uint32]$id -bor ([uint32]$data.Length -shl 16)
	if ($sub) { $h = $h -bor [uint32]2147483648 }
	return ,([byte[]]([BitConverter]::GetBytes([uint32]$h) + $data))
}

function Float3([float]$a, [float]$b, [float]$c)
{
	return ,([byte[]]([BitConverter]::GetBytes([single]$a) + [BitConverter]::GetBytes([single]$b) + [BitConverter]::GetBytes([single]$c)))
}

function Header([int]$chunkId, [uint64]$usec, [int]$frame)
{
	# timestamp, version 2.3, frame id, packets in the frame
	$data = [byte[]]([BitConverter]::GetBytes($usec) + [byte[]](2, 3, ($frame % 256), 1))
	return ,(Chunk $chunkId $data $false)
}

function DataTracker([int]$id, [float]$px, [float]$py, [float]$pz, [float]$sx, [float]$sy, [float]$sz)
{
	$fields = [byte[]]((Chunk 0 (Float3 $px $py $pz) $false) + (Chunk 1 (Float3 $sx $sy $sz) $false))
	return ,(Chunk $id $fields $true)
}

function InfoTracker([int]$id, [string]$name)
{
	return ,(Chunk $id (Chunk 0 ([System.Text.Encoding]::ASCII.GetBytes($name)) $false) $true)
}

$udp = New-Object System.Net.Sockets.UdpClient
if ($Via) { $udp.Client.SetSocketOption('IP', 'MulticastInterface', [System.Net.IPAddress]::Parse($Via).GetAddressBytes()) }
$udp.Connect([System.Net.IPAddress]::Parse($Group), $Port)

$frames = [int]($Seconds * $Rate)
for ($f = 0; $f -lt $frames; $f++)
{
	$usec = [uint64]($f * 1000000 / $Rate)

	$trackers = [byte[]]((DataTracker $Id $X $Y $Z $Vx $Vy $Vz) + (DataTracker 500 4.5e-18 -6.8e-19 1.9e-17 0 0 0))
	$list = Chunk 1 $trackers $true
	$packet = Chunk 0x6755 ([byte[]]((Header 0 $usec $f) + $list)) $true
	[void]$udp.Send($packet, $packet.Length)

	if ($f % $Rate -eq 0)
	{
		$names = Chunk 2 ([byte[]]((InfoTracker $Id "Sim") + (InfoTracker 500 "Unused"))) $true
		$system = Chunk 1 ([System.Text.Encoding]::ASCII.GetBytes("DrawZone PSN sim")) $false
		$info = Chunk 0x6756 ([byte[]]((Header 0 $usec $f) + $system + $names)) $true
		[void]$udp.Send($info, $info.Length)
	}

	Start-Sleep -Milliseconds ([int](1000 / $Rate))
}

$udp.Close()
"sent $frames frames, tracker $Id at ($X, $Y, $Z) m to ${Group}:$Port"
