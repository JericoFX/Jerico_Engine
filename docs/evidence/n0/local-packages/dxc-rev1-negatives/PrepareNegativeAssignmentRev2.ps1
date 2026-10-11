#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($PSVersionTable.PSEdition -ne 'Core') { throw 'PowerShell Core required' }
$root = [IO.Path]::GetFullPath($PSScriptRoot)
if ($root -ne 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\dxc-rev1-negatives') { throw 'Assigned output root differs' }
$encoding = [Text.UTF8Encoding]::new($false)
function Write-New([string] $path, [string] $text) {
    if (Test-Path -LiteralPath $path) { throw "Preserve existing evidence: $path" }
    [IO.File]::WriteAllText($path,$text,$encoding)
}
function Entry([string] $path) { [ordered]@{ path = [IO.Path]::GetFullPath($path); bytes = (Get-Item -LiteralPath $path).Length; sha256 = (Get-FileHash -LiteralPath $path).Hash } }
function Json($value) { ($value | ConvertTo-Json -Depth 24) + "`n" }
$oldAssignmentPath = Join-Path $root 'negative-assignment.json'
$oldDriverPath = Join-Path $root 'RunDxcNegatives.ps1'
if ((Get-FileHash -LiteralPath $oldAssignmentPath).Hash -ne '11B08617C06A45BD3DBAD2839492C26DE819F205E18CD1D3B6023EE04B331DFC') { throw 'Historical assignment changed' }
if ((Get-FileHash -LiteralPath $oldDriverPath).Hash -ne '7022F2F2A31B905C483EDBA95EC074DFD1B5D829687B303224479E5059540AA0') { throw 'Historical driver changed' }
$old = Get-Content -LiteralPath $oldAssignmentPath -Raw | ConvertFrom-Json -AsHashtable
$assignment = ([IO.File]::ReadAllText($oldAssignmentPath).Replace('attempt1','attempt-rev2') | ConvertFrom-Json -AsHashtable)
$freezePath = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/dxc-rev1-preparation/consumer-candidate-rev2/source-freeze-dxc-consumer-rev2.json'
$freezeHash = '4427A9E7947AC6999D9FFA36FA55704885CADBF4E4C0E745D4B89CE6ADC0F03B'
if ((Get-FileHash -LiteralPath $freezePath).Hash -ne $freezeHash) { throw 'Approved consumer freeze differs' }
$freeze = Get-Content -LiteralPath $freezePath -Raw | ConvertFrom-Json
if ($freeze.files.Count -ne 23373) { throw 'Approved input count differs' }
$oldFreeze = Get-Content -LiteralPath $old.sourceFreeze.path -Raw | ConvertFrom-Json
$current = @{}
foreach ($file in $freeze.files) {
    $key = [IO.Path]::GetFullPath($file.path)
    if ($current.ContainsKey($key)) { throw 'Duplicate freeze path' }
    $current[$key] = $file
    if ((Get-Item -LiteralPath $key).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $key).Hash -ne $file.sha256) { throw "Input changed: $key" }
}
$deltas = @(foreach ($file in $oldFreeze.files) {
    $key = [IO.Path]::GetFullPath($file.path)
    if (-not $current.ContainsKey($key)) { throw "Previous input absent: $key" }
    if ($file.sha256 -ne $current[$key].sha256 -or $file.bytes -ne $current[$key].bytes) { [ordered]@{ before = $file; after = $current[$key] } }
})
if ($deltas.Count -ne 1 -or $deltas[0].after.path -notlike '*DirectXShaderCompilerDxcConsumer.cpp' -or $deltas[0].after.sha256 -ne 'A09161480B2D8C90C1B9DA96A50DE1F3909C09CD7D6EA0554556212C95851C3A') { throw 'Unexpected current source deltas' }
$assignment.sourceFreeze = @{ path = [IO.Path]::GetFullPath($freezePath); sha256 = $freezeHash; files = 23373 }
$attempt = Join-Path $root 'attempt-rev2'
if (Test-Path -LiteralPath $attempt) { throw 'Fresh attempt already exists' }
$missingCl = Join-Path $attempt 'fixtures/missing-secondary-cl.exe'
$missingNinja = Join-Path $attempt 'fixtures/missing-secondary-ninja.exe'
$missingFunctionFile = Join-Path $attempt 'fixtures/missing-after-cl-sha'
$helperPath = Join-Path $attempt 'fixtures/wrong-cl-sha.cmake'
$helperInclude = 'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/Python/JERequireLocalPath.cmake'
$originalClCase = @($assignment.cases | Where-Object label -eq 'cl-sha')[0]
$clPath = @($originalClCase.argv | Where-Object { $_ -like '-DJE_DXC_CL=*' })[0].Substring(12).Replace('\','/')
$helperContent = @"
cmake_minimum_required(VERSION 3.24)
include("$helperInclude")
# FUNCTION-GUARD: actual shared verification function; no producer or tool execution.
JE_python_verify_file("$clPath" "$('0' * 64)" "audited existing external CL")
JE_python_verify_file("$($missingFunctionFile.Replace('\','/'))" "$('0' * 64)" "JE secondary missingfile")
"@ + "`n"
$assignment.helperFixture = @{ path = $helperPath; content = $helperContent; include = (Entry $helperInclude); scope = 'FUNCTION-GUARD' }
foreach ($case in $assignment.cases) {
    $case.scope = if ($case.label -eq 'overlay-mismatch') { 'PRIVATE-OVERLAY' } elseif ($case.label -eq 'cl-sha') { 'FUNCTION-GUARD' } else { 'PRODUCER' }
    if ($case.scope -eq 'PRODUCER') {
        $case.argv = @($case.argv | ForEach-Object { if ($_ -like '-DJE_DXC_CL=*') { "-DJE_DXC_CL=$missingCl" } else { $_ } })
        $case.secondaryMissingPath = $missingCl
        if ($case.label -in @('missing-tool','python-sha')) {
            $case.argv = @($case.argv | ForEach-Object { if ($_ -like '-DJE_DXC_NINJA=*') { "-DJE_DXC_NINJA=$missingNinja" } else { $_ } })
            $case.secondaryMissingPath = $missingNinja
        }
    } elseif ($case.scope -eq 'FUNCTION-GUARD') {
        $case.argv = @('-P',$helperPath)
        $case.secondaryMissingPath = $missingFunctionFile
    } else {
        $case.secondaryMissingPath = Join-Path $attempt 'fixtures/overlay-work/dxc/utils/version/version.inc'
    }
}
$assignment.pathsMustRemainAbsent = @($assignment.pathsMustRemainAbsent) + @($missingCl,$missingNinja,$missingFunctionFile,(Join-Path $attempt 'fixtures/overlay-work/dxc/utils/version/version.inc'))
$assignment.scopeCounts = @{ PRODUCER = 16; 'PRIVATE-OVERLAY' = 1; 'FUNCTION-GUARD' = 1 }
$assignment.fixtureDesign = @($assignment.fixtureDesign | ForEach-Object { $_.Replace('Three fixture files','Four fixture files') }) + @('Each producer variant has a missing tool secondary guard before output creation. Python SHA additionally stops at absent Ninja; Ninja SHA stops at absent CL. No variant supplies all positive producer inputs.','CL SHA is FUNCTION-GUARD: the real JE_python_verify_file followed by missing file fallback; it is not counted as a producer invocation.','The fourth runtime fixture is a local CMake script calling only the actual verification function, never the compiler or producer.','Overlay mismatch also leaves the next fixed version file absent. No fixture or junction is created during writer preparation.')
$assignment.resultContract = '18 labels: 16 PRODUCER + 1 PRIVATE-OVERLAY + 1 FUNCTION-GUARD. Require nonzero, specific normalized diagnostic, no production output, and unchanged four fixtures/full memberships. Preserve all 23373 source inputs and five production witnesses. Stop on any failure.'
$assignmentPath = Join-Path $root 'negative-assignment-rev2.json'
Write-New $assignmentPath (Json $assignment)
$driver = [IO.File]::ReadAllText($oldDriverPath).Replace("'attempt1'","'attempt-rev2'").Replace("'negative-assignment.json'","'negative-assignment-rev2.json'").Replace('11B08617C06A45BD3DBAD2839492C26DE819F205E18CD1D3B6023EE04B331DFC',(Get-FileHash -LiteralPath $assignmentPath).Hash).Replace('23348','23373')
$anchor = '$fixtureFiles = @((Join-Path $attempt ''fixtures/existing-output/retain.txt''),$changedArchive,$wrongOverlay)'
if (-not $driver.Contains($anchor)) { throw 'Unique fixture anchor absent' }
$driver = $driver.Replace($anchor, '[IO.File]::WriteAllText($assignment.helperFixture.path,$assignment.helperFixture.content,$encoding)' + "`n" + '$fixtureFiles = @((Join-Path $attempt ''fixtures/existing-output/retain.txt''),$changedArchive,$wrongOverlay,$assignment.helperFixture.path)')
$driver = $driver.Replace('$junctionTarget = Join-Path $attempt ''fixtures/reparse-target''','$junctionTarget = [IO.Path]::GetFullPath((Join-Path $attempt ''fixtures/reparse-target''))')
$driver = $driver.Replace('label = $case.label; nativeExit', 'label = $case.label; scope = $case.scope; secondaryMissingPath = $case.secondaryMissingPath; nativeExit')
$driverPath = Join-Path $root 'RunDxcNegativesRev2.ps1'
Write-New $driverPath $driver
$tokens = $null; $errors = $null
[void][Management.Automation.Language.Parser]::ParseFile($driverPath,[ref]$tokens,[ref]$errors)
if ($errors.Count) { throw 'Driver parser errors' }
$proof = [ordered]@{ status = 'PREPARED ONLY; NOT RUN'; nativeExecuted = $false; acceptance = $false; sourceInputs = 23373; unchangedPreviousInputs = 23347; authorizedDeltas = $deltas; additionalInputs = 25; scopes = $assignment.scopeCounts; fixtureFilesCreatedByWriter = 0; runtimeFixtureCount = 4; powershellParserErrors = $errors.Count; historicalInputs = @((Entry $oldDriverPath),(Entry $oldAssignmentPath),(Entry (Join-Path $root 'negative-setup-freeze.json'))); driver = (Entry $driverPath); assignment = (Entry $assignmentPath); secondaryGuardCases = @($assignment.cases | ForEach-Object { @{ label = $_.label; scope = $_.scope; path = $_.secondaryMissingPath; expectedError = $_.expectedError; mustRemainAbsent = $_.mustRemainAbsent } }); limitations = 'No negative CMake invocation, compiler, test, fixture, or junction was executed/created. Native results pending parent review and Luna.' }
$proofPath = Join-Path $root 'negative-preparation-proof-rev2.json'
Write-New $proofPath (Json $proof)
$freezeEntries = @((Entry $driverPath),(Entry $assignmentPath),(Entry $PSCommandPath),(Entry $proofPath),(Entry $freezePath),(Entry $oldDriverPath),(Entry $oldAssignmentPath),(Entry (Join-Path $root 'negative-setup-freeze.json'))) + @($assignment.productionWitnesses)
Write-New (Join-Path $root 'negative-setup-freeze-rev2.json') (Json @{ status = 'PENDING LUNA EXECUTION'; sourceInputs = 23373; files = $freezeEntries; caseCount = 18; scopes = $assignment.scopeCounts; nativeExecuted = $false })
[pscustomobject]@{ driver = (Entry $driverPath); assignment = (Entry $assignmentPath); freeze = (Entry (Join-Path $root 'negative-setup-freeze-rev2.json')); proof = (Entry $proofPath) } | ConvertTo-Json -Depth 4
