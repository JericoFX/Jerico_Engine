$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$proofPath = Join-Path $PSScriptRoot 'source-eol-attributes-proof-v3.json'
$proof = Get-Content -LiteralPath $proofPath -Raw | ConvertFrom-Json
$tree = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'upstream-tree.json') -Raw | ConvertFrom-Json
$snapshot = 'F:\JericoEngine\Dependencies\ispc-texcomp-36b80aac50ea401a9adde44d86712c2241b2acfd'
$archive = 'F:\JericoEngine\Dependencies\archives\ispc-texcomp-36b80aac50ea401a9adde44d86712c2241b2acfd.tar.gz'
function FileSha256([string]$Path) {
    $stream = [IO.File]::OpenRead($Path)
    try { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream)) }
    finally { $stream.Dispose() }
}
function BlobSha1([byte[]]$Bytes) {
    $hash = [Security.Cryptography.IncrementalHash]::CreateHash([Security.Cryptography.HashAlgorithmName]::SHA1)
    try {
        $hash.AppendData([Text.Encoding]::ASCII.GetBytes("blob $($Bytes.Length)" + [char]0))
        $hash.AppendData($Bytes)
        [Convert]::ToHexString($hash.GetHashAndReset()).ToLowerInvariant()
    } finally { $hash.Dispose() }
}
if ((FileSha256 $proofPath) -cne 'DFA03D0B9E11BBFF63668830EE454B97E01E95817AFC36085B437467348A7234') { throw 'Proof changed' }
if ($tree.truncated -or $tree.sha -cne $proof.sourceCommit -or $proof.snapshotMutation) { throw 'Unacceptable primary tree or mutation' }
if ((FileSha256 $archive) -cne $proof.sourceArchiveSHA256 -or (Get-Item -LiteralPath $archive).Length -ne $proof.sourceArchiveBytes) { throw 'Archive differs' }
$blobs = @{}
foreach ($blob in $tree.tree | Where-Object type -eq 'blob') { $blobs.Add($blob.path, $blob) }
if ($blobs.Count -ne 238 -or @($proof.entries).Count -ne 238) { throw 'Unexpected source file coverage' }
$attributes = @{}
foreach ($attr in $proof.attributes) {
    $bytes = [IO.File]::ReadAllBytes((Join-Path $snapshot $attr.path))
    if ((BlobSha1 $bytes) -cne $attr.gitBlobSHA1 -or (FileSha256 (Join-Path $snapshot $attr.path)) -cne $attr.sha256) { throw 'Committed attributes differ' }
    $rules = @([Text.Encoding]::UTF8.GetString($bytes).Split("`n") | Where-Object { $_ -match '^\*\.[^ ]+ eol=crlf$' })
    if (@(Compare-Object $rules $attr.rules).Count) { throw 'Rule receipt differs from committed bytes' }
    $attributes.Add($attr.path, $attr)
}
$seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$raw = 0
$normalized = 0
$rows = @()
foreach ($entry in $proof.entries) {
    if (-not $seen.Add($entry.path) -or -not $blobs.ContainsKey($entry.path)) { throw 'Duplicate or missing primary source' }
    $path = [IO.Path]::GetFullPath((Join-Path $snapshot $entry.path))
    if (-not $path.StartsWith($snapshot + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Source path escapes snapshot' }
    $info = Get-Item -LiteralPath $path
    if ($info.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Source is a reparse point' }
    $bytes = [IO.File]::ReadAllBytes($path)
    $blob = $blobs[$entry.path]
    if ($bytes.Length -ne $entry.rawBytes -or (FileSha256 $path) -cne $entry.rawSHA256 -or (BlobSha1 $bytes) -cne $entry.rawGitBlobSHA1 -or $blob.sha -cne $entry.primaryGitBlobSHA1 -or $blob.size -ne $entry.primaryBytes) { throw "Raw/primary mismatch: $($entry.path)" }
    if ($entry.normalizationApplied) {
        if (-not $attributes.ContainsKey($entry.attributePath)) { throw 'Unattributed normalization' }
        $attr = $attributes[$entry.attributePath]
        $pattern = $entry.attributeRule.Split(' ')[0]
        if (-not $entry.path.StartsWith($attr.prefix, [StringComparison]::Ordinal) -or [IO.Path]::GetFileName($path) -cnotlike $pattern -or $entry.attributeRule -notin $attr.rules -or $entry.attributeGitBlobSHA1 -cne $attr.gitBlobSHA1) { throw 'Inapplicable committed EOL rule' }
        $buffer = [IO.MemoryStream]::new()
        try {
            for ($i = 0; $i -lt $bytes.Length; $i++) {
                if ($bytes[$i] -eq 13 -and $i + 1 -lt $bytes.Length -and $bytes[$i + 1] -eq 10) { continue }
                $buffer.WriteByte($bytes[$i])
            }
            $comparison = $buffer.ToArray()
        } finally { $buffer.Dispose() }
        if ($comparison.Length -ne $blob.size -or (BlobSha1 $comparison) -cne $blob.sha -or [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($comparison)) -cne $entry.normalizedSHA256 -or $bytes.Length - $comparison.Length -ne $entry.removedCRBytes) { throw 'Difference is not exclusively attributed CR before LF' }
        $normalized++
    } else {
        if ($entry.rawGitBlobSHA1 -cne $blob.sha -or $bytes.Length -ne $blob.size) { throw 'Exact source differs' }
        $raw++
    }
    $rows += [pscustomobject]@{ path = $entry.path; bytes = $bytes.Length; rawSHA256 = $entry.rawSHA256; primaryGitBlobSHA1 = $blob.sha; comparisonOnlyCRLF = [bool]$entry.normalizationApplied }
}
$actual = @(Get-ChildItem -LiteralPath $snapshot -Recurse -Force -File)
if ($actual.Count -ne $seen.Count -or $raw -ne 72 -or $normalized -ne 166 -or @($proof.libraryFiles).Count -ne 8 -or @($proof.libraryFiles | Where-Object normalizationApplied).Count) { throw 'Source coverage or owning library changed' }
$receipt = [ordered]@{
    decision = 'SOURCE PROVENANCE ACCEPTED FOR PRODUCER PREPARATION ONLY'
    proofSHA256 = FileSha256 $proofPath
    sourceCommit = $proof.sourceCommit
    sourceArchiveSHA256 = FileSha256 $archive
    sourceFiles = $seen.Count
    rawExact = $raw
    attributedCRLFComparisonOnly = $normalized
    owningLibraryRawExact = 8
    snapshotMutated = $false
    nativeExecuted = $false
    packageAccepted = $false
    limitation = 'Committed rules and exact remaining bytes verified; archive export mechanism is not established. No bytes were rewritten.'
    entries = $rows
}
$target = Join-Path $PSScriptRoot 'parent-source-eol-review-rev1.json'
$stream = [IO.File]::Open($target, [IO.FileMode]::CreateNew)
try {
    $json = [Text.UTF8Encoding]::new($false).GetBytes(($receipt | ConvertTo-Json -Depth 8) + "`n")
    $stream.Write($json, 0, $json.Length)
} finally { $stream.Dispose() }
[pscustomobject]@{ decision = $receipt.decision; sourceFiles = $seen.Count; rawExact = $raw; CRLFComparisonOnly = $normalized; receipt = $target; sha256 = FileSha256 $target } | ConvertTo-Json
