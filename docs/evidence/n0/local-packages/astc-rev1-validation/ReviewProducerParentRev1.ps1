$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$engine = 'F:\JericoEngine\JericoEngine'
$local = Join-Path $engine 'build\n0-validation\local-packages'
$prep = Join-Path $local 'astc-rev1-preparation'
$producer = Join-Path $local 'astc-local-rev1'
function HashFile([string]$Path) {
    $stream = [IO.File]::OpenRead($Path)
    try { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream)) }
    finally { $stream.Dispose() }
}
$launch = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'luna-launch-rev1.json') -Raw | ConvertFrom-Json
$result = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'attempt1/result.json') -Raw | ConvertFrom-Json
$freezePath = Join-Path $prep 'source-freeze-astc-candidate.json'
$freeze = @(Get-Content -LiteralPath $freezePath -Raw | ConvertFrom-Json)
if ($result.exitCode -ne 0 -or -not $result.producerOnly -or $launch.attempt1ExistedBeforeLaunch -or $launch.producerOutputExistedBeforeLaunch -or (HashFile $freezePath) -cne $launch.freezeSHA256 -or (HashFile (Join-Path $prep 'RunASTCProducer.ps1')) -cne $launch.driverSHA256) { throw 'Native launch/result attribution differs' }
if ($freeze.Count -ne 556) { throw 'Freeze coverage changed' }
foreach ($entry in $freeze) {
    if ((HashFile $entry.path) -cne $entry.sha256 -or (Get-Item -LiteralPath $entry.path).Length -ne $entry.bytes) { throw 'Frozen native input differs' }
}
foreach ($phase in @('preflight', 'postflight')) {
    $check = Get-Content -LiteralPath (Join-Path $PSScriptRoot "attempt1/$phase.json") -Raw | ConvertFrom-Json
    if ($check.count -ne 556 -or @($check.deltas).Count) { throw 'Native input delta found' }
}
$phases = @()
foreach ($phase in @('configure', 'build-Release', 'build-Debug')) {
    $record = [IO.File]::ReadAllText((Join-Path $producer "$phase-result.txt"))
    if ($record -notmatch '(?m)^result=0\r?$' -or (Get-Item -LiteralPath (Join-Path $producer "$phase-stderr.log")).Length -ne 0) { throw 'Native phase failed' }
    $phases += [pscustomobject]@{ phase = $phase; result = $record; argvSHA256 = HashFile (Join-Path $producer "$phase-argv.txt"); stdoutSHA256 = HashFile (Join-Path $producer "$phase-stdout.log"); stderrSHA256 = HashFile (Join-Path $producer "$phase-stderr.log") }
}
$hunks = Get-Content -LiteralPath (Join-Path $engine 'cmake/Tools/LocalPackages/ASTCEncoder/PatchHunks.json') -Raw | ConvertFrom-Json
foreach ($patch in $hunks.files) {
    $private = Join-Path $producer ('source-work/astc-encoder-' + $hunks.sourceCommit + '/' + $patch.path)
    if ((HashFile $private) -cne $patch.afterSHA) { throw 'Private patch output differs' }
}
$archive = Join-Path $producer 'packages/astc-encoder-3.2-je-rev1-windows.tar.xz'
if ((HashFile $archive) -cne '4EED3F722E8F4CCA60BF14684F22FA351D0D59960ED518BE04CCE30C707949CA' -or (Get-Item -LiteralPath $archive).Length -ne 1179220) { throw 'Produced archive differs' }
$sidecar = [IO.File]::ReadAllText($archive + '.SHA256SUMS').Trim()
if ($sidecar -cne '4eed3f722e8f4cca60bf14684f22fa351d0d59960ed518be04cce30c707949ca *astc-encoder-3.2-je-rev1-windows.tar.xz') { throw 'Archive sidecar differs' }
$receipt = [ordered]@{
    decision = 'PRODUCER ACCEPTED FOR INDEPENDENT ARCHIVE AUDIT AND CONSUMER VALIDATION'
    launcherSHA256 = HashFile (Join-Path $PSScriptRoot 'luna-launch-rev1.json')
    nativeResultSHA256 = HashFile (Join-Path $PSScriptRoot 'attempt1/result.json')
    sourceFreezeSHA256 = HashFile $freezePath
    sourceFreezeEntries = $freeze.Count
    currentFrozenDeltas = 0
    nativeExit = $result.exitCode
    nativeSeconds = $result.seconds
    phases = $phases
    privatePatchExact = $true
    archiveBytes = 1179220
    archiveSHA256 = HashFile $archive
    warnings = 'Two Release D9025 messages are the original reviewed /GL- override; Debug and configure did not emit that warning.'
    archiveIndependentAudit = 'PENDING'
    consumers = 'NOT_RUN'
    guards = 'NOT_RUN'
    catalogIntegrated = $false
    engineAccepted = $false
}
$target = Join-Path $PSScriptRoot 'parent-producer-review-rev1.json'
$stream = [IO.File]::Open($target, [IO.FileMode]::CreateNew)
try { $json = [Text.UTF8Encoding]::new($false).GetBytes(($receipt | ConvertTo-Json -Depth 8) + "`n"); $stream.Write($json, 0, $json.Length) }
finally { $stream.Dispose() }
[pscustomobject]@{ decision = $receipt.decision; nativeExit = $result.exitCode; receiptSHA256 = HashFile $target } | ConvertTo-Json
