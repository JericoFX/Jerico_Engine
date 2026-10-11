# Prepared for explicit parent assignment to Luna; NEVER runs on creation.
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$engine='F:/JericoEngine/JericoEngine'
$audit="$engine/build/n0-validation/local-packages/libsamplerate-rev2-audit-v2"
$sourceFreeze="$audit/source-freeze-libsamplerate-rev2.json"
$argumentsFile="$audit/validation-arguments-rev2.json"
$cmake='C:\Program Files\CMake\bin\cmake.exe'
$core='C:/Users/Jerico/.cache/codex-runtimes/codex-primary-runtime/dependencies/native/powershell/pwsh.exe'
$nativeArguments=[string[]]@(
 '-DJE_SAMPLERATE_SOURCE_ARCHIVE=F:/JericoEngine/Dependencies/archives/libsamplerate-a147b6dc4046deda22b2aa772b348717de772e2b.tar.gz',
 '-DJE_SAMPLERATE_SOURCE_MANIFEST=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev1-audit/source-manifest.json',
 '-DJE_SAMPLERATE_OUTPUT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev2-build',
 '-P',
 'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/LibSamplerate/BuildLibSampleratePackage.cmake')
$output="$engine/build/n0-validation/local-packages/libsamplerate-rev2-build"
$run=Join-Path $PSScriptRoot 'producer-run'
$utf8=[Text.UTF8Encoding]::new($false)
function Hash([string]$path){$hasher=[Security.Cryptography.SHA256]::Create();try{$stream=[IO.File]::OpenRead($path);try{$digest=$hasher.ComputeHash($stream)}finally{$stream.Dispose()}}finally{$hasher.Dispose()};return ([BitConverter]::ToString($digest)).Replace('-','')}
function WriteJson([string]$name,$object){$path=Join-Path $run $name;$stream=[IO.File]::Open($path,[IO.FileMode]::CreateNew);try{$data=$utf8.GetBytes(($object|ConvertTo-Json -Depth 16)+"`n");$stream.Write($data,0,$data.Length)}finally{$stream.Dispose()}}
function Snapshot([string]$name,$freeze){$records=@();foreach($entry in $freeze.entries){$exists=[IO.File]::Exists($entry.path);$bytes=$null;$actual=$null;$failure=$null;if($exists){try{$bytes=([IO.FileInfo]::new($entry.path)).Length;$actual=Hash $entry.path}catch{$failure=$_.Exception.Message}};$records+=[pscustomobject]@{path=$entry.path;expectedBytes=$entry.bytes;expectedSHA256=$entry.sha256;exists=$exists;actualBytes=$bytes;actualSHA256=$actual;matches=($exists -and $bytes -eq $entry.bytes -and $actual -ceq $entry.sha256);failure=$failure}};$receipt=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');expectedCount=152;count=$records.Count;matched=@($records|Where-Object matches).Count;entries=$records};WriteJson $name $receipt;return [pscustomobject]$receipt}
# Audit tool/authority identities before any producer invocation or output creation.
if($PSVersionTable.PSEdition -cne 'Core' -or $PSVersionTable.PSVersion.ToString() -cne '7.6.5'){throw 'Use the audited PowerShell Core7.6.5 launcher'}
if((Hash $core) -cne '362A356CE7F0940EC74F73A8FC2C990A2CC24A38A11C90BBD8ECA947110AD139'){throw 'Core launcher hash changed'}
$currentProcess=[Diagnostics.Process]::GetCurrentProcess()
if((Hash $currentProcess.MainModule.FileName) -cne (Hash $core)){throw 'Wrapper is not running under the audited Core launcher'}
if((Hash $cmake) -cne '1AA884BF1F4949327FFFCC8EE4A97C2D684BDC1D0A64B71F01DC16321C7FBC64'){throw 'CMake launcher hash changed'}
if((Hash $sourceFreeze) -cne '1AFEF870853E0E9FB64FB27F9DE7A24C3B4081252B0314A93AA6075C8BC03C29'){throw 'Source freeze authority changed'}
if((Hash $argumentsFile) -cne 'BA3B3D3E25CB765D1B978ED147F8AA0546A2BA6F0D9B3D85910E6C9EA14AAAF0'){throw 'Argument authority changed'}
$freeze=Get-Content -LiteralPath $sourceFreeze -Raw|ConvertFrom-Json
$approved=Get-Content -LiteralPath $argumentsFile -Raw|ConvertFrom-Json
if($freeze.count -ne 152 -or $freeze.entries.Count -ne 152 -or $nativeArguments.Count -ne 5 -or $approved.producer.count -ne 5){throw 'Freeze/argument count changed'}
for($i=0;$i -lt 5;$i++){if($nativeArguments[$i] -cne $approved.producer.arguments[$i]){throw "Producer argument $i differs from approved literal"}}
if(Test-Path -LiteralPath $run){throw 'Producer-run evidence already exists; preserve it'}
if(Test-Path -LiteralPath $output){throw 'Fresh producer output already exists; preserve it'}
[IO.Directory]::CreateDirectory($run)|Out-Null
$wrapperStart=[DateTime]::UtcNow
WriteJson 'wrapper-start.json' ([ordered]@{utc=$wrapperStart.ToString('o');PID=$PID;launcher=$currentProcess.MainModule.FileName;launcherSHA256=(Hash $core);PowerShell=$PSVersionTable.PSVersion.ToString();script=$PSCommandPath;scriptSHA256=(Hash $PSCommandPath);workingDirectory=$engine;scope='Single libsamplerate producer, no consumer/guards/Root/retry'})
$before=$null;$after=$null;$nativeResult=$null;$nativePID=$null;$nativeStart=$null;$nativeSeconds=$null;$wrapperFailure=$null;$attempted=$false
try{
    $before=Snapshot 'preflight-152.json' $freeze
    if($before.matched -ne 152){throw 'Preflight inputs changed; native producer not dispatched'}
    $stdoutPath=Join-Path $run 'producer.stdout.raw.log'
    $stderrPath=Join-Path $run 'producer.stderr.raw.log'
    $info=[Diagnostics.ProcessStartInfo]::new()
    $info.FileName=$cmake
    $info.WorkingDirectory=$engine
    $info.UseShellExecute=$false
    $info.CreateNoWindow=$true
    $info.RedirectStandardOutput=$true
    $info.RedirectStandardError=$true
    foreach($argument in $nativeArguments){$info.ArgumentList.Add($argument)}
    $process=[Diagnostics.Process]::new()
    $process.StartInfo=$info
    $stdout=[IO.File]::Open($stdoutPath,[IO.FileMode]::CreateNew)
    $stderr=[IO.File]::Open($stderrPath,[IO.FileMode]::CreateNew)
    try{
        $nativeStart=[DateTime]::UtcNow
        $watch=[Diagnostics.Stopwatch]::StartNew()
        if(-not $process.Start()){throw 'CMake process failed to start'}
        $attempted=$true;$nativePID=$process.Id
        # Copy byte streams without shell formatting, merged streams or truncated output.
        $stdoutCopy=$process.StandardOutput.BaseStream.CopyToAsync($stdout)
        $stderrCopy=$process.StandardError.BaseStream.CopyToAsync($stderr)
        try{
            WriteJson 'native-start.json' ([ordered]@{utc=$nativeStart.ToString('o');PID=$nativePID;executable=$cmake;executableSHA256=(Hash $cmake);arguments=$nativeArguments;count=$nativeArguments.Count;output=$output})
        }finally{
            # Even an evidence-write failure must not leave an unowned native process.
            $process.WaitForExit()
            $nativeResult=$process.ExitCode
            $watch.Stop();$nativeSeconds=$watch.Elapsed.TotalSeconds
            [Threading.Tasks.Task]::WhenAll([Threading.Tasks.Task[]]@($stdoutCopy,$stderrCopy)).GetAwaiter().GetResult()
        }
    }finally{$stdout.Dispose();$stderr.Dispose();$process.Dispose()}
}catch{$wrapperFailure=$_.Exception.ToString()}
finally{
    $after=Snapshot 'postflight-152.json' $freeze
    $logs=@();foreach($name in @('producer.stdout.raw.log','producer.stderr.raw.log')){$path=Join-Path $run $name;if([IO.File]::Exists($path)){$logs+=[pscustomobject]@{path=$path;bytes=([IO.FileInfo]::new($path)).Length;sha256=(Hash $path)}}}
    WriteJson 'producer-result.json' ([ordered]@{wrapperStartUTC=$wrapperStart.ToString('o');wrapperEndUTC=[DateTime]::UtcNow.ToString('o');wrapperPID=$PID;nativeAttempted=$attempted;nativePID=$nativePID;nativeStartUTC=$(if($nativeStart){$nativeStart.ToString('o')}else{$null});nativeExitCode=$nativeResult;nativeElapsedSeconds=$nativeSeconds;preflightMatches=$(if($before){$before.matched}else{0});postflightMatches=$after.matched;inputs=152;wrapperFailure=$wrapperFailure;rawLogs=$logs;output=$output;outputRetained=$true;scope='Producer only; no consumer/guards/Root; no retries; no acceptance claim'})
}
if($wrapperFailure -or -not $attempted -or $after.matched -ne 152){exit 2}
exit $nativeResult