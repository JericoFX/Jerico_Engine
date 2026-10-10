$ErrorActionPreference = 'Stop'
$jeAuditPath = Join-Path $PSScriptRoot 'python-wheelhouse-audit.json'
$jeAudit = Get-Content -LiteralPath $jeAuditPath -Raw | ConvertFrom-Json
if ((Get-FileHash -LiteralPath $jeAuditPath -Algorithm SHA256).Hash -ne '846792188E3B957B4D4F8EB9A41D74ACD4BAE87AC516DD104AE1E2AFC24600E8') {
    throw 'Reviewed acquisition manifest changed'
}
$jeArtifacts = @($jeAudit.base_artifacts) + @($jeAudit.build_tools)
if ($jeArtifacts.Count -ne 44) { throw 'Expected 43 retained pins and one build tool' }
$jeDestination = 'F:\JericoEngine\Dependencies\python-wheelhouse'
if (Test-Path -LiteralPath $jeDestination) { throw 'Acquisition destination already exists' }
foreach ($jeArtifact in $jeArtifacts) {
    $jeUri = [Uri]$jeArtifact.url
    if ($jeUri.Scheme -ne 'https' -or $jeUri.Host -ne 'files.pythonhosted.org' -or $jeUri.Query -or $jeUri.UserInfo) { throw 'Unreviewed artifact origin' }
    if ($jeArtifact.name -in @('boto3', 'botocore', 's3transfer')) { throw 'AWS package excluded' }
    if ($jeArtifact.filename -ne [IO.Path]::GetFileName($jeUri.AbsolutePath) -or $jeArtifact.filename -match '[:\\/]' -or $jeArtifact.filename -in @('.', '..')) { throw 'Unsafe artifact filename' }
    if ($jeArtifact.sha256 -notmatch '^[a-f0-9]{64}$') { throw 'Invalid artifact SHA256' }
}
$jeAncestor = [IO.DirectoryInfo]'F:\JericoEngine\Dependencies'
while ($null -ne $jeAncestor) {
    if ($jeAncestor.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Destination ancestor is a reparse point' }
    $jeAncestor = $jeAncestor.Parent
}
$jeHandler = [Net.Http.HttpClientHandler]::new()
$jeHandler.AllowAutoRedirect = $false
$jeClient = [Net.Http.HttpClient]::new($jeHandler)
$jeClient.Timeout = [TimeSpan]::FromMinutes(5)
$jeHeads = @()
$jeReceipts = @()
try {
    # Every destination is audited before acquiring any binary payload.
    foreach ($jeArtifact in $jeArtifacts) {
        $jeRequest = [Net.Http.HttpRequestMessage]::new([Net.Http.HttpMethod]::Head, $jeArtifact.url)
        $jeResponse = $jeClient.SendAsync($jeRequest, [Net.Http.HttpCompletionOption]::ResponseHeadersRead).GetAwaiter().GetResult()
        try {
            if ([int]$jeResponse.StatusCode -ne 200 -or $jeResponse.Headers.Location) { throw ('Unexpected HEAD/redirect: ' + $jeArtifact.filename) }
            $jeHeads += [pscustomobject]@{filename=$jeArtifact.filename; url=$jeArtifact.url; status=200; bytes=$jeResponse.Content.Headers.ContentLength; redirects_followed=0}
        } finally { $jeResponse.Dispose(); $jeRequest.Dispose() }
    }
    $jeHeads | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'python-wheelhouse-destination-head.json') -Encoding utf8
    [IO.Directory]::CreateDirectory($jeDestination) | Out-Null
    foreach ($jeArtifact in $jeArtifacts) {
        $jeTimer = [Diagnostics.Stopwatch]::StartNew()
        $jePath = Join-Path $jeDestination $jeArtifact.filename
        $jeResponse = $jeClient.GetAsync($jeArtifact.url, [Net.Http.HttpCompletionOption]::ResponseHeadersRead).GetAwaiter().GetResult()
        try {
            if ([int]$jeResponse.StatusCode -ne 200 -or $jeResponse.Headers.Location) { throw ('Unexpected GET/redirect: ' + $jeArtifact.filename) }
            $jeStream = [IO.File]::Open($jePath, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
            try { $jeResponse.Content.CopyToAsync($jeStream).GetAwaiter().GetResult() } finally { $jeStream.Dispose() }
        } finally { $jeResponse.Dispose() }
        $jeHash = (Get-FileHash -LiteralPath $jePath -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($jeHash -ne $jeArtifact.sha256) { throw ('Artifact SHA256 mismatch: ' + $jeArtifact.filename) }
        $jeTimer.Stop()
        $jeReceipts += [pscustomobject]@{name=$jeArtifact.name; version=$jeArtifact.version; file=$jePath; url=$jeArtifact.url; status=200; redirects_followed=0; sha256=$jeHash; bytes=(Get-Item -LiteralPath $jePath).Length; elapsed_seconds=$jeTimer.Elapsed.TotalSeconds; installed=$false}
        $jeReceipts | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'python-wheelhouse-acquisition.json') -Encoding utf8
    }
    [pscustomobject]@{files=$jeReceipts.Count; bytes=($jeReceipts | Measure-Object bytes -Sum).Sum; all_sha256_match=$true; aws_packages=0; redirects_followed=0; installed=$false} | ConvertTo-Json -Compress
} finally { $jeClient.Dispose(); $jeHandler.Dispose() }
