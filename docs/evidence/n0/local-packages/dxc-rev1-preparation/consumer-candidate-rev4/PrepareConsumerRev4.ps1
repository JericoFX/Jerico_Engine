#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = [IO.Path]::GetFullPath($PSScriptRoot)
$prep = Split-Path $root -Parent
if($root -ne 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\dxc-rev1-preparation\consumer-candidate-rev4'){throw 'Assigned candidate output differs'}
$encoding = [Text.UTF8Encoding]::new($false)
function Hash([string] $path) { $stream=[IO.File]::OpenRead($path);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()} }
function Entry([string] $path,[string] $role) { [ordered]@{path=[IO.Path]::GetFullPath($path);bytes=([IO.FileInfo]::new($path)).Length;sha256=(Hash $path);role=$role} }
function Write-New([string] $path,[string] $text) {if(Test-Path -LiteralPath $path){throw "Retain existing evidence: $path"};[IO.File]::WriteAllText($path,$text,$encoding)}
$oldPath=Join-Path $prep 'consumer-candidate-rev3/source-freeze-dxc-consumer-rev3.json'
if((Hash $oldPath) -ne 'B06DB30C0DCDEC4EF685C8606C0A8FD19F54D1B1E967C90DA35DB4842750B909'){throw 'Approved diagnostic freeze differs'}
$old=Get-Content -LiteralPath $oldPath -Raw|ConvertFrom-Json
if($old.files.Count -ne 23379){throw 'Historical count differs'}
foreach($file in $old.files){if(([IO.FileInfo]::new($file.path)).Length -ne $file.bytes -or (Hash $file.path) -ne $file.sha256){throw "Unexpected preflight delta: $($file.path)"}}
$cpp='F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/DirectXShaderCompilerDxc/tests/DirectXShaderCompilerDxcConsumer.cpp'
if((Hash $cpp) -ne '4ED3A3DCFBDDC5E3E8E4006B449E5BE8F77E5C81DECF9F9C39F3DB14623E1407'){throw 'Consumer diagnostic source differs'}
$beforeBytes=[IO.File]::ReadAllBytes($cpp)
$before=$encoding.GetString($beforeBytes)
$from='            Require(FAILED(status) && errors.Get() != nullptr && errors->GetStringLength() > 0 && !result->HasOutput(DXC_OUT_OBJECT), "malformed shader rejected with diagnostics and no object");'
$to=@'
            Hr(objectHr, "invalid shader object query");
            Hr(resultHr, "invalid shader result query");
            // DXC registers the object slot on failure; an empty blob emits no bytecode.
            Require(FAILED(status) && errors.Get() != nullptr && errors->GetStringLength() > 0
                && (diagnosticObject.Get() == nullptr || diagnosticObject->GetBufferSize() == 0)
                && (diagnosticResult.Get() == nullptr || diagnosticResult->GetBufferSize() == 0),
                "malformed shader rejected with diagnostics and no object bytes");
            Require(!std::filesystem::exists(output), "malformed shader created no output path");
'@
$to=$to.Replace("`r`n","`n")
if([regex]::Matches($before,[regex]::Escape($from)).Count -ne 1){throw 'Unique old invalid oracle absent'}
$after=$before.Replace($from,$to)
if($after.Replace($to,$from) -cne $before){throw 'Exact inverse differs'}
$beforePath=Join-Path $root 'DirectXShaderCompilerDxcConsumer.rev3.before.cpp'
if(Test-Path -LiteralPath $beforePath){throw 'Before evidence exists'}
[IO.File]::WriteAllBytes($beforePath,$beforeBytes)
[IO.File]::WriteAllText($cpp,$after,$encoding)
$line=$before.Substring(0,$before.IndexOf($from)).Split("`n").Count
$relative='cmake/Tools/LocalPackages/DirectXShaderCompilerDxc/tests/DirectXShaderCompilerDxcConsumer.cpp'
$lines=$to.Split("`n")
$diff="diff --git a/$relative b/$relative`n--- a/$relative`n+++ b/$relative`n@@ -$line,1 +$line,$($lines.Count) @@`n-$from`n"+(($lines|ForEach-Object{"+$_"}) -join "`n")+"`n"
$diffPath=Join-Path $root 'source-diff-rev3-to-rev4.patch'
Write-New $diffPath $diff
$assignment=Get-Content -LiteralPath (Join-Path $prep 'consumer-candidate-rev3/validation-assignment.json') -Raw|ConvertFrom-Json -AsHashtable
$assignment.status='CORRECTED INVALID OUTPUT-BYTES ORACLE; ALL FOUR CASES PENDING LUNA'
$assignment.consumerRevision=4
$assignment.correctedOracle='Failed status, successful diagnostic/object/result queries, nonempty diagnostics, no object or result bytes, and absent invalid output path; registered output slot is not emitted bytecode'
$assignment.diagnosticQueries='All diagnostic prints preserved; require errorsHr/objectHr/resultHr success after printing observed values'
foreach($command in @('consumerConfigure','consumerBuild','consumerCTest')){$assignment[$command].argv=@($assignment[$command].argv|ForEach-Object{$_.Replace('dxc-native-consumer-rev3','dxc-native-consumer-rev4')})}
if(Test-Path -LiteralPath 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/dxc-native-consumer-rev4'){throw 'Fresh consumer build exists'}
Write-New (Join-Path $root 'validation-assignment.json') (($assignment|ConvertTo-Json -Depth 16)+"`n")
$receipt=[ordered]@{status='SOURCE ORACLE CORRECTION ONLY; NATIVE PENDING';before=(Entry $beforePath 'exact diagnostic rev3 before bytes');after=(Entry $cpp 'one authorized invalid assertion replacement');exactInverse=$true;diagnosticsUnchanged=$true;positiveOraclesUnchanged=$true;producerChanged=$false;packageChanged=$false;parentReviewedNativeValues=@{status='0x80004005';errorsHRESULT='S_OK';errorsLength=502;hasOutput=1;objectHRESULT='S_OK';resultHRESULT='S_OK';sameBlob=$true;objectBytes=0;resultBytes=0};sourceContract=@('dxcapi.impl.h:428-454 HasOutput records kind, GetOutput returns registered object independent of operation status','dxcompilerobj.cpp:649-660 registers initial output memory stream blob;1305-1315 stores object before setting failed operation status');preserved='API1.9, PE/full40pin/hash8/count4954/ownership, all diagnostic prints, retail hash, validator, SPIRV and real dxsc';nativeExecuted=$false;acceptance=$false}
Write-New (Join-Path $root 'source-change-receipt.json') (($receipt|ConvertTo-Json -Depth 16)+"`n")
$inputs=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($file in $old.files){$path=[IO.Path]::GetFullPath($file.path);if($path -eq [IO.Path]::GetFullPath($cpp)){$inputs.Add($path,(Entry $cpp 'invalid-only emitted-bytes oracle correction'))}else{if(([IO.FileInfo]::new($path)).Length -ne $file.bytes -or (Hash $path) -ne $file.sha256){throw "Unexpected source delta: $path"};$inputs.Add($path,$file)}}
foreach($path in (@($oldPath)+@(Get-ChildItem -LiteralPath $root -File|ForEach-Object FullName))){$full=[IO.Path]::GetFullPath($path);if(-not $inputs.ContainsKey($full)){$inputs.Add($full,(Entry $full 'historical / exact corrected candidate evidence'))}}
$files=@($inputs.Values|Sort-Object path)
foreach($file in $files){if(([IO.FileInfo]::new($file.path)).Length -ne $file.bytes -or (Hash $file.path) -ne $file.sha256){throw "Final delta: $($file.path)"}}
$freezePath=Join-Path $root 'source-freeze-dxc-consumer-rev4.json'
$barePath=Join-Path $root 'source-freeze-dxc-consumer-rev4-bare.json'
Write-New $freezePath (([ordered]@{status='CORRECTED INVALID ORACLE; FOUR CASES PENDING NATIVE';fileCount=$files.Count;approvedHistoricalInputs=23379;authorizedHistoricalDeltas=1;unchangedHistorical=23378;sourceChange=$receipt;nativeExecuted=$false;acceptance=$false;files=$files}|ConvertTo-Json -Depth 16)+"`n")
Write-New $barePath ((ConvertTo-Json -InputObject $files -Depth 12)+"`n")
$proof=[ordered]@{status='FROZEN FOR PARENT REVIEW';verified=$files.Count;historicalInputs=23379;deltas=1;unknownDeltas=0;source=(Entry $cpp 'single invalid oracle edit');diff=(Entry $diffPath 'actual exact inverse diff');assignment=(Entry (Join-Path $root 'validation-assignment.json') 'fresh rev4 all four cases');freeze=(Entry $freezePath 'complete corrected source freeze');bareFreeze=(Entry $barePath 'bare file list');nativeExecuted=$false;acceptance=$false}
Write-New (Join-Path $root 'candidate-proof.json') (($proof|ConvertTo-Json -Depth 16)+"`n")
$proof|ConvertTo-Json -Depth 16
