# Continuation of the reviewed SDK98 delta after configure/build exited0. No native work without -Execute and parent lease.
param([switch]$Execute)
$ErrorActionPreference = 'Stop'
if (!$Execute) { Write-Output 'PREPARED_NOT_RUN: parent review and exclusive native lease required.'; return }
if ($PSVersionTable.PSVersion.Major -lt 7) { throw 'Configured PowerShell7 is required.' }
$repo = 'F:/JericoEngine/JericoEngine'
$producer = "$repo/build/n0-validation/original-engine"
$originalRunner = "$PSScriptRoot/RunSdkDelta98.ps1"
$recipePath = "$PSScriptRoot/recipe.json"
$manifestPath = "$PSScriptRoot/pins.json"
$inputPath = "$PSScriptRoot/completion-inputs.json"
foreach ($item in @(@($originalRunner,'31903EC1D010DD36EF4D07E6CB4501B2816800A92F450C7CDC58395A00D07C79'),@($recipePath,'6B153CE1D33AC7CF4F6F9EB9DB955C44BCA41E243CFBAC7455FB1DF87FBF966F'),@($manifestPath,'19E15DAF7295590B061A4834F7A670949F6A66ABF9A961D85238A1F19A1453D9'),@($inputPath,'8F134D7D60476B1203BC6FA1828A1B6AF019422C25FA68187CB6F79AE933B6AC'))) {
    if ((Get-FileHash -LiteralPath $item[0]).Hash -cne $item[1]) { throw "Sealed continuation input drift: $($item[0])" }
}
$recipe = Get-Content -LiteralPath $recipePath -Raw | ConvertFrom-Json
$completion = Get-Content -LiteralPath $inputPath -Raw | ConvertFrom-Json
$BindingPath = $completion.bindingPin.path
if ($completion.bindingPin.sha256 -cne '481564C33C6DEC28EDE9CECDA727B6971B54219772954039ABE2F70B9F693FF0' -or (Get-FileHash -LiteralPath $BindingPath).Hash -cne $completion.bindingPin.sha256) { throw 'Exact original parent binding required.' }
$binding = Get-Content -LiteralPath $BindingPath -Raw | ConvertFrom-Json
if ($binding.approved -isnot [bool] -or !$binding.approved -or $binding.reviewer -cne '/root' -or $binding.revision -ne 98) { throw 'Original parent-approved binding required.' }
$sdk = [IO.Path]::GetFullPath($recipe.baselineSdk)
$binary = [IO.Path]::GetFullPath($recipe.newConsumerBuild)
$overlay = [IO.Path]::GetFullPath($recipe.newOverlay)
$out = "$repo/build/n0-validation/native-rpi98/sdk-delta98-completion-attempt1"
$boundary = Get-Content -LiteralPath $recipe.baselineBoundaryPins -Raw | ConvertFrom-Json
$functionSource = $recipe.guardReuse.source
$functionPin = @($boundary | Where-Object { [IO.Path]::GetFullPath($_.path) -ieq [IO.Path]::GetFullPath($functionSource) })
if ($functionPin.Count -ne 1 -or (Get-FileHash -LiteralPath $functionSource).Hash -cne $functionPin[0].sha256) { throw 'Audited SDK96 helper source drift.' }
$tokens=$null; $errors=$null
$sdkAst=[Management.Automation.Language.Parser]::ParseFile($functionSource,[ref]$tokens,[ref]$errors)
if ($errors.Count) { throw 'SDK96 helper source parse error.' }
$originalAst=[Management.Automation.Language.Parser]::ParseFile($originalRunner,[ref]$tokens,[ref]$errors)
if ($errors.Count) { throw 'Sealed SDK98 source parse error.' }
$functionProof=@()
foreach ($pair in @(@($sdkAst,@('Json','Save','Pin','Verify','NativeAdmission','Resources','Run')),@($originalAst,@('SamePath','NoReparse','CopyPinned','CheckNoEngineFallback','VerifyOverlay')))) {
    foreach ($name in $pair[1]) {
        $nodes=@($pair[0].FindAll({param($n) $n -is [Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -ceq $name},$true))
        if ($nodes.Count -ne 1) { throw "Ambiguous audited function: $name" }
        . ([scriptblock]::Create($nodes[0].Extent.Text))
        $functionProof+=@{name=$name;startLine=$nodes[0].Extent.StartLineNumber;endLine=$nodes[0].Extent.EndLineNumber;text=$nodes[0].Extent.Text}
    }
}
$originalText=[IO.File]::ReadAllText($originalRunner)
function OriginalRange($first,$last) {
    $start=$originalText.IndexOf($first,[StringComparison]::Ordinal)
    $end=$originalText.IndexOf($last,$start+$first.Length,[StringComparison]::Ordinal)
    if ($start -lt 0 -or $end -lt 0 -or $originalText.IndexOf($first,$start+$first.Length,[StringComparison]::Ordinal) -ge 0) { throw 'Sealed source range is ambiguous.' }
    $originalText.Substring($start,$end-$start)
}
if (!(SamePath $sdk "$repo/build/sdk-vk96") -or !(SamePath $binary "$repo/build/sdk-vk-consumer98") -or !(SamePath $overlay "$repo/build/sdk-rpi98-delta") -or !(SamePath $out $completion.output)) { throw 'Unexpected continuation layout.' }
foreach ($path in @($sdk,$binary,$overlay,$out,$PSScriptRoot)) { NoReparse $path }
if (Test-Path -LiteralPath $out) { throw 'Fresh continuation evidence directory required.' }
foreach ($path in @($recipe.loaderExecutable,"$overlay/bin/JENativeSdkConsumer.pdb")) { if (Test-Path -LiteralPath $path) { throw 'No overwrite of overlay consumer EXE/PDB.' } }
foreach ($pin in @(Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json)) { Verify @([pscustomobject]@{path=(Join-Path $PSScriptRoot $pin.relative);bytes=$pin.bytes;sha256=$pin.sha256}) }
Resources; NativeAdmission
# Reuse all original source/baseline/binding/XML/provider checks. Only compare the binding to the original runner path.
$validation=OriginalRange '$runtime = Json $recipe.baselineRuntimePins' 'function CopyPinned('
if ([regex]::Matches($validation,'\$PSCommandPath').Count -ne 1) { throw 'Original runner path check changed.' }
. ([scriptblock]::Create($validation.Replace('$PSCommandPath','$originalRunner')))
Verify $completion.closedEvidencePins; Verify $completion.consumerPins; Verify $completion.generatedBeforePins; Verify @($completion.fsutilPin)
foreach ($phase in @('configure-consumer','build-consumer')) {
    $oldOut=$recipe.newEvidence
    $result=Json "$oldOut/$phase.result.json"; $start=Json "$oldOut/$phase.start.json"
    $argv=if ($phase -eq 'configure-consumer') { $recipe.configureArgv } else { $recipe.buildArgv }
    $cwd=if ($phase -eq 'configure-consumer') { $recipe.configureCwd } else { $recipe.buildCwd }
    if ($result.exitCode -ne 0 -or $result.ramGuardTriggered -or $result.timedOut -or $result.priorityError -or !(SamePath $start.executable $recipe.executable) -or !(SamePath $start.cwd $cwd) -or ($start.argv | ConvertTo-Json -Compress) -cne ($argv | ConvertTo-Json -Compress)) { throw 'Closed configure/build is not reusable.' }
    Verify @($result.stdout,$result.stderr)
}
$generated="$binary/bin/profile"
function VerifyGenerated32 {
    $dlls=@(Get-ChildItem -LiteralPath $generated -Filter '*.dll' -File)
    if ($dlls.Count -ne 32 -or (($dlls.Name | Sort-Object) -join '|') -cne (($recipe.expectedRuntimeNames | Sort-Object) -join '|')) { throw 'Exact32 generated deployment names required.' }
    foreach ($dll in $dlls) {
        NoReparse $dll.FullName
        $expected=if ($providerMap.ContainsKey($dll.Name)) { $providerMap[$dll.Name] } else { @($runtime | Where-Object { $_.name -ceq $dll.Name })[0] }
        Verify @([pscustomobject]@{path=$dll.FullName;bytes=$expected.bytes;sha256=$expected.sha256})
    }
}
function RequireOneOwnLink($path,$name) {
    NoReparse $path
    if (!(SamePath $path "$binary/bin/profile/$name") -or [IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($path)) -ine [IO.Path]::GetFullPath($generated)) { throw 'Deployment repair escaped owned generated directory.' }
    $lines=@(& $completion.fsutilPin.path hardlink list $path 2>&1); $nativeCode=$LASTEXITCODE
    $text=($lines | ForEach-Object { [string]$_ }) -join "`r`n"
    [IO.File]::WriteAllText("$out/hardlinks-$name.stdout.log",$text,[Text.UTF8Encoding]::new($false))
    if ($nativeCode -ne 0) { throw "Hardlink inspection failed: $nativeCode" }
    $links=@($lines | ForEach-Object { ([string]$_).Trim() } | Where-Object { $_ })
    $exact=[IO.Path]::GetFullPath($path).Substring(2)
    if ($links.Count -ne 1 -or $links[0] -ine $exact) { throw 'Repair requires one regular own file link, never a baseline hardlink.' }
    Save "hardlinks-$name.json" @{path=$path;nativeExit=$nativeCode;links=$links;fsutil=$completion.fsutilPin}
}
New-Item -ItemType Directory -Path $out -ErrorAction Stop | Out-Null
$failure=$null; $overlayComplete=$true
try {
    Save 'preflight.json' @{originalRunner=$completion.originalRunnerPin;binding=$completion.bindingPin;completionInputs=(Pin $inputPath);functions=$functionProof;reusedNativePhases=@('configure-consumer','build-consumer');pendingNativePhases=@('pe-imports','loader');noConfigureBuildInstall=$true}
    # Re-evaluate the exact existing reports and actual command-log checks, with no native regeneration.
    . ([scriptblock]::Create((OriginalRange '    $reports = @(Get-ChildItem' "    Run 'build-consumer'")))
    . ([scriptblock]::Create((OriginalRange '    $commands = @(Get-ChildItem' '    $generated = ')))
    VerifyBaseline; VerifyOverlay
    $repairs=@()
    if (@($completion.permittedOldDeploymentPins).Count -ne 2) { throw 'Exactly two reviewed stale deployment copies required.' }
    foreach ($name in @('RenderIntegration.dll','EditorRenderIntegration.dll')) {
        $old=@($completion.permittedOldDeploymentPins | Where-Object { [IO.Path]::GetFileName($_.path) -ceq $name })
        $baseline=@($runtime | Where-Object { $_.name -ceq $name })
        if ($old.Count -ne 1 -or $baseline.Count -ne 1 -or $old[0].sha256 -cne $baseline[0].sha256 -or $old[0].bytes -ne $baseline[0].bytes) { throw 'Stale copy is not the exact SDK96 baseline.' }
        Verify @($old[0]); RequireOneOwnLink $old[0].path $name
        $backup="$out/deploy-before/$name"
        CopyPinned $old[0] $backup
        Verify @($old[0]); NoReparse $old[0].path; Verify @($providerMap[$name])
        [IO.File]::Copy("$overlay/bin/$name",$old[0].path,$true)
        Verify @([pscustomobject]@{path=$old[0].path;bytes=$providerMap[$name].bytes;sha256=$providerMap[$name].sha256})
        $repairs+=@{before=$old[0];backup=(Pin $backup);after=(Pin $old[0].path);source=(Pin "$overlay/bin/$name")}
    }
    Save 'deployment-repair.json' @{files=$repairs;count=2;publicBodyNotCopiedAgain=$true;baselineSdkNotWritten=$true}
    VerifyBaseline; VerifyOverlay; VerifyGenerated32; Verify $completion.consumerPins
    Run 'pe-imports' $recipe.dumpbin @('/imports',"$generated/JENativeSdkConsumer.exe") $generated
    $pe = [IO.File]::ReadAllText("$out/pe-imports.stdout.log")
    foreach ($symbol in @('CreateRpiRuntimeModule','CreateRpiEditorModule','CreateBootstrapModule','CreateToolsModule','CreateShaderCompilerToolsModule','HashCombineShaderOptionValues@HashedVariantInfoSourceData@Shader@RenderBuilders@Assets@JE@@','??0ModelAssetBuilderContext','??0MaterialAssetBuilderContext','??0ModelAssetPostBuildContext','RTTI_IsTypeOf@ModelAssetBuilderContext','RTTI_IsTypeOf@MaterialAssetBuilderContext','RTTI_IsTypeOf@ModelAssetPostBuildContext','RenderIntegration.dll','EditorRenderIntegration.dll','RenderAssetBuilders.dll')) { if (!$pe.Contains($symbol)) { throw "Real import absent: $symbol" } }
    CopyPinned (Pin "$generated/JENativeSdkConsumer.exe") $recipe.loaderExecutable
    if (Test-Path -LiteralPath "$generated/JENativeSdkConsumer.pdb") { CopyPinned (Pin "$generated/JENativeSdkConsumer.pdb") "$overlay/bin/JENativeSdkConsumer.pdb" }
    VerifyOverlay
    Run 'loader' $recipe.loaderExecutable $recipe.loaderArgv $recipe.loaderCwd $recipe.loaderPath $recipe.loaderTimeoutMs
    $stdout = [IO.File]::ReadAllText("$out/loader.stdout.log"); $stderr = [IO.File]::ReadAllText("$out/loader.stderr.log")
    foreach ($message in @('JE SDK: Materials, Camera and RenderIntegration exports resolved; empty PassPipelinePreview exercised.','JE SDK: native RenderAssetBuilders factory/descriptors/recreation and three public contexts exercised without activation or cook.','JE SDK: shader compiler factory/descriptors/services/recreation and exported shader option hash exercised without activation or cook.','JE SDK: native Bootstrap true/false factory/descriptors/services/reflection/recreation and stable buses exercised without activation or window.',$recipe.expectedNewMessage)) { if (!$stdout.Contains($message)) { throw 'Missing actual lifecycle success message.' } }
    if (($stdout+$stderr) -match '(?i)(assert|access violation|allocator.*(error|leak)|unhandled exception)') { throw 'Native diagnostic error.' }
    VerifyBaseline; VerifyOverlay; VerifyEngine; Verify $binding.providerPins; Verify $binding.headerPins; Verify @($binding.publicBodyDllPin,$binding.publicPeProofPin,$binding.publicXmlPin)
    Save 'summary.json' @{status='FOCAL_API_ABI_LIFECYCLE_COMPLETE_PARENT_ACCEPTANCE_PENDING';nativeExit=0;runtimeDlls=32;copiedDlls=3;hardlinks=29;imports6=$observedTargets;publicBody=(Pin "$overlay/bin/Atom_RPI.Public.dll");newFactoryLifecycles=2;activationOrGpu=$false;sdk96Mutation=$false;consumer96Mutation=$false}
} catch {
    $failure = $_.Exception.Message
    Save 'failure.json' @{error=$failure;nativePass=$false;noAutomaticRetry=$true}
} finally {
    $postErrors = @()
    try { VerifyBaseline; VerifyEngine; Verify $binding.providerPins; Verify $binding.headerPins; Verify @($binding.publicBodyDllPin,$binding.publicPeProofPin,$binding.publicXmlPin) } catch { $postErrors += $_.Exception.Message }
    if ($overlayComplete) { try { VerifyOverlay } catch { $postErrors += $_.Exception.Message } }
    $roots = @()
    foreach ($startFile in @(Get-ChildItem -LiteralPath $out -Filter '*.start.json' -File)) {
        $start = Json $startFile.FullName; $live = Get-CimInstance Win32_Process -Filter "ProcessId=$($start.pid)"
        $same = $live -and $live.ExecutablePath -and (SamePath $live.ExecutablePath $start.executable) -and ([datetime]$live.CreationDate).ToUniversalTime().Ticks -eq ([datetime]$start.creationUtc).ToUniversalTime().Ticks
        $roots += @{pid=$start.pid;creationUtc=([datetime]$start.creationUtc).ToUniversalTime().ToString('o');livePidObserved=[bool]$live;stillSameIdentity=[bool]$same}
        if ($same) { $postErrors += "Own phase root remains: $($start.pid)" }
    }
    Save 'postflight.json' @{error=$failure;verificationErrors=$postErrors;phaseRoots=$roots;baseline32AndBoundary16Unchanged=($postErrors.Count -eq 0);sdk96AndConsumer96NeverWrittenByWrapper=$true;noGlobalCleanup=$true}
    if ($postErrors.Count) { throw ($postErrors -join '; ') }
}
if ($failure) { throw $failure }

Verify $completion.closedEvidencePins; Verify $completion.consumerPins; VerifyGenerated32
Save 'completion-postflight.json' @{closedEvidenceUnchanged=$true;consumerExePdbUnchanged=$true;generated32=@(Get-ChildItem -LiteralPath $generated -File -Filter '*.dll' | ForEach-Object { Pin $_.FullName });nativePhasesExecuted=@('pe-imports','loader');configureBuildInstallRepeated=$false;parentAcceptancePending=$true}
