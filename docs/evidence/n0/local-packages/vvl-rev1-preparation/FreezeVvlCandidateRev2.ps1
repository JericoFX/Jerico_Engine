$ErrorActionPreference = 'Stop'
$repo = 'F:/JericoEngine/JericoEngine'
$prep = "$repo/build/n0-validation/local-packages/vvl-rev1-preparation"
$source = "$repo/cmake/Tools/LocalPackages/VulkanValidationLayers"
$oldAudit = "$repo/build/n0-validation/local-packages/vulkan-validationlayers-current-audit"
$encoding = [System.Text.UTF8Encoding]::new($false)
function WriteNewJson([string]$Path, $Value) {
    if (Test-Path -LiteralPath $Path) { throw "Existing evidence: $Path" }
    [IO.File]::WriteAllText($Path, ($Value | ConvertTo-Json -Depth 100) + "`n", $encoding)
}
function Witness([string]$Path, [string]$Role) {
    $full = [IO.Path]::GetFullPath($Path)
    $item = Get-Item -LiteralPath $full
    if ($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw "Not a regular file: $full" }
    [ordered]@{ path=$full; bytes=$item.Length; sha256=(Get-FileHash -LiteralPath $full -Algorithm SHA256).Hash.ToLowerInvariant(); role=$Role }
}
function RequireWitness($Entry) {
    $actual = Witness $Entry.path 'verified existing input'
    if ($actual.bytes -ne $Entry.bytes -or $actual.sha256 -ne $Entry.sha256) { throw "Changed input: $($Entry.path)" }
}
$prior = Get-Content -LiteralPath "$oldAudit/source-freeze-vvl-audit.json" -Raw | ConvertFrom-Json
foreach ($entry in @($prior.files) + @($prior.referenceInputs)) { RequireWitness $entry }
$comparisons = @(Get-Content -LiteralPath "$prep/acquisition/primary-tree-comparison-summary.json" -Raw | ConvertFrom-Json)
if ($comparisons.Count -ne 7 -or ($comparisons.matches | Measure-Object -Sum).Sum -ne 2785) { throw 'Unexpected primary source closure' }
foreach ($entry in $comparisons) {
    if ($entry.truncated -or $entry.bad.Count -or $entry.missing.Count -or $entry.gitlinks.Count -or $entry.matches -ne $entry.primaryFileCount) { throw "Incomplete primary tree: $($entry.id)" }
}
$immutable = [Collections.Generic.List[object]]::new()
foreach ($id in 'vvl','headers','utility','spirv-headers','spirv-tools','robin','mimalloc') {
    $extraction = Get-Content -LiteralPath "$prep/acquisition/$id/source-extraction.json" -Raw | ConvertFrom-Json
    $actualPaths = @(Get-ChildItem -LiteralPath $extraction.snapshotPath -Recurse -File)
    if ($actualPaths.Count -ne $extraction.fileCount) { throw "Tree count changed: $id" }
    foreach ($entry in $extraction.files) {
        RequireWitness $entry
        $immutable.Add((Witness $entry.path "immutable upstream snapshot: $id"))
    }
}
$sources = @(Get-ChildItem -LiteralPath $source -Recurse -File | Sort-Object FullName | ForEach-Object { Witness $_.FullName 'new provider source' })
if ($sources.Count -ne 10) { throw 'Expected exactly ten provider sources' }
$diff = [Text.StringBuilder]::new()
foreach ($entry in $sources) {
    $relative = [IO.Path]::GetRelativePath($repo, $entry.path).Replace('\','/')
    $text = [IO.File]::ReadAllText($entry.path).Replace("`r`n", "`n")
    $hasFinalNewline = $text.EndsWith("`n")
    $body = if ($hasFinalNewline) { $text.Substring(0,$text.Length-1) } else { $text }
    $lines = $body.Split("`n")
    [void]$diff.Append("diff --git a/$relative b/$relative`nnew file mode 100644`n--- /dev/null`n+++ b/$relative`n@@ -0,0 +1,$($lines.Count) @@`n")
    foreach ($line in $lines) { [void]$diff.Append("+$line`n") }
    if (-not $hasFinalNewline) { [void]$diff.Append("\ No newline at end of file`n") }
}
$diffPath = "$prep/source-diff-vvl-rev1-rev2.patch"
if (Test-Path -LiteralPath $diffPath) { throw "Existing evidence: $diffPath" }
[IO.File]::WriteAllText($diffPath, $diff.ToString(), $encoding)
$cmake = 'C:/Program Files/CMake/bin/cmake.exe'
$ctest = 'C:/Program Files/CMake/bin/ctest.exe'
$python = 'F:/JericoEngine/JericoEngine/build/n0-validation/original-engine-deps/Python-rev3/packages/python-3.10.13-je-rev3-windows/python/python.exe'
$loader = 'C:/Windows/System32/vulkan-1.dll'
$output = "$repo/build/n0-validation/local-packages/vvl-local-rev1"
$consumer = "$repo/build/n0-validation/local-packages/vvl-rev1-consumer"
$package = "$consumer/package"
$headers = "$prep/acquisition/headers/snapshot"
$commands = @(
    [ordered]@{ label='producer'; executable=$cmake; argv=@("-DJE_VVL_ARCHIVES_ROOT=$prep/acquisition", "-DJE_VVL_PYTHON=$python", '-DJE_VVL_PYTHON_SHA256=6235abe2248c14ad86dfe00dbb7bc2396eb4b91f663412c211905e279bce9d6e', "-DJE_VVL_OUTPUT=$output", '-P', "$source/BuildVulkanValidationLayersPackage.cmake") },
    [ordered]@{ label='extract-produced-archive'; precondition='Create the new package working directory only after producer exit0; never overwrite'; workingDirectory=$package; executable=$cmake; argv=@('-E','tar','xJf',"$output/packages/vulkan-validationlayers-1.3.261-je-rev1-windows.tar.xz") },
    [ordered]@{ label='consumer-configure'; executable=$cmake; argv=@('-S',"$source/tests",'-B',"$consumer/build",'-G','Visual Studio 17 2022','-A','x64','-T','v143',"-DJE_VVL_PACKAGE_ROOT=$package","-DJE_VVL_HEADERS_ROOT=$headers","-DJE_VVL_LOADER=$loader",'-DJE_VVL_LOADER_SHA256=0419974f00e82a3d619077ba414da265a774f8db9d45ad93bc1843f44b2c2c1f') },
    [ordered]@{ label='consumer-build'; executable=$cmake; argv=@('--build',"$consumer/build",'--config','Release','--target','JEVulkanValidationLayersConsumer','--parallel','2') },
    [ordered]@{ label='consumer-ctest'; executable=$ctest; argv=@('--test-dir',"$consumer/build",'-C','Release','--output-on-failure','-R','^JE\.LocalPackages\.VVL\.') }
)
WriteNewJson "$prep/validation-assignment-vvl-rev1-rev2.json" ([ordered]@{
    status='PREPARED NOT RUN; parent review and Luna dispatch required'; nativeOwner='Luna'; commands=$commands
    sourceRoot=$source; outputMustBeFresh=$output; consumerOutputMustBeFresh=$consumer
    nativeCases=@('release.exports','release.runtime','debug.exports','debug.runtime')
    producerNegatives=@('missing or changed source archive/hash before output creation','remote/UNC/symlink input before I/O','existing output retained byte-identically','wrong private SPIRV patch witness rejected')
    capture='Capture each argv/stdout/stderr/native exit immediately, before another native call; retain failures and postflight all frozen inputs'
    limits=@('No native execution by writer','No catalogue/cache/root changes','No certification of redistribution rights','Native consumer does not prove Editor rendering or frame performance')
})
WriteNewJson "$prep/candidate-receipt-vvl-rev1-rev2.json" ([ordered]@{
    status='IMPLEMENTED NOT NATIVELY VERIFIED'; sourceFileCount=$sources.Count; sourceBytes=($sources.bytes | Measure-Object -Sum).Sum
    sourceDiff=(Witness $diffPath 'new-files textual diff; bytes remain authoritative in freeze')
    sourceArchiveCount=7; snapshotFileCount=$immutable.Count; primaryGitBlobMatches=2785; priorAuditVerified=(@($prior.files).Count+@($prior.referenceInputs).Count)
    firstTarAuditPreserved='PAX global comment metadata rejected by first inspector; revision2 accepts only exact commit comment and never materializes metadata'
    patch='Only private SPIRV-Tools version-description line; before/after SHA and exact inverse preserved'
    productPython= $false; buildGenerationPython='Previously audited external CPython3.10.13 JE rev3 build-generation tool, not deployed'
    features='Release+Debug, C++17, allocator/hash ON, tests/updater/codegen OFF; original Release pair deployment'
    helperChanged= $false; sharedConfigurationChanged= $false; nativeExecuted= $false; acceptance='PENDING PARENT REVIEW / LUNA NATIVE / ROOT / EDITOR'
})
$references = [Collections.Generic.List[object]]::new()
foreach ($entry in @($prior.files) + @($prior.referenceInputs)) { $references.Add((Witness $entry.path $entry.role)) }
foreach ($path in @("$oldAudit/source-freeze-vvl-audit.json", "$oldAudit/parent-acquisition-provider-approval.json", "$repo/cmake/Tools/LocalPackages/JECreateLocalPackage.cmake", "$repo/cmake/Tools/LocalPackages/Python/JERequireLocalPath.cmake", "$repo/LICENSE.txt", "$repo/LICENSE_APACHE2.TXT", "$repo/LICENSE_MIT.TXT", $cmake, $ctest, $python, $loader)) { $references.Add((Witness $path 'unchanged reference/tool')) }
$pythonPackage = "$repo/build/n0-validation/original-engine-deps/Python-rev3/packages/python-3.10.13-je-rev3-windows"
foreach ($file in Get-ChildItem -LiteralPath $pythonPackage -Recurse -File) {
    $references.Add((Witness $file.FullName 'previously audited external CPython rev3 tool payload, not distributed'))
}
foreach ($file in Get-ChildItem -LiteralPath $prep -Recurse -File) {
    if ($file.FullName -notmatch '[\\/]snapshot[\\/]') { $references.Add((Witness $file.FullName 'owned acquisition/preparation evidence or archive')) }
}
$unique = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($entry in @($sources) + @($immutable.ToArray()) + @($references.ToArray())) {
    if ($unique.ContainsKey($entry.path)) {
        if ($unique[$entry.path].sha256 -ne $entry.sha256) { throw "Duplicate divergent input: $($entry.path)" }
    } else { $unique.Add($entry.path,$entry) }
}
$all = @($unique.Values | Sort-Object path)
foreach ($entry in $all) { RequireWitness $entry }
WriteNewJson "$prep/source-freeze-vvl-provider-rev1-rev2.json" ([ordered]@{
    schemaVersion=1; sourceFileCount=$sources.Count; immutableFileCount=$immutable.Count; uniqueInputCount=$all.Count
    files=$sources; inputs=$all; verification='All byte counts/SHA256 reread; full trees matched retained extraction manifests. No native execution.'
})
WriteNewJson "$prep/source-freeze-vvl-provider-rev1-rev2-bare.json" $all
WriteNewJson "$prep/freeze-verification-vvl-rev1-rev2.json" ([ordered]@{
    inputCount=$all.Count; matched=$all.Count; changed=@(); sourceFiles=$sources.Count; immutableFiles=$immutable.Count
    freeze=(Witness "$prep/source-freeze-vvl-provider-rev1-rev2.json" 'final provider freeze')
    bareFreeze=(Witness "$prep/source-freeze-vvl-provider-rev1-rev2-bare.json" 'deduplicated bare freeze')
    assignment=(Witness "$prep/validation-assignment-vvl-rev1-rev2.json" 'proposed native commands, NOT RUN')
})
Get-Content -LiteralPath "$prep/freeze-verification-vvl-rev1-rev2.json" -Raw
