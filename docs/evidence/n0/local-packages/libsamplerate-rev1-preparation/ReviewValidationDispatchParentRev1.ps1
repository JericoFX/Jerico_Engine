# Parent data-only integration review; no native build or test invocation.
#Requires -Version 7.0
#Requires -PSEdition Core
$ErrorActionPreference='Stop'
$taskRepo='F:/JericoEngine/JericoEngine'
$taskBase=Join-Path $taskRepo 'build/n0-validation/local-packages'
$taskPrep=Join-Path $taskBase 'libsamplerate-rev1-validation-preparation'
$taskAudit=Join-Path $taskBase 'libsamplerate-rev1-package-audit'
function Hash-TaskFile([string]$path){$s=[IO.File]::OpenRead($path);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($s))}finally{$s.Dispose()}}
function Read-TaskJson([string]$path){[IO.File]::ReadAllText($path)|ConvertFrom-Json}
$taskFreezePath=Join-Path $taskPrep 'driver-freeze.json'
if((Hash-TaskFile $taskFreezePath) -cne '6899CF7A787BB3FF53ADD6188CFE41D63F985CEC4D0E03EE3F36B7CDA90B84A6'){throw 'Driver freeze identity differs'}
$taskFreeze=Read-TaskJson $taskFreezePath
if($taskFreeze.entries.Count -ne 177){throw 'Driver freeze count differs'}
foreach($taskEntry in $taskFreeze.entries){
    if([IO.FileInfo]::new($taskEntry.path).Length -ne $taskEntry.bytes -or (Hash-TaskFile $taskEntry.path) -cne $taskEntry.sha256){throw "Frozen input drift: $($taskEntry.path)"}
    $taskNode=[IO.FileInfo]::new($taskEntry.path)
    while($taskNode){if($taskNode.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Reparse input rejected'};if($taskNode -is [IO.FileInfo]){$taskNode=$taskNode.Directory}else{$taskNode=$taskNode.Parent}}
}
$taskInputs=Read-TaskJson (Join-Path $taskPrep 'validation-inputs.json')
if($taskInputs.entries.Count -ne 174 -or $taskInputs.count -ne 174){throw 'Validation inputs count differs'}
$taskAssignment=Read-TaskJson (Join-Path $taskPrep 'assignment.json')
if((Hash-TaskFile (Join-Path $taskPrep 'assignment.json')) -cne '54747431982F0AFB1196E29A9751414584F2D90E9A3546C4A6FC3595A258505D'){throw 'Assignment identity differs'}
$taskAuthority=Read-TaskJson $taskAssignment.consumer.nativeConfigureArgumentAuthority
if((Hash-TaskFile $taskAssignment.consumer.nativeConfigureArgumentAuthority) -cne 'B623FE1D877D8F83A22E9D5D124C13E3DDEF875AD0A9BDC0D06FD821093BC585' -or $taskAuthority.consumer.count -ne 11 -or $taskAuthority.guards.Count -ne 8){throw 'Native argument authority differs'}
$taskArchiveAudit=Read-TaskJson (Join-Path $taskAudit 'archive-audit-before-extraction.json')
$taskCoff=Read-TaskJson (Join-Path $taskAudit 'coff-static-library-audit.json')
$taskSdk=$taskAssignment.consumer.sdkRoot
$taskSdkFiles=@(Get-ChildItem -LiteralPath $taskSdk -Recurse -File)
if($taskSdkFiles.Count -ne 24 -or $taskArchiveAudit.entries.Count -ne 24 -or $taskArchiveAudit.contentManifestEntries -ne 23 -or -not $taskArchiveAudit.manifestCoverageComplete -or $taskArchiveAudit.links -ne 0 -or $taskArchiveAudit.caseCollisions -ne 0 -or $taskArchiveAudit.sourceNoticeFindPairs -ne 20){throw 'Archive/SDK coverage differs'}
foreach($taskEntry in $taskArchiveAudit.entries){$taskPath=Join-Path $taskSdk $taskEntry.path;if([IO.FileInfo]::new($taskPath).Length -ne $taskEntry.bytes -or (Hash-TaskFile $taskPath) -cne $taskEntry.sha256){throw 'SDK differs from archive audit'}}
if($taskCoff.machine -cne 'AMD64' -or $taskCoff.objectMembers.Count -ne 4 -or $taskCoff.apiDefinitionsCount -ne 20 -or $taskCoff.dynamicImportObjects -ne 0 -or $taskCoff.pythonSymbolReferences -ne 0){throw 'Static library COFF/API differs'}
$taskSource='F:/JericoEngine/Dependencies/libsamplerate-a147b6dc4046deda22b2aa772b348717de772e2b'
$taskProvider=Join-Path $taskRepo 'cmake/Tools/LocalPackages/LibSamplerate'
$taskPairs=@(
    @('libsamplerate/COPYING',(Join-Path $taskSource 'COPYING')),
    @('LICENSES/libsamplerate/COPYING',(Join-Path $taskSource 'COPYING')),
    @('libsamplerate/include/samplerate.h',(Join-Path $taskSource 'include/samplerate.h')),
    @('LICENSES/libsamplerate/source-notices/samplerate.h',(Join-Path $taskSource 'include/samplerate.h')),
    @('LICENSES/libsamplerate/SourceManifest.json',(Join-Path $taskBase 'libsamplerate-rev1-audit/source-manifest.json')),
    @('JERequireLocalPath.cmake',(Join-Path $taskRepo 'cmake/Tools/LocalPackages/Python/JERequireLocalPath.cmake'))
)
foreach($taskName in @('Findlibsamplerate.cmake','README.md','NOTICE.txt')){$taskPairs+=,@($taskName,(Join-Path $taskProvider $taskName))}
foreach($taskName in @('LICENSE.txt','LICENSE_APACHE2.TXT','LICENSE_MIT.TXT')){$taskPairs+=,@("LICENSES/O3DE/$taskName",(Join-Path $taskRepo $taskName))}
foreach($taskFile in @(Get-ChildItem -LiteralPath (Join-Path $taskSource 'src') -File|Where-Object{$_.Extension -in @('.c','.h')})){$taskPairs+=,@("LICENSES/libsamplerate/source-notices/$($taskFile.Name)",$taskFile.FullName)}
if($taskPairs.Count -ne 20){throw 'Source/notice pair count differs'}
foreach($taskPair in $taskPairs){if((Hash-TaskFile (Join-Path $taskSdk $taskPair[0])) -cne (Hash-TaskFile $taskPair[1])){throw "Source/API/notice pair differs: $($taskPair[0])"}}
foreach($taskName in @('RunConsumer.ps1','RunGuards.ps1','CommonExecution.ps1')){$taskTokens=$null;$taskErrors=$null;[Management.Automation.Language.Parser]::ParseFile((Join-Path $taskPrep $taskName),[ref]$taskTokens,[ref]$taskErrors)|Out-Null;if($taskErrors.Count){throw 'Driver parse error'}}
foreach($taskAbsent in @($taskAssignment.consumer.freshBuild,$taskAssignment.consumer.evidence,$taskAssignment.guards.fixtures,$taskAssignment.guards.evidence)){if(Test-Path -LiteralPath $taskAbsent){throw "Native output must be absent: $taskAbsent"}}
$taskReceipt=[ordered]@{status='PARENT DISPATCH DATA REVIEW PASS; NATIVE NOT RUN';utc=[DateTime]::UtcNow.ToString('o');currentFrozenInputs=177;inputDeltas=0;consumerAndGuardsInputCount=174;SDKFiles=24;SDKArchiveBytesExact=$true;sourceApiNoticePairs=20;sourceApiNoticePairsHashEqual=$true;staticLibraryAMD64=$true;staticLibraryObjects=4;publicCApiDefinitions=20;driversParsed=3;assignmentSHA256=(Hash-TaskFile (Join-Path $taskPrep 'assignment.json'));driverFreezeSHA256=(Hash-TaskFile $taskFreezePath);archiveAuditSHA256=(Hash-TaskFile (Join-Path $taskAudit 'archive-audit-before-extraction.json'));coffReceiptSHA256=(Hash-TaskFile (Join-Path $taskAudit 'coff-static-library-audit.json'));consumerExpectedCasesPerConfiguration=7;guardsExpectedRejections=8;guardOldOutputScope='Existence only, not full private old-build manifest';CRT='Release library /MD with Release /MD and Debug /MDd consumer; no general cross-CRT acceptance';engineAccepted=$false;nativeExecuted=$false}
$taskOut=Join-Path $taskPrep 'parent-dispatch-review-rev1.json'
$taskStream=[IO.File]::Open($taskOut,[IO.FileMode]::CreateNew)
try{$taskBytes=[Text.UTF8Encoding]::new($false).GetBytes(($taskReceipt|ConvertTo-Json -Depth 6)+"`n");$taskStream.Write($taskBytes,0,$taskBytes.Length)}finally{$taskStream.Dispose()}
$taskReceipt|ConvertTo-Json -Compress
