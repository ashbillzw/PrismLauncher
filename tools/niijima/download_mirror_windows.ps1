[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Output,
    [ValidateRange(1,4)][int]$Parallel=4,
    [ValidateRange(30,1800)][int]$TimeoutSec=120
)
# PowerShell 7+: use the Windows/.NET network route, preserving upstream hash checks.
$ErrorActionPreference='Stop'
$ProgressPreference='SilentlyContinue'
$outputRoot=[IO.Path]::GetFullPath($Output)
$payloadRoot=Join-Path $outputRoot 'publish'
$manifestPath=Join-Path $outputRoot 'download-manifest.json'
$manifest=Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$marker=Join-Path $outputRoot 'download-complete.json'
if (Test-Path -LiteralPath $marker) { Remove-Item -LiteralPath $marker }
$pending=@()
foreach ($record in $manifest.files) {
    $source=[Uri]$record.source
    if ($source.Scheme -ne 'https' -or $source.UserInfo -or $source.Fragment) { throw "Unsupported source URL: $source" }
    $target=[IO.Path]::GetFullPath((Join-Path $payloadRoot $record.path))
    if (-not $target.StartsWith($payloadRoot.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe path' }
    if ((Test-Path -LiteralPath $target) -and (Get-Item -LiteralPath $target).Length -eq $record.size -and (Get-FileHash -LiteralPath $target -Algorithm SHA1).Hash -eq $record.sha1) { continue }
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target)) | Out-Null
    $pending += $record
}
Write-Output ('Verified cache: '+($manifest.file_count-$pending.Count)+'; pending: '+$pending.Count)
$cancel=[Threading.CancellationTokenSource]::new()
try {
    $results=@($pending | ForEach-Object -ThrottleLimit $Parallel -Parallel {
        $ErrorActionPreference='Stop'
        $ProgressPreference='SilentlyContinue'
        $cancel=$using:cancel
        $record=$_
        if ($cancel.IsCancellationRequested) { return }
        $target=[IO.Path]::GetFullPath((Join-Path $using:payloadRoot $record.path))
        $part=$target+'.net-part'
        try {
            Invoke-WebRequest -Uri $record.source -OutFile $part -TimeoutSec $using:TimeoutSec -MaximumRetryCount 0 -ErrorAction Stop
            if ((Get-Item -LiteralPath $part).Length -ne $record.size -or (Get-FileHash -LiteralPath $part -Algorithm SHA1).Hash -ne $record.sha1) { throw 'SHA1/size mismatch' }
            Move-Item -LiteralPath $part -Destination $target -Force
            Write-Host ('OK '+$record.path)
            [PSCustomObject]@{path=$record.path;source=$record.source;ok=$true}
        } catch {
            $cancel.Cancel()
            Write-Warning ('FAILED '+$record.source+': '+$_.Exception.Message)
            [PSCustomObject]@{path=$record.path;source=$record.source;ok=$false;error=$_.Exception.Message}
        }
    })
} finally { $cancel.Dispose() }
$results | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $outputRoot 'net-download-results.json') -Encoding utf8
if (@($results | Where-Object { -not $_.ok }).Count -or $results.Count -ne $pending.Count) { throw 'Download incomplete. Verified files were retained; no success marker written.' }
@{files=$manifest.file_count;total_bytes=$manifest.total_bytes;base_url=$manifest.base_url;manifest_sha256=(Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash.ToLowerInvariant();verification='Every listed file matched upstream SHA1 and byte length.'} | ConvertTo-Json | Set-Content -LiteralPath $marker -Encoding utf8
Write-Output ('COMPLETE: '+$manifest.file_count+' verified files / '+$manifest.total_bytes+' bytes. URL configuration may still be required.')
exit 0
