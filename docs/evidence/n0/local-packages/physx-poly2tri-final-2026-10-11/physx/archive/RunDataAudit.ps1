# Data reader only: no SDK, compiler, CMake, package executable or installation.
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$base=$PSScriptRoot
$python='F:/JericoEngine/JericoEngine/build/n0-validation/original-engine-deps/Python-rev3/packages/python-3.10.13-je-rev3-windows/python/python.exe'
$pin='6235ABE2248C14AD86DFE00DBB7BC2396EB4B91F663412C211905E279BCE9D6E'
function HashFile([string]$p){$s=[IO.File]::OpenRead($p);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($s))}finally{$s.Dispose()}}
function NewReceipt([string]$n,$v){$s=[IO.File]::Open((Join-Path $base $n),[IO.FileMode]::CreateNew);try{$b=[Text.UTF8Encoding]::new($false).GetBytes(($v|ConvertTo-Json -Depth 8)+"`n");$s.Write($b,0,$b.Length)}finally{$s.Dispose()}}
if((HashFile $python) -cne $pin){throw 'Audited external Python data reader changed'}
$freeze='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/physx-final-preparation-rev2/source-freeze-five.json'
$source=Get-Content -LiteralPath $freeze -Raw|ConvertFrom-Json
function CaptureSource([string]$n){$rows=@();foreach($e in $source.entries){$h=HashFile $e.path;$size=([IO.FileInfo]::new($e.path)).Length;$rows+=[ordered]@{path=$e.path;sha256=$h;bytes=$size;matches=($h -ceq $e.sha256 -and $size -eq $e.bytes)}};NewReceipt $n $rows;if(@($rows|Where-Object {-not $_.matches}).Count){throw 'PhysX owned sources changed'}}
CaptureSource 'source-before.json'
$psi=[Diagnostics.ProcessStartInfo]::new();$psi.FileName=$python;$psi.WorkingDirectory=$base;$psi.UseShellExecute=$false;$psi.CreateNoWindow=$true;$psi.RedirectStandardOutput=$true;$psi.RedirectStandardError=$true
foreach($v in @('-I','-B',"$base/AuditPhysXPackage.py")){$psi.ArgumentList.Add($v)}
$psi.Environment['PYTHONDONTWRITEBYTECODE']='1'
$out=[IO.File]::Open("$base/read.stdout.raw.log",[IO.FileMode]::CreateNew);$err=[IO.File]::Open("$base/read.stderr.raw.log",[IO.FileMode]::CreateNew)
$p=[Diagnostics.Process]::new();$p.StartInfo=$psi;$start=[DateTime]::UtcNow;$watch=[Diagnostics.Stopwatch]::StartNew();$exitCode=$null
try{
 if(-not $p.Start()){throw 'Data reader failed to start'}
 NewReceipt 'data-reader-start.json' ([ordered]@{utc=$start.ToString('o');wrapperPID=$PID;readerPID=$p.Id;executable=$python;sha256=$pin;arguments=@('-I','-B',"$base/AuditPhysXPackage.py");scope='Isolated stdlib tar/json/hash/path data; not package binary execution'})
 $o=$p.StandardOutput.BaseStream.CopyToAsync($out);$e=$p.StandardError.BaseStream.CopyToAsync($err);$p.WaitForExit();$exitCode=$p.ExitCode;$watch.Stop();[Threading.Tasks.Task]::WhenAll([Threading.Tasks.Task[]]@($o,$e)).GetAwaiter().GetResult()
}finally{$out.Dispose();$err.Dispose();$p.Dispose();CaptureSource 'source-after.json'}
NewReceipt 'data-reader-result.json' ([ordered]@{exitCode=$exitCode;elapsedSeconds=$watch.Elapsed.TotalSeconds;stdoutSHA256=(HashFile "$base/read.stdout.raw.log");stderrSHA256=(HashFile "$base/read.stderr.raw.log");sourcesMatched=5;acceptance='Parent only; no SDK/product package executable ran'})
if($exitCode -ne 0){[IO.File]::ReadAllText("$base/read.stderr.raw.log");exit 1}
[IO.File]::ReadAllText("$base/read.stdout.raw.log")
