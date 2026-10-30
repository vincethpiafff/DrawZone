# make_psn_session.ps1 - writes the session used by check_psn.ps1, from a copy of a session
#
#   powershell -ExecutionPolicy Bypass -File make_psn_session.ps1 -From TEST_ZONES.noisette -To TEST_PSN.noisette
#
# Text replacement only, the original is not touched : replaces its sources by Augmenta and 2 PSN sources
# on ports no real system uses (-NoPsn : none, for check_sim.ps1).
# PSN Z is marked Use As Scene (-NoChoice : no source is).

param([string]$From, [string]$To, [switch]$NoPsn, [switch]$NoChoice)

$text = [System.IO.File]::ReadAllText($From)

$augmenta = '{"niceName": "Augmenta", "type": "Augmenta"}'
$psnY = '{"parameters": [{"value": [1.0, 0.5], "controlAddress": "/offset"}, {"value": 56575, "hexMode": false, "controlAddress": "/multicastPort"}], "niceName": "PSN Y", "type": "PSN"}'
$psnZ = '{"parameters": [{"value": [-8.0, -2.0], "controlAddress": "/areaPosition"}, {"value": [16.0, 12.0], "controlAddress": "/areaSize"}, {"value": 56576, "hexMode": false, "controlAddress": "/multicastPort"}, {"value": "Z", "controlAddress": "/upAxis"}], "niceName": "PSN Z", "type": "PSN"}'
if (-not $NoChoice) { $psnZ = $psnZ.Replace('{"parameters": [', '{"parameters": [{"value": true, "controlAddress": "/useAsScene"}, ') }
# the sources of the session are replaced : its PSN source would hear the real Kratos
$items = if ($NoPsn) { $augmenta } else { "$augmenta, $psnY, $psnZ" }
$pattern = '(?s)("sources": \{[^\[]*?"items": \[).*?(\], "viewOffset")'
if (-not [regex]::IsMatch($text, $pattern)) { throw "Sources not found in $From" }
$text = [regex]::Replace($text, $pattern, { param($m) $m.Groups[1].Value + $items + $m.Groups[2].Value })

[System.IO.File]::WriteAllText($To, $text)
"written $To"
