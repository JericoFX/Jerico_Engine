$ErrorActionPreference='Stop'
$audit='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/dxc-current-audit'
$root=Join-Path $PSScriptRoot 'acquisition'
$inputs=Get-Content -LiteralPath "$root/acquisition-summary.json" -Raw|ConvertFrom-Json
$results=@()
foreach($input in $inputs){
 $dir=Join-Path $root $input.id
 try{
  if($input.status -ne 'ACQUIRED_PENDING_TAR_AUDIT'){throw 'Acquisition did not pass'}
  $receipt=Get-Content -LiteralPath "$dir/source-acquisition.json" -Raw|ConvertFrom-Json
  if((Get-FileHash -LiteralPath $receipt.archivePath).Hash.ToLowerInvariant() -ne $receipt.sha256){throw 'Archive changed'}
  $tree=Get-Content -LiteralPath "$dir/primary-git-tree.json" -Raw|ConvertFrom-Json -Depth 100
  if($tree.truncated){throw 'Primary tree is truncated'}
  $primary=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal)
  foreach($entry in $tree.tree){if($entry.type -eq 'blob'){$primary.Add($entry.path,$entry)}}
  $gitlinks=@($tree.tree|Where-Object type -eq commit)
  if($input.id -eq 'dxc'){
   $expected=@{'external/DirectX-Headers'='980971e835876dc0cde415e8f9bc646e64667bf7';'external/SPIRV-Headers'='aa6cef192b8e693916eb713e7a9ccadf06062ceb';'external/SPIRV-Tools'='a62abcb402009b9ca5975e6167c09f237f630e0e'}
   if($gitlinks.Count -ne 3){throw 'Unexpected source gitlink count'}
   foreach($link in $gitlinks){if($expected[$link.path] -ne $link.sha){throw 'Unexpected gitlink identity'}}
  }elseif($gitlinks.Count){throw 'Unexpected transitive gitlink'}
  $rootName="$($receipt.repository.Split('/')[1])-$($receipt.commit)"
  $entries=[Collections.Generic.List[object]]::new();$violations=[Collections.Generic.List[object]]::new();$pax=[Collections.Generic.List[object]]::new()
  $names=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
  $seenFiles=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
  $stream=[IO.File]::OpenRead($receipt.archivePath);$gzip=[IO.Compression.GZipStream]::new($stream,[IO.Compression.CompressionMode]::Decompress);$tar=[System.Formats.Tar.TarReader]::new($gzip)
  try{
   while($null -ne ($e=$tar.GetNextEntry())){
    if($e.EntryType -eq [System.Formats.Tar.TarEntryType]::GlobalExtendedAttributes){
     $attrs=$e.GlobalExtendedAttributes
     if($e.Name -ne 'pax_global_header' -or $attrs.Count -ne 1 -or $attrs['comment'] -ne $receipt.commit -or $pax.Count -ne 0){throw 'Unexpected global PAX metadata'}
     $pax.Add(@{name=$e.Name;type=[string]$e.EntryType;comment=$attrs['comment'];materialized=$false});continue
    }
    $name=$e.Name.TrimEnd('/');$kind=[string]$e.EntryType
    $rel=if($name -eq $rootName){''}elseif($name.StartsWith("$rootName/",[StringComparison]::Ordinal)){$name.Substring($rootName.Length+1)}else{$null}
    $why=@()
    if($null -eq $rel){$why+='Wrong archive root'}
    if($name -match '(^/|\\|:|[<>"|?*\x00-\x1f])' -or ($name.Split('/')|Where-Object{$_ -in @('..','.') -or $_ -match '[ .]$|^(?i:CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(\.|$)'})){$why+='Unsafe Windows relative path'}
    if(-not $names.Add($name)){$why+='Case-insensitive duplicate/collision'}
    if($kind -notin @('Directory','RegularFile','V7RegularFile')){$why+="Disallowed type: $kind"}
    if($why.Count){$violations.Add(@{path=$e.Name;type=$kind;reasons=$why})}
    $r=[ordered]@{path=$rel;archivePath=$e.Name;type=$kind;bytes=$e.Length}
    if($kind -in @('RegularFile','V7RegularFile')){
     $ms=[IO.MemoryStream]::new();if($e.Length -gt 0){$e.DataStream.CopyTo($ms)};[byte[]]$bytes=$ms.ToArray();$ms.Dispose()
     if($bytes.Length -ne $e.Length){throw 'Truncated entry'}
     $r.sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
     [byte[]]$header=[Text.Encoding]::ASCII.GetBytes("blob $($bytes.Length)`0");[byte[]]$blob=[byte[]]::new($header.Length+$bytes.Length);[Array]::Copy($header,0,$blob,0,$header.Length);[Array]::Copy($bytes,0,$blob,$header.Length,$bytes.Length)
     $r.measuredGitBlobSHA1=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData($blob)).ToLowerInvariant()
     [void]$seenFiles.Add($rel)
     $r.primaryGitBlobMatch=$primary.ContainsKey($rel) -and $primary[$rel].sha -eq $r.measuredGitBlobSHA1 -and $primary[$rel].size -eq $r.bytes
     if(-not $r.primaryGitBlobMatch){$violations.Add(@{path=$rel;reasons=@('Primary Gitblob missing/different')})}
    }
    $entries.Add([pscustomobject]$r)
   }
  }finally{$tar.Dispose();$gzip.Dispose();$stream.Dispose()}
  $missing=@($primary.Keys|Where-Object{-not $seenFiles.Contains($_)}|Sort-Object)
  foreach($path in $missing){$violations.Add(@{path=$path;reasons=@('Primary blob absent from archive; no export-ignore exception approved')})}
  $textsRoot=if($input.id -eq 'dxc'){"$audit/texts"}else{"$audit/externals/$($input.id)"}
  $byPath=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal)
  foreach($entry in $entries){if($entry.type -ne 'Directory'){$byPath.Add($entry.path,$entry)}}
  $critical=@()
  foreach($text in Get-ChildItem -LiteralPath $textsRoot -Recurse -File){
   $rel=[IO.Path]::GetRelativePath($textsRoot,$text.FullName).Replace('\','/')
   if($input.id -eq 'spirv-headers' -and $rel -eq 'spirv.hpp'){$rel='include/spirv/unified1/spirv.hpp'}
   $sha=(Get-FileHash -LiteralPath $text.FullName).Hash.ToLowerInvariant()
   $match=$byPath.ContainsKey($rel) -and $byPath[$rel].sha256 -eq $sha
   $critical+=@{path=$rel;expectedSHA256=$sha;archiveMatch=$match}
   if(-not $match){$violations.Add(@{path=$rel;reasons=@('Pinned primary text differs/missing')})}
  }
  $files=@($entries|Where-Object type -ne Directory)
  $record=[ordered]@{repository=$receipt.repository;commit=$receipt.commit;archiveSHA256=$receipt.sha256;root=$rootName;entryCount=$entries.Count;fileCount=$files.Count;fileBytes=($files|Measure-Object bytes -Sum).Sum;linkCount=@($entries|Where-Object type -match Link).Count;paxMetadata=@($pax);violations=@($violations);primaryBlobCount=$primary.Count;primaryGitBlobMatches=@($files|Where-Object primaryGitBlobMatch).Count;primaryOmissions=$missing;gitlinks=$gitlinks;pinnedPrimaryTextComparisons=$critical;entries=@($entries);gitlinkSemantics='Commit nodes are not blob payloads in codeload archive; exact three declared nodes recorded, two selected Windows sources acquired independently. No link materialization.'}
  if(Test-Path -LiteralPath "$dir/source-tar-audit.json"){throw 'Audit evidence exists'}
  [IO.File]::WriteAllText("$dir/source-tar-audit.json",($record|ConvertTo-Json -Depth 20),[Text.UTF8Encoding]::new($false))
  if($violations.Count){throw "Tar has $($violations.Count) violations; not extracted"}
  $snapshot=Join-Path $dir 'snapshot';if(Test-Path -LiteralPath $snapshot){throw 'Snapshot exists'};[IO.Directory]::CreateDirectory($snapshot)|Out-Null
  $stream=[IO.File]::OpenRead($receipt.archivePath);$gzip=[IO.Compression.GZipStream]::new($stream,[IO.Compression.CompressionMode]::Decompress);$tar=[System.Formats.Tar.TarReader]::new($gzip)
  try{
   while($null -ne ($e=$tar.GetNextEntry())){
    if($e.EntryType -eq [System.Formats.Tar.TarEntryType]::GlobalExtendedAttributes){if($e.Name -ne 'pax_global_header' -or $e.GlobalExtendedAttributes.Count -ne 1 -or $e.GlobalExtendedAttributes['comment'] -ne $receipt.commit){throw 'Unexpected PAX extraction metadata'};continue}
    $name=$e.Name.TrimEnd('/');if($name -eq $rootName){continue};$rel=$name.Substring($rootName.Length+1);$dest=Join-Path $snapshot $rel
    if($e.EntryType -eq [System.Formats.Tar.TarEntryType]::Directory){[IO.Directory]::CreateDirectory($dest)|Out-Null;continue}
    if($e.EntryType -notin @([System.Formats.Tar.TarEntryType]::RegularFile,[System.Formats.Tar.TarEntryType]::V7RegularFile)){throw 'Nonregular file reached extraction'}
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($dest))|Out-Null;$out=[IO.File]::Open($dest,[IO.FileMode]::CreateNew)
    try{if($e.Length -gt 0){$e.DataStream.CopyTo($out)}}finally{$out.Dispose()}
   }
  }finally{$tar.Dispose();$gzip.Dispose();$stream.Dispose()}
  foreach($file in $files){$path=Join-Path $snapshot $file.path;if((Get-Item -LiteralPath $path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant() -ne $file.sha256){throw "Extracted hash mismatch: $path"}}
  $ex=[ordered]@{snapshotPath=$snapshot;fileCount=$files.Count;verified=$files.Count;files=@($files|ForEach-Object{@{path=(Join-Path $snapshot $_.path);relativePath=$_.path;bytes=$_.bytes;sha256=$_.sha256;gitBlobSHA1=$_.measuredGitBlobSHA1}});immutableIntent='Never modify this snapshot; future approved producer must copy it'}
  [IO.File]::WriteAllText("$dir/source-extraction.json",($ex|ConvertTo-Json -Depth 15),[Text.UTF8Encoding]::new($false))
  $results+=@{id=$input.id;status='TAR_SAFE_FULL_GITBLOB_EXTRACT_VERIFIED';snapshotPath=$snapshot;fileCount=$files.Count;primaryMatches=$record.primaryGitBlobMatches;pinnedTextCount=$critical.Count;sha256=$receipt.sha256}
 }catch{$results+=@{id=$input.id;status='FAILED_PRESERVED';error=$_.Exception.Message}}
}
$summary="$root/tar-extraction-summary.json";if(Test-Path -LiteralPath $summary){throw 'Summary exists'}
[IO.File]::WriteAllText($summary,($results|ConvertTo-Json -Depth 10),[Text.UTF8Encoding]::new($false))
$results|ConvertTo-Json
