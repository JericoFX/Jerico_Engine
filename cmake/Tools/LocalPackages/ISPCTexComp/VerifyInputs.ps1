# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
param([Parameter(Mandatory=$true)][string]$SourceRoot,
    [Parameter(Mandatory=$true)][string]$SourceManifest,
    [Parameter(Mandatory=$true)][string]$ToolRoot,
    [Parameter(Mandatory=$true)][string]$ToolManifest)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
foreach($inputPair in @(@{root=$SourceRoot;manifest=$SourceManifest;count=238},
    @{root=$ToolRoot;manifest=$ToolManifest;count=175})){
    $root=[IO.Path]::GetFullPath($inputPair.root).TrimEnd('\','/')
    $ancestor=[IO.DirectoryInfo]::new($root)
    while($ancestor){
        if($ancestor.Exists -and ($ancestor.Attributes -band [IO.FileAttributes]::ReparsePoint)){
            throw 'JE ISPCTexComp: input ancestor is a reparse point'
        }
        $ancestor=$ancestor.Parent
    }
    $manifest=Get-Content -LiteralPath $inputPair.manifest -Raw | ConvertFrom-Json
    $records=@($manifest.entries)
    if($manifest.files -ne $inputPair.count -or $records.Count -ne $inputPair.count){
        throw 'JE ISPCTexComp: input manifest count mismatch'
    }
    $names=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $queue=[Collections.Generic.Queue[string]]::new()
    $queue.Enqueue($root)
    $count=0
    while($queue.Count){
        foreach($item in (Get-ChildItem -LiteralPath $queue.Dequeue() -Force)){
            if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'JE ISPCTexComp: reparse input'}
            if($item.PSIsContainer){$queue.Enqueue($item.FullName)}else{$count++}
        }
    }
    if($count -ne $inputPair.count){throw 'JE ISPCTexComp: input file count mismatch'}
    foreach($record in $records){
        if($record.path -match '(^[\\/]|(^|[\\/])\.\.?([\\/]|$)|:|\\)' -or -not $names.Add($record.path)){
            throw 'JE ISPCTexComp: unsafe or duplicate manifest path'
        }
        $path=[IO.Path]::GetFullPath((Join-Path $root $record.path))
        if(-not $path.StartsWith($root+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'JE ISPCTexComp: escaping manifest path'}
        $item=Get-Item -LiteralPath $path
        if($item.PSIsContainer -or $item.Length -ne $record.bytes){throw 'JE ISPCTexComp: input size mismatch'}
        $hasher=[Security.Cryptography.SHA256]::Create()
        try{
            $stream=[IO.File]::OpenRead($path)
            try{$digest=$hasher.ComputeHash($stream)}finally{$stream.Dispose()}
        }finally{$hasher.Dispose()}
        if(([BitConverter]::ToString($digest)).Replace('-','') -ne $record.sha256){throw 'JE ISPCTexComp: input SHA256 mismatch'}
    }
}
Write-Output 'JE ISPCTexComp: immutable source 238 and external tool 175 files verified; no compiler executed'
