#Requires -Version 7.0
$ErrorActionPreference='Stop'
$taskRepo='F:/JericoEngine/JericoEngine'
$taskBase="$taskRepo/build/n0-validation/local-packages"
$taskOldRoot="$taskBase/root-vvl-local-final"
$taskNewRoot="$taskBase/root-dxc-local-final"
if(Test-Path -LiteralPath $taskNewRoot){throw 'Preserve existing Root preparation'}
function Get-TaskHash([string]$path){
    $stream=[IO.File]::OpenRead($path)
    try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}
}
if((Get-TaskHash "$taskOldRoot/source-freeze-root-batch.json") -ne 'C92B8061F8E5E0FB48FBD419DBCB51DA2689355C6F9322B6A9282F4B73FF8A2E' -or (Get-TaskHash "$taskOldRoot/run-root-configure-vvl-parent-rev2.ps1") -ne 'BAC177F29A387A8F6C01A4733AC9C37DD008C24D211BFBD46B846EDFA8FB8492'){throw 'Reviewed Root donor changed'}
$taskWbRoot="$taskBase/whitebox-surface-materials-audit"
$taskWbFreeze="$taskWbRoot/source-freeze-whitebox-surface-materials-rev1.json"
if((Get-TaskHash $taskWbFreeze) -ne '52C12F26F2898076B99F2811523AC7B8311555780BAA3C8CCCA4639E1B5F4D5B' -or (Get-TaskHash "$taskWbRoot/parent-source-review-rev1.json") -ne 'AD1F09223E8121B13015763EA245DBF78A4EAA02C1E7587EEDA9A3847EF991CB'){throw 'WhiteBox parent review changed'}
$taskDxcReceipt="$taskBase/dxc-rev1-preparation/parent-final-package-integration.json"
if((Get-TaskHash $taskDxcReceipt) -ne 'C17C04CD193C82EDEB514B3C7C588C45E6813E9FB6276763076F8BCA7B9208C2'){throw 'DXC package acceptance changed'}
$taskOldFreeze=@(Get-Content -Raw -LiteralPath "$taskOldRoot/source-freeze-root-batch.json" | ConvertFrom-Json)
$taskWb=Get-Content -Raw -LiteralPath $taskWbFreeze | ConvertFrom-Json
$taskDxc=Get-Content -Raw -LiteralPath $taskDxcReceipt | ConvertFrom-Json
if($taskOldFreeze.Count -ne 15597 -or $taskWb.files.Count -ne 25 -or $taskWb.references.Count -ne 35 -or $taskDxc.providerFiles.Count -ne 12 -or $taskDxc.consumerCases -ne 4 -or $taskDxc.negativeCases -ne 18){throw 'Accepted input membership differs'}
$taskApproved=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($entry in @($taskWb.files)+@($taskWb.references)){$taskApproved[[IO.Path]::GetFullPath($entry.path)]=$entry}
$taskCatalog="$taskRepo/cmake/3rdParty/Platform/Windows/BuiltInPackages_windows.cmake"
$taskApproved[[IO.Path]::GetFullPath($taskCatalog)]=@{sha256=$taskDxc.catalogAfterSHA256;bytes=[IO.FileInfo]::new($taskCatalog).Length}
$taskMap=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$taskDeltas=[Collections.Generic.List[object]]::new()
function Add-TaskInput($entry,[string]$origin,[bool]$allowReviewedDelta){
    $path=if([IO.Path]::IsPathFullyQualified($entry.path)){$entry.path}else{Join-Path $taskRepo $entry.path}
    $full=[IO.Path]::GetFullPath($path);$file=[IO.FileInfo]::new($full)
    if(-not $file.Exists -or ($file.Attributes -band [IO.FileAttributes]::ReparsePoint)){throw "Missing/reparse Root input: $full"}
    $hash=Get-TaskHash $full
    $relative=if($full.StartsWith([IO.Path]::GetFullPath("$taskRepo/"),[StringComparison]::OrdinalIgnoreCase)){[IO.Path]::GetRelativePath($taskRepo,$full).Replace([char]92,[char]47)}else{$full}
    if($null -ne $entry.sha256 -and ($hash -ne $entry.sha256 -or $file.Length -ne $entry.bytes)){
        if(-not $allowReviewedDelta -or -not $taskApproved.ContainsKey($full) -or $taskApproved[$full].sha256 -ne $hash -or $taskApproved[$full].bytes -ne $file.Length){throw "Unreviewed Root drift: $relative"}
        $taskDeltas.Add(@{path=$relative;beforeSHA256=$entry.sha256;afterSHA256=$hash;reason='Parent reviewed WhiteBox surface materials or accepted DXC catalog binding'})
    }
    if($taskMap.ContainsKey($full)){if($taskMap[$full].sha256 -ne $hash -or $taskMap[$full].bytes -ne $file.Length){throw 'Root merge mismatch'};return}
    $taskMap.Add($full,@{path=$relative;bytes=$file.Length;sha256=$hash.ToLowerInvariant();origin=$origin})
}
foreach($entry in $taskOldFreeze){Add-TaskInput $entry 'Original Root61 reviewed inputs' $true}
foreach($entry in @($taskWb.files)+@($taskWb.references)){Add-TaskInput $entry 'WhiteBox surface/UV parent source review; native pending' $false}
foreach($entry in $taskDxc.providerFiles){Add-TaskInput $entry 'Parent accepted DXC provider' $false}
foreach($file in Get-ChildItem -LiteralPath "$taskBase/dxc-native-extracted-rev1" -File -Recurse -Force){Add-TaskInput @{path=$file.FullName} 'Parent accepted actual extracted DXC payload' $false}
foreach($path in @($taskDxcReceipt,"$taskWbRoot/parent-source-review-rev1.json","$taskBase/root-package-cache/DirectXShaderCompilerDxc-1.8.2505.1-o3de-je-rev1-windows.tar.xz","$taskBase/root-package-cache/DirectXShaderCompilerDxc-1.8.2505.1-o3de-je-rev1-windows.tar.xz.SHA256SUMS")){Add-TaskInput @{path=$path} 'Parent integration witness' $false}
[void][IO.Directory]::CreateDirectory($taskNewRoot)
$taskEncoding=[Text.UTF8Encoding]::new($false)
$taskFreezePath="$taskNewRoot/source-freeze-root-batch.json"
[IO.File]::WriteAllText($taskFreezePath,($taskMap.Values | Sort-Object path | ConvertTo-Json -Depth 7)+"`n",$taskEncoding)
$taskAssignment=Get-Content -Raw -LiteralPath "$taskOldRoot/root-batch-assignment.json" | ConvertFrom-Json
$taskPriorArguments=@($taskAssignment.arguments)
$taskAssignment.purpose='Original offline Root61 after accepted DXC and parent-reviewed WhiteBox materials/UV; identical native argv, current freeze. Editor/runtime pending.'
$taskAssignment.sourceFreeze=$taskFreezePath;$taskAssignment.sourceFreezeSHA256=Get-TaskHash $taskFreezePath;$taskAssignment.sourceCount=$taskMap.Count
$taskAssignmentPath="$taskNewRoot/root-batch-assignment.json"
[IO.File]::WriteAllText($taskAssignmentPath,($taskAssignment | ConvertTo-Json -Depth 10)+"`n",$taskEncoding)
$taskDriver=[IO.File]::ReadAllText("$taskOldRoot/run-root-configure-vvl-parent-rev2.ps1")
$taskReplacements=@(
    @('root-vvl-local-final','root-dxc-local-final'),
    @('7C4BAE03D9641E44F1AE0A4F7DD615081A357DBBAB176FFE5A2FCF55438D572A',(Get-TaskHash $taskAssignmentPath)),
    @('C92B8061F8E5E0FB48FBD419DBCB51DA2689355C6F9322B6A9282F4B73FF8A2E',(Get-TaskHash $taskFreezePath)),
    @('15597',[string]$taskMap.Count),
    @('.Count -ne 34 -or $wheelFiles.Count -ne 44','.Count -ne 36 -or $wheelFiles.Count -ne 44'),
    @("'vulkan-validationlayers-1.3.261-je-rev1-windows.tar.xz')","'vulkan-validationlayers-1.3.261-je-rev1-windows.tar.xz','DirectXShaderCompilerDxc-1.8.2505.1-o3de-je-rev1-windows.tar.xz')")
)
foreach($replacement in $taskReplacements){if(-not $taskDriver.Contains($replacement[0])){throw 'Reviewed Root driver fragment absent'};$taskDriver=$taskDriver.Replace($replacement[0],$replacement[1])}
$tokens=$null;$errors=$null
[void][Management.Automation.Language.Parser]::ParseInput($taskDriver,[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Derived Root driver parser errors'}
if($taskPriorArguments.Count -ne 61 -or ($taskPriorArguments -join "`n") -cne ($taskAssignment.arguments -join "`n")){throw 'Native Root argv changed'}
$taskDriverPath="$taskNewRoot/run-root-configure-dxc-parent-rev1.ps1"
[IO.File]::WriteAllText($taskDriverPath,$taskDriver,$taskEncoding)
$taskReceipt=[ordered]@{status='PARENT CURRENT FREEZE/DERIVED ROOT DRIVER; NATIVE NOT RUN';sourceCount=$taskMap.Count;oldSourceCount=15597;reviewedDeltas=@($taskDeltas.ToArray());arguments=61;argumentsUnchanged=$true;cacheFiles=36;wheelhouseFiles=44;assignmentSHA256=(Get-TaskHash $taskAssignmentPath);freezeSHA256=(Get-TaskHash $taskFreezePath);driverPath=$taskDriverPath;driverSHA256=(Get-TaskHash $taskDriverPath);syntaxErrors=0;nativeOwner='Existing validation_luna only';limits='Original configuration route with uncompiled Python/WhiteBox/SMC/SQLite candidates. No baseline HEAD, Editor, runtime or visual acceptance.'}
[IO.File]::WriteAllText("$taskNewRoot/parent-preflight-review.json",($taskReceipt | ConvertTo-Json -Depth 8)+"`n",$taskEncoding)
$taskReceipt | ConvertTo-Json -Depth 8
