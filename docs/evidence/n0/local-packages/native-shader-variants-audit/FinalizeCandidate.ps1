$ErrorActionPreference = 'Stop'
$repo = 'F:\JericoEngine\JericoEngine'
$audit = Join-Path $repo 'build\n0-validation\local-packages\native-shader-variants-audit'
$utf8 = [Text.UTF8Encoding]::new($false)
$before = Get-Content (Join-Path $audit 'before-inputs.json') -Raw | ConvertFrom-Json
$paths = @(
 'Gems/Atom/Tools/ShaderManagementConsole/Code/Source/Window/ShaderManagementConsoleWindow.h',
 'Gems/Atom/Tools/ShaderManagementConsole/Code/Source/Window/ShaderManagementConsoleWindow.cpp',
 'Gems/Atom/Tools/ShaderManagementConsole/Code/Source/Data/ShaderVariantListGenerator.h',
 'Gems/Atom/Tools/ShaderManagementConsole/Code/Source/Data/ShaderVariantListGenerator.cpp',
 'Gems/Atom/Tools/ShaderManagementConsole/Code/shadermanagementconsole_files.cmake',
 'Gems/Atom/Tools/ShaderManagementConsole/Code/CMakeLists.txt',
 'Gems/Atom/Tools/ShaderManagementConsole/Code/Tests/ShaderVariantListGeneratorTests.cpp',
 'Gems/Atom/Tools/ShaderManagementConsole/Code/Tests/shadervariantgenerator_tests_files.cmake'
)
$diff = [Collections.Generic.List[string]]::new()
$checks = @()
$entries = foreach ($p in $paths) {
 $current = Join-Path $repo $p
 $original = Join-Path (Join-Path $audit 'before') $p
 $oldLines = @(); if (Test-Path -LiteralPath $original) { $oldLines = [IO.File]::ReadAllLines($original) }
 $newLines = [IO.File]::ReadAllLines($current)
 $ops = [Collections.Generic.List[object]]::new()
 $oldIndex = 0; $newIndex = 0
 foreach ($line in $newLines) {
  if ($oldIndex -lt $oldLines.Count -and [string]::Equals($line,$oldLines[$oldIndex],[StringComparison]::Ordinal)) {
   $ops.Add([pscustomobject]@{kind=' ';text=$line;old=$oldIndex+1;new=$newIndex+1}); $oldIndex++
  } else { $ops.Add([pscustomobject]@{kind='+';text=$line;old=$oldIndex+1;new=$newIndex+1}) }
  $newIndex++
 }
 if ($oldIndex -ne $oldLines.Count) { throw "Original lines changed or removed beyond this addition-only scope: $p" }
 $added = @($ops | Where-Object kind -eq '+').Count
 $diff.Add("--- a/$p"); $diff.Add("+++ b/$p")
 $cursor = 0
 while ($cursor -lt $ops.Count) {
  while ($cursor -lt $ops.Count -and $ops[$cursor].kind -eq ' ') { $cursor++ }
  if ($cursor -eq $ops.Count) { break }
  $start = [Math]::Max(0,$cursor-3); $lastAdd = $cursor; $end = [Math]::Min($ops.Count-1,$cursor+3)
  for ($scan=$cursor+1; $scan -lt $ops.Count; $scan++) {
   if ($ops[$scan].kind -eq '+') { $lastAdd=$scan; $end=[Math]::Min($ops.Count-1,$scan+3) }
   elseif ($scan -gt $lastAdd+6) { break }
  }
  $oldCount=0; for($k=$start;$k -le $end;$k++){ if($ops[$k].kind -eq ' '){$oldCount++} }
  $oldStart=$ops[$start].old; if($oldCount -eq 0){$oldStart--}
  $diff.Add("@@ -$oldStart,$oldCount +$($ops[$start].new),$($end-$start+1) @@")
  for($k=$start;$k -le $end;$k++){ $diff.Add($ops[$k].kind+$ops[$k].text) }
  $cursor=$end+1
 }
 $oldText=''; if(Test-Path -LiteralPath $original){$oldText=[IO.File]::ReadAllText($original)}
 $newText=[IO.File]::ReadAllText($current)
 $checks += [pscustomobject]@{path=$p;existingOriginal=(Test-Path -LiteralPath $original);originalLinesPreservedInOrder=$true;addedLines=$added;removedLines=0;oldCRLF=([regex]::Matches($oldText,"`r`n")).Count;oldBareLF=([regex]::Matches($oldText,"(?<!`r)`n")).Count;newCRLF=([regex]::Matches($newText,"`r`n")).Count;newBareLF=([regex]::Matches($newText,"(?<!`r)`n")).Count}
 [pscustomobject]@{path=$p;sha256=(Get-FileHash -LiteralPath $current -Algorithm SHA256).Hash;bytes=(Get-Item -LiteralPath $current).Length}
}
$references = @(
 'Gems/Atom/Tools/ShaderManagementConsole/Code/Source/ShaderManagementConsoleRequestBus.h',
 'Gems/Atom/Tools/ShaderManagementConsole/Code/Source/ShaderManagementConsoleApplication.cpp',
 'Gems/Atom/Tools/ShaderManagementConsole/Code/Source/Document/ShaderManagementConsoleDocument.cpp',
 'Gems/Atom/Tools/ShaderManagementConsole/Code/Source/Document/ShaderManagementConsoleDocumentRequestBus.h',
 'Gems/Atom/Tools/ShaderManagementConsole/Scripts/GenerateShaderVariantListUtil.py',
 'Gems/Atom/RPI/Code/Include/Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h',
 'Gems/Atom/RPI/Code/Include/Atom/RPI.Reflect/Shader/ShaderOptionGroup.h',
 'Gems/Atom/RPI/Code/Source/RPI.Reflect/Shader/ShaderVariantAsset.cpp',
 'Gems/Atom/RPI/Code/Include/Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h',
 'Gems/Atom/Tools/AtomToolsFramework/Code/Source/Document/AtomToolsDocumentSystem.cpp'
) | ForEach-Object { $p=$_;[pscustomobject]@{path=$p;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $p)).Hash;bytes=(Get-Item -LiteralPath (Join-Path $repo $p)).Length} }
[IO.File]::WriteAllText((Join-Path $audit 'source-diff.patch'),($diff -join "`n")+"`n",$utf8)
$freeze=[ordered]@{scope='Native single selected shader variant list';checkout=$repo;status='SOURCE_FROZEN_NOT_COMPILED';files=@($entries);referenceInputs=@($references);nativeValidationOwner='Luna only after parent review'}
[IO.File]::WriteAllText((Join-Path $audit 'source-freeze-native-shader-variants.json'),($freeze|ConvertTo-Json -Depth 8),$utf8)
$receipt=[ordered]@{
 scope='Single selected .shader native action; no batch/combinatorial dialog/NewProject/NewComponent port';
 changedSourceCount=8;regressionCount=13;regressions='RED_NOT_RUN';build='NOT_RUN';ctest='NOT_RUN';gui='NOT_RUN';saveUndoReopen='NOT_RUN; existing real document APIs retained, not credited by algorithm tests';
 sourceChecks=$checks;
 integration='GetSourceAssetInfo/FindMaterialAssetsUsingShader/GetMaterialInstanceShaderItems; MaterialOwnsShaderOption; ShaderOptionGroup/ShaderVariantListSourceData; CreateDocumentFromFilePath/BeginEdit/Set/EndEdit/DestroyDocument';
 sourceContractCorrections=@('Hint union across all materials excluding system keys','Dedup after material-owned projection and system overrides','Empty system expansion starts from actual Name default','Expansion skips groups missing that option instead of cloning duplicate rows','17-bit shader product stable-ID overflow rejected before expansion');
 untouched=@('Historical Python scripts','Existing compaction-action connection defect','Document and public buses','B38 freeze histories','Engine/Python/Multiplayer composition','Other writer/provider sources');
 limitations=@('No executed behavioral RED or GREEN','No native material asset/GUI/Undo/Save acceptance','Dependency-query bus returns vector without explicit database-failure status; recorded errors abort, native database behavior still pending','Stable IDs are reproducible for a fixed sorted material input and descriptor order; regeneration with changed variants is not an incremental stable-ID preservation scheme','Project-path runtime and asynchronous asset rebuild behavior remain native integration gates');
 freezeSha256=(Get-FileHash (Join-Path $audit 'source-freeze-native-shader-variants.json')).Hash;
 diffSha256=(Get-FileHash (Join-Path $audit 'source-diff.patch')).Hash
}
[IO.File]::WriteAllText((Join-Path $audit 'candidate-receipt.json'),($receipt|ConvertTo-Json -Depth 8),$utf8)
$entries | Format-Table -AutoSize
Get-FileHash (Join-Path $audit 'source-freeze-native-shader-variants.json'),(Join-Path $audit 'source-diff.patch'),(Join-Path $audit 'candidate-receipt.json') | Format-Table -AutoSize
