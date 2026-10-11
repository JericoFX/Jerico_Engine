# Data-only candidate finalizer. No native execution or source edits.
$ErrorActionPreference='Stop'
$root='F:/JericoEngine/JericoEngine'
$out="$root/build/n0-validation/local-packages/astc-rev1-preparation"
$audit="$root/build/n0-validation/local-packages/astc-rev1-audit"
$provider="$root/cmake/Tools/LocalPackages/ASTCEncoder"
$cmake='C:/Program Files/CMake/bin/cmake.exe'
$source='F:/JericoEngine/Dependencies/astc-encoder-f32d8d4dd9e06f8986cd5e10f12e199fc023f150'
$archive='F:/JericoEngine/Dependencies/archives/astc-encoder-f32d8d4dd9e06f8986cd5e10f12e199fc023f150.tar.gz'
$producerOut="$root/build/n0-validation/local-packages/astc-local-rev1"
$consumerOut="$root/build/n0-validation/local-packages/astc-native-consumer-rev1"
$extracted="$root/build/n0-validation/local-packages/astc-native-extracted-rev1"
function Hash([string]$path){$stream=[IO.File]::OpenRead($path); try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}}
$producerArgs=@("-DJE_ASTC_SOURCE_ARCHIVE=$archive",'-DJE_ASTC_SOURCE_SHA256=291c26fcf20b8e23a6eb1ad4dd75f5afecf79d316c0477c532df6d5693291403','-DJE_ASTC_SOURCE_URL=https://codeload.github.com/ARM-software/astc-encoder/tar.gz/f32d8d4dd9e06f8986cd5e10f12e199fc023f150',"-DJE_ASTC_OUTPUT=$producerOut",'-P',"$provider/BuildASTCEncoderPackage.cmake")
$assignment=[ordered]@{status='PENDING PARENT REVIEW / LUNA NATIVE EXECUTION';nativeExecuted=$false;nativeOwner='Luna';producer=[ordered]@{executable=$cmake;argv=$producerArgs;freshOutput=$producerOut};package=[ordered]@{identity='astc-encoder-3.2-je-rev1-windows';archive="$producerOut/packages/astc-encoder-3.2-je-rev1-windows.tar.xz";sidecar="$producerOut/packages/astc-encoder-3.2-je-rev1-windows.tar.xz.SHA256SUMS";sha256='MEASURE FROM REAL PRODUCTION';extractedRoot=$extracted;extract='Audit real produced archive paths/types/links/hashes before fresh extraction; never substitute stage for consumer'};consumerConfigure=@('-S',"$provider/tests",'-B',$consumerOut,'-G','Visual Studio 17 2022','-A','x64','-T','v143',"-DJE_ASTC_PACKAGE_ROOT=$extracted");consumerBuild=@(@('--build',$consumerOut,'--config','Release','--parallel','8'),@('--build',$consumerOut,'--config','Debug','--parallel','8'));consumerCTest=@(@('--test-dir',$consumerOut,'-C','Release','--output-on-failure'),@('--test-dir',$consumerOut,'-C','Debug','--output-on-failure'));testCountPerConfig=9;scope='Tools/Editor under PAL_TRAIT_BUILD_HOST_TOOLS; no Runtime executable deployment';guards=@(@{scope='PRODUCER';case='missing-required-argument';expected='is required'},@{scope='PRODUCER';case='archive-missing';expected='source archive file is missing'},@{scope='PRODUCER';case='archive-hash';expected='source archive SHA256 mismatch'},@{scope='PRODUCER';case='pin-url-or-sha';expected='must match the audited 3.2 archive'},@{scope='PRODUCER';case='URI';expected='must not be a URI'},@{scope='PRODUCER';case='UNC-relative';expected='must be an absolute local path'},@{scope='PRODUCER';case='symlink-junction-ancestor';expected='must not use symlink paths'},@{scope='PRODUCER';case='output-existing';expected='output must be new'},@{scope='PRODUCER';case='output-outside-build';expected='must be beneath this checkout/build'},@{scope='PATCH-HELPER';case='pristine-byte-mismatch';expected='pristine patch input SHA256 mismatch';fixture='Only private copy under own fresh attempt; invoke ApplyASTCPatch, not a claimed producer case'});guardFixturePolicy='Luna creates fresh local fixtures; producer negatives keep a missing archive secondary backstop so regression cannot reach build; no global path mutations';limits=@('No native PASS yet','No original legacy binary hash claim','No Editor visual/performance acceptance','No current catalogue/cache/Root mutation')}
$assignment | ConvertTo-Json -Depth 12 | Set-Content "$out/validation-assignment.json" -Encoding utf8NoBOM
$builder=[Text.StringBuilder]::new()
foreach($file in (Get-ChildItem $provider -File -Recurse | Sort-Object FullName)){
 $rel=[IO.Path]::GetRelativePath($root,$file.FullName).Replace('\','/')
 [void]$builder.Append("diff --git a/$rel b/$rel`nnew file mode 100644`n--- /dev/null`n+++ b/$rel`n")
 $lines=[IO.File]::ReadAllLines($file.FullName); [void]$builder.Append("@@ -0,0 +1,$($lines.Count) @@`n")
 foreach($line in $lines){[void]$builder.Append("+$line`n")}
}
[IO.File]::WriteAllText("$out/source-diff.patch",$builder.ToString(),[Text.UTF8Encoding]::new($false))
$entries=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
function Add([string]$path,[string]$owner){$full=[IO.Path]::GetFullPath($path); if(-not $entries.ContainsKey($full)){$item=Get-Item -LiteralPath $full; $entries[$full]=[ordered]@{path=$full;sha256=(Hash $full);bytes=$item.Length;owner=$owner}}}
$manifest=Get-Content "$audit/source-extraction.json" -Raw | ConvertFrom-Json
foreach($file in $manifest.manifest){if((Hash $file.path) -ne $file.sha256){throw "Snapshot delta: $($file.path)"}; Add $file.path 'immutable ARM source / exact primary Gitblob'}
foreach($file in Get-ChildItem $provider -File -Recurse){Add $file.FullName 'ASTC provider candidate'}
foreach($file in Get-ChildItem $audit -File){Add $file.FullName 'source/recipe/patch audit historical receipts'}
foreach($path in @($archive,$cmake,"$root/cmake/Tools/LocalPackages/JECreateLocalPackage.cmake","$root/cmake/Tools/LocalPackages/Python/JERequireLocalPath.cmake","$root/LICENSE.txt","$root/LICENSE_APACHE2.TXT","$root/LICENSE_MIT.TXT","$out/validation-assignment.json","$out/source-diff.patch","$out/RunASTCProducer.ps1","$out/FinalizeCandidate.ps1")){Add $path 'explicit shared/build/provenance input'}
$witnessPath="$root/build/n0-validation/local-packages/dxc-rev1-preparation/parent-native-toolchain-rev1/witness.json"
$witness=Get-Content $witnessPath -Raw | ConvertFrom-Json
foreach($file in $witness.files){if((Hash $file.path) -ne $file.sha256){throw "Existing toolchain witness delta: $($file.path)"};Add $file.path 'unchanged installed VS/v143 SDK toolchain witness'}
Add $witnessPath 'existing parent toolchain witness'
$freeze=@($entries.Values | Sort-Object path)
$freeze | ConvertTo-Json -Depth 6 | Set-Content "$out/source-freeze-astc-candidate.json" -Encoding utf8NoBOM
$errors=$null;$tokens=$null;[Management.Automation.Language.Parser]::ParseFile("$out/RunASTCProducer.ps1",[ref]$tokens,[ref]$errors) | Out-Null
if($errors.Count){throw 'Producer harness PowerShell parse errors'}
[ordered]@{status='STATIC INPUT VERIFICATION ONLY';nativeExecuted=$false;providerFiles=(Get-ChildItem $provider -File -Recurse).Count;sourceFiles=$manifest.files;toolchainFiles=$witness.files.Count;sourceBytes=$manifest.bytes;freezeCount=$freeze.Count;freezeSHA256=(Hash "$out/source-freeze-astc-candidate.json");assignmentSHA256=(Hash "$out/validation-assignment.json");driverSHA256=(Hash "$out/RunASTCProducer.ps1");diffSHA256=(Hash "$out/source-diff.patch");parserErrors=$errors.Count;archiveSHA256=(Hash $archive);guards='planned / not executed';patch='original four hunks / two files exact inverse proof; CMake application pending'} | ConvertTo-Json -Depth 5 | Set-Content "$out/candidate-proof.json" -Encoding utf8NoBOM
Get-Content "$out/candidate-proof.json"
