# Data/evidence harness for explicit Luna assignment; not a product provider.
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$script:PreparationRoot=$PSScriptRoot
$script:Engine='F:/JericoEngine/JericoEngine'
$script:CMake='C:\Program Files\CMake\bin\cmake.exe'
$script:CTest='C:\Program Files\CMake\bin\ctest.exe'
$script:Core='C:/Users/Jerico/.cache/codex-runtimes/codex-primary-runtime/dependencies/native/powershell/pwsh.exe'
$script:Phases=[Collections.Generic.List[object]]::new()
function GetHash([string]$path){$h=[Security.Cryptography.SHA256]::Create();try{$s=[IO.File]::OpenRead($path);try{$b=$h.ComputeHash($s)}finally{$s.Dispose()}}finally{$h.Dispose()};([BitConverter]::ToString($b)).Replace('-','')}
function WriteReceipt([string]$name,$object){$p=Join-Path $script:RunRoot $name;$s=[IO.File]::Open($p,[IO.FileMode]::CreateNew);try{$b=[Text.UTF8Encoding]::new($false).GetBytes(($object|ConvertTo-Json -Depth 16)+"`n");$s.Write($b,0,$b.Length)}finally{$s.Dispose()}}
function CaptureInputs([string]$name){$records=@();foreach($e in $script:InputFreeze.entries){$exists=[IO.File]::Exists($e.path);$actual=$null;$bytes=$null;$errorText=$null;if($exists){try{$actual=GetHash $e.path;$bytes=([IO.FileInfo]::new($e.path)).Length}catch{$errorText=$_.Exception.Message}};$records+=[pscustomobject]@{path=$e.path;expectedSHA256=$e.sha256;expectedBytes=$e.bytes;actualSHA256=$actual;actualBytes=$bytes;matches=($exists -and $actual -ceq $e.sha256 -and $bytes -eq $e.bytes);error=$errorText}};$result=[pscustomobject]@{utc=[DateTime]::UtcNow.ToString('o');count=$records.Count;matched=@($records|Where-Object matches).Count;entries=$records};WriteReceipt $name $result;return $result}
function BeginRun([string]$name){
    if($PSVersionTable.PSEdition -cne 'Core' -or $PSVersionTable.PSVersion.ToString() -cne '7.6.5'){throw 'Use audited Core7.6.5'}
    $launcher=[Diagnostics.Process]::GetCurrentProcess().MainModule.FileName
    if((GetHash $launcher) -cne '362A356CE7F0940EC74F73A8FC2C990A2CC24A38A11C90BBD8ECA947110AD139'){throw 'Wrapper launcher changed'}
    if((GetHash $script:CMake) -cne '1AA884BF1F4949327FFFCC8EE4A97C2D684BDC1D0A64B71F01DC16321C7FBC64'){throw 'CMake changed'}
    if((GetHash $script:CTest) -cne '73BAACBEB272CA6F40422B4F789403390AF678BEB491783CEF1727D69CD3E1CB'){throw 'CTest changed'}
    $script:InputFreeze=Get-Content -LiteralPath (Join-Path $script:PreparationRoot 'validation-inputs.json') -Raw|ConvertFrom-Json
    if($script:InputFreeze.count -ne $script:InputFreeze.entries.Count){throw 'Input freeze count differs'}
    $script:RunRoot=Join-Path $script:PreparationRoot $name
    if(Test-Path -LiteralPath $script:RunRoot){throw 'Preserve existing run evidence'}
    [IO.Directory]::CreateDirectory($script:RunRoot)|Out-Null
    $script:WrapperStart=[DateTime]::UtcNow
    WriteReceipt 'wrapper-start.json' ([ordered]@{utc=$script:WrapperStart.ToString('o');PID=$PID;launcher=$launcher;launcherSHA256=(GetHash $launcher);PowerShell=$PSVersionTable.PSVersion.ToString();scope=$name;inputFreezeSHA256=(GetHash (Join-Path $script:PreparationRoot 'validation-inputs.json'))})
    $script:Before=CaptureInputs 'preflight.json'
    # Caller checks this inside its try/finally so a mismatch also gets postflight.
}
function Native([string]$name,[string]$executable,[string[]]$arguments){
    if($executable -notin @($script:CMake,$script:CTest)){throw 'Only assigned CMake/CTest phases permitted'}
    $info=[Diagnostics.ProcessStartInfo]::new();$info.FileName=$executable;$info.WorkingDirectory=$script:Engine;$info.UseShellExecute=$false;$info.CreateNoWindow=$true;$info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true
    foreach($argument in $arguments){$info.ArgumentList.Add($argument)}
    $process=[Diagnostics.Process]::new();$process.StartInfo=$info
    $outPath=Join-Path $script:RunRoot ($name+'.stdout.raw.log');$errPath=Join-Path $script:RunRoot ($name+'.stderr.raw.log')
    $out=[IO.File]::Open($outPath,[IO.FileMode]::CreateNew);$err=[IO.File]::Open($errPath,[IO.FileMode]::CreateNew)
    $start=[DateTime]::UtcNow;$watch=[Diagnostics.Stopwatch]::StartNew();$nativePID=$null;$exitCode=$null
    try{
        if(-not $process.Start()){throw 'Native process did not start'};$nativePID=$process.Id
        $copyOut=$process.StandardOutput.BaseStream.CopyToAsync($out);$copyErr=$process.StandardError.BaseStream.CopyToAsync($err)
        try{WriteReceipt ($name+'.start.json') ([ordered]@{utc=$start.ToString('o');PID=$nativePID;executable=$executable;executableSHA256=(GetHash $executable);arguments=$arguments;count=$arguments.Count})}
        finally{$process.WaitForExit();$exitCode=$process.ExitCode;$watch.Stop();[Threading.Tasks.Task]::WhenAll([Threading.Tasks.Task[]]@($copyOut,$copyErr)).GetAwaiter().GetResult()}
    }finally{$out.Dispose();$err.Dispose();$process.Dispose()}
    $result=[pscustomobject]@{name=$name;utc=$start.ToString('o');PID=$nativePID;executable=$executable;arguments=$arguments;count=$arguments.Count;exitCode=$exitCode;elapsedSeconds=$watch.Elapsed.TotalSeconds;stdout=$outPath;stdoutBytes=([IO.FileInfo]::new($outPath)).Length;stdoutSHA256=(GetHash $outPath);stderr=$errPath;stderrBytes=([IO.FileInfo]::new($errPath)).Length;stderrSHA256=(GetHash $errPath)}
    $script:Phases.Add($result);WriteReceipt ($name+'.result.json') $result;return $result
}
function EndRun([string]$status,[string]$failure,$checks){$after=CaptureInputs 'postflight.json';$result=[ordered]@{status=$status;wrapperStartUTC=$script:WrapperStart.ToString('o');wrapperEndUTC=[DateTime]::UtcNow.ToString('o');wrapperPID=$PID;preflightMatches=$script:Before.matched;postflightMatches=$after.matched;inputs=$script:InputFreeze.count;phases=$script:Phases.ToArray();checks=$checks;failure=$failure;acceptance='Parent only; this driver does not certify Atom/editor/AP/cook/runtime';retry='None'};WriteReceipt 'validation-result.json' $result;if($after.matched -ne $script:InputFreeze.count){throw 'Postflight source/artifact inputs changed'};return $result}