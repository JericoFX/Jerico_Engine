$ErrorActionPreference = 'Stop'
$archive = 'F:/JericoEngine/Dependencies/archives/antlr4-8e6fd9147b3c9d36b60e2b6656871a55227efb1b.tar.gz'
$destination = 'F:/JericoEngine/Dependencies/antlr4-8e6fd9147b3c9d36b60e2b6656871a55227efb1b'
$audit = $PSScriptRoot
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLower() -ne 'c52fb2a90ae082ab5b5ec6b41f6c0e66f691962e4b4f772580e5da678fe51fed') { throw 'Archive pin mismatch' }
if (Test-Path -LiteralPath $destination) { throw 'Immutable destination already exists' }
$records = [Collections.Generic.List[object]]::new()
$shaByPath = @{}
$files = [Collections.Generic.Dictionary[string,byte[]]]::new([StringComparer]::OrdinalIgnoreCase)
$paths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$input = [IO.File]::OpenRead($archive)
$gzip = [IO.Compression.GZipStream]::new($input,[IO.Compression.CompressionMode]::Decompress)
$reader = [System.Formats.Tar.TarReader]::new($gzip)
try {
    while ($null -ne ($entry = $reader.GetNextEntry())) {
        if ($entry.EntryType.ToString() -eq 'GlobalExtendedAttributes') {
            if ($entry.Name -ne 'pax_global_header' -or $records.Count -ne 0) { throw 'Unexpected global PAX record' }
            $records.Add([ordered]@{path=$entry.Name;type='GlobalExtendedAttributes';bytes=0;sha256=$null})
            continue
        }
        $name = $entry.Name.TrimEnd('/')
        if ($name -notmatch '^antlr4-8e6fd9147b3c9d36b60e2b6656871a55227efb1b(?:/|$)' -or $name -match '\\|:|(^|/)\.\.?(/|$)' -or $name.StartsWith('/')) { throw "Unsafe archive path: $name" }
        foreach ($part in $name.Split('/')) {
            if ($part -match '[. ]$|^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)') { throw "Unsafe Windows path: $name" }
        }
        if (-not $paths.Add($name)) { throw "Duplicate/case collision: $name" }
        $kind = $entry.EntryType.ToString()
        if ($kind -notin @('Directory','RegularFile','V7RegularFile')) { throw "Unsupported archive type $kind for $name" }
        $sha = $null
        if ($kind -ne 'Directory') {
            $memory = [IO.MemoryStream]::new()
            if ($entry.Length -gt 0) { $entry.DataStream.CopyTo($memory) }
            $bytes = $memory.ToArray()
            if ($bytes.Length -ne $entry.Length) { throw "Archive size mismatch: $name" }
            $sha = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLower()
            $files.Add($name,$bytes)
            $shaByPath[$name] = $sha
            $memory.Dispose()
        }
        $records.Add([ordered]@{path=$name;type=$kind;bytes=$entry.Length;sha256=$sha})
    }
}
finally { $reader.Dispose(); $gzip.Dispose(); $input.Dispose() }
foreach ($name in $files.Keys) {
    $parent = [IO.Path]::GetDirectoryName($name.Replace('/',[IO.Path]::DirectorySeparatorChar))
    while ($parent) {
        if ($files.ContainsKey($parent.Replace('\','/'))) { throw "File/directory collision: $name" }
        $parent = [IO.Path]::GetDirectoryName($parent)
    }
}
$license = [Text.Encoding]::UTF8.GetString($files['antlr4-8e6fd9147b3c9d36b60e2b6656871a55227efb1b/LICENSE.txt'])
if ($license -notmatch 'Redistribution and use in source and binary forms' -or $license -notmatch 'Neither name of copyright holders') { throw 'Unexpected LICENSE' }
$tree = Get-Content (Join-Path $audit 'source-tree.json') -Raw | ConvertFrom-Json
if ($tree.truncated) { throw 'Primary tree truncated' }
$blobs = @{}
foreach ($item in $tree.tree) { if ($item.type -eq 'blob') { $blobs[$item.path] = $item.sha } }
$matched = 0
foreach ($name in $files.Keys) {
    $relative = $name.Substring('antlr4-8e6fd9147b3c9d36b60e2b6656871a55227efb1b/'.Length)
    if (-not $blobs.ContainsKey($relative)) { throw "Archive file absent from primary Git tree: $relative" }
    $bytes = $files[$name]
    $prefix = [Text.Encoding]::UTF8.GetBytes("blob $($bytes.Length)`0")
    $blob = [Convert]::ToHexString([Security.Cryptography.SHA1]::HashData([byte[]]($prefix + $bytes))).ToLower()
    if ($blob -ne $blobs[$relative]) { throw "Primary Git blob mismatch: $relative" }
    $matched++
}
if ($matched -ne $blobs.Count) { throw 'Archive / tree file count mismatch' }
[IO.Directory]::CreateDirectory($destination) | Out-Null
foreach ($name in $files.Keys) {
    $relative = $name.Substring('antlr4-8e6fd9147b3c9d36b60e2b6656871a55227efb1b/'.Length)
    $path = Join-Path $destination $relative
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path)) | Out-Null
    [IO.File]::WriteAllBytes($path,$files[$name])
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLower() -ne $shaByPath[$name]) { throw "Extraction hash mismatch: $name" }
}
$result = [ordered]@{archive=$archive;archiveSHA256='c52fb2a90ae082ab5b5ec6b41f6c0e66f691962e4b4f772580e5da678fe51fed';entries=$records.Count;files=$files.Count;fileBytes=(($records|ForEach-Object{$_.bytes})|Measure-Object -Sum).Sum;links=0;linksMaterialized=0;pathCaseCollisions=0;destination=$destination;extractionHashesMatched=$true;license=$license;gitBlobMatches=$matched;primaryTreeFiles=$blobs.Count;snapshotScope="complete safe archive";members=$records}
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $audit 'source-tar-audit.json') -Encoding utf8
$result | Select-Object archive,entries,files,fileBytes,links,destination,extractionHashesMatched | ConvertTo-Json
