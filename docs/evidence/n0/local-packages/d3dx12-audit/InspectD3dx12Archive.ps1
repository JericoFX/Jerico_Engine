$ErrorActionPreference = 'Stop'
$archive = 'F:/JericoEngine/Dependencies/archives/o3de-3p-70f0b7b5257bf7cf935bf1cfd270e1b53ccead18.tar.gz'
$destination = 'F:/JericoEngine/Dependencies/d3dx12-headers-o3de-70f0b7b5'
$audit = $PSScriptRoot
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLower() -ne 'f69edb8bdf9d3dcd83cf296db31130f4d16c82efb9b4cdebd76c8035ccaa6042') { throw 'Archive pin mismatch' }
if (Test-Path -LiteralPath $destination) { throw 'Immutable destination already exists' }
$records = [Collections.Generic.List[object]]::new()
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
        if ($name -notmatch '^3p-package-source-70f0b7b5257bf7cf935bf1cfd270e1b53ccead18(?:/|$)' -or $name -match '\\|:|(^|/)\.\.?(/|$)' -or $name.StartsWith('/')) { throw "Unsafe archive path: $name" }
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
$license = [Text.Encoding]::UTF8.GetString($files['3p-package-source-70f0b7b5257bf7cf935bf1cfd270e1b53ccead18/package-system/d3dx12-windows/d3dx12/license.txt'])
if ($license -notmatch 'Microsoft Corporation' -or $license -notmatch 'Permission is hereby granted') { throw 'Unexpected package-system/d3dx12-windows/d3dx12/license.txt' }
$tree = Get-Content (Join-Path $audit 'source-tree.json') -Raw | ConvertFrom-Json
if ($tree.truncated) { throw 'Primary tree truncated' }
$blobs = @{}
foreach ($item in $tree.tree) { if ($item.type -eq 'blob') { $blobs[$item.path] = $item.sha } }
$matched = 0
foreach ($name in $files.Keys) {
    $relative = $name.Substring('3p-package-source-70f0b7b5257bf7cf935bf1cfd270e1b53ccead18/'.Length)
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
    $relative = $name.Substring('3p-package-source-70f0b7b5257bf7cf935bf1cfd270e1b53ccead18/'.Length)
    if (-not $relative.StartsWith('package-system/d3dx12-windows/', [StringComparison]::Ordinal)) { continue }
    $path = Join-Path $destination $relative
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path)) | Out-Null
    [IO.File]::WriteAllBytes($path,$files[$name])
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLower() -ne ($records | Where-Object path -CEQ $name).sha256) { throw "Extraction hash mismatch: $name" }
}
$result = [ordered]@{archive=$archive;archiveSHA256='f69edb8bdf9d3dcd83cf296db31130f4d16c82efb9b4cdebd76c8035ccaa6042';entries=$records.Count;files=$files.Count;fileBytes=(($records|ForEach-Object{$_.bytes})|Measure-Object -Sum).Sum;links=0;linksMaterialized=0;pathCaseCollisions=0;destination=$destination;extractionHashesMatched=$true;license=$license;gitBlobMatches=$matched;primaryTreeFiles=$blobs.Count;snapshotSubtree="package-system/d3dx12-windows/";subtreeLinks=0;subtreeFiles=@($files.Keys|Where-Object { $_ -clike "*/package-system/d3dx12-windows/*" }).Count;members=$records}
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $audit 'source-subtree-tar-audit.json') -Encoding utf8
$result | Select-Object archive,entries,files,fileBytes,links,destination,extractionHashesMatched | ConvertTo-Json
