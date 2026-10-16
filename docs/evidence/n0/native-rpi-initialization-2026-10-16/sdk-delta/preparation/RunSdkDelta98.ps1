# Private targeted SDK delta. No native phase or mount is created without -Execute and parent binding.
param([switch]$Execute, [string]$BindingPath, [string]$BindingSHA256)
$ErrorActionPreference = 'Stop'
if (!$Execute) { Write-Output 'PREPARED_NOT_RUN: parent-reviewed binding and exclusive native lease required.'; return }
$repo = 'F:/JericoEngine/JericoEngine'
$producer = "$repo/build/n0-validation/original-engine"
$recipePath = "$PSScriptRoot/recipe.json"
if ((Get-FileHash -LiteralPath $recipePath).Hash -cne '6B153CE1D33AC7CF4F6F9EB9DB955C44BCA41E243CFBAC7455FB1DF87FBF966F') { throw 'Approved recipe drift.' }
$manifestPath = "$PSScriptRoot/pins.json"
if ((Get-FileHash -LiteralPath $manifestPath).Hash -cne '19E15DAF7295590B061A4834F7A670949F6A66ABF9A961D85238A1F19A1453D9') { throw 'Reviewed candidate manifest drift.' }
foreach ($pin in @(Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json)) {
    $path = Join-Path $PSScriptRoot $pin.relative
    if ((Get-Item -LiteralPath $path).Length -ne $pin.bytes -or (Get-FileHash -LiteralPath $path).Hash -cne $pin.sha256) { throw "Reviewed candidate input drift: $($pin.relative)" }
}
$recipe = Get-Content -LiteralPath $recipePath -Raw | ConvertFrom-Json
$sdk = [IO.Path]::GetFullPath($recipe.baselineSdk)
$binary = [IO.Path]::GetFullPath($recipe.newConsumerBuild)
$overlay = [IO.Path]::GetFullPath($recipe.newOverlay)
$out = [IO.Path]::GetFullPath($recipe.newEvidence)
function SamePath($a, $b) { [IO.Path]::GetFullPath($a).TrimEnd('\','/') -ieq [IO.Path]::GetFullPath($b).TrimEnd('\','/') }
function NoReparse($path) {
    $current = [IO.Path]::GetFullPath($path)
    while ($current) {
        if ((Test-Path -LiteralPath $current) -and ((Get-Item -LiteralPath $current -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw "Reparse point: $current" }
        $next = [IO.Path]::GetDirectoryName($current)
        if ($next -eq $current) { break }; $current = $next
    }
}
if (!(SamePath $sdk "$repo/build/sdk-vk96") -or !(SamePath $binary "$repo/build/sdk-vk-consumer98") -or !(SamePath $overlay "$repo/build/sdk-rpi98-delta") -or !(SamePath $out "$PSScriptRoot/execution-delta98-attempt1")) { throw 'Unexpected SDK/build/overlay/evidence layout.' }
foreach ($path in @($sdk,$binary,$overlay,$out,$PSScriptRoot)) { NoReparse $path }
foreach ($path in @($binary,$overlay,$out)) { if (Test-Path -LiteralPath $path) { throw "Fresh absent destination required: $path" } }
if (!$BindingPath -or $BindingSHA256 -cnotmatch '^[0-9A-F]{64}$' -or (Get-FileHash -LiteralPath $BindingPath).Hash -cne $BindingSHA256) { throw 'Exact parent binding pin required.' }
$binding = Get-Content -LiteralPath $BindingPath -Raw | ConvertFrom-Json
if ($binding.approved -isnot [bool] -or !$binding.approved -or $binding.reviewer -cne '/root' -or $binding.revision -ne 98) { throw 'Parent-approved revision98 binding required.' }
# Import only seven audited function definitions. Never dot-source or invoke the installer top level.
$boundary = Get-Content -LiteralPath $recipe.baselineBoundaryPins -Raw | ConvertFrom-Json
$functionSource = $recipe.guardReuse.source
$functionPin = @($boundary | Where-Object { SamePath $_.path $functionSource })
if ($functionPin.Count -ne 1 -or (Get-FileHash -LiteralPath $functionSource).Hash -cne $functionPin[0].sha256) { throw 'Audited function source drift.' }
$tokens = $null; $parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($functionSource, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw 'Cannot parse audited SDK96 function source.' }
$functionProof = @()
foreach ($name in @('Json','Save','Pin','Verify','NativeAdmission','Resources','Run')) {
    $nodes = @($ast.FindAll({ param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -ceq $name }, $true))
    if ($nodes.Count -ne 1) { throw "Expected one audited function: $name" }
    . ([scriptblock]::Create($nodes[0].Extent.Text))
    $functionProof += @{name=$name;startLine=$nodes[0].Extent.StartLineNumber;endLine=$nodes[0].Extent.EndLineNumber;text=$nodes[0].Extent.Text}
}
Resources; NativeAdmission
$runtime = Json $recipe.baselineRuntimePins
$publicEvidence = Json $recipe.publicBodyEvidencePath
if (@($runtime).Count -ne 32 -or @($runtime.name | Select-Object -Unique).Count -ne 32 -or (($runtime.name | Sort-Object) -join '|') -cne (($recipe.expectedRuntimeNames | Sort-Object) -join '|')) { throw 'Exact baseline32 closure required.' }
if (@($recipe.sourcePins).Count -ne 4 -or @($recipe.headerPins).Count -ne 2 -or @($boundary).Count -ne 16) { throw 'Expected four consumer sources, two headers and sixteen immutable boundary pins.' }
function VerifyBaseline { Verify $runtime; Verify $boundary; Verify $recipe.sourcePins; Verify $recipe.headerPins; Verify $publicEvidence.sdkBoundaryPins }
foreach ($pin in @($runtime)+@($boundary)+@($recipe.sourcePins)+@($recipe.headerPins)+@($publicEvidence.sdkBoundaryPins)) { NoReparse $pin.path }
if (!(SamePath $binding.wrapperPin.path $PSCommandPath) -or !(SamePath $binding.recipePin.path $recipePath)) { throw 'Parent binding must cover this exact wrapper and recipe.' }
Verify @($binding.wrapperPin,$binding.recipePin)
VerifyBaseline
if ($binding.freezeSHA256 -cne $binding.freezePin.sha256) { throw 'Binding freeze relationship is not explicit.' }
if ($binding.freezePin.sha256 -cne $publicEvidence.freezePin.sha256 -or !(SamePath $binding.freezePin.path $publicEvidence.freezePin.path)) { throw 'Binding must use the reviewed final GREEN R5 freeze.' }
Verify @($binding.freezePin)
$frozen = Json $binding.freezePin.path
if (@($frozen.inputs).Count -ne 14063 -or @($frozen.absentInputs).Count -ne 61) { throw 'Exact final98 engine cohort14063/61 required after metadata-only RPI asset registration.' }
function VerifyEngine {
    Verify $frozen.inputs
    foreach ($path in $frozen.absentInputs) { if (Test-Path -LiteralPath $path) { throw "Retired engine input returned: $path" } }
}
VerifyEngine
if (@($binding.greenXmlPins).Count -ne 2 -or !@($binding.hostEvidencePins).Count -or @($binding.providerPins).Count -ne 4 -or @($binding.headerPins).Count -ne 2 -or @($binding.exportPins).Count -ne 2) { throw 'Binding requires two GREEN XML, host evidence, four providers, two headers and two exports.' }
Verify $binding.greenXmlPins; Verify $binding.hostEvidencePins; Verify $binding.providerPins; Verify $binding.headerPins; Verify $binding.exportPins
$completedCases = @()
foreach ($pin in $binding.greenXmlPins) {
    [xml]$xml = [IO.File]::ReadAllText($pin.path)
    foreach ($attribute in @('failures','errors','disabled')) { if ($xml.DocumentElement.GetAttribute($attribute) -cne '0') { throw "Native GREEN XML has nonzero/missing $attribute." } }
    foreach ($case in @($xml.SelectNodes('//testcase'))) {
        if ($case.SelectNodes('failure|error|skipped').Count -or $case.status -cne 'run' -or $case.result -cne 'completed') { throw 'GREEN evidence contains a failed/incomplete case.' }
        $completedCases += "$($case.classname).$($case.name)"
    }
}
if ($completedCases.Count -ne 12 -or @($completedCases | Select-Object -Unique).Count -ne 12) { throw 'Exact twelve completed native factory/serialization controls required.' }
foreach ($field in @('publicBodyDllPin','publicPeProofPin','publicXmlPin')) {
    $expected = $publicEvidence.$field; $actual = $binding.$field
    if (!(SamePath $actual.path $expected.path) -or $actual.bytes -ne $expected.bytes -or $actual.sha256 -cne $expected.sha256) { throw "Exact supplemental Public evidence required: $field" }
    Verify @($actual)
}
if (!(SamePath $binding.publicBodyDllPin.path "$producer/bin/profile/Atom_RPI.Public.dll")) { throw 'Public body is not the authoritative Profile DLL.' }
NoReparse $binding.publicBodyDllPin.path
$publicPe = Json $binding.publicPeProofPin.path
if ($publicPe.owner -cne '/root' -or !$publicPe.publicBodyOnlyDelta -or $publicPe.publicExportOrdinalsCompared -ne 7836 -or $publicPe.publicAbiDiffCount -ne 0 -or !$publicPe.publicDependentsSame -or @($publicPe.publicDependents).Count -ne 13) { throw 'Reviewed Public export/ordinal/dependency identity proof required.' }
$currentPublicProof = @($publicPe.results | Where-Object { $_.name -ceq 'public-current-exports' })
$baselinePublicProof = @($publicPe.results | Where-Object { $_.name -ceq 'public-baseline-exports' })
$baselinePublic = @($runtime | Where-Object { $_.name -ceq 'Atom_RPI.Public.dll' })
if ($currentPublicProof.Count -ne 1 -or $baselinePublicProof.Count -ne 1 -or $baselinePublic.Count -ne 1 -or $currentPublicProof[0].inputSHA256 -cne $binding.publicBodyDllPin.sha256 -or $baselinePublicProof[0].inputSHA256 -cne $baselinePublic[0].sha256) { throw 'Public ABI proof does not bind the actual current and SDK96 DLLs.' }
[xml]$publicXml = [IO.File]::ReadAllText($binding.publicXmlPin.path)
foreach ($attribute in @('failures','errors','disabled')) { if ($publicXml.DocumentElement.GetAttribute($attribute) -cne '0') { throw "Public576 XML has nonzero/missing $attribute." } }
$publicCases = @($publicXml.SelectNodes('//testcase'))
$publicIdentities = @()
foreach ($case in $publicCases) {
    if ($case.SelectNodes('failure|error|skipped').Count -or $case.status -cne 'run' -or $case.result -cne 'completed') { throw 'Public576 evidence contains a failed/incomplete case.' }
    $publicIdentities += "$($case.classname).$($case.name)"
}
if ($publicCases.Count -ne 576 -or @($publicIdentities | Select-Object -Unique).Count -ne 576) { throw 'Exact576 completed Public cases required in addition to twelve native cases.' }
$providerMap = @{}
foreach ($name in @('RenderIntegration','EditorRenderIntegration')) {
    foreach ($extension in @('dll','lib')) {
        $pins = @($binding.providerPins | Where-Object { [IO.Path]::GetFileName($_.path) -ceq "$name.$extension" })
        if ($pins.Count -ne 1) { throw "Exact provider required: $name.$extension" }
        $directory = if ($extension -eq 'dll') { 'bin' } else { 'lib' }
        if (!(SamePath $pins[0].path "$producer/$directory/profile/$name.$extension")) { throw 'Provider is not the authoritative Profile output.' }
        NoReparse $pins[0].path; $providerMap["$name.$extension"] = $pins[0]
    }
}
$providerMap['Atom_RPI.Public.dll'] = $binding.publicBodyDllPin
$headerMap = @{
    'JE/Render/Integration/RenderIntegrationModule.h' = "$repo/Engine/Render/Integration/Include/JE/Render/Integration/RenderIntegrationModule.h"
    'JE/Editor/RenderIntegration/RenderIntegrationModule.h' = "$repo/Editor/Render/Integration/Include/JE/Editor/RenderIntegration/RenderIntegrationModule.h"
}
foreach ($pin in $recipe.headerPins) {
    $actual = @($binding.headerPins | Where-Object { SamePath $_.path $headerMap[$pin.relative] })
    if ($actual.Count -ne 1 -or $actual[0].sha256 -cne $pin.sha256 -or $actual[0].bytes -ne $pin.bytes) { throw 'Public header binding does not match reviewed exact copy.' }
}
foreach ($name in @('RenderIntegration','EditorRenderIntegration')) {
    $exports = @($binding.exportPins | Where-Object { $_.provider -ceq $name })
    if ($exports.Count -ne 1) { throw "Exact measured export report required: $name" }
    $text = [IO.File]::ReadAllText($exports[0].path)
    $symbols = if ($name -eq 'RenderIntegration') { @('CreateRpiRuntimeModule','CreateRuntimeModule','CreateBootstrapModule') } else { @('CreateRpiEditorModule','CreateAuthoringModule') }
    foreach ($symbol in $symbols) { if (!$text.Contains($symbol)) { throw "Native provider export absent: $symbol" } }
}
function CopyPinned($pin, $destination) {
    NoReparse $destination
    if (Test-Path -LiteralPath $destination) { throw "No overwrite: $destination" }
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination)) | Out-Null
    [IO.File]::Copy($pin.path,$destination,$false)
    Verify @([pscustomobject]@{path=$destination;bytes=$pin.bytes;sha256=$pin.sha256})
}
function CheckNoEngineFallback($text) {
    $normal = $text.Replace('\','/').ToLowerInvariant()
    foreach ($part in @('Engine','Editor','Code','Gems','Assets','Registry')) {
        if ($normal.Contains(($repo+'/'+$part+'/').ToLowerInvariant())) { throw "Source-engine path in external consumer: $part" }
    }
    foreach ($part in @('bin','lib','Engine','Editor','Code','Gems')) {
        if ($normal.Contains(($producer+'/'+$part+'/').ToLowerInvariant())) { throw "Original-engine output/include fallback: $part" }
    }
}
function VerifyOverlay {
    $dlls = @(Get-ChildItem -LiteralPath "$overlay/bin" -Filter '*.dll' -File)
    if ($dlls.Count -ne 32 -or (($dlls.Name | Sort-Object) -join '|') -cne (($recipe.expectedRuntimeNames | Sort-Object) -join '|')) { throw 'Overlay runtime closure changed.' }
    foreach ($dll in $dlls) {
        NoReparse $dll.FullName
        $expected = if ($providerMap.ContainsKey($dll.Name)) { $providerMap[$dll.Name] } else { @($runtime | Where-Object { $_.name -ceq $dll.Name })[0] }
        Verify @([pscustomobject]@{path=$dll.FullName;bytes=$expected.bytes;sha256=$expected.sha256})
    }
    foreach ($name in @('RenderIntegration.lib','EditorRenderIntegration.lib')) { $expected=$providerMap[$name]; Verify @([pscustomobject]@{path="$overlay/lib/$name";bytes=$expected.bytes;sha256=$expected.sha256}) }
    foreach ($pin in $recipe.headerPins) { Verify @([pscustomobject]@{path="$overlay/include/$($pin.relative)";bytes=$pin.bytes;sha256=$pin.sha256}) }
}
New-Item -ItemType Directory -Path $out -ErrorAction Stop | Out-Null
$failure = $null
$overlayComplete = $false
try {
    Save 'preflight.json' @{binding=(Pin $BindingPath);recipe=(Pin $recipePath);functions=$functionProof;baseline32=$runtime;boundary16=$boundary;publicEvidence=$publicEvidence;source6=@($recipe.sourcePins)+@($recipe.headerPins);greenCases=$completedCases;publicCaseCount=$publicCases.Count;status='READY_TO_EXECUTE_ONLY_WITH_PARENT_LEASE'}
    New-Item -ItemType Directory -Path $overlay -ErrorAction Stop | Out-Null
    foreach ($directory in @('bin','lib','include')) { New-Item -ItemType Directory -Path "$overlay/$directory" -ErrorAction Stop | Out-Null }
    foreach ($pin in $runtime) {
        $destination = "$overlay/bin/$($pin.name)"
        if ($providerMap.ContainsKey($pin.name)) { CopyPinned $providerMap[$pin.name] $destination }
        else {
            if (Test-Path -LiteralPath $destination) { throw 'Hardlink destination exists.' }
            New-Item -ItemType HardLink -Path $destination -Target $pin.path -ErrorAction Stop | Out-Null
            Verify @([pscustomobject]@{path=$destination;bytes=$pin.bytes;sha256=$pin.sha256})
        }
    }
    foreach ($name in @('RenderIntegration.lib','EditorRenderIntegration.lib')) { CopyPinned $providerMap[$name] "$overlay/lib/$name" }
    foreach ($pin in $recipe.headerPins) { CopyPinned $pin "$overlay/include/$($pin.relative)" }
    VerifyOverlay; $overlayComplete = $true
    Save 'overlay-before-configure.json' @{files=@(Get-ChildItem -LiteralPath $overlay -Recurse -File | ForEach-Object { Pin $_.FullName });hardlinksReadOnlyByUse=29;copiedDlls=3;copiedImportlibs=2;copiedHeaders=2}
    if (Test-Path -LiteralPath $binary) { throw 'Consumer destination appeared before configure.' }
    Run 'configure-consumer' $recipe.executable $recipe.configureArgv $recipe.configureCwd
    VerifyBaseline; VerifyOverlay
    $reports = @(Get-ChildItem -LiteralPath $binary -Filter 'native-*-profile.txt' -File)
    if ($reports.Count -ne 6) { throw 'Exactly six installed-target reports required.' }
    $observedTargets = @()
    foreach ($report in $reports) {
        $data = @{}; foreach ($line in [IO.File]::ReadAllLines($report.FullName)) { $parts=$line.Split('=',2); if ($parts.Count -eq 2) { $data[$parts[0]]=$parts[1] } }
        $observedTargets += $data.target
        if ($data.imported -cne 'TRUE') { throw 'Non-imported target in external consumer.' }
        $real = $data.target.Split('::')[-1]
        if ($real -in @('RenderIntegration','EditorRenderIntegration')) {
            if (!(SamePath $data.location "$overlay/bin/$real.dll") -or !(SamePath $data.implib "$overlay/lib/$real.lib")) { throw 'Reviewed provider override not effective.' }
        } elseif (!$data.location.Replace('\','/').StartsWith($sdk.Replace('\','/')+'/',[StringComparison]::OrdinalIgnoreCase) -or !$data.implib.Replace('\','/').StartsWith($sdk.Replace('\','/')+'/',[StringComparison]::OrdinalIgnoreCase)) { throw 'Unchanged imported target escaped SDK96.' }
        CheckNoEngineFallback ([IO.File]::ReadAllText($report.FullName))
    }
    if ((($observedTargets | Sort-Object) -join '|') -cne (($recipe.importedTargets | Sort-Object) -join '|')) { throw 'Six imported-target identity drift.' }
    $publicReportPath = Join-Path $binary $recipe.publicBodyReport
    $publicReport = @{}; foreach ($line in [IO.File]::ReadAllLines($publicReportPath)) { $parts=$line.Split('=',2); if ($parts.Count -eq 2) { $publicReport[$parts[0]]=$parts[1] } }
    $publicImplib = @($publicEvidence.sdkBoundaryPins | Where-Object { [IO.Path]::GetFileName($_.path) -ceq 'Atom_RPI.Public.lib' })
    if ($publicImplib.Count -ne 1 -or $publicReport.target -cne 'Gem::Atom_RPI.Public' -or $publicReport.imported -cne 'TRUE' -or !(SamePath $publicReport.location "$overlay/bin/Atom_RPI.Public.dll") -or !(SamePath $publicReport.implib $publicImplib[0].path)) { throw 'Public body-only target override or retained SDK96 import library is not effective.' }
    CheckNoEngineFallback ([IO.File]::ReadAllText($publicReportPath))
    $projects = @(Get-ChildItem -LiteralPath $binary -Recurse -Filter 'JENativeSdkConsumer.vcxproj' -File)
    if ($projects.Count -ne 1) { throw 'Exactly one external consumer project required.' }
    $project = [IO.File]::ReadAllText($projects[0].FullName); CheckNoEngineFallback $project
    foreach ($token in @('stdcpp20','MultiThreadedDLL','<UseStandardPreprocessor>true</UseStandardPreprocessor>')) { if (!$project.Contains($token)) { throw "Compiler contract missing: $token" } }
    Save 'configure-boundary.json' @{reports=@($reports | ForEach-Object { Pin $_.FullName });publicBodyReport=(Pin $publicReportPath);retainedPublicImplib=$publicImplib[0];project=(Pin $projects[0].FullName);targets=$observedTargets;sourceEngineFallback=$false}
    Run 'build-consumer' $recipe.executable $recipe.buildArgv $recipe.buildCwd
    VerifyBaseline; VerifyOverlay
    $commands = @(Get-ChildItem -LiteralPath $binary -Recurse -File | Where-Object { $_.Name -in @('CL.command.1.tlog','link.command.1.tlog') })
    if (!$commands.Count) { throw 'Actual compiler/link command logs not observed.' }
    foreach ($command in $commands) { CheckNoEngineFallback ([IO.File]::ReadAllText($command.FullName)) }
    Save 'actual-command-boundary.json' @{commands=@($commands | ForEach-Object { Pin $_.FullName });sourceEngineFallback=$false}
    $generated = "$binary/bin/profile"
    $dlls = @(Get-ChildItem -LiteralPath $generated -Filter '*.dll' -File)
    if ($dlls.Count -ne 32 -or (($dlls.Name | Sort-Object) -join '|') -cne (($recipe.expectedRuntimeNames | Sort-Object) -join '|')) { throw 'Exact32 generated deployment names required.' }
    foreach ($dll in $dlls) {
        $expected = if ($providerMap.ContainsKey($dll.Name)) { $providerMap[$dll.Name] } else { @($runtime | Where-Object { $_.name -ceq $dll.Name })[0] }
        Verify @([pscustomobject]@{path=$dll.FullName;bytes=$expected.bytes;sha256=$expected.sha256})
    }
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
