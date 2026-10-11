# Static finalizer only: preserves producer/source freeze and measures new audited data.
$ErrorActionPreference='Stop'
$root='F:/JericoEngine/JericoEngine';$base="$root/build/n0-validation/local-packages";$out="$base/astc-rev1-consumer-preparation"
function Hash([string]$path){$stream=[IO.File]::OpenRead($path);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}}
$oldPath="$base/astc-rev1-preparation/source-freeze-astc-candidate.json"
$old=Get-Content $oldPath -Raw | ConvertFrom-Json
$entries=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
function Add([string]$path,[string]$owner){$full=[IO.Path]::GetFullPath($path);if(-not $entries.ContainsKey($full)){$item=Get-Item -LiteralPath $full;$entries[$full]=[ordered]@{path=$full;bytes=$item.Length;sha256=(Hash $full);owner=$owner}}}
foreach($file in $old){if((Hash $file.path) -ne $file.sha256 -or (Get-Item -LiteralPath $file.path).Length -ne $file.bytes){throw "Original556 delta: $($file.path)"};Add $file.path $file.owner}
$producer=Get-Content "$base/astc-rev1-preparation/validation-assignment.json" -Raw | ConvertFrom-Json
$build="$base/astc-native-consumer-rev1"
$expectedTests=@('ldr8','ldr16','ldr32','srgb','hdr-rgb','hdr','normal','invalid','cli') | ForEach-Object {"JE.LocalPackages.ASTC.$_"}
$assignment=[ordered]@{status='PENDING PARENT REVIEW / LUNA EXECUTION';nativeExecuted=$false;freshAttempt="$base/astc-rev1-consumer-validation/attempt1";freshBuild=$build;cmake='C:/Program Files/CMake/bin/cmake.exe';ctest='C:/Program Files/CMake/bin/ctest.exe';configure=@($producer.consumerConfigure);expectedTests=@($expectedTests);configurations=@('Release','Debug');testsPerConfiguration=9;package=@{archive=$producer.package.archive;sha256='4eed3f722e8f4cca60bf14684f22fa351d0d59960ed518be04cce30c707949ca';bytes=1179220;extractedRoot=$producer.package.extractedRoot;members=63;payloadBytes=8503430};nativeScope='One configure; one build and CTest for each config. No producer, no extraction, no negatives dispatched by this driver.';guardsProposed=$producer.guards;guardPolicy='Not executed/fixtures not created. Producer negatives always retain missing archive secondary backstop except actual tiny-file hash case; no variant may reach a successful producer. PATCH-HELPER case uses only private files under own fresh attempt and before/after hash witness, not a claimed producer case.';acceptance='Parent acceptance only; no Editor/engine PASS from this consumer'}
$assignment | ConvertTo-Json -Depth 10 | Set-Content "$out/consumer-assignment.json" -Encoding utf8NoBOM
foreach($file in (Get-Content "$out/extraction-manifest.json" -Raw | ConvertFrom-Json)){if((Hash $file.path) -ne $file.sha256 -or (Get-Item -LiteralPath $file.path).Length -ne $file.bytes){throw 'Extracted payload delta'};Add $file.path 'actual archive extraction / read-only consumer input'}
foreach($file in Get-ChildItem "$base/astc-local-rev1/packages" -File -Recurse){Add $file.FullName 'produced package/archive/sidecar untouched'}
foreach($file in Get-ChildItem "$base/astc-local-rev1" -File){Add $file.FullName 'producer native argv/results/raw logs'}
foreach($file in @('Source/CMakeLists.txt','Source/cmake_core.cmake')){Add "$base/astc-local-rev1/source-work/astc-encoder-f32d8d4dd9e06f8986cd5e10f12e199fc023f150/$file" 'private original patch outputs / expected post-hashes'}
Add $oldPath 'original 556 source freeze preserved'
Add "$base/astc-rev1-preparation/parent-source-driver-review-rev1.json" 'parent source/producer review'
Add 'C:/Program Files/CMake/bin/ctest.exe' 'existing CTest executable'
Add "$root/build/n0-validation/original-engine-deps/Python-rev3/packages/python-3.10.13-je-rev3-windows/python/python.exe" 'external audited Python only static tar/lzma inspection'
foreach($file in Get-ChildItem $out -File){if($file.Name -notin @('source-freeze-astc-consumer.json','consumer-readiness.json')){Add $file.FullName 'new archive audit/consumer harness evidence'}}
$freeze=@($entries.Values | Sort-Object path)
$freeze | ConvertTo-Json -Depth 6 | Set-Content "$out/source-freeze-astc-consumer.json" -Encoding utf8NoBOM
$errors=$null;$tokens=$null;[Management.Automation.Language.Parser]::ParseFile("$out/RunASTCConsumer.ps1",[ref]$tokens,[ref]$errors) | Out-Null
if($errors.Count){throw 'Consumer driver AST parse failure'}
[ordered]@{status='STATIC DATA AUDIT / READY FOR PARENT REVIEW';nativeExecuted=$false;originalSourceInputs=556;originalInputDeltas=0;providerFiles=9;packageMembers=63;noticeWitnesses=56;newFreezeCount=$freeze.Count;freezeSHA256=(Hash "$out/source-freeze-astc-consumer.json");assignmentSHA256=(Hash "$out/consumer-assignment.json");driverSHA256=(Hash "$out/RunASTCConsumer.ps1");archiveAuditSHA256=(Hash "$out/archive-audit.json");extractionManifestSHA256=(Hash "$out/extraction-manifest.json");parserErrors=$errors.Count;negativeStatus='PROPOSED / NOT DISPATCHED';nativeTestExpectation='9Release+9Debug, no skip'} | ConvertTo-Json -Depth 5 | Set-Content "$out/consumer-readiness.json" -Encoding utf8NoBOM
Get-Content "$out/consumer-readiness.json"
