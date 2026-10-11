#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root=[IO.Path]::GetFullPath($PSScriptRoot)
if($root -ne 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\dxc-rev1-negatives'){throw 'Assigned root differs'}
$encoding=[Text.UTF8Encoding]::new($false)
function Hash([string] $path){$stream=[IO.File]::OpenRead($path);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}}
function Entry([string] $path){[ordered]@{path=[IO.Path]::GetFullPath($path);bytes=([IO.FileInfo]::new($path)).Length;sha256=(Hash $path)}}
function Write-New([string] $path,[string] $text){if(Test-Path -LiteralPath $path){throw "Retain existing evidence: $path"};[IO.File]::WriteAllText($path,$text,$encoding)}
function Json($value){($value|ConvertTo-Json -Depth 24)+"`n"}
$oldAssignmentPath=Join-Path $root 'negative-assignment-rev2.json'
$oldDriverPath=Join-Path $root 'RunDxcNegativesRev2.ps1'
if((Hash $oldAssignmentPath) -ne 'D0AF36852D02984ED2F645ADDD29395923768DBA8B8838A1A5DD3F88B6FC8D7C' -or (Hash $oldDriverPath) -ne 'ECBF9321FFCCE2BCEC73DBD60F9C0916AD235969BEBAE381BCAC107ADFAF845F'){throw 'Reviewed negative rev2 differs'}
$old=Get-Content -LiteralPath $oldAssignmentPath -Raw|ConvertFrom-Json -AsHashtable
$assignment=[IO.File]::ReadAllText($oldAssignmentPath).Replace('attempt-rev2','attempt-rev3')|ConvertFrom-Json -AsHashtable
$freezePath='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/dxc-rev1-preparation/consumer-candidate-rev4/source-freeze-dxc-consumer-rev4.json'
$freezeHash='A708661417616D36547CF224DD24712176157A1B02A8086A93DD64079AEEAD92'
if((Hash $freezePath) -ne $freezeHash){throw 'Final corrected consumer freeze differs'}
$freeze=Get-Content -LiteralPath $freezePath -Raw|ConvertFrom-Json
if($freeze.files.Count -ne 23385){throw 'Final source input count differs'}
$filesByPath=@{}
foreach($file in $freeze.files){$path=[IO.Path]::GetFullPath($file.path);if($filesByPath.ContainsKey($path)){throw 'Duplicate freeze path'};$filesByPath[$path]=$file;if(([IO.FileInfo]::new($path)).Length -ne $file.bytes -or (Hash $path) -ne $file.sha256){throw "Final frozen source changed: $path"}}
$oldFreeze=Get-Content -LiteralPath $old.sourceFreeze.path -Raw|ConvertFrom-Json
$deltas=@(foreach($file in $oldFreeze.files){$path=[IO.Path]::GetFullPath($file.path);if(-not $filesByPath.ContainsKey($path)){throw 'Old input missing'};if($file.sha256 -ne $filesByPath[$path].sha256 -or $file.bytes -ne $filesByPath[$path].bytes){@{before=$file;after=$filesByPath[$path]}}})
if($deltas.Count -ne 1 -or $deltas[0].after.path -notlike '*DirectXShaderCompilerDxcConsumer.cpp' -or $deltas[0].after.sha256 -ne '3FE2DA8FC5D468FFDCC14090AD2D2FB7EC0B44A328059AB47D33AD6E4E723064'){throw 'Unexpected source reconciliation'}
if($assignment.cases.Count -ne 18){throw 'Case count differs'}
for($i=0;$i -lt $old.cases.Count;$i++){
    $expected=($old.cases[$i]|ConvertTo-Json -Depth 16).Replace('attempt-rev2','attempt-rev3')
    if(($assignment.cases[$i]|ConvertTo-Json -Depth 16) -cne $expected){throw 'Case argv/guard/scope differs beyond fresh path'}
    if(Test-Path -LiteralPath $assignment.cases[$i].secondaryMissingPath){throw 'Secondary missing guard exists'}
}
$assignment.sourceFreeze=@{path=[IO.Path]::GetFullPath($freezePath);sha256=$freezeHash;files=23385}
$assignment.resultContract=$assignment.resultContract.Replace('23373','23385')
$assignment.status='PREPARED FINAL NEGATIVE REV3; CORRECTED CONSUMER SOURCE; NOT RUN'
$attempt=Join-Path $root 'attempt-rev3'
if(Test-Path -LiteralPath $attempt){throw 'Fresh negative attempt exists'}
$assignmentPath=Join-Path $root 'negative-assignment-rev3.json'
Write-New $assignmentPath (Json $assignment)
$driver=[IO.File]::ReadAllText($oldDriverPath).Replace("'attempt-rev2'","'attempt-rev3'").Replace("'negative-assignment-rev2.json'","'negative-assignment-rev3.json'").Replace('D0AF36852D02984ED2F645ADDD29395923768DBA8B8838A1A5DD3F88B6FC8D7C',(Hash $assignmentPath)).Replace('23373','23385')
$hashFunction=@'
# Stream hashing avoids per-file cmdlet overhead; the SHA256 guard is unchanged.
function Get-JEFileSHA256([string] $path) {
    $stream = [IO.File]::OpenRead($path)
    try { return [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream)) }
    finally { $stream.Dispose() }
}
'@
$hashFunction=$hashFunction.Replace("`r`n","`n")+"`n"
$anchor='$root = [IO.Path]::GetFullPath($PSScriptRoot)'
if([regex]::Matches($driver,[regex]::Escape($anchor)).Count -ne 1){throw 'Hash insertion anchor differs'}
$driver=$driver.Replace($anchor,$hashFunction+$anchor)
$pattern='\(Get-FileHash -LiteralPath (\$\w+(?:\.\w+)?)\)\.Hash'
$hashCalls=[regex]::Matches($driver,$pattern).Count
if($hashCalls -ne 6){throw "Hash call coverage differs: $hashCalls"}
$driver=[regex]::Replace($driver,$pattern,'(Get-JEFileSHA256 $1)')
if($driver.Contains('Get-FileHash')){throw 'Unconverted slow hash call'}
$driverPath=Join-Path $root 'RunDxcNegativesRev3.ps1'
Write-New $driverPath $driver
$tokens=$null;$errors=$null
[void][Management.Automation.Language.Parser]::ParseFile($driverPath,[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Driver parser errors'}
$proofPath=Join-Path $root 'negative-preparation-proof-rev3.json'
$proof=[ordered]@{status='PREPARATION ONLY; NO NATIVE';sourceInputs=23385;previousSourceInputs=23373;unchangedPreviousInputs=23372;authorizedSourceDeltas=$deltas;additionalInputs=12;cases=18;scopeCounts=$assignment.scopeCounts;caseArgvAndGuardsEqualAfterNaming=$true;streamSHA256CallCount=$hashCalls;ASTerrors=$errors.Count;fixturesCreated=0;junctionsCreated=0;attemptExists=$false;historicalWitnesses=@((Entry $oldAssignmentPath),(Entry $oldDriverPath),(Entry (Join-Path $root 'negative-review-freeze-rev2.json')));driver=(Entry $driverPath);assignment=(Entry $assignmentPath);nativeExecuted=$false;acceptance=$false}
Write-New $proofPath (Json $proof)
$setupPath=Join-Path $root 'negative-setup-freeze-rev3.json'
$entries=@((Entry $driverPath),(Entry $assignmentPath),(Entry $PSCommandPath),(Entry $proofPath),(Entry $freezePath),(Entry $oldDriverPath),(Entry $oldAssignmentPath),(Entry (Join-Path $root 'negative-review-freeze-rev2.json')))+@($assignment.productionWitnesses)
Write-New $setupPath (Json @{status='PENDING REVIEW / LUNA EXECUTION';fileCount=$entries.Count;files=$entries;sourceInputs=23385;caseCount=18;scopeCounts=$assignment.scopeCounts;nativeExecuted=$false;acceptance=$false})
$reviewSource=Join-Path $root 'BuildNegativeReviewRev2.ps1'
if((Hash $reviewSource) -ne 'CCBE84A952DB5BE732E134B62407D880643E67FB093E925CB1659B7E5C73A0B2'){throw 'Reviewed static proof builder differs'}
$review=[IO.File]::ReadAllText($reviewSource).Replace('Rev2','Rev3').Replace('rev2','rev3').Replace('ECBF9321FFCCE2BCEC73DBD60F9C0916AD235969BEBAE381BCAC107ADFAF845F',(Hash $driverPath)).Replace('D0AF36852D02984ED2F645ADDD29395923768DBA8B8838A1A5DD3F88B6FC8D7C',(Hash $assignmentPath)).Replace("'RunDxcNegatives.ps1'","'RunDxcNegativesRev2.ps1'").Replace('--- RunDxcNegatives.ps1 (historical rev1)','--- RunDxcNegativesRev2.ps1 (historical rev2)')
Write-New (Join-Path $root 'BuildNegativeReviewRev3.ps1') $review
@{driver=(Entry $driverPath);assignment=(Entry $assignmentPath);setupFreeze=(Entry $setupPath);proof=(Entry $proofPath);reviewBuilder=(Entry (Join-Path $root 'BuildNegativeReviewRev3.ps1'));nativeExecuted=$false}|ConvertTo-Json -Depth 8
