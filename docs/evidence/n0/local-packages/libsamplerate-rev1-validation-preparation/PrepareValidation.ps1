$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$engine='F:/JericoEngine/JericoEngine'
$base="$engine/build/n0-validation/local-packages"
$audit="$base/libsamplerate-rev1-package-audit"
$prep="$base/libsamplerate-rev1-validation-preparation"
$oldAudit="$base/libsamplerate-rev1-audit"
$sourceFreeze="$oldAudit/source-freeze-libsamplerate-rev1.json"
$arguments="$oldAudit/validation-arguments-rev1.json"
function Hash([string]$path){(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash}
function NewJson([string]$path,$obj){$s=[IO.File]::Open($path,[IO.FileMode]::CreateNew);try{$b=[Text.UTF8Encoding]::new($false).GetBytes(($obj|ConvertTo-Json -Depth 20)+"`n");$s.Write($b,0,$b.Length)}finally{$s.Dispose()}}
function Entry([string]$path,[string]$role){[ordered]@{path=$path.Replace('\','/');bytes=([IO.FileInfo]::new($path)).Length;sha256=Hash $path;role=$role}}
if((Hash $sourceFreeze) -cne '56C34D9835C18C444EFA77F4B2042B1A31D938E49E0BF74948CB7DCA8550A986'){throw 'Original source freeze changed'}
if((Hash $arguments) -cne 'B623FE1D877D8F83A22E9D5D124C13E3DDEF875AD0A9BDC0D06FD821093BC585'){throw 'Original argument authority changed'}
$reader='F:/JericoEngine/JericoEngine/build/n0-validation/original-engine-deps/Python-rev3/packages/python-3.10.13-je-rev3-windows/python/python.exe'
if((Hash $reader) -cne '6235ABE2248C14AD86DFE00DBB7BC2396EB4B91F663412C211905E279BCE9D6E'){throw 'Audited data reader changed'}
NewJson "$audit/audit-reader-receipt.json" ([ordered]@{status='EXTERNAL_STDLIB_DATA_AUDIT_EXIT_ZERO';receiptUTC=[DateTime]::UtcNow.ToString('o');executable=$reader;executableSHA256=Hash $reader;arguments=@('-I','-B',"$audit/AuditPackage.py");PYTHONDONTWRITEBYTECODE='1';exitCode=0;scope='Own tar/json/hash/path/struct COFF data reader only; no acquired code loading or execution';scriptSHA256=Hash "$audit/AuditPackage.py";stdoutSHA256=Hash "$audit/audit.stdout.raw.log";stderrBytes=([IO.FileInfo]::new("$audit/audit.stderr.raw.log")).Length;stderrSHA256=Hash "$audit/audit.stderr.raw.log";nativeBuildTestExecuted=$false})
$original=Get-Content -LiteralPath $sourceFreeze -Raw|ConvertFrom-Json
if($original.count -ne 136 -or $original.entries.Count -ne 136){throw 'Original count differs'}
$entries=@()
foreach($e in $original.entries){$actual=Entry $e.path $e.role;if($actual.sha256 -cne $e.sha256 -or $actual.bytes -ne $e.bytes){throw "Original input differs $($e.path)"};$entries+=$actual}
$sdk=Get-Content -LiteralPath "$audit/extracted-package-manifest.json" -Raw|ConvertFrom-Json
if($sdk.count -ne 24){throw 'Unexpected SDK count'}
foreach($e in $sdk.entries){$actual=Entry "$($sdk.root)/$($e.path)" 'audited-extracted-SDK';if($actual.sha256 -cne $e.sha256 -or $actual.bytes -ne $e.bytes){throw 'SDK bytes differ'};$entries+=$actual}
$archive="$base/libsamplerate-rev1-build/packages/libsamplerate-0.2.1-je-rev1-windows.tar.xz"
if((Hash $archive) -cne '4B202E429E792B7BA7CBE7B592A22CD4E1FEE5E84C47FA10E23B6C33AA5D3028'){throw 'Produced archive changed'}
foreach($path in @($archive,"$archive.SHA256SUMS")){$entries+=Entry $path 'produced-package-authority'}
foreach($name in @('archive-audit-before-extraction.json','extracted-package-manifest.json','coff-static-library-audit.json','package-audit-summary.json','audit-reader-receipt.json','AuditPackage.py')){$entries+=Entry "$audit/$name" 'independent-package-audit'}
foreach($path in @($sourceFreeze,$arguments,"$base/libsamplerate-rev1-preparation/producer-run/producer-result.json")){$entries+=Entry $path 'original-frozen-authority'}
foreach($name in @('CommonExecution.ps1','RunConsumer.ps1','RunGuards.ps1')){
 $path="$prep/$name";$tokens=$null;$errors=$null;[Management.Automation.Language.Parser]::ParseFile($path,[ref]$tokens,[ref]$errors)|Out-Null
 if($errors.Count){throw 'Driver parse failed'};$entries+=Entry $path 'validation-driver-source'
}
if(@($entries.path | Group-Object | Where-Object Count -gt 1).Count){throw 'Duplicate authority paths'}
$inputs="$prep/validation-inputs.json"
NewJson $inputs ([ordered]@{status='FROZEN_NOT_DISPATCHED';count=$entries.Count;originalInputCount=136;auditedSDKCount=24;driversIncludedInPrePost=3;entries=$entries})
$launcher='C:/Users/Jerico/.cache/codex-runtimes/codex-primary-runtime/dependencies/native/powershell/pwsh.exe'
NewJson "$prep/assignment.json" ([ordered]@{status='PREPARED_NOT_DISPATCHED';owner='Luna only after explicit parent review/dispatch';launcher=$launcher;launcherSHA256='362A356CE7F0940EC74F73A8FC2C990A2CC24A38A11C90BBD8ECA947110AD139';inputs=[ordered]@{path=$inputs;sha256=Hash $inputs;count=$entries.Count;original136Unchanged=$true;sdk24ByteExact=$true;driverSourceCount=3};consumer=[ordered]@{wrapperArguments=@('-NoLogo','-NoProfile','-File',"$prep/RunConsumer.ps1");scriptSHA256=Hash "$prep/RunConsumer.ps1";configurations=@('Release','Debug');cases=7;nativeConfigureArgumentAuthority=$arguments;authoritySHA256=Hash $arguments;freshBuild="$base/libsamplerate-rev1-consumer";sdkRoot=$sdk.root;evidence="$prep/consumer-run";JUnit=@('Release-junit.xml','Debug-junit.xml');CRT='Release static library /MD; consumer /MD or /MDd, all linker diagnostics preserved; no general CRT compatibility acceptance'};guards=[ordered]@{wrapperArguments=@('-NoLogo','-NoProfile','-File',"$prep/RunGuards.ps1");scriptSHA256=Hash "$prep/RunGuards.ps1";cases=8;fixtures="$base/libsamplerate-rev1-guards";evidence="$prep/guards-run";authority=$arguments;authoritySHA256=Hash $arguments;scope='nonzero native result+specific diagnostic+unchanged output existence; old-output existence only, not a complete private build manifest'};nativeExecuted=$false;automaticDispatch=$false;retry=$false;Root=$false;runtimeAccepted=$false})
NewJson "$prep/preparation-receipt.json" ([ordered]@{status='PREPARED_NOT_RUN';scripts=3;parseErrors=0;originalInputsMatched=136;sdkMatched=24;inputs=$entries.Count;archiveBytes=([IO.FileInfo]::new($archive)).Length;archiveSHA256=Hash $archive;consumerCasesPerConfiguration=7;guardCases=8;allRunDirectoriesAbsent=(-not(Test-Path "$prep/consumer-run") -and -not(Test-Path "$prep/guards-run"));freshConsumerBuildAbsent=(-not(Test-Path "$base/libsamplerate-rev1-consumer"));guardFixturesAbsent=(-not(Test-Path "$base/libsamplerate-rev1-guards"));noOriginalInputWritten=$true;dataAuditOnly=$true;nativeExecuted=$false})
$final=@($entries)
foreach($path in @($inputs,"$prep/assignment.json","$prep/preparation-receipt.json")){$final+=Entry $path 'validation-driver-authority'}
NewJson "$prep/driver-freeze.json" ([ordered]@{status='FROZEN_FOR_PARENT_REVIEW_NOT_DISPATCHED';count=$final.Count;validationInputs=$entries.Count;entries=$final})
foreach($path in @($inputs,"$prep/assignment.json","$prep/driver-freeze.json","$prep/preparation-receipt.json","$audit/archive-audit-before-extraction.json","$audit/coff-static-library-audit.json")){[pscustomobject]@{path=$path;sha256=Hash $path;bytes=([IO.FileInfo]::new($path)).Length}|ConvertTo-Json -Compress}