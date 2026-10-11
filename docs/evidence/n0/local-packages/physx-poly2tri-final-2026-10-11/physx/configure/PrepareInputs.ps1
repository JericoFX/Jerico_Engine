# Data-only preparation. Does not execute CMake, SDK scripts, compilers or tools.
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$engine='F:/JericoEngine/JericoEngine'
$audit="$engine/build/n0-validation/local-packages/physx-source-audit-rev1"
$manifestPath="$engine/build/n0-validation/local-packages/physx-rev1-acquisition/source-snapshot-manifest.json"
$archive='F:/JericoEngine/Dependencies/archives/PhysX-0bbcff3d0c541325f4d14c36ee18f24e22e35e6e.tar.gz'
$private="$PSScriptRoot/private-source"
$helpers="$PSScriptRoot/private-helpers"
function Hash([string]$path){$s=[IO.File]::OpenRead($path);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($s))}finally{$s.Dispose()}}
function NewFile([string]$path,[byte[]]$bytes){[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path))|Out-Null;$s=[IO.File]::Open($path,[IO.FileMode]::CreateNew);try{$s.Write($bytes,0,$bytes.Length)}finally{$s.Dispose()}}
function Receipt([string]$name,$value){NewFile (Join-Path $PSScriptRoot $name) ([Text.UTF8Encoding]::new($false).GetBytes(($value|ConvertTo-Json -Depth 12)+"`n"))}
function Local([string]$path){$full=[IO.Path]::GetFullPath($path);if($full.StartsWith('\\') -or $path.Contains('://')){throw 'Only absolute local paths'};$cur=$full;while($cur){if(Test-Path -LiteralPath $cur){if((Get-Item -LiteralPath $cur -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Reparse path: $cur"}};$parent=[IO.Path]::GetDirectoryName($cur);if($parent -eq $cur){break};$cur=$parent};return $full}
if((Hash $manifestPath) -cne '202329A844DF20F8E3349C589A3D9567F7D633615B2B17E37FD49929F03AC2AD'){throw 'Pinned SDK manifest changed'}
if((Hash $archive) -cne 'A2DFA049B0C7FA67723D27D70520D0ED7BCB5DD3C2D9DF8DFB169D8A39C4C142'){throw 'Pinned SDK archive changed'}
$manifest=Get-Content -LiteralPath $manifestPath -Raw|ConvertFrom-Json
if($manifest.entries.Count -ne 1788 -or $manifest.linksMaterialized -ne 0){throw 'SDK manifest coverage differs'}
$expected=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($entry in $manifest.entries){
    if($entry.path -notmatch '^(physx/|LICENSE\.md$|README\.md$)' -or $entry.path -match '(^|/)\.\.(/|$)|[:\\]' -or -not $expected.Add($entry.path)){throw 'Invalid SDK membership'}
    $path=Local (Join-Path $manifest.root $entry.path)
    if((Hash $path) -cne $entry.sha256.ToUpperInvariant() -or ([IO.FileInfo]::new($path)).Length -ne $entry.bytes){throw "SDK source mismatch: $($entry.path)"}
}
$actual=@(Get-ChildItem -LiteralPath $manifest.root -File -Recurse -Force)
if($actual.Count -ne 1788){throw 'SDK snapshot membership differs'}
foreach($file in $actual){if(-not $expected.Contains([IO.Path]::GetRelativePath($manifest.root,$file.FullName).Replace('\','/'))){throw 'Unknown SDK member'}}
foreach($dir in @($private,$helpers)){Local $dir|Out-Null;if(Test-Path -LiteralPath $dir){throw 'Private input directory must be fresh'}}
# Verify official helper documents against their primary Git tree before copying.
$tree=Get-Content -LiteralPath "$audit/official-build-helper-tree.json" -Raw|ConvertFrom-Json
$helperMap=@{'NvidiaBuildOptions.cmake'='externals/cmakemodules/NvidiaBuildOptions.cmake';'SetOutputPaths.cmake'='externals/cmakemodules/SetOutputPaths.cmake';'GetCompilerAndPlatform.cmake'='externals/cmakemodules/GetCompilerAndPlatform.cmake';'LICENSE.md'='LICENSE.md';'README.md'='README.md';'externals-README.md'='externals/README.md'}
$helperRecords=@()
foreach($name in $helperMap.Keys){
    $original=switch($name){'LICENSE.md'{"$audit/official-helper-repo-LICENSE.md"};'README.md'{"$audit/official-helper-repo-README.md"};'externals-README.md'{"$audit/official-helper-externals-README.md"};default{"$audit/official-build-helper-$name"}}
    Local $original|Out-Null;$bytes=[IO.File]::ReadAllBytes($original);$prefix=[Text.Encoding]::UTF8.GetBytes("blob $($bytes.Length)`0");$blobBytes=[byte[]]::new($prefix.Length+$bytes.Length);[Array]::Copy($prefix,$blobBytes,$prefix.Length);[Array]::Copy($bytes,0,$blobBytes,$prefix.Length,$bytes.Length)
    $blob=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData($blobBytes)).ToLowerInvariant();$node=@($tree.tree|Where-Object path -CEQ $helperMap[$name])
    if($node.Count -ne 1 -or $node[0].sha -cne $blob){throw "Official helper blob mismatch: $name"}
    $helperRecords+= [pscustomobject]@{original=$original;path="$helpers/$name";sha256=(Hash $original);bytes=$bytes.Length;gitBlob=$blob}
}
foreach($entry in $manifest.entries){$from=Join-Path $manifest.root $entry.path;$to=Join-Path $private $entry.path;NewFile $to ([IO.File]::ReadAllBytes($from));if((Hash $to) -cne $entry.sha256.ToUpperInvariant()){throw 'Private copy mismatch'}}
foreach($entry in $helperRecords){NewFile $entry.path ([IO.File]::ReadAllBytes($entry.original))}
$patches=@()
function Patch([string]$path,[string]$before,[string]$after){
    $bytes=[IO.File]::ReadAllBytes($path);$text=[Text.Encoding]::UTF8.GetString($bytes)
    if(($text.Split([string[]]@($before),[StringSplitOptions]::None).Count-1) -ne 1){throw "Expected unique hunk: $path"}
    $patched=$text.Replace($before,$after)
    if($patched.Replace($after,$before) -cne $text){throw 'Patch inverse failed'}
    $preHash=Hash $path
    [IO.File]::WriteAllBytes($path,[Text.UTF8Encoding]::new($false).GetBytes($patched))
    $script:patches+= [ordered]@{path=$path;beforeSHA256=$preHash;afterSHA256=(Hash $path);before=$before;after=$after;inverseByteExact=$true}
}
$windows="$private/physx/source/compiler/cmake/windows/CMakeLists.txt"
Patch $windows 'IF(PX_COPY_EXTERNAL_DLL OR PUBLIC_RELEASE)' 'IF(PX_COPY_EXTERNAL_DLL)'
Patch $windows "# for public release we copy the dlls for GPU other platforms so we dont keep duplicates around the repository`nIF(PUBLIC_RELEASE)" "# for public release we copy the dlls for GPU other platforms so we dont keep duplicates around the repository`nIF(PX_COPY_EXTERNAL_DLL AND PUBLIC_RELEASE)"
Patch "$helpers/GetCompilerAndPlatform.cmake" "`tELSEIF (MSVC_VERSION GREATER_EQUAL 1920)" "`tELSEIF (MSVC_VERSION GREATER_EQUAL 1930)`n`t`tSET(COMPILER_SUFFIX `"vc143`")`n`tELSEIF (MSVC_VERSION GREATER_EQUAL 1920)"
Receipt 'private-patch-proof.json' ([ordered]@{status='DATA_PREPARED; native NOT_RUN';sourceCommit=$manifest.sourceCommit;SDKRegularFiles=1788;helperSourceCommit='a2c0428acab643e60618c681b501e86f7fd558cc';helpers=$helperRecords;patches=$patches;CPUOnly=$true;PUBLIC_RELEASE=1;PX_PUBLIC_RELEASE=1;GPUPhysics='not provisioned; not equivalent to O3DE rev4';Jolt='final requirement remains';generatedFileExpected="$private/physx/include/PxConfig.h"})
