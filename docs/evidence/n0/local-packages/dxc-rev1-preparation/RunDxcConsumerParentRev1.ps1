#Requires -Version 7.0
param([Parameter(Mandatory=$true)][string]$DriverSHA256)
$ErrorActionPreference='Stop'
$taskRepo='F:/JericoEngine/JericoEngine'
$taskBase="$taskRepo/build/n0-validation/local-packages"
$taskRun="$taskBase/dxc-rev1-validation/consumer-attempt1"
$taskExtract="$taskBase/dxc-native-extracted-rev1"
$taskConsumer="$taskBase/dxc-native-consumer-rev1"
$taskFreezePath="$taskBase/dxc-rev1-preparation/provider-candidate-rev2/source-freeze-dxc-provider-rev2-final.json"
$taskAssignmentPath="$taskBase/dxc-rev1-preparation/provider-candidate-rev2/validation-assignment.json"
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
foreach($path in @($taskRun,$taskExtract,$taskConsumer)){
    $full=Assert-TaskLocalPath $path
    if(-not $full.StartsWith([IO.Path]::GetFullPath("$taskRepo/build/"),[StringComparison]::OrdinalIgnoreCase)){throw 'Output escaped checkout/build'}
    if([IO.Directory]::Exists($full) -or [IO.File]::Exists($full)){throw "Output exists; preserve it: $full"}
}
Require-TaskHash $taskFreezePath '123AE0639F373D0EF16E6147FC74765CBCBBD7AF42CCEF1535D58A5B7A1C5CCD'
Require-TaskHash $taskAssignmentPath '5968A5F2785CC65BEBA8AD14EAB987C429645F2EC5BB3CCBBE3FA21902CE9585'
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
if($taskFreeze.files.Count -ne 23348 -or $taskFreeze.providerFileCount -ne 12 -or $taskFreeze.parallelJobs -ne 6){throw 'Unexpected frozen candidate'}
if($taskAssignment.package.extractedRoot.Replace([char]92,[char]47) -ne $taskExtract -or $taskAssignment.consumerBuild.argv -notcontains $taskConsumer.Replace([char]47,[char]92)){throw 'Consumer assignment differs'}
$taskExtraInputs=@(
    @{path='C:/Program Files/CMake/bin/ctest.exe';bytes=14012488;sha256='73BAACBEB272CA6F40422B4F789403390AF678BEB491783CEF1727D69CD3E1CB'},
    @{path=$PSCommandPath;bytes=[IO.FileInfo]::new($PSCommandPath).Length;sha256=$DriverSHA256},
    @{path=$taskArchive;bytes=7352940;sha256=$taskArchiveSHA},
    @{path="$taskArchive.SHA256SUMS";bytes=131;sha256='8A9C9F9F562F099559770A5770B1465928471416181CEBC38198B2FEFF2E627E'}
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
Write-TaskEvidence "$taskRun/preflight.json" @{sourceInputs=23348;toolchainInputs=44;extraInputs=$taskExtraInputs;fourMembershipRoots=$taskMembershipRoots;changes=@();status='PASS'}
$taskFailed=$false;$taskResults=[ordered]@{};$taskCases=@();$taskPayload=@()
try{
    $env:PYTHONDONTWRITEBYTECODE='1'
    $taskPython="$taskRepo/build/n0-validation/original-engine-deps/Python-rev3/packages/python-3.10.13-je-rev3-windows/python/python.exe"
    # This isolated stdlib-only auditor reads the actual tar; it never extracts or ships Python.
    $taskAuditCode=@'
import hashlib,json,pathlib,re,sys,tarfile
archive,expected_hash,pins_path,output=sys.argv[1:]
assert sys.version_info[:3]==(3,10,13), 'External Python version differs'
def sha(data): return hashlib.sha256(data).hexdigest().upper()
def hashfile(path):
    with open(path,'rb') as f: return sha(f.read())
assert hashfile(archive)==expected_hash, 'Actual archive SHA differs'
names={
'DirectXShaderCompilerDxc/JERecipe.json','DirectXShaderCompilerDxc/bin/dxc.exe',
'DirectXShaderCompilerDxc/bin/dxcompiler.dll','DirectXShaderCompilerDxc/bin/dxil.dll',
'DirectXShaderCompilerDxc/bin/dxsc.exe','FindDirectXShaderCompilerDxc.cmake',
'JERequireLocalPath.cmake','LICENSES/O3DE/LICENSE.txt','LICENSES/O3DE/LICENSE_APACHE2.TXT',
'LICENSES/O3DE/LICENSE_MIT.TXT','LICENSES/SourceNotices.txt','NOTICE.txt',
'PackageInfo.json','README.md','SHA256SUMS'}
payload={}; content={}; seen=set()
with tarfile.open(archive,'r:xz') as tf:
    for member in tf:
        n=member.name
        assert n in names and n not in seen, 'Unexpected or duplicate member: '+n
        assert member.isfile() and not member.issym() and not member.islnk(), 'Nonregular member: '+n
        assert not n.startswith(('/', '\\')) and '\\' not in n and ':' not in n and '..' not in pathlib.PurePosixPath(n).parts, 'Unsafe member: '+n
        assert 0 < member.size < 250_000_000, 'Unexpected member size: '+n
        seen.add(n)
        data=tf.extractfile(member).read()
        assert len(data)==member.size, 'Short member: '+n
        payload[n]={'name':n,'bytes':len(data),'sha256':sha(data)}
        if n in ('PackageInfo.json','DirectXShaderCompilerDxc/JERecipe.json','SHA256SUMS'): content[n]=data
assert seen==names and len(payload)==15, 'Archive membership differs'
manifest={}
for line in content['SHA256SUMS'].decode('utf-8').splitlines():
    match=re.fullmatch(r'([a-f0-9]{64}) \*(.+)',line)
    assert match and match[2] in names-{'SHA256SUMS'} and match[2] not in manifest, 'Bad manifest record'
    manifest[match[2]]=match[1].upper()
assert set(manifest)==names-{'SHA256SUMS'} and len(manifest)==14, 'Manifest membership differs'
assert all(payload[n]['sha256']==h for n,h in manifest.items()), 'Member content SHA differs'
info=json.loads(content['PackageInfo.json']); recipe=json.loads(content['DirectXShaderCompilerDxc/JERecipe.json'])
pins=json.loads(pathlib.Path(pins_path).read_text(encoding='utf-8'))
assert info['schemaVersion']==1 and info['name']=='DirectXShaderCompilerDxc-1.8.2505.1-o3de-je-rev1-windows' and info['version']=='1.8.2505.1', 'Package identity differs'
assert info['source']=={k:pins['sources'][0][k] for k in ('url','commit','sha256')}, 'Package source provenance differs'
assert all(recipe.get(k)==v for k,v in pins.items()), 'Recipe pins differ'
extra={'sourceNumericVersion':'1.8.2505.0','sourceShortHash':'1cc0ab76','historyCount':4954,
'historyCountMethod':'Pinned GitHub REST unfiltered history cardinality; not local git rev-list',
'externalPythonToolSHA256':'6235abe2248c14ad86dfe00dbb7bc2396eb4b91f663412c211905e279bce9d6e',
'pythonDistributed':False,'generator':'Ninja','toolset':'v143','parallelJobs':6}
assert all(recipe.get(k)==v for k,v in extra.items()) and set(recipe)==set(pins)|set(extra), 'Recipe closure differs'
assert payload['LICENSES/SourceNotices.txt']['sha256']==pins['sourceNoticeSHA256'].upper(), 'Original notices differ'
result={'status':'PASS','archive':archive,'sha256':expected_hash,'memberCount':15,'manifestRecords':14,
'files':[payload[n] for n in sorted(payload)],'packageInfo':info,'recipe':recipe,
'scope':'Actual produced archive audit before CMake extraction; no consumer or engine acceptance'}
with open(output,'x',encoding='utf-8',newline='\n') as f: json.dump(result,f,indent=2);f.write('\n')
print('Actual DXC tar audit: 15 regular members, 14 verified payload hashes; no Python payload')
'@
    $taskAuditPath="$taskRun/archive-audit.json"
    $taskResults['archiveAudit']=Invoke-TaskNative 'archive-audit' $taskPython @('-I','-B','-c',$taskAuditCode,$taskArchive,$taskArchiveSHA,"$taskRepo/cmake/Tools/LocalPackages/DirectXShaderCompilerDxc/SourcePins.json",$taskAuditPath) $taskRepo
    if($taskResults['archiveAudit'].nativeExit -ne 0){throw 'Actual archive audit failed; no extraction'}
    $taskAudit=Get-Content -Raw -LiteralPath $taskAuditPath | ConvertFrom-Json
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
    [void][IO.Directory]::CreateDirectory($taskExtract)
    $taskResults['extraction']=Invoke-TaskNative 'extraction' $taskAssignment.consumerConfigure.executable @('-E','tar','xf',$taskArchive) $taskExtract
    if($taskResults['extraction'].nativeExit -ne 0){throw 'Actual archive extraction failed'}
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
    Write-TaskEvidence "$taskRun/postflight.json" @{sourceInputs=23348;toolchainInputs=44;extraInputs=$taskExtraInputs;changes=$taskChanges;status=if($taskChanges.Count){'FAIL'}else{'PASS'}}
    Write-TaskEvidence "$taskRun/consumer-result.json" @{status=if($taskFailed){'FAIL; RETAIN ACTUAL RESULTS'}else{'FOCAL CONSUMER FINISHED; PARENT/GUARDS/PE/ROOT REQUIRED'};nativeResults=$taskResults;cases=$taskCases;changedPaths=$taskChanges;payloadStatus=$taskPayloadStatus;rootExecuted=$false;engineAccepted=$false}
}
if($taskFailed){exit 1}
exit 0
