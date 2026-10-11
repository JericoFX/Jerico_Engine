$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$engine='F:/JericoEngine/JericoEngine'
$audit="$engine/build/n0-validation/local-packages/libsamplerate-rev1-audit"
$provider="$engine/cmake/Tools/LocalPackages/LibSamplerate"
$source='F:/JericoEngine/Dependencies/libsamplerate-a147b6dc4046deda22b2aa772b348717de772e2b'
$archive='F:/JericoEngine/Dependencies/archives/libsamplerate-a147b6dc4046deda22b2aa772b348717de772e2b.tar.gz'
$research="$engine/build/n0-validation/local-packages/n0-remaining-packages-audit-rev1/research"
$preparation="$engine/build/n0-validation/local-packages/libsamplerate-rev1-preparation"
if(Test-Path -LiteralPath $preparation){throw 'preserve preparation'}
[IO.Directory]::CreateDirectory($preparation)|Out-Null
function Hash([string]$path){(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash}
function NewBytes([string]$path,[byte[]]$bytes){$s=[IO.File]::Open($path,[IO.FileMode]::CreateNew);try{$s.Write($bytes,0,$bytes.Length)}finally{$s.Dispose()}}
function NewText([string]$path,[string]$text){NewBytes $path ([Text.UTF8Encoding]::new($false).GetBytes($text))}
function NewJson([string]$path,$obj){NewText $path (($obj|ConvertTo-Json -Depth 20)+"`n")}
function Entry([string]$path,[string]$role){[ordered]@{path=$path.Replace('\','/');bytes=([IO.FileInfo]::new($path)).Length;sha256=Hash $path;role=$role}}
$files=@(Get-ChildItem -LiteralPath $provider -Recurse -File | Sort-Object FullName)
if($files.Count -ne 6){throw 'unexpected candidate scope'}
$diff=''
foreach($file in $files){$rel=$file.FullName.Substring($engine.Length+1).Replace('\','/');$text=[IO.File]::ReadAllText($file.FullName);$lines=@($text.TrimEnd("`n").Split("`n"));$diff+="diff --git a/$rel b/$rel`nnew file mode 100644`n--- /dev/null`n+++ b/$rel`n@@ -0,0 +1,$($lines.Count) @@`n";foreach($line in $lines){$diff+='+'+$line+"`n"}}
NewText "$audit/source-diff-rev1.patch" $diff
$manifest=Get-Content -LiteralPath "$audit/source-manifest.json" -Raw|ConvertFrom-Json
$entries=@()
foreach($file in $files){$entries+=Entry $file.FullName 'candidate-owned'}
foreach($e in $manifest.entries){$actual=Entry "$source/$($e.path)" 'immutable-source';if($actual.sha256 -cne $e.sha256 -or $actual.bytes -ne $e.bytes){throw 'immutable source changed'};$entries+=$actual}
$entries+=Entry $archive 'immutable-archive'
foreach($name in @('source-manifest.json','primary-tree.json','source-eol-attribution-proof.json','archive-audit-before-extraction.json','source-extraction-receipt.json','source-get.json','source-head.json','primary-def-blob.json','primary-def.raw','AuditSourceV2.py')){$entries+=Entry "$audit/$name" 'source-audit-authority'}
foreach($name in @('libsamplerate/build_config.json','libsamplerate/Findlibsamplerate.cmake.template','samplerate-history-original-build_config.json','samplerate-tag.json','samplerate-annotated-tag','native-recipes-research-summary.json')){$entries+=Entry "$research/$name" 'approved-research-recipe'}
foreach($rel in @('cmake/Tools/LocalPackages/JECreateLocalPackage.cmake','cmake/Tools/LocalPackages/Python/JERequireLocalPath.cmake','LICENSE.txt','LICENSE_APACHE2.TXT','LICENSE_MIT.TXT','Gems/Microphone/Code/CMakeLists.txt','Gems/Microphone/Code/Source/Platform/Windows/platform_windows.cmake','Gems/Microphone/Code/Source/Platform/Windows/MicrophoneSystemComponent_Windows.cpp')){$entries+=Entry "$engine/$rel" 'existing-shared-authority-readonly'}
$old=Get-Content -LiteralPath "$engine/build/n0-validation/local-packages/ispc-texcomp-rev1-audit/source-freeze-ispc-texcomp-rev2.json" -Raw|ConvertFrom-Json
foreach($e in $old.entries | Where-Object {$_.path -match 'cl.exe$|MSBuild.exe$|vcvarsall.bat$'}){$entries+=Entry $e.path 'installed-tool-readonly'}
foreach($path in @('C:/Program Files/CMake/bin/cmake.exe','C:/Program Files/CMake/bin/ctest.exe','C:/Users/Jerico/.cache/codex-runtimes/codex-primary-runtime/dependencies/native/powershell/pwsh.exe')){$entries+=Entry $path 'installed-tool-readonly'}
if(@($entries.path | Group-Object | Where-Object Count -gt 1).Count){throw 'duplicate freeze input'}
$sourceFreeze="$audit/source-freeze-libsamplerate-rev1.json"
NewJson $sourceFreeze ([ordered]@{status='FROZEN_FOR_PARENT_REVIEW; NATIVE_NOT_RUN';count=$entries.Count;candidateFiles=6;immutableSourceFiles=99;entries=$entries})
$producer=[string[]]@("-DJE_SAMPLERATE_SOURCE_ARCHIVE=$archive","-DJE_SAMPLERATE_SOURCE_MANIFEST=$audit/source-manifest.json","-DJE_SAMPLERATE_OUTPUT=$engine/build/n0-validation/local-packages/libsamplerate-rev1-build",'-P',"$provider/BuildLibSampleratePackage.cmake")
$consumer=[string[]]@('-S',"$provider/tests",'-B',"$engine/build/n0-validation/local-packages/libsamplerate-rev1-consumer",'-G','Visual Studio 17 2022','-A','x64','-T','v143',"-DJE_SAMPLERATE_PACKAGE_ROOT=$engine/build/n0-validation/local-packages/libsamplerate-rev1-package-audit/extracted/libsamplerate-0.2.1-je-rev1-windows")
$guardDefs=@(
 @('uri','JE_SAMPLERATE_SOURCE_ARCHIVE','https://invalid.example/libsamplerate.tar.gz','must not be a URI'),
 @('unc','JE_SAMPLERATE_SOURCE_ARCHIVE','//localhost/not-a-local-input/libsamplerate.tar.gz','absolute local path'),
 @('missing-archive','JE_SAMPLERATE_SOURCE_ARCHIVE',"$engine/build/n0-validation/local-packages/libsamplerate-rev1-guards/absent.tar.gz",'source archive file is missing'),
 @('archive-hash','JE_SAMPLERATE_SOURCE_ARCHIVE',"$engine/build/n0-validation/local-packages/libsamplerate-rev1-guards/wrong.tar.gz",'source archive SHA256 mismatch'),
 @('missing-manifest','JE_SAMPLERATE_SOURCE_MANIFEST',"$engine/build/n0-validation/local-packages/libsamplerate-rev1-guards/absent.json",'source manifest file is missing'),
 @('manifest-hash','JE_SAMPLERATE_SOURCE_MANIFEST',"$engine/build/n0-validation/local-packages/libsamplerate-rev1-guards/wrong.json",'source manifest SHA256 mismatch'),
 @('external-output','JE_SAMPLERATE_OUTPUT','F:/JericoEngine/Dependencies/libsamplerate-forbidden-output','output must be beneath this checkout/build'),
 @('old-output','JE_SAMPLERATE_OUTPUT',"$engine/build/n0-validation/local-packages/libsamplerate-rev1-build",'output must be new'))
$guards=@();foreach($g in $guardDefs){$guards+=[ordered]@{name=$g[0];replaceVariable=$g[1];value=$g[2];expectedDiagnostic=$g[3];expectedNonzero=$true;expectedNoNewOutput=$true}}
$argsFile="$audit/validation-arguments-rev1.json"
NewJson $argsFile ([ordered]@{status='PREPARED_NOT_RUN';producer=[ordered]@{count=$producer.Count;arguments=$producer};consumer=[ordered]@{count=$consumer.Count;arguments=$consumer;configurations=@('Release','Debug');cases=@('api','conversions','fast','medium','best','reset','errors');archiveAuditBeforeConsumer=$true};guards=$guards;guardPrerequisites='After retained producer output exists. New guard-owned wrong.tar.gz and wrong.json are plain invalid local bytes, no immutable input alteration; output unchanged means directory existence, not a full old-build manifest.';executionOwner='Luna after parent review/dispatch';retry=$false})
$reader='F:/JericoEngine/JericoEngine/build/n0-validation/original-engine-deps/Python-rev3/packages/python-3.10.13-je-rev3-windows/python/python.exe'
NewJson "$audit/candidate-receipt-rev1.json" ([ordered]@{status='SOURCE_CANDIDATE_PREPARED; PRODUCER_CONSUMER_GUARDS_NATIVE_NOT_RUN';sourceCommit='a147b6dc4046deda22b2aa772b348717de772e2b';version='0.2.1';archiveSHA256=Hash $archive;archiveBytes=([IO.FileInfo]::new($archive)).Length;sourceFiles=99;rawExactPrimaryFiles=98;attributeExplainedCRLFFiles=1;sourceBytesRewritten=$false;sourceDiffSHA256=Hash "$audit/source-diff-rev1.patch";sourceFreezeSHA256=Hash $sourceFreeze;sourceFreezeCount=$entries.Count;argumentAuthoritySHA256=Hash $argsFile;candidateFiles=@($files | ForEach-Object {Entry $_.FullName 'candidate-owned'});externalDataReader=[ordered]@{path=$reader;sha256=Hash $reader;arguments=@('-I','-B');scope='stdlib tar/hash/path/json audit only';firstRawBlobCheckExit=1;secondAttributedCheckExit=0;sourceExecuted=$false;firstFailurePreserved=$true};closure='C99 samplerate static, all converters; no downloads or native transitive dependency in selected Windows closure; CheckCSourceRuns processor clipping remains upstream and pending';consumerScope='seven cases Release/MD and Debug/MDd against Release/MD library; mixed CRT general compatibility not asserted';engineAccepted=$false;catalogIntegrated=$false;noGit=$true;noNativeExecuted=$true;noDelegation=$true})
# Reuse the already reviewed producer capture pattern, adapted only to these
# literal arguments/authorities. This script is created and parsed, not run.
$driver=[IO.File]::ReadAllText("$engine/build/n0-validation/local-packages/ispc-texcomp-rev2-preparation/RunProducer.ps1")
$start=$driver.IndexOf('$nativeArguments=[string[]]@(');$end=$driver.IndexOf('$output=', $start)
$argumentLiteral='$nativeArguments=[string[]]@('+"`n"+ (($producer | ForEach-Object {" '$_'"}) -join ",`n")+")`n"
$driver=$driver.Substring(0,$start)+$argumentLiteral+$driver.Substring($end)
$driver=$driver.Replace('ispc-texcomp-rev1-audit','libsamplerate-rev1-audit').Replace('source-freeze-ispc-texcomp-rev2.json','source-freeze-libsamplerate-rev1.json').Replace('validation-arguments-rev2.json','validation-arguments-rev1.json').Replace('ispc-texcomp-rev1-build','libsamplerate-rev1-build').Replace('ISPCTexComp','libsamplerate')
$driver=$driver.Replace('5A94E4F3E8E84D0ADE4DFD3CDFF9A2027314C465B2B5B78ACD5B48B26E28BD47',(Hash $sourceFreeze)).Replace('35B6A7D77269F60E2A067B401980E01C3F4576E2F755D0DE09CFB3B1851127F4',(Hash $argsFile))
$driver=$driver.Replace('458',$entries.Count.ToString()).Replace('$nativeArguments.Count -ne 11','$nativeArguments.Count -ne 5').Replace('$approved.producer.count -ne 11','$approved.producer.count -ne 5').Replace('$i -lt 11','$i -lt 5')
$driverPath="$preparation/RunProducer.ps1";NewText $driverPath $driver
$tokens=$null;$errors=$null;[Management.Automation.Language.Parser]::ParseFile($driverPath,[ref]$tokens,[ref]$errors)|Out-Null
if($errors.Count){throw 'prepared driver parse error'}
NewJson "$preparation/assignment.json" ([ordered]@{status='PREPARED_NOT_DISPATCHED';owner='Luna only after parent review';scope='Single offline producer; no consumer/guards/Root/retry';launcher='C:/Users/Jerico/.cache/codex-runtimes/codex-primary-runtime/dependencies/native/powershell/pwsh.exe';launcherSHA256='362A356CE7F0940EC74F73A8FC2C990A2CC24A38A11C90BBD8ECA947110AD139';wrapperArguments=@('-NoLogo','-NoProfile','-File',$driverPath);scriptSHA256=Hash $driverPath;sourceFreeze=$sourceFreeze;sourceFreezeSHA256=Hash $sourceFreeze;inputs=$entries.Count;nativeArgumentCount=5;freshOutput="$engine/build/n0-validation/local-packages/libsamplerate-rev1-build";runEvidence="$preparation/producer-run";nativeNotRun=$true;driverParseErrors=0;consumerRequiresFreshAudit=$true})
$driverEntries=@($entries)
foreach($path in @($sourceFreeze,$argsFile,"$audit/source-diff-rev1.patch","$audit/candidate-receipt-rev1.json",$driverPath,"$preparation/assignment.json")){$driverEntries+=Entry $path 'driver-authority'}
NewJson "$preparation/driver-freeze.json" ([ordered]@{status='PREPARED_NOT_RUN';count=$driverEntries.Count;entries=$driverEntries})
$immutableAfter=0;foreach($e in $manifest.entries){if((Hash "$source/$($e.path)") -cne $e.sha256){throw 'snapshot postflight differs'};$immutableAfter++}
NewJson "$audit/preparation-postflight-rev1.json" ([ordered]@{status='STATIC_PREPARATION_CHECKS_ONLY';immutableSourceMatches=$immutableAfter;immutableSourceCount=99;archiveMatches=((Hash $archive) -ceq 'F90E46BBF9BAE05139E297CEEBD5394EF77CE5E294E6B46BEFFCC15B9446A041');sourceCandidateCount=6;driverParseErrors=0;sourceFreezeCount=$entries.Count;driverFreezeCount=$driverEntries.Count;sourceFreezeSHA256=Hash $sourceFreeze;driverFreezeSHA256=Hash "$preparation/driver-freeze.json";producerOutputExists=(Test-Path "$engine/build/n0-validation/local-packages/libsamplerate-rev1-build");nativeExecuted=$false})
Get-Content -LiteralPath "$audit/preparation-postflight-rev1.json"