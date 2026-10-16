# Private SDK83 resume: successful configure is reused, never re-run.
param([switch]$Execute)
$ErrorActionPreference='Stop'
if(!$Execute){Write-Output 'PREPARED_NOT_RUN: reuse successful configure83; no install/configure/quarantine, build or loader started.';return}
$repo='F:/JericoEngine/JericoEngine'
$area="$repo/build/n0-validation/native-render-builders-integration-rev1"
$build="$repo/build/n0-validation/original-engine"
$recipe=Get-Content -LiteralPath "$PSScriptRoot/recipe.json" -Raw|ConvertFrom-Json
$prep=Get-Content -LiteralPath "$PSScriptRoot/preparation.json" -Raw|ConvertFrom-Json
$sdk=$recipe.installedSdkPrefix
$binary=$recipe.consumerBuild
$out="$PSScriptRoot/execution-consume-resume-attempt2"
$previousOut="$PSScriptRoot/execution-consume"
$InstallStateSHA256='D9BB641D07877E7243C71152A50BBB6DC10CC8FDA3840ED6C431F3973861FA5A'
$QuarantineReviewPath="$PSScriptRoot/parent-quarantine-review83.json"
$QuarantineReviewSHA256='201FCAEE931ADDE4F1C4C55F0B9B60C723C4EE7708D54D0B788E7F73E824463F'
$EffectiveMaterialReviewPath=$prep.effectiveMaterialReview
$EffectiveMaterialReviewSHA256='02674917BC8E39D4A9A6E0BDC63AA61DB4171AD79DC28D70144D5D693480AB7E'
function Json($path) { Get-Content -LiteralPath $path -Raw | ConvertFrom-Json }
function Save($name, $data) {
    $stream = [IO.File]::Open("$out/$name", [IO.FileMode]::CreateNew)
    try { $bytes = [Text.UTF8Encoding]::new($false).GetBytes((ConvertTo-Json -InputObject $data -Depth 16)); $stream.Write($bytes, 0, $bytes.Length) } finally { $stream.Dispose() }
}
function Pin($path) { $f = Get-Item -LiteralPath $path; [pscustomobject]@{path=$f.FullName;bytes=$f.Length;sha256=(Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash} }
function Verify($pins) { foreach ($p in $pins) { $now = Pin $p.path; if ($now.bytes -ne $p.bytes -or $now.sha256 -cne $p.sha256) { throw "Input drift: $($p.path)" } } }
function NativeAdmission {
    $os = Get-CimInstance Win32_OperatingSystem
    $used = 100 * (1 - $os.FreePhysicalMemory / $os.TotalVisibleMemorySize)
    $freeGiB = $os.FreePhysicalMemory / 1MB
    if ($used -ge 70 -or $freeGiB -lt 10) { throw "Native admission requires RAM <70% and >=10 GiB free: used=$used freeGiB=$freeGiB" }
}
function VerifyFrozenCohort {
    Verify @($prep.freezePin, $prep.cachePin)
    $frozen = Json $prep.nativeFreeze
    if ($frozen.Revision -ne 82 -or @($frozen.inputs).Count -ne 14034) { throw 'Expected complete source82 cohort of14034.' }
    $unique = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($pin in $frozen.inputs) {
        if (!$pin.path -or !$unique.Add([IO.Path]::GetFullPath($pin.path))) { throw 'Empty/duplicate frozen input path.' }
    }
    Verify $frozen.inputs
    return $frozen
}
function VerifyProtected {
    Verify @($prep.protectedPin, $prep.ownerPin)
    $authored = Json $prep.protectedPin.path; $owner = Json $prep.ownerPin.path
    if (@($authored.inputs).Count -ne 40 -or @($owner.inputs).Count -ne 26) { throw 'Protected40/owner26 counts must be exact.' }
    Verify $authored.inputs; Verify $owner.inputs
}
function VerifyEffectiveMaterialGate {
    if (!$EffectiveMaterialReviewPath -or $EffectiveMaterialReviewSHA256 -cnotmatch '^[0-9A-F]{64}$' -or (Pin $EffectiveMaterialReviewPath).sha256 -cne $EffectiveMaterialReviewSHA256) { throw 'Explicit reviewed effective-Material77 receipt pin required.' }
    $gate = Json $EffectiveMaterialReviewPath
    if ($gate.approved -isnot [bool] -or !$gate.approved -or $gate.reviewer -cne '/root' -or $gate.revision -ne 77 -or $gate.freezeSHA256 -cne 'E00D49F4CF078AD10E3F171064DF6E13F629F14136138D8E5A064C4A1102A4AD') { throw 'Prior Material77 approval is valid only for identical current object/DLL/export pins; source82 is frozen independently.' }
    Verify @($gate.object, $gate.dll, $gate.exportReport)
    $buildRoot = [IO.Path]::GetFullPath($build) + [IO.Path]::DirectorySeparatorChar
    $objectPath = [IO.Path]::GetFullPath($gate.object.path)
    if (!$objectPath.StartsWith($buildRoot, [StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetExtension($objectPath) -ine '.obj' -or [IO.Path]::GetFullPath($gate.dll.path) -ine [IO.Path]::GetFullPath("$build/bin/profile/Atom_RPI.Public.dll")) { throw 'Effective Material object/DLL must be actual current producer outputs.' }
    $minimum = [DateTimeOffset]'2026-10-16T02:46:07Z'
    foreach ($pin in @($gate.object, $gate.dll)) {
        $actual = Get-Item -LiteralPath $pin.path
        if (!$pin.lastWriteUtc -or ([DateTimeOffset]$actual.LastWriteTimeUtc) -le $minimum -or ([DateTimeOffset]$actual.LastWriteTimeUtc) -ne ([DateTimeOffset]$pin.lastWriteUtc)) { throw "Effective Material output timestamp mismatch: $($pin.path)" }
    }
    $exports = [IO.File]::ReadAllText($gate.exportReport.path)
    foreach ($name in @('UpdateImageAssetSubscriptions','CancelPendingImageReload','OnAssetReloaded','OnSystemTick')) {
        if (!$exports.Contains($name + '@Material@RPI@AZ@@')) { throw "Effective Material export evidence missing: $name" }
    }
    return @{review=(Pin $EffectiveMaterialReviewPath);object=$gate.object;dll=$gate.dll;exportReport=$gate.exportReport}
}
function AssertProductRegistry($path) {
    $text = [IO.File]::ReadAllText($path)
    if ($text -notmatch 'Atom_RHI_Vulkan.Private' -or $text -match 'Atom_RHI_(?:DX12|Metal|Null)|Python') { throw "Product registry is not Vulkan-only: $path" }
}
function Resources {
    $os = Get-CimInstance Win32_OperatingSystem
    $used = 100 * (1 - $os.FreePhysicalMemory / $os.TotalVisibleMemorySize)
    if ($used -ge 80) { throw "RAM guard: $used" }
    $jobs = @(Get-CimInstance Win32_Process | Where-Object { $_.Name -match '^(cmake|MSBuild|cl|link|ctest|AzTestRunner|Editor|AssetProcessor|AssetProcessorBatch|AssetBuilder|MaterialEditor|MaterialCanvas|PassCanvas|ShaderManagementConsole|JENativeSdkConsumer)\.exe$' })
    if ($jobs.Count) { throw 'Another native phase is active; preserve its ownership.' }
}
function Run($phase, $exe, $arguments, $cwd, $pathValue = $null, $timeoutMs = 1800000) {
    Resources
    NativeAdmission
    $info = [Diagnostics.ProcessStartInfo]::new(); $info.FileName = $exe; $info.WorkingDirectory = $cwd
    $info.UseShellExecute = $false; $info.CreateNoWindow = $true; $info.RedirectStandardOutput = $true; $info.RedirectStandardError = $true
    $info.Environment['MSBUILDDISABLENODEREUSE'] = '1'
    if ($phase -eq 'build-consumer') { $info.Environment['_CL_'] = (([string]$info.Environment['_CL_']) + ' /MP1').Trim() }
    if ($pathValue) { $info.Environment['PATH'] = $pathValue }
    foreach ($arg in $arguments) { $info.ArgumentList.Add([string]$arg) }
    $process = [Diagnostics.Process]::new(); $process.StartInfo = $info
    $clock = [Diagnostics.Stopwatch]::StartNew()
    if (!$process.Start()) { throw "Cannot start $phase" }
    $ownedPid = $process.Id; $creation = $process.StartTime.ToUniversalTime()
    Save "$phase.start.json" @{pid=$ownedPid;creationUtc=$creation.ToString('o');executable=$exe;argv=@($arguments);cwd=$cwd;path=$pathValue}
    $stdout = $process.StandardOutput.ReadToEndAsync(); $stderr = $process.StandardError.ReadToEndAsync()
    $priorityError = $null
    try { $process.PriorityClass = [Diagnostics.ProcessPriorityClass]::BelowNormal } catch { $priorityError = $_.Exception.Message }
    $guard = $false; $timedOut = $false
    $samples = [Collections.Generic.List[object]]::new()
    while (!$process.WaitForExit(3000)) {
        $os = Get-CimInstance Win32_OperatingSystem
        $guard = (100 * (1 - $os.FreePhysicalMemory / $os.TotalVisibleMemorySize)) -ge 80
        $samples.Add(@{utc=[DateTime]::UtcNow.ToString('o');usedPercent=100*(1-$os.FreePhysicalMemory/$os.TotalVisibleMemorySize);freeBytes=[long]$os.FreePhysicalMemory*1KB;originalPid=$ownedPid;originalCreationUtc=$creation.ToString('o')})
        $timedOut = $clock.ElapsedMilliseconds -ge $timeoutMs
        if ($guard -or $timedOut -or $priorityError) {
            $identity = Get-CimInstance Win32_Process -Filter "ProcessId=$ownedPid"
            if ($identity -and ([IO.Path]::GetFullPath($identity.ExecutablePath) -ine [IO.Path]::GetFullPath($exe) -or [Math]::Abs((([DateTime]$identity.CreationDate).ToUniversalTime() - $creation).TotalMilliseconds) -ge 1000)) { throw 'PID identity changed; cannot terminate safely.' }
            if (!$process.HasExited) { $process.Kill($true) }; break
        }
    }
    $process.WaitForExit(); $code = $process.ExitCode; $clock.Stop()
    [IO.File]::WriteAllText("$out/$phase.stdout.log", $stdout.GetAwaiter().GetResult())
    [IO.File]::WriteAllText("$out/$phase.stderr.log", $stderr.GetAwaiter().GetResult())
    Save "$phase.result.json" @{exitCode=$code;seconds=$clock.Elapsed.TotalSeconds;ramGuardTriggered=$guard;timedOut=$timedOut;priorityRequested='BelowNormal';priorityError=$priorityError;ramSamples=$samples.ToArray();ramPollMilliseconds=3000;stdout=(Pin "$out/$phase.stdout.log");stderr=(Pin "$out/$phase.stderr.log")}
    $process.Dispose()
    if ($code -ne 0 -or $guard -or $timedOut -or $priorityError) { throw "Native phase failed: $phase, exit=$code, RAM=$guard, timeout=$timedOut" }
}
function Inventory($root) {
    @(Get-ChildItem -LiteralPath $root -Recurse -Force | ForEach-Object {
        if ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Unexpected reparse point: $($_.FullName)" }
        if (!$_.PSIsContainer) { $pin = Pin $_.FullName; [pscustomobject]@{relativePath=[IO.Path]::GetRelativePath($root, $_.FullName).Replace('\','/');bytes=$pin.bytes;sha256=$pin.sha256} }
    })
}
Verify $prep.pins
Verify @(
    @{path="$PSScriptRoot/run-sdk-current.ps1";bytes=34190;sha256='DB3CE9A55864A0E471952B2BB1AEF89FB9FA8379C277F72A6B1E7A098FE0D763'},
    @{path="$PSScriptRoot/recipe.json";bytes=11620;sha256='523192450686949AC787B97D7E065A54928CE46F8CB2A25728CD18BF2C383418'},
    @{path="$PSScriptRoot/preparation.json";bytes=14173;sha256='29B3CA98264D7035CE2B5B96848AD96AE237988FED9E806E88502F1B88193DE7'},
    @{path="$previousOut/configure.result.json";bytes=2735;sha256='C8703FB1CC0295D110700A09BC2E7E0AD92F58304D11E460F51F92F789CD1297'},
    @{path="$previousOut/configure.start.json";bytes=4366;sha256='DF7D91F5AE678D4F37460D53520B8D19CDD48633851DB93D90486C246EB5269E'},
    @{path="$previousOut/configure.stdout.log";bytes=8306;sha256='52E5B9D2721E3593F78FEBE30A713B708FB563247D1A5EAAC6A6151A89FEB062'},
    @{path="$previousOut/configure.stderr.log";bytes=2741;sha256='0377F06F12D888EF9591677975406050FD281FF29B12818C0C4791F67715FA3E'},
    @{path="$previousOut/installed-files.json";bytes=2308831;sha256='FE03F8306F295631518C26100C9B38A0F852BA238D5CC9A7D4D93F3BF870078F'},
    @{path="$previousOut/install-verification.json";bytes=423;sha256='B0BCE2639139EF4590C2CB85861A447B88A7EFBFEEB2C9B216C93F536AF8A75E'},
    @{path="$previousOut/parent-quarantine-proof.json";bytes=729;sha256='D9AD15258DBA6307FFE242FB530D05D4ADF3555E3143A73FE434BAD4538268EE'},
    @{path="$previousOut/sdk-configure-additions.json";bytes=0;sha256='E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855'}
)
Resources
NativeAdmission
if((Test-Path -LiteralPath $out) -or !(Test-Path -LiteralPath $binary -PathType Container) -or !(Test-Path -LiteralPath $sdk -PathType Container)){throw 'Existing configured consumer83/SDK and absent resume output required.'}
if(Test-Path -LiteralPath $recipe.loaderExecutable){throw 'Consumer83 executable must be absent before its first build.'}
if(@(Get-ChildItem -LiteralPath $previousOut -Filter 'build-consumer.*' -File).Count){throw 'Prior consumer build evidence found; resume contract is configure-only.'}
if(![string]::IsNullOrEmpty($env:DESTDIR)){throw 'DESTDIR must be empty.'}
$configured=Json "$previousOut/configure.result.json"
$configureStart=Json "$previousOut/configure.start.json"
Verify @($configured.stdout,$configured.stderr)
if($configured.exitCode -ne 0 -or $configured.timedOut -or $configured.ramGuardTriggered -or $configured.priorityError -or $configured.priorityRequested -cne 'BelowNormal' -or $configureStart.pid -le 0 -or !([DateTimeOffset]$configureStart.creationUtc) -or [IO.Path]::GetFullPath($configureStart.executable) -ine [IO.Path]::GetFullPath($recipe.executable) -or [IO.Path]::GetFullPath($configureStart.cwd) -ine [IO.Path]::GetFullPath($recipe.configureCwd) -or @($configureStart.argv).Count -ne 43 -or ($configureStart.argv -join '|') -cne ($recipe.configureArgv -join '|')){throw 'Pinned successful configure83 identity/argv/result required.'}
$cache=[Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal)
foreach($line in Get-Content -LiteralPath "$binary/CMakeCache.txt"){if($line -match '^([^#/][^:]*):[^=]*=(.*)$'){$cache[$Matches[1]]=$Matches[2]}}
foreach($pair in @(@('CMAKE_HOME_DIRECTORY',"$PSScriptRoot/source"),@('CMAKE_CACHEFILE_DIR',$binary),@('JE_SDK_PREFIX',$sdk))){if(!$cache.ContainsKey($pair[0]) -or [IO.Path]::GetFullPath($cache[$pair[0]]) -ine [IO.Path]::GetFullPath($pair[1])){throw "Configured cache binding differs: $($pair[0])"}}
if($cache['CMAKE_GENERATOR'] -cne 'Visual Studio 17 2022' -or $cache['CMAKE_GENERATOR_PLATFORM'] -cne 'x64'){throw 'Configured generator/platform differs.'}
foreach($arg in $recipe.configureArgv){if($arg -cmatch '^-D([^:]+):([^=]+)=(.*)$'){$key=$Matches[1];$value=$Matches[3];if(!$cache.ContainsKey($key) -or $cache[$key] -cne $value){throw "Configured cache argument differs: $key"}}}
if((Pin $prep.parentReview82).sha256 -cne $prep.parentReview82SHA256){throw 'Parent review82 pin differs.'}
$review82=Json $prep.parentReview82
if($review82.approved -isnot[bool] -or !$review82.approved -or $review82.reviewer -cne '/root' -or $review82.sourceFreezeSHA256 -cne $prep.freezePin.sha256){throw 'Actual parent82 approval required.'}
$start=Json $prep.hostsStart;$hosts=Json $prep.hostsResult;$post=Json $prep.hostsPostflight
if($start.phase -cne 'build-shader-hosts82' -or $hosts.phase -cne 'build-shader-hosts82' -or $hosts.exitCode -ne 0 -or $hosts.ramGuardTriggered -or $hosts.timedOut -or [IO.Path]::GetFullPath($hosts.executable) -ine [IO.Path]::GetFullPath($recipe.executable) -or $hosts.pid -ne $start.pid -or $start.pid -le 0 -or ([DateTimeOffset]$hosts.processCreationUtc) -ne ([DateTimeOffset]$start.processCreationUtc) -or ($start.argv -join '|') -cne ($hosts.argv -join '|')){throw 'Actual successful HOSTS10 source82 identity required.'}
$expectedHosts=@('Editor','AssetProcessor','AssetProcessorBatch','AssetBuilder','SerializeContextTools','MaterialEditor','MaterialCanvas','PassCanvas','ShaderManagementConsole','JEEditorFixture.GameLauncher')
$targetStart=[Array]::IndexOf([object[]]$hosts.argv,'--target')+1;$targetEnd=[Array]::IndexOf([object[]]$hosts.argv,'--parallel')
if($targetStart -lt 1 -or $targetEnd -ne $targetStart+10 -or ($hosts.argv[$targetStart..($targetEnd-1)] -join '|') -cne ($expectedHosts -join '|')){throw 'Exact10 host targets required.'}
if($post.fatal -or !$post.allFrozenInputsMatched -or $post.preCount -ne 14034 -or $post.postCount -ne 14034 -or $post.freezeSHA256 -cne $prep.freezePin.sha256 -or $post.nativeCases.Count -ne 811 -or $post.suites.Count -ne 8 -or @($post.phases|Where-Object {$_.exitCode -ne 0 -or $_.ramGuardTriggered}).Count){throw 'Actual closed811/8suites/HOSTS10 source82 evidence required; no rerun.'}
foreach($name in @('je_precompiled_base.azshader','je_precompiled_vulkan_0.azshadervariant','JEPrecompiledVulkan81.precompiledshader')){if(Test-Path -LiteralPath "$repo/Projects/JEEditorFixture/Assets/Tests/JENativeShader84/$name"){throw 'Parent must complete exact AP84 fixture teardown before SDK install/consume.'}}
if ($review82.nativePostflight.path -ine $prep.hostsPostflight -or $review82.nativePostflight.sha256 -cne (Pin $prep.hostsPostflight).sha256) { throw 'Reviewed native82 postflight identity differs.' }
$source=VerifyFrozenCohort
VerifyProtected
$effectiveMaterial=VerifyEffectiveMaterialGate
foreach($path in $prep.productRegistries){AssertProductRegistry $path}
$quarantine=Json $prep.quarantineProposal
if($quarantine.fileCount -ne 35 -or $quarantine.entries.Count -ne 35){throw 'Exact35 quarantine plan required.'}
$installOut="$PSScriptRoot/execution-install"
if($InstallStateSHA256 -cnotmatch '^[0-9A-F]{64}$' -or (Pin "$installOut/install-state.json").sha256 -cne $InstallStateSHA256){throw 'Pinned successful Install phase state required.'}
$installState=Json "$installOut/install-state.json"
Verify $installState.artifacts
if($installState.state -cne 'INSTALL_COMPLETE_PARENT_EXACT35_QUARANTINE_REQUIRED' -or $installState.sdk -cne $sdk -or $installState.sourceInputs -ne 14034 -or $installState.nativeInstall.exitCode -ne 0 -or $installState.nativeInstall.ramGuardTriggered -or $installState.nativeInstall.timedOut -or $installState.nativeInstall.priorityError){throw 'Install phase was not a real clean native completion.'}
if($QuarantineReviewSHA256 -cnotmatch '^[0-9A-F]{64}$' -or !$QuarantineReviewPath -or (Pin $QuarantineReviewPath).sha256 -cne $QuarantineReviewSHA256){throw 'Parent exact35 quarantine approval pin required.'}
$pruneReview=Json $QuarantineReviewPath
if($pruneReview.approved -isnot[bool] -or !$pruneReview.approved -or $pruneReview.reviewer -cne '/root'){throw 'Parent exact35 quarantine approval required.'}
$prior=@(Json "$installOut/installed-before-quarantine.json");$current=@(Inventory $sdk)
$allowed=@{};foreach($pin in $prior){$allowed[$pin.relativePath]=$pin}
foreach($pin in $quarantine.entries){if(!$allowed.ContainsKey($pin.relativePath) -or $allowed[$pin.relativePath].bytes -ne $pin.bytes -or $allowed[$pin.relativePath].sha256 -cne $pin.sha256){throw 'Quarantine plan does not match actual installed35 bytes.'};$allowed.Remove($pin.relativePath)}
if($current.Count -ne $allowed.Count){throw 'SDK change exceeds exact35 parent quarantine.'}
foreach($pin in $current){$expected=$allowed[$pin.relativePath];if(!$expected -or $pin.bytes -ne $expected.bytes -or $pin.sha256 -cne $expected.sha256){throw 'Unapproved SDK change after install.'}}
$inputs=@((Json "$installOut/frozen-install-inputs.json").inputs)
if((Pin "$build/install_manifest.txt").sha256 -cne (Pin "$installOut/install_manifest.txt").sha256){throw 'Original generated install manifest changed after install.'}
Verify $inputs; $source = VerifyFrozenCohort; VerifyProtected; $effectiveMaterial = VerifyEffectiveMaterialGate
$installProof=Json "$previousOut/install-verification.json"
if($installProof.files -ne 11218 -or $installProof.forbiddenCount -ne 0 -or !$installProof.currentPayloadsVerified){throw 'Prior installation verification is not accepted11218/zero forbidden.'}
$previousPrune=Json "$previousOut/parent-quarantine-proof.json"
if($previousPrune.removed -ne 35 -or $previousPrune.otherChanges -ne 0 -or $previousPrune.review.sha256 -cne $QuarantineReviewSHA256 -or $previousPrune.plan.sha256 -cne (Pin $prep.quarantineProposal).sha256){throw 'Prior exact35 quarantine proof differs.'}
$installed=@(Json "$previousOut/installed-files.json")
$afterConfigure=@($current)
if($installed.Count -ne 11218 -or $afterConfigure.Count -ne 11218){throw 'Exact11218 installed files required before resume.'}
$installedMap=@{};foreach($pin in $installed){if($installedMap.ContainsKey($pin.relativePath)){throw 'Duplicate installed inventory entry.'};$installedMap[$pin.relativePath]=$pin}
foreach($pin in $afterConfigure){$old=$installedMap[$pin.relativePath];if(!$old -or $old.bytes -ne $pin.bytes -or $old.sha256 -cne $pin.sha256){throw "SDK differs from successful configure baseline: $($pin.relativePath)"}}
$adds=@($afterConfigure|Where-Object{!$installedMap.ContainsKey($_.relativePath)})
if($adds.Count -ne 0){throw 'Resume requires zero real configure additions.'}
[void][IO.Directory]::CreateDirectory($out)
Save 'emptyarrayProbe.json' @()
if([IO.File]::ReadAllText("$out/emptyarrayProbe.json") -cne '[]'){throw 'Empty-array Save regression failed.'}
Save 'sdk-configure-additions.json' $adds
Save 'installed-files.json' $installed
Save 'install-verification.json' $installProof
Save 'effective-material-gate.json' $effectiveMaterial
Save 'host-gate.json' @{parentReview=(Pin $prep.parentReview82);hosts=(Pin $prep.hostsResult);sourceCount=14034;authoredCount=40;ownerCount=26;sourceFreeze=$prep.freezePin;cache=$prep.cachePin}
Save 'parent-quarantine-proof.json' $previousPrune
Save 'resume-provenance.json' @{mode='RESUME_AFTER_SUCCESSFUL_CONFIGURE_ONLY';priorOutput=$previousOut;configureResult=(Pin "$previousOut/configure.result.json");configureStart=(Pin "$previousOut/configure.start.json");configureCache=(Pin "$binary/CMakeCache.txt");installedInventory=(Pin "$previousOut/installed-files.json");installState=(Pin "$installOut/install-state.json");quarantineReview=(Pin $QuarantineReviewPath);sdkFiles=$afterConfigure.Count;actualConfigureAdditions=$adds.Count;originalFailure=(Pin "$previousOut/sdk-configure-additions.json");newEmptyArrayProbe=(Pin "$out/emptyarrayProbe.json");noInstallConfigureOrQuarantineRepeated=$true}
$projects = @(Get-ChildItem -LiteralPath $binary -Recurse -Filter 'JENativeSdkConsumer.vcxproj' -File)
if ($projects.Count -ne 1) { throw 'Expected one generated consumer project.' }
[xml]$xml = [IO.File]::ReadAllText($projects[0].FullName)
$profileGroups = @($xml.SelectNodes("//*[local-name()='ItemDefinitionGroup']") | Where-Object { $_.Condition -match '(?i)profile\|x64' })
if (!$profileGroups.Count) { throw 'No Profile compiler contract.' }
foreach ($g in $profileGroups) { if ($g.ClCompile.RuntimeLibrary -ne 'MultiThreadedDLL' -or $g.ClCompile.LanguageStandard -ne 'stdcpp20' -or $g.ClCompile.UseStandardPreprocessor -ne 'true' -or $g.ClCompile.ConformanceMode -ne 'true') { throw 'Consumer compiler policy drift.' } }
$reports = @(Get-ChildItem -LiteralPath $binary -Recurse -Filter 'native-*-profile.txt' -File)
if ($reports.Count -ne 5) { throw 'Five native imported-target reports required.' }
foreach ($report in $reports) { foreach ($line in Get-Content -LiteralPath $report.FullName) { if ($line -match '^(?:location|implib)=(.+)$' -and ![IO.Path]::GetFullPath($Matches[1]).StartsWith([IO.Path]::GetFullPath($sdk)+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Import escaped SDK.' } } }
# Preserve the prior approved external include roots; only the SDK prefix is rebased.
$oldReview = Json $prep.previousConfigureReview
$allowedIncludes = @($oldReview.includeRoots | ForEach-Object { $_.Replace('\','/').Replace($prep.previousSdkPrefix, $sdk).TrimEnd('/') })
$privateRoot = [IO.Path]::GetFullPath("$PSScriptRoot/source").Replace('\','/')
foreach ($g in $profileGroups) { foreach ($include in ([string]$g.ClCompile.AdditionalIncludeDirectories -split ';')) { $normalized = $include.Replace('\','/').TrimEnd('/'); if ($include -and $include -notmatch '^%\(' -and $normalized -notin $allowedIncludes -and $normalized -ine $privateRoot -and !$normalized.StartsWith(([IO.Path]::GetFullPath($binary).Replace('\','/').TrimEnd('/')+'/'),[StringComparison]::OrdinalIgnoreCase) -and !$normalized.StartsWith(([IO.Path]::GetFullPath($sdk).Replace('\','/').TrimEnd('/')+'/'),[StringComparison]::OrdinalIgnoreCase)) { throw "Unreviewed include root: $include" } } }
if (@(Get-ChildItem -LiteralPath $binary -Recurse -Filter '*.cmake' -File | Where-Object { [IO.File]::ReadAllText($_.FullName) -match '(?s)\bNAME\s+(?:AtomShader\.(?:Builders|Static)|Atom_RPI\.Builders)\s+IMPORTED\b' }).Count) { throw 'Retired builder target in generated consumer metadata.' }
Save 'configure-verification.json' @{project=(Pin $projects[0].FullName);imports=@($reports | ForEach-Object { Pin $_.FullName });compiler='/MD C++20 conformant preprocessor';sdkAdds=$adds.Count}
Run 'build-consumer' $recipe.executable $recipe.buildArgv $binary
$consumerDir = $recipe.loaderCwd
$deploy = @()
foreach ($dll in Get-ChildItem -LiteralPath $consumerDir -Filter '*.dll' -File) { $origin = "$sdk/bin/Windows/profile/Default/$($dll.Name)"; if (!(Test-Path -LiteralPath $origin) -or (Pin $origin).sha256 -cne (Pin $dll.FullName).sha256) { throw "Consumer DLL not byte-exact from SDK: $($dll.Name)" }; $deploy += @{copy=(Pin $dll.FullName);source=(Pin $origin)} }
if ($deploy.Count -ne 31) { throw 'Exact31 SDK runtime DLL closure required.' }
$priorNames=@((Json $prep.priorSdk77Deploy)|ForEach-Object {[IO.Path]::GetFileName($_.copy.path)}|Sort-Object)
$currentNames=@($deploy|ForEach-Object {[IO.Path]::GetFileName($_.copy.path)}|Sort-Object)
if (($priorNames -join '|') -cne ($currentNames -join '|')) { throw 'Deployed SDK DLL identities differ from accepted31 closure.' }
Save 'runtime-deploy-review.json' $deploy
Run 'pe-imports' $prep.dumpbin @('/imports', $recipe.loaderExecutable) $consumerDir
$pe = [IO.File]::ReadAllText("$out/pe-imports.stdout.log")
foreach ($symbol in @('CreateToolsModule','CreateShaderCompilerToolsModule','HashCombineShaderOptionValues@HashedVariantInfoSourceData@Shader@RenderBuilders@Assets@JE@@','??0ModelAssetBuilderContext','??0MaterialAssetBuilderContext','??0ModelAssetPostBuildContext','RTTI_IsTypeOf@ModelAssetBuilderContext','RTTI_IsTypeOf@MaterialAssetBuilderContext','RTTI_IsTypeOf@ModelAssetPostBuildContext','RenderAssetBuilders.dll')) { if (!$pe.Contains($symbol)) { throw "Missing real PE import evidence: $symbol" } }
Run 'loader' $recipe.loaderExecutable $recipe.loaderArgs $consumerDir $recipe.loaderPath $recipe.loaderTimeoutMs
$stdout = [IO.File]::ReadAllText("$out/loader.stdout.log"); $stderr = [IO.File]::ReadAllText("$out/loader.stderr.log")
foreach ($message in @('JE SDK: Materials, Camera and RenderIntegration exports resolved; empty PassPipelinePreview exercised.','JE SDK: native RenderAssetBuilders factory/descriptors/recreation and three public contexts exercised without activation or cook.','JE SDK: shader compiler factory/descriptors/services/recreation and exported shader option hash exercised without activation or cook.')) { if (!$stdout.Contains($message)) { throw 'Missing native lifecycle success message.' } }
if ($stderr -match '(?i)(assert|access violation|allocator.*(error|leak)|unhandled exception)') { throw 'Loader diagnostic error.' }
Verify $inputs; $source = VerifyFrozenCohort; VerifyProtected; Verify $prep.pins; $effectiveMaterial = VerifyEffectiveMaterialGate
$finalSdk = Inventory $sdk
if (($finalSdk | ConvertTo-Json -Depth 4 -Compress) -cne ($afterConfigure | ConvertTo-Json -Depth 4 -Compress)) { throw 'SDK changed after configure baseline.' }
Save 'sdk-final-inventory.json' $finalSdk
Resources
Save 'result.json' @{effectiveMaterialReview=(Pin $EffectiveMaterialReviewPath);state='SDK_PROFILE_IMPORT_LIFECYCLE_REPLAY_PASSED_NOT_PRODUCT_ACCEPTANCE';sourceInputs=14034;authoredInputs=40;privateSources=4;hosts=10;ownerInputs=26;backendProfile='Vulkan-only; Null explicit Tools permitted';exe=(Pin $recipe.loaderExecutable);sdkInventory=(Pin "$out/sdk-final-inventory.json");limits=$recipe.limits;dynamicNoPythonNoNullShipping='NOT_TESTED_BY_THIS_CONSUMER'}


