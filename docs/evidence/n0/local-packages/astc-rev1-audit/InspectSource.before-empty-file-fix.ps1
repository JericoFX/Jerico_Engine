#Requires -Version 7.0
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=$PSScriptRoot
$pin='f32d8d4dd9e06f8986cd5e10f12e199fc023f150'
$archive="F:/JericoEngine/Dependencies/archives/astc-encoder-$pin.tar.gz"
$snapshot="F:/JericoEngine/Dependencies/astc-encoder-$pin"
$prefix="astc-encoder-$pin/"
$encoding=[Text.UTF8Encoding]::new($false)
function Hash([string]$path){$s=[IO.File]::OpenRead($path);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($s))}finally{$s.Dispose()}}
function Write-NewJson([string]$path,$value){if(Test-Path -LiteralPath $path){throw 'Retain previous receipt'};[IO.File]::WriteAllText($path,($value|ConvertTo-Json -Depth 12)+"`n",$encoding)}
if((Hash $archive) -ne '291C26FCF20B8E23A6EB1AD4DD75F5AFECF79D316C0477C532DF6D5693291403'){throw 'Archive differs from acquired exact pin'}
if(Test-Path -LiteralPath $snapshot){throw 'Snapshot exists; preserve it'}
$tree=Get-Content -LiteralPath (Join-Path $root 'upstream-tree.json') -Raw|ConvertFrom-Json
if($tree.truncated){throw 'Primary tree incomplete'}
$blobs=[Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal)
foreach($item in $tree.tree){if($item.type -eq 'blob'){$blobs.Add($item.path,$item.sha)}}
$seen=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$files=[Collections.Generic.List[object]]::new();$directories=[Collections.Generic.List[string]]::new();$violations=[Collections.Generic.List[string]]::new();$memberCount=0;$metadata=0;$totalBytes=0L
$stream=[IO.File]::OpenRead($archive);$gzip=[IO.Compression.GZipStream]::new($stream,[IO.Compression.CompressionMode]::Decompress);$tar=[System.Formats.Tar.TarReader]::new($gzip)
try{while($entry=$tar.GetNextEntry()){
    $memberCount++
    if($entry.EntryType -eq [System.Formats.Tar.TarEntryType]::GlobalExtendedAttributes){
        if($metadata -ne 0 -or $entry.Name -ne 'pax_global_header' -or $entry.GlobalExtendedAttributes.Count -ne 1 -or $entry.GlobalExtendedAttributes['comment'] -cne $pin){throw 'Unexpected PAX metadata'}
        $metadata++;continue
    }
    if(-not $entry.Name.StartsWith($prefix,[StringComparison]::Ordinal)){throw 'Archive root differs'}
    $relative=$entry.Name.Substring($prefix.Length).TrimEnd('/')
    if($entry.Name -match '\\|:|^/' -or ($relative -and @($relative.Split('/')|Where-Object{$_ -in @('.','..','') -or $_ -match '[<>:"|?*]|[. ]$|^(?i:CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)'}).Count)){throw 'Unsafe member path'}
    if($relative -and -not $seen.Add($relative)){throw 'Case-insensitive archive collision'}
    if($entry.EntryType -eq [System.Formats.Tar.TarEntryType]::Directory){$directories.Add($relative);continue}
    if($entry.EntryType -notin @([System.Formats.Tar.TarEntryType]::RegularFile,[System.Formats.Tar.TarEntryType]::V7RegularFile)){throw 'Link/special file rejected'}
    if(-not $blobs.ContainsKey($relative)){throw "Archive path absent from primary tree: $relative"}
    $memory=[IO.MemoryStream]::new()
    try{$entry.DataStream.CopyTo($memory);$bytes=$memory.ToArray()}finally{$memory.Dispose()}
    if($bytes.Length -ne $entry.Length){throw 'Truncated tar data'}
    $git=[Security.Cryptography.IncrementalHash]::CreateHash([Security.Cryptography.HashAlgorithmName]::SHA1)
    try{$git.AppendData([Text.Encoding]::ASCII.GetBytes("blob $($bytes.Length)`0"));$git.AppendData($bytes);$blob=[Convert]::ToHexString($git.GetHashAndReset()).ToLowerInvariant()}finally{$git.Dispose()}
    if($blob -cne $blobs[$relative]){$violations.Add("Raw Gitblob mismatch: $relative")}
    $files.Add(@{path=$relative;member=$entry.Name;bytes=$entry.Length;sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes));gitBlob=$blob;primaryGitBlob=$blobs[$relative];rawGitBlobMatch=($blob -ceq $blobs[$relative])})
    $totalBytes+=$entry.Length
}}finally{$tar.Dispose();$gzip.Dispose();$stream.Dispose()}
$missing=@($blobs.Keys|Where-Object{-not $seen.Contains($_)})
if($missing.Count){$violations.Add("Missing primary blobs: $($missing -join ', ')")}
$audit=@{archive=$archive;sha256=(Hash $archive);sourceCommit=$pin;tag='3.2';members=$memberCount;files=$files.Count;directories=$directories.Count;paxComment=$pin;metadataEntries=$metadata;fileBytes=$totalBytes;links=0;primaryBlobs=$blobs.Count;gitlinks=@($tree.tree|Where-Object type -eq commit|Select-Object path,sha);missing=$missing;violations=@($violations);entries=@($files);status=if($violations.Count){'FAIL; NO EXTRACTION'}else{'AUDITED; EXTRACTION ALLOWED'};nativeExecuted=$false}
Write-NewJson (Join-Path $root 'source-tar-audit.json') $audit
if($violations.Count){throw 'Source archive Gitblob closure failed; audit preserved'}
[void][IO.Directory]::CreateDirectory($snapshot)
$expected=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal)
foreach($file in $files){$expected.Add($file.path,$file)}
$stream=[IO.File]::OpenRead($archive);$gzip=[IO.Compression.GZipStream]::new($stream,[IO.Compression.CompressionMode]::Decompress);$tar=[System.Formats.Tar.TarReader]::new($gzip)
try{while($entry=$tar.GetNextEntry()){
    if($entry.EntryType -eq [System.Formats.Tar.TarEntryType]::GlobalExtendedAttributes){continue}
    $relative=$entry.Name.Substring($prefix.Length).TrimEnd('/')
    $path=[IO.Path]::GetFullPath((Join-Path $snapshot $relative))
    if($path -ne [IO.Path]::GetFullPath($snapshot) -and -not $path.StartsWith([IO.Path]::GetFullPath($snapshot)+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Extraction containment failed'}
    if($entry.EntryType -eq [System.Formats.Tar.TarEntryType]::Directory){[void][IO.Directory]::CreateDirectory($path);continue}
    [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path))
    $out=[IO.File]::Open($path,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
    try{$entry.DataStream.CopyTo($out)}finally{$out.Dispose()}
    if(([IO.FileInfo]::new($path)).Length -ne $expected[$relative].bytes -or (Hash $path) -ne $expected[$relative].sha256){throw 'Extracted raw bytes differ'}
}}finally{$tar.Dispose();$gzip.Dispose();$stream.Dispose()}
$actual=@(Get-ChildItem -LiteralPath $snapshot -Recurse -File)
if($actual.Count -ne $files.Count){throw 'Extracted file membership differs'}
$manifest=@(foreach($file in $files){@{path=[IO.Path]::GetFullPath((Join-Path $snapshot $file.path));relative=$file.path;bytes=$file.bytes;sha256=$file.sha256;gitBlob=$file.gitBlob}})
Write-NewJson (Join-Path $root 'source-extraction.json') @{sourceRoot=$snapshot;archiveSHA=(Hash $archive);files=$files.Count;bytes=$totalBytes;allRawHashesMatch=$true;gitBlobMatches=$files.Count;nativeExecuted=$false;manifest=$manifest;googleTest='gitlink not acquired; UNITTEST=OFF matches recipe default';testImageAssets='preserved only as immutable source input; not packaged'}
@{snapshot=$snapshot;files=$files.Count;bytes=$totalBytes;sha256=(Hash $archive);gitBlobMatches=$files.Count;nativeExecuted=$false}|ConvertTo-Json
