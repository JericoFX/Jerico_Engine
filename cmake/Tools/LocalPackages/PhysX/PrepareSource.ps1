# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Data-only preparation: no SDK/CMake/compiler/Git/packman process is launched.
param(
    [Parameter(Mandatory)][string]$SourceArchive,
    [Parameter(Mandatory)][string]$SourceRoot,
    [Parameter(Mandatory)][string]$SourceManifest,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$VSInstance
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
function JEHash([string]$path){$s=[IO.File]::OpenRead($path);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($s))}finally{$s.Dispose()}}
function JELocal([string]$path){
    if(-not [IO.Path]::IsPathFullyQualified($path) -or $path.StartsWith('\\') -or $path.StartsWith('//') -or $path.Contains('://')){throw 'JE PhysX: absolute local paths required'}
    $full=[IO.Path]::GetFullPath($path);$cur=$full
    while($cur){if(Test-Path -LiteralPath $cur){if((Get-Item -LiteralPath $cur -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw "JE PhysX: reparse path rejected $cur"}};$next=[IO.Path]::GetDirectoryName($cur);if($next -eq $cur){break};$cur=$next}
    return $full.Replace('\','/')
}
function JENewFile([string]$path,[byte[]]$bytes){[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path))|Out-Null;$s=[IO.File]::Open($path,[IO.FileMode]::CreateNew);try{$s.Write($bytes,0,$bytes.Length)}finally{$s.Dispose()}}
foreach($name in @('SourceArchive','SourceRoot','SourceManifest','OutputDirectory','VSInstance')){Set-Variable -Name $name -Value (JELocal (Get-Variable -Name $name -ValueOnly))}
$recipe=Get-Content -LiteralPath "$PSScriptRoot/Recipe.json" -Raw|ConvertFrom-Json
$buildRoot=[IO.Path]::GetFullPath("$PSScriptRoot/../../../../build").Replace('\','/')
if(-not $OutputDirectory.StartsWith($buildRoot+'/',[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $OutputDirectory)){throw 'JE PhysX: fresh output beneath checkout/build required'}
if(-not (Test-Path -LiteralPath "$VSInstance/VC/Tools/MSVC/14.44.35207")){throw 'JE PhysX: audited VS2022/v143 installation is missing'}
if((JEHash $SourceArchive) -cne $recipe.sourceArchiveSHA256.ToUpperInvariant() -or (JEHash $SourceManifest) -cne $recipe.sourceManifestSHA256.ToUpperInvariant()){throw 'JE PhysX: pinned archive/manifest changed'}
$manifest=Get-Content -LiteralPath $SourceManifest -Raw|ConvertFrom-Json
if($manifest.entries.Count -ne 1788 -or $manifest.sourceCommit -cne $recipe.sourceCommit -or $manifest.linksMaterialized -ne 0){throw 'JE PhysX: incomplete source manifest'}
$seen=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($entry in $manifest.entries){
    if($entry.path -notmatch '^(physx/|LICENSE\.md$|README\.md$)' -or $entry.path -match '(^|/)\.\.(/|$)|[:\\]' -or -not $seen.Add($entry.path)){throw 'JE PhysX: invalid source membership'}
    $path=JELocal (Join-Path $SourceRoot $entry.path)
    if((JEHash $path) -cne $entry.sha256.ToUpperInvariant() -or ([IO.FileInfo]::new($path)).Length -ne $entry.bytes){throw "JE PhysX: source differs $($entry.path)"}
}
$all=@(Get-ChildItem -LiteralPath $SourceRoot -Recurse -Force)
foreach($item in $all){if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'JE PhysX: source reparse member rejected'}}
$files=@($all|Where-Object {-not $_.PSIsContainer})
if($files.Count -ne 1788){throw 'JE PhysX: source membership count differs'}
foreach($file in $files){if(-not $seen.Contains([IO.Path]::GetRelativePath($SourceRoot,$file.FullName).Replace('\','/'))){throw 'JE PhysX: unexpected source member'}}
foreach($helper in $recipe.helperOriginals){$path=JELocal "$PSScriptRoot/official-build-helpers/$($helper.path)";if((JEHash $path) -cne $helper.sha256){throw 'JE PhysX: official helper/notice changed'}}
$private="$OutputDirectory/private-source";$helpers="$OutputDirectory/private-helpers"
foreach($entry in $manifest.entries){JENewFile "$private/$($entry.path)" ([IO.File]::ReadAllBytes((Join-Path $SourceRoot $entry.path)))}
foreach($helper in $recipe.helperOriginals){JENewFile "$helpers/$($helper.path)" ([IO.File]::ReadAllBytes("$PSScriptRoot/official-build-helpers/$($helper.path)"))}
$patches=[Collections.Generic.List[object]]::new()
function JEPatch([string]$path,[string]$before,[string]$after){
    $text=[IO.File]::ReadAllText($path)
    if(($text.Split([string[]]@($before),[StringSplitOptions]::None).Count-1) -ne 1){throw 'JE PhysX: exact unique patch hunk missing'}
    $patched=$text.Replace($before,$after)
    if($patched.Replace($after,$before) -cne $text){throw 'JE PhysX: patch inverse differs'}
    $beforeHash=JEHash $path;[IO.File]::WriteAllBytes($path,[Text.UTF8Encoding]::new($false).GetBytes($patched))
    $patches.Add([ordered]@{path=$path;beforeSHA256=$beforeHash;afterSHA256=(JEHash $path);before=$before;after=$after;inverseByteExact=$true})
}
$windows="$private/physx/source/compiler/cmake/windows/CMakeLists.txt"
JEPatch $windows 'IF(PX_COPY_EXTERNAL_DLL OR PUBLIC_RELEASE)' 'IF(PX_COPY_EXTERNAL_DLL)'
JEPatch $windows "# for public release we copy the dlls for GPU other platforms so we dont keep duplicates around the repository`nIF(PUBLIC_RELEASE)" "# for public release we copy the dlls for GPU other platforms so we dont keep duplicates around the repository`nIF(PX_COPY_EXTERNAL_DLL AND PUBLIC_RELEASE)"
JEPatch "$helpers/GetCompilerAndPlatform.cmake" "`tELSEIF (MSVC_VERSION GREATER_EQUAL 1920)" "`tELSEIF (MSVC_VERSION GREATER_EQUAL 1930)`n`t`tSET(COMPILER_SUFFIX `"vc143`")`n`tELSEIF (MSVC_VERSION GREATER_EQUAL 1920)"
if((JEHash $windows) -cne '5DBE8B8DACC1178C992BAA1ED32731378115A671B3D34F989704063B3021E581' -or (JEHash "$helpers/GetCompilerAndPlatform.cmake") -cne 'D3F4995DC5633D96959E390A1AF5A7AB5561B5E6060232070EE04AA1F560623C'){throw 'JE PhysX: adapted output bytes differ from reviewed configure inputs'}
$arguments=@('-S',"$private/physx/compiler/public",'-B',"$OutputDirectory/build",'-G','Visual Studio 17 2022','-A','x64','-T','v143',"-DCMAKE_GENERATOR_INSTANCE:PATH=$VSInstance","-DPHYSX_ROOT_DIR:PATH=$private/physx","-DCMAKEMODULES_PATH:PATH=$helpers",'-DCMAKEMODULES_VERSION:STRING=JE-official-4.1-a2c0428','-DCMAKEMODULES_NAME:STRING=JE-OfficialPhysXBuildHelpers','-DTARGET_BUILD_PLATFORM:STRING=windows','-DPX_OUTPUT_ARCH:STRING=x86',"-DPX_OUTPUT_LIB_DIR:PATH=$OutputDirectory/binaries","-DPX_OUTPUT_BIN_DIR:PATH=$OutputDirectory/binaries",'-DPX_GENERATE_STATIC_LIBRARIES:BOOL=ON','-DNV_USE_STATIC_WINCRT:BOOL=OFF','-DNV_USE_DEBUG_WINCRT:BOOL=ON','-DPX_BUILDSNIPPETS:BOOL=OFF','-DPX_BUILDPVDRUNTIME:BOOL=OFF','-DPX_COPY_EXTERNAL_DLL:BOOL=OFF','-DFETCHCONTENT_FULLY_DISCONNECTED:BOOL=ON','-DCMAKE_FIND_USE_PACKAGE_REGISTRY:BOOL=OFF','-DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY:BOOL=OFF')
$result=[ordered]@{status='DATA_PREPARED; NATIVE_NOT_RUN';configureArguments=$arguments;argc=$arguments.Count;patches=$patches.ToArray();targets=@('PhysX','PhysXPvdSDK','PhysXVehicle','PhysXCharacterKinematic','PhysXExtensions','PhysXCooking','PhysXCommon','PhysXFoundation','LowLevel','LowLevelAABB','LowLevelDynamics','PhysXTask','SceneQuery','SimulationController');configurations=@('debug','checked','profile','release');scope='CPU configure/build must be separately assigned; no native executed here'}
JENewFile "$OutputDirectory/preparation.json" ([Text.UTF8Encoding]::new($false).GetBytes(($result|ConvertTo-Json -Depth 10)+"`n"))
