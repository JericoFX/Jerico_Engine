#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
$checkout = 'F:/JericoEngine/JericoEngine'
$root = "$checkout/build/n0-validation/local-packages/vvl-rev1-negatives"
if (Test-Path -LiteralPath $root) { throw 'Negative fixtures already exist; preserve them' }
[System.IO.Directory]::CreateDirectory("$root/fixtures/empty-archives") | Out-Null
[System.IO.Directory]::CreateDirectory("$root/fixtures/changed-archives/vvl") | Out-Null
[System.IO.Directory]::CreateDirectory("$root/fixtures/wrong-patch/utils") | Out-Null
[System.IO.Directory]::CreateDirectory("$root/fixtures/existing-output") | Out-Null
$encoding = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText("$root/fixtures/existing-output/retain.txt", 'Existing output must stay byte-identical.', $encoding)
$archive = "$PSScriptRoot/acquisition/vvl/vvl-a228e25346a931c791714977becb48b6702ec118.tar.gz"
$changed = "$root/fixtures/changed-archives/vvl/vvl-a228e25346a931c791714977becb48b6702ec118.tar.gz"
[System.IO.File]::Copy($archive, $changed, $false)
$stream = [System.IO.File]::Open($changed, [System.IO.FileMode]::Append)
try { $stream.WriteByte(0) } finally { $stream.Dispose() }
$originalPatch = "$PSScriptRoot/acquisition/spirv-tools/snapshot/utils/update_build_version.py"
$wrongPatch = [System.IO.File]::ReadAllText($originalPatch) + "`n# Deliberately changed negative fixture; never part of a package.`n"
[System.IO.File]::WriteAllText("$root/fixtures/wrong-patch/utils/update_build_version.py", $wrongPatch, $encoding)
$patchScript = @'
cmake_minimum_required(VERSION 3.24)
include("F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/Python/JERequireLocalPath.cmake")
include("F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/VulkanValidationLayers/ApplySpirvToolsProvenance.cmake")
JE_vvl_apply_spirv_provenance("F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-negatives/fixtures/wrong-patch")
'@
[System.IO.File]::WriteAllText("$root/fixtures/wrong-patch.cmake", $patchScript + "`n", $encoding)
[System.IO.Directory]::CreateDirectory("$root/fixtures/wrong-version") | Out-Null
$versionScript = @'
cmake_minimum_required(VERSION 3.24)
project(JEVvlWrongVersion LANGUAGES NONE)
list(PREPEND CMAKE_MODULE_PATH "F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-consumer/package")
find_package(vulkan-validationlayers 1.3.262 EXACT REQUIRED MODULE)
'@
[System.IO.File]::WriteAllText("$root/fixtures/wrong-version/CMakeLists.txt", $versionScript + "`n", $encoding)
# Windows junctions need no SDK installation. The ancestor guard must reject it.
New-Item -ItemType Junction -Path "$root/fixtures/archive-junction" -Value "$PSScriptRoot/acquisition" | Out-Null
$common = @(
    "-DJE_VVL_ARCHIVES_ROOT=$PSScriptRoot/acquisition",
    "-DJE_VVL_PYTHON=$checkout/build/n0-validation/original-engine-deps/Python-rev3/packages/python-3.10.13-je-rev3-windows/python/python.exe",
    '-DJE_VVL_PYTHON_SHA256=6235abe2248c14ad86dfe00dbb7bc2396eb4b91f663412c211905e279bce9d6e'
)
$cases = @(
    @{label='missing-archive'; extra=@("-DJE_VVL_ARCHIVES_ROOT=$root/fixtures/empty-archives"); pattern='vvl exact source archive file is missing'},
    @{label='changed-archive'; extra=@("-DJE_VVL_ARCHIVES_ROOT=$root/fixtures/changed-archives"); pattern='vvl exact source archive SHA256 mismatch'},
    @{label='missing-tool'; extra=@("-DJE_VVL_PYTHON=$root/fixtures/missing-python.exe"); pattern='audited external Python3 generation tool file is missing'},
    @{label='changed-tool-pin'; extra=@('-DJE_VVL_PYTHON_SHA256=0000000000000000000000000000000000000000000000000000000000000000'); pattern='audited external Python3 generation tool SHA256 mismatch'},
    @{label='remote-uri'; extra=@('-DJE_VVL_ARCHIVES_ROOT=https://invalid.example/vvl'); pattern='must not be a URI'},
    @{label='unc'; extra=@('-DJE_VVL_ARCHIVES_ROOT=//invalid.example/never-contacted/vvl'); pattern='must be an absolute local path'},
    @{label='junction'; extra=@("-DJE_VVL_ARCHIVES_ROOT=$root/fixtures/archive-junction"); pattern='must not use symlink paths'},
    @{label='existing-output'; extra=@("-DJE_VVL_OUTPUT=$root/fixtures/existing-output"); pattern='output exists; retained attempts are never overwritten'},
    @{label='outside-build'; extra=@("-DJE_VVL_OUTPUT=$checkout/je-vvl-negative-never-created"); pattern='output must be beneath this checkout/build'}
)
$commands = foreach ($case in $cases) {
    $out = "$root/not-created-$($case.label)"
    if ($case.label -eq 'outside-build') { $out="$checkout/je-vvl-negative-never-created" }
    [ordered]@{label=$case.label; executable='C:/Program Files/CMake/bin/cmake.exe'; argv=@($common)+@("-DJE_VVL_OUTPUT=$out")+@($case.extra)+@('-P',"$checkout/cmake/Tools/LocalPackages/VulkanValidationLayers/BuildVulkanValidationLayersPackage.cmake"); expectedError=$case.pattern; outputMustRemainAbsent=if($case.label -eq 'existing-output'){$null}else{$out}}
}
$commands += [ordered]@{label='wrong-private-patch'; executable='C:/Program Files/CMake/bin/cmake.exe'; argv=@('-P',"$root/fixtures/wrong-patch.cmake"); expectedError='pristine SPIRV version generator SHA256 mismatch'; outputMustRemainAbsent=$null}
$commands += [ordered]@{label='wrong-package-version'; executable='C:/Program Files/CMake/bin/cmake.exe'; argv=@('-S',"$root/fixtures/wrong-version",'-B',"$root/wrong-version-build"); expectedError='Found unsuitable version'; outputMustRemainAbsent=$null}
$frozenFixtures = foreach($path in @("$root/fixtures/existing-output/retain.txt","$root/fixtures/wrong-patch/utils/update_build_version.py","$root/fixtures/wrong-patch.cmake","$root/fixtures/wrong-version/CMakeLists.txt",$changed)) {
    $item=Get-Item -LiteralPath $path
    [ordered]@{path=$item.FullName; bytes=$item.Length; sha256=(Get-FileHash -LiteralPath $path).Hash}
}
$assignment = [ordered]@{status='PARENT PREPARED; NOT RUN'; commands=@($commands); fixtures=@($frozenFixtures); nativeOwner='Luna'; precondition='Producer and extracted-package consumer completed with exit0 and frozen inputs unchanged'; limit='Negative configure failures prove rejection only; no engine/Editor acceptance'}
[System.IO.File]::WriteAllText("$root/negative-assignment-rev1.json", ($assignment | ConvertTo-Json -Depth 10) + "`n", $encoding)
[pscustomobject]@{status=$assignment.status; cases=$commands.Count; fixtureFiles=$frozenFixtures.Count; nativeStatus='NOT RUN'} | ConvertTo-Json
