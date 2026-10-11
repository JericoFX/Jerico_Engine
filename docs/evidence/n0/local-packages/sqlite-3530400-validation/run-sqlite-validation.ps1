$ErrorActionPreference='Stop'
$repo='F:\JericoEngine\JericoEngine'
$audit=Join-Path $repo 'build\n0-validation\local-packages\sqlite-3530400-audit'
$root=Join-Path $repo 'build\n0-validation\local-packages\sqlite-3530400-validation'
$cmake='C:\Program Files\CMake\bin\cmake.exe'
$freezePath=Join-Path $audit 'source-freeze-sqlite-current.json'
$argsPath=Join-Path $audit 'validation-arguments.json'
$freezeExpected='BA64F11199BBDC4A9CD9A613D40E74B0A0FD864264725295852082FC55517518'
$freezeHash=(Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash
if($freezeHash -ne $freezeExpected){throw 'SQLite source freeze hash mismatch; no verifier launched'}
$freeze=Get-Content -LiteralPath $freezePath -Raw|ConvertFrom-Json
if($freeze.fileCount -ne 14 -or $freeze.files.Count -ne 14){throw 'SQLite freeze entry count mismatch; no verifier launched'}
$freezeAudit=@()
foreach($item in $freeze.files){if(-not(Test-Path -LiteralPath $item.path -PathType Leaf)){throw "frozen input missing: $($item.path)"};$file=Get-Item -LiteralPath $item.path;$hash=(Get-FileHash -LiteralPath $item.path -Algorithm SHA256).Hash;$match=($file.Length -eq $item.bytes -and $hash -ieq $item.sha256);$freezeAudit += [pscustomobject]@{path=$item.path;bytes=$file.Length;sha256=$hash;matches=$match};if(-not $match){throw "frozen input mismatch: $($item.path)"}}
$freezeAudit|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $root 'freeze-before.json') -Encoding utf8
$arguments=Get-Content -LiteralPath $argsPath -Raw|ConvertFrom-Json
$sourceRoot='F:\JericoEngine\Dependencies\sqlite-amalgamation-3530400'
$archive='F:\JericoEngine\Dependencies\archives\sqlite-amalgamation-3530400.zip'
$verifier='F:\JericoEngine\JericoEngine\cmake\Tools\LocalPackages\SQLite\VerifySQLiteSource.cmake'
$sourceFiles=@($freeze.files|Where-Object {$_.path -like 'F:\JericoEngine\Dependencies\sqlite-amalgamation-3530400\*'})
$expectedSuccess='exact official 3.53.4 archive and four source blobs verified'
$caseSpecs=@(
    [pscustomobject]@{name='positive';sourceKind='copy';sourceValue=$null;expected=$expectedSuccess;success=$true;mutate='none'},
    [pscustomobject]@{name='URI';sourceKind='override';sourceValue='https://invalid.example/sqlite';expected='JE local Python: JE_SQLITE_SOURCE_DIR must not be a URI';success=$false;mutate='none'},
    [pscustomobject]@{name='UNC';sourceKind='override';sourceValue='//invalid.example/share/sqlite';expected='JE local Python: JE_SQLITE_SOURCE_DIR must be an absolute local path';success=$false;mutate='none'},
    [pscustomobject]@{name='Missing';sourceKind='missing';sourceValue=$null;expected='JE SQLite: source directory is missing';success=$false;mutate='none'},
    [pscustomobject]@{name='BlobChanged';sourceKind='copy';sourceValue=$null;expected='SHA256 mismatch';success=$false;mutate='blob'},
    [pscustomobject]@{name='ExtraEntry';sourceKind='copy';sourceValue=$null;expected='source must contain exactly the four audited release files';success=$false;mutate='extra'},
    [pscustomobject]@{name='ArchiveTamper';sourceKind='copy';sourceValue=$null;expected='SHA256 mismatch';success=$false;mutate='archive'}
)
$summaryCases=@()
foreach($spec in $caseSpecs){
    $caseRoot=Join-Path $root $spec.name
    if(Test-Path -LiteralPath $caseRoot){throw "case output already exists: $caseRoot"}
    $fixtures=Join-Path $caseRoot 'fixtures'
    [IO.Directory]::CreateDirectory($fixtures)|Out-Null
    $caseSource=Join-Path $fixtures 'source'
    $caseArchive=Join-Path $fixtures 'sqlite-amalgamation-3530400.zip'
    foreach($item in $sourceFiles){$fileName=Split-Path -Leaf $item.path;if(-not(Test-Path -LiteralPath $caseSource)){[IO.Directory]::CreateDirectory($caseSource)|Out-Null};Copy-Item -LiteralPath $item.path -Destination (Join-Path $caseSource $fileName)}
    Copy-Item -LiteralPath $archive -Destination $caseArchive
    if($spec.mutate -eq 'blob'){$target=Join-Path $caseSource 'sqlite3.h';$stream=[IO.File]::Open($target,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None);try{$byte=$stream.ReadByte();if($byte -lt 0){throw 'sqlite3.h unexpectedly empty'};$stream.Position=0;$stream.WriteByte([byte]($byte -bxor 1))}finally{$stream.Dispose()}}
    if($spec.mutate -eq 'extra'){[IO.File]::WriteAllText((Join-Path $caseSource 'extra.txt'),'unexpected entry',[Text.UTF8Encoding]::new($false))}
    if($spec.mutate -eq 'archive'){$stream=[IO.File]::Open($caseArchive,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None);try{$stream.Position=$stream.Length-1;$byte=$stream.ReadByte();$stream.Position=$stream.Length-1;$stream.WriteByte([byte]($byte -bxor 1))}finally{$stream.Dispose()}}
    if($spec.sourceKind -eq 'override'){$sourceArg=$spec.sourceValue}elseif($spec.sourceKind -eq 'missing'){$sourceArg=Join-Path $fixtures 'missing-source'}else{$sourceArg=$caseSource}
    if($spec.name -in @('URI','UNC')){$sourceDef="-DJE_SQLITE_SOURCE_DIR:STRING=$sourceArg"}else{$sourceDef="-DJE_SQLITE_SOURCE_DIR:PATH=$sourceArg"}
    $argv=[string[]]@($sourceDef,"-DJE_SQLITE_SOURCE_ARCHIVE:FILEPATH=$caseArchive",'-P',$verifier)
    if($argv.Count -ne 4 -or $argv[2] -ne '-P' -or $argv[3] -ne $verifier){throw "argv guard failed for $($spec.name); not launching"}
    $fixtureFiles=@(Get-ChildItem -LiteralPath $fixtures -Recurse -File -Force|ForEach-Object{$rel=$_.FullName.Substring($fixtures.Length+1).Replace('\','/');[pscustomobject]@{path=$rel;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}})
    $pre=[ordered]@{case=$spec.name;freeze_sha256=$freezeHash;source_arg=$sourceArg;source_arg_type=if($spec.name -in @('URI','UNC')){'STRING'}else{'PATH'};archive_arg=$caseArchive;archive_sha256=(Get-FileHash -LiteralPath $caseArchive -Algorithm SHA256).Hash;fixture_files=$fixtureFiles;executable=$cmake;argv=$argv;argc=$argv.Count;cwd=(Get-Location).Path;expected=$spec.expected;start_utc=[DateTime]::UtcNow.ToString('o')}
    $pre|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $caseRoot 'inputs-before.json') -Encoding utf8
    $start=[Diagnostics.ProcessStartInfo]::new();$start.FileName=$cmake;$start.WorkingDirectory=(Get-Location).Path;$start.UseShellExecute=$false;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
    foreach($arg in $argv){[void]$start.ArgumentList.Add($arg)}
    $process=[Diagnostics.Process]::new();$process.StartInfo=$start;$timer=[Diagnostics.Stopwatch]::StartNew();if(-not $process.Start()){throw "Process.Start failed for $($spec.name)"}
    $stdoutTask=$process.StandardOutput.ReadToEndAsync();$stderrTask=$process.StandardError.ReadToEndAsync();$process.WaitForExit();$stdout=$stdoutTask.GetAwaiter().GetResult();$stderr=$stderrTask.GetAwaiter().GetResult();$exit=$process.ExitCode;$timer.Stop();$process.Dispose()
    [IO.File]::WriteAllText((Join-Path $caseRoot 'stdout.log'),$stdout,[Text.UTF8Encoding]::new($false));[IO.File]::WriteAllText((Join-Path $caseRoot 'stderr.log'),$stderr,[Text.UTF8Encoding]::new($false))
    $combined="$stdout`n$stderr";$diagMatch=$combined.Contains($spec.expected);$caseFileAfter=@(Get-ChildItem -LiteralPath $fixtures -Recurse -File -Force|ForEach-Object{$rel=$_.FullName.Substring($fixtures.Length+1).Replace('\','/');[pscustomobject]@{path=$rel;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}})
    $sameFixture=($fixtureFiles.Count -eq $caseFileAfter.Count);if($sameFixture){for($i=0;$i -lt $fixtureFiles.Count;$i++){if($fixtureFiles[$i].path -ne $caseFileAfter[$i].path -or $fixtureFiles[$i].bytes -ne $caseFileAfter[$i].bytes -or $fixtureFiles[$i].sha256 -ne $caseFileAfter[$i].sha256){$sameFixture=$false;break}}}
    $unexpected=@(Get-ChildItem -LiteralPath $caseRoot -Recurse -File -Force|Where-Object {$_.Name -eq 'PackageInfo.json' -or $_.Extension -in @('.tar','.xz')})
    $expectExit=if($spec.success){0}else{1};$passed=($exit -eq $expectExit -and $diagMatch -and $sameFixture -and $unexpected.Count -eq 0)
    $result=[ordered]@{case=$spec.name;exit_code=$exit;elapsed_seconds=$timer.Elapsed.TotalSeconds;end_utc=[DateTime]::UtcNow.ToString('o');helper_freeze_sha256=(Get-FileHash -LiteralPath $verifier -Algorithm SHA256).Hash;path_helper_sha256=(Get-FileHash -LiteralPath (Join-Path $repo 'cmake\Tools\LocalPackages\Python\JERequireLocalPath.cmake') -Algorithm SHA256).Hash;archive_sha256_after=(Get-FileHash -LiteralPath $caseArchive -Algorithm SHA256).Hash;fixture_files_after=$caseFileAfter;diagnostic_match=$diagMatch;expected_diagnostic=$spec.expected;fixture_unchanged_during_cmake=$sameFixture;unexpected_package_artifact_count=$unexpected.Count;passed=$passed;stdout_bytes=([Text.Encoding]::UTF8.GetByteCount($stdout));stderr_bytes=([Text.Encoding]::UTF8.GetByteCount($stderr))}
    $result|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $caseRoot 'result.json') -Encoding utf8
    $summaryCases += [pscustomobject]@{case=$spec.name;exit_code=$exit;elapsed_seconds=$timer.Elapsed.TotalSeconds;diagnostic_match=$diagMatch;fixture_unchanged=$sameFixture;unexpected_artifacts=$unexpected.Count;passed=$passed;result_path=(Join-Path $caseRoot 'result.json')}
    if(-not $passed){$summaryCases|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $root 'cases-summary.json') -Encoding utf8;throw "SQLite verifier case failed: $($spec.name)"}
}
$postAudit=@();foreach($item in $freeze.files){$file=Get-Item -LiteralPath $item.path;$hash=(Get-FileHash -LiteralPath $item.path -Algorithm SHA256).Hash;$match=($file.Length -eq $item.bytes -and $hash -ieq $item.sha256);$postAudit += [pscustomobject]@{path=$item.path;bytes=$file.Length;sha256=$hash;matches=$match}}
$postFailures=@($postAudit|Where-Object{-not $_.matches})
$post=[ordered]@{freeze_sha256=(Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash;file_count=$postAudit.Count;all_inputs_match=($postFailures.Count -eq 0);failures=$postFailures;cases=$summaryCases}
$post|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $root 'cases-summary.json') -Encoding utf8
$postAudit|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $root 'freeze-after.json') -Encoding utf8
if($postFailures.Count -ne 0){throw 'SQLite freeze postflight mismatch'}
$summaryCases|ConvertTo-Json -Depth 5