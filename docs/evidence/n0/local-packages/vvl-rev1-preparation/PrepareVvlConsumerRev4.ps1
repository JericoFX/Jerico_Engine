$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = $PSScriptRoot
$output = Join-Path $root 'consumer-rev4'
$oldFreeze = Join-Path $root 'consumer-rev3/source-freeze-vvl-provider-consumer-rev3.json'
if (Test-Path -LiteralPath $output) { throw 'New evidence exists' }
if ((Get-FileHash -LiteralPath $oldFreeze).Hash -ne 'CE6EC1352E4F911A62D04FB3637218FECC1F5191FD34C915C3A9328ADAAF492B') { throw 'Rev3 freeze changed' }
$freeze = Get-Content -LiteralPath $oldFreeze -Raw | ConvertFrom-Json
foreach ($entry in $freeze.inputs) {
    if ((Get-Item -LiteralPath $entry.path).Length -ne $entry.bytes -or (Get-FileHash -LiteralPath $entry.path).Hash.ToLowerInvariant() -ne $entry.sha256) { throw "Unexpected input delta: $($entry.path)" }
}
$provider = 'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/VulkanValidationLayers'
$encoding = [Text.UTF8Encoding]::new($false, $true)
$sources = @('tests/CMakeLists.txt', 'tests/VulkanValidationLayersConsumer.cpp', 'README.md')
$records = @(); $diff = [Collections.Generic.List[string]]::new()
[IO.Directory]::CreateDirectory($output) | Out-Null
foreach ($relative in $sources) {
    $path = Join-Path $provider $relative
    [byte[]]$raw = [IO.File]::ReadAllBytes($path)
    $before = $encoding.GetString($raw)
    $beforeHash = (Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant()
    $changes = @()
    if ($relative -eq 'tests/VulkanValidationLayersConsumer.cpp') {
        if ([regex]::Matches($before, "`r`n").Count -ne 3 -or [regex]::Matches($before, "`r").Count -ne 3) { throw 'CPP must contain exactly three added CRs' }
        $changes = @(
            @{ before = '        // A raw layer forwards unknown queries through an initialized dispatch chain.' + "`r`n"; after = '        // A raw layer forwards unknown queries through an initialized dispatch chain.' + "`n" },
            @{ before = '        Require(create && enumerate, "real loader global entry points");' + "`r`n"; after = '        Require(create && enumerate, "real loader global entry points");' + "`n" },
            @{ before = '        Require(getInstance(instance, "JE_missing_function") == nullptr, "loader unknown instance entry point rejection");' + "`r`n"; after = '        Require(getInstance(instance, "JE_missing_function") == nullptr, "loader unknown instance entry point rejection");' + "`n" }
        )
    } elseif ($relative -eq 'tests/CMakeLists.txt') {
        if ($before.Contains("`r")) { throw 'Unexpected original CMake EOL' }
        $changes = @(
            @{ before = "enable_testing()`n"; after = @'
enable_testing()
set(JE_EMPTY_IMPLICIT_LAYER_PATH "${CMAKE_CURRENT_BINARY_DIR}/je-vvl-empty-implicit-layers")
JE_python_require_local_path(JE_EMPTY_IMPLICIT_LAYER_PATH)
file(MAKE_DIRECTORY "${JE_EMPTY_IMPLICIT_LAYER_PATH}")
file(GLOB JE_IMPLICIT_LAYER_CONTENTS LIST_DIRECTORIES true "${JE_EMPTY_IMPLICIT_LAYER_PATH}/*")
if(JE_IMPLICIT_LAYER_CONTENTS)
    message(FATAL_ERROR "JE VVL consumer implicit layer directory must be empty")
endif()

'@ },
            @{ before = '        set_tests_properties(JE.LocalPackages.VVL.${JE_CONFIG}.${JE_CASE} PROPERTIES TIMEOUT 60)'; after = @'
        set_tests_properties(JE.LocalPackages.VVL.${JE_CONFIG}.${JE_CASE} PROPERTIES TIMEOUT 60)
        if(JE_CASE STREQUAL "runtime")
            set_tests_properties(JE.LocalPackages.VVL.${JE_CONFIG}.${JE_CASE} PROPERTIES
                ENVIRONMENT "VK_IMPLICIT_LAYER_PATH=${JE_EMPTY_IMPLICIT_LAYER_PATH}")
        endif()
'@ }
        )
        # Here-strings contain LF; preserve the original final separator after the replaced enable_testing line.
        $changes[0].after = $changes[0].after.TrimEnd("`n") + "`n"
    } else {
        if ($before.Contains("`r")) { throw 'Unexpected original README EOL' }
        $changes = @(@{ before = @'
The exports case checks all five actual exports, the exact layer/API version,
enumeration truncation, rejection of an unknown layer/function, and the
original version1/version2 loader negotiation behavior. The runtime case
loads the archived pair through the existing Vulkan loader, requires the
exact `VUID-VkInstanceCreateInfo-flags-parameter` plus rejection for malformed
flags, then creates/destroys a valid instance. Failure to load the actual
'@; after = @'
The exports case checks all five actual exports, the exact layer/API version,
enumeration truncation, rejection of an unknown layer, the valid global
`vkCreateInstance` query, and original version1/version2 loader negotiation.
The runtime case loads the archived pair through the existing Vulkan loader,
checks unknown-function rejection for both a global NULL query and a valid
instance, requires the exact `VUID-VkInstanceCreateInfo-flags-parameter` plus
rejection for malformed flags, then creates/destroys a valid instance.
CTest gives only the runtime cases an empty private `VK_IMPLICIT_LAYER_PATH`
under the consumer build directory, isolating unrelated inherited implicit
manifests while preserving the explicit package layer and all error checks.
Failure to load the actual
'@ })
    }
    $after = $before
    $diff.Add('--- a/cmake/Tools/LocalPackages/VulkanValidationLayers/' + $relative)
    $diff.Add('+++ b/cmake/Tools/LocalPackages/VulkanValidationLayers/' + $relative)
    $delta = 0
    foreach ($change in $changes) {
        if ([regex]::Matches($before, [regex]::Escape($change.before)).Count -ne 1) { throw 'Replacement must be exact and unique' }
        $line = $before.Substring(0, $before.IndexOf($change.before)).Split("`n").Count
        $oldLines = $change.before.TrimEnd("`n").Split("`n")
        $newLines = $change.after.TrimEnd("`n").Split("`n")
        $diff.Add(('@@ -{0},{1} +{2},{3} @@' -f $line, $oldLines.Count, ($line + $delta), $newLines.Count))
        foreach ($s in $oldLines) { $diff.Add('-' + $s) }
        foreach ($s in $newLines) { $diff.Add('+' + $s) }
        $delta += $newLines.Count - $oldLines.Count
        $after = $after.Replace($change.before, $change.after)
    }
    $inverse = $after
    foreach ($change in $changes) { $inverse = $inverse.Replace($change.after, $change.before) }
    if ($inverse -cne $before -or -not [Linq.Enumerable]::SequenceEqual[byte]($encoding.GetBytes($inverse), $raw)) { throw 'Source inverse must be byteexact' }
    [IO.File]::WriteAllBytes((Join-Path $output ($relative.Replace('/', '_') + '.before')), $raw)
    [IO.File]::WriteAllText($path, $after, $encoding)
    $record = [ordered]@{ path = $path; beforeSHA256 = $beforeHash; beforeBytes = $raw.Length; afterSHA256 = (Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant(); afterBytes = (Get-Item -LiteralPath $path).Length; inverseByteExact = $true }
    $records += $record
    foreach ($entry in @($freeze.files) + @($freeze.inputs)) {
        if ([IO.Path]::GetFullPath($entry.path).Equals([IO.Path]::GetFullPath($path), [StringComparison]::OrdinalIgnoreCase)) { $entry.bytes = $record.afterBytes; $entry.sha256 = $record.afterSHA256 }
    }
}
[IO.File]::WriteAllText((Join-Path $output 'source-diff-consumer-rev4.patch'), ($diff -join "`n") + "`n", $encoding)
foreach ($entry in $freeze.inputs) {
    if ((Get-Item -LiteralPath $entry.path).Length -ne $entry.bytes -or (Get-FileHash -LiteralPath $entry.path).Hash.ToLowerInvariant() -ne $entry.sha256) { throw 'Postflight source delta' }
}
$freeze.verification = '6471 pre/post SHA256 verified; exactly three authorized provider files changed. CPP only removes three added CR bytes. Native CTest pending Luna.'
$freeze | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath (Join-Path $output 'source-freeze-vvl-provider-consumer-rev4.json') -Encoding utf8
ConvertTo-Json -InputObject @($freeze.inputs) -Depth 20 | Set-Content -LiteralPath (Join-Path $output 'source-freeze-vvl-provider-consumer-rev4-bare.json') -Encoding utf8
[ordered]@{ sources = $records; inputCount = 6471; sourceCount = 10; unexpectedDeltas = 0; nativeExecuted = $false; historicalRev3Preserved = $true; eolCorrection = 'Rev3 preservation claim was incorrect: original CPP had0CRLF/183LF, rev3 had3CRLF/183LF; rev4 removes exactly those3CR, yielding0CRLF/186LF. No semantic CPP changes.'; runtimeIsolation = 'Only CTest runtime ENVIRONMENT VK_IMPLICIT_LAYER_PATH points at an empty private build directory. No global environment/registry change or suppressed diagnostic.errors.'; packageArchiveRebuilt = $false; packagedReadmeHistorical = $true; status = 'SOURCE REVIEW READY; native consumer validation pending' } | ConvertTo-Json -Depth 15 | Set-Content -LiteralPath (Join-Path $output 'candidate-receipt-consumer-rev4.json') -Encoding utf8
Get-ChildItem -LiteralPath $output -File | ForEach-Object { [pscustomobject]@{ name = $_.Name; bytes = $_.Length; sha256 = (Get-FileHash -LiteralPath $_.FullName).Hash } } | ConvertTo-Json
