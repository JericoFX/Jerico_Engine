#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = [IO.Path]::GetFullPath($PSScriptRoot)
$prep = Split-Path $root -Parent
if ($root -ne 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\dxc-rev1-preparation\consumer-candidate-rev3') { throw 'Assigned candidate output differs' }
$encoding = [Text.UTF8Encoding]::new($false)
function Write-New([string] $path,[string] $text) { if(Test-Path -LiteralPath $path){throw "Retain existing evidence: $path"}; [IO.File]::WriteAllText($path,$text,$encoding) }
function Entry([string] $path,[string] $role) { [ordered]@{path=[IO.Path]::GetFullPath($path);bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash;role=$role} }
$oldPath = Join-Path $prep 'consumer-candidate-rev2/source-freeze-dxc-consumer-rev2.json'
if((Get-FileHash -LiteralPath $oldPath).Hash -ne '4427A9E7947AC6999D9FFA36FA55704885CADBF4E4C0E745D4B89CE6ADC0F03B'){throw 'Approved rev2 freeze differs'}
$old = Get-Content -LiteralPath $oldPath -Raw | ConvertFrom-Json
if($old.files.Count -ne 23373){throw 'Approved source count differs'}
foreach($file in $old.files){if((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $file.path).Hash -ne $file.sha256){throw "Unexpected preflight delta: $($file.path)"}}
$cpp = 'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/DirectXShaderCompilerDxc/tests/DirectXShaderCompilerDxcConsumer.cpp'
if((Get-FileHash -LiteralPath $cpp).Hash -ne 'A09161480B2D8C90C1B9DA96A50DE1F3909C09CD7D6EA0554556212C95851C3A'){throw 'Consumer rev2 differs'}
$beforeBytes = [IO.File]::ReadAllBytes($cpp)
$before = $encoding.GetString($beforeBytes)
$from = '            Hr(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr), "invalid shader diagnostics");'
$to = @'
            const HRESULT errorsHr = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
            const BOOL hasObject = result->HasOutput(DXC_OUT_OBJECT);
            ComPtr<IDxcBlob> diagnosticObject;
            const HRESULT objectHr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&diagnosticObject), nullptr);
            ComPtr<IDxcBlob> diagnosticResult;
            const HRESULT resultHr = result->GetResult(&diagnosticResult);
            std::printf("INVALID OBSERVED: status=0x%08lx errorsHr=0x%08lx errorsLength=%zu hasObject=%u\n",
                static_cast<unsigned long>(status), static_cast<unsigned long>(errorsHr),
                errors.Get() != nullptr ? errors->GetStringLength() : size_t{0}, static_cast<unsigned>(hasObject));
            std::printf("INVALID OBJECT: hr=0x%08lx pointer=%p bytes=%zu\n", static_cast<unsigned long>(objectHr),
                static_cast<void*>(diagnosticObject.Get()), diagnosticObject.Get() != nullptr ? diagnosticObject->GetBufferSize() : size_t{0});
            std::printf("INVALID RESULT: hr=0x%08lx pointer=%p bytes=%zu\n", static_cast<unsigned long>(resultHr),
                static_cast<void*>(diagnosticResult.Get()), diagnosticResult.Get() != nullptr ? diagnosticResult->GetBufferSize() : size_t{0});
            Hr(errorsHr, "invalid shader diagnostics");
'@
$to = $to.Replace("`r`n","`n")
if([regex]::Matches($before,[regex]::Escape($from)).Count -ne 1){throw 'Unique invalid diagnostic call absent'}
$after = $before.Replace($from,$to)
if($after.Replace($to,$from) -cne $before){throw 'Diagnostic inverse differs'}
$assertion = '            Require(FAILED(status) && errors.Get() != nullptr && errors->GetStringLength() > 0 && !result->HasOutput(DXC_OUT_OBJECT), "malformed shader rejected with diagnostics and no object");'
if([regex]::Matches($after,[regex]::Escape($assertion)).Count -ne 1){throw 'Original failing assertion must remain exact'}
$beforePath = Join-Path $root 'DirectXShaderCompilerDxcConsumer.rev2.before.cpp'
if(Test-Path -LiteralPath $beforePath){throw 'Historical before evidence exists'}
[IO.File]::WriteAllBytes($beforePath,$beforeBytes)
[IO.File]::WriteAllText($cpp,$after,$encoding)
$line = $before.Substring(0,$before.IndexOf($from)).Split("`n").Count
$relative = 'cmake/Tools/LocalPackages/DirectXShaderCompilerDxc/tests/DirectXShaderCompilerDxcConsumer.cpp'
$newLines = $to.Split("`n")
$diff = "diff --git a/$relative b/$relative`n--- a/$relative`n+++ b/$relative`n@@ -$line,1 +$line,$($newLines.Count) @@`n-$from`n" + (($newLines|ForEach-Object{"+$_"}) -join "`n")+"`n"
$diffPath = Join-Path $root 'source-diff-rev2-to-rev3.patch'
Write-New $diffPath $diff
$assignment = Get-Content -LiteralPath (Join-Path $prep 'consumer-candidate-rev2/validation-assignment.json') -Raw | ConvertFrom-Json -AsHashtable
$assignment.status = 'DIAGNOSTIC CANDIDATE; ASSERTION UNCHANGED; PENDING LUNA NATIVE'
$assignment.consumerRevision = 3
$assignment.correctedOracle = 'No new oracle correction; retain failing invalid assertion exactly and report actual HRESULT/output-slot/blob-size values'
$assignment.diagnosticQueries = 'GetOutput(OBJECT) and GetResult HRESULT/pointer/size are reported only, never required to succeed'
foreach($command in @('consumerConfigure','consumerBuild','consumerCTest')){$assignment[$command].argv=@($assignment[$command].argv|ForEach-Object{$_.Replace('dxc-native-consumer-rev2','dxc-native-consumer-rev3')})}
if(Test-Path -LiteralPath 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/dxc-native-consumer-rev3'){throw 'Fresh diagnostic build path already exists'}
Write-New (Join-Path $root 'validation-assignment.json') (($assignment|ConvertTo-Json -Depth 16)+"`n")
$receipt = [ordered]@{status='INSTRUMENTATION ONLY; NO NATIVE EXECUTION';before=(Entry $beforePath 'exact before bytes');after=(Entry $cpp 'invalid diagnostic instrumentation only');exactInverse=$true;originalFailingAssertionUnchanged=$true;positiveOraclesUnchanged=$true;producerChanged=$false;packageChanged=$false;queriesMustSucceed=$false;diagnosticFields=@('operation status HRESULT','errors HRESULT / length','HasOutput OBJECT','GetOutput OBJECT HRESULT / pointer / byte size','GetResult HRESULT / pointer / byte size');nativeExecuted=$false;acceptance=$false}
Write-New (Join-Path $root 'source-change-receipt.json') (($receipt|ConvertTo-Json -Depth 16)+"`n")
$inputs = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($file in $old.files){
    $path=[IO.Path]::GetFullPath($file.path)
    if($path -eq [IO.Path]::GetFullPath($cpp)){$inputs.Add($path,(Entry $cpp 'one authorized invalid-only diagnostic insertion'))}
    else{if((Get-Item -LiteralPath $path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $path).Hash -ne $file.sha256){throw "Unexpected post-edit delta: $path"};$inputs.Add($path,$file)}
}
$paths = @($oldPath)+@(Get-ChildItem -LiteralPath $root -File|ForEach-Object FullName)
foreach($path in $paths){$full=[IO.Path]::GetFullPath($path);if(-not $inputs.ContainsKey($full)){$inputs.Add($full,(Entry $full 'immutable diagnostic candidate / historical provenance'))}}
$files=@($inputs.Values|Sort-Object path)
foreach($file in $files){if((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $file.path).Hash -ne $file.sha256){throw "Final freeze delta: $($file.path)"}}
$freezePath=Join-Path $root 'source-freeze-dxc-consumer-rev3.json'
$barePath=Join-Path $root 'source-freeze-dxc-consumer-rev3-bare.json'
Write-New $freezePath (([ordered]@{status='DIAGNOSTIC ONLY; ORIGINAL ORACLES PRESERVED; PENDING NATIVE';fileCount=$files.Count;approvedHistoricalInputs=23373;authorizedHistoricalDeltas=1;unchangedHistorical=23372;sourceChange=$receipt;nativeExecuted=$false;acceptance=$false;files=$files}|ConvertTo-Json -Depth 16)+"`n")
Write-New $barePath ((ConvertTo-Json -InputObject $files -Depth 12)+"`n")
$proof=[ordered]@{status='FROZEN FOR PARENT REVIEW; NO NATIVE';verified=$files.Count;historicalInputs=23373;deltas=1;unknownDeltas=0;source=(Entry $cpp 'single diagnostic source edit');diff=(Entry $diffPath 'actual exact inverse diff');assignment=(Entry (Join-Path $root 'validation-assignment.json') 'fresh rev3 consumer only');freeze=(Entry $freezePath 'diagnostic complete freeze');bareFreeze=(Entry $barePath 'bare file list');nativeExecuted=$false;acceptance=$false}
Write-New (Join-Path $root 'candidate-proof.json') (($proof|ConvertTo-Json -Depth 16)+"`n")
$proof|ConvertTo-Json -Depth 16
