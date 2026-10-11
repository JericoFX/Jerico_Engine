#Requires -Version 7.0
#Requires -PSEdition Core
$ErrorActionPreference='Stop'
$taskRepo='F:\JericoEngine\JericoEngine'
$taskPub=Join-Path $taskRepo 'build/n0-validation/local-packages/publication-texture-codecs-parent-rev1'
if([IO.Directory]::Exists($taskPub)){throw 'Publication preparation already exists; preserve it'}
$taskBranch=git branch --show-current
$taskExit=$LASTEXITCODE
if($taskExit -ne 0 -or $taskBranch -ne 'development'){throw 'Expected development'}
$taskHead=git rev-parse HEAD
$taskExit=$LASTEXITCODE
if($taskExit -ne 0 -or $taskHead -ne 'c7f23309e1e69ccb669cd0e6626fd53818ca6769'){throw 'Unexpected publication base'}
$taskIndex=@(git diff --cached --name-only)
$taskExit=$LASTEXITCODE
if($taskExit -ne 0 -or $taskIndex.Count){throw 'Unknown existing index; preserve and inspect'}
function Hash-TaskFile([string]$path){$stream=[IO.File]::OpenRead($path);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}}
$taskReceiptPath=Join-Path $taskRepo 'build/n0-validation/local-packages/texture-codecs-parent-rev1/parent-package-integration.json'
if((Hash-TaskFile $taskReceiptPath) -ne 'B56F98A47D4D0BB5B4DA14EF9DE267141A482E97FAD7A2CC83C7BEE57D3371EE'){throw 'Integration receipt drift'}
$taskReceipt=[IO.File]::ReadAllText($taskReceiptPath)|ConvertFrom-Json
if($taskReceipt.providerFiles.Count -ne 17 -or $taskReceipt.engineAccepted){throw 'Unexpected acceptance scope'}
$taskOwned=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($taskFile in $taskReceipt.providerFiles){
    $taskPath=Join-Path $taskRepo $taskFile.path
    if([IO.FileInfo]::new($taskPath).Length -ne $taskFile.bytes -or (Hash-TaskFile $taskPath) -ne $taskFile.sha256){throw "Provider drift: $($taskFile.path)"}
    [void]$taskOwned.Add($taskFile.path)
}
foreach($taskPath in @('.gitattributes','docs/work/ACTIVE_SLICE.md','docs/work/HANDOFF.md','cmake/3rdParty/Platform/Windows/BuiltInPackages_windows.cmake')){[void]$taskOwned.Add($taskPath)}
if((Hash-TaskFile (Join-Path $taskRepo 'cmake/3rdParty/Platform/Windows/BuiltInPackages_windows.cmake')) -ne $taskReceipt.catalogAfterSHA256){throw 'Catalog drift'}
$taskPaths=@(git -c core.quotepath=false ls-files --modified --others --exclude-standard)
$taskExit=$LASTEXITCODE
if($taskExit -ne 0){throw 'Cannot inventory changed files'}
$taskPreserved=@(foreach($taskPath in $taskPaths | Sort-Object -Unique){
    if($taskOwned.Contains($taskPath) -or $taskPath.StartsWith('docs/evidence/n0/') -or $taskPath.StartsWith('cmake/Tools/LocalPackages/LibSamplerate/') -or $taskPath.StartsWith('cmake/Tools/LocalPackages/OpenImageIO/')){continue}
    $taskFull=Join-Path $taskRepo $taskPath
    if(-not [IO.File]::Exists($taskFull)){throw "Unknown changed path missing: $taskPath"}
    if([IO.File]::GetAttributes($taskFull) -band [IO.FileAttributes]::ReparsePoint){throw 'Unknown path reparse point'}
    [ordered]@{path=$taskPath;bytes=[IO.FileInfo]::new($taskFull).Length;sha256=(Hash-TaskFile $taskFull)}
})
[void][IO.Directory]::CreateDirectory($taskPub)
[IO.File]::WriteAllText((Join-Path $taskPub 'preserved-before.json'),([ordered]@{baseSHA=$taskHead;branch=$taskBranch;files=$taskPreserved;futureAuthorizedWriterPrefixes=@('cmake/Tools/LocalPackages/LibSamplerate/','cmake/Tools/LocalPackages/OpenImageIO/')}|ConvertTo-Json -Depth 6),[Text.UTF8Encoding]::new($false))
$taskDonor=Join-Path $taskRepo 'build/n0-validation/local-packages/publication-dxc-parent-rev1/verify-staged-evidence-dxc-parent-rev1.ps1'
$taskVerifier=[IO.File]::ReadAllText($taskDonor)
$taskOld=@"
`$taskDxcReceipt=Get-Content -Raw -LiteralPath (Join-Path `$taskRepo 'build/n0-validation/local-packages/dxc-rev1-preparation/parent-final-package-integration.json') | ConvertFrom-Json
`$taskDxcFiles=@(`$taskDxcReceipt.providerFiles)
if(`$taskDxcFiles.Count -ne 12){throw 'Accepted DXC source count differs'}
foreach(`$taskSource in `$taskDxcFiles){`$taskMasterMap.Add(`$taskSource.path,`$taskSource.sha256)}
"@
$taskNew=@"
`$taskCodecReceipt=Get-Content -Raw -LiteralPath (Join-Path `$taskRepo 'build/n0-validation/local-packages/texture-codecs-parent-rev1/parent-package-integration.json') | ConvertFrom-Json
`$taskCodecFiles=@(`$taskCodecReceipt.providerFiles)
if(`$taskCodecFiles.Count -ne 17){throw 'Accepted codec source count differs'}
foreach(`$taskSource in `$taskCodecFiles){`$taskMasterMap.Add(`$taskSource.path,`$taskSource.sha256)}
"@
if(-not $taskVerifier.Contains($taskOld)){throw 'Donor source block changed'}
$taskVerifier=$taskVerifier.Replace('cmake/Tools/LocalPackages/DirectXShaderCompilerDxc','cmake/Tools/LocalPackages/ASTCEncoder cmake/Tools/LocalPackages/ISPCTexComp').Replace($taskOld,$taskNew).Replace('acceptedDxcProviderEntries=$taskDxcFiles.Count','acceptedCodecProviderEntries=$taskCodecFiles.Count')
if($taskVerifier.Contains('taskDxc')){throw 'Stale donor variable'}
$taskParseTokens=$null;$taskParseErrors=$null
[void][Management.Automation.Language.Parser]::ParseInput($taskVerifier,[ref]$taskParseTokens,[ref]$taskParseErrors)
if($taskParseErrors.Count){throw 'Generated verifier parse errors'}
$taskVerifierPath=Join-Path $taskPub 'verify-staged-evidence-texture-codecs-parent-rev1.ps1'
[IO.File]::WriteAllText($taskVerifierPath,$taskVerifier,[Text.UTF8Encoding]::new($false))
[ordered]@{status='PUBLICATION_PREPARATION_READY';preservedFiles=$taskPreserved.Count;providerFiles=17;baseSHA=$taskHead;verifierSHA256=(Hash-TaskFile $taskVerifierPath);integrationSHA256=(Hash-TaskFile $taskReceiptPath)} | ConvertTo-Json -Compress
