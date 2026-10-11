param([Parameter(Mandatory=$true)][string]$Root, [Parameter(Mandatory=$true)][string]$Manifest)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
# The CMake caller verifies the audited manifest/archive hashes before this read.
$rootFull=[IO.Path]::GetFullPath($Root).TrimEnd('\','/')
$records=Get-Content -LiteralPath $Manifest -Raw | ConvertFrom-Json
if($records.Count -ne 20858){throw 'JE OpenSSL: Perl manifest file count mismatch'}
$directories=[Collections.Generic.Queue[string]]::new()
$directories.Enqueue($rootFull)
$fileCount=0
while($directories.Count){
    $directory=$directories.Dequeue()
    foreach($item in (Get-ChildItem -LiteralPath $directory -Force)){
        # Reject a link before descending; Windows PowerShell must not follow it.
        if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'JE OpenSSL: Perl input contains a reparse point'}
        if($item.PSIsContainer){$directories.Enqueue($item.FullName)}else{$fileCount++}
    }
}
if($fileCount -ne 20858){throw 'JE OpenSSL: Perl input file count mismatch'}
foreach($record in $records){
    if($record.path -match '(^[\\/]|(^|[\\/])\.\.?([\\/]|$)|:|\\)'){throw 'JE OpenSSL: unsafe Perl manifest path'}
    $path=[IO.Path]::GetFullPath((Join-Path $rootFull $record.path))
    if(-not $path.StartsWith($rootFull+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'JE OpenSSL: escaping Perl manifest path'}
    $item=Get-Item -LiteralPath $path
    if($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -or $item.Length -ne $record.bytes){throw 'JE OpenSSL: invalid Perl input file'}
    $hasher=[Security.Cryptography.SHA256]::Create()
    try{
        $stream=[IO.File]::OpenRead($path)
        try{$digest=$hasher.ComputeHash($stream)}finally{$stream.Dispose()}
    }finally{$hasher.Dispose()}
    $actualHash=([BitConverter]::ToString($digest)).Replace('-','')
    if($actualHash -ne $record.sha256){throw 'JE OpenSSL: Perl input SHA256 mismatch'}
}
Write-Output 'JE OpenSSL: all 20858 Perl input files verified; no tool executed'
