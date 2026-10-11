$ErrorActionPreference='Stop'
$repo='F:\JericoEngine\JericoEngine'
$audit=Join-Path $repo 'build\n0-validation\local-packages\sqlite-3530400-audit'
$root=Join-Path $repo 'build\n0-validation\local-packages\sqlite-3530400-validation'
$cmake='C:\Program Files\CMake\bin\cmake.exe'
$freezePath=Join-Path $audit 'source-freeze-sqlite-current.json'
$freeze=Get-Content -LiteralPath $freezePath -Raw|ConvertFrom-Json
$archive='F:\JericoEngine\Dependencies\archives\sqlite-amalgamation-3530400.zip'
$verifier='F:\JericoEngine\JericoEngine\cmake\Tools\LocalPackages\SQLite\VerifySQLiteSource.cmake'
$pathHelper=Join-Path $repo 'cmake\Tools\LocalPackages\Python\JERequireLocalPath.cmake'
$expectedFreeze='BA64F11199BBDC4A9CD9A613D40E74B0A0FD864264725295852082FC55517518'
$expectedVerifier='930EE4014DABEED4259A93FE8711204171F09371CAF44DD0C73AA596EE20D803'
$expectedPathHelper='4B74565484AC8EA233D6D899E74D3128E1023B92F20DA3BE2CCB540EFFA05143'
if((Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash -ne $expectedFreeze){throw 'historical freeze manifest changed'}
if((Get-FileHash -LiteralPath $verifier -Algorithm SHA256).Hash -ne $expectedVerifier -or (Get-FileHash -LiteralPath $pathHelper -Algorithm SHA256).Hash -ne $expectedPathHelper){throw 'SQLite verifier inputs changed; no cases launched'}
$sourceItems=@($freeze.files|Where-Object {$_.path -like 'F:\JericoEngine\Dependencies\sqlite-amalgamation-3530400\*'})
$focusItems=@($freeze.files|Where-Object {$_.path -eq $verifier -or $_.path -eq $pathHelper -or $_.path -eq $archive -or $_.path -like 'F:\JericoEngine\Dependencies\sqlite-amalgamation-3530400\*'})
if($sourceItems.Count -ne 4 -or $focusItems.Count -ne 7){throw 'unexpected focused source manifest count'}
$focusAudit=@()
foreach($item in $focusItems){$fi=Get-Item -LiteralPath $item.path;$h=(Get-FileHash -LiteralPath $item.path -Algorithm SHA256).Hash;$ok=($fi.Length -eq $item.bytes -and $h -ieq $item.sha256);$focusAudit += [pscustomobject]@{path=$item.path;bytes=$fi.Length;sha256=$h;matches=$ok};if(-not $ok){throw "focused input mismatch: $($item.path)"}}
$focusAudit|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $root 'focused-inputs-before.json') -Encoding utf8
$failure=[ordered]@{case='BlobChanged initial fixture setup';status='NOT_RUN';reason='PowerShell could not open copied read-only sqlite3.h for fixture mutation; no CMake invocation occurred';exception='UnauthorizedAccessException';original_fixture='BlobChanged/fixtures/source/sqlite3.h';corrected_run='BlobChanged-corrected'}
$failure|ConvertTo-Json -Depth 3|Set-Content -LiteralPath (Join-Path $root 'BlobChanged\harness-setup-failure.json') -Encoding utf8
$specs=@(
 [pscustomobject]@{name='BlobChanged';folder='BlobChanged-corrected';mutate='blob';expected='SHA256 mismatch'},
 [pscustomobject]@{name='ExtraEntry';folder='ExtraEntry';mutate='extra';expected='source must contain exactly the four audited release files'},
 [pscustomobject]@{name='ArchiveTamper';folder='ArchiveTamper';mutate='archive';expected='SHA256 mismatch'}
)
foreach($spec in $specs){
 $caseRoot=Join-Path $root $spec.folder
 if(Test-Path -LiteralPath $caseRoot){throw "case output exists: $caseRoot"}
 $fixtures=Join-Path $caseRoot 'fixtures';$caseSource=Join-Path $fixtures 'source'
 [IO.Directory]::CreateDirectory($caseSource)|Out-Null
 foreach($item in $sourceItems){
  $dest=Join-Path $caseSource (Split-Path -Leaf $item.path);Copy-Item -LiteralPath $item.path -Destination $dest
  $fi=Get-Item -LiteralPath $dest -Force;$fi.Attributes=$fi.Attributes -band (-bnot [IO.FileAttributes]::ReadOnly)
 }
 $caseArchive=Join-Path $fixtures 'sqlite-amalgamation-3530400.zip';Copy-Item -LiteralPath $archive -Destination $caseArchive
 $fi=Get-Item -LiteralPath $caseArchive -Force;$fi.Attributes=$fi.Attributes -band (-bnot [IO.FileAttributes]::ReadOnly)
 if($spec.mutate -eq 'blob'){
  $target=Join-Path $caseSource 'sqlite3.h';$stream=[IO.File]::Open($target,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
  try{$byte=$stream.ReadByte();if($byte -lt 0){throw 'sqlite3.h unexpectedly empty'};$stream.Position=0;$stream.WriteByte([byte]($byte -bxor 1))}finally{$stream.Dispose()}
 }
 if($spec.mutate -eq 'extra'){[IO.File]::WriteAllText((Join-Path $caseSource 'extra.txt'),'unexpected entry',[Text.UTF8Encoding]::new($false))}
 if($spec.mutate -eq 'archive'){
  $stream=[IO.File]::Open($caseArchive,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
  try{$stream.Position=$stream.Length-1;$byte=$stream.ReadByte();$stream.Position=$stream.Length-1;$stream.WriteByte([byte]($byte -bxor 1))}finally{$stream.Dispose()}
 }
 $argv=[string[]]@("-DJE_SQLITE_SOURCE_DIR:PATH=$caseSource","-DJE_SQLITE_SOURCE_ARCHIVE:FILEPATH=$caseArchive",'-P',$verifier)
 if($argv.Count -ne 4 -or $argv[2] -ne '-P' -or $argv[3] -ne $verifier){throw "argv guard failed: $($spec.name)"}
 $fixtureBefore=@(Get-ChildItem -LiteralPath $fixtures -Recurse -File -Force|ForEach-Object{$rel=$_.FullName.Substring($fixtures.Length+1).Replace('\','/');[pscustomobject]@{path=$rel;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}})
 $pre=[ordered]@{case=$spec.name;freeze_sha256=$expectedFreeze;verifier_sha256=(Get-FileHash -LiteralPath $verifier -Algorithm SHA256).Hash;path_helper_sha256=(Get-FileHash -LiteralPath $pathHelper -Algorithm SHA256).Hash;source_arg=$caseSource;archive_arg=$caseArchive;argv=$argv;argc=$argv.Count;cwd=(Get-Location).Path;fixture_files=$fixtureBefore;expected_diagnostic=$spec.expected;start_utc=[DateTime]::UtcNow.ToString('o')}
 $pre|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $caseRoot 'inputs-before.json') -Encoding utf8
 $start=[Diagnostics.ProcessStartInfo]::new();$start.FileName=$cmake;$start.WorkingDirectory=(Get-Location).Path;$start.UseShellExecute=$false;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
 foreach($arg in $argv){[void]$start.ArgumentList.Add($arg)}
 $p=[Diagnostics.Process]::new();$p.StartInfo=$start;$timer=[Diagnostics.Stopwatch]::StartNew();if(-not $p.Start()){throw "Process.Start failed: $($spec.name)"}
 $stdoutTask=$p.StandardOutput.ReadToEndAsync();$stderrTask=$p.StandardError.ReadToEndAsync();$p.WaitForExit();$stdout=$stdoutTask.GetAwaiter().GetResult();$stderr=$stderrTask.GetAwaiter().GetResult();$nativeExit=$p.ExitCode;$timer.Stop();$p.Dispose()
 [IO.File]::WriteAllText((Join-Path $caseRoot 'stdout.log'),$stdout,[Text.UTF8Encoding]::new($false));[IO.File]::WriteAllText((Join-Path $caseRoot 'stderr.log'),$stderr,[Text.UTF8Encoding]::new($false))
 $fixtureAfter=@(Get-ChildItem -LiteralPath $fixtures -Recurse -File -Force|ForEach-Object{$rel=$_.FullName.Substring($fixtures.Length+1).Replace('\','/');[pscustomobject]@{path=$rel;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}})
 $same=($fixtureBefore.Count -eq $fixtureAfter.Count)
 if($same){for($i=0;$i -lt $fixtureBefore.Count;$i++){if($fixtureBefore[$i].path -ne $fixtureAfter[$i].path -or $fixtureBefore[$i].bytes -ne $fixtureAfter[$i].bytes -or $fixtureBefore[$i].sha256 -ne $fixtureAfter[$i].sha256){$same=$false;break}}}
 $unexpected=@(Get-ChildItem -LiteralPath $caseRoot -Recurse -File -Force|Where-Object{$_.Name -eq 'PackageInfo.json' -or $_.Extension -in @('.tar','.xz')})
 $diagnostic=([string]::Concat($stdout,[Environment]::NewLine,$stderr)).Contains($spec.expected)
 $pass=($nativeExit -eq 1 -and $diagnostic -and $same -and $unexpected.Count -eq 0)
 $result=[ordered]@{case=$spec.name;exit_code=$nativeExit;elapsed_seconds=$timer.Elapsed.TotalSeconds;end_utc=[DateTime]::UtcNow.ToString('o');verifier_sha256=(Get-FileHash -LiteralPath $verifier -Algorithm SHA256).Hash;path_helper_sha256=(Get-FileHash -LiteralPath $pathHelper -Algorithm SHA256).Hash;archive_sha256_after=(Get-FileHash -LiteralPath $caseArchive -Algorithm SHA256).Hash;fixture_files_after=$fixtureAfter;diagnostic_match=$diagnostic;expected_diagnostic=$spec.expected;fixture_unchanged_during_cmake=$same;unexpected_package_artifact_count=$unexpected.Count;stdout_bytes=([Text.Encoding]::UTF8.GetByteCount($stdout));stderr_bytes=([Text.Encoding]::UTF8.GetByteCount($stderr));passed=$pass}
 $result|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $caseRoot 'result.json') -Encoding utf8
 if(-not $pass){throw "SQLite case failed: $($spec.name)"}
}
$focusAfter=@()
foreach($item in $focusItems){$fi=Get-Item -LiteralPath $item.path;$h=(Get-FileHash -LiteralPath $item.path -Algorithm SHA256).Hash;$ok=($fi.Length -eq $item.bytes -and $h -ieq $item.sha256);$focusAfter += [pscustomobject]@{path=$item.path;bytes=$fi.Length;sha256=$h;matches=$ok}}
$focusAfter|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $root 'focused-inputs-after.json') -Encoding utf8
if(@($focusAfter|Where-Object{-not $_.matches}).Count -ne 0){throw 'focused SQLite input postflight mismatch'}