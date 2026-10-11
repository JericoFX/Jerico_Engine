$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
function JEHash([string]$Path) {
    $Stream = [IO.File]::OpenRead($Path)
    try { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($Stream)) }
    finally { $Stream.Dispose() }
}
function JECheck([string]$Path, [string]$Expected) {
    if ((JEHash $Path) -cne $Expected) { throw "Changed review input: $Path" }
}
function JENoReparse([string]$Path) {
    $Node = [IO.FileInfo]::new($Path)
    while ($null -ne $Node) {
        if (($Node.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Reparse input: $Path" }
        if ($Node -is [IO.FileInfo]) { $Node = $Node.Directory } else { $Node = $Node.Parent }
    }
}
$AssignmentPath = Join-Path $PSScriptRoot 'assignment.json'
JECheck $AssignmentPath '000D2D8B8944B5245527B6AB72D840B9CA17C4DEFE73A3A228E35E464151E193'
$Assignment = [IO.File]::ReadAllText($AssignmentPath) | ConvertFrom-Json
JECheck $Assignment.validationInputs '24000B278E47F18F63E8E42E8EA7E57AC9519977CB7832E628C8647590786F8C'
JECheck $Assignment.driverFreeze 'C4515095FBB0F721788F343707FE1F36E0F0E742F709F3218789914C1A2E9852'
JECheck $Assignment.argumentAuthority 'BA3B3D3E25CB765D1B978ED147F8AA0546A2BA6F0D9B3D85910E6C9EA14AAAF0'
$Freeze = [IO.File]::ReadAllText($Assignment.driverFreeze) | ConvertFrom-Json
if ($Freeze.count -ne 196 -or $Freeze.entries.Count -ne 196) { throw 'Expected 196 frozen driver inputs' }
foreach ($Entry in $Freeze.entries) {
    JENoReparse $Entry.path
    if ([IO.FileInfo]::new($Entry.path).Length -ne $Entry.bytes) { throw "Changed input size: $($Entry.path)" }
    JECheck $Entry.path $Entry.sha256
}
foreach ($Driver in $Assignment.drivers) {
    JECheck $Driver.path $Driver.sha256
    $Tokens = $null; $ParseErrors = $null
    $null = [Management.Automation.Language.Parser]::ParseFile($Driver.path, [ref]$Tokens, [ref]$ParseErrors)
    if ($ParseErrors.Count -ne 0) { throw 'Driver parse errors' }
}
$AuditRoot = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev2-package-audit'
JECheck "$AuditRoot/AuditPackage.py" 'B4F8B3108BE95185B984B21CF8F13ECAF2662FD398A349D3A961A3E1C13C7CA6'
JECheck "$AuditRoot/archive-audit-before-extraction.json" 'A1DF5AB91FBCAC2E365EF82596184CB2566E390B6D4C0EDB2A1FD165D27715A3'
JECheck "$AuditRoot/patch-provenance-audit.json" 'DA747CFA3CAA7F9ED69C5E828AD072D66AD49BEBFDC467F3D33DE4DBBC0509F3'
JECheck "$AuditRoot/coff-static-library-audit.json" '0B1F2CFC9C2E5E3AC5017B838A8CEE64F4D69CA7493D952626A3C75BEF077115'
$Audit = [IO.File]::ReadAllText("$AuditRoot/archive-audit-before-extraction.json") | ConvertFrom-Json
JECheck $Audit.archive '01086CD19A1D241B4B0504FE16142C80BF29AF329543E67FF773301EDAA95122'
if ($Audit.regularFiles -ne 27 -or $Audit.contentManifestEntries -ne 26 -or $Audit.links -ne 0 -or
    $Audit.caseCollisions -ne 0 -or -not $Audit.manifestCoverageComplete -or -not $Audit.payloadByteExact) { throw 'Archive audit coverage differs' }
$SdkRoot = $Assignment.SDKRoot.Replace('\\', '/')
$ActualSdk = @(Get-ChildItem -LiteralPath $SdkRoot -File -Recurse)
if ($ActualSdk.Count -ne 27 -or $Audit.entries.Count -ne 27) { throw 'SDK membership differs' }
foreach ($Entry in $Audit.entries) {
    $Path = Join-Path $SdkRoot $Entry.path
    JENoReparse $Path
    JECheck $Path $Entry.sha256
    if ([IO.FileInfo]::new($Path).Length -ne $Entry.bytes) { throw 'SDK size differs' }
}
$Coff = [IO.File]::ReadAllText("$AuditRoot/coff-static-library-audit.json") | ConvertFrom-Json
if ($Coff.machine -cne 'AMD64' -or $Coff.objectMembers.Count -ne 4 -or $Coff.apiDefinitionsCount -ne 20 -or
    $Coff.dynamicImportObjects -ne 0 -or $Coff.pythonSymbolReferences -ne 0 -or $Coff.loadedOrExecuted) { throw 'COFF audit differs' }
JECheck (Join-Path $SdkRoot 'libsamplerate/lib/samplerate.lib') $Coff.librarySHA256
$Authority = [IO.File]::ReadAllText($Assignment.argumentAuthority) | ConvertFrom-Json
if ($Authority.consumer.arguments.Count -ne 11 -or $Authority.producer.arguments.Count -ne 5 -or $Authority.guards.Count -ne 8) { throw 'Argument authority coverage differs' }
foreach ($Output in @($Assignment.freshConsumerOutput, $Assignment.freshGuardOutput, "$PSScriptRoot/consumer-run", "$PSScriptRoot/guards-run")) {
    if (Test-Path -LiteralPath $Output) { throw "Output must be absent before first dispatch: $Output" }
}
$Receipt = [ordered]@{
    utc = [DateTime]::UtcNow.ToString('o')
    status = 'PARENT_STATIC_REVIEW_FOR_SINGLE_LUNA_DISPATCH_ONLY'
    assignmentSHA256 = JEHash $AssignmentPath
    frozenInputsMatched = 196
    extractedSdkFilesMatched = 27
    archiveSHA256 = JEHash $Audit.archive
    sourceAndDriversRead = $true
    apiDefinitions = 20
    casesPerConfiguration = 7
    configurations = @('Release', 'Debug')
    negativeGuards = 8
    oldOutputGuardLimit = 'Retained rev1 directory existence only'
    compatibilityLimit = 'No general mixed-CRT or legacy ABI acceptance'
    nativeExecutedByParent = $false
    engineEditorRuntimeAccepted = $false
}
$Path = Join-Path $PSScriptRoot 'parent-dispatch-review-rev1.json'
$Bytes = [Text.UTF8Encoding]::new($false).GetBytes(($Receipt | ConvertTo-Json -Depth 8) + "`n")
$Stream = [IO.File]::Open($Path, [IO.FileMode]::CreateNew)
try { $Stream.Write($Bytes, 0, $Bytes.Length) } finally { $Stream.Dispose() }
$Receipt | ConvertTo-Json -Depth 8
"receiptSHA256=$(JEHash $Path)"
