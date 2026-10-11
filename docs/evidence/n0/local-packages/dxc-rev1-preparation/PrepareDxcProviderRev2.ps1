$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = $PSScriptRoot
$engine = 'F:\JericoEngine\JericoEngine'
$provider = Join-Path $engine 'cmake/Tools/LocalPackages/DirectXShaderCompilerDxc'
$previous = Join-Path $root 'provider-candidate-rev1'
$out = Join-Path $root 'provider-candidate-rev2'
if (Test-Path -LiteralPath $out) { throw 'Rev2 evidence exists; preserve it' }
$oldPath = Join-Path $previous 'source-freeze-dxc-provider-rev1.json'
if ((Get-FileHash -LiteralPath $oldPath).Hash -ne 'C1062E584831C449B36F2D37FFD870F7489DF5AA18AD5AD069C9B14F7E2DD529') { throw 'Rev1 freeze changed' }
$old = Get-Content -LiteralPath $oldPath -Raw | ConvertFrom-Json
if ($old.files.Count -ne 23322) { throw 'Rev1 count changed' }
foreach ($file in $old.files) {
    if ((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $file.path).Hash.ToLowerInvariant() -ne $file.sha256) { throw "Unapproved preflight delta: $($file.path)" }
}
New-Item -ItemType Directory -Path (Join-Path $out 'before') | Out-Null
$sources = @(Get-ChildItem -LiteralPath $provider -Recurse -File | Sort-Object FullName)
if ($sources.Count -ne 12) { throw 'Provider membership changed' }
foreach ($file in $sources) {
    $relative = $file.FullName.Substring($provider.Length + 1)
    $target = Join-Path (Join-Path $out 'before') $relative
    New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force | Out-Null
    [IO.File]::WriteAllBytes($target, [IO.File]::ReadAllBytes($file.FullName))
}
$changes = @(
    @{ file = 'BuildDirectXShaderCompilerDxcPackage.cmake'; before = 'CL CL_SHA256 OUTPUT)'; after = 'CL CL_SHA256 PARALLEL_JOBS OUTPUT)' },
    @{ file = 'BuildDirectXShaderCompilerDxcPackage.cmake'; before = "endforeach()`nforeach(JE_PATH IN ITEMS JE_DXC_ARCHIVES_ROOT"; after = "endforeach()`nif(NOT JE_DXC_PARALLEL_JOBS MATCHES `"^[1-9][0-9]*$`")`n    message(FATAL_ERROR `"JE DXC: JE_DXC_PARALLEL_JOBS must be a positive integer`")`nendif()`nforeach(JE_PATH IN ITEMS JE_DXC_ARCHIVES_ROOT" },
    @{ file = 'BuildDirectXShaderCompilerDxcPackage.cmake'; before = '--target all llvm-as llvm-dis --parallel)'; after = '--target all llvm-as llvm-dis --parallel "${JE_DXC_PARALLEL_JOBS}")' },
    @{ file = 'BuildDirectXShaderCompilerDxcPackage.cmake'; before = 'string(JSON JE_RECIPE SET "${JE_RECIPE}" toolset "\"v143\"")'; after = "string(JSON JE_RECIPE SET `"`${JE_RECIPE}`" toolset `"\`"v143\`"`")`nstring(JSON JE_RECIPE SET `"`${JE_RECIPE}`" parallelJobs `"`${JE_DXC_PARALLEL_JOBS}`")" },
    @{ file = 'README.md'; before = "VS development environment is required for the original Ninja build.`n"; after = 'VS development environment is required for the original Ninja build.

`JE_DXC_PARALLEL_JOBS` is required and must be a positive integer before
any output is created. Native build argv uses explicit `--parallel <jobs>`;
the initial reviewed assignment uses 6 jobs to contain concurrent memory
usage while preserving every original target. The actual value is recorded
in JERecipe.json and the native argv receipt.
' },
    @{ file = 'README.md'; before = "  '-DJE_DXC_OUTPUT="; after = "  '-DJE_DXC_PARALLEL_JOBS=6' `` `n  '-DJE_DXC_OUTPUT=" }
)
foreach ($change in $changes) {
    $path = Join-Path $provider $change.file
    $text = [IO.File]::ReadAllText($path)
    if ([regex]::Matches($text, [regex]::Escape($change.before)).Count -ne 1) { throw "Nonunique exact context: $($change.file)" }
    [IO.File]::WriteAllText($path, $text.Replace($change.before, $change.after), [Text.UTF8Encoding]::new($false))
}
$deltas = @()
foreach ($file in $sources) {
    $path = $file.FullName
    $relative = $path.Substring($provider.Length + 1)
    $beforePath = Join-Path (Join-Path $out 'before') $relative
    $inverse = [IO.File]::ReadAllText($path)
    for ($index = $changes.Count - 1; $index -ge 0; --$index) {
        $change = $changes[$index]
        if ($change.file -eq $relative) {
            if ([regex]::Matches($inverse, [regex]::Escape($change.after)).Count -ne 1) { throw "Nonunique inverse context: $relative" }
            $inverse = $inverse.Replace($change.after, $change.before)
        }
    }
    $expectedBytes = [IO.File]::ReadAllBytes($beforePath)
    $inverseBytes = [Text.UTF8Encoding]::new($false).GetBytes($inverse)
    if ([Convert]::ToBase64String($expectedBytes) -ne [Convert]::ToBase64String($inverseBytes)) { throw "Inverse byte delta: $relative" }
    $beforeSHA = (Get-FileHash -LiteralPath $beforePath).Hash.ToLowerInvariant()
    $afterSHA = (Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant()
    if ($beforeSHA -ne $afterSHA) { $deltas += [ordered]@{ path = $path; beforeBytes = $expectedBytes.Length; beforeSHA256 = $beforeSHA; afterBytes = (Get-Item -LiteralPath $path).Length; afterSHA256 = $afterSHA; inverseByteExact = $true } }
}
if ($deltas.Count -ne 2) { throw 'Expected exactly producer and README source deltas' }
$patch = [Text.StringBuilder]::new()
foreach ($delta in $deltas) {
    $relative = $delta.path.Substring($engine.Length + 1).Replace('\','/')
    $localRelative = $delta.path.Substring($provider.Length + 1)
    $beforeLines = @([IO.File]::ReadAllLines((Join-Path (Join-Path $out 'before') $localRelative)))
    $afterLines = @([IO.File]::ReadAllLines($delta.path))
    [void] $patch.Append("diff --git a/$relative b/$relative`n--- a/$relative`n+++ b/$relative`n@@ -1,$($beforeLines.Count) +1,$($afterLines.Count) @@`n")
    foreach ($line in $beforeLines) { [void] $patch.Append("-$line`n") }
    foreach ($line in $afterLines) { [void] $patch.Append("+$line`n") }
}
[IO.File]::WriteAllText((Join-Path $out 'source-diff-rev1-to-rev2.patch'), $patch.ToString(), [Text.UTF8Encoding]::new($false))
$assignment = Get-Content -LiteralPath (Join-Path $previous 'validation-assignment.json') -Raw | ConvertFrom-Json -AsHashtable
$producer = @($assignment.producer.argv)
$insert = [Array]::IndexOf($producer, '-P')
if ($insert -lt 1) { throw 'Original producer argv malformed' }
$assignment.producer.argv = @($producer[0..($insert-1)]) + @('-DJE_DXC_PARALLEL_JOBS=6') + @($producer[$insert..($producer.Length-1)])
$assignment.producer.parallelJobs = 6
$assignment.consumerBuild.argv = @($assignment.consumerBuild.argv) + @('6')
$assignment.package.sidecar = $assignment.package.archive + '.SHA256SUMS'
$assignment.guards = @($assignment.guards) + @('missing parallel jobs','zero/negative/noninteger parallel jobs')
$assignment.revision = 2
$assignment.parallelSource = 'https://cmake.org/cmake/help/v4.3/manual/cmake.1.html#build-a-project'
$assignment | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath (Join-Path $out 'validation-assignment.json') -Encoding utf8
$receipt = [ordered]@{ status = 'EXACT SOURCE REVISION ONLY; NATIVE NOT RUN'; revision = 2; oldInputCount = 23322; providerFiles = 12; changedProviderFiles = 2; unchangedProviderFiles = 10; sourceDeltas = $deltas; sourceReplacements = $changes.Count; inverseByteExact = $true; parallelJobs = 6; guardPlacement = 'Required argument and canonical positive integer check before path reads/output creation/native commands'; targetsUnchanged = 'all llvm-as llvm-dis'; sidecarCorrection = '.tar.xz.SHA256SUMS from actual shared helper; rev1 assignment preserved'; sourceGeneration = 'Original codegen remains enabled; HLSL_COPY_GENERATED_SOURCES default OFF untouched'; primarySource = 'https://cmake.org/cmake/help/v4.3/manual/cmake.1.html#build-a-project'; nativeExecuted = $false; acceptance = $false }
$receipt | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath (Join-Path $out 'source-change-receipt.json') -Encoding utf8
$inputs = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($file in $old.files) {
    $path = [IO.Path]::GetFullPath($file.path)
    $sha = (Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant()
    $bytes = (Get-Item -LiteralPath $path).Length
    $delta = @($deltas | Where-Object { $_.path -eq $path })
    if ($delta.Count -eq 1) {
        if ($delta[0].beforeSHA256 -ne $file.sha256 -or $sha -ne $delta[0].afterSHA256) { throw 'Authorized source transition differs' }
        $inputs.Add($path,[ordered]@{ path = $path; bytes = $bytes; sha256 = $sha; role = 'approved parallel-job revision to existing DXC provider candidate' })
    } else {
        if ($sha -ne $file.sha256 -or $bytes -ne $file.bytes) { throw "Unexpected old-input delta: $path" }
        $inputs.Add($path,$file)
    }
}
foreach ($path in @($oldPath, (Join-Path $previous 'source-freeze-dxc-provider-rev1-bare.json'), (Join-Path $previous 'candidate-proof.json'), $PSCommandPath) + @(Get-ChildItem -LiteralPath $out -Recurse -File | ForEach-Object FullName)) {
    $absolute = [IO.Path]::GetFullPath($path)
    if (-not $inputs.ContainsKey($absolute)) { $inputs.Add($absolute,[ordered]@{ path = $absolute; bytes = (Get-Item -LiteralPath $absolute).Length; sha256 = (Get-FileHash -LiteralPath $absolute).Hash.ToLowerInvariant(); role = 'preserved rev1 / own exact rev2 provenance evidence' }) }
}
$files = @($inputs.Values | Sort-Object path)
foreach ($file in $files) { if ((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $file.path).Hash.ToLowerInvariant() -ne $file.sha256) { throw "Final rehash delta: $($file.path)" } }
$freeze = Join-Path $out 'source-freeze-dxc-provider-rev2.json'
$bare = Join-Path $out 'source-freeze-dxc-provider-rev2-bare.json'
[ordered]@{ status = 'CANDIDATE FOR PARENT REVIEW; NATIVE PENDING'; revision = 2; fileCount = $files.Count; historicalInputCount = 23322; authorizedHistoricalDeltas = 2; historicalUnchanged = 23320; providerFileCount = 12; sourceDeltas = $deltas; parallelJobs = 6; nativeExecuted = $false; files = $files } | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $freeze -Encoding utf8
ConvertTo-Json -InputObject $files -Depth 12 | Set-Content -LiteralPath $bare -Encoding utf8
$proof = [ordered]@{ status = 'STATIC REHASH AND EXACT INVERSE ONLY'; verified = $files.Count; historicalInputs = 23322; authorizedDeltas = 2; unknownDeltas = 0; providerSources = 12; unchangedProviderSources = 10; sourceDiffSHA256 = (Get-FileHash -LiteralPath (Join-Path $out 'source-diff-rev1-to-rev2.patch')).Hash; assignmentSHA256 = (Get-FileHash -LiteralPath (Join-Path $out 'validation-assignment.json')).Hash; receiptSHA256 = (Get-FileHash -LiteralPath (Join-Path $out 'source-change-receipt.json')).Hash; freezeSHA256 = (Get-FileHash -LiteralPath $freeze).Hash; bareFreezeSHA256 = (Get-FileHash -LiteralPath $bare).Hash; parallelJobs = 6; nativeExecuted = $false; acceptance = $false }
$proof | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $out 'candidate-proof.json') -Encoding utf8
$proof | ConvertTo-Json -Depth 12
