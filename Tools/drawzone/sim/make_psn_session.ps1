# make_psn_session.ps1 - writes the session used by check_psn.ps1, from a copy of a session
#
#   powershell -ExecutionPolicy Bypass -File make_psn_session.ps1 -From TEST_ZONES.noisette -To TEST_PSN.noisette
#
# Text replacement only, the original is not touched : replaces its sources by Augmenta and 2 PSN sources
# on ports no real system uses (-NoPsn : none, for check_sim.ps1, PSN output off).
# With PSN sources, the PSN output is on, on Loopback only.
# PSN Z is marked Use As Scene (-NoChoice : no source is).

# -OutputMode : Send of the PSN output, for check_psn_modes.ps1
# -DefaultIds : Augmenta keeps its default Id Offset, for check_id_offset.ps1 (else 0, so the ids in the checks are the ids sent)

param([string]$From, [string]$To, [switch]$NoPsn, [switch]$NoChoice, [switch]$DefaultIds, [string]$OutputMode = "Player Slots")

$text = [System.IO.File]::ReadAllText($From)

# OSC on a port Fusion does not use (augmenta_sim.ps1 sends there)
if ($text.Contains('"oscInput": {"parameters"')) { throw "OSC input settings in $From : not handled" }
$text = $text.Replace('"oscInput": {', '"oscInput": {"parameters": [{"value": 12999, "hexMode": false, "controlAddress": "/localPort"}], ')
$augmenta = if ($DefaultIds) { '{"niceName": "Augmenta", "type": "Augmenta"}' } else { '{"parameters": [{"value": 0, "hexMode": false, "controlAddress": "/idOffset"}], "niceName": "Augmenta", "type": "Augmenta"}' }
$psnY = '{"parameters": [{"value": [1.0, 0.5], "controlAddress": "/offset"}, {"value": 56575, "hexMode": false, "controlAddress": "/multicastPort"}], "niceName": "PSN Y", "type": "PSN"}'
$psnZ = '{"parameters": [{"value": [-8.0, -2.0], "controlAddress": "/areaPosition"}, {"value": [16.0, 12.0], "controlAddress": "/areaSize"}, {"value": 56576, "hexMode": false, "controlAddress": "/multicastPort"}, {"value": "Z", "controlAddress": "/upAxis"}], "niceName": "PSN Z", "type": "PSN"}'
if (-not $NoChoice) { $psnZ = $psnZ.Replace('{"parameters": [', '{"parameters": [{"value": true, "controlAddress": "/useAsScene"}, ') }
# the sources of the session are replaced : its PSN source would hear the real Kratos
$items = if ($NoPsn) { $augmenta } else { "$augmenta, $psnY, $psnZ" }
$pattern = '(?s)("sources": \{[^\[]*?"items": \[).*?(\], "viewOffset")'
if (-not [regex]::IsMatch($text, $pattern)) { throw "Sources not found in $From" }
$text = [regex]::Replace($text, $pattern, { param($m) $m.Groups[1].Value + $items + $m.Groups[2].Value })
# the PSN output of the session is replaced too : with -NoPsn it stays off, its default
$text = [regex]::Replace($text, '"psnOutput": \{[^{}]*(\{[^{}]*\}[^{}]*)*\}, ', '')
if (-not $NoPsn) { $text = $text.Replace('"sources": {', '"psnOutput": {"parameters": [{"value": true, "controlAddress": "/enabled"}, {"value": "' + $OutputMode + '", "controlAddress": "/send"}, {"value": "Loopback Pseudo-Interface 1 (127.0.0.1)", "controlAddress": "/networkInterface"}]}, "sources": {') }
# check_psn.ps1 tests a held slot
if (-not $NoPsn) { $text = $text.Replace('{"value": "Next Person Takes Slot", "controlAddress": "/slotRelease"}', '{"value": "Hold Spot To Reclaim", "controlAddress": "/slotRelease"}') }

[System.IO.File]::WriteAllText($To, $text)
"written $To"
