$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
function JEHash([string]$Path) {
    $Stream = [IO.File]::OpenRead($Path)
    try { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($Stream)) }
    finally { $Stream.Dispose() }
}
function JECheck([string]$Path, [string]$Hash) {
    if ((JEHash $Path) -cne $Hash) { throw "Changed result/input: $Path" }
}
function JEResult([string]$Path) {
    [IO.File]::ReadAllText($Path) | ConvertFrom-Json
}
function JEPhase($Phase) {
    foreach ($Stream in @('stdout', 'stderr')) {
        JECheck $Phase.$Stream $Phase.($Stream + 'SHA256')
        if ([IO.FileInfo]::new($Phase.$Stream).Length -ne $Phase.($Stream + 'Bytes')) { throw 'Raw log size differs' }
    }
}
$Base = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages'
$RemainingRoot = "$Base/libsamplerate-rev2-guards-remaining-preparation/review2"
$Freeze = JEResult "$RemainingRoot/driver-freeze.json"
JECheck "$RemainingRoot/driver-freeze.json" 'B5175ED72D0923D50A3CBC5F18DD913894CF7B2E5D28923B803F7E5526ACAD83'
if ($Freeze.entries.Count -ne 210) { throw 'Expected 210 final frozen inputs' }
foreach ($Entry in $Freeze.entries) {
    if ([IO.FileInfo]::new($Entry.path).Length -ne $Entry.bytes) { throw 'Frozen input size differs' }
    JECheck $Entry.path $Entry.sha256
}
$ConsumerPath = "$PSScriptRoot/consumer-run/validation-result.json"
$Consumer = JEResult $ConsumerPath
if ($Consumer.status -cne 'CONSUMER_CASES_PASSED_ONLY' -or $Consumer.failure -or
    $Consumer.preflightMatches -ne 195 -or $Consumer.postflightMatches -ne 195 -or $Consumer.phases.Count -ne 5) { throw 'Consumer coverage/failure differs' }
foreach ($Phase in $Consumer.phases) {
    JEPhase $Phase
    if ($Phase.exitCode -ne 0) { throw 'Consumer native phase failed' }
}
$ExpectedCases = @('api','conversions','fast','medium','best','reset','errors') | ForEach-Object { 'JE.LocalPackages.LibSamplerate.' + $_ }
foreach ($Check in $Consumer.checks) {
    JECheck $Check.junit $Check.junitSHA256
    JECheck $Check.executable $Check.executableSHA256
    JECheck $Check.staticLibraryPath 'F522378C7F300B064A678DAD0663D73D5B917769D1BC421933D8786DA15A8DB3'
    [xml]$JUnit = [IO.File]::ReadAllText($Check.junit)
    $Cases = @($JUnit.SelectNodes('//testcase'))
    if ($Cases.Count -ne 7 -or $JUnit.SelectNodes('//failure|//error|//skipped').Count -or
        @(Compare-Object ($Cases.name | Sort-Object) ($ExpectedCases | Sort-Object)).Count) { throw 'JUnit coverage/outcome differs' }
    $LastTest = [IO.File]::ReadAllText("$PSScriptRoot/consumer-run/$($Check.configuration)-LastTest.raw.log")
    if ([regex]::Matches($LastTest, '(?m)^Test Passed\.\r?$').Count -ne 7) { throw 'LastTest coverage differs' }
}
if ($Consumer.checks.Count -ne 2 -or $Consumer.checks.configuration[0] -cne 'Release' -or $Consumer.checks.configuration[1] -cne 'Debug') { throw 'Configuration coverage differs' }
$PartialPath = "$PSScriptRoot/guards-run/validation-result.json"
JECheck $PartialPath '0192C2623701D70CD3BF6C87A34ED639AD2EAD7E8D0924CDCFDA280149072BA0'
$Partial = JEResult $PartialPath
$RemainingPath = "$RemainingRoot/remaining-guards-run/validation-result.json"
$Remaining = JEResult $RemainingPath
if ($Partial.phases.Count -ne 2 -or $Remaining.phases.Count -ne 6 -or $Remaining.failure -or
    $Remaining.preflightMatches -ne 209 -or $Remaining.postflightMatches -ne 209 -or
    $Remaining.status -cne 'SIX_REMAINING_EXPECTED_GUARD_REJECTIONS_ONLY') { throw 'Guard coverage/failure differs' }
$GuardChecks = @()
foreach ($Run in @($Partial, $Remaining)) {
    foreach ($Phase in $Run.phases) {
        JEPhase $Phase
        $Name = $Phase.name.Substring(6)
        $Check = @($Run.checks | Where-Object name -CEQ $Name)
        if ($Check.Count -ne 1 -or $Phase.exitCode -ne 1) { throw 'Expected one native rejection per guard' }
        $Diagnostic = [IO.File]::ReadAllText($Phase.stderr) + [IO.File]::ReadAllText($Phase.stdout)
        $Normalized = [regex]::Replace($Diagnostic, '\s+', ' ').Trim()
        if (-not $Normalized.Contains($Check[0].expectedDiagnostic) -or $Check[0].beforeExists -ne $Check[0].afterExists -or
            (Test-Path -LiteralPath $Check[0].output) -ne $Check[0].beforeExists) { throw 'Guard diagnostic/output differs' }
        $GuardChecks += [ordered]@{ name=$Name; exitCode=$Phase.exitCode; PID=$Phase.PID; seconds=$Phase.elapsedSeconds; rejection=$true }
    }
}
if (@($GuardChecks.name | Sort-Object -Unique).Count -ne 8) { throw 'Expected eight unique guards without reruns' }
$DebugLog = [IO.File]::ReadAllText("$PSScriptRoot/consumer-run/build-Debug.stdout.raw.log")
if (-not $DebugLog.Contains('LNK4098')) { throw 'Expected retained Debug mixed-CRT warning' }
$Receipt = [ordered]@{
    utc = [DateTime]::UtcNow.ToString('o')
    status = 'PARENT_FOCAL_LIBSAMPLERATE_REV2_ACCEPTED_FOR_CATALOG_INTEGRATION'
    acceptedForCatalogIntegration = $true
    sourceInputDeltas = 0
    inputsRehashed = 210
    consumerCases = 14
    consumerResultSHA256 = JEHash $ConsumerPath
    consumerPhases = @($Consumer.phases | Select-Object name,exitCode,elapsedSeconds,PID)
    guards = $GuardChecks
    guardCases = 8
    historicalPartialGuardResultSHA256 = JEHash $PartialPath
    remainingGuardResultSHA256 = JEHash $RemainingPath
    matcherCorrection = 'Only diagnostic whitespace; URI/UNC native runs reused; six previously unexecuted guards ran once'
    archiveSHA256 = '01086CD19A1D241B4B0504FE16142C80BF29AF329543E67FF773301EDAA95122'
    archiveBytes = 3954496
    nativeDebugWarning = 'LNK4098 retained: Release /MD library with Debug /MDd consumer; no general mixed-CRT or legacy ABI acceptance'
    oldOutputGuardLimit = 'Directory existence only'
    nativeExecutionByParent = $false
    engineEditorRuntimeAccepted = $false
}
$Bytes = [Text.UTF8Encoding]::new($false).GetBytes(($Receipt | ConvertTo-Json -Depth 10) + "`n")
$Path = Join-Path $PSScriptRoot 'parent-focal-acceptance-rev1.json'
$Stream = [IO.File]::Open($Path, [IO.FileMode]::CreateNew)
try { $Stream.Write($Bytes, 0, $Bytes.Length) } finally { $Stream.Dispose() }
$Receipt | ConvertTo-Json -Depth 10
"receiptSHA256=$(JEHash $Path)"
