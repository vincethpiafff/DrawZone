# augmenta_sim.ps1 - sends one fake Augmenta person (OSC protocol v2) to test DrawZone
#
#   powershell -ExecutionPolicy Bypass -File augmenta_sim.ps1 -X 6.15 -Y 2.0 -Seconds 2
#
# Positions in meters, in a scene of SceneW x SceneH (sent as /scene, like Fusion).
# Sends /object/update at Rate Hz for Seconds, then /object/leave unless -NoLeave.
# Works alongside Augmenta Fusion : use an id Fusion does not use (default 77).

param(
	[string]$TargetHost = "127.0.0.1",
	[int]$Port = 12999,	# the test sessions of make_psn_session.ps1, not the one Fusion sends to
	[int]$Id = 77,
	[float]$X = 4,
	[float]$Y = 2.25,
	[float]$Vx = 0,
	[float]$Vy = 0,
	[float]$SceneW = 8,
	[float]$SceneH = 4.5,
	[float]$Height = 1.75,
	[float]$Seconds = 2,
	[int]$Rate = 30,
	[switch]$NoLeave
)

function OscString([string]$s)
{
	$b = [System.Text.Encoding]::ASCII.GetBytes($s)
	$len = $b.Length + 1
	$padded = [int]([math]::Ceiling($len / 4) * 4)
	$out = New-Object byte[] $padded
	[Array]::Copy($b, $out, $b.Length)
	return ,$out
}

function BigEndian([byte[]]$b)
{
	if ([BitConverter]::IsLittleEndian) { [Array]::Reverse($b) }
	return ,$b
}

# args : array of [int] or [float] values
function OscMessage([string]$address, [object[]]$values)
{
	$tags = ","
	$data = New-Object System.Collections.Generic.List[byte]
	foreach ($v in $values)
	{
		if ($v -is [int]) { $tags += "i"; $data.AddRange((BigEndian ([BitConverter]::GetBytes([int32]$v)))) }
		else { $tags += "f"; $data.AddRange((BigEndian ([BitConverter]::GetBytes([single]$v)))) }
	}
	$msg = New-Object System.Collections.Generic.List[byte]
	$msg.AddRange((OscString $address))
	$msg.AddRange((OscString $tags))
	$msg.AddRange($data)
	return ,$msg.ToArray()
}

$udp = New-Object System.Net.Sockets.UdpClient
$udp.Connect($TargetHost, $Port)

$nx = [single]($X / $SceneW)
$ny = [single]($Y / $SceneH)
$frames = [int]($Seconds * $Rate)
$age = [single]0

for ($f = 0; $f -lt $frames; $f++)
{
	$scene = OscMessage "/scene" @([int]$f, [int]1, [single]$SceneW, [single]$SceneH)
	[void]$udp.Send($scene, $scene.Length)

	# frame, id, oid, age, cx, cy, vx, vy, orientation, bbox x, y, w, h, rotation, height
	$args15 = @([int]$f, [int]$Id, [int]0, $age, $nx, $ny, [single]($Vx / $SceneW), [single]($Vy / $SceneH), [single]0,
		$nx, $ny, [single]0.05, [single]0.05, [single]0, [single]$Height)
	$address = if ($f -eq 0) { "/object/enter" } else { "/object/update" }
	$m = OscMessage $address $args15
	[void]$udp.Send($m, $m.Length)

	$age += [single](1.0 / $Rate)
	Start-Sleep -Milliseconds ([int](1000 / $Rate))
}

if (-not $NoLeave)
{
	$leave = OscMessage "/object/leave" @([int]$frames, [int]$Id, [int]0)
	[void]$udp.Send($leave, $leave.Length)
}

$udp.Close()
"sent $frames frames, id $Id at ($X, $Y) m"
