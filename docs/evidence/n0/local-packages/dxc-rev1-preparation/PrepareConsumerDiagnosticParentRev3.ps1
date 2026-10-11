#Requires -Version 7.0
$ErrorActionPreference='Stop'
$taskOutput="$PSScriptRoot/RunDxcConsumerDiagnosticParentRev3.ps1"
$taskReceipt="$PSScriptRoot/parent-consumer-diagnostic-assignment-rev3.json"
foreach($path in @($taskOutput,$taskReceipt)){if(Test-Path -LiteralPath $path){throw 'Preserve existing parent preparation'}}
function Get-TaskHash([string]$path){
    $stream=[IO.File]::OpenRead($path)
    try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}
}
$taskOriginal="$PSScriptRoot/RunDxcConsumerParentRev2.ps1"
if((Get-TaskHash $taskOriginal) -ne '3DB4A0662F011C4BE27F51BBEC99A9005A24B341067CA0B05EC5AE0F371A20A8'){throw 'Reviewed parent driver changed'}
$taskText=[IO.File]::ReadAllText($taskOriginal)
$taskReplacements=@(
    @('consumer-attempt2','consumer-diagnostic-attempt3'),
    @('dxc-native-consumer-rev2','dxc-native-consumer-rev3'),
    @('consumer-candidate-rev2','consumer-candidate-rev3'),
    @('4427A9E7947AC6999D9FFA36FA55704885CADBF4E4C0E745D4B89CE6ADC0F03B','B06DB30C0DCDEC4EF685C8606C0A8FD19F54D1B1E967C90DA35DB4842750B909'),
    @('87C0542F096189E5D34231106CDABD03C70D59B7FB3DC39740DC02AFD2FFBE90','A321C3DF24DF972D0FC963C024F733E73F6263E61D278DF288EEA012F2D34705'),
    @('23373','23379'),
    @('(@($taskAssignment.consumerCTest.argv)+@(''--output-junit'',"$taskRun/ctest.junit.xml"))','(@($taskAssignment.consumerCTest.argv)+@(''-R'',''^JE[.]LocalPackages[.]DXC[.]invalid$'',''--output-junit'',"$taskRun/ctest.junit.xml"))'),
    @('$taskNodes.Count -ne 4 -or -not $taskNames.SetEquals([string[]]$taskAssignment.consumerCTest.cases)','$taskNodes.Count -ne 1 -or -not $taskNames.SetEquals([string[]]@(''JE.LocalPackages.DXC.invalid''))'),
    @('FOCAL CONSUMER FINISHED; PARENT/GUARDS/PE/ROOT REQUIRED','DIAGNOSTIC FINISHED; ORIGINAL ASSERTION UNCHANGED; NO ACCEPTANCE')
)
foreach($replacement in $taskReplacements){
    if(-not $taskText.Contains($replacement[0])){throw "Required reviewed driver fragment absent: $($replacement[0])"}
    $taskText=$taskText.Replace($replacement[0],$replacement[1])
}
$tokens=$null;$errors=$null
[void][Management.Automation.Language.Parser]::ParseInput($taskText,[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Derived diagnostic driver parser errors'}
[IO.File]::WriteAllText($taskOutput,$taskText,[Text.UTF8Encoding]::new($false))
$taskAssignment=Get-Content -Raw -LiteralPath "$PSScriptRoot/consumer-candidate-rev3/validation-assignment.json" | ConvertFrom-Json
$taskNativeCTest=@($taskAssignment.consumerCTest.argv)+@('-R','^JE[.]LocalPackages[.]DXC[.]invalid$','--output-junit','F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/dxc-rev1-validation/consumer-diagnostic-attempt3/ctest.junit.xml')
$taskResult=[ordered]@{
    status='PARENT REVIEWED DIAGNOSTIC DISPATCH; EXPECTED HISTORICAL ASSERTION RED'
    nativeOwner='Existing validation_luna only'; driver=$taskOutput; driverSHA256=(Get-TaskHash $taskOutput)
    sourceFreezeSHA256='B06DB30C0DCDEC4EF685C8606C0A8FD19F54D1B1E967C90DA35DB4842750B909'
    sourceInputs=23379; cppSHA256='4ED3A3DCFBDDC5E3E8E4006B449E5BE8F77E5C81DECF9F9C39F3DB14623E1407'
    configure=$taskAssignment.consumerConfigure; build=$taskAssignment.consumerBuild
    ctest=@{executable=$taskAssignment.consumerCTest.executable;argv=$taskNativeCTest;cases=@('JE.LocalPackages.DXC.invalid')}
    purpose='Observe HRESULT, diagnostics, advertised slot and both object lengths while retaining the historical rejection assertion.'
    preserved='Source pins, original producer, all positive oracles and immutable 15-file extracted package.'
    producerRebuild=$false; extraction=$false; rootExecuted=$false; acceptance=$false; parserErrors=0
}
[IO.File]::WriteAllText($taskReceipt,($taskResult | ConvertTo-Json -Depth 12)+"`n",[Text.UTF8Encoding]::new($false))
[ordered]@{driverSHA256=$taskResult.driverSHA256;assignmentSHA256=(Get-TaskHash $taskReceipt);parserErrors=0} | ConvertTo-Json
