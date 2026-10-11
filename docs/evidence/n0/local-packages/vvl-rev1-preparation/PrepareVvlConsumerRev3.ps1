$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$base = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-preparation'
$output = Join-Path $base 'consumer-rev3'
$source = 'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/VulkanValidationLayers/tests/VulkanValidationLayersConsumer.cpp'
$oldFreeze = Join-Path $base 'source-freeze-vvl-provider-rev1-rev2.json'
if (Test-Path -LiteralPath $output) { throw 'New evidence output already exists' }
if ((Get-FileHash -LiteralPath $oldFreeze -Algorithm SHA256).Hash -ne 'C516374E93BED5F7B829A92CD0356D25A96A7289A9C17BE2D2FAA590AA3F90E3') { throw 'Historical freeze changed' }
$freeze = Get-Content -LiteralPath $oldFreeze -Raw | ConvertFrom-Json
if ($freeze.files.Count -ne 10 -or $freeze.inputs.Count -ne 6471) { throw 'Historical input counts changed' }
foreach ($entry in $freeze.inputs) {
    if ((Get-Item -LiteralPath $entry.path).Length -ne $entry.bytes -or
        (Get-FileHash -LiteralPath $entry.path -Algorithm SHA256).Hash -ne $entry.sha256) { throw "Unexpected input delta: $($entry.path)" }
}
$raw = [IO.File]::ReadAllBytes($source)
$encoding = [Text.UTF8Encoding]::new($false, $true)
$before = $encoding.GetString($raw)
if ((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne '53FDA14F06E763F4EF420300C63ABE90DD38A741E70A7C0908A71262F0C04DA2') { throw 'Consumer baseline changed' }
$eol = "`r`n"
$changes = @(
    @{
        before = '        Require(getInstance(VK_NULL_HANDLE, "JE_missing_function") == nullptr, "unknown entry point rejection");'
        after = '        // A raw layer forwards unknown queries through an initialized dispatch chain.' + $eol +
            '        Require(getInstance(VK_NULL_HANDLE, "vkCreateInstance") != nullptr, "layer global create entry point");'
    },
    @{
        before = '        Require(create && enumerate, "real loader global entry points");'
        after = '        Require(create && enumerate, "real loader global entry points");' + $eol +
            '        Require(getInstance(VK_NULL_HANDLE, "JE_missing_function") == nullptr, "loader unknown global entry point rejection");'
    },
    @{
        before = '        const auto destroy = reinterpret_cast<PFN_vkDestroyInstance>(getInstance(instance, "vkDestroyInstance"));'
        after = '        Require(getInstance(instance, "JE_missing_function") == nullptr, "loader unknown instance entry point rejection");' + $eol +
            '        const auto destroy = reinterpret_cast<PFN_vkDestroyInstance>(getInstance(instance, "vkDestroyInstance"));'
    }
)
$after = $before
$diff = [Collections.Generic.List[string]]::new()
$diff.Add('--- a/cmake/Tools/LocalPackages/VulkanValidationLayers/tests/VulkanValidationLayersConsumer.cpp')
$diff.Add('+++ b/cmake/Tools/LocalPackages/VulkanValidationLayers/tests/VulkanValidationLayersConsumer.cpp')
$lineDelta = 0
foreach ($change in $changes) {
    if ([regex]::Matches($before, [regex]::Escape($change.before)).Count -ne 1) { throw 'Replacement context is not unique' }
    $oldLine = ($before.Substring(0, $before.IndexOf($change.before)).Split("`n")).Count
    $newLines = $change.after.Split(@($eol), [StringSplitOptions]::None)
    $diff.Add(('@@ -{0},1 +{1},{2} @@' -f $oldLine, ($oldLine + $lineDelta), $newLines.Count))
    $diff.Add('-' + $change.before)
    foreach ($line in $newLines) { $diff.Add('+' + $line) }
    $lineDelta += $newLines.Count - 1
    $after = $after.Replace($change.before, $change.after)
}
$inverse = $after
foreach ($change in $changes) { $inverse = $inverse.Replace($change.after, $change.before) }
if ($inverse -cne $before -or -not [Linq.Enumerable]::SequenceEqual[byte]($encoding.GetBytes($inverse), $raw)) { throw 'Byte inverse failed' }
$nullRawUnknown = 'getInstance(VK_NULL_HANDLE, "JE_missing_function")'
if ($after.Substring(0, $after.IndexOf('    struct Diagnostics')).Contains($nullRawUnknown)) { throw 'Raw layer invalid query remains' }
[IO.Directory]::CreateDirectory($output) | Out-Null
[IO.File]::WriteAllBytes((Join-Path $output 'VulkanValidationLayersConsumer.before.cpp'), $raw)
[IO.File]::WriteAllText($source, $after, $encoding)
[IO.File]::WriteAllText((Join-Path $output 'source-diff-consumer-rev3.patch'), ($diff -join "`n") + "`n", $encoding)
$newHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()
$newSize = (Get-Item -LiteralPath $source).Length
$changed = 0
foreach ($entry in $freeze.inputs) {
    if ([IO.Path]::GetFullPath($entry.path).Equals([IO.Path]::GetFullPath($source), [StringComparison]::OrdinalIgnoreCase)) {
        $entry.sha256 = $newHash; $entry.bytes = $newSize; ++$changed
    }
}
if ($changed -ne 1) { throw 'Consumer input must occur exactly once' }
foreach ($entry in $freeze.files) {
    if ([IO.Path]::GetFullPath($entry.path).Equals([IO.Path]::GetFullPath($source), [StringComparison]::OrdinalIgnoreCase)) {
        $entry.sha256 = $newHash; $entry.bytes = $newSize
    }
}
$freeze.verification = '6471 inputs rehashed before edit and after edit; exactly one authorized CPP delta. Native execution pending.'
foreach ($entry in $freeze.inputs) {
    if ((Get-Item -LiteralPath $entry.path).Length -ne $entry.bytes -or
        (Get-FileHash -LiteralPath $entry.path -Algorithm SHA256).Hash -ne $entry.sha256) { throw "Postflight input delta: $($entry.path)" }
}
$freeze | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath (Join-Path $output 'source-freeze-vvl-provider-consumer-rev3.json') -Encoding utf8
ConvertTo-Json -InputObject @($freeze.inputs) -Depth 20 | Set-Content -LiteralPath (Join-Path $output 'source-freeze-vvl-provider-consumer-rev3-bare.json') -Encoding utf8
[ordered]@{
    schemaVersion = 1; historicalFreeze = $oldFreeze; source = $source
    beforeSha256 = '53FDA14F06E763F4EF420300C63ABE90DD38A741E70A7C0908A71262F0C04DA2'
    afterSha256 = $newHash; afterBytes = $newSize; sourceCount = 10; inputCount = 6471
    authorizedDeltaCount = 1; inverseByteExact = $true; encodingAndCrlfPreserved = $true
    demonstratedFailure = 'Native AV 0xC0000005 at TRACE12 direct layer getInstance(NULL, JE_missing_function).'
    internalCause = 'Inferred without stack: unknown layer query forwards through dispatch-key access before a chain exists.'
    fix = 'Direct layer queries known global vkCreateInstance; real loader rejects unknown global NULL and valid-instance queries.'
    preserved = @('API1.3.261', 'five exports', 'enumeration/truncation', 'unknown layer', 'negotiation1/2', 'exact invalid-flags VUID and VK_ERROR_VALIDATION_FAILED_EXT', 'valid instance creation/destruction without errors')
    primarySources = @('https://docs.vulkan.org/refpages/latest/refpages/source/vkGetInstanceProcAddr.html', 'https://raw.githubusercontent.com/KhronosGroup/Vulkan-Loader/main/docs/LoaderLayerInterface.md')
    status = 'SOURCE REVIEW READY; native consumer rebuild/tests pending Luna; provider archive/DLLs unchanged.'
} | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $output 'candidate-receipt-consumer-rev3.json') -Encoding utf8
Get-ChildItem -LiteralPath $output -File | ForEach-Object { [pscustomobject]@{ name = $_.Name; bytes = $_.Length; sha256 = (Get-FileHash -LiteralPath $_.FullName).Hash } } | ConvertTo-Json
