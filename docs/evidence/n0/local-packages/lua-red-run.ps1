$ErrorActionPreference='Continue'
$root='F:\JericoEngine\JericoEngine'
$base=Join-Path $root 'build\n0-validation\local-packages'
$red=Join-Path $base 'lua-red-unpatched'
$evidence=Join-Path $base 'lua-red-evidence'
$freezePath=Join-Path $base 'source-freeze-lua-parent.json'
$freezeExpected='322BBCDE2DFD6A4B6F9007889BA81E10F09C534A9F8A602034726FFBCF3DBF59'
$archive='F:\JericoEngine\Dependencies\archives\lua-5.4.4-source.tar.gz'
$archiveExpected='2ff89bbea22a7c8b910d6b006f5ba4559edcf468a25537cedf9988ed90f6b450'
$recipe=Join-Path $root 'cmake\Tools\LocalPackages\Lua'
if ((Get-Location).Path -ne $base) { throw 'Unexpected CWD' }
if ((Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash -ne $freezeExpected) { throw 'Lua source freeze SHA mismatch' }
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $archiveExpected) { throw 'Lua archive SHA mismatch' }
if (Test-Path -LiteralPath $red) { throw 'RED output already exists' }
if (Test-Path -LiteralPath $evidence) { throw 'RED evidence directory already exists' }
New-Item -ItemType Directory -Path $red,$evidence | Out-Null
function Get-FreezeState {
  $spec=Get-Content -LiteralPath $freezePath -Raw | ConvertFrom-Json
  $files=@($spec | ForEach-Object { $p=Join-Path $root $_.path; $h=(Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant(); [pscustomobject]@{path=$_.path;expected=$_.sha256;actual=$h;match=($h -eq $_.sha256)} })
  [pscustomobject]@{manifest_sha256=(Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash; files=$files; all_match=(@($files|Where-Object{-not $_.match}).Count -eq 0)}
}
function Invoke-Logged([string]$name,[string]$exe,[string[]]$argv,[int]$expectedCount) {
  if ($argv.Count -ne $expectedCount) { throw "$name argv count $($argv.Count), expected $expectedCount" }
  $record=[pscustomobject]@{cwd=(Get-Location).Path; executable=$exe; argv=$argv; argv_count=$argv.Count; started=(Get-Date).ToString('o')}
  $record | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $evidence "$name.argv.json") -Encoding utf8
  $timer=[Diagnostics.Stopwatch]::StartNew()
  & $exe @argv 2>&1 | Tee-Object -FilePath (Join-Path $evidence "$name.log")
  $code=$LASTEXITCODE
  $timer.Stop()
  $timer.Elapsed.TotalSeconds.ToString('F6',[Globalization.CultureInfo]::InvariantCulture) | Set-Content -LiteralPath (Join-Path $evidence "$name.elapsed.txt") -NoNewline
  $code | Set-Content -LiteralPath (Join-Path $evidence "$name.exitcode") -NoNewline
  return $code
}
$before=Get-FreezeState
if (-not $before.all_match) { throw 'Lua source freeze inputs changed before RED' }
$archiveBefore=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
$members=@(& tar.exe -tzf $archive 2>&1)
$tarListExit=$LASTEXITCODE
if ($tarListExit -ne 0) { throw "tar listing failed $tarListExit" }
if (@($members | Where-Object { $_ -notmatch '^lua-5\.4\.4/' -or $_ -match '(^|/)\.\.(?:/|$)' -or $_ -match '^/' }).Count -ne 0) { throw 'Lua archive has unsafe/unexpected path entries' }
$members | Set-Content -LiteralPath (Join-Path $evidence 'lua-archive-members.txt')
$extract=Join-Path $red 'source'
New-Item -ItemType Directory -Path $extract | Out-Null
$tarArgs=[string[]]@('-xzf',$archive,'-C',$extract)
if ($tarArgs.Count -ne 4 -or $tarArgs[0] -ne '-xzf' -or $tarArgs[2] -ne '-C') { throw 'tar extraction argv guard failed' }
$tarExit=Invoke-Logged 'red-extract' 'tar.exe' $tarArgs 4
if ($tarExit -ne 0) { throw "Lua archive extraction failed $tarExit" }
$work=Join-Path $extract 'lua-5.4.4'
if (-not (Test-Path -LiteralPath (Join-Path $work 'onelua.c'))) { throw 'Unpatched archive source root missing onelua.c' }
$redBuild=Join-Path $red 'build-lualib'
$cfgArgs=[string[]]@('-S',$recipe,'-B',$redBuild,'-G','Visual Studio 17 2022','-A','x64','-T','v143',"-DJE_LUA_WORK:PATH=$work")
if ($cfgArgs.Count -ne 10 -or $cfgArgs[0] -ne '-S' -or $cfgArgs[2] -ne '-B' -or $cfgArgs[6] -ne '-A' -or $cfgArgs[8] -ne '-T') { throw 'RED configure argv guard failed' }
$cfgExit=Invoke-Logged 'red-lualib-configure' 'cmake.exe' $cfgArgs 10
if ($cfgExit -ne 0) { throw "RED Lua wrapper configure failed $cfgExit" }
$buildArgs=[string[]]@('--build',$redBuild,'--target','lualib','--config','Release')
if ($buildArgs.Count -ne 6) { throw 'RED lualib build argv guard failed' }
$libExit=Invoke-Logged 'red-lualib-build' 'cmake.exe' $buildArgs 6
if ($libExit -ne 0) { throw "RED unpatched Lua library build failed $libExit" }
$stage=Join-Path $red 'stage'
New-Item -ItemType Directory -Path (Join-Path $stage 'Lua\include\Lua'),(Join-Path $stage 'Lua\lib\release') | Out-Null
Copy-Item -LiteralPath (Join-Path $redBuild 'Release\lualib.lib') -Destination (Join-Path $stage 'Lua\lib\release\lualib.lib')
Copy-Item -Path (Join-Path $work '*.h') -Destination (Join-Path $stage 'Lua\include\Lua')
Copy-Item -LiteralPath (Join-Path $recipe 'FindLua.cmake') -Destination $stage
Copy-Item -LiteralPath (Join-Path $root 'cmake\Tools\LocalPackages\Python\JERequireLocalPath.cmake') -Destination $stage
$consumerBuild=Join-Path $red 'build-consumer'
$tests=Join-Path $recipe 'tests'
$consumerArgs=[string[]]@('-S',$tests,'-B',$consumerBuild,'-G','Visual Studio 17 2022','-A','x64',"-DJE_LUA_PACKAGE_ROOT:PATH=$stage")
if ($consumerArgs.Count -ne 9 -or $consumerArgs[0] -ne '-S' -or $consumerArgs[2] -ne '-B' -or $consumerArgs[6] -ne '-A') { throw 'RED consumer configure argv guard failed' }
$conCfgExit=Invoke-Logged 'red-consumer-configure' 'cmake.exe' $consumerArgs 9
if ($conCfgExit -ne 0) { throw "RED consumer configure failed $conCfgExit" }
$conBuildArgs=[string[]]@('--build',$consumerBuild,'--target','JELuaConsumer','--config','Release')
$conBuildExit=Invoke-Logged 'red-consumer-build' 'cmake.exe' $conBuildArgs 6
if ($conBuildExit -ne 0) { throw "RED consumer build failed $conBuildExit" }
$ctestArgs=[string[]]@('--test-dir',$consumerBuild,'-C','Release','--output-on-failure','-V')
$ctestExit=Invoke-Logged 'red-consumer-ctest' 'ctest.exe' $ctestArgs 6
$after=Get-FreezeState
$archiveAfter=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
$state=[pscustomobject]@{freeze_before=$before;freeze_after=$after;archive_sha_before=$archiveBefore;archive_sha_after=$archiveAfter;red_extract_exit=$tarExit;lualib_configure_exit=$cfgExit;lualib_build_exit=$libExit;consumer_configure_exit=$conCfgExit;consumer_build_exit=$conBuildExit;consumer_ctest_exit=$ctestExit;stage=$stage;library=[pscustomobject]@{path=(Join-Path $stage 'Lua\lib\release\lualib.lib');bytes=(Get-Item -LiteralPath (Join-Path $stage 'Lua\lib\release\lualib.lib')).Length;sha256=(Get-FileHash -LiteralPath (Join-Path $stage 'Lua\lib\release\lualib.lib') -Algorithm SHA256).Hash};headers=(Get-ChildItem -LiteralPath (Join-Path $stage 'Lua\include\Lua') -Filter '*.h' -File).Count}
$state | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $evidence 'red-result.json') -Encoding utf8
