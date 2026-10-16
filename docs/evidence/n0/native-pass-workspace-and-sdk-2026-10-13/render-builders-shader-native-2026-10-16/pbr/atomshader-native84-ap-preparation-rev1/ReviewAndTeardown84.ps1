$ErrorActionPreference='Stop'
$repo='F:\JericoEngine\JericoEngine'
$pbr=Join-Path $repo 'build/n0-validation/native-render-builders-integration-rev1/material-pbr-preview-integration-rev1'
$out=Join-Path $pbr 'native-atomshader-ap84-attempt1'
$reviewPath=Join-Path $out 'parent-review84.json'
$teardownPath=Join-Path $out 'parent-temporary-source-teardown84.json'
if((Test-Path -LiteralPath $reviewPath) -or (Test-Path -LiteralPath $teardownPath)){throw 'Preserve existing parent receipts.'}
function Save($path,$value){[IO.File]::WriteAllText($path,($value|ConvertTo-Json -Depth 14),[Text.UTF8Encoding]::new($false))}
function Rows($q,$name){@(($q.queries|Where-Object name -ceq $name).rows)}
function Identities($rows){@($rows|ForEach-Object {'{0}|{1}|{2}|{3}|{4}|{5}' -f $_.SourceGuidHex,$_.SourceName,$_.Platform,$_.JobKey,$_.BuilderGuid.hex,$_.Status}|Sort-Object)}
$post=Get-Content -LiteralPath (Join-Path $out 'postflight.json') -Raw|ConvertFrom-Json
if(!$post.allFrozenInputsMatched -or $post.preCount -ne 14037 -or $post.postCount -ne 14037 -or $post.authored.matched -ne 40 -or $post.ownerTextures.matched -ne 26 -or $post.fatal -cne 'focal audit: Successful job with zero registered products is not evidence.'){throw 'Expected preserved incomplete AP84 result differs.'}
$native=@($post.phases|Where-Object phase -ceq 'assetprocessorbatch-shader84')[0]
if($native.exitCode -ne 1 -or $native.ramGuardTriggered -or (Get-FileHash -LiteralPath $native.stdout).Hash -cne $native.stdoutSha256){throw 'Actual native result differs.'}
$before=Get-Content -LiteralPath (Join-Path $out 'before-database/readonly-query.json') -Raw|ConvertFrom-Json
$after=Get-Content -LiteralPath (Join-Path $out 'after-database/readonly-query.json') -Raw|ConvertFrom-Json
foreach($q in @($before,$after)){if($q.failure -or $q.openFlags -ne 65537 -or $q.openResult -ne 0 -or $q.closeResult -ne 0 -or @($q.queries|Where-Object {$_.error -or !$_.statementReadonly}).Count){throw 'Closed snapshot query invalid.'}}
$oldGlobal=Rows $before 'globalNonCompleted';$newGlobal=Rows $after 'globalNonCompleted'
if($oldGlobal.Count -ne 347 -or $newGlobal.Count -ne 347 -or ((Identities $oldGlobal)-join "`n") -cne ((Identities $newGlobal)-join "`n")){throw 'Global347 inventory drift.'}
$beforeJobs=Rows $before 'jobs';$jobs=Rows $after 'jobs';$products=Rows $after 'products'
if($jobs.Count -ne 6 -or $products.Count -ne 13){throw 'Expected actual six jobs/thirteen products.'}
$proofs=@(foreach($j in $jobs){
 if($j.Status -ne 4 -or $j.ErrorCount -ne 0 -or $j.JobRunKey -le 0){throw 'Focal job did not complete without errors.'}
 $old=@($beforeJobs|Where-Object {$_.SourceGuidHex -ceq $j.SourceGuidHex -and $_.Platform -ceq $j.Platform -and $_.JobKey -ceq $j.JobKey -and $_.BuilderGuid.hex -ceq $j.BuilderGuid.hex})
 if($old.Count -gt 1 -or ($old.Count -eq 1 -and $j.JobRunKey -le $old[0].JobRunKey)){throw 'Job is not fresh.'}
 $log=([string]$j.LastLogFile).Replace('@log@',(Join-Path $repo 'build/n0-validation/ap84-logs'))
 $archive=$null;$logHash=$null;$logState='PRESENT'
 if(Test-Path -LiteralPath $log){
  if((Get-Item -LiteralPath $log).LastWriteTimeUtc -lt ([DateTimeOffset]$native.startedUtc).UtcDateTime){throw 'Log predates cook.'}
  $archive=Join-Path $out ('all-six-focal-job-logs/'+[IO.Path]::GetFileName($log));[void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($archive))
  $logHash=(Get-FileHash -LiteralPath $log).Hash
  if(Test-Path -LiteralPath $archive){if((Get-FileHash -LiteralPath $archive).Hash -cne $logHash){throw 'Earlier archive differs; preserve.'}}else{[IO.File]::Copy($log,$archive,$false)}
 }elseif($j.JobKey -ceq 'azshadervariant'){$logState='NOT_MATERIALIZED_COPY_JOB_LOG_POLICY_UNVERIFIED'}else{throw 'Shader builder job log absent.'}
 $jobProducts=@($products|Where-Object JobPK -eq $j.JobID)
 $kind='NONEMPTY_REGISTERED_PRODUCTS'
 if(!$jobProducts.Count){
  $text=[IO.File]::ReadAllText($log)
  if($j.JobKey -ceq 'Shader Variant Asset_varianttree'){$expected='because all valid RHI backends are using specialization constants for shader options.'}
  elseif($j.JobKey -ceq 'Shader Variant Asset_variantbatch'){$expected="because it's using specialization constants for shader options."}
  else{throw 'Unexpected zero-product job.'}
  if(!$text.Contains($expected)){throw 'Expected specialization-constant branch not demonstrated.'}
  $kind='EXPECTED_NO_OUTPUT_BRANCH_ONLY_POSITIVE_GATE_PENDING'
 }
 [pscustomobject]@{source=$j.SourceName;builder=$j.BuilderGuid.hex;key=$j.JobKey;platform=$j.Platform;status=$j.Status;runKey=$j.JobRunKey;oldRunKey=if($old.Count){$old[0].JobRunKey}else{$null};errors=$j.ErrorCount;warnings=$j.WarningCount;products=$jobProducts.Count;observedBranch=$kind;jobLogState=$logState;archivedLog=$archive;logSHA256=$logHash}
})
$oldProducts=Rows $before 'products'
function ProductIds($rows){@($rows|ForEach-Object {'{0}|{1}|{2}|{3}' -f $_.SourceGuidHex,$_.ProductName,$_.SubID,$_.AssetType.hex}|Sort-Object)}
$oldSourceGuids=@((Rows $before 'sources').SourceGuid.hex)
$preservedProducts=@($products|Where-Object {$_.SourceGuidHex -cin $oldSourceGuids})
if($oldProducts.Count -ne 11 -or ((ProductIds $oldProducts)-join "`n") -cne ((ProductIds $preservedProducts)-join "`n")){throw 'Original product identity drift.'}
$sources=Rows $after 'sources'
$precompiledJob=@($jobs|Where-Object JobKey -ceq 'PrecompiledShader Asset Builder')[0]
$rootJob=@($jobs|Where-Object JobKey -ceq 'azshadervariant')[0]
$precompiledProduct=@($products|Where-Object JobPK -eq $precompiledJob.JobID)[0]
$rootProduct=@($products|Where-Object JobPK -eq $rootJob.JobID)[0]
$rootSource=@($sources|Where-Object {$_.SourceGuid.hex -ceq $rootJob.SourceGuidHex})[0]
if($precompiledProduct.SubID -ne 0 -or $precompiledProduct.AssetType.hex -cne '823395A3D57049F499A9D820CD1DEF98' -or $rootProduct.SubID -ne 0 -or $rootProduct.AssetType.hex -cne '51BED81536D8410E90F01FA9FF765FBA'){throw 'Precompiled/root type or SubID differs.'}
$rootDeps=@((Rows $after 'productDependencies')|Where-Object {$_.ProductPK -eq $precompiledProduct.ProductID -and $_.DependencySourceGuid.hex -ceq $rootSource.SourceGuid.hex -and $_.DependencySubID -eq 0 -and $_.Platform -ceq 'pc' -and $_.DependencyFlags -eq 0 -and $_.FromAssetId -eq 1 -and $_.UnresolvedPath -ceq ''})
if($rootDeps.Count -ne 1){throw 'Actual PreLoad root AssetId not demonstrated.'}
$productPins=@(foreach($product in $products){
 $relative=[string]$product.ProductName
 $cachePath=if($relative.StartsWith('common/')){Join-Path $repo ('Projects/JEEditorFixture/Cache/Intermediate Assets/'+$relative.Substring(7))}else{Join-Path $repo ('Projects/JEEditorFixture/Cache/'+$relative)}
 $item=Get-Item -LiteralPath $cachePath;if($item.Length -le 0){throw 'Physical product absent/empty.'}
 $archive=Join-Path $out ('selected-products/'+$relative);[void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($archive));[IO.File]::Copy($cachePath,$archive,$false)
 [pscustomobject]@{database=$product;path=$cachePath;bytes=$item.Length;sha256=(Get-FileHash -LiteralPath $cachePath).Hash;archive=$archive}
})
$review=@{approved=$false;reviewer='/root';state='FOUR_BUILDERS_EXECUTED_POSITIVE_VARIANT_OUTPUT_GATE_INCOMPLETE';rawNativeExit=1;elapsedSeconds=$native.elapsedSeconds;peakRamUsedPercent=$native.peakRamUsedPercent;freshJobs=$proofs;registeredPhysicalProducts=$productPins;originalElevenProductIdentitiesPreserved=$true;global347IdentitiesPreserved=$true;precompiledRootDependency=$rootDeps;preloadEncoding='AssetCommon.cpp CreateFlags(PreLoad) encodes INTEGER0 in dependency flags; actual root GUID/subID/type and FromAssetId verified together.';postflightPreserved=$true;positiveVariantOutputAcceptance=$false;copyJobDetailedLogGate='PENDING_POLICY_REVIEW: LastLogFile recorded but not materialized; preserved console Processing/Processed, database and byte-exact copy are observations, not a generic log waiver.';next='Separate private workload using existing per-shader setting to exercise the positive VariantTree/VariantBatch producers. No engine behavior change or weakened no-product oracle.';limits='No global AP, runtime Ready, visual, GPU, whole Gem or Shipping acceptance. Raw original rejection retained.';GemIncrement=0}
Save $reviewPath $review
$staging=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'parent-staging-receipt84.json') -Raw|ConvertFrom-Json
$expected=[IO.Path]::GetFullPath((Join-Path $repo 'Projects/JEEditorFixture/Assets/Tests/JENativeShader84'))
if([IO.Path]::GetFullPath($staging.directory) -ine $expected -or $staging.copies.Count -ne 3){throw 'Staging ownership differs.'}
$active=@(Get-CimInstance Win32_Process|Where-Object Name -in @('Editor.exe','AssetProcessor.exe','AssetProcessorBatch.exe','AssetBuilder.exe','cmake.exe','MSBuild.exe','cl.exe','link.exe','ctest.exe','AzTestRunner.exe'))
if($active.Count){throw 'No teardown while native owner active.'}
$cursor=Get-Item -LiteralPath $expected
while($cursor -and $cursor.FullName.Length -ge $repo.Length){if($cursor.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Reparse point in owned staging ancestry.'};$cursor=$cursor.Parent}
$contents=@(Get-ChildItem -LiteralPath $expected -Force)
if($contents.Count -ne 3 -or @($contents|Where-Object PSIsContainer).Count){throw 'Unexpected staged contents; preserve.'}
foreach($entry in $staging.copies){if([IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($entry.path)) -ine $expected -or (Get-Item -LiteralPath $entry.path).Length -ne $entry.bytes -or (Get-FileHash -LiteralPath $entry.path).Hash -cne $entry.sha256 -or (Get-FileHash -LiteralPath $entry.provenance).Hash -cne $entry.sha256){throw 'Temporary input changed; preserve.'}}
foreach($entry in $staging.copies){Remove-Item -LiteralPath $entry.path; if(Test-Path -LiteralPath $entry.path){throw 'Temporary file remains.'}}
if(@(Get-ChildItem -LiteralPath $expected -Force).Count){throw 'Directory is not empty.'}
Remove-Item -LiteralPath $expected
Save $teardownPath @{state='EXACT_THREE_TEMPORARY_SOURCES_REMOVED_AFTER_CLOSED_SNAPSHOTS';owner='/root';utc=[DateTime]::UtcNow.ToString('o');stagingReceipt=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'parent-staging-receipt84.json')).Hash;removed=$staging.copies;privateInputsPreserved=$true;productsArchived=$productPins.Count;directoryAbsent=!(Test-Path -LiteralPath $expected);recursiveDelete=$false;originalEngineInputsChanged=$false}
[pscustomobject]@{review=$reviewPath;sha256=(Get-FileHash -LiteralPath $reviewPath).Hash;positiveGate='INCOMPLETE';productsArchived=$productPins.Count;temporarySourcesRemoved=3;teardown=$teardownPath}|ConvertTo-Json
