$ErrorActionPreference='Stop'
$Audit=$PSScriptRoot
$Commit='a40189cf881e9f0db80511c382292a5604c3c3d1'
$Archive=Join-Path $Audit "libpng-$Commit.tar.gz"
$Root="F:/JericoEngine/Dependencies/libpng-$Commit"
$Prefix="libpng-$Commit/"
$Expected='ED747514984FDDF095142AA85E983111F13D64B877B1065ED0D990F7ADB31EB7'
if((Get-FileHash -LiteralPath $Archive -Algorithm SHA256).Hash -ne $Expected){throw 'Archive pin mismatch before extraction'}
if(Test-Path -LiteralPath $Root){throw 'Immutable destination exists; inspect and preserve'}
$ancestor=[IO.Path]::GetFullPath($Root)
while($ancestor){if(Test-Path -LiteralPath $ancestor){if((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Reparse ancestor'}};$next=[IO.Path]::GetDirectoryName($ancestor);if($next -eq $ancestor){break};$ancestor=$next}
Add-Type -AssemblyName System.Formats.Tar
$fs=[IO.File]::OpenRead($Archive);$gzip=[IO.Compression.GZipStream]::new($fs,[IO.Compression.CompressionMode]::Decompress);$tar=[Formats.Tar.TarReader]::new($gzip)
$seen=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$files=[Collections.Generic.List[object]]::new();$entries=[Collections.Generic.List[object]]::new();$dirs=0;$metadata=0;$bytesTotal=0L
try{
 while($entry=$tar.GetNextEntry($false)){
  $type=$entry.EntryType.ToString();$name=$entry.Name
  if($type -eq 'GlobalExtendedAttributes'){$metadata++;$entries.Add([ordered]@{path=$name;type=$type});continue}
  if($type -notin @('Directory','RegularFile','V7RegularFile')){throw "Forbidden tar type: $type $name"}
  if(-not $name.StartsWith($Prefix,[StringComparison]::Ordinal)){throw "Unexpected archive root: $name"}
  $relative=$name.Substring($Prefix.Length).TrimEnd('/')
  if($relative -eq ''){if($type -ne 'Directory'){throw 'Root is not directory'};$entries.Add([ordered]@{path=$name;type=$type});continue}
  if($relative -match '[\\:\x00-\x1f]' -or $relative.StartsWith('/')){throw "Unsafe path: $relative"}
  foreach($part in $relative.Split('/')){if($part -in @('','..','.','.git') -or $part -match '[. ]$|^(?i:CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)'){throw "Unsafe component: $relative"}}
  if(-not $seen.Add($relative)){throw "Case/path collision: $relative"}
  if($type -eq 'Directory'){$dirs++;$entries.Add([ordered]@{path=$relative;type=$type});continue}
  $mem=[IO.MemoryStream]::new();$entry.DataStream.CopyTo($mem);$blob=$mem.ToArray();$mem.Dispose()
  if($blob.LongLength -ne $entry.Length){throw "Length mismatch: $relative"}
  if($blob.Length -ge 43 -and [Text.Encoding]::ASCII.GetString($blob,0,[Math]::Min(128,$blob.Length)).StartsWith('version https://git-lfs.github.com/spec/v1')){throw "LFS pointer: $relative"}
  $hash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($blob));$bytesTotal+=$blob.LongLength
  $files.Add([pscustomobject]@{path=$relative;bytes=$blob.LongLength;sha256=$hash;blob=$blob})
  $entries.Add([ordered]@{path=$relative;type=$type;bytes=$blob.LongLength;sha256=$hash})
 }
}finally{$tar.Dispose();$gzip.Dispose();$fs.Dispose()}
$entries|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $Audit 'archive-manifest-before-extraction.json') -Encoding utf8
$files|Select-Object path,bytes,sha256|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $Audit 'source-manifest-before-extraction.json') -Encoding utf8
[IO.Directory]::CreateDirectory($Root)|Out-Null
foreach($file in $files){$destination=Join-Path $Root $file.path;[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination))|Out-Null;[IO.File]::WriteAllBytes($destination,$file.blob);if((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -ne $file.sha256){throw 'Extracted hash mismatch'};[IO.File]::SetAttributes($destination,[IO.FileAttributes]::ReadOnly)}
$actual=@(Get-ChildItem -LiteralPath $Root -File -Recurse -Force)
if($actual.Count -ne $files.Count){throw 'Extracted count mismatch'}
$manifest=[ordered]@{root=$Root;archive=$Archive;archiveSha256=$Expected;archiveBytes=(Get-Item -LiteralPath $Archive).Length;archiveMembers=$entries.Count;directories=$dirs;metadata=$metadata;files=$files.Count;payloadBytes=$bytesTotal;links=0;collisions=0;lfsPointers=0;readOnlyFiles=$true;sourceMatchesArchive=$true;entries=@($files|Select-Object path,bytes,sha256)}
$manifest|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $Audit 'immutable-source-manifest.json') -Encoding utf8
$manifest|Select-Object root,archiveSha256,archiveBytes,archiveMembers,directories,metadata,files,payloadBytes,links,collisions,lfsPointers,sourceMatchesArchive|ConvertTo-Json