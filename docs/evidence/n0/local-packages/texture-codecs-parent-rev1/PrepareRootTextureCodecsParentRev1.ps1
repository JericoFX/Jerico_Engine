#Requires -Version 7.0
param([Parameter(Mandatory)][ValidatePattern('^[A-F0-9]{64}$')][string]$IntegrationSHA256)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$taskRepo='F:/JericoEngine/JericoEngine'
$taskBase="$taskRepo/build/n0-validation/local-packages"
$taskOldRoot="$taskBase/root-dxc-local-final"
$taskNewRoot="$taskBase/root-texture-codecs-local-final"
if(Test-Path -LiteralPath $taskNewRoot){throw 'Preserve existing Root preparation'}
function Get-TaskHash([string]$p){$s=[IO.File]::OpenRead($p);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($s))}finally{$s.Dispose()}}
function Write-TaskNew([string]$p,[string]$text){$s=[IO.File]::Open($p,[IO.FileMode]::CreateNew);try{$b=[Text.UTF8Encoding]::new($false).GetBytes($text);$s.Write($b)}finally{$s.Dispose()}}
$taskOldFreezePath="$taskOldRoot/source-freeze-root-batch.json"
$taskOldDriverPath="$taskOldRoot/run-root-configure-dxc-parent-rev1.ps1"
if((Get-TaskHash $taskOldFreezePath) -ne 'AF617F8664743B42A80CC17A31475998C2F1A3819BE430C0FD073A4F7696EAA3' -or (Get-TaskHash $taskOldDriverPath) -ne 'D0876C989526EE283CEFFDF03674C3154740263A0877F7A29A9DE1763DABAA23'){throw 'Reviewed Root donor changed'}
$taskIntegrationPath="$PSScriptRoot/parent-package-integration.json"
if((Get-TaskHash $taskIntegrationPath) -ne $IntegrationSHA256){throw 'Parent integration witness changed'}
$taskIntegration=Get-Content -LiteralPath $taskIntegrationPath -Raw | ConvertFrom-Json
if($taskIntegration.cacheFilesAfter -ne 40 -or $taskIntegration.providerCount -ne 17 -or $taskIntegration.engineAccepted){throw 'Integrated focal scope differs'}
$taskAstcPath="$taskBase/astc-rev1-consumer-preparation/source-freeze-astc-consumer.json"
$taskIspcPath="$taskBase/ispc-texcomp-rev1-validation-preparation/validation-inputs.json"
if((Get-TaskHash $taskAstcPath) -ne 'AE4E7EB656FEB7D6711EBDE3695C52884C1ADC156AFB6B139B40B94337E2E1EA' -or (Get-TaskHash $taskIspcPath) -ne 'DF45F46828446FE0113777546CFDE2983FC099588404548821C9D24A7B6E8710'){throw 'Accepted package source freeze changed'}
$taskOldFreeze=@(Get-Content -LiteralPath $taskOldFreezePath -Raw | ConvertFrom-Json)
$taskAstc=@(Get-Content -LiteralPath $taskAstcPath -Raw | ConvertFrom-Json)
$taskIspc=(Get-Content -LiteralPath $taskIspcPath -Raw | ConvertFrom-Json).entries
if($taskOldFreeze.Count -ne 15674 -or $taskAstc.Count -ne 712 -or $taskIspc.Count -ne 490){throw 'Reviewed freeze membership differs'}
$taskCatalog=[IO.Path]::GetFullPath("$taskRepo/cmake/3rdParty/Platform/Windows/BuiltInPackages_windows.cmake")
$taskMap=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$taskDeltas=[Collections.Generic.List[object]]::new()
function Add-TaskInput($entry,[string]$origin){
    $full=if([IO.Path]::IsPathFullyQualified($entry.path)){[IO.Path]::GetFullPath($entry.path)}else{[IO.Path]::GetFullPath((Join-Path $taskRepo $entry.path))}
    $fi=[IO.FileInfo]::new($full)
    if(-not $fi.Exists -or ($fi.Attributes -band [IO.FileAttributes]::ReparsePoint)){throw "Missing/reparse frozen input: $full"}
    $hash=Get-TaskHash $full
    $relative=if($full.StartsWith([IO.Path]::GetFullPath("$taskRepo/"),[StringComparison]::OrdinalIgnoreCase)){[IO.Path]::GetRelativePath($taskRepo,$full).Replace([char]92,[char]47)}else{$full}
    if($entry.PSObject.Properties['sha256'] -and ($entry.sha256 -ne $hash -or $entry.bytes -ne $fi.Length)){
        if($full -ne $taskCatalog -or $hash -ne $taskIntegration.catalogAfterSHA256){throw "Unreviewed Root drift: $relative"}
        $taskDeltas.Add(@{origin=$origin;path=$relative;beforeSHA256=$entry.sha256;afterSHA256=$hash;reason='Parent accepted two texture codec catalog bindings; exact inverse retained'})
    }
    if($taskMap.ContainsKey($full)){if($taskMap[$full].sha256 -ne $hash -or $taskMap[$full].bytes -ne $fi.Length){throw 'Root merge mismatch'};return}
    $taskMap.Add($full,@{path=$relative;bytes=$fi.Length;sha256=$hash.ToLowerInvariant();origin=$origin})
}
foreach($e in $taskOldFreeze){Add-TaskInput $e 'Original Root61 current reviewed inputs'}
foreach($e in $taskAstc){Add-TaskInput $e 'Parent accepted ASTC producer/archive/native consumers/guards inputs'}
foreach($e in $taskIspc){Add-TaskInput $e 'Parent accepted ISPC producer/archive/native consumers/guards inputs'}
foreach($p in $taskIntegration.packages){foreach($suffix in @('.tar.xz','.tar.xz.SHA256SUMS')){Add-TaskInput ([pscustomobject]@{path="$taskBase/root-package-cache/$($p.name)$suffix"}) 'Parent integrated actual package/cache witness'}}
foreach($p in @($taskIntegrationPath,"$taskBase/ispc-texcomp-rev1-validation-preparation/parent-focal-acceptance-rev1.json","$taskBase/astc-rev1-guard-preparation/parent-guard-result-review-rev1.json")){Add-TaskInput ([pscustomobject]@{path=$p}) 'Parent focal acceptance/integration witness'}
[void][IO.Directory]::CreateDirectory($taskNewRoot)
$taskFreezePath="$taskNewRoot/source-freeze-root-batch.json"
Write-TaskNew $taskFreezePath (($taskMap.Values | Sort-Object path | ConvertTo-Json -Depth 8)+"`n")
$taskAssignment=Get-Content -LiteralPath "$taskOldRoot/root-batch-assignment.json" -Raw | ConvertFrom-Json
$taskPriorArguments=@($taskAssignment.arguments)
$taskAssignment.purpose='Original offline Root61 after accepted ASTC/ISPCTexComp; identical native argv with current freeze. Editor/AP/runtime and effective dependency removal pending.'
$taskAssignment.sourceFreeze=$taskFreezePath
$taskAssignment.sourceFreezeSHA256=Get-TaskHash $taskFreezePath
$taskAssignment.sourceCount=$taskMap.Count
$taskAssignment.packageCacheFileCount=40
$taskAssignment.preparationStatus='PARENT REVIEW CANDIDATE; NATIVE NOT RUN'
$taskAssignment | Add-Member -NotePropertyName textureCodecsIntegrationSHA256 -NotePropertyValue $IntegrationSHA256
$taskAssignmentPath="$taskNewRoot/root-batch-assignment.json"
Write-TaskNew $taskAssignmentPath (($taskAssignment | ConvertTo-Json -Depth 12)+"`n")
$taskDriver=[IO.File]::ReadAllText($taskOldDriverPath)
$taskReplacements=@(
    @('root-dxc-local-final','root-texture-codecs-local-final'),
    @('9CF710AFC67CBFF9BF635E5E901911108A17B233940FE6EFBED7C083C48BA2F0',(Get-TaskHash $taskAssignmentPath)),
    @('AF617F8664743B42A80CC17A31475998C2F1A3819BE430C0FD073A4F7696EAA3',(Get-TaskHash $taskFreezePath)),
    @('15674',[string]$taskMap.Count),
    @('.Count -ne 36 -or $wheelFiles.Count -ne 44','.Count -ne 40 -or $wheelFiles.Count -ne 44'),
    @("'DirectXShaderCompilerDxc-1.8.2505.1-o3de-je-rev1-windows.tar.xz')","'DirectXShaderCompilerDxc-1.8.2505.1-o3de-je-rev1-windows.tar.xz','astc-encoder-3.2-je-rev1-windows.tar.xz','ISPCTexComp-36b80aa-je-rev1-windows.tar.xz')")
)
foreach($r in $taskReplacements){if(-not $taskDriver.Contains($r[0])){throw 'Reviewed driver replacement fragment absent'};$taskDriver=$taskDriver.Replace($r[0],$r[1])}
$taskLaunchBefore='[void]$process.Start()'
$taskLaunchAfter=@'
$configureLaunchRequestedUTC=[DateTime]::UtcNow.ToString('o')
[void]$process.Start()
$configureNativeStart=[ordered]@{wrapperPID=$PID;nativePID=$process.Id;launchRequestedUTC=$configureLaunchRequestedUTC;nativeProcessStartUTC=$process.StartTime.ToUniversalTime().ToString('o');recordedUTC=[DateTime]::UtcNow.ToString('o');executable=$cmake;executableSHA256=(Get-TaskInputHash -LiteralPath $cmake).Hash;arguments=$arguments;argumentCount=$arguments.Count;assignmentSHA256=$assignmentSha;freezeSHA256=$freezeSha}
$configureNativeStream=[IO.File]::Open((Join-Path $evidence 'configure-native-start.json'),[IO.FileMode]::CreateNew)
try{$configureNativeBytes=[Text.UTF8Encoding]::new($false).GetBytes(($configureNativeStart | ConvertTo-Json -Depth 6)+"`n");$configureNativeStream.Write($configureNativeBytes)}finally{$configureNativeStream.Dispose()}
'@
if([regex]::Matches($taskDriver,[regex]::Escape($taskLaunchBefore)).Count -ne 1){throw 'Configure launch fragment not unique'}
$taskDriver=$taskDriver.Replace($taskLaunchBefore,$taskLaunchAfter)
$tokens=$null;$errors=$null
[void][Management.Automation.Language.Parser]::ParseInput($taskDriver,[ref]$tokens,[ref]$errors)
if($errors.Count -or $taskPriorArguments.Count -ne 61 -or ($taskPriorArguments -join "`n") -cne ($taskAssignment.arguments -join "`n")){throw 'Derived driver syntax/argv differs'}
$taskDriverPath="$taskNewRoot/run-root-configure-texture-codecs-parent-rev1.ps1"
Write-TaskNew $taskDriverPath $taskDriver
$taskReceipt=[ordered]@{
    status='PARENT CURRENT FREEZE/DERIVED ORIGINAL ROOT61; NATIVE NOT RUN'
    sourceCount=$taskMap.Count;oldSourceCount=15674;reviewedCatalogDeltas=$taskDeltas.ToArray()
    arguments=61;argumentsUnchanged=$true;cacheFiles=40;wheelhouseFiles=44;syntaxErrors=0
    assignmentSHA256=(Get-TaskHash $taskAssignmentPath);freezeSHA256=(Get-TaskHash $taskFreezePath)
    donorDriverSHA256=(Get-TaskHash $taskOldDriverPath);driverPath=$taskDriverPath;driverSHA256=(Get-TaskHash $taskDriverPath)
    replacements=$taskReplacements;nativeLaunchReceiptAdded=$true;integrationSHA256=$IntegrationSHA256
    nativeOwner='/root/validation_luna';limits='Original configure only; source candidates still uncompiled; no engine acceptance or dependency footprint improvement.'
}
Write-TaskNew "$taskNewRoot/parent-preparation-review.json" (($taskReceipt | ConvertTo-Json -Depth 10)+"`n")
$taskReceipt | ConvertTo-Json -Depth 8
