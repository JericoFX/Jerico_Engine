# Parent-reviewed dispatch only; Luna is the sole native executor.
# Requires PowerShell 7/Core. This harness executes only the producer, once.
# Consumers/guards follow separately after the real archive passes parent review.
#Requires -Version 7.0
#Requires -PSEdition Core
$ErrorActionPreference='Stop'
$PSNativeCommandUseErrorActionPreference=$false
$here=$PSScriptRoot
$freeze=Get-Content "$here/source-freeze-astc-candidate.json" -Raw | ConvertFrom-Json
$assignment=Get-Content "$here/validation-assignment.json" -Raw | ConvertFrom-Json
function Hash([string]$path){$stream=[IO.File]::OpenRead($path);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}}
function Verify([string]$phase){$deltas=@();foreach($file in $freeze){if(-not(Test-Path -LiteralPath $file.path -PathType Leaf)){$deltas+=$file.path;continue};$item=Get-Item -LiteralPath $file.path;if($item.Length -ne $file.bytes -or (Hash $file.path) -ne $file.sha256){$deltas+=$file.path}};[ordered]@{phase=$phase;count=$freeze.Count;deltas=$deltas} | ConvertTo-Json -Depth 5 | Set-Content "$attempt/$phase.json" -Encoding utf8NoBOM;if($deltas.Count){throw "$phase frozen inputs changed"}}
$attempt='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/astc-rev1-validation/attempt1'
if(Test-Path -LiteralPath $attempt){throw 'Fresh attempt required; prior outputs preserved'}
if(Test-Path -LiteralPath $assignment.producer.freshOutput){throw 'Producer output already exists; preserved'}
New-Item -ItemType Directory -Path $attempt | Out-Null
Verify 'preflight'
[ordered]@{executable=$assignment.producer.executable;argv=@($assignment.producer.argv)} | ConvertTo-Json -Depth 5 | Set-Content "$attempt/argv.json" -Encoding utf8NoBOM
$timer=[Diagnostics.Stopwatch]::StartNew()
& $assignment.producer.executable @($assignment.producer.argv) 1>"$attempt/stdout.log" 2>"$attempt/stderr.log"
$exitCode=$LASTEXITCODE
$timer.Stop()
[ordered]@{exitCode=$exitCode;seconds=$timer.Elapsed.TotalSeconds;producerOnly=$true} | ConvertTo-Json | Set-Content "$attempt/result.json" -Encoding utf8NoBOM
Verify 'postflight'
exit $exitCode
