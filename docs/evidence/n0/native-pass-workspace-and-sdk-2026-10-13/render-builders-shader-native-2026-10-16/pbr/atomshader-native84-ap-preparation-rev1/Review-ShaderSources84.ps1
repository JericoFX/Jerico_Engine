function Review-ShaderSources84($beforeQuery, $afterQuery, $phase) {
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
        @{path=$sources[0].path;key='Shader Asset';platform='pc';builder='C94DA15182BC447586FAE6C92A0BD6F8';names=@('simpletextured.azshader')},
        @{path=$sources[1].path;key='HashedShaderVariantList';platform='common';builder='D7FB0C17131B43E8BCCC408C1763E538';names=@('simpletextured.hashedvariantlist','simpletextured_0.hashedvariantbatch')},
        @{path=$sources[2].path;key='Shader Variant Asset_varianttree';platform='pc';builder='C959AEC220834488AD88F61B1144535B';names=@('simpletextured.azshadervarianttree')},
        @{path=$sources[3].path;key='Shader Variant Asset_variantbatch';platform='pc';builder='C959AEC220834488AD88F61B1144535B';names=@('simpletextured_vulkan_1.azshadervariant','simpletextured_vulkan_2.azshadervariant','simpletextured_vulkan_3.azshadervariant')},
        @{path=(Join-Path $workload.stageDirectory 'JEPrecompiledVulkan81.precompiledshader');key='PrecompiledShader Asset Builder';platform='pc';builder='50D3185B489C4C8E84DCF99A75FDB72F';names=@('jeprecompiledvulkan81.azshader')},
        @{path=(Join-Path $workload.stageDirectory 'je_precompiled_vulkan_0.azshadervariant');key='azshadervariant';platform='pc';builder=$null;names=@('je_precompiled_vulkan_0.azshadervariant')}
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
        if (!$log.StartsWith(($logDirectory+[IO.Path]::DirectorySeparatorChar),[StringComparison]::OrdinalIgnoreCase) -or !([IO.Path]::GetFileName($log).EndsWith('-'+$job[0].JobRunKey+'.log',[StringComparison]::Ordinal))) { throw 'Fresh RunKey log must belong to unique AP84 log root.' }
        $logFile = Get-Item -LiteralPath $log
        if ($logFile.LastWriteTimeUtc -lt ([DateTimeOffset]$phase.startedUtc).UtcDateTime) { throw 'Job log predates AP84.' }
        $jobProducts = @($products | Where-Object JobPK -eq $job[0].JobID)
        if (!$jobProducts.Count) { throw 'Successful job with zero registered products is not evidence.' }
        $outputProofs = [Collections.Generic.List[object]]::new()
        foreach ($name in $spec.names) {
            $matched = @($jobProducts | Where-Object { [IO.Path]::GetFileName($_.ProductName) -ieq $name })
            if ($matched.Count -ne 1) { throw "Expected output not registered exactly once: $name" }
            $product = $matched[0]
            $path = if ($name.EndsWith('.hashedvariantlist') -or $name.EndsWith('.hashedvariantbatch')) { Join-Path $cacheRoot ('Intermediate Assets/shaders/'+$name) } else { Join-Path $cacheRoot $product.ProductName }
            $path = [IO.Path]::GetFullPath($path)
            if (!$path.StartsWith(($cacheRoot+[IO.Path]::DirectorySeparatorChar),[StringComparison]::OrdinalIgnoreCase)) { throw 'Output escapes cache.' }
            $file = Get-Item -LiteralPath $path
            if ($file.Length -le 0) { throw 'Empty output does not complete the gate.' }
            $outputProofs.Add(@{database=$product;path=$path;bytes=$file.Length;sha256=(Get-FileHash -LiteralPath $path).Hash})
        }
        if (@($jobProducts | Where-Object ProductName -match '(?i)(?:_null|_dx12|_metal)(?:_|\.)').Count) { throw 'Non-Vulkan output API in focal product family.' }
        $archiveDirectory = Join-Path $evidence 'focal-job-logs'
        [void][IO.Directory]::CreateDirectory($archiveDirectory)
        $archivedLog = Join-Path $archiveDirectory ([IO.Path]::GetFileName($log))
        [IO.File]::Copy($log,$archivedLog,$false)
        $proofs.Add(@{source=$source;sourcePath=$spec.path;job=$job[0];oldJob=$oldJob;outputs=$outputProofs.ToArray();log=@{original=$log;archive=$archivedLog;bytes=$logFile.Length;sha256=(Get-FileHash -LiteralPath $archivedLog).Hash}})
    }
    $precompiled = @($proofs | Where-Object { $_.job.JobKey -ceq 'PrecompiledShader Asset Builder' })[0]
    $rootCopy = @($proofs | Where-Object { $_.job.JobKey -ceq 'azshadervariant' })[0]
    $precompiledProduct = $precompiled.outputs[0].database
    $rootProduct = $rootCopy.outputs[0].database
    if ($precompiledProduct.SubID -ne 0 -or $precompiledProduct.AssetType.hex -cne '823395A3D57049F499A9D820CD1DEF98' -or $rootProduct.SubID -ne 0 -or $rootProduct.AssetType.hex -cne '51BED81536D8410E90F01FA9FF765FBA') { throw 'Precompiled/root-copy AssetId/type contract differs.' }
    $dependencies = @(Rows $afterQuery 'productDependencies')
    $rootDependency = @($dependencies | Where-Object { $_.ProductPK -eq $precompiledProduct.ProductID -and $_.DependencySourceGuid.hex -ceq $rootCopy.source.SourceGuid.hex -and $_.DependencySubID -eq 0 -and $_.Platform -ceq 'pc' })
    if ($rootDependency.Count -ne 1) { throw 'Precompiled product does not reference actual copied-root SourceGuid/subID0.' }
    $oldGlobal = @(Rows $beforeQuery 'globalNonCompleted')
    $newGlobal = @(Rows $afterQuery 'globalNonCompleted')
    $oldIdentities = @($oldGlobal | ForEach-Object { (Identity $_)+'|'+$_.Status } | Sort-Object)
    $newIdentities = @($newGlobal | ForEach-Object { (Identity $_)+'|'+$_.Status } | Sort-Object)
    if (($oldIdentities -join "`n") -cne ($newIdentities -join "`n")) { throw 'Global non-completed identities/status changed; no347 waiver.' }
    foreach ($globalRows in @(@{rows=$oldGlobal},@{rows=$newGlobal})) {
        if (@($globalRows.rows | Where-Object Status -in @(2,3) | Select-Object -ExpandProperty SourceGuidHex -Unique).Count -ne 347) { throw 'Historical failed-source inventory differs from347.' }
    }
    return @{state='FOUR_BUILDERS_FRESH_EVIDENCE_CAPTURED_PARENT_REVIEW_REQUIRED';nativeExit=$phase.exitCode;globalPass=$false;proofs=$proofs.ToArray();sourceRows=$afterSources;productDependencies=$dependencies;sourceDependencies=@(Rows $afterQuery 'sourceDependencies');beforeGlobalNonCompleted=$oldGlobal;afterGlobalNonCompleted=$newGlobal;rootDependency=$rootDependency;limits='Fresh registered completed jobs, raw current logs and nonempty products only; PreLoad flag, full runtime deserialization/Ready and Vulkan API payload review remain parent gates. No GPU/UI/Gem acceptance. Raw shader payload may have no Source row; required six actual job sources must resolve uniquely.'}
}
