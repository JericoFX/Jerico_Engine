$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$engine='F:/JericoEngine/JericoEngine'
$base="$engine/build/n0-validation/local-packages"
$provider="$engine/cmake/Tools/LocalPackages/LibSamplerate"
$history="$base/libsamplerate-rev1-validation-preparation/preserved-provider-red"
$audit="$base/libsamplerate-rev2-audit-v2"
if(Test-Path -LiteralPath $audit){throw 'New audit already exists'}
[IO.Directory]::CreateDirectory($audit)|Out-Null
$utf8=[Text.UTF8Encoding]::new($false)
function WriteNew([string]$path,[string]$text){$stream=[IO.File]::Open($path,[IO.FileMode]::CreateNew);try{$data=$utf8.GetBytes($text);$stream.Write($data,0,$data.Length)}finally{$stream.Dispose()}}
function WriteJson([string]$path,$value){WriteNew $path (($value|ConvertTo-Json -Depth 24)+"`n")}
function Hash([string]$path){return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash}
function Entry([string]$path,[string]$role){return [pscustomobject]@{path=$path.Replace('\','/');bytes=([IO.FileInfo]::new($path)).Length;sha256=(Hash $path);role=$role}}
function JEUnifiedDiff([string]$name,[string]$old,[string]$new){
 $a=[string[]]@();if($old.Length){$a=[regex]::Split($old.TrimEnd("`n"),"`n")};$b=[string[]]@();if($new.Length){$b=[regex]::Split($new.TrimEnd("`n"),"`n")}
 $matrix=[int[,]]::new(($a.Length+1),($b.Length+1))
 for($i=$a.Length-1;$i -ge 0;$i--){for($j=$b.Length-1;$j -ge 0;$j--){if($a[$i] -ceq $b[$j]){$matrix[$i,$j]=$matrix[($i+1),($j+1)]+1}else{$matrix[$i,$j]=[Math]::Max($matrix[($i+1),$j],$matrix[$i,($j+1)])}}}
 $ops=[Collections.Generic.List[object]]::new();$i=0;$j=0
 while($i -lt $a.Length -or $j -lt $b.Length){
  if($i -lt $a.Length -and $j -lt $b.Length -and $a[$i] -ceq $b[$j]){$ops.Add(@{kind=' ';text=$a[$i];old=($i+1);new=($j+1)});$i++;$j++}
  elseif($i -lt $a.Length -and ($j -ge $b.Length -or $matrix[($i+1),$j] -ge $matrix[$i,($j+1)])){$ops.Add(@{kind='-';text=$a[$i];old=($i+1);new=($j+1)});$i++}
  else{$ops.Add(@{kind='+';text=$b[$j];old=($i+1);new=($j+1)});$j++}
 }
 $ranges=[Collections.Generic.List[object]]::new()
 for($k=0;$k -lt $ops.Count;$k++){if($ops[$k].kind -cne ' '){$start=[Math]::Max(0,$k-3);$end=[Math]::Min($ops.Count-1,$k+3);if($ranges.Count -and $start -le $ranges[$ranges.Count-1].end+1){$ranges[$ranges.Count-1].end=$end}else{$ranges.Add(@{start=$start;end=$end})}}}
 if(-not $ranges.Count){return ''}
 $out=[Text.StringBuilder]::new();[void]$out.AppendLine("--- a/$name");[void]$out.AppendLine("+++ b/$name")
 foreach($range in $ranges){$slice=@($ops.GetRange($range.start,$range.end-$range.start+1));$oldCount=@($slice|Where-Object kind -cne '+').Count;$newCount=@($slice|Where-Object kind -cne '-').Count;[void]$out.AppendLine("@@ -$($slice[0].old),$oldCount +$($slice[0].new),$newCount @@");foreach($op in $slice){[void]$out.AppendLine($op.kind+$op.text)}}
 return $out.ToString().Replace("`r`n","`n")
}
$previousFreeze="$base/libsamplerate-rev1-audit/source-freeze-libsamplerate-rev1.json"
$prior=Get-Content -LiteralPath $previousFreeze -Raw|ConvertFrom-Json
if((Hash $previousFreeze) -cne '56C34D9835C18C444EFA77F4B2042B1A31D938E49E0BF74948CB7DCA8550A986'){throw 'Prior source authority changed'}
$redPath="$base/libsamplerate-rev1-validation-preparation/parent-consumer-red-review-rev1.json"
if((Hash $redPath) -cne '64C0C13DFD4D00BE190A52927D430D3B01E8B2CB7C7CF150AF246F0AD5858C77'){throw 'RED authority changed'}
$sourceRecords=@();$diff=[Text.StringBuilder]::new();$historyRecords=@();$candidate=@()
foreach($f in Get-ChildItem -LiteralPath $provider -Recurse -File | Sort-Object FullName){
 $relative=$f.FullName.Replace('\','/').Substring($provider.Length+1);$oldFile="$history/$relative";$old='';$oldHash=$null;$oldBytes=0
 if([IO.File]::Exists($oldFile)){$old=[IO.File]::ReadAllText($oldFile);$oldHash=Hash $oldFile;$oldBytes=([IO.FileInfo]::new($oldFile)).Length;$priorEntry=@($prior.entries|Where-Object path -ceq "$provider/$relative");if($priorEntry.Count -ne 1 -or $oldHash -cne $priorEntry[0].sha256 -or $oldBytes -ne $priorEntry[0].bytes){throw 'Preserved original changed'};$historyRecords+=Entry $oldFile 'preserved-rev1-candidate'}
 $new=[IO.File]::ReadAllText($f.FullName);[void]$diff.Append((JEUnifiedDiff "cmake/Tools/LocalPackages/LibSamplerate/$relative" $old $new))
 $candidate+=Entry $f.FullName 'candidate-owned'
 $sourceRecords+=@{path=$relative;oldSHA256=$oldHash;oldBytes=$oldBytes;newSHA256=(Hash $f.FullName);newBytes=$f.Length;status=$(if(-not $oldHash){'ADDED'}elseif($oldHash -ceq (Hash $f.FullName)){'UNCHANGED'}else{'CHANGED'})}
}
$originalSource='F:/JericoEngine/Dependencies/libsamplerate-a147b6dc4046deda22b2aa772b348717de772e2b/src/samplerate.c'
$original=[IO.File]::ReadAllText($originalSource)
$applicator=[IO.File]::ReadAllText("$provider/ApplySamplerateChannelGuard.cmake")
$oldHunk=[regex]::Match($applicator,'set\(JE_PATCH_BEFORE \[=\[(.*?)\]=\]\)',[Text.RegularExpressions.RegexOptions]::Singleline).Groups[1].Value
$newHunk=[regex]::Match($applicator,'set\(JE_PATCH_AFTER \[=\[(.*?)\]=\]\)',[Text.RegularExpressions.RegexOptions]::Singleline).Groups[1].Value
if(-not $oldHunk.Length -or -not $newHunk.Length -or $original.Split($oldHunk,[StringSplitOptions]::None).Length -ne 2){throw 'Hunk not unique'}
$modified=$original.Replace($oldHunk,$newHunk);$modifiedBytes=$utf8.GetBytes($modified);$modifiedHash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($modifiedBytes))
$patchMetadata=Get-Content -LiteralPath "$provider/SamplerateChannelGuard.json" -Raw|ConvertFrom-Json
if((Hash $originalSource).ToLowerInvariant() -cne $patchMetadata.originalSHA256 -or $modifiedHash.ToLowerInvariant() -cne $patchMetadata.modifiedSHA256 -or $modifiedBytes.Length -ne $patchMetadata.modifiedBytes -or $modified.Contains("`r")){throw 'Data-only patch byte check differs'}
# Reconstruct the exact unified hunk as data; no CMake or acquired source execution.
$oldLines=$oldHunk.Split("`n");$newLines=$newHunk.Split("`n")
$expectedPatch="--- a/src/samplerate.c`n+++ b/src/samplerate.c`n@@ -25,6 +25,12 @@`n src_new (int converter_type, int channels, int *error)`n {`n"
foreach($line in $newLines[2..7]){$expectedPatch+="+$line`n"}
$expectedPatch+=" `treturn psrc_set_converter (converter_type, channels, error) ;`n } /* src_new */`n `n SRC_STATE*`n"
if([IO.File]::ReadAllText("$provider/SamplerateChannelGuard.patch") -cne $expectedPatch){throw 'Patch document differs from applicator'}
$oldConsumer=[IO.File]::ReadAllText("$history/tests/LibSamplerateConsumer.cpp")
$newConsumer=[IO.File]::ReadAllText("$provider/tests/LibSamplerateConsumer.cpp")
$positiveExact=$oldConsumer.Substring(0,$oldConsumer.IndexOf('    void Errors()')) -ceq $newConsumer.Substring(0,$newConsumer.IndexOf('    long EmptyCallback'))
$tailExact=$oldConsumer.Substring($oldConsumer.IndexOf('        Require(src_delete(nullptr)')) -ceq $newConsumer.Substring($newConsumer.IndexOf('        Require(src_delete(nullptr)'))
if(-not $positiveExact -or -not $tailExact){throw 'Existing positive/errors-tail oracles changed'}
$unchangedPrior=@();foreach($e in $prior.entries|Where-Object role -cne 'candidate-owned'){if((Hash $e.path) -cne $e.sha256 -or ([IO.FileInfo]::new($e.path)).Length -ne $e.bytes){throw "Prior immutable/shared input changed: $($e.path)"};$unchangedPrior+=$e}
$diffPath="$audit/source-diff-rev2.patch";WriteNew $diffPath $diff.ToString()
$proofPath="$audit/candidate-byte-proof-rev2.json"
WriteJson $proofPath ([ordered]@{utc=[DateTime]::UtcNow.ToString('o');status='STATIC_DATA_CHECK_ONLY; NATIVE_NOT_RUN';revision='rev2';genuineRED=$redPath;genuineREDSHA256=(Hash $redPath);previousInputsUnchanged=$unchangedPrior.Count;preservedOriginals=$historyRecords.Count;candidateFiles=$candidate.Count;changed=@($sourceRecords|Where-Object status -ceq 'CHANGED').Count;added=@($sourceRecords|Where-Object status -ceq 'ADDED').Count;unchanged=@($sourceRecords|Where-Object status -ceq 'UNCHANGED').Count;files=$sourceRecords;originalSource=$originalSource;originalSHA256=(Hash $originalSource);modifiedSHA256ComputedInMemory=$modifiedHash;modifiedBytes=$modifiedBytes.Length;uniqueHunk=$true;exactPatchEqualsApplicator=$true;immutableSourceWritten=$false;positiveCaseOraclesByteExact=$positiveExact;existingErrorsTailAndDispatchByteExact=$tailExact;sourceDiff=$diffPath;sourceDiffSHA256=(Hash $diffPath);nativeExecuted=$false;scope='One src_new private-work guard; all five converters/version/API/CRT unchanged; consumer error oracle strengthened'})
$oldArgsPath="$base/libsamplerate-rev1-audit/validation-arguments-rev1.json"
$argumentAuthority=Get-Content -LiteralPath $oldArgsPath -Raw|ConvertFrom-Json
$argumentAuthority.status='PREPARED_NOT_RUN_REV2'
$argumentAuthority.producer.arguments[2]='-DJE_SAMPLERATE_OUTPUT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev2-build'
$argumentAuthority.consumer.arguments[3]='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev2-consumer'
$argumentAuthority.consumer.arguments[10]='-DJE_SAMPLERATE_PACKAGE_ROOT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev2-package-audit/extracted/libsamplerate-0.2.1-je-rev2-windows'
foreach($guard in $argumentAuthority.guards){if($guard.name -ne 'old-output'){$guard.value=$guard.value.Replace('libsamplerate-rev1-guards','libsamplerate-rev2-guards')}}
$argumentAuthority.guardPrerequisites='After retained rev1 producer output exists. New rev2 guard-owned invalid local bytes; old-output oracle only existing directory, not a complete historical build manifest. Consumer/guards require separate assignment after fresh rev2 archive audit.'
$argsPath="$audit/validation-arguments-rev2.json";WriteJson $argsPath $argumentAuthority
$entries=@($candidate)+@($unchangedPrior)+@($historyRecords)
foreach($p in @($previousFreeze,$oldArgsPath,$redPath,$proofPath,$diffPath,$argsPath,$PSCommandPath)){$entries+=Entry $p 'revision-authority-readonly'}
$freezePath="$audit/source-freeze-libsamplerate-rev2.json"
WriteJson $freezePath ([ordered]@{status='FROZEN_FOR_PARENT_REVIEW; PRODUCER_CONSUMER_GUARDS_NOT_RUN';count=$entries.Count;candidateFiles=$candidate.Count;immutableSourceFiles=99;derivesFrom=$previousFreeze;derivesFromSHA256=(Hash $previousFreeze);historicalRed=$redPath;historicalRedSHA256=(Hash $redPath);entries=$entries})
[ordered]@{sourceFreeze=$freezePath;sourceFreezeSHA256=(Hash $freezePath);inputs=$entries.Count;candidateFiles=$candidate.Count;proof=$proofPath;proofSHA256=(Hash $proofPath);diff=$diffPath;diffSHA256=(Hash $diffPath);arguments=$argsPath;argumentsSHA256=(Hash $argsPath)}|ConvertTo-Json
