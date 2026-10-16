function Review-ShaderSources85($beforeQuery, $afterQuery, $phase) {
    if ($phase.ramGuardTriggered -or $phase.exitCode -notin @(0,1)) { throw 'Abnormal or guarded AP exit cannot complete the focal gate.' }
    foreach ($query in @($beforeQuery, $afterQuery)) {
        if ($query.failure -or $query.openFlags -ne 65537 -or $query.openResult -ne 0 -or $query.closeResult -ne 0 -or @($query.queries).Count -ne 8 -or @($query.queries | Where-Object { $_.error -or !$_.statementReadonly -or $_.prepareResult -ne 0 -or $_.stepResult -ne 101 -or $_.finalizeResult -ne 0 }).Count) { throw 'Readonly snapshot query incomplete.' }
    }
    function Rows($query, $name) {
        $found = @($query.queries | Where-Object name -ceq $name)
        if ($found.Count -ne 1) { throw "Expected one snapshot query: $name" }
        return @($found[0].rows)
    }
    function Identity($job) { return "$($job.SourceGuidHex)|$($job.SourceName)|$($job.Platform)|$($job.JobKey)|$($job.BuilderGuid.hex)" }
    $beforeSources = @(Rows $beforeQuery 'sources')
    $afterSources = @(Rows $afterQuery 'sources')
    $beforeJobs = @(Rows $beforeQuery 'jobs')
    $afterJobs = @(Rows $afterQuery 'jobs')
    $products = @(Rows $afterQuery 'products')
    $cacheRoot = Join-Path $repo 'Projects/JEEditorFixture/Cache'
    $proofs = [Collections.Generic.List[object]]::new()
    $specs = @(
        @{path=$sources[0].path;key='Shader Asset';platform='pc';builder='C94DA15182BC447586FAE6C92A0BD6F8';products=@(@{name='jevariantpositive85.azshader';type='823395A3D57049F499A9D820CD1DEF98';subid=0})},
        @{path=$sources[1].path;key='HashedShaderVariantList';platform='common';builder='D7FB0C17131B43E8BCCC408C1763E538';products=@(@{name='jevariantpositive85.hashedvariantlist';type='D86DA375DD7745F981D52E50C24C8469';subid=0;path=$workload.generatedIntermediates[0]},@{name='jevariantpositive85_0.hashedvariantbatch';type='D86DA375DD7745F981D52E50C24C8469';subid=1;path=$workload.generatedIntermediates[1]})},
        @{path=$workload.generatedIntermediates[0];key='Shader Variant Asset_varianttree';platform='pc';builder='C959AEC220834488AD88F61B1144535B';products=@(@{name='jevariantpositive85.azshadervarianttree';type='EBF48506F8BB4B378FACF132BF83E42D';subid=0})},
        @{path=$workload.generatedIntermediates[1];key='Shader Variant Asset_variantbatch';platform='pc';builder='C959AEC220834488AD88F61B1144535B';products=@(@{name='jevariantpositive85_vulkan_1.azshadervariant';type='51BED81536D8410E90F01FA9FF765FBA';subid=2147483649},@{name='jevariantpositive85_vulkan_2.azshadervariant';type='51BED81536D8410E90F01FA9FF765FBA';subid=2147483650},@{name='jevariantpositive85_vulkan_3.azshadervariant';type='51BED81536D8410E90F01FA9FF765FBA';subid=2147483651})}
    )
    foreach ($spec in $specs) {
        $matchesForPath = @($afterSources | Where-Object { [IO.Path]::GetFullPath((Join-Path $_.ScanFolder $_.SourceName)) -ieq [IO.Path]::GetFullPath($spec.path) })
        if ($matchesForPath.Count -ne 1) { throw "Actual source/watchfolder identity missing or ambiguous: $($spec.path)" }
        $source = $matchesForPath[0]
        $oldSource = @($beforeSources | Where-Object { [IO.Path]::GetFullPath((Join-Path $_.ScanFolder $_.SourceName)) -ieq [IO.Path]::GetFullPath($spec.path) })
        if ($oldSource.Count -gt 1 -or ($oldSource.Count -eq 1 -and ($oldSource[0].SourceGuid.hex -cne $source.SourceGuid.hex -or $oldSource[0].PortableKey -cne $source.PortableKey))) { throw 'Existing SourceGuid/watchfolder identity changed.' }
        $job = @($afterJobs | Where-Object { $_.SourceGuidHex -ceq $source.SourceGuid.hex -and $_.JobKey -ceq $spec.key -and $_.Platform -ceq $spec.platform -and (!$spec.builder -or $_.BuilderGuid.hex -ceq $spec.builder) })
        if ($job.Count -ne 1 -or $job[0].Status -ne 4 -or $job[0].ErrorCount -ne 0 -or $job[0].JobRunKey -le 0) { throw "Requested real job did not complete without errors: $($spec.path) / $($spec.key)" }
        $oldJob = @($beforeJobs | Where-Object { (Identity $_) -ceq (Identity $job[0]) })
        if ($oldJob.Count -gt 1 -or ($oldJob.Count -eq 1 -and $job[0].JobRunKey -le $oldJob[0].JobRunKey)) { throw 'Cached or ambiguous job cannot prove a fresh cook.' }
        $log = [IO.Path]::GetFullPath(([string]$job[0].LastLogFile).Replace('@log@',$logDirectory))
        if (!$log.StartsWith(($logDirectory+[IO.Path]::DirectorySeparatorChar),[StringComparison]::OrdinalIgnoreCase) -or !([IO.Path]::GetFileName($log).EndsWith('-'+$job[0].JobRunKey+'.log',[StringComparison]::Ordinal))) { throw 'Fresh RunKey log must belong to unique AP85 log root.' }
        $logFile = Get-Item -LiteralPath $log
        if ($logFile.LastWriteTimeUtc -lt ([DateTimeOffset]$phase.startedUtc).UtcDateTime) { throw 'Job log predates AP85.' }
        $jobProducts = @($products | Where-Object JobPK -eq $job[0].JobID)
        if (!$jobProducts.Count) { throw 'Successful job with zero registered products is not evidence.' }
        $outputProofs = [Collections.Generic.List[object]]::new()
        foreach ($expectedProduct in $spec.products) {
            $name=$expectedProduct.name
            $matched = @($jobProducts | Where-Object { [IO.Path]::GetFileName($_.ProductName) -ieq $name })
            if ($matched.Count -ne 1) { throw "Expected output not registered exactly once: $name" }
            $product = $matched[0]
            $normalizedSubId=([long]$product.SubID -band 0xFFFFFFFFL)
            if($product.AssetType.hex -cne $expectedProduct.type -or $normalizedSubId -ne $expectedProduct.subid){throw ('Output type/UInt32 subID differs: '+$name)}
            $path = if ($expectedProduct.path) { $expectedProduct.path } else { Join-Path $cacheRoot $product.ProductName }
            $path = [IO.Path]::GetFullPath($path)
            if (!$path.StartsWith(($cacheRoot+[IO.Path]::DirectorySeparatorChar),[StringComparison]::OrdinalIgnoreCase)) { throw 'Output escapes cache.' }
            $file = Get-Item -LiteralPath $path
            if ($file.Length -le 0) { throw 'Empty output does not complete the gate.' }
            $outputProofs.Add(@{database=$product;uint32SubId=$normalizedSubId;path=$path;bytes=$file.Length;sha256=(Get-FileHash -LiteralPath $path).Hash})
        }
        if (@($jobProducts | Where-Object ProductName -match '(?i)(?:_null|_dx12|_metal)(?:_|\.)').Count) { throw 'Non-Vulkan output API in focal product family.' }
        $archiveDirectory = Join-Path $evidence 'focal-job-logs'
        [void][IO.Directory]::CreateDirectory($archiveDirectory)
        $archivedLog = Join-Path $archiveDirectory ([IO.Path]::GetFileName($log))
        [IO.File]::Copy($log,$archivedLog,$false)
        $proofs.Add(@{source=$source;sourcePath=$spec.path;job=$job[0];oldJob=$oldJob;allJobProducts=$jobProducts;outputs=$outputProofs.ToArray();log=@{original=$log;archive=$archivedLog;bytes=$logFile.Length;sha256=(Get-FileHash -LiteralPath $archivedLog).Hash}})
    }
    if($proofs.Count -ne 4 -or @($proofs|ForEach-Object {$_.outputs}).Count -ne 7){throw 'Exactly four fresh jobs/seven mandatory semantic products required.'}
    $shaderProof=@($proofs|Where-Object {$_.job.JobKey -ceq 'Shader Asset'})[0]
    $options=@($products|Where-Object {$_.JobPK -eq $shaderProof.job.JobID -and [IO.Path]::GetFileName($_.ProductName) -ieq 'jevariantpositive85_vulkan.options.json'})
    if($options.Count -ne 1 -or $options[0].AssetType.hex -cne '6977AEB117AD4992957B23BB2E85B18B' -or ([long]$options[0].SubID -band 0xFFFFFFFFL) -ne 2147483654){throw 'Fresh Vulkan options artifact type/subID missing.'}
    $optionsPath=[IO.Path]::GetFullPath((Join-Path $cacheRoot $options[0].ProductName))
    if(!$optionsPath.StartsWith(($cacheRoot+[IO.Path]::DirectorySeparatorChar),[StringComparison]::OrdinalIgnoreCase)){throw 'Options path escapes cache.'}
    $optionsFile=Get-Item -LiteralPath $optionsPath
    if($optionsFile.Length -le 0){throw 'Empty options JSON.'}
    $json=Get-Content -LiteralPath $optionsPath -Raw|ConvertFrom-Json
    if($json -isnot [pscustomobject]){throw 'Options JSON must be an object.'}
    $field=@($json.PSObject.Properties|Where-Object Name -ceq 'specializationConstants')
    if($field.Count -gt 1 -or ($field.Count -eq 1 -and ($field[0].Value -isnot [bool] -or $field[0].Value))){throw 'specializationConstants must be boolfalse or omitted native-defaultfalse; true/string/null rejected.'}
    $optionsProof=@{database=$options[0];uint32SubId=([long]$options[0].SubID -band 0xFFFFFFFFL);path=$optionsPath;bytes=$optionsFile.Length;sha256=(Get-FileHash -LiteralPath $optionsPath).Hash;memberPresent=($field.Count -eq 1);representation=if($field.Count){'EXPLICIT_BOOLEAN_FALSE'}else{'OMITTED_NATIVE_DEFAULT_FALSE'};semanticSpecializationConstants=$false;contract=$workload.enumProof;nativeRead='AzslCompiler.cpp938-941; defaultfalse if member absent. No inference from skip log text.'}
    $dependencies = @(Rows $afterQuery 'productDependencies')
    $oldGlobal = @(Rows $beforeQuery 'globalNonCompleted')
    $newGlobal = @(Rows $afterQuery 'globalNonCompleted')
    $oldIdentities = @($oldGlobal | ForEach-Object { (Identity $_)+'|'+$_.Status } | Sort-Object)
    $newIdentities = @($newGlobal | ForEach-Object { (Identity $_)+'|'+$_.Status } | Sort-Object)
    if (($oldIdentities -join "`n") -cne ($newIdentities -join "`n")) { throw 'Global non-completed identities/status changed; no347 waiver.' }
    foreach ($globalRows in @(@{rows=$oldGlobal},@{rows=$newGlobal})) {
        if (@($globalRows.rows | Where-Object Status -in @(2,3) | Select-Object -ExpandProperty SourceGuidHex -Unique).Count -ne 347) { throw 'Historical failed-source inventory differs from347.' }
    }
    return @{state='FOUR_SHADER_JOBS_THREE_BUILDER_UUIDS_FRESH_POSITIVE_EVIDENCE_PARENT_REVIEW_REQUIRED';nativeExit=$phase.exitCode;globalPass=$false;proofs=$proofs.ToArray();sourceRows=$afterSources;productDependencies=$dependencies;sourceDependencies=@(Rows $afterQuery 'sourceDependencies');beforeGlobalNonCompleted=$oldGlobal;afterGlobalNonCompleted=$newGlobal;options=$optionsProof;limits='Fresh registered completed jobs, raw current logs and nonempty products only; Seven mandatory semantic products require exact types/UInt32 SubIDs and nonempty files; generated intermediate SourceGuids are distinct actual DB identities. Options metadata false/defaultfalse is captured. No AssetHandler Ready/runtime/GPU/UI/Gem acceptance. Precompiled fourth builder is reused from84, not rerun85.'}
}
