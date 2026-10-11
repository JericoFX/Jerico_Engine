$ErrorActionPreference = 'Stop'
$repo = 'F:/JericoEngine/JericoEngine'
$base = "$repo/build/n0-validation/local-packages"
function Write-NewJson([string]$path, $value) {
    if (Test-Path -LiteralPath $path) { throw "Existing evidence must be preserved: $path" }
    [IO.File]::WriteAllText($path, ($value | ConvertTo-Json -Depth 7) + "`n", [Text.UTF8Encoding]::new($false))
}
$oldFreezePath = "$base/source-freeze-root-googletest-mcpp-splash.json"
$oldAssignmentPath = "$base/root-googletest-mcpp-splash-assignment.json"
if ((Get-FileHash -LiteralPath $oldFreezePath -Algorithm SHA256).Hash -ne 'A1DA254EBC36446F21E61F41220AD90D26A7CF7A99EC8174AF36AAAB5038FD75') { throw 'Historical Root414 freeze changed' }
if ((Get-FileHash -LiteralPath $oldAssignmentPath -Algorithm SHA256).Hash -ne 'BB8093B78C55B75DCEEFBA43479496CCDC46B9D41631A08554E653FC33E4748A') { throw 'Historical Root414 assignment changed' }
$oldFreeze = Get-Content -LiteralPath $oldFreezePath -Raw | ConvertFrom-Json
$oldAssignment = Get-Content -LiteralPath $oldAssignmentPath -Raw | ConvertFrom-Json
if ($oldFreeze.Count -ne 414 -or @($oldAssignment.arguments).Count -ne 45) { throw 'Historical scope mismatch' }
$allowedOldChanges = @('cmake/3rdParty/Platform/Windows/BuiltInPackages_windows.cmake', 'cmake/Tools/LocalPackages/mcpp/README.md')
$oldDeltas = @()
$paths = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach ($row in $oldFreeze) {
    $relative = $row.path.Replace('\', '/')
    $absolute = Join-Path $repo $relative
    $hash = (Get-FileHash -LiteralPath $absolute -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($hash -ne $row.sha256) {
        if ($relative -notin $allowedOldChanges) { throw "Unexpected Root414 change: $relative" }
        $oldDeltas += [pscustomobject]@{ path=$relative; beforeSHA256=$row.sha256; afterSHA256=$hash }
    }
    [void]$paths.Add($relative)
}
if ($oldDeltas.Count -ne 2) { throw 'Expected catalogue and mcpp README changes were not found' }
$providers = @(@{name='Expat';count=8}, @{name='TIFF';count=9}, @{name='PNG';count=7}, @{name='d3dx12';count=6})
foreach ($provider in $providers) {
    $files = @(Get-ChildItem -LiteralPath "$repo/cmake/Tools/LocalPackages/$($provider.name)" -File -Recurse -Force)
    if ($files.Count -ne $provider.count) { throw "Provider source scope changed: $($provider.name)" }
    foreach ($file in $files) { [void]$paths.Add($file.FullName.Substring($repo.Length + 1).Replace('\', '/')) }
}
$newFreeze = @($paths | Sort-Object | ForEach-Object {
    $absolute = Join-Path $repo $_
    [pscustomobject]@{ path=$_; bytes=(Get-Item -LiteralPath $absolute).Length; sha256=(Get-FileHash -LiteralPath $absolute -Algorithm SHA256).Hash.ToLowerInvariant() }
})
if ($newFreeze.Count -ne 444) { throw "New Root scope mismatch: $($newFreeze.Count)" }
$freezePath = "$base/source-freeze-root-native-packages.json"
Write-NewJson $freezePath $newFreeze
$freezeSha = (Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash

$oldMcppPath = "$base/mcpp-audit/source-freeze-parent17-rev2.json"
$oldMcpp = Get-Content -LiteralPath $oldMcppPath -Raw | ConvertFrom-Json
$mcppDeltas = @()
$mcppInputs = @($oldMcpp.inputs | ForEach-Object {
    $hash = (Get-FileHash -LiteralPath $_.path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($hash -ne $_.sha256) {
        if ($_.path -ne "$repo/cmake/Tools/LocalPackages/mcpp/README.md") { throw "Unexpected mcpp native input change: $($_.path)" }
        $mcppDeltas += [pscustomobject]@{path=$_.path;beforeSHA256=$_.sha256;afterSHA256=$hash}
    }
    [pscustomobject]@{path=$_.path;bytes=(Get-Item -LiteralPath $_.path).Length;sha256=$hash}
})
if ($mcppInputs.Count -ne 17 -or $mcppDeltas.Count -ne 1) { throw 'mcpp current scope mismatch' }
$mcppPath = "$base/mcpp-audit/source-freeze-root-current17.json"
Write-NewJson $mcppPath ([ordered]@{schema=1;purpose='Current Root preflight; native rev2 proof remains historical, only README changed after validation';previousFreeze=$oldMcppPath;previousFreezeSHA256=(Get-FileHash -LiteralPath $oldMcppPath -Algorithm SHA256).Hash;inputCount=17;inputs=$mcppInputs;approvedDocumentationDelta=$mcppDeltas})
$mcppSha = (Get-FileHash -LiteralPath $mcppPath -Algorithm SHA256).Hash
$cacheCount = @(Get-ChildItem -LiteralPath "$base/root-package-cache" -File -Force).Count
if ($cacheCount -ne 30) { throw "Unexpected root cache count: $cacheCount" }
$assignmentPath = "$base/root-native-packages-assignment.json"
$assignment = [ordered]@{
    purpose='Root configure with the same 45 offline arguments after accepted Expat/TIFF/PNG/d3dx12 packages; no native product acceptance inferred'
    sourceFreeze=$freezePath; sourceFreezeSHA256=$freezeSha; sourceCount=444
    arguments=[string[]]$oldAssignment.arguments; previousRoot414=$oldAssignmentPath
    buildOwner='/root/validation_luna'; nativeEditorStatus='NOT COMPILED / NOT EXECUTED'
    privateSourceCopyAssignment=$oldAssignment.privateSourceCopyAssignment
    privateSourceCopyAssignmentSHA256=$oldAssignment.privateSourceCopyAssignmentSHA256
    allowedPrivateCopyWrites=$oldAssignment.allowedPrivateCopyWrites
    googletestFreeze=$oldAssignment.googletestFreeze; googletestFreezeSHA256=$oldAssignment.googletestFreezeSHA256
    mcppFreeze=$mcppPath; mcppFreezeSHA256=$mcppSha
    approvedPriorRootDeltas=$oldDeltas; packageCacheFileCount=30
    newAcceptedPackages=@(
        @{name='expat-2.4.2-je-rev1-windows';sha256='c102370257ed28d9f9ea5e61ca10432edc14e8389aa781380beaa9b2a8ae6925'},
        @{name='tiff-4.2.0.15-je-rev1-windows';sha256='d54ecd9a2463472e0df48e9baff07744ca1512fe761bc142ade353b347bf1dd2'},
        @{name='png-1.6.37-je-rev1-windows';sha256='7d13e9a70172f713f66daf870a7359191096308ebe501c17121bfb24dbefe9b4'},
        @{name='d3dx12-headers-je-rev1-windows';sha256='5ba66c64483f19e72beea0d1ccc94ad3e0175e88ec256668fc5b0725727ffb73'}
    )
    pythonPolicy='External build/codegen tool only; Engine/Game/Editor link/deploy/runtime absence still unverified'
}
foreach ($package in $assignment.newAcceptedPackages) {
    if ((Get-FileHash -LiteralPath "$base/root-package-cache/$($package.name).tar.xz" -Algorithm SHA256).Hash -ine $package.sha256) { throw "Accepted package changed: $($package.name)" }
}
Write-NewJson $assignmentPath $assignment
$assignmentSha = (Get-FileHash -LiteralPath $assignmentPath -Algorithm SHA256).Hash
$template = "$base/original-engine-googletest-mcpp-splash-configure/run-root-configure-googletest.ps1"
if ((Get-FileHash -LiteralPath $template -Algorithm SHA256).Hash -ne '170C8E860A51E0BF1925478687AA9F3670224E83313D9FB30C2400248D5BC470') { throw 'Historical working driver changed' }
$driver = [IO.File]::ReadAllText($template)
$changes = @(
    @('original-engine-googletest-mcpp-splash-configure','original-engine-native-packages-configure'),
    @('root-googletest-mcpp-splash-assignment.json','root-native-packages-assignment.json'),
    @('source-freeze-root-googletest-mcpp-splash.json','source-freeze-root-native-packages.json'),
    @('BB8093B78C55B75DCEEFBA43479496CCDC46B9D41631A08554E653FC33E4748A',$assignmentSha),
    @('A1DA254EBC36446F21E61F41220AD90D26A7CF7A99EC8174AF36AAAB5038FD75',$freezeSha),
    @('$freeze.Count -ne 414','$freeze.Count -ne 444'),
    @('mcpp-audit/source-freeze-parent17-rev2.json','mcpp-audit/source-freeze-root-current17.json'),
    @('D7C8399550C2E48EDB6F6A160F6AC2EFE76C656BCCBACB6636CD35FBEEE596BF',$mcppSha),
    @('Count -ne 22','Count -ne 30'),
    @("'mcpp-2.7.2_az.2-je-rev1-windows.tar.xz')", "'mcpp-2.7.2_az.2-je-rev1-windows.tar.xz','expat-2.4.2-je-rev1-windows.tar.xz','tiff-4.2.0.15-je-rev1-windows.tar.xz','png-1.6.37-je-rev1-windows.tar.xz','d3dx12-headers-je-rev1-windows.tar.xz')"),
    @('accepted LZ4/OpenSSL root cache sidecar mismatch','accepted native root cache sidecar mismatch')
)
foreach ($change in $changes) {
    if (-not $driver.Contains($change[0])) { throw "Driver replacement missing: $($change[0])" }
    $driver = $driver.Replace($change[0], $change[1])
}
$driverDir = "$base/original-engine-native-packages-configure"
if (Test-Path -LiteralPath $driverDir) { throw 'New driver directory already exists' }
[void][IO.Directory]::CreateDirectory($driverDir)
$driverPath = "$driverDir/run-root-configure-native-packages.ps1"
[IO.File]::WriteAllText($driverPath, $driver, [Text.UTF8Encoding]::new($false))
$tokens = $null; $errors = $null
[void][Management.Automation.Language.Parser]::ParseFile($driverPath, [ref]$tokens, [ref]$errors)
if ($errors.Count) { throw ($errors.Message -join '; ') }
Write-NewJson "$base/root-native-packages-parent-preflight.json" ([ordered]@{sourceCount=444;argvCount=45;cacheCount=$cacheCount;oldRootDeltas=$oldDeltas;currentMcppDocumentationDelta=$mcppDeltas;assignmentSHA256=$assignmentSha;freezeSHA256=$freezeSha;mcppFreezeSHA256=$mcppSha;driverPath=$driverPath;driverSHA256=(Get-FileHash -LiteralPath $driverPath -Algorithm SHA256).Hash;driverParserErrors=0;nativeInvocation='NOT_RUN; assigned to Luna after parent diff review'})
[ordered]@{sourceCount=444;argvCount=45;cacheCount=$cacheCount;assignmentSHA256=$assignmentSha;freezeSHA256=$freezeSha;mcppFreezeSHA256=$mcppSha;driverSHA256=(Get-FileHash -LiteralPath $driverPath -Algorithm SHA256).Hash;nativeInvocation='NOT_RUN'} | ConvertTo-Json
