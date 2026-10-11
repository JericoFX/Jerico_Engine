#Requires -Version 7.0
param([Parameter(Mandatory=$true)][string]$DriverSHA256)
$ErrorActionPreference='Stop'
$taskRepo='F:/JericoEngine/JericoEngine'
$taskBase="$taskRepo/build/n0-validation/local-packages"
$taskRun="$taskBase/dxc-rev1-validation/consumer-attempt4"
$taskExtract="$taskBase/dxc-native-extracted-rev1"
$taskConsumer="$taskBase/dxc-native-consumer-rev4"
$taskFreezePath="$taskBase/dxc-rev1-preparation/consumer-candidate-rev4/source-freeze-dxc-consumer-rev4.json"
$taskAssignmentPath="$taskBase/dxc-rev1-preparation/consumer-candidate-rev4/validation-assignment.json"
$taskToolchainPath="$taskBase/dxc-rev1-preparation/parent-native-toolchain-rev1/witness.json"
$taskProducedPath="$taskBase/dxc-rev1-validation/attempt1/produced-package.json"
$taskProducerResultPath="$taskBase/dxc-rev1-validation/attempt1/validation-result.json"
$taskArchive="$taskBase/dxc-local-rev1/packages/DirectXShaderCompilerDxc-1.8.2505.1-o3de-je-rev1-windows.tar.xz"
$taskArchiveSHA='62FC6978933215440718D8D810AB5ECF20A36368A8592D12822F902ED0654782'
function Get-TaskHash([string]$path){
    $stream=[IO.File]::OpenRead($path)
    try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}
}
function Require-TaskHash([string]$path,[string]$hash){if((Get-TaskHash $path) -ne $hash){throw "Frozen input changed: $path"}}
function Assert-TaskLocalPath([string]$path){
    if(-not [IO.Path]::IsPathFullyQualified($path) -or $path -match '^[\\/]{2}|://'){throw 'Absolute local path required'}
    $full=[IO.Path]::GetFullPath($path);$cursor=$full
    while($cursor){
        if(([IO.File]::Exists($cursor) -or [IO.Directory]::Exists($cursor)) -and ([IO.File]::GetAttributes($cursor) -band [IO.FileAttributes]::ReparsePoint)){throw "Reparse path rejected: $cursor"}
        $cursor=[IO.Path]::GetDirectoryName($cursor)
    }
    return $full
}
if($PSVersionTable.PSEdition -ne 'Core'){throw 'PowerShell Core required'}
Require-TaskHash $PSCommandPath $DriverSHA256
foreach($path in @($taskRun,$taskConsumer)){
    $full=Assert-TaskLocalPath $path
    if(-not $full.StartsWith([IO.Path]::GetFullPath("$taskRepo/build/"),[StringComparison]::OrdinalIgnoreCase)){throw 'Output escaped checkout/build'}
    if([IO.Directory]::Exists($full) -or [IO.File]::Exists($full)){throw "Output exists; preserve it: $full"}
}
Require-TaskHash $taskFreezePath 'A708661417616D36547CF224DD24712176157A1B02A8086A93DD64079AEEAD92'
Require-TaskHash $taskAssignmentPath '848BCA5E32ED6E815126EAC789EEB8F874865D51B4A5C2D748DA1B04DBC2664B'
Require-TaskHash $taskToolchainPath 'F9923C56A303C4AA7D75F61E4C85A4304559A73AB4EB6B76D32502EA3B691107'
Require-TaskHash $taskProducedPath '2BCCBF74E624D3F6CA2CF12124F78FA3446D1E49AB182DE5CC758D53D6F80844'
Require-TaskHash $taskProducerResultPath '5ABB25C1883ADEA32789D9C0FE18C1F3C2A2654A5984BC994DC619861862AB08'
Require-TaskHash (Assert-TaskLocalPath $taskArchive) $taskArchiveSHA
Require-TaskHash "$taskArchive.SHA256SUMS" '8A9C9F9F562F099559770A5770B1465928471416181CEBC38198B2FEFF2E627E'
$taskFreeze=Get-Content -Raw -LiteralPath $taskFreezePath | ConvertFrom-Json
$taskAssignment=Get-Content -Raw -LiteralPath $taskAssignmentPath | ConvertFrom-Json
$taskToolchain=Get-Content -Raw -LiteralPath $taskToolchainPath | ConvertFrom-Json
$taskProducerResult=Get-Content -Raw -LiteralPath $taskProducerResultPath | ConvertFrom-Json
if($taskProducerResult.producer.nativeExit -ne 0 -or $taskProducerResult.changedPaths.Count){throw 'Real producer not clean'}
if($taskFreeze.files.Count -ne 23385){throw 'Unexpected frozen candidate'}
if($taskAssignment.packageRoot.Replace([char]92,[char]47) -ne $taskExtract -or $taskAssignment.consumerBuild.argv -notcontains $taskConsumer.Replace([char]47,[char]92)){throw 'Consumer assignment differs'}
$taskExtraInputs=@(
    @{path='C:/Program Files/CMake/bin/ctest.exe';bytes=14012488;sha256='73BAACBEB272CA6F40422B4F789403390AF678BEB491783CEF1727D69CD3E1CB'},
    @{path=$PSCommandPath;bytes=[IO.FileInfo]::new($PSCommandPath).Length;sha256=$DriverSHA256},
    @{path=$taskArchive;bytes=7352940;sha256=$taskArchiveSHA},
    @{path="$taskArchive.SHA256SUMS";bytes=131;sha256='8A9C9F9F562F099559770A5770B1465928471416181CEBC38198B2FEFF2E627E'}
)
$taskExtraInputs+=@(
    @{path="$taskBase/dxc-rev1-validation/consumer-attempt1/archive-audit.json";bytes=6220;sha256='7F385D2CB8B425AE4A7F6586DA7CFF8D8A20766F60B769AB1E55E886276B42A0'},
    @{path="$taskBase/dxc-rev1-validation/consumer-attempt1/postflight.json";bytes=1156;sha256='5071D82C8A6507D2400DF4E8785089658A1E86781CBBA268F53C509194871C26'}
)
$taskInputs=@($taskFreeze.files)+@($taskToolchain.files)+$taskExtraInputs
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
function Write-TaskEvidence([string]$path,$value){[IO.File]::WriteAllText($path,($value | ConvertTo-Json -Depth 14)+"`n",[Text.UTF8Encoding]::new($false))}
function Invoke-TaskNative([string]$label,[string]$executable,[string[]]$arguments,[string]$workingDirectory){
    Write-TaskEvidence "$taskRun/$label.argv.json" @{executable=$executable;argv=$arguments;workingDirectory=$workingDirectory}
    $start=[Diagnostics.ProcessStartInfo]::new();$start.FileName=$executable;$start.WorkingDirectory=$workingDirectory
    $start.UseShellExecute=$false;$start.CreateNoWindow=$true;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
    foreach($arg in $arguments){$start.ArgumentList.Add($arg)}
    $process=[Diagnostics.Process]::new();$process.StartInfo=$start
    $stdout=[IO.File]::Create("$taskRun/$label.stdout.log");$stderr=[IO.File]::Create("$taskRun/$label.stderr.log")
    $watch=[Diagnostics.Stopwatch]::StartNew()
    try{
        if(-not $process.Start()){throw "Native process did not start: $label"}
        $out=$process.StandardOutput.BaseStream.CopyToAsync($stdout);$err=$process.StandardError.BaseStream.CopyToAsync($stderr)
        $nextProgress=30
        while(-not $process.WaitForExit(1000)){if($watch.Elapsed.TotalSeconds -ge $nextProgress){Write-Host "$label running: $([Math]::Round($watch.Elapsed.TotalSeconds))s";$nextProgress+=30}}
        $nativeExit=$process.ExitCode;$watch.Stop();[Threading.Tasks.Task]::WaitAll(@($out,$err))
        $result=@{nativeExit=$nativeExit;elapsedSeconds=$watch.Elapsed.TotalSeconds}
        Write-TaskEvidence "$taskRun/$label.result.json" $result
        Write-Host "$label exit=$nativeExit elapsed=$([Math]::Round($watch.Elapsed.TotalSeconds,3))s"
        return $result
    }finally{$stdout.Dispose();$stderr.Dispose();$process.Dispose()}
}
Write-TaskEvidence "$taskRun/preflight.json" @{sourceInputs=23385;toolchainInputs=44;extraInputs=$taskExtraInputs;fourMembershipRoots=$taskMembershipRoots;changes=@();status='PASS'}
$taskFailed=$false;$taskResults=[ordered]@{};$taskCases=@();$taskPayload=@()
try{
    $env:PYTHONDONTWRITEBYTECODE='1'
    $taskAuditPath="$taskBase/dxc-rev1-validation/consumer-attempt1/archive-audit.json"
    Require-TaskHash $taskAuditPath '7F385D2CB8B425AE4A7F6586DA7CFF8D8A20766F60B769AB1E55E886276B42A0'
    Require-TaskHash "$taskBase/dxc-rev1-validation/consumer-attempt1/postflight.json" '5071D82C8A6507D2400DF4E8785089658A1E86781CBBA268F53C509194871C26'
    $taskPrior=Get-Content -Raw -LiteralPath "$taskBase/dxc-rev1-validation/consumer-attempt1/consumer-result.json" | ConvertFrom-Json
    if($taskPrior.nativeResults.archiveAudit.nativeExit -ne 0 -or $taskPrior.nativeResults.extraction.nativeExit -ne 0 -or $taskPrior.changedPaths.Count -or $taskPrior.payloadStatus -ne 'PASS'){throw 'Previous actual archive/extraction evidence invalid'}
    $taskAudit=Get-Content -Raw -LiteralPath $taskAuditPath | ConvertFrom-Json
    if($taskAudit.status -ne 'PASS' -or $taskAudit.sha256 -ne $taskArchiveSHA -or $taskAudit.memberCount -ne 15 -or $taskAudit.manifestRecords -ne 14){throw 'Actual previous archive audit differs'}
    $taskResults['archiveAudit']=@{reusedVerifiedEvidence=$true;nativeExit=$taskPrior.nativeResults.archiveAudit.nativeExit;historicalElapsedSeconds=$taskPrior.nativeResults.archiveAudit.elapsedSeconds;currentNativeExecuted=$false;evidence=$taskAuditPath}
    $taskResults['extraction']=@{reusedVerifiedEvidence=$true;nativeExit=$taskPrior.nativeResults.extraction.nativeExit;historicalElapsedSeconds=$taskPrior.nativeResults.extraction.elapsedSeconds;currentNativeExecuted=$false;scope='Existing actual package read-only; all 15 members rehashed before/after'}
    $taskSourcePayload=@{
        'FindDirectXShaderCompilerDxc.cmake'="$taskRepo/cmake/Tools/LocalPackages/DirectXShaderCompilerDxc/FindDirectXShaderCompilerDxc.cmake"
        'JERequireLocalPath.cmake'="$taskRepo/cmake/Tools/LocalPackages/Python/JERequireLocalPath.cmake"
        'NOTICE.txt'="$taskRepo/cmake/Tools/LocalPackages/DirectXShaderCompilerDxc/NOTICE.txt"
        'README.md'="$taskRepo/cmake/Tools/LocalPackages/DirectXShaderCompilerDxc/README.md"
        'LICENSES/SourceNotices.txt'="$taskRepo/cmake/Tools/LocalPackages/DirectXShaderCompilerDxc/SourceNotices.txt"
        'LICENSES/O3DE/LICENSE.txt'="$taskRepo/LICENSE.txt"
        'LICENSES/O3DE/LICENSE_APACHE2.TXT'="$taskRepo/LICENSE_APACHE2.TXT"
        'LICENSES/O3DE/LICENSE_MIT.TXT'="$taskRepo/LICENSE_MIT.TXT"
    }
    foreach($name in $taskSourcePayload.Keys){
        $entry=@($taskAudit.files | Where-Object name -CEQ $name)
        if($entry.Count -ne 1 -or $entry[0].sha256 -ne (Get-TaskHash $taskSourcePayload[$name])){throw "Packaged source/notice differs: $name"}
    }
    function Test-TaskPayload{
        $files=[Collections.Generic.List[object]]::new();$stack=[Collections.Generic.Stack[string]]::new();$stack.Push($taskExtract)
        while($stack.Count){
            foreach($item in Get-ChildItem -LiteralPath $stack.Pop() -Force){
                if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Extracted reparse rejected'}
                if($item.PSIsContainer){$stack.Push($item.FullName)}else{$name=[IO.Path]::GetRelativePath($taskExtract,$item.FullName).Replace([char]92,[char]47);$entry=@($taskAudit.files | Where-Object name -CEQ $name);if($entry.Count -ne 1 -or $entry[0].bytes -ne $item.Length -or $entry[0].sha256 -ne (Get-TaskHash $item.FullName)){throw "Actual extracted member differs: $name"};$files.Add($entry[0])}
            }
        }
        if($files.Count -ne 15){throw 'Actual extracted membership differs'}
        return $files.ToArray()
    }
    $taskPayload=@(Test-TaskPayload)
    Write-TaskEvidence "$taskRun/extracted-payload-before.json" @{status='PASS';files=$taskPayload}
    $taskResults['configure']=Invoke-TaskNative 'configure' $taskAssignment.consumerConfigure.executable @($taskAssignment.consumerConfigure.argv) $taskRepo
    if($taskResults['configure'].nativeExit -ne 0){throw 'Original DXC consumer configure failed'}
    $taskResults['build']=Invoke-TaskNative 'build' $taskAssignment.consumerBuild.executable @($taskAssignment.consumerBuild.argv) $taskRepo
    if($taskResults['build'].nativeExit -ne 0){throw 'Original DXC consumer build failed'}
    $taskResults['ctest']=Invoke-TaskNative 'ctest' $taskAssignment.consumerCTest.executable (@($taskAssignment.consumerCTest.argv)+@('--output-junit',"$taskRun/ctest.junit.xml")) $taskRepo
    if([IO.File]::Exists("$taskConsumer/Testing/Temporary/LastTest.log")){[IO.File]::Copy("$taskConsumer/Testing/Temporary/LastTest.log","$taskRun/LastTest.log",$false)}
    [xml]$taskJUnit=[IO.File]::ReadAllText("$taskRun/ctest.junit.xml")
    $taskNodes=@($taskJUnit.SelectNodes('//testcase'));$taskNames=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach($node in $taskNodes){
        if(-not $taskNames.Add($node.GetAttribute('name')) -or $node.SelectNodes('./failure|./error|./skipped').Count -or ($node.HasAttribute('status') -and $node.GetAttribute('status') -ne 'run')){throw 'Actual CTest failure/error/skipped/duplicate case'}
    }
    if($taskNodes.Count -ne 4 -or -not $taskNames.SetEquals([string[]]$taskAssignment.consumerCTest.cases)){throw 'Actual CTest case membership differs'}
    $taskCases=@($taskNames | Sort-Object)
    if($taskResults['ctest'].nativeExit -ne 0){throw 'Actual CTest exit nonzero'}
}catch{
    $taskFailed=$true
    Write-TaskEvidence "$taskRun/driver-error.json" @{message=$_.Exception.Message;scriptStack=$_.ScriptStackTrace;actualNativeResults=$taskResults}
}finally{
    try{$taskChanges=@((Test-TaskInputs).ToArray())}catch{$taskChanges=@($_.Exception.Message)}
    if($taskChanges.Count){$taskFailed=$true}
    $taskPayloadStatus='NOT_EXTRACTED'
    if([IO.Directory]::Exists($taskExtract)){
        try{$taskPayload=@(Test-TaskPayload);$taskPayloadStatus='PASS'}catch{$taskPayloadStatus='FAIL';$taskFailed=$true;Write-TaskEvidence "$taskRun/extracted-payload-error.json" @{message=$_.Exception.Message}}
        Write-TaskEvidence "$taskRun/extracted-payload-after.json" @{status=$taskPayloadStatus;files=$taskPayload}
    }
    Write-TaskEvidence "$taskRun/postflight.json" @{sourceInputs=23385;toolchainInputs=44;extraInputs=$taskExtraInputs;changes=$taskChanges;status=if($taskChanges.Count){'FAIL'}else{'PASS'}}
    Write-TaskEvidence "$taskRun/consumer-result.json" @{status=if($taskFailed){'FAIL; RETAIN ACTUAL RESULTS'}else{'FOCAL CONSUMER FINISHED; PARENT/GUARDS/PE/ROOT REQUIRED'};nativeResults=$taskResults;cases=$taskCases;changedPaths=$taskChanges;payloadStatus=$taskPayloadStatus;rootExecuted=$false;engineAccepted=$false}
}
if($taskFailed){exit 1}
exit 0
