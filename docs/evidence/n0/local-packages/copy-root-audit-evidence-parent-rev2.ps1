param([string[]]$AdditionalFolders = @())
$ErrorActionPreference = 'Stop'
$taskRepo = 'F:\JericoEngine\JericoEngine'
$taskEvidenceRoot = Join-Path $taskRepo 'docs/evidence/n0'
$taskCopyCsv = Join-Path $taskEvidenceRoot 'local-packages/copy-manifest.csv'
$taskMaster = Join-Path $taskEvidenceRoot 'evidence-files.sha256'
$taskRows = [System.Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($row in Import-Csv -LiteralPath $taskCopyCsv) {
    $destination = if ([IO.Path]::IsPathRooted($row.destination)) { $row.destination } else { Join-Path $taskRepo $row.destination }
    if (-not (Test-Path -LiteralPath $destination -PathType Leaf) -or
        (Get-Item -LiteralPath $destination).Length -ne [long]$row.bytes -or
        (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -ne $row.sha256) {
        throw "Existing copied evidence changed: $($row.destination)"
    }
    $canonicalDestination = [IO.Path]::GetFullPath($destination)
    if (-not $canonicalDestination.StartsWith(($taskEvidenceRoot + '\'), [StringComparison]::OrdinalIgnoreCase)) { throw 'Existing evidence path escaped intended root' }
    $destinationKey = $canonicalDestination.Substring($taskRepo.Length + 1).Replace('\', '/')
    $taskRows.Add($destinationKey, $row)
}
$taskSources = [System.Collections.Generic.Dictionary[string,string]]::new([StringComparer]::OrdinalIgnoreCase)
function Add-TaskEvidenceFile([string]$sourceRelative, [string]$destinationRelative) {
    $sourceRelative = $sourceRelative.Replace([char]92, [char]47)
    $destinationRelative = $destinationRelative.Replace([char]92, [char]47)
    $source = [IO.Path]::GetFullPath((Join-Path $taskRepo $sourceRelative))
    $destination = [IO.Path]::GetFullPath((Join-Path $taskRepo $destinationRelative))
    if (-not $source.StartsWith(($taskRepo + '\'), [StringComparison]::OrdinalIgnoreCase) -or
        -not $destination.StartsWith(($taskEvidenceRoot + '\'), [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Evidence copy escaped the intended workspace'
    }
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw "Missing evidence: $sourceRelative" }
    if ((Get-Item -LiteralPath $source).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Evidence source is a reparse point' }
    if ($taskSources.ContainsKey($destinationRelative)) {
        if ($taskSources[$destinationRelative] -ne $sourceRelative) { throw 'Evidence destination collision' }
    } else { $taskSources.Add($destinationRelative, $sourceRelative) }
}
$taskExtensions = @('.json', '.log', '.txt', '.ps1', '.yaml', '.patch', '.md', '.cmake', '.h', '.c', '.cpp', '.cmd', '.exitcode')
$taskExcludedDirectories = '\\(extracted|consumer|CMakeFiles|source-work|source|source-extraction|stage|packages|fixtures?|[^\\]*-root|[^\\]*-build|Debug|Release)\\'
foreach ($name in @('expat-audit', 'tiff-audit', 'png-audit', 'd3dx12-audit', 'antlr4-audit',
        'expat-rev1-validation', 'tiff-rev1-validation', 'png-rev1-validation', 'd3dx12-rev1-validation', 'openexr-audit', 'openexr-rev1-validation', 'openexr-rev2-validation', 'root-antlr-preparation', 'root-antlr-openexr-final')) {
    $relativeBase = "build/n0-validation/local-packages/$name"
    $base = Join-Path $taskRepo $relativeBase
    foreach ($file in Get-ChildItem -LiteralPath $base -File -Recurse -Force) {
        $relative = $file.FullName.Substring($base.Length).TrimStart('\')
        if ($taskExtensions -notcontains $file.Extension.ToLowerInvariant() -or
            ('\' + $relative) -match $taskExcludedDirectories -or $file.Name -in @('CMakeCache.txt', 'CMakeLists.txt')) { continue }
        $tail = $relative.Replace('\', '/')
        Add-TaskEvidenceFile "$relativeBase/$tail" "docs/evidence/n0/local-packages/$name/$tail"
    }
}
$taskRootAttempt = 'build/n0-validation/local-packages/original-engine-native-packages-configure/attempt1'
foreach ($file in Get-ChildItem -LiteralPath (Join-Path $taskRepo $taskRootAttempt) -File -Force) {
    if ($file.Extension.ToLowerInvariant() -notin @('.json', '.log')) { continue }
    Add-TaskEvidenceFile "$taskRootAttempt/$($file.Name)" "docs/evidence/n0/local-packages/original-engine-native-packages-configure/attempt1/$($file.Name)"
}
foreach ($name in @('root-native-packages-assignment.json', 'source-freeze-root-native-packages.json',
        'root-native-packages-parent-preflight.json', 'prepare-root-native-packages-parent.ps1', 'root-openexr-copy-manifest.json', 'openexr-parent-integration.json', 'root-expat-copy-manifest.json', 'expat-parent-integration.json', 'root-png-copy-manifest.json', 'png-parent-integration.json', 'root-d3dx12-copy-manifest.json', 'd3dx12-parent-integration.json', 'copy-additional-openexr-root-evidence-parent.ps1')) {
    Add-TaskEvidenceFile "build/n0-validation/local-packages/$name" "docs/evidence/n0/local-packages/$name"
}
Add-TaskEvidenceFile 'build/n0-validation/local-packages/original-engine-native-packages-configure/run-root-configure-native-packages.ps1' 'docs/evidence/n0/local-packages/original-engine-native-packages-configure/run-root-configure-native-packages.ps1'
Add-TaskEvidenceFile 'build/n0-validation/local-packages/mcpp-audit/source-freeze-root-current17.json' 'docs/evidence/n0/local-packages/mcpp-audit/source-freeze-root-current17.json'
Add-TaskEvidenceFile 'build/n0-validation/local-packages/copy-verified-native-evidence-parent.ps1' 'docs/evidence/n0/local-packages/copy-verified-native-evidence-parent.ps1'
foreach ($file in Get-ChildItem -LiteralPath (Join-Path $taskRepo 'build/n0-validation/local-packages/openexr-atomics-diagnostic') -File -Force) {
    if ($file.Extension.ToLowerInvariant() -notin @('.json', '.log', '.txt', '.ps1', '.exitcode', '.vcxproj')) { continue }
    Add-TaskEvidenceFile "build/n0-validation/local-packages/openexr-atomics-diagnostic/$($file.Name)" "docs/evidence/n0/local-packages/openexr-atomics-diagnostic/$($file.Name)"
}
foreach ($name in @('CMakeLists.txt', 'atomic_probe.c')) {
    Add-TaskEvidenceFile "build/n0-validation/local-packages/openexr-atomics-diagnostic/project/$name" "docs/evidence/n0/local-packages/openexr-atomics-diagnostic/project/$name"
}
foreach ($name in @('donor-provenance.json', 'blueprint-reference-metadata.json',
        'donor-JERICOEDITOR_MASTER.md', 'donor-JERICOEDITOR_VISUAL_DESIGN.md',
        'source-freeze-ui-design-docs.json', 'doc-change-receipt.json', 'owned-documents.diff')) {
    Add-TaskEvidenceFile "build/n0-validation/ui-design-audit/$name" "docs/evidence/n0/ui-design-audit/$name"
}
foreach ($taskFolder in $AdditionalFolders) {
    if ($taskFolder -notmatch '^[a-z0-9][a-z0-9-]*$') { throw 'Invalid audit folder name' }
    $taskRelativeBase = "build/n0-validation/local-packages/$taskFolder"
    $taskAbsoluteBase = Join-Path $taskRepo $taskRelativeBase
    if (-not (Test-Path -LiteralPath $taskAbsoluteBase -PathType Container) -or
        ((Get-Item -LiteralPath $taskAbsoluteBase).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Audit folder missing or is a reparse point' }
    foreach ($taskFile in Get-ChildItem -LiteralPath $taskAbsoluteBase -File -Recurse -Force) {
        $taskTail = $taskFile.FullName.Substring($taskAbsoluteBase.Length).TrimStart('\')
        if ($taskFile.Name -eq 'CMakeCache.txt' -or $taskFile.Name -eq 'CMakeLists.txt') { continue }
        if (('\' + $taskTail) -match '\\(extracted|consumer|CMakeFiles|source-work|source|source-extraction|stage|packages|fixtures?|[^\\]*-root|[^\\]*-build|Debug|Release)\\') { continue }
        if ($taskFile.Extension -notmatch '^\.(json|log|txt|ps1|yaml|patch|md|cmake|h|c|cpp|cmd|exitcode)$') { continue }
        Add-TaskEvidenceFile "$taskRelativeBase/$taskTail" "docs/evidence/n0/local-packages/$taskFolder/$taskTail"
    }
}
Add-TaskEvidenceFile 'build/n0-validation/local-packages/copy-root-audit-evidence-parent-rev2.ps1' 'docs/evidence/n0/local-packages/copy-root-audit-evidence-parent-rev2.ps1'
$taskNewCount = 0
$taskNewBytes = 0L
foreach ($entry in $taskSources.GetEnumerator() | Sort-Object Key) {
    $source = Join-Path $taskRepo $entry.Value
    $destination = Join-Path $taskRepo $entry.Key
    $sourceFile = Get-Item -LiteralPath $source
    $sourceHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
    if (Test-Path -LiteralPath $destination) {
        if (-not (Test-Path -LiteralPath $destination -PathType Leaf) -or
            (Get-Item -LiteralPath $destination).Length -ne $sourceFile.Length -or
            (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -ne $sourceHash) {
            throw "Would overwrite changed evidence: $($entry.Key)"
        }
    } else {
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination)) | Out-Null
        [IO.File]::Copy($source, $destination, $false)
        $taskNewCount++
        $taskNewBytes += $sourceFile.Length
    }
    if ((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -ne $sourceHash) { throw 'Copied bytes differ' }
    if (-not $taskRows.ContainsKey($entry.Key)) {
        $taskRows.Add($entry.Key, [pscustomobject]@{source=$entry.Value;destination=$entry.Key;bytes=$sourceFile.Length;sha256=$sourceHash})
    }
}
$taskRows.Values | Sort-Object destination | Export-Csv -LiteralPath $taskCopyCsv -NoTypeInformation -Encoding utf8
$taskHashLines = foreach ($file in Get-ChildItem -LiteralPath $taskEvidenceRoot -File -Recurse -Force | Sort-Object FullName) {
    if ($file.FullName -eq $taskMaster) { continue }
    $relative = $file.FullName.Substring($taskEvidenceRoot.Length).TrimStart('\').Replace('\', '/')
    '{0} *{1}' -f (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant(), $relative
}
[IO.File]::WriteAllLines($taskMaster, [string[]]$taskHashLines, [Text.UTF8Encoding]::new($false))
[ordered]@{newFiles=$taskNewCount;newBytes=$taskNewBytes;copyRows=$taskRows.Count;hashEntries=$taskHashLines.Count;copyCsvSHA256=(Get-FileHash -LiteralPath $taskCopyCsv -Algorithm SHA256).Hash;masterSHA256=(Get-FileHash -LiteralPath $taskMaster -Algorithm SHA256).Hash} | ConvertTo-Json -Compress
