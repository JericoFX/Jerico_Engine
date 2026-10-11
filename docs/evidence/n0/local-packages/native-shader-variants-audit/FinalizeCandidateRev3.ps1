$ErrorActionPreference='Stop'
$repo='F:\JericoEngine\JericoEngine'
$audit=Join-Path $repo 'build\n0-validation\local-packages\native-shader-variants-audit'
$utf8=[Text.UTF8Encoding]::new($false)
$previous=Get-Content (Join-Path $audit 'source-freeze-native-shader-variants-rev2.json') -Raw|ConvertFrom-Json
$doc='Gems/Atom/Tools/ShaderManagementConsole/Code/Source/Document/ShaderManagementConsoleDocument.cpp'
$header='Gems/Atom/Tools/ShaderManagementConsole/Code/Source/Document/ShaderManagementConsoleDocument.h'
$paths=@($previous.files.path)
$diff=[Collections.Generic.List[string]]::new()
$checks=@();$beforeEntries=@()
$files=foreach($p in $paths){
 $current=Join-Path $repo $p; $original=Join-Path (Join-Path $audit 'before') $p
 $old=@(); if(Test-Path -LiteralPath $original){$old=[IO.File]::ReadAllLines($original);$beforeEntries+=[pscustomobject]@{path=$p;sha256=(Get-FileHash -LiteralPath $original).Hash;bytes=(Get-Item -LiteralPath $original).Length}}
 $new=[IO.File]::ReadAllLines($current)
 $lcs=[int[,]]::new($old.Count+1,$new.Count+1)
 for($i=$old.Count-1;$i -ge 0;$i--){for($j=$new.Count-1;$j -ge 0;$j--){if([string]::Equals($old[$i],$new[$j],[StringComparison]::Ordinal)){$lcs[$i,$j]=1+$lcs[($i+1),($j+1)]}else{$lcs[$i,$j]=[Math]::Max($lcs[($i+1),$j],$lcs[$i,($j+1)])}}}
 $ops=[Collections.Generic.List[object]]::new();$i=0;$j=0
 while($i -lt $old.Count -or $j -lt $new.Count){
  if($i -lt $old.Count -and $j -lt $new.Count -and [string]::Equals($old[$i],$new[$j],[StringComparison]::Ordinal)){$ops.Add([pscustomobject]@{kind=' ';text=$old[$i];old=$i+1;new=$j+1});$i++;$j++}
  elseif($j -eq $new.Count -or ($i -lt $old.Count -and $lcs[($i+1),$j] -ge $lcs[$i,($j+1)])){$ops.Add([pscustomobject]@{kind='-';text=$old[$i];old=$i+1;new=$j+1});$i++}
  else{$ops.Add([pscustomobject]@{kind='+';text=$new[$j];old=$i+1;new=$j+1});$j++}
 }
 $diff.Add("--- a/$p");$diff.Add("+++ b/$p");$cursor=0
 while($cursor -lt $ops.Count){
  while($cursor -lt $ops.Count -and $ops[$cursor].kind -eq ' '){$cursor++};if($cursor -eq $ops.Count){break}
  $start=[Math]::Max(0,$cursor-3);$lastChange=$cursor;$end=[Math]::Min($ops.Count-1,$cursor+3)
  for($scan=$cursor+1;$scan -lt $ops.Count;$scan++){if($ops[$scan].kind -ne ' '){$lastChange=$scan;$end=[Math]::Min($ops.Count-1,$scan+3)}elseif($scan -gt $lastChange+6){break}}
  $oldCount=0;$newCount=0;for($k=$start;$k -le $end;$k++){if($ops[$k].kind -ne '+'){$oldCount++};if($ops[$k].kind -ne '-'){$newCount++}}
  $oldStart=$ops[$start].old;$newStart=$ops[$start].new;if($oldCount -eq 0){$oldStart--};if($newCount -eq 0){$newStart--}
  $diff.Add("@@ -$oldStart,$oldCount +$newStart,$newCount @@");for($k=$start;$k -le $end;$k++){$diff.Add($ops[$k].kind+$ops[$k].text)};$cursor=$end+1
 }
 $oldText='';if(Test-Path -LiteralPath $original){$oldText=[IO.File]::ReadAllText($original)};$newText=[IO.File]::ReadAllText($current)
 $checks+=[pscustomobject]@{path=$p;addedLines=@($ops|Where-Object kind -eq '+').Count;removedLines=@($ops|Where-Object kind -eq '-').Count;oldCRLF=([regex]::Matches($oldText,"`r`n")).Count;oldBareLF=([regex]::Matches($oldText,"(?<!`r)`n")).Count;newCRLF=([regex]::Matches($newText,"`r`n")).Count;newBareLF=([regex]::Matches($newText,"(?<!`r)`n")).Count}
 [pscustomobject]@{path=$p;sha256=(Get-FileHash -LiteralPath $current).Hash;bytes=(Get-Item -LiteralPath $current).Length}
}
$references=@($previous.referenceInputs|Where-Object path -ne $doc)
$additionalReferencePaths=@(
 'Gems/Atom/Tools/AtomToolsFramework/Code/Include/AtomToolsFramework/Util/Util.h',
 'Gems/Atom/Tools/AtomToolsFramework/Code/Source/Util/Util.cpp',
 'Gems/Atom/Tools/AtomToolsFramework/Code/Source/Document/AtomToolsDocument.cpp',
 'Code/Framework/AzCore/Platform/Windows/AzCore/AzCore_Traits_Windows.h',
 'Code/Framework/AzCore/AzCore/IO/Path/Path.inl'
)
$references+=@($additionalReferencePaths|ForEach-Object{$p=$_;[pscustomobject]@{path=$p;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $p)).Hash;bytes=(Get-Item -LiteralPath (Join-Path $repo $p)).Length}})
foreach($entry in $references){if((Get-FileHash -LiteralPath (Join-Path $repo $entry.path)).Hash -ne $entry.sha256){throw "Reference changed $($entry.path)"}}
$deltas=foreach($entry in $files){$old=$previous.files|Where-Object path -eq $entry.path; if($old -and $old.sha256 -ne $entry.sha256){[pscustomobject]@{path=$entry.path;beforeSha256=$old.sha256;afterSha256=$entry.sha256}}}
$freeze=[ordered]@{revision=3;checkout=$repo;scope='Native single selected shader variant list + exact snapshot document setter';status='SOURCE_FROZEN_NOT_COMPILED';files=@($files);referenceInputs=@($references);previousFreeze='source-freeze-native-shader-variants-rev2.json';previousFreezeSha256=(Get-FileHash (Join-Path $audit 'source-freeze-native-shader-variants-rev2.json')).Hash;nativeValidationOwner='Luna only after parent review'}
[IO.File]::WriteAllText((Join-Path $audit 'source-freeze-native-shader-variants-rev3.json'),($freeze|ConvertTo-Json -Depth 8),$utf8)
[IO.File]::WriteAllText((Join-Path $audit 'source-diff-rev3.patch'),($diff -join "`n")+"`n",$utf8)
$receipt=[ordered]@{
 revision=3;sourceCount=10;referenceCount=14;regressionCount=13;regressions='RED_NOT_RUN';build='NOT_RUN';ctest='NOT_RUN';gui='NOT_RUN';undoSaveReopen='NOT_RUN';
 changesFromRev2=@('Window shaderPath becomes mutable and passes ValidateDocumentPath before LoadAsset/Generate/factory, matching the exact document OS-normalized baseline path; no lax equality');
 changedExistingRev2=@($deltas);originalDocumentInputs=@($beforeEntries|Where-Object path -in @($doc,$header));sourceLineChecks=$checks;
 untouched='Public document buses and signatures, EndEdit, existing compact action defect, 13 generator tests, helper, historical scripts, other owners, rev1 receipts';
 limitations=@('No native behavior/compiler evidence','No executed regression against document zero-list Undo, Save/Reopen, window cancellation or source reload','Applies exactly the captured validated snapshot; does not claim filesystem/asset lock atomicity after capture','FindMaterialAssetsUsingShader still returns vector without explicit database-failure result; recorded errors abort','Batch generation and standalone combinatorial dialog remain pending');
 freezeSha256=(Get-FileHash (Join-Path $audit 'source-freeze-native-shader-variants-rev3.json')).Hash;diffSha256=(Get-FileHash (Join-Path $audit 'source-diff-rev3.patch')).Hash
}
[IO.File]::WriteAllText((Join-Path $audit 'candidate-receipt-rev3.json'),($receipt|ConvertTo-Json -Depth 9),$utf8)
[pscustomobject]@{sources=$files.Count;references=$references.Count;changedRev1=$deltas.Count;freezeSha=$receipt.freezeSha256;diffSha=$receipt.diffSha256;receiptSha=(Get-FileHash (Join-Path $audit 'candidate-receipt-rev3.json')).Hash}|ConvertTo-Json -Compress
