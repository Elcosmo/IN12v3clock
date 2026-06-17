$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$dockerRepoRoot = $repoRoot

function Invoke-Native {
    param(
        [Parameter(Mandatory = $true)][string]$Command,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    Write-Host "==> $Command $($Arguments -join ' ')"
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Command failed with exit code $LASTEXITCODE"
    }
}

function Invoke-HostTest {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$TestCommand
    )

    Write-Host "==> $Name"
    Invoke-Native docker @(
        "run", "--rm",
        "-v", "${dockerRepoRoot}:/work",
        "-w", "/work",
        "gcc:13",
        "sh", "-c", $TestCommand
    )
}

Set-Location $repoRoot

Invoke-HostTest "schedule" "gcc -std=c99 -Wall -Wextra -Werror -I. tests/test_schedule.c -o /tmp/test_schedule && /tmp/test_schedule"
Invoke-HostTest "ds3231" "gcc -std=c99 -Wall -Wextra -Werror -Itests/mock -I. tests/test_ds3231.c ds3231.c -o /tmp/test_ds3231 && /tmp/test_ds3231"
Invoke-HostTest "display gate" "gcc -std=c99 -Wall -Wextra -Itests/mock_display -I. tests/test_display_gate.c display.c -o /tmp/test_display_gate && /tmp/test_display_gate"
Invoke-HostTest "hc595 safe boot" "gcc -std=c99 -Wall -Wextra -Werror -Itests/mock_display -I. tests/test_hc595_init.c hc595.c -o /tmp/test_hc595_init && /tmp/test_hc595_init"
Invoke-HostTest "iface rtc recovery" "gcc -std=c99 -Wall -Wextra -Itests/mock_display -I. tests/test_iface_rtc_recovery.c iface.c -o /tmp/test_iface_rtc_recovery && /tmp/test_iface_rtc_recovery"

Invoke-Native docker @("compose", "build")
Invoke-Native docker @("compose", "run", "--rm", "sdcc", "make", "clean")
Invoke-Native docker @("compose", "run", "--rm", "sdcc", "make")

Invoke-HostTest "S19 validation" "gcc -std=c99 -Wall -Wextra -Werror tests/test_s19.c -o /tmp/test_s19 && /tmp/test_s19 SDCC/main.s19"

Write-Host "==> firmware size"
$areas = @{}
Get-Content (Join-Path $repoRoot "SDCC/main.map") | ForEach-Object {
    if ($_ -match '^\s*(DATA|INITIALIZED|HOME|GSINIT|GSFINAL|CONST|INITIALIZER|CODE)\s+\S+\s+\S+\s+=\s+([0-9]+)\.') {
        if (-not $areas.ContainsKey($matches[1])) {
            $areas[$matches[1]] = [int]$matches[2]
        }
    }
}

$flash = 0
foreach ($name in @("HOME", "GSINIT", "GSFINAL", "CONST", "INITIALIZER", "CODE")) {
    if ($areas.ContainsKey($name)) {
        $flash += $areas[$name]
    }
}

$ram = 0
foreach ($name in @("DATA", "INITIALIZED")) {
    if ($areas.ContainsKey($name)) {
        $ram += $areas[$name]
    }
}

Write-Host ("Flash: {0} / 8192 bytes" -f $flash)
Write-Host ("RAM: {0} / 1024 bytes" -f $ram)

if ($flash -gt 8192) {
    throw "Flash exceeds STM8S003F3P6 Program Memory"
}

Write-Host "PASS firmware CI checks"
