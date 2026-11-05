# make_psn_session.ps1 - writes the session used by check_psn.ps1, from a copy of a session
#
#   powershell -ExecutionPolicy Bypass -File make_psn_session.ps1 -From TEST_ZONES.noisette -To TEST_PSN.noisette
#
# Text replacement only, the original is not touched : replaces its sources by Augmenta and 2 PSN sources
# on ports no real system uses (-NoPsn : none, for check_sim.ps1, PSN output off).
# With PSN sources, the PSN output is on, on Loopback only.
# PSN Z is marked Use As Surface (-NoChoice : no source is).

# -OutputMode : Send of the PSN output, for check_psn_modes.ps1
# -DefaultIds : Augmenta keeps its default Id Offset, for check_id_offset.ps1 (else 0, so the ids in the checks are the ids sent)
# -Surfaces : Augmenta (SOL) and Augmenta 2 (MUR), PSN output on, for check_psn_surfaces.ps1 ; -MurSlotsOff : MUR without player slots

param([string]$From, [string]$To, [switch]$NoPsn, [switch]$NoChoice, [switch]$DefaultIds, [switch]$Surfaces, [switch]$MurSlotsOff, [string]$OutputMode = "Player Slots")

$text = [System.IO.File]::ReadAllText($From)

# OSC on a port Fusion does not use (augmenta_sim.ps1 sends there) ; source Augmenta on that port = the module input
if ($text.Contains('"oscInput": {"parameters"')) { throw "OSC input settings in $From : not handled" }
$text = $text.Replace('"oscInput": {', '"oscInput": {"parameters": [{"value": 12999, "hexMode": false, "controlAddress": "/localPort"}], ')
$augmenta = if ($DefaultIds) { '{"parameters": [{"value": 12999, "hexMode": false, "controlAddress": "/port"}], "niceName": "Augmenta", "type": "Augmenta"}' } else { '{"parameters": [{"value": 12999, "hexMode": false, "controlAddress": "/port"}, {"value": 0, "hexMode": false, "controlAddress": "/idOffset"}], "niceName": "Augmenta", "type": "Augmenta"}' }
$psnY = '{"parameters": [{"value": [1.0, 0.5], "controlAddress": "/offset"}, {"value": 56575, "hexMode": false, "controlAddress": "/multicastPort"}], "niceName": "PSN Y", "type": "PSN"}'
$psnZ = '{"parameters": [{"value": [-8.0, -2.0], "controlAddress": "/areaPosition"}, {"value": [16.0, 12.0], "controlAddress": "/areaSize"}, {"value": 56576, "hexMode": false, "controlAddress": "/multicastPort"}, {"value": "Z", "controlAddress": "/upAxis"}], "niceName": "PSN Z", "type": "PSN"}'
if (-not $NoChoice) { $psnZ = $psnZ.Replace('{"parameters": [', '{"parameters": [{"value": true, "controlAddress": "/useAsSurface"}, ') }
# module-wide slots and video output of the sessions before surfaces : SOL gets its own below
$text = [regex]::Replace($text, '"playerSlots": \{"parameters": \[[^\]]*\](, "editorIsCollapsed": true)?, "slots": \[(\[[^\]]*\](, )?)*\]\}, ', '')
$text = [regex]::Replace($text, '"videoOutput": \{"parameters": \[[^\]]*?(\[[^\]]*\][^\]]*?)*\](, "editorIsCollapsed": true)?\}, ', '')
# the surfaces of the session are replaced by 2 known ones : SOL (Auto, 2 slots JOUEUR 1 / 2, Spout on) and MUR (Manual 8 x 3)
$slots = '"playerSlots": {"parameters": [{"value": 2, "hexMode": false, "controlAddress": "/slotCount"}, {"value": "Next Person Takes Slot", "controlAddress": "/slotRelease"}], "slots": [["JOUEUR 1", "ffff0000"], ["JOUEUR 2", "ff0000ff"]]}'
$sol = '{"niceName": "SOL", "type": "Surface", "containers": {' + $slots + ', "videoOutput": {"parameters": [{"value": true, "controlAddress": "/spout"}]}}}'
$mur = '{"parameters": [{"value": "Manual", "controlAddress": "/sizeMode"}, {"value": [8.0, 3.0], "controlAddress": "/size"}], "niceName": "MUR", "type": "Surface"}'
if ($MurSlotsOff) { $mur = $mur.Replace('"type": "Surface"}', '"type": "Surface", "containers": {"playerSlots": {"parameters": [{"value": false, "controlAddress": "/enabled"}]}}}') }
$surfacesPattern = '(?s)"(surfaces|scenes)": {("editorIsCollapsed": true, )?"items": \[.*?\], "viewOffset": \[[^\]]*\], "viewZoom": [0-9.]+\}, '
$surfacesJson = '"surfaces": {"items": [' + $sol + ', ' + $mur + ']}, '
if ([regex]::IsMatch($text, $surfacesPattern)) { $text = [regex]::Replace($text, $surfacesPattern, { param($m) $surfacesJson }) }
else { $text = $text.Replace('"sources": {', $surfacesJson + '"sources": {') }

# the sources of the session are replaced : its PSN source would hear the real Kratos
# -NoPsn : a 2nd Fusion scene on its own port (12998, no real system), Id Offset 1000, placed 8 m to the right, on MUR
$augmenta2 = '{"parameters": [{"value": "/mur", "controlAddress": "/surface"}, {"value": 12998, "hexMode": false, "controlAddress": "/port"}, {"value": 1000, "hexMode": false, "controlAddress": "/idOffset"}, {"value": [8.0, 0.0], "controlAddress": "/offset"}], "niceName": "Augmenta 2", "type": "Augmenta"}'
$items = if ($NoPsn -or $Surfaces) { "$augmenta, $augmenta2" } else { "$augmenta, $psnY, $psnZ" }
$pattern = '(?s)("sources": \{[^\[]*?"items": \[).*?(\], "viewOffset")'
if (-not [regex]::IsMatch($text, $pattern)) { throw "Sources not found in $From" }
$text = [regex]::Replace($text, $pattern, { param($m) $m.Groups[1].Value + $items + $m.Groups[2].Value })
# the PSN output of the session is replaced too : with -NoPsn it stays off, its default
$text = [regex]::Replace($text, '"psnOutput": \{[^{}]*(\{[^{}]*\}[^{}]*)*\}, ', '')
if (-not $NoPsn) { $text = $text.Replace('"sources": {', '"psnOutput": {"parameters": [{"value": true, "controlAddress": "/enabled"}, {"value": "' + $OutputMode + '", "controlAddress": "/send"}, {"value": "Loopback Pseudo-Interface 1 (127.0.0.1)", "controlAddress": "/networkInterface"}]}, "sources": {') }
# the checks read the Resolution of the video output, computed only while Spout or NDI is on : Spout on in every copy
if (-not $text.Contains('{"value": false, "controlAddress": "/spout"}') -and -not $text.Contains('{"value": true, "controlAddress": "/spout"}')) { throw "Spout setting not found in $From" }
$text = $text.Replace('{"value": false, "controlAddress": "/spout"}', '{"value": true, "controlAddress": "/spout"}')
# check_psn.ps1 tests a held slot
if (-not $NoPsn) { $text = $text.Replace('{"value": "Next Person Takes Slot", "controlAddress": "/slotRelease"}', '{"value": "Hold Spot To Reclaim", "controlAddress": "/slotRelease"}') }

[System.IO.File]::WriteAllText($To, $text)
"written $To"
