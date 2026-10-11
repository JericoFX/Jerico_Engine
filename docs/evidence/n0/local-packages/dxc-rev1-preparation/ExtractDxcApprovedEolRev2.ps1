$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = $PSScriptRoot
$dir = Join-Path $root 'acquisition/dxc'
$approvalPath = Join-Path $root 'parent-extraction-review-dxc-rev1.json'
$proofPath = Join-Path $dir 'archive-eol-export-proof.json'
if ((Get-FileHash -LiteralPath $approvalPath).Hash -ne 'A76243686C559E05FA2BE7B305C0557821821CB4C48C2E4AE57C952EAF9CFCBA') { throw 'Parent approval changed' }
if ((Get-FileHash -LiteralPath $proofPath).Hash -ne 'EAE85AECFBF6ACF5532BA296195872FD1E1D9C184213D3F3EA9458C808DE85FF') { throw 'EOL proof changed' }
$approval = Get-Content -LiteralPath $approvalPath -Raw | ConvertFrom-Json
$proof = Get-Content -LiteralPath $proofPath -Raw | ConvertFrom-Json
$receipt = Get-Content -LiteralPath (Join-Path $dir 'source-acquisition.json') -Raw | ConvertFrom-Json
$prior = Get-Content -LiteralPath (Join-Path $dir 'source-tar-audit.json') -Raw | ConvertFrom-Json -Depth 100
$tree = Get-Content -LiteralPath (Join-Path $dir 'primary-git-tree.json') -Raw | ConvertFrom-Json -Depth 100
if ((Get-FileHash -LiteralPath $receipt.archivePath).Hash.ToLowerInvariant() -ne $approval.archiveSHA256 -or $tree.truncated -or $prior.fileCount -ne 17851 -or $prior.linkCount -ne 0 -or $prior.primaryOmissions.Count -ne 0) { throw 'Historical archive/tree audit changed' }
$snapshot = Join-Path $dir 'snapshot'
$auditPath = Join-Path $dir 'source-tar-audit-rev2.json'
$extractionPath = Join-Path $dir 'source-extraction-rev2.json'
foreach ($path in @($snapshot, $auditPath, $extractionPath)) { if (Test-Path -LiteralPath $path) { throw "New output exists: $path" } }
$allowed = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal)
foreach ($p in $proof.paths) {
    if ($p.path -notin $approval.exactExceptionPaths -or -not $p.normalizedMatchesPrimary -or -not $p.inverseArchiveByteExact -or $p.bareCR -ne 0 -or $p.bareLF -ne 0) { throw 'Unapproved EOL exception' }
    $allowed.Add($p.path, $p)
}
if ($allowed.Count -ne 19 -or $approval.exactExceptionPaths.Count -ne 19) { throw 'Exact exception cardinality changed' }
foreach ($violation in $prior.violations) {
    if (-not $allowed.ContainsKey($violation.path) -or $violation.reasons.Count -ne 1 -or
        $violation.reasons[0] -notin @('Primary Gitblob missing/different', 'Pinned primary text differs/missing')) { throw 'Unapproved historical violation' }
}
if ($prior.violations.Count -ne 21) { throw 'Historical failed audit changed' }
$primary = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal)
foreach ($p in $tree.tree) { if ($p.type -eq 'blob') { $primary.Add($p.path, $p) } }
if ($primary.Count -ne 17851) { throw 'Primary blob count changed' }
$expected = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal)
foreach ($p in $prior.entries) { $expected.Add($p.archivePath.TrimEnd('/'), $p) }
$seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$files = [Collections.Generic.List[object]]::new()
$rawMatches = 0; $derivedMatches = 0; $paxCount = 0
$stream = [IO.File]::OpenRead($receipt.archivePath)
$gzip = [IO.Compression.GZipStream]::new($stream, [IO.Compression.CompressionMode]::Decompress)
$tar = [System.Formats.Tar.TarReader]::new($gzip)
try {
    while ($null -ne ($entry = $tar.GetNextEntry())) {
        if ($entry.EntryType -eq [System.Formats.Tar.TarEntryType]::GlobalExtendedAttributes) {
            if ($entry.Name -ne 'pax_global_header' -or $entry.GlobalExtendedAttributes.Count -ne 1 -or $entry.GlobalExtendedAttributes['comment'] -ne $receipt.commit -or ++$paxCount -ne 1) { throw 'Unexpected PAX metadata' }
            continue
        }
        $name = $entry.Name.TrimEnd('/')
        if (-not $expected.ContainsKey($name) -or -not $seen.Add($name)) { throw 'Unknown/duplicate tar path' }
        $old = $expected[$name]
        if ([string]$entry.EntryType -ne $old.type -or $entry.Length -ne $old.bytes -or $name -match '(^/|\\|:|[<>"|?*\x00-\x1f])' -or
            ($name.Split('/') | Where-Object { $_ -in @('..', '.') -or $_ -match '[ .]$|^(?i:CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(\.|$)' })) { throw 'Unsafe or changed entry' }
        if ($entry.EntryType -eq [System.Formats.Tar.TarEntryType]::Directory) { continue }
        if ($entry.EntryType -notin @([System.Formats.Tar.TarEntryType]::RegularFile, [System.Formats.Tar.TarEntryType]::V7RegularFile)) { throw 'Nonregular payload' }
        $memory = [IO.MemoryStream]::new()
        if ($entry.Length -gt 0) { $entry.DataStream.CopyTo($memory) }
        [byte[]]$bytes = $memory.ToArray(); $memory.Dispose()
        $sha = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
        if ($bytes.Length -ne $old.bytes -or $sha -ne $old.sha256) { throw 'Raw payload differs from historical audit' }
        $blobBytes = $bytes
        $derivation = 'RAW'
        if ($allowed.ContainsKey($old.path)) {
            $p = $allowed[$old.path]
            if ($sha -ne $p.archiveSHA256 -or $bytes.Length -ne $p.archiveBytes) { throw 'Exception raw identity changed' }
            $text = [Text.UTF8Encoding]::new($false, $true).GetString($bytes)
            $lf = $text.Replace("`r`n", "`n")
            if ($lf.Contains("`r") -or $lf.Replace("`n", "`r`n") -cne $text) { throw 'Not exact reversible CRLF export' }
            $blobBytes = [Text.UTF8Encoding]::new($false, $true).GetBytes($lf)
            if ($blobBytes.Length -ne $p.primaryBlobBytes) { throw 'Derived byte size changed' }
            $derivation = 'CRLF_TO_LF_PROOF_ONLY; extracted RAW bytes'
            ++$derivedMatches
        } else { ++$rawMatches }
        [byte[]]$header = [Text.Encoding]::ASCII.GetBytes("blob $($blobBytes.Length)`0")
        [byte[]]$blob = [byte[]]::new($header.Length + $blobBytes.Length)
        [Array]::Copy($header, 0, $blob, 0, $header.Length); [Array]::Copy($blobBytes, 0, $blob, $header.Length, $blobBytes.Length)
        $blobSha = [Convert]::ToHexString([Security.Cryptography.SHA1]::HashData($blob)).ToLowerInvariant()
        if (-not $primary.ContainsKey($old.path) -or $primary[$old.path].sha -ne $blobSha -or $primary[$old.path].size -ne $blobBytes.Length) { throw "Primary blob mismatch: $($old.path)" }
        $files.Add([ordered]@{ relativePath = $old.path; path = Join-Path $snapshot $old.path; bytes = $bytes.Length; sha256 = $sha; primaryGitBlobSHA1 = $blobSha; comparison = $derivation })
    }
} finally { $tar.Dispose(); $gzip.Dispose(); $stream.Dispose() }
if ($seen.Count -ne $expected.Count -or $files.Count -ne 17851 -or $rawMatches -ne 17832 -or $derivedMatches -ne 19 -or $paxCount -ne 1) { throw 'Full tar comparison incomplete' }
$audit = [ordered]@{ schemaVersion = 2; archiveSHA256 = $receipt.sha256; commit = $receipt.commit; historicalFailedAuditPreserved = $true; approvalPath = $approvalPath; approvalSHA256 = (Get-FileHash $approvalPath).Hash; entryCount = $seen.Count; fileCount = $files.Count; rawGitBlobMatches = $rawMatches; exactApprovedEolDerivedMatches = $derivedMatches; links = 0; omissions = 0; violations = @(); normalizedPayloadWritten = $false; files = @($files) }
[IO.File]::WriteAllText($auditPath, ($audit | ConvertTo-Json -Depth 15), [Text.UTF8Encoding]::new($false))
[IO.Directory]::CreateDirectory($snapshot) | Out-Null
$stream = [IO.File]::OpenRead($receipt.archivePath)
$gzip = [IO.Compression.GZipStream]::new($stream, [IO.Compression.CompressionMode]::Decompress)
$tar = [System.Formats.Tar.TarReader]::new($gzip)
try {
    while ($null -ne ($entry = $tar.GetNextEntry())) {
        if ($entry.EntryType -eq [System.Formats.Tar.TarEntryType]::GlobalExtendedAttributes) { continue }
        $old = $expected[$entry.Name.TrimEnd('/')]
        if ($old.path -eq '') { continue }
        $destination = Join-Path $snapshot $old.path
        if ($entry.EntryType -eq [System.Formats.Tar.TarEntryType]::Directory) { [IO.Directory]::CreateDirectory($destination) | Out-Null; continue }
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination)) | Out-Null
        $out = [IO.File]::Open($destination, [IO.FileMode]::CreateNew)
        try { if ($entry.Length -gt 0) { $entry.DataStream.CopyTo($out) } } finally { $out.Dispose() }
    }
} finally { $tar.Dispose(); $gzip.Dispose(); $stream.Dispose() }
foreach ($file in $files) {
    if ((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $file.path).Hash.ToLowerInvariant() -ne $file.sha256) { throw 'Extracted RAW bytes mismatch' }
}
if (@(Get-ChildItem -LiteralPath $snapshot -Recurse -File).Count -ne 17851) { throw 'Snapshot cardinality mismatch' }
[ordered]@{ snapshotPath = $snapshot; fileCount = 17851; verified = 17851; normalizedPayloadWritten = $false; rawGitBlobMatches = 17832; approvedDerivedMatches = 19; files = @($files); immutableIntent = 'Never modify; future producer must copy privately'; nativeExecuted = $false } | ConvertTo-Json -Depth 15 | Set-Content -LiteralPath $extractionPath -Encoding utf8
@($auditPath, $extractionPath) | ForEach-Object { [pscustomobject]@{ path = $_; bytes = (Get-Item -LiteralPath $_).Length; sha256 = (Get-FileHash -LiteralPath $_).Hash } } | ConvertTo-Json
