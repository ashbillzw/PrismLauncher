param(
    [Parameter(Mandatory = $true)]
    [string]$BuildScript,
    [string[]]$BuildArguments = @()
)

# Launch with powershell.exe -File <this script> -BuildScript <build script>.
# Enter an interactive prompt only when a key cancels the countdown.
try {
    & $BuildScript @BuildArguments
} catch {
    Write-Host ($_ | Out-String) -ForegroundColor Red
}

# Discard keys entered during the build; only countdown input keeps the window.
while ([Console]::KeyAvailable) { $null = [Console]::ReadKey($true) }
Write-Host 'Build finished. Closing in 15 seconds; press any key to keep this window open.'
$countdown = [Diagnostics.Stopwatch]::StartNew()
while ($countdown.Elapsed.TotalSeconds -lt 15) {
    if ([Console]::KeyAvailable) {
        $null = [Console]::ReadKey($true)
        Write-Host "`rWindow kept open. Close it when finished.                       "
        $Host.EnterNestedPrompt()
        return
    }
    $remaining = [Math]::Ceiling(15 - $countdown.Elapsed.TotalSeconds)
    Write-Host ("`rClosing in {0,2} seconds; press any key to keep open." -f $remaining) -NoNewline
    Start-Sleep -Milliseconds 100
}
Write-Host ''
exit
