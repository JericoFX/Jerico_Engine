# Private two-phase SDK replay; no native work without -Execute and explicit Phase.
param(
    [switch]$Execute,
    [ValidateSet('Install','Consume')][string]$Phase,
    [string]$InstallStateSHA256,
    [string]$QuarantineReviewPath,
    [string]$QuarantineReviewSHA256,
    [string]$EffectiveMaterialReviewPath,
    [string]$EffectiveMaterialReviewSHA256
)
$ErrorActionPreference='Stop'
if(!$Execute){Write-Output 'PREPARED_NOT_RUN: parent owns AP84 teardown, Install, exact35 quarantine, then Consume.';return}
if(!$Phase){throw 'Explicit Install or Consume phase required.'}
$repo='F:/JericoEngine/JericoEngine'
$area="$repo/build/n0-validation/native-render-builders-integration-rev1"
$build="$repo/build/n0-validation/original-engine"
$recipe=Get-Content -LiteralPath "$PSScriptRoot/recipe.json" -Raw|ConvertFrom-Json
$prep=Get-Content -LiteralPath "$PSScriptRoot/preparation.json" -Raw|ConvertFrom-Json
$sdk=$recipe.installedSdkPrefix
$binary=$recipe.consumerBuild
$out="$PSScriptRoot/execution-$($Phase.ToLowerInvariant())"
function Json($path) { Get-Content -LiteralPath $path -Raw | ConvertFrom-Json }
function Save($name, $data) {
    $stream = [IO.File]::Open("$out/$name", [IO.FileMode]::CreateNew)
    try { $bytes = [Text.UTF8Encoding]::new($false).GetBytes(($data | ConvertTo-Json -Depth 16)); $stream.Write($bytes, 0, $bytes.Length) } finally { $stream.Dispose() }
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
Resources
NativeAdmission
if((Test-Path -LiteralPath $out) -or (Test-Path -LiteralPath $binary) -or !(Test-Path -LiteralPath $sdk -PathType Container)){throw 'Existing sdk-vk77, fresh consumer83 and new phase evidence required.'}
if(![string]::IsNullOrEmpty($env:DESTDIR)){throw 'DESTDIR must be empty.'}
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
[void][IO.Directory]::CreateDirectory($out)
Save 'effective-material-gate.json' $effectiveMaterial
Save 'host-gate.json' @{parentReview=(Pin $prep.parentReview82);hosts=(Pin $prep.hostsResult);sourceCount=14034;authoredCount=40;ownerCount=26;sourceFreeze=$prep.freezePin;cache=$prep.cachePin}
$quarantine=Json $prep.quarantineProposal
if($quarantine.fileCount -ne 35 -or $quarantine.entries.Count -ne 35){throw 'Exact35 quarantine plan required.'}
if($Phase -eq 'Install'){
 $baseline=@(Inventory $sdk);$expected=@(Json $prep.baselineSdk77Inventory)
 if($baseline.Count -ne $expected.Count){throw 'Existing SDK differs from accepted77 baseline.'}
 $expectedMap=@{};foreach($pin in $expected){$expectedMap[$pin.relativePath]=$pin}
 foreach($pin in $baseline){$old=$expectedMap[$pin.relativePath];if(!$old -or $pin.bytes -ne $old.bytes -or $pin.sha256 -cne $old.sha256){throw "Existing SDK77 baseline mismatch: $($pin.relativePath)"}}
 Save 'sdk-baseline77.json' $baseline
# Follow the actual generated include closure, not every stale cmake_install file in the build tree.
$paths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$seenScripts = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$pending = [Collections.Generic.Queue[string]]::new(); $pending.Enqueue("$build/cmake_install.cmake")
$declaredMissing = [Collections.Generic.List[object]]::new()
while ($pending.Count) {
    $script = [IO.Path]::GetFullPath($pending.Dequeue()); if (!$seenScripts.Add($script)) { continue }; [void]$paths.Add($script)
    $text = [IO.File]::ReadAllText($script)
    if ($text -match '(?im)^\s*(?:file\(INSTALL|ly_copy\()[^\r\n]*[/\\](?:Atom_RHI_(?:DX12|Metal)[^/\\"]*\.(?:dll|lib|pdb|exe)|(?:python(?:[0-9]+)?(?:_d)?|PythonBindings|EditorPythonBindings)\.(?:dll|exe))"') { throw "Forbidden backend/Python runtime install source: $script" }
    if ($text -match '(?im)^\s*(?:file\(\s*(?:DOWNLOAD|UPLOAD)|FetchContent_|ExternalProject_)') { throw "Network directive in install closure: $script" }
    foreach ($inc in [regex]::Matches($text, '(?m)^\s*include\("([^"]+)"\)')) {
        $include = $inc.Groups[1].Value.Replace('${CMAKE_INSTALL_CONFIG_NAME}', 'profile').Replace('${CMAKE_INSTALL_PREFIX}', $sdk)
        if ($include.Contains('$')) { throw "Unresolved install include: $include" }; $pending.Enqueue($include)
    }
    $rules = [regex]::Matches($text, '(?m)^\s*file\(INSTALL\b(?:"[^"]*"|[^()"])*\)')
    if ($rules.Count -ne [regex]::Matches($text, '(?m)^\s*file\(INSTALL\b').Count) { throw "Unparsed install rule in $script" }
    foreach ($rule in $rules) {
        if ($rule.Value -match '\bDESTINATION\s+"([^"]+)"') {
            $destination = $Matches[1].Replace('${CMAKE_INSTALL_PREFIX}', $sdk).Replace('${CMAKE_INSTALL_CONFIG_NAME}', 'profile')
            $sdkRoot = [IO.Path]::GetFullPath($sdk).TrimEnd('\','/')
            $destinationRoot = [IO.Path]::GetFullPath($destination).TrimEnd('\','/')
            if ($destination.Contains('$') -or ($destinationRoot -ine $sdkRoot -and !$destinationRoot.StartsWith($sdkRoot+[IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase))) { throw "Install destination outside new SDK: $destination" }
        }
        if ($rule.Value -notmatch '(?s)\b(?:FILES|DIRECTORY)\s+((?:"[^"]*"\s*)+)') { throw "Unparsed install sources in $script" }
        foreach ($literal in [regex]::Matches($Matches[1], '"([^"]+)"')) {
            $p = $literal.Groups[1].Value
            if ($p -match '\$') { throw "Unresolved install source: $p" }
            if ($p -match '(?i)/(debug|release|relwithdebinfo)/') { continue }
            if (Test-Path -LiteralPath $p -PathType Leaf) { [void]$paths.Add([IO.Path]::GetFullPath($p)) }
            elseif (Test-Path -LiteralPath $p -PathType Container) { foreach ($f in Get-ChildItem -LiteralPath $p -Recurse -File -Force) { [void]$paths.Add($f.FullName) } }
            else { $declaredMissing.Add(@{script=$script;source=$p;optional=($rule.Value -match '\bOPTIONAL\b')}) }
        }
    }
}
Save 'declared-missing-install-sources.json' @($declaredMissing)
if (@($declaredMissing | Where-Object { !$_.optional -and $_.source -match '(?i)/profile/' }).Count) { throw 'Required Profile install payload is absent; do not build unrelated suites or hide it with a partial component.' }
# Include every current Profile output, native export, runtime registry and cache before install.
foreach ($dir in @("$build/bin/profile","$build/lib/profile","$build/install","$build/runtime_dependencies")) {
    if (Test-Path -LiteralPath $dir) { foreach ($f in Get-ChildItem -LiteralPath $dir -File -Recurse -Force) { [void]$paths.Add($f.FullName) } }
}
[void]$paths.Add("$build/CMakeCache.txt")
$inputs = @($paths | Sort-Object | ForEach-Object { Pin $_ })
Save 'frozen-install-inputs.json' @{sourceFreeze=(Pin $prep.sourceFreeze);sourceCount=14034;inputs=$inputs;missing=$declaredMissing;prefix=$sdk;componentRestriction=$null;outsidePrefixWrite="$build/install_manifest.txt";Profiler2='AzCore/Debug/Profiler.cpp and Profiler.h are source-frozen; current AzCore DLL/import library/PDB plus all current Profile payloads rehashed.'}
& $prep.generatedChecker -BuildRoot $build -RootInstallSHA256 (Pin "$build/cmake_install.cmake").sha256 -ResultPath "$out/generated-config-check.json"
Run 'install' $recipe.executable $recipe.installArgv $repo
 Verify $inputs;$source=VerifyFrozenCohort;VerifyProtected;$effectiveMaterial=VerifyEffectiveMaterialGate
 $installed=@(Inventory $sdk);Save 'installed-before-quarantine.json' $installed
 foreach($pin in $quarantine.entries){$now=Pin (Join-Path $sdk $pin.relativePath);if($now.bytes -ne $pin.bytes -or $now.sha256 -cne $pin.sha256){throw 'Obsolete file changed during install; parent must review, not wildcard prune.'}}
 $manifest=@(Get-Content -LiteralPath "$build/install_manifest.txt"|Sort-Object -Unique)
 foreach($path in $manifest){if(![IO.Path]::GetFullPath($path).StartsWith([IO.Path]::GetFullPath($sdk).TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or !(Test-Path -LiteralPath $path -PathType Leaf)){throw 'Install manifest destination outside prefix/missing.'}}
 [IO.File]::Copy("$build/install_manifest.txt","$out/install_manifest.txt",$false)
 Save 'install-state.json' @{state='INSTALL_COMPLETE_PARENT_EXACT35_QUARANTINE_REQUIRED';sourceInputs=14034;sdk=$sdk;freeze=$prep.freezePin;artifacts=@((Pin "$out/frozen-install-inputs.json"),(Pin "$out/installed-before-quarantine.json"),(Pin "$out/install_manifest.txt"),(Pin "$out/install.start.json"),(Pin "$out/install.result.json"),(Pin $prep.quarantineProposal));nativeInstall=(Json "$out/install.result.json");noPruneByRunner=$true;productAcceptance=$false}
 Write-Output 'INSTALL83_COMPLETE; parent must quarantine exact35, review, then authorize Consume.'
 return
}
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
Save 'parent-quarantine-proof.json' @{review=(Pin $QuarantineReviewPath);plan=(Pin $prep.quarantineProposal);removed=35;otherChanges=0}
$inputs=@((Json "$installOut/frozen-install-inputs.json").inputs)
if((Pin "$build/install_manifest.txt").sha256 -cne (Pin "$installOut/install_manifest.txt").sha256){throw 'Original generated install manifest changed after install.'}
Verify $inputs; $source = VerifyFrozenCohort; VerifyProtected; $effectiveMaterial = VerifyEffectiveMaterialGate
$installed = Inventory $sdk
Save 'installed-files.json' $installed
$manifest = @(Get-Content -LiteralPath "$build/install_manifest.txt" | Sort-Object -Unique)
foreach ($p in $manifest) { if (![IO.Path]::GetFullPath($p).StartsWith([IO.Path]::GetFullPath($sdk).TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or !(Test-Path -LiteralPath $p -PathType Leaf)) { throw "Invalid installed manifest path: $p" } }
[IO.File]::Copy("$build/install_manifest.txt", "$out/install_manifest.txt", $false)
$manifestSet = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($p in $manifest) { [void]$manifestSet.Add([IO.Path]::GetRelativePath($sdk, $p).Replace('\','/')) }
$extras = @($installed | Where-Object { !$manifestSet.Contains($_.relativePath) })
$oldExtras=@((Json $prep.previousInstallInventory).manifestExtrasNotListed)
$knownAdds=@(Json $prep.priorSdk77ConfigureAdds)
$extraSet=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($path in $oldExtras){[void]$extraSet.Add($path)}
foreach($pin in $knownAdds){[void]$extraSet.Add($pin.relativePath)}
$baselineMap=@{};foreach($pin in @(Json $prep.baselineSdk77Inventory)){$baselineMap[$pin.relativePath]=$pin}
foreach($pin in $extras){$old=$baselineMap[$pin.relativePath];if(!$extraSet.Remove($pin.relativePath) -or !$old -or $old.bytes -ne $pin.bytes -or $old.sha256 -cne $pin.sha256){Save 'unreviewed-install-extras.json' $extras;throw 'Unapproved extra or changed historical side-effect file.'}}
if($extraSet.Count){throw 'Historical extras/44configure additions changed; no broad waiver.'}
$forbidden = @($installed | Where-Object { $_.relativePath -match '(?i)(?:^|/)(MaterialEditor|MaterialCanvas|PassCanvas|O3DEQtControlGallery|ReflectedPropertyEditorStandalone|DPEDebugViewStandalone|ProcessLaunchTest)\.exe$|(?:^|/)[^/]*\.Tests(?:\.Static)?\.(dll|lib|pdb|exe)$|(?:Atom_RPI\.Builders|AtomShader\.(?:Builders|Static))\.(dll|lib|pdb)$|(?:^|/)Atom_RHI_(?:DX12|Metal)[^/]*\.(dll|lib|pdb|exe)$|(?:^|/)(?:python(?:[0-9]+)?(?:_d)?|PythonBindings|EditorPythonBindings)\.(dll|exe)$' })
if ($forbidden.Count) { Save 'forbidden-install-payloads.json' $forbidden; throw 'Excluded test or retired host payload installed.' }
foreach ($name in @('AzCore.dll','RenderAssetBuilders.dll','EditorMaterials.dll','EditorCamera.dll','EditorRenderIntegration.dll','ShaderManagementConsole.exe')) { if (@($installed | Where-Object { $_.relativePath -ceq "bin/Windows/profile/Default/$name" }).Count -ne 1) { throw "Required SDK payload missing: $name" } }
foreach ($p in $installed | Where-Object { $_.relativePath -match '\.cmake$|cmake_dependencies.*\.setreg$' }) {
    $text = [IO.File]::ReadAllText((Join-Path $sdk $p.relativePath))
    if ($text -match '(?s)\bNAME\s+[^\s()]+\.Tests(?:\.Static)?\s+IMPORTED\b|\"[^\"]+\.Tests\.dll\"|\bNAME\s+Atom_RHI_(?:DX12|Metal)[^\s()]*\s+IMPORTED\b') { throw "Test export/runtime metadata remains: $($p.relativePath)" }
}
if (@($installed | Where-Object relativePath -like 'Gems/Atom/Asset/Shader/*').Count) { throw 'Retired Shader owner packaging remains.' }
$productRegistryCopies = @($installed | Where-Object { $_.relativePath -match '/cmake_dependencies\.(?:editor|jeeditorfixture_gamelauncher)\.setreg$' })
$registryNames = @('cmake_dependencies.editor.setreg', 'cmake_dependencies.jeeditorfixture_gamelauncher.setreg')
$profileRegistryCopies = @($productRegistryCopies | Where-Object { $_.relativePath -cin @($registryNames | ForEach-Object { "bin/Windows/profile/Default/Registry/$_" }) })
if ($profileRegistryCopies.Count -ne 2 -or $productRegistryCopies.Count -ne 6 -or @($prep.productRegistries).Count -ne 2) { throw 'Exactly two Profile and six total Editor/Game product registry copies required.' }
foreach ($name in $registryNames) {
    $sources = @($prep.productRegistries | Where-Object { [IO.Path]::GetFileName($_) -ceq $name })
    if ($sources.Count -ne 1) { throw "Exactly one original product registry required: $name" }
    $sourcePin = Pin $sources[0]
    AssertProductRegistry $sources[0]
    foreach ($configuration in @('debug', 'profile', 'release')) {
        $relativePath = "bin/Windows/$configuration/Default/Registry/$name"
        $copies = @($productRegistryCopies | Where-Object { $_.relativePath -ceq $relativePath })
        if ($copies.Count -ne 1) { throw "Exactly one product registry required: $relativePath" }
        $copyPin = Pin (Join-Path $sdk $relativePath)
        if ($copyPin.bytes -ne $copies[0].bytes -or $copyPin.sha256 -cne $copies[0].sha256 -or $copyPin.bytes -ne $sourcePin.bytes -or $copyPin.sha256 -cne $sourcePin.sha256) { throw "Installed product registry mismatch: $relativePath" }
        AssertProductRegistry (Join-Path $sdk $relativePath)
    }
}
$profilerHeader = @($installed | Where-Object { $_.relativePath -ceq 'Code/Framework/AzCore/AzCore/Debug/Profiler.h' })
if ($profilerHeader.Count -ne 1 -or $profilerHeader[0].sha256 -cne (Pin "$repo/Code/Framework/AzCore/AzCore/Debug/Profiler.h").sha256) { throw 'Installed Profiler2 public header mismatch.' }
foreach ($f in $installed | Where-Object { $_.relativePath -match '/profile/Default/.*\.(dll|lib|pdb|exe)$' }) {
    $leaf = [IO.Path]::GetFileName($f.relativePath); $origin = if ($f.relativePath.StartsWith('lib/')) { "$build/lib/profile/$leaf" } else { "$build/bin/profile/$leaf" }
    if ((Test-Path -LiteralPath $origin) -and (Pin $origin).sha256 -cne $f.sha256) { throw "Installed current payload mismatch: $leaf" }
}
Save 'install-verification.json' @{manifestCount=$manifest.Count;files=$installed.Count;forbiddenCount=0;profilerHeader=$profilerHeader;currentPayloadsVerified=$true;scope='Profile SDK installation, not Shipping or dynamic noPython/noNull acceptance.'}

Run 'configure' $recipe.executable $recipe.configureArgv $recipe.configureCwd
$afterConfigure = Inventory $sdk
$beforeMap = @{}; foreach ($p in $installed) { $beforeMap[$p.relativePath] = $p }
$newMap = @{}; foreach ($p in $afterConfigure) { $newMap[$p.relativePath] = $p }
foreach ($p in $installed) { if (!$newMap.ContainsKey($p.relativePath) -or $newMap[$p.relativePath].sha256 -cne $p.sha256) { throw "Installed SDK baseline changed by configure: $($p.relativePath)" } }
$allowedAdds = @{}; foreach ($p in (Json $prep.previousConfigureDelta).added) { $allowedAdds[$p.path] = $p }
$adds = @($afterConfigure | Where-Object { !$beforeMap.ContainsKey($_.relativePath) })
foreach ($p in $adds) { if (!$allowedAdds.ContainsKey($p.relativePath) -or $allowedAdds[$p.relativePath].sha256 -cne $p.sha256) { throw "Unreviewed SDK configure addition: $($p.relativePath)" } }
Save 'sdk-configure-additions.json' $adds
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


