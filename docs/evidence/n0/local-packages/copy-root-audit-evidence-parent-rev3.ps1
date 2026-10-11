param([string[]]$AdditionalFolders = @())
$ErrorActionPreference = 'Stop'
$taskRepo = 'F:\JericoEngine\JericoEngine'
$taskEvidenceRoot = Join-Path $taskRepo 'docs/evidence/n0'
$taskCopyCsv = Join-Path $taskEvidenceRoot 'local-packages/copy-manifest.csv'
$taskMaster = Join-Path $taskEvidenceRoot 'evidence-files.sha256'
$taskRows = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$taskSources = [Collections.Generic.Dictionary[string,string]]::new([StringComparer]::OrdinalIgnoreCase)
$taskSkippedDirectories = [Collections.Generic.List[string]]::new()
function Get-TaskEvidenceHash([string]$path) {
    $stream = [IO.File]::OpenRead($path)
    try { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream)) }
    finally { $stream.Dispose() }
}
function Assert-TaskEvidencePath([string]$path,[string]$root) {
    $full = [IO.Path]::GetFullPath($path)
    if (-not $full.StartsWith([IO.Path]::GetFullPath($root).TrimEnd([char]92) + [char]92,[StringComparison]::OrdinalIgnoreCase)) { throw 'Evidence path escaped intended root' }
    $cursor = $full
    while ($cursor.Length -ge $root.Length) {
        if ([IO.File]::Exists($cursor) -or [IO.Directory]::Exists($cursor)) {
            if ([IO.File]::GetAttributes($cursor) -band [IO.FileAttributes]::ReparsePoint) { throw "Reparse evidence path: $cursor" }
        }
        $cursor = [IO.Path]::GetDirectoryName($cursor)
        if (-not $cursor) { break }
    }
    return $full
}
foreach ($row in Import-Csv -LiteralPath $taskCopyCsv) {
    $destination = if ([IO.Path]::IsPathRooted($row.destination)) { $row.destination } else { Join-Path $taskRepo $row.destination }
    $full = Assert-TaskEvidencePath $destination $taskEvidenceRoot
    $file = [IO.FileInfo]::new($full)
    if (-not $file.Exists -or $file.Length -ne [long]$row.bytes -or (Get-TaskEvidenceHash $full) -ne $row.sha256) { throw "Existing copied evidence changed: $($row.destination)" }
    $key = $full.Substring($taskRepo.Length + 1).Replace([char]92,[char]47)
    $taskRows.Add($key,$row)
}
function Add-TaskEvidenceFile([string]$sourceRelative,[string]$destinationRelative) {
    $sourceRelative = $sourceRelative.Replace([char]92,[char]47)
    $destinationRelative = $destinationRelative.Replace([char]92,[char]47)
    $source = Assert-TaskEvidencePath (Join-Path $taskRepo $sourceRelative) $taskRepo
    [void](Assert-TaskEvidencePath (Join-Path $taskRepo $destinationRelative) $taskEvidenceRoot)
    if (-not [IO.File]::Exists($source)) { throw "Missing evidence: $sourceRelative" }
    if ($taskSources.ContainsKey($destinationRelative)) {
        if ($taskSources[$destinationRelative] -ne $sourceRelative) { throw 'Evidence destination collision' }
    } else { $taskSources.Add($destinationRelative,$sourceRelative) }
}
$taskExtensions = @('.json','.log','.txt','.ps1','.yaml','.patch','.diff','.md','.cmake','.h','.c','.cpp','.cmd','.exitcode','.xml','.sha256','.sha256sums','.files','.before')
$taskExcludedNames = @('snapshot','extracted','source-work','source','source-extraction','stage','packages','fixtures','fixture','CMakeFiles','CMakeScratch','Testing','Debug','Release','RelWithDebInfo','MinSizeRel','build','install','consumer')
foreach ($folder in $AdditionalFolders) {
    if ($folder -notmatch '^[a-z0-9][a-z0-9-]*$') { throw 'Invalid audit folder name' }
    $sourceBase = Join-Path $taskRepo "build/n0-validation/local-packages/$folder"
    [void](Assert-TaskEvidencePath $sourceBase $taskRepo)
    if (-not [IO.Directory]::Exists($sourceBase)) { throw "Audit directory missing: $folder" }
    $stack = [Collections.Generic.Stack[string]]::new()
    $stack.Push($sourceBase)
    while ($stack.Count) {
        $directory = $stack.Pop()
        foreach ($item in Get-ChildItem -LiteralPath $directory -Force) {
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { $taskSkippedDirectories.Add($item.FullName); continue }
            if ($item.PSIsContainer) {
                if ($item.Name -in $taskExcludedNames -or $item.Name -match '^build-|^install-|(-root|-build)$') { $taskSkippedDirectories.Add($item.FullName); continue }
                $stack.Push($item.FullName)
                continue
            }
            if ($item.Name -in @('CMakeCache.txt','CMakeLists.txt') -or $item.Extension.ToLowerInvariant() -notin $taskExtensions) { continue }
            $tail = $item.FullName.Substring($sourceBase.Length + 1).Replace([char]92,[char]47)
            Add-TaskEvidenceFile "build/n0-validation/local-packages/$folder/$tail" "docs/evidence/n0/local-packages/$folder/$tail"
        }
    }
}
foreach ($name in @('vulkan-validationlayers-1.3.261-je-rev1-windows.files','vulkan-validationlayers-1.3.261-je-rev1-windows.tar.xz.SHA256SUMS')) {
    Add-TaskEvidenceFile "build/n0-validation/local-packages/vvl-local-rev1/packages/$name" "docs/evidence/n0/local-packages/vvl-local-rev1/packages/$name"
}
Add-TaskEvidenceFile 'build/n0-validation/local-packages/vvl-rev4-consumer/build/Testing/Temporary/LastTest.log' 'docs/evidence/n0/local-packages/vvl-rev4-validation/attempt1/LastTest.log'
$taskNegativeAssignment = Get-Content -Raw -LiteralPath (Join-Path $taskRepo 'build/n0-validation/local-packages/vvl-rev1-negatives/negative-assignment-rev1.json') | ConvertFrom-Json
if ($taskNegativeAssignment.fixtures.Count -ne 5) { throw 'Expected five exact negative fixtures' }
foreach ($entry in $taskNegativeAssignment.fixtures) {
    $taskFixture = Assert-TaskEvidencePath $entry.path (Join-Path $taskRepo 'build/n0-validation/local-packages/vvl-rev1-negatives')
    if ([IO.FileInfo]::new($taskFixture).Length -ne $entry.bytes -or (Get-TaskEvidenceHash $taskFixture) -ne $entry.sha256) { throw 'Prepared negative fixture drift' }
    $taskSourceRelative = $taskFixture.Substring($taskRepo.Length + 1).Replace([char]92,[char]47)
    $taskDestinationRelative = $taskSourceRelative.Replace('build/n0-validation/local-packages/','docs/evidence/n0/local-packages/')
    Add-TaskEvidenceFile $taskSourceRelative $taskDestinationRelative
}
Add-TaskEvidenceFile 'build/n0-validation/local-packages/copy-root-audit-evidence-parent-rev3.ps1' 'docs/evidence/n0/local-packages/copy-root-audit-evidence-parent-rev3.ps1'
$newFiles=0;$newBytes=0L
foreach ($entry in $taskSources.GetEnumerator() | Sort-Object Key) {
    $source = Join-Path $taskRepo $entry.Value
    $destination = Join-Path $taskRepo $entry.Key
    $file = [IO.FileInfo]::new($source)
    $hash = Get-TaskEvidenceHash $source
    if ([IO.File]::Exists($destination)) {
        if ([IO.FileInfo]::new($destination).Length -ne $file.Length -or (Get-TaskEvidenceHash $destination) -ne $hash) { throw "Would overwrite changed evidence: $($entry.Key)" }
    } else {
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination)) | Out-Null
        [IO.File]::Copy($source,$destination,$false)
        $newFiles++;$newBytes+=$file.Length
    }
    if ((Get-TaskEvidenceHash $source) -ne $hash -or (Get-TaskEvidenceHash $destination) -ne $hash) { throw 'Source drift/copied bytes differ' }
    if (-not $taskRows.ContainsKey($entry.Key)) { $taskRows.Add($entry.Key,[pscustomobject]@{source=$entry.Value;destination=$entry.Key;bytes=$file.Length;sha256=$hash}) }
}
$taskRows.Values | Sort-Object destination | Export-Csv -LiteralPath $taskCopyCsv -NoTypeInformation -Encoding utf8
$masterFiles = [Collections.Generic.List[object]]::new()
$masterDirectories = [Collections.Generic.Stack[string]]::new()
$masterDirectories.Push($taskEvidenceRoot)
while ($masterDirectories.Count) {
    foreach ($item in Get-ChildItem -LiteralPath $masterDirectories.Pop() -Force) {
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Reparse point in saved evidence; traversal refused' }
        if ($item.PSIsContainer) { $masterDirectories.Push($item.FullName) } else { $masterFiles.Add($item) }
    }
}
$hashLines = foreach ($file in $masterFiles | Sort-Object FullName) {
    if ($file.FullName -eq $taskMaster) { continue }
    [void](Assert-TaskEvidencePath $file.FullName $taskEvidenceRoot)
    $relative = $file.FullName.Substring($taskEvidenceRoot.Length + 1).Replace([char]92,[char]47)
    '{0} *{1}' -f (Get-TaskEvidenceHash $file.FullName).ToLowerInvariant(),$relative
}
[IO.File]::WriteAllLines($taskMaster,[string[]]$hashLines,[Text.UTF8Encoding]::new($false))
[ordered]@{newFiles=$newFiles;newBytes=$newBytes;copyRows=$taskRows.Count;hashEntries=$hashLines.Count;skippedBeforeTraversal=$taskSkippedDirectories.Count;copyCsvSHA256=(Get-TaskEvidenceHash $taskCopyCsv);masterSHA256=(Get-TaskEvidenceHash $taskMaster)} | ConvertTo-Json -Compress
