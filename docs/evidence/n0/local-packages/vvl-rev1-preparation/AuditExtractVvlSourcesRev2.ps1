$ErrorActionPreference='Stop'
$audit='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vulkan-validationlayers-current-audit'
$acquisition=Join-Path $PSScriptRoot 'acquisition'
$inputs=Get-Content "$acquisition/acquisition-summary.json" -Raw|ConvertFrom-Json
$results=@()
foreach($input in $inputs){
 $dir=Join-Path $acquisition $input.id
 try{
  if($input.status -ne 'ACQUIRED_PENDING_TAR_AUDIT'){throw 'Acquisition did not pass'}
  $receipt=Get-Content "$dir/source-acquisition.json" -Raw|ConvertFrom-Json
  if((Get-FileHash $receipt.archivePath).Hash.ToLowerInvariant() -ne $receipt.sha256){throw 'Archive changed'}
  $rootName="$($receipt.repository.Split('/')[1])-$($receipt.commit)"
  $entries=[Collections.Generic.List[object]]::new();$violations=[Collections.Generic.List[object]]::new()
  $pax=[Collections.Generic.List[object]]::new();$names=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
  $stream=[IO.File]::OpenRead($receipt.archivePath);$gzip=[IO.Compression.GZipStream]::new($stream,[IO.Compression.CompressionMode]::Decompress);$tar=[System.Formats.Tar.TarReader]::new($gzip)
  try{
   while($null -ne ($e=$tar.GetNextEntry())){
    if($e.EntryType -eq [System.Formats.Tar.TarEntryType]::GlobalExtendedAttributes){
     $attrs=$e.GlobalExtendedAttributes
     if($e.Name -ne 'pax_global_header' -or $attrs.Count -ne 1 -or $attrs['comment'] -ne $receipt.commit -or $pax.Count -ne 0){throw 'Unexpected global PAX metadata'}
     $pax.Add(@{name=$e.Name;type=[string]$e.EntryType;comment=$attrs['comment'];materialized=$false});continue
    }
    $name=$e.Name.TrimEnd('/');$kind=[string]$e.EntryType;$relative=$(if($name -eq $rootName){''}elseif($name.StartsWith("$rootName/",[StringComparison]::Ordinal)){$name.Substring($rootName.Length+1)}else{$null})
    $why=@()
    if($null -eq $relative){$why+='Wrong archive root'}
    if($name -match '(^/|\\|:|[<>"|?*\x00-\x1f])' -or ($name.Split('/')|Where-Object{$_ -in @('..','.') -or $_ -match '[ .]$|^(?i:CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(\.|$)'})){$why+='Unsafe Windows relative path'}
    if(-not $names.Add($name)){$why+='Case-insensitive duplicate/collision'}
    if($kind -notin @('Directory','RegularFile','V7RegularFile')){$why+="Disallowed type: $kind"}
    if($why.Count){$violations.Add(@{path=$e.Name;type=$kind;reasons=$why})}
    $r=[ordered]@{path=$relative;archivePath=$e.Name;type=$kind;bytes=$e.Length}
    if($kind -in @('RegularFile','V7RegularFile')){
     $ms=[IO.MemoryStream]::new();if($e.Length -gt 0){$e.DataStream.CopyTo($ms)};[byte[]]$b=$ms.ToArray();$ms.Dispose()
     if($b.Length -ne $e.Length){throw 'Truncated entry'}
     $r.sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($b)).ToLowerInvariant()
     [byte[]]$h=[Text.Encoding]::ASCII.GetBytes("blob $($b.Length)`0");[byte[]]$gb=[byte[]]::new($h.Length+$b.Length);[Array]::Copy($h,0,$gb,0,$h.Length);[Array]::Copy($b,0,$gb,$h.Length,$b.Length)
     $r.measuredGitBlobSHA1=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData($gb)).ToLowerInvariant()
    }
    $entries.Add([pscustomobject]$r)
   }
  }finally{$tar.Dispose();$gzip.Dispose();$stream.Dispose()}
  $critical=@()
  foreach($text in Get-ChildItem -LiteralPath "$audit/texts/$($input.id)" -Recurse -File){
   $rel=[IO.Path]::GetRelativePath("$audit/texts/$($input.id)",$text.FullName).Replace('\','/')
   $found=@($entries|Where-Object path -EQ $rel)
   $sha=(Get-FileHash $text.FullName).Hash.ToLowerInvariant();$match=$found.Count -eq 1 -and $found[0].sha256 -eq $sha
   $critical+=@{path=$rel;expectedSHA256=$sha;archiveMatch=$match}
   if(-not $match){$violations.Add(@{path=$rel;reasons=@('Pinned primary text differs/missing')})}
  }
  $files=@($entries|Where-Object type -NE Directory)
  $auditReceipt=[ordered]@{repository=$receipt.repository;commit=$receipt.commit;archiveSHA256=$receipt.sha256;root=$rootName;entryCount=$entries.Count;fileCount=$files.Count;fileBytes=($files|Measure-Object bytes -Sum).Sum;linkCount=@($entries|Where-Object type -Match 'Link').Count;paxMetadata=@($pax);violations=@($violations);pinnedPrimaryTextComparisons=$critical;fullPrimaryGitTree='NOT YET AVAILABLE; measured Gitblobs are not claimed as primary-tree matches';entries=@($entries)}
  [IO.File]::WriteAllText("$dir/source-tar-audit-rev2.json",($auditReceipt|ConvertTo-Json -Depth 15),[Text.UTF8Encoding]::new($false))
  if($violations.Count){throw "Tar has $($violations.Count) violations; not extracted"}
  $snapshot=Join-Path $dir 'snapshot';if(Test-Path $snapshot){throw 'Snapshot exists'};[IO.Directory]::CreateDirectory($snapshot)|Out-Null
  $stream=[IO.File]::OpenRead($receipt.archivePath);$gzip=[IO.Compression.GZipStream]::new($stream,[IO.Compression.CompressionMode]::Decompress);$tar=[System.Formats.Tar.TarReader]::new($gzip)
  try{
   while($null -ne ($e=$tar.GetNextEntry())){
    if($e.EntryType -eq [System.Formats.Tar.TarEntryType]::GlobalExtendedAttributes){if($e.Name -ne 'pax_global_header' -or $e.GlobalExtendedAttributes.Count -ne 1 -or $e.GlobalExtendedAttributes['comment'] -ne $receipt.commit){throw 'Unexpected PAX during extraction'};continue};$name=$e.Name.TrimEnd('/');if($name -eq $rootName){continue};$rel=$name.Substring($rootName.Length+1);$dest=Join-Path $snapshot $rel
    if($e.EntryType -eq [System.Formats.Tar.TarEntryType]::Directory){[IO.Directory]::CreateDirectory($dest)|Out-Null;continue}
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($dest))|Out-Null;$out=[IO.File]::Open($dest,[IO.FileMode]::CreateNew)
    try{if($e.Length -gt 0){$e.DataStream.CopyTo($out)}}finally{$out.Dispose()}
   }
  }finally{$tar.Dispose();$gzip.Dispose();$stream.Dispose()}
  $verified=0
  foreach($file in $files){$path=Join-Path $snapshot $file.path;if((Get-Item $path).Length -ne $file.bytes -or (Get-FileHash $path).Hash.ToLowerInvariant() -ne $file.sha256){throw "Extracted hash mismatch $path"};$verified++}
  $ex=[ordered]@{snapshotPath=$snapshot;fileCount=$files.Count;verified=$verified;files=@($files|ForEach-Object{@{path=(Join-Path $snapshot $_.path);relativePath=$_.path;bytes=$_.bytes;sha256=$_.sha256;measuredGitBlobSHA1=$_.measuredGitBlobSHA1}});immutableIntent='Never patch/mutate this snapshot; producer uses a new private copy'}
  [IO.File]::WriteAllText("$dir/source-extraction.json",($ex|ConvertTo-Json -Depth 15),[Text.UTF8Encoding]::new($false))
  $results+=@{id=$input.id;status='TAR_SAFE_EXTRACT_VERIFIED';snapshotPath=$snapshot;fileCount=$files.Count;pinnedTextCount=$critical.Count;sha256=$receipt.sha256}
 }catch{$results+=@{id=$input.id;status='FAILED_PRESERVED';error=$_.Exception.Message}}
}
[IO.File]::WriteAllText("$acquisition/tar-extraction-summary-rev2.json",($results|ConvertTo-Json -Depth 10),[Text.UTF8Encoding]::new($false))
$results|ConvertTo-Json