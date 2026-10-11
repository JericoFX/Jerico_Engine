$ErrorActionPreference='Stop'
$dir=Join-Path $PSScriptRoot 'acquisition/dxc'
$receipt=Get-Content -LiteralPath "$dir/source-acquisition.json" -Raw|ConvertFrom-Json
$audit=Get-Content -LiteralPath "$dir/source-tar-audit.json" -Raw|ConvertFrom-Json -Depth 100
$tree=Get-Content -LiteralPath "$dir/primary-git-tree.json" -Raw|ConvertFrom-Json -Depth 100
$bad=@($audit.entries|Where-Object{ $_.type -eq 'RegularFile' -and -not $_.primaryGitBlobMatch})
if($bad.Count -ne 19 -or @($bad|Where-Object path -notmatch '\.(cmd|bat)$').Count){throw 'Mismatch set differs from observed nineteen batch scripts'}
$wanted=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal)
foreach($entry in $bad){$wanted.Add($entry.path,($tree.tree|Where-Object path -ceq $entry.path))}
$proof=@();$contract=@()
$apiDir=Join-Path $PSScriptRoot 'version-api-texts';if(Test-Path -LiteralPath $apiDir){throw 'API text evidence exists'}
[IO.Directory]::CreateDirectory($apiDir)|Out-Null
$stream=[IO.File]::OpenRead($receipt.archivePath);$gzip=[IO.Compression.GZipStream]::new($stream,[IO.Compression.CompressionMode]::Decompress);$tar=[System.Formats.Tar.TarReader]::new($gzip)
try{
 while($null -ne ($entry=$tar.GetNextEntry())){
  if($entry.EntryType -ne [System.Formats.Tar.TarEntryType]::RegularFile){continue}
  $path=$entry.Name.Substring($audit.root.Length+1)
  $selectedApi=($path -eq 'include/dxc/dxcapi.h' -or $path -match '^tools/clang/tools/(dxcompiler|dxclib|dxc)/.*\.(cpp|h)$')
  if(-not $wanted.ContainsKey($path) -and -not $selectedApi){continue}
  $ms=[IO.MemoryStream]::new();$entry.DataStream.CopyTo($ms);$bytes=$ms.ToArray();$ms.Dispose()
  if($wanted.ContainsKey($path)){
   $text=[Text.Encoding]::UTF8.GetString($bytes)
   $normal=$text.Replace("`r`n","`n")
   $normalized=[Text.Encoding]::UTF8.GetBytes($normal)
   $prefix=[Text.Encoding]::ASCII.GetBytes("blob $($normalized.Length)`0")
   $blob=[byte[]]::new($prefix.Length+$normalized.Length)
   [Array]::Copy($prefix,0,$blob,0,$prefix.Length);[Array]::Copy($normalized,0,$blob,$prefix.Length,$normalized.Length)
   $normalizedBlob=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData($blob)).ToLowerInvariant()
   $inverse=[Text.Encoding]::UTF8.GetBytes($normal.Replace("`n","`r`n"))
   $inverseSha=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($inverse)).ToLowerInvariant()
   $archiveSha=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
   $row=[ordered]@{path=$path;archiveBytes=$bytes.Length;archiveSHA256=$archiveSha;primaryBlobBytes=$wanted[$path].size;primaryGitBlob=$wanted[$path].sha;crlfPairs=[regex]::Matches($text,"`r`n").Count;bareCR=[regex]::Matches($text,"`r(?!`n)").Count;bareLF=[regex]::Matches($text,"(?<!`r)`n").Count;normalizedBytes=$normalized.Length;normalizedGitBlob=$normalizedBlob;normalizedMatchesPrimary=($normalizedBlob -eq $wanted[$path].sha -and $normalized.Length -eq $wanted[$path].size);inverseArchiveByteExact=($inverseSha -eq $archiveSha -and $inverse.Length -eq $bytes.Length);rule='Root .gitattributes *.cmd/*.bat text eol=crlf; archive preserved, no normalization written'}
   $proof+=$row
  }
  if($selectedApi){
   $text=[Text.Encoding]::UTF8.GetString($bytes)
   if($text -match 'IDxcVersionInfo2|GetCommitInfo\('){
    $output=Join-Path $apiDir $path.Replace('/','__')
    [IO.File]::WriteAllBytes($output,$bytes)
    $contract+=@{path=$path;evidencePath=$output;sha256=(Get-FileHash -LiteralPath $output).Hash;matches=@($text.Split("`n")|Select-String -Pattern 'IDxcVersionInfo2|GetCommitInfo\(' | ForEach-Object{@{line=$_.LineNumber;text=$_.Line.TrimEnd()}})}
   }
  }
 }
}finally{$tar.Dispose();$gzip.Dispose();$stream.Dispose()}
$attribute=$audit.entries|Where-Object path -eq '.gitattributes'
$result=[ordered]@{status='SPECIFIC EOL EXPORT PROOF; PARENT EXCEPTION REQUIRED BEFORE DXC EXTRACTION';archiveSHA256=$receipt.sha256;expectedMismatchCount=19;paths=$proof;allNormalizedPrimaryMatches=(@($proof|Where-Object{-not $_.normalizedMatchesPrimary}).Count -eq 0);allInverseByteExact=(@($proof|Where-Object{-not $_.inverseArchiveByteExact}).Count -eq 0);gitattributesGitBlob=$attribute.measuredGitBlobSHA1;gitattributesPrimaryMatch=$attribute.primaryGitBlobMatch;originalAuditPreserved=$true;archiveModified=$false;snapshotExtracted=$false}
[IO.File]::WriteAllText("$dir/archive-eol-export-proof.json",($result|ConvertTo-Json -Depth 20),[Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $PSScriptRoot 'version-api-text-receipt.json'),($contract|ConvertTo-Json -Depth 10),[Text.UTF8Encoding]::new($false))
[ordered]@{mismatchCount=$proof.Count;normalizedAll=$result.allNormalizedPrimaryMatches;inverseAll=$result.allInverseByteExact;apiTexts=$contract.Count}|ConvertTo-Json
