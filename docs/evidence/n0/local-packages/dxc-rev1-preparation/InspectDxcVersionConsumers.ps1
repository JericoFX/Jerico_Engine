$ErrorActionPreference='Stop'
$dir=Join-Path $PSScriptRoot 'acquisition/dxc'
$receipt=Get-Content -LiteralPath "$dir/source-acquisition.json" -Raw|ConvertFrom-Json
$audit=Get-Content -LiteralPath "$dir/source-tar-audit.json" -Raw|ConvertFrom-Json -Depth 100
$rows=@()
$stream=[IO.File]::OpenRead($receipt.archivePath);$gzip=[IO.Compression.GZipStream]::new($stream,[IO.Compression.CompressionMode]::Decompress);$tar=[System.Formats.Tar.TarReader]::new($gzip)
try{
 while($null -ne ($entry=$tar.GetNextEntry())){
  if($entry.EntryType -ne [System.Formats.Tar.TarEntryType]::RegularFile){continue}
  $path=$entry.Name.Substring($audit.root.Length+1)
  if($path -notmatch '\.(cpp|h|py|cmake)$' -or $path -match '/(test|unittests|HLSLDisabled)/'){continue}
  if($entry.Length -eq 0){continue}
  $ms=[IO.MemoryStream]::new();$entry.DataStream.CopyTo($ms);$text=[Text.Encoding]::UTF8.GetString($ms.ToArray());$ms.Dispose()
  if($text -match 'getGitCommitCount\(|getGitCommitHash\(|GetCommitInfo\(|IDxcVersionInfo2'){
   $matches=@($text.Split("`n")|Select-String -Pattern 'getGitCommitCount\(|getGitCommitHash\(|GetCommitInfo\(|IDxcVersionInfo2'|ForEach-Object{@{line=$_.LineNumber;text=$_.Line.TrimEnd()}})
   $rows+=@{path=$path;matches=$matches}
  }
 }
}finally{$tar.Dispose();$gzip.Dispose();$stream.Dispose()}
$path=Join-Path $PSScriptRoot 'version-consumer-static-search.json';if(Test-Path -LiteralPath $path){throw 'Consumer evidence exists'}
[IO.File]::WriteAllText($path,($rows|ConvertTo-Json -Depth 12),[Text.UTF8Encoding]::new($false))
$rows|ConvertTo-Json -Depth 12
