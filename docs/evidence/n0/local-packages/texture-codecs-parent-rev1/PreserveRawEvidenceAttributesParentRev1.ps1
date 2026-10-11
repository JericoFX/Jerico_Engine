#Requires -Version 7.0
#Requires -PSEdition Core
$ErrorActionPreference='Stop'
$taskRepo='F:\JericoEngine\JericoEngine'
$taskPub=Join-Path $taskRepo 'build/n0-validation/local-packages/publication-texture-codecs-parent-rev1'
function Hash-TaskFile([string]$path){$stream=[IO.File]::OpenRead($path);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}}
$taskLog=Get-Content -LiteralPath (Join-Path $taskPub 'staged-whitespace-initial.log')
$taskPaths=@($taskLog|ForEach-Object{if($_ -match '^([^:]+):\d+: '){$Matches[1]}}|Sort-Object -Unique)
if($taskPaths.Count -ne 43){throw 'Raw whitespace diagnostic count changed'}
$taskSource='cmake/Tools/LocalPackages/ASTCEncoder/astc-encoder-3.2-rev1.patch'
if((Hash-TaskFile (Join-Path $taskRepo $taskSource)) -ne '88E0389289A28DB8D937842D890A7EC4953D2295C263997236B05BE4B3B5BD00'){throw 'Pinned source patch drift'}
$taskCopyMap=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($taskRow in Import-Csv -LiteralPath (Join-Path $taskRepo 'docs/evidence/n0/local-packages/copy-manifest.csv')){$taskCopyMap.Add($taskRow.destination,$taskRow)}
$taskRules=@(foreach($taskPath in $taskPaths){
    if($taskPath -eq $taskSource){continue}
    if(-not $taskPath.StartsWith('docs/evidence/n0/') -or -not $taskCopyMap.ContainsKey($taskPath)){throw "Exception outside captured raw evidence: $taskPath"}
    $taskRow=$taskCopyMap[$taskPath]
    $taskFull=Join-Path $taskRepo $taskPath
    if([IO.FileInfo]::new($taskFull).Length -ne $taskRow.bytes -or (Hash-TaskFile $taskFull) -ne $taskRow.sha256){throw 'Captured original bytes changed'}
    '/'+$taskPath.Substring('docs/evidence/n0/'.Length)+' -whitespace'
})
if($taskRules.Count -ne 42){throw 'Expected42 captured evidence exceptions'}
$taskAttributes=Join-Path $taskRepo 'docs/evidence/n0/.gitattributes'
$taskText=[IO.File]::ReadAllText($taskAttributes)
$taskBeforeHash=Hash-TaskFile $taskAttributes
foreach($taskRule in $taskRules){if($taskText.Contains($taskRule)){throw 'Exception already exists; preserve previous attempt'}}
if(-not $taskText.EndsWith("`n")){$taskText+="`r`n"}
$taskText+="# Original codec recipes, notices and captured CMake data keep exact whitespace.`r`n"+($taskRules -join "`r`n")+"`r`n"
[IO.File]::WriteAllText($taskAttributes,$taskText,[Text.UTF8Encoding]::new($false))
[ordered]@{status='PINNED_RAW_DATA_ATTRIBUTES_ONLY';sourcePatch=$taskSource;sourcePatchSHA256=(Hash-TaskFile (Join-Path $taskRepo $taskSource));evidenceExceptions=$taskRules.Count;attributesBeforeSHA256=$taskBeforeHash;attributesAfterSHA256=(Hash-TaskFile $taskAttributes);paths=$taskPaths;sourceTestsAndAdaptedCodeWhitespaceChecksPreserved=$true} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $taskPub 'raw-attributes-receipt.json') -Encoding utf8NoBOM
