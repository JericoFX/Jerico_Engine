#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSEdition -ne 'Core') { throw 'PowerShell Core is required' }
$reviewRoot = $PSScriptRoot
$receiptPath = Join-Path $reviewRoot 'parent-source-review-vvl-rev1.json'
if (Test-Path -LiteralPath $receiptPath) { throw 'Review receipt already exists; preserve it' }
$expected = [ordered]@{
    'source-freeze-vvl-provider-rev1-rev2.json' = 'C516374E93BED5F7B829A92CD0356D25A96A7289A9C17BE2D2FAA590AA3F90E3'
    'validation-assignment-vvl-rev1-rev2.json' = 'AF316CA5C13A937819B9B729AF5E249569C54013FF766BDC7CFD2779E127CA93'
    'source-diff-vvl-rev1-rev2.patch' = '70BC5E788407FECDD2BE32F4063305E94DA015595DB546B0D411C30F1C56F1EB'
}
foreach ($name in $expected.Keys) {
    if ((Get-FileHash -LiteralPath (Join-Path $reviewRoot $name) -Algorithm SHA256).Hash -ne $expected[$name]) {
        throw "Candidate witness differs: $name"
    }
}
$freeze = Get-Content -LiteralPath (Join-Path $reviewRoot 'source-freeze-vvl-provider-rev1-rev2.json') -Raw | ConvertFrom-Json
if ($freeze.uniqueInputCount -ne 6471 -or $freeze.inputs.Count -ne 6471 -or $freeze.files.Count -ne 10) { throw 'Unexpected freeze coverage' }
$seen = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$mismatches = @()
foreach ($entry in $freeze.inputs) {
    $canonical = [System.IO.Path]::GetFullPath($entry.path)
    if (-not $seen.Add($canonical)) { throw "Duplicate frozen path: $canonical" }
    $item = Get-Item -LiteralPath $canonical
    if ($item.PSIsContainer -or $item.Length -ne $entry.bytes -or (Get-FileHash -LiteralPath $canonical -Algorithm SHA256).Hash -ne $entry.sha256) {
        $mismatches += $canonical
    }
}
if ($mismatches.Count) { throw "Frozen input mismatch: $($mismatches -join ', ')" }
$toolExpected = @(
    @{path='C:/Program Files/CMake/bin/cmake.exe'; sha256='1AA884BF1F4949327FFFCC8EE4A97C2D684BDC1D0A64B71F01DC16321C7FBC64'},
    @{path='C:/Users/Jerico/.cache/codex-runtimes/codex-primary-runtime/dependencies/native/powershell/pwsh.exe'; sha256='362A356CE7F0940EC74F73A8FC2C990A2CC24A38A11C90BBD8ECA947110AD139'},
    @{path='C:/Windows/System32/vulkan-1.dll'; sha256='0419974F00E82A3D619077BA414DA265A774F8DB9D45AD93BC1843F44B2C2C1F'}
)
$tools = foreach ($entry in $toolExpected) {
    $item = Get-Item -LiteralPath $entry.path
    $sha = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash
    if ($sha -ne $entry.sha256) { throw "Tool witness differs: $($entry.path)" }
    [ordered]@{path=$item.FullName; bytes=$item.Length; sha256=$sha; fileVersion=$item.VersionInfo.FileVersion}
}
$receipt = [ordered]@{
    status='PARENT SOURCE REVIEW ACCEPTED FOR FOCAL NATIVE EXECUTION; NOT COMPILED'
    checkedInputs=$seen.Count; expectedInputs=6471; sourceFiles=10; mismatches=$mismatches
    candidateWitnesses=$expected; toolWitnesses=@($tools)
    sourceReview=@(
        'Reviewed final producer, wrapper, Find, private provenance patch, seven pins, README, NOTICE and native consumer.'
        'Seven primary archives match selected original dependency closure; immutable source trees were verified independently.'
        'CPython3.10.13 rev3 is the audited external code-generation tool only; bytecode writes disabled.'
        'Release and Debug builds retained; original release-pair deployment; SPIRV shared auxiliary and Python not packaged.'
        'Fresh private outputs; direct local inputs; no updater/download/global installation.'
        'Consumer tests real DLL exports, real loader rejection and valid Vulkan instance; no Editor/frame acceptance.'
    )
    nativeOwner='Luna'; nativeStatus='NOT RUN'
}
[System.IO.File]::WriteAllText($receiptPath, ($receipt | ConvertTo-Json -Depth 10) + "`n", [System.Text.UTF8Encoding]::new($false))
$receipt | Select-Object status,checkedInputs,sourceFiles,nativeStatus | ConvertTo-Json
