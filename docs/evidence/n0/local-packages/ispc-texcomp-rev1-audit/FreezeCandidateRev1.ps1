$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$engine='F:/JericoEngine/JericoEngine'
$owner='cmake/Tools/LocalPackages/ISPCTexComp'
$sourceRoot='F:/JericoEngine/Dependencies/ispc-texcomp-36b80aac50ea401a9adde44d86712c2241b2acfd'
$toolRoot='F:/JericoEngine/Dependencies/tools/ispc-v1.16.1-windows'
$vs='C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools'
$utf8=[Text.UTF8Encoding]::new($false)
function WriteNew([string]$name,[string]$text){$path=Join-Path $PSScriptRoot $name;$s=[IO.File]::Open($path,[IO.FileMode]::CreateNew);try{$b=$utf8.GetBytes($text);$s.Write($b,0,$b.Length)}finally{$s.Dispose()};return $path}
function JsonNew($name,$obj){WriteNew $name (($obj|ConvertTo-Json -Depth 16)+"`n")}
function Record([string]$path,[string]$role){$item=Get-Item -LiteralPath $path;[pscustomobject][ordered]@{path=$item.FullName.Replace('\','/');role=$role;bytes=$item.Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
$sources=@(Get-ChildItem -LiteralPath (Join-Path $engine $owner) -File -Recurse | Sort-Object FullName)
if($sources.Count -ne 8){throw 'Unexpected owned file count'}
$entries=@();foreach($source in $sources){$entries+=Record $source.FullName 'candidate source'}
foreach($pair in @(@{path=(Join-Path $PSScriptRoot 'source-immutable-manifest.json');root=$sourceRoot;role='immutable encoder source'},@{path=(Join-Path $PSScriptRoot 'tool-immutable-manifest.json');root=$toolRoot;role='immutable external compiler input'})){
 $manifest=Get-Content -LiteralPath $pair.path -Raw|ConvertFrom-Json
 foreach($file in $manifest.entries){$record=Record (Join-Path $pair.root $file.path) $pair.role;if($record.bytes -ne $file.bytes -or $record.sha256 -cne $file.sha256){throw 'Immutable input changed'};$entries+=$record}
}
$referencePaths=@(
 'cmake/Tools/LocalPackages/JECreateLocalPackage.cmake',
 'cmake/Tools/LocalPackages/Python/JERequireLocalPath.cmake',
 'LICENSE.txt','LICENSE_APACHE2.TXT','LICENSE_MIT.TXT',
 'cmake/3rdParty/Platform/Windows/BuiltInPackages_windows.cmake',
 'Gems/Atom/Asset/ImageProcessingAtom/Code/CMakeLists.txt',
 'Gems/Atom/Asset/ImageProcessingAtom/Code/Source/Compressors/ISPCTextureCompressor.cpp',
 'Gems/Atom/Asset/ImageProcessingAtom/Code/Source/Compressors/ISPCTextureCompressor.h')
foreach($path in $referencePaths){$entries+=Record (Join-Path $engine $path) 'engine reference'}
foreach($path in @('recipe-FindISPCTexComp.cmake','recipe-build_config.json','recipe-build_windows.bat','recipe-build_package_image.py','recipe-ISPCTexComp_36b80aa.patch','upstream-tree.json','upstream-commit.json','source-immutable-manifest.json','tool-immutable-manifest.json','source-eol-attributes-proof-v3.json','upstream-ISPC Texture Compressor__DXUT__.gitattributes','upstream-ISPC Texture Compressor__DirectXTex__.gitattributes','ispc-v1.16.1-release.json','ispc-v1.16.1-tag.json','ispc-v1.16.1-tag-object.json','source-tool-head-audit.json','source-acquisition.json','tool-acquisition.json','primary-DDSTextureLoader.cpp','primary-DDSTextureLoader-blob.json')){$entries+=Record (Join-Path $PSScriptRoot $path) 'audited provenance reference'}
$nativePins=@(
 @{path='F:/JericoEngine/Dependencies/archives/ispc-texcomp-36b80aac50ea401a9adde44d86712c2241b2acfd.tar.gz';pin='029373e6ec5b10d7e29eefdc5a231fb86c79b76a4a39c58c2727c6729de28ab7'},
 @{path='F:/JericoEngine/Dependencies/archives/ispc-v1.16.1-windows.zip';pin='b34de2c36aff2afaa56b669ea41f9e614a045564ca74fc0b138e17ccea4880b7'},
 @{path="$vs/VC/Auxiliary/Build/vcvarsall.bat";pin='ee67e4181ae2578b7842f9f1549a7f66e2166638fa544ce6838d9e9033a02275'},
 @{path="$vs/MSBuild/Current/Bin/amd64/MSBuild.exe";pin='58fbe0acac5265fe9a93acb291e1dbd50432c4ebf00ff9b9fb4c34bc0f247c73'},
 @{path="$vs/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe";pin='88c8344236a27a6e727e0a8edc49aaa2690bdc7a9464b9d18cc7abe70a9f1c0d'},
 @{path="$vs/MSBuild/Microsoft/VC/v170/Microsoft.Cl.Common.props";pin='9d9c3aa8770dfc7e7430b831252e1efbab0799c7ce0d25a03485ff506bed575b'})
$pinChecks=@();foreach($item in $nativePins){$record=Record $item.path 'audited archive or installed native tool input';if($item.pin.Length -ne 64 -or $record.sha256.ToLowerInvariant() -cne $item.pin){throw 'Source literal pin mismatch'};$pinChecks+=[pscustomobject]@{path=$record.path;literal=$item.pin;actual=$record.sha256;matches=$true};$entries+=$record}
$names=@($entries.path|Sort-Object -Unique);if($names.Count -ne $entries.Count){throw 'Duplicate freeze paths'}
$parserTokens=$null;$parserErrors=$null;[Management.Automation.Language.Parser]::ParseFile((Join-Path $engine "$owner/VerifyInputs.ps1"),[ref]$parserTokens,[ref]$parserErrors)|Out-Null;if($parserErrors.Count){throw 'Verifier syntax invalid'}
$patch='';$added=0;foreach($source in $sources){$relative=$source.FullName.Substring($engine.Length+1).Replace('\','/');$text=[IO.File]::ReadAllText($source.FullName);$lines=@($text.TrimEnd("`r","`n").Split("`n"));$added+=$lines.Count;$patch+="diff --git a/$relative b/$relative`nnew file mode 100644`n--- /dev/null`n+++ b/$relative`n@@ -0,0 +1,$($lines.Count) @@`n";foreach($line in $lines){$patch+='+'+$line.TrimEnd("`r")+"`n"}}
$patchPath=WriteNew 'source-diff-rev1.patch' $patch
$freeze=JsonNew 'source-freeze-ispc-texcomp-rev1.json' ([ordered]@{schemaVersion=1;status='SOURCE_CANDIDATE_NOT_RUN';candidateSources=$sources.Count;immutableEncoderFiles=238;immutableCompilerFiles=175;references=$entries.Count-421;count=$entries.Count;entries=$entries})
$output="$engine/build/n0-validation/local-packages/ispc-texcomp-rev1-build"
$consumer="$engine/build/n0-validation/local-packages/ispc-texcomp-rev1-consumer"
$producerArguments=@(
 '-DJE_ISPC_SOURCE_ARCHIVE=F:/JericoEngine/Dependencies/archives/ispc-texcomp-36b80aac50ea401a9adde44d86712c2241b2acfd.tar.gz',
 "-DJE_ISPC_SOURCE_ROOT=$sourceRoot", "-DJE_ISPC_SOURCE_MANIFEST=$PSScriptRoot/source-immutable-manifest.json",
 '-DJE_ISPC_TOOL_ARCHIVE=F:/JericoEngine/Dependencies/archives/ispc-v1.16.1-windows.zip',
 "-DJE_ISPC_TOOL_ROOT=$toolRoot", "-DJE_ISPC_TOOL_MANIFEST=$PSScriptRoot/tool-immutable-manifest.json",
 "-DJE_ISPC_VS_ROOT=$vs", "-DJE_ISPC_OUTPUT=$output", '-P', "$engine/$owner/BuildISPCTexCompPackage.cmake")
$consumerArguments=@('-S',"$engine/$owner/tests",'-B',$consumer,'-G','Visual Studio 17 2022','-A','x64',"-DJE_ISPC_PACKAGE_ROOT=$output/packages/ISPCTexComp-36b80aa-je-rev1-windows.contents")
$guards=@(
 @{name='uri';argument='JE_ISPC_TOOL_ROOT';value='https://example.invalid/ispc';expected='must not be a URI'},
 @{name='unc';argument='JE_ISPC_SOURCE_ROOT';value='//localhost/share/encoder';expected='must be an absolute local path'},
 @{name='missing';argument='JE_ISPC_SOURCE_ARCHIVE';value="$engine/build/n0-validation/local-packages/ispc-missing-source.tar.gz";expected='file is missing'},
 @{name='archive-hash';argument='JE_ISPC_SOURCE_ARCHIVE';value="$engine/build/n0-validation/local-packages/ispc-texcomp-rev1-guards/not-archive.txt";expected='SHA256 mismatch';fixture='new local text fixture, never mutate immutable archive'},
 @{name='manifest-hash';argument='JE_ISPC_TOOL_MANIFEST';value="$engine/build/n0-validation/local-packages/ispc-texcomp-rev1-guards/not-manifest.json";expected='SHA256 mismatch';fixture='new local JSON fixture, never mutate audited manifest'},
 @{name='external-output';argument='JE_ISPC_OUTPUT';value='F:/JericoEngine/Dependencies/ispc-output-forbidden';expected='output must be beneath'},
 @{name='old-output';argument='JE_ISPC_OUTPUT';value=$output;expected='output must be new';fixture='only after successful producer or create a distinct guard-owned directory'},
 @{name='wrong-source-root';argument='JE_ISPC_SOURCE_ROOT';value=$toolRoot;expected='input file count mismatch'})
$argsPath=JsonNew 'validation-arguments-rev1.json' ([ordered]@{status='NOT_RUN';producer=[ordered]@{arguments=$producerArguments;count=$producerArguments.Count;freshOutput=$output};consumer=[ordered]@{arguments=$consumerArguments;count=$consumerArguments.Count;buildRelease=@('--build',$consumer,'--config','Release');testRelease=@('--test-dir',$consumer,'-C','Release','--output-on-failure');buildDebug=@('--build',$consumer,'--config','Debug');testDebug=@('--test-dir',$consumer,'-C','Debug','--output-on-failure')};guards=$guards;guardRule='Replace one -D argument in the exact producer list; fresh output for each guard, except old-output. Fail before output/tool execution. Fixtures only under own new guards directory.'})
$receipt=JsonNew 'candidate-receipt-rev1.json' ([ordered]@{status='IMPLEMENTED_SOURCE_ONLY_NOT_RUN';scope=$owner;sourceFiles=@($entries|Where-Object role -eq 'candidate source');sourceLinesAdded=$added;sourceLinesDeleted=0;freeze=[ordered]@{path=$freeze;sha256=(Get-FileHash -LiteralPath $freeze).Hash;count=$entries.Count;array='entries'};diff=[ordered]@{path=$patchPath;sha256=(Get-FileHash -LiteralPath $patchPath).Hash};arguments=[ordered]@{path=$argsPath;sha256=(Get-FileHash -LiteralPath $argsPath).Hash};pinChecks=$pinChecks;staticChecks=[ordered]@{PSsyntax='ParseFile zero errors; not executed';archiveManifests='238 source plus 175 tool raw hashes/sizes unchanged';EOL='attributed comparison only; snapshots unmodified';nativeBuild='NOT_RUN';consumerCases='8 NOT_RUN';guardCases='8 NOT_RUN';target='Gem::ImageProcessingAtom.Editor.Static / 3rdParty::ISPCTexComp';originalExports=26};limitations=@('No original Atom/editor/AP/cook/runtime acceptance','No full image-quality decoder oracle for BC6H/BC7/ETC1/ASTC','No ISA forcing or cross-machine dispatch proof','External ISPC binary has measured official-release SHA256; upstream digest unavailable','New v143 source build does not prove historical package ABI','Release /MD SDK and Debug /MDd consumer require native evidence; no general mixed-CRT claim');previousReceipts='Acquisition/audit RED/v2/v3 kept unchanged; no source normalization or unrelated provider mutation'})
[pscustomobject]@{sources=$sources.Count;count=$entries.Count;linesAdded=$added;freeze=$freeze;freezeSHA256=(Get-FileHash -LiteralPath $freeze).Hash;diffSHA256=(Get-FileHash -LiteralPath $patchPath).Hash;receipt=$receipt;receiptSHA256=(Get-FileHash -LiteralPath $receipt).Hash;argumentsSHA256=(Get-FileHash -LiteralPath $argsPath).Hash}|ConvertTo-Json