#Requires -Version 7.0
$ErrorActionPreference='Stop'
function Get-TaskHash([string]$path){
    $stream=[IO.File]::OpenRead($path)
    try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}
}
$taskOutput="$PSScriptRoot/RunDxcConsumerParentRev4.ps1"
$taskReceipt="$PSScriptRoot/parent-consumer-assignment-rev4.json"
foreach($path in @($taskOutput,$taskReceipt)){if(Test-Path -LiteralPath $path){throw 'Preserve previous preparation'}}
$taskOld="$PSScriptRoot/RunDxcConsumerParentRev2.ps1"
if((Get-TaskHash $taskOld) -ne '3DB4A0662F011C4BE27F51BBEC99A9005A24B341067CA0B05EC5AE0F371A20A8'){throw 'Reviewed driver changed'}
$taskCandidate="$PSScriptRoot/consumer-candidate-rev4"
$taskSource='F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/DirectXShaderCompilerDxc/tests/DirectXShaderCompilerDxcConsumer.cpp'
if((Get-TaskHash $taskSource) -ne '3FE2DA8FC5D468FFDCC14090AD2D2FB7EC0B44A328059AB47D33AD6E4E723064'){throw 'Corrected consumer changed'}
$taskDiff=Get-Content -LiteralPath "$taskCandidate/source-diff-rev3-to-rev4.patch"
$taskRemoved=@($taskDiff | Where-Object { $_.StartsWith('-') -and -not $_.StartsWith('---') } | ForEach-Object { $_.Substring(1) })
$taskAdded=@($taskDiff | Where-Object { $_.StartsWith('+') -and -not $_.StartsWith('+++') } | ForEach-Object { $_.Substring(1) })
$taskBefore=[IO.File]::ReadAllText("$taskCandidate/DirectXShaderCompilerDxcConsumer.rev3.before.cpp")
$taskCurrent=[IO.File]::ReadAllText($taskSource)
if($taskRemoved.Count -ne 1 -or $taskAdded.Count -ne 8 -or $taskCurrent.Replace(($taskAdded -join "`n")+"`n",($taskRemoved -join "`n")+"`n") -cne $taskBefore){throw 'Corrected source inverse differs'}
$taskFreeze="$taskCandidate/source-freeze-dxc-consumer-rev4.json"
$taskWorkerAssignment="$taskCandidate/validation-assignment.json"
if((Get-TaskHash $taskFreeze) -ne 'A708661417616D36547CF224DD24712176157A1B02A8086A93DD64079AEEAD92' -or (Get-TaskHash $taskWorkerAssignment) -ne '848BCA5E32ED6E815126EAC789EEB8F874865D51B4A5C2D748DA1B04DBC2664B'){throw 'Candidate evidence changed'}
if((Get-Content -Raw -LiteralPath $taskFreeze | ConvertFrom-Json).files.Count -ne 23385){throw 'Final candidate count differs'}
$taskText=[IO.File]::ReadAllText($taskOld)
$taskReplacements=@(
    @('consumer-attempt2','consumer-attempt4'),
    @('dxc-native-consumer-rev2','dxc-native-consumer-rev4'),
    @('consumer-candidate-rev2','consumer-candidate-rev4'),
    @('source-freeze-dxc-consumer-rev2.json','source-freeze-dxc-consumer-rev4.json'),
    @('4427A9E7947AC6999D9FFA36FA55704885CADBF4E4C0E745D4B89CE6ADC0F03B','A708661417616D36547CF224DD24712176157A1B02A8086A93DD64079AEEAD92'),
    @('87C0542F096189E5D34231106CDABD03C70D59B7FB3DC39740DC02AFD2FFBE90','848BCA5E32ED6E815126EAC789EEB8F874865D51B4A5C2D748DA1B04DBC2664B'),
    @('23373','23385')
)
foreach($replacement in $taskReplacements){if(-not $taskText.Contains($replacement[0])){throw 'Reviewed driver fragment absent'};$taskText=$taskText.Replace($replacement[0],$replacement[1])}
$tokens=$null;$errors=$null
[void][Management.Automation.Language.Parser]::ParseInput($taskText,[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Final driver parser errors'}
foreach($path in @('F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/dxc-rev1-validation/consumer-attempt4','F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/dxc-native-consumer-rev4')){if(Test-Path -LiteralPath $path){throw 'Final native output exists'}}
[IO.File]::WriteAllText($taskOutput,$taskText,[Text.UTF8Encoding]::new($false))
$taskResult=[ordered]@{status='PARENT EXACT DIFF/INVERSE REVIEW; FOUR NATIVE CASES ASSIGNED';nativeOwner='Existing validation_luna only';driver=$taskOutput;driverSHA256=(Get-TaskHash $taskOutput);sourceInputs=23385;freezePath=$taskFreeze;freezeSHA256=(Get-TaskHash $taskFreeze);workerAssignmentSHA256=(Get-TaskHash $taskWorkerAssignment);cppSHA256=(Get-TaskHash $taskSource);inverseExact=$true;parserErrors=0;cases=@('JE.LocalPackages.DXC.dxil','JE.LocalPackages.DXC.spirv','JE.LocalPackages.DXC.invalid','JE.LocalPackages.DXC.dxsc');review='Pinned upstream plus actual diagnostic established registered zero-length output slot on failure. Corrected oracle requires successful diagnostic/object/result queries, failed operation status, nonempty diagnostics, both byte lengths zero or null, and absent output path. Positive oracles unchanged.';producerRebuild=$false;extraction=$false;acceptance=$false}
[IO.File]::WriteAllText($taskReceipt,($taskResult | ConvertTo-Json -Depth 8)+"`n",[Text.UTF8Encoding]::new($false))
[ordered]@{driverSHA256=$taskResult.driverSHA256;assignmentSHA256=(Get-TaskHash $taskReceipt);inverseExact=$true;parserErrors=0} | ConvertTo-Json
