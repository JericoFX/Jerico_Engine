#Requires -Version 7.0
param([Parameter(Mandatory=$true)][string]$ToolchainWitnessSHA256)
$ErrorActionPreference='Stop'
if($PSVersionTable.PSEdition -ne 'Core'){throw 'PowerShell Core required'}
$taskRepo='F:/JericoEngine/JericoEngine'
$taskBase="$taskRepo/build/n0-validation/local-packages"
$taskRun="$taskBase/dxc-rev1-validation/attempt1"
$taskOutput="$taskBase/dxc-local-rev1"
$taskFreezePath="$taskBase/dxc-rev1-preparation/provider-candidate-rev2/source-freeze-dxc-provider-rev2-final.json"
$taskAssignmentPath="$taskBase/dxc-rev1-preparation/provider-candidate-rev2/validation-assignment.json"
$taskToolchainPath="$taskBase/dxc-rev1-preparation/parent-native-toolchain-rev1/witness.json"
function Get-TaskHash([string]$path){
    $stream=[IO.File]::OpenRead($path)
    try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}
}
function Require-TaskHash([string]$path,[string]$hash){
    if((Get-TaskHash $path) -ne $hash){throw "Frozen witness changed: $path"}
}
function Assert-TaskLocalPath([string]$path){
    if($path -match '^[\\/]{2}|://'){throw 'Local path required'}
    $full=[IO.Path]::GetFullPath($path)
    $cursor=$full
    while($cursor){
        if(([IO.File]::Exists($cursor) -or [IO.Directory]::Exists($cursor)) -and ([IO.File]::GetAttributes($cursor) -band [IO.FileAttributes]::ReparsePoint)){throw "Reparse path rejected: $cursor"}
        $cursor=[IO.Path]::GetDirectoryName($cursor)
    }
    return $full
}
foreach($path in @($taskRun,$taskOutput)){
    $full=Assert-TaskLocalPath $path
    if(-not $full.StartsWith([IO.Path]::GetFullPath("$taskRepo/build/"),[StringComparison]::OrdinalIgnoreCase)){throw 'Output escaped checkout/build'}
    if([IO.Directory]::Exists($full) -or [IO.File]::Exists($full)){throw "Output exists; preserve it: $full"}
}
Require-TaskHash $taskFreezePath '123AE0639F373D0EF16E6147FC74765CBCBBD7AF42CCEF1535D58A5B7A1C5CCD'
Require-TaskHash $taskAssignmentPath '5968A5F2785CC65BEBA8AD14EAB987C429645F2EC5BB3CCBBE3FA21902CE9585'
Require-TaskHash $taskToolchainPath $ToolchainWitnessSHA256
$taskFreeze=Get-Content -Raw -LiteralPath $taskFreezePath | ConvertFrom-Json
$taskAssignment=Get-Content -Raw -LiteralPath $taskAssignmentPath | ConvertFrom-Json
$taskToolchain=Get-Content -Raw -LiteralPath $taskToolchainPath | ConvertFrom-Json
if($taskFreeze.files.Count -ne 23348 -or $taskFreeze.providerFileCount -ne 12 -or $taskFreeze.parallelJobs -ne 6){throw 'Unexpected frozen candidate'}
if($taskAssignment.producer.freshOutput.Replace([char]92,[char]47) -ne $taskOutput -or $taskAssignment.producer.executable -ne 'C:/Program Files/CMake/bin/cmake.exe' -or $taskAssignment.producer.argv -notcontains '-DJE_DXC_PARALLEL_JOBS=6'){throw 'Producer assignment differs'}
if($taskToolchain.instanceId -ne '72be9b85' -or $taskToolchain.installationPath -ne 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools'){throw 'Existing VS instance differs'}
$taskInputs=@($taskFreeze.files)+@($taskToolchain.files)
$taskMembershipRoots=@(
    "$taskBase/dxc-rev1-preparation/acquisition/dxc/snapshot",
    "$taskBase/dxc-rev1-preparation/acquisition/spirv-headers/snapshot",
    "$taskBase/dxc-rev1-preparation/acquisition/spirv-tools/snapshot",
    "$taskRepo/build/n0-validation/original-engine-deps/Python-rev3/packages/python-3.10.13-je-rev3-windows/python/Lib"
)
function Test-TaskInputs{
    $bad=[Collections.Generic.List[string]]::new()
    foreach($entry in $taskInputs){
        $full=Assert-TaskLocalPath $entry.path
        if(-not [IO.File]::Exists($full)){$bad.Add("Missing: $full");continue}
        if([IO.FileInfo]::new($full).Length -ne $entry.bytes -or (Get-TaskHash $full) -ne $entry.sha256){$bad.Add("Changed: $full")}
    }
    foreach($root in $taskMembershipRoots){
        $prefix=[IO.Path]::GetFullPath("$root/")
        $expected=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        foreach($entry in $taskFreeze.files){$full=[IO.Path]::GetFullPath($entry.path);if($full.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)){[void]$expected.Add($full)}}
        if(-not $expected.Count){throw "No frozen membership for $root"}
        $actual=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        $stack=[Collections.Generic.Stack[string]]::new();$stack.Push((Assert-TaskLocalPath $root))
        while($stack.Count){
            foreach($item in Get-ChildItem -LiteralPath $stack.Pop() -Force){
                if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Reparse membership rejected: $($item.FullName)"}
                if($item.PSIsContainer){$stack.Push($item.FullName)}else{[void]$actual.Add($item.FullName)}
            }
        }
        if(-not $expected.SetEquals($actual)){$bad.Add("Membership changed: $root")}
    }
    return ,$bad
}
$taskBefore=Test-TaskInputs
if($taskBefore.Count){throw ($taskBefore -join '; ')}
[void][IO.Directory]::CreateDirectory($taskRun)
function Write-TaskEvidence([string]$path,$value){[IO.File]::WriteAllText($path,($value | ConvertTo-Json -Depth 12)+"`n",[Text.UTF8Encoding]::new($false))}
Write-TaskEvidence "$taskRun/preflight.json" @{sourceInputs=23348;toolchainInputs=$taskToolchain.files.Count;fourMembershipRoots=$taskMembershipRoots;changes=@();status='PASS'}
$taskFailed=$false;$taskNativeExit=$null;$taskResult=$null
try{
    if($env:VSCMD_DEBUG){throw 'VS debug environment logging must be absent; no secret environment dumps'}
    Import-Module -Name $taskToolchain.modulePath
    $taskCommand=Get-Command Enter-VsDevShell
    foreach($name in @('VsInstanceId','Arch','HostArch','SkipAutomaticLocation')){if(-not $taskCommand.Parameters.ContainsKey($name)){throw "Installed DevShell parameter absent: $name"}}
    Enter-VsDevShell -VsInstanceId '72be9b85' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation *> "$taskRun/vs-dev-shell.log"
    if($env:VSCMD_ARG_TGT_ARCH -ne 'x64' -or $env:VSCMD_ARG_HOST_ARCH -ne 'x64' -or $env:VCToolsVersion.TrimEnd([char]92) -ne '14.44.35207' -or $env:WindowsSDKVersion.TrimEnd([char]92) -ne '10.0.26100.0'){throw 'Actual VS/SDK x64 environment differs'}
    $env:PYTHONDONTWRITEBYTECODE='1'
    $taskEnvironment=[ordered]@{}
    foreach($name in @('VSCMD_ARG_TGT_ARCH','VSCMD_ARG_HOST_ARCH','VCToolsVersion','WindowsSDKVersion','WindowsSdkDir','VSINSTALLDIR','INCLUDE','LIB','LIBPATH','PYTHONDONTWRITEBYTECODE')){$taskEnvironment[$name]=[Environment]::GetEnvironmentVariable($name,'Process')}
    $taskEnvironment['pathSHA256']=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($env:PATH)))
    $taskEnvironment['pathEntryCount']=@($env:PATH.Split(';')).Count
    $taskEnvironment['scope']='Child PowerShell process only; no global environment change; PATH contents not dumped'
    Write-TaskEvidence "$taskRun/selected-environment.json" $taskEnvironment
    Write-TaskEvidence "$taskRun/producer.argv.json" @{executable=$taskAssignment.producer.executable;argv=@($taskAssignment.producer.argv);workingDirectory=$taskRepo}
    $taskStart=[Diagnostics.ProcessStartInfo]::new()
    $taskStart.FileName=$taskAssignment.producer.executable;$taskStart.WorkingDirectory=$taskRepo
    $taskStart.UseShellExecute=$false;$taskStart.CreateNoWindow=$true
    $taskStart.RedirectStandardOutput=$true;$taskStart.RedirectStandardError=$true
    foreach($arg in $taskAssignment.producer.argv){$taskStart.ArgumentList.Add([string]$arg)}
    $taskProcess=[Diagnostics.Process]::new();$taskProcess.StartInfo=$taskStart
    $taskStdout=[IO.File]::Create("$taskRun/producer.stdout.log");$taskStderr=[IO.File]::Create("$taskRun/producer.stderr.log")
    $taskWatch=[Diagnostics.Stopwatch]::StartNew();$taskPeak=0L;$taskSamples=0;$taskNextProgress=30
    try{
        if(-not $taskProcess.Start()){throw 'Producer did not start'}
        $taskOut=$taskProcess.StandardOutput.BaseStream.CopyToAsync($taskStdout)
        $taskErr=$taskProcess.StandardError.BaseStream.CopyToAsync($taskStderr)
        Write-TaskEvidence "$taskRun/producer.running.json" @{pid=$taskProcess.Id;startedUTC=[DateTime]::UtcNow.ToString('o');nativeOwner='Luna';parallelJobs=6}
        while(-not $taskProcess.WaitForExit(1000)){
            $taskProcess.Refresh();$taskPeak=[Math]::Max($taskPeak,$taskProcess.PeakWorkingSet64);$taskSamples++
            if($taskWatch.Elapsed.TotalSeconds -ge $taskNextProgress){Write-Host "DXC producer running: $([Math]::Round($taskWatch.Elapsed.TotalSeconds))s; six jobs; logs retained";$taskNextProgress+=30}
        }
        $taskNativeExit=$taskProcess.ExitCode
        $taskWatch.Stop();[Threading.Tasks.Task]::WaitAll(@($taskOut,$taskErr))
        $taskResult=@{nativeExit=$taskNativeExit;elapsedSeconds=$taskWatch.Elapsed.TotalSeconds;parallelJobs=6;peakParentWorkingSetBytes=$taskPeak;parentSamples=$taskSamples;memoryScope='Only outer CMake producer; excludes native child processes and engine';scope='Dependency producer only; no consumer/Root/Editor acceptance'}
        Write-TaskEvidence "$taskRun/producer-result.json" $taskResult
        Write-Host "DXC producer: exit=$taskNativeExit elapsed=$([Math]::Round($taskWatch.Elapsed.TotalSeconds,3))s"
    }finally{$taskStdout.Dispose();$taskStderr.Dispose();$taskProcess.Dispose()}
    if($taskNativeExit -ne 0){throw 'Real producer failed; retained logs; no retry or consumer'}
    $taskArchive="$taskOutput/packages/DirectXShaderCompilerDxc-1.8.2505.1-o3de-je-rev1-windows.tar.xz"
    $taskSidecar="$taskArchive.SHA256SUMS"
    if(-not [IO.File]::Exists($taskArchive) -or -not [IO.File]::Exists($taskSidecar)){throw 'Required real package/sidecar absent'}
    $taskSidecarText=[IO.File]::ReadAllText($taskSidecar)
    if($taskSidecarText -notmatch '^([a-f0-9]{64}) \*DirectXShaderCompilerDxc-1[.]8[.]2505[.]1-o3de-je-rev1-windows[.]tar[.]xz\r?\n$'){throw 'Actual sidecar syntax differs'}
    $taskArchiveHash=Get-TaskHash $taskArchive
    if($taskArchiveHash -ne $Matches[1]){throw 'Actual produced archive differs from sidecar'}
    Write-TaskEvidence "$taskRun/produced-package.json" @{archive=$taskArchive;bytes=[IO.FileInfo]::new($taskArchive).Length;sha256=$taskArchiveHash;sidecar=$taskSidecar;sidecarSHA256=(Get-TaskHash $taskSidecar);status='PRODUCED ONLY; ARCHIVE/CONSUMER/GUARDS/ROOT PENDING'}
}catch{
    $taskFailed=$true
    Write-TaskEvidence "$taskRun/driver-error.json" @{message=$_.Exception.Message;scriptStack=$_.ScriptStackTrace;nativeExit=$taskNativeExit}
}finally{
    try{$taskAfter=Test-TaskInputs;$taskChanges=@($taskAfter.ToArray())}catch{$taskChanges=@($_.Exception.Message)}
    if($taskChanges.Count){$taskFailed=$true}
    Write-TaskEvidence "$taskRun/postflight.json" @{sourceInputs=23348;toolchainInputs=$taskToolchain.files.Count;changes=$taskChanges;status=if($taskChanges.Count){'FAIL'}else{'PASS'}}
    Write-TaskEvidence "$taskRun/validation-result.json" @{status=if($taskFailed){'FAIL; RETAIN ACTUAL RESULTS'}else{'PRODUCER FINISHED; PARENT/CONSUMER/GUARDS/ROOT REQUIRED'};producer=$taskResult;changedPaths=$taskChanges;consumerExecuted=$false;rootExecuted=$false}
}
if($taskFailed){exit 1}
exit 0
