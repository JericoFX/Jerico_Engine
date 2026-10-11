$ErrorActionPreference = 'Stop'
$archive = 'F:/JericoEngine/Dependencies/archives/mcpp-2.7.2.tar.gz'
$destination = 'F:/JericoEngine/Dependencies/mcpp-2.7.2'
$audit = $PSScriptRoot
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLower() -ne '3b9b4421888519876c4fc68ade324a3bbd81ceeb7092ecdbbc2055099fcb8864') { throw 'Archive pin mismatch' }
if (Test-Path -LiteralPath $destination) { throw 'Immutable destination already exists' }
$records = [Collections.Generic.List[object]]::new()
$files = [Collections.Generic.Dictionary[string,byte[]]]::new([StringComparer]::OrdinalIgnoreCase)
$paths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$input = [IO.File]::OpenRead($archive)
$gzip = [IO.Compression.GZipStream]::new($input,[IO.Compression.CompressionMode]::Decompress)
$reader = [System.Formats.Tar.TarReader]::new($gzip)
try {
    while ($null -ne ($entry = $reader.GetNextEntry())) {
        $name = $entry.Name.TrimEnd('/')
        if ($name -notmatch '^mcpp-2\.7\.2(?:/|$)' -or $name -match '\\|:|(^|/)\.\.?(/|$)' -or $name.StartsWith('/')) { throw "Unsafe archive path: $name" }
        foreach ($part in $name.Split('/')) {
            if ($part -match '[. ]$|^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)') { throw "Unsafe Windows path: $name" }
        }
        if (-not $paths.Add($name)) { throw "Duplicate/case collision: $name" }
        $kind = $entry.EntryType.ToString()
        if ($kind -notin @('Directory','RegularFile','V7RegularFile')) { throw "Unsupported archive type $kind for $name" }
        $sha = $null
        if ($kind -ne 'Directory') {
            $memory = [IO.MemoryStream]::new()
            $entry.DataStream.CopyTo($memory)
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
$license = [Text.Encoding]::UTF8.GetString($files['mcpp-2.7.2/LICENSE'])
if ($license -notmatch 'Kiyoshi Matsui' -or $license -notmatch 'Redistribution and use in source and binary forms') { throw 'Unexpected LICENSE' }
[IO.Directory]::CreateDirectory($destination) | Out-Null
foreach ($name in $files.Keys) {
    $relative = $name.Substring('mcpp-2.7.2/'.Length)
    $path = Join-Path $destination $relative
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path)) | Out-Null
    [IO.File]::WriteAllBytes($path,$files[$name])
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLower() -ne ($records | Where-Object path -CEQ $name).sha256) { throw "Extraction hash mismatch: $name" }
}
$result = [ordered]@{archive=$archive;archiveSHA256='3b9b4421888519876c4fc68ade324a3bbd81ceeb7092ecdbbc2055099fcb8864';entries=$records.Count;files=$files.Count;fileBytes=(($records|ForEach-Object{$_.bytes})|Measure-Object -Sum).Sum;links=0;pathCaseCollisions=0;destination=$destination;extractionHashesMatched=$true;license=$license;members=$records}
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $audit 'mcpp-source-tar-audit.json') -Encoding utf8
$result | Select-Object archive,entries,files,fileBytes,links,destination,extractionHashesMatched | ConvertTo-Json
