# Parent-only evidence publication. Preparation does not execute this script.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$taskRepo = 'F:\JericoEngine\JericoEngine'
$taskSourceRoot = Join-Path $taskRepo 'build/n0-validation/local-packages'
$taskEvidenceRoot = Join-Path $taskRepo 'docs/evidence/n0/local-packages'
$taskSavedEvidenceRoot = Join-Path $taskRepo 'docs/evidence/n0'
$taskCopyCsv = Join-Path $taskEvidenceRoot 'copy-manifest.csv'
$taskReference = Join-Path $taskSourceRoot 'copy-root-audit-evidence-parent-rev4.ps1'
$taskSources = [Collections.Generic.Dictionary[string,string]]::new([StringComparer]::OrdinalIgnoreCase)
$taskRows = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$taskSkippedDirectories = [Collections.Generic.List[string]]::new()
function Get-TaskEvidenceHash([string]$path) {
    $stream = [IO.File]::OpenRead($path)
    try { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream)) }
    finally { $stream.Dispose() }
}
function Assert-TaskEvidencePath([string]$path,[string]$root) {
    $full = [IO.Path]::GetFullPath($path)
    $root = [IO.Path]::GetFullPath($root).TrimEnd([char]92)
    if (-not $full.StartsWith($root + [char]92,[StringComparison]::OrdinalIgnoreCase)) { throw 'Evidence path escaped intended root' }
    $cursor = $full
    while ($cursor.Length -ge $root.Length) {
        if (([IO.File]::Exists($cursor) -or [IO.Directory]::Exists($cursor)) -and
            ([IO.File]::GetAttributes($cursor) -band [IO.FileAttributes]::ReparsePoint)) { throw "Reparse evidence path: $cursor" }
        $cursor = [IO.Path]::GetDirectoryName($cursor)
        if (-not $cursor) { break }
    }
    return $full
}
if ($PSVersionTable.PSEdition -cne 'Core' -or $PSVersionTable.PSVersion.ToString() -cne '7.6.5') { throw 'Use the audited Core7.6.5 launcher' }
if ((Get-TaskEvidenceHash ([Diagnostics.Process]::GetCurrentProcess().MainModule.FileName)) -cne '362A356CE7F0940EC74F73A8FC2C990A2CC24A38A11C90BBD8ECA947110AD139') { throw 'Core launcher changed' }
if ((Get-TaskEvidenceHash $taskReference) -cne '9CD82BFCA99034D238A83A004374A6311FBA3828C2B371A910103D7D94A9BA81') { throw 'Reference copier changed' }
foreach ($authority in @(
    @('libsamplerate-rev2-validation-preparation/parent-focal-acceptance-rev1.json','A6AAB1FA968FCC9FA4BB44056457F452E29E8F34BBFCFCD318CCA361ACECB316'),
    @('libsamplerate-parent-rev2/parent-package-integration.json','DBDFEF16691219C6A01B72896DB6BCEDD652F6F9AEA92D0DA33C5C8206C1DC57'))) {
    if ((Get-TaskEvidenceHash (Join-Path $taskSourceRoot $authority[0])) -cne $authority[1]) { throw 'Closed LibSamplerate authority changed' }
}
[void](Assert-TaskEvidencePath $taskCopyCsv $taskEvidenceRoot)
$taskCsvBefore = [IO.File]::ReadAllBytes($taskCopyCsv)
$taskCsvBeforeHash = Get-TaskEvidenceHash $taskCopyCsv
# Verify every existing CSV destination, preserving its original row and hash.
# This reads saved evidence only, never any source folder belonging to OIIO.
foreach ($row in Import-Csv -LiteralPath $taskCopyCsv) {
    $destination = if ([IO.Path]::IsPathRooted($row.destination)) { $row.destination } else { Join-Path $taskRepo $row.destination }
    $full = Assert-TaskEvidencePath $destination $taskSavedEvidenceRoot
    if (-not [IO.File]::Exists($full) -or [IO.FileInfo]::new($full).Length -ne [long]$row.bytes -or
        (Get-TaskEvidenceHash $full) -ne $row.sha256) { throw "Existing copied evidence changed: $($row.destination)" }
    $key = $full.Substring($taskRepo.Length + 1).Replace([char]92,[char]47)
    $taskRows.Add($key,$row)
}
$taskOldRowCount = $taskRows.Count
function Add-TaskEvidenceFile([string]$sourceRelative) {
    $sourceRelative = $sourceRelative.Replace([char]92,[char]47)
    $source = Assert-TaskEvidencePath (Join-Path $taskSourceRoot $sourceRelative) $taskSourceRoot
    $destinationRelative = 'docs/evidence/n0/local-packages/' + $sourceRelative
    [void](Assert-TaskEvidencePath (Join-Path $taskRepo $destinationRelative) $taskEvidenceRoot)
    if (-not [IO.File]::Exists($source)) { throw "Missing evidence: $sourceRelative" }
    if ($taskSources.ContainsKey($destinationRelative)) {
        if ($taskSources[$destinationRelative] -cne $source) { throw 'Evidence destination collision' }
    } else { $taskSources.Add($destinationRelative,$source) }
}
# Fixed closed scopes only: no caller-supplied folders, OIIO or root traversal.
$taskFolders = @(
    'libsamplerate-rev1-audit','libsamplerate-rev1-preparation',
    'libsamplerate-rev1-package-audit','libsamplerate-rev1-validation-preparation',
    'libsamplerate-rev2-audit','libsamplerate-rev2-audit-v2','libsamplerate-rev2-preparation',
    'libsamplerate-rev2-package-audit','libsamplerate-rev2-validation-preparation',
    'libsamplerate-rev2-guards-remaining-preparation','libsamplerate-parent-rev2')
$taskExtensions = @('.json','.log','.txt','.ps1','.py','.yaml','.patch','.diff','.md','.cmake','.cmd','.exitcode','.xml','.sha256')
$taskExcludedNames = @('snapshot','extracted','source-work','source','source-extraction','stage','packages',
    'fixtures','fixture','CMakeFiles','CMakeScratch','Testing','Debug','Release','RelWithDebInfo',
    'MinSizeRel','build','install','consumer','toolchain','tools','vendor','third-party','preserved-provider-red')
foreach ($folder in $taskFolders) {
    $sourceBase = Join-Path $taskSourceRoot $folder
    [void](Assert-TaskEvidencePath $sourceBase $taskSourceRoot)
    if (-not [IO.Directory]::Exists($sourceBase)) { throw "Closed audit directory missing: $folder" }
    $stack = [Collections.Generic.Stack[string]]::new()
    $stack.Push($sourceBase)
    while ($stack.Count) {
        $directory = $stack.Pop()
        foreach ($item in Get-ChildItem -LiteralPath $directory -Force) {
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Reparse closed evidence: $($item.FullName)" }
            if ($item.PSIsContainer) {
                if ($item.Name -in $taskExcludedNames -or $item.Name -match '^build-|^install-|(-root|-build|-consumer)$') {
                    $taskSkippedDirectories.Add($item.FullName); continue
                }
                $stack.Push($item.FullName); continue
            }
            if ($item.Name -in @('CMakeCache.txt','CMakeLists.txt') -or $item.Extension.ToLowerInvariant() -notin $taskExtensions) { continue }
            $tail = $item.FullName.Substring($sourceBase.Length + 1).Replace([char]92,[char]47)
            Add-TaskEvidenceFile "$folder/$tail"
        }
    }
}
# No traversal of producer build/toolchain/source/package payload trees.
foreach ($revision in @(1,2)) {
    $folder = "libsamplerate-rev$revision-build"
    foreach ($name in @('configure-argv.txt','configure-result.txt','configure-stdout.log','configure-stderr.log',
        'build-Release-argv.txt','build-Release-result.txt','build-Release-stdout.log','build-Release-stderr.log')) {
        Add-TaskEvidenceFile "$folder/$name"
    }
    foreach ($suffix in @('.files','.tar.xz.SHA256SUMS')) {
        Add-TaskEvidenceFile "$folder/packages/libsamplerate-0.2.1-je-rev$revision-windows$suffix"
    }
}
# Only the six preserved provider sources owned by this writer; not upstream code.
$taskRedReview = Get-Content -Raw -LiteralPath (Join-Path $taskSourceRoot 'libsamplerate-rev1-validation-preparation/parent-consumer-red-review-rev1.json') | ConvertFrom-Json
$taskRedNames = @('BuildLibSampleratePackage.cmake','Findlibsamplerate.cmake','NOTICE.txt','README.md','tests/CMakeLists.txt','tests/LibSamplerateConsumer.cpp')
if ($taskRedReview.preservedSource.Count -ne 6) { throw 'Expected six preserved provider sources' }
foreach ($name in $taskRedNames) {
    $relative = "libsamplerate-rev1-validation-preparation/preserved-provider-red/$name"
    $full = Assert-TaskEvidencePath (Join-Path $taskSourceRoot $relative) $taskSourceRoot
    $reference = @($taskRedReview.preservedSource | Where-Object { [IO.Path]::GetFullPath($_.copy) -ceq $full })
    if ($reference.Count -ne 1 -or [IO.FileInfo]::new($full).Length -ne $reference[0].bytes -or
        (Get-TaskEvidenceHash $full) -cne $reference[0].sha256) { throw 'Preserved provider source changed' }
    Add-TaskEvidenceFile $relative
}
# Complete preflight before writing any saved evidence or CSV bytes.
$taskPlan = foreach ($entry in $taskSources.GetEnumerator() | Sort-Object Key) {
    $destination = Join-Path $taskRepo $entry.Key
    [void](Assert-TaskEvidencePath $destination $taskEvidenceRoot)
    $file = [IO.FileInfo]::new($entry.Value)
    $hash = Get-TaskEvidenceHash $entry.Value
    if ([IO.File]::Exists($destination) -and ([IO.FileInfo]::new($destination).Length -ne $file.Length -or
        (Get-TaskEvidenceHash $destination) -cne $hash)) { throw "Would overwrite changed evidence: $($entry.Key)" }
    if ($taskRows.ContainsKey($entry.Key) -and ([long]$taskRows[$entry.Key].bytes -ne $file.Length -or
        $taskRows[$entry.Key].sha256 -ne $hash)) { throw 'Existing CSV row disagrees with planned evidence' }
    [pscustomobject]@{source=$entry.Value;destination=$entry.Key;bytes=$file.Length;sha256=$hash}
}
$newFiles = 0; $newBytes = 0L
$taskNewRows = [Collections.Generic.List[object]]::new()
foreach ($entry in $taskPlan) {
    $destination = Join-Path $taskRepo $entry.destination
    [void](Assert-TaskEvidencePath $destination $taskEvidenceRoot)
    if ([IO.FileInfo]::new($entry.source).Length -ne $entry.bytes -or (Get-TaskEvidenceHash $entry.source) -cne $entry.sha256) { throw 'Source drift before copy' }
    if (-not [IO.File]::Exists($destination)) {
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination)) | Out-Null
        [void](Assert-TaskEvidencePath $destination $taskEvidenceRoot)
        [IO.File]::Copy($entry.source,$destination,$false)
        $newFiles++; $newBytes += $entry.bytes
    }
    if ((Get-TaskEvidenceHash $entry.source) -cne $entry.sha256 -or [IO.FileInfo]::new($destination).Length -ne $entry.bytes -or
        (Get-TaskEvidenceHash $destination) -cne $entry.sha256) { throw 'Source drift/copied bytes differ' }
    if (-not $taskRows.ContainsKey($entry.destination)) { $taskNewRows.Add($entry); $taskRows.Add($entry.destination,$entry) }
}
if ((Get-TaskEvidenceHash $taskCopyCsv) -cne $taskCsvBeforeHash) { throw 'Copy CSV changed concurrently; not modified by this script' }
# Append only new rows: every prior CSV byte, row order and hash remain exact.
if ($taskNewRows.Count) {
    $csvLines = @($taskNewRows.ToArray() | ConvertTo-Csv -NoTypeInformation)
    if ($csvLines[0] -cne '"source","destination","bytes","sha256"') { throw 'CSV column contract differs' }
    $separator = if ($taskCsvBefore.Length -and $taskCsvBefore[$taskCsvBefore.Length-1] -ne 10) { "`r`n" } else { '' }
    $appendBytes = [Text.UTF8Encoding]::new($false).GetBytes($separator + (($csvLines | Select-Object -Skip 1) -join "`r`n") + "`r`n")
    $stream = [IO.File]::Open($taskCopyCsv,[IO.FileMode]::Append,[IO.FileAccess]::Write,[IO.FileShare]::None)
    try { $stream.Write($appendBytes,0,$appendBytes.Length) } finally { $stream.Dispose() }
    $after = [IO.File]::ReadAllBytes($taskCopyCsv)
    if ($after.Length -ne $taskCsvBefore.Length + $appendBytes.Length) { throw 'CSV append length differs' }
    for ($i=0;$i -lt $taskCsvBefore.Length;$i++) { if ($after[$i] -ne $taskCsvBefore[$i]) { throw 'Prior CSV byte changed' } }
    for ($i=0;$i -lt $appendBytes.Length;$i++) { if ($after[$taskCsvBefore.Length+$i] -ne $appendBytes[$i]) { throw 'CSV appended byte differs' } }
}
[ordered]@{scope='Closed LibSamplerate evidence only';plannedFiles=@($taskPlan).Count;newFiles=$newFiles;newBytes=$newBytes;
    priorCopyRows=$taskOldRowCount;newCopyRows=$taskNewRows.Count;copyRows=$taskRows.Count;previousCsvPrefixByteExact=$true;
    copyCsvBeforeSHA256=$taskCsvBeforeHash;copyCsvSHA256=(Get-TaskEvidenceHash $taskCopyCsv);
    skippedBeforeTraversal=$taskSkippedDirectories.Count;packageMetadataPairs=2;preservedProviderSources=6;
    OIIOTraversed=$false;globalEvidenceRootTraversed=$false;globalMasterRefresh='Parent final proof step; this copier does not traverse/rewrite the global master'} | ConvertTo-Json -Compress
