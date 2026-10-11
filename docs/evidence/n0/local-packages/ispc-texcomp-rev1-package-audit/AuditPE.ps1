$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=Join-Path $PSScriptRoot 'extracted/ISPCTexComp-36b80aa-je-rev1-windows'
$dll=Join-Path $root 'ISPCTexComp/bin/ispc_texcomp.dll'
$lib=Join-Path $root 'ISPCTexComp/bin/ispc_texcomp.lib'
$data=[IO.File]::ReadAllBytes($dll)
function U16([int]$offset){[BitConverter]::ToUInt16($data,$offset)}
function U32([int]$offset){[BitConverter]::ToUInt32($data,$offset)}
$pe=U32 60
if($data[0] -ne 77 -or $data[1] -ne 90 -or (U32 $pe) -ne 17744 -or (U16 ($pe+4)) -ne 34404 -or -not ((U16 ($pe+22)) -band 8192)){throw 'Not original x64 PE DLL'}
$optional=$pe+24
if((U16 $optional) -ne 523){throw 'Not PE32+'}
$sectionCount=U16 ($pe+6)
$sectionOffset=$optional+(U16 ($pe+20))
$sections=@();for($i=0;$i -lt $sectionCount;$i++){$p=$sectionOffset+40*$i;$sections+=[pscustomobject]@{rva=(U32 ($p+12));raw=(U32 ($p+20));bytes=(U32 ($p+16))}}
function Offset([uint32]$rva){foreach($section in $sections){if($rva -ge $section.rva -and $rva -lt $section.rva+$section.bytes){return [int]($section.raw+$rva-$section.rva)}};throw 'RVA outside raw sections'}
function CString([int]$offset){$end=$offset;while($end -lt $data.Length -and $data[$end] -ne 0){$end++};if($end -eq $data.Length){throw 'Unterminated PE string'};[Text.Encoding]::ASCII.GetString($data,$offset,$end-$offset)}
$export=Offset (U32 ($optional+112))
$count=U32 ($export+24)
$names=Offset (U32 ($export+32))
$exports=@();for($i=0;$i -lt $count;$i++){$exports+=CString (Offset (U32 ($names+4*$i)))}
$def='F:/JericoEngine/Dependencies/ispc-texcomp-36b80aac50ea401a9adde44d86712c2241b2acfd/ispc_texcomp/ispc_texcomp.def'
$expected=@(Get-Content -LiteralPath $def|Select-Object -Skip 2|ForEach-Object {$_.Trim()}|Where-Object {$_})
if($exports.Count -ne 26 -or $expected.Count -ne 26 -or @(Compare-Object ($exports|Sort-Object) ($expected|Sort-Object)).Count){throw 'Original exports differ'}
$imports=@();$descriptor=Offset (U32 ($optional+120));while((U32 ($descriptor+12)) -ne 0){$imports+=CString (Offset (U32 ($descriptor+12)));$descriptor+=20}
if(@($imports|Where-Object {$_ -match 'ispc|python|gcc|mingw'}).Count){throw 'Unexpected tool/foreign runtime DLL imported'}
$archive=[IO.File]::ReadAllBytes($lib)
if([Text.Encoding]::ASCII.GetString($archive,0,8) -cne "!<arch>`n"){throw 'Not COFF import archive'}
$offset=8;$members=@();$importNames=@();while($offset -lt $archive.Length){if($offset+60 -gt $archive.Length -or $archive[$offset+58] -ne 96 -or $archive[$offset+59] -ne 10){throw 'Malformed COFF archive header'};$name=[Text.Encoding]::ASCII.GetString($archive,$offset,16).Trim();$size=[int]::Parse([Text.Encoding]::ASCII.GetString($archive,$offset+48,10).Trim());$payload=$offset+60;if($size -lt 0 -or $payload+$size -gt $archive.Length){throw 'Truncated COFF member'};$machine=$null;$kind='linker table';if($name -notin @('/','//')){if([BitConverter]::ToUInt16($archive,$payload) -eq 0 -and [BitConverter]::ToUInt16($archive,$payload+2) -eq 65535){$kind='import object';$machine=[BitConverter]::ToUInt16($archive,$payload+6);$strings=[Text.Encoding]::ASCII.GetString($archive,$payload+20,$size-20).Split([char]0);if($strings.Length -lt 2 -or $strings[1] -cne 'ispc_texcomp.dll'){throw 'Foreign import library target'};$importNames+=$strings[0]}else{$kind='COFF object';$machine=[BitConverter]::ToUInt16($archive,$payload)};if($machine -ne 34404){throw 'Import library has non-x64 object'}};$members+=[pscustomobject]@{name=$name;bytes=$size;kind=$kind;machine=$machine};$offset=$payload+$size+($size%2)}
if($offset -ne $archive.Length -or $importNames.Count -ne 26 -or @(Compare-Object ($importNames|Sort-Object) ($expected|Sort-Object)).Count){throw 'Import library exports differ'}
$log='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/ispc-texcomp-rev1-build/build.stdout.log'
$lines=[IO.File]::ReadAllLines($log)
$warnings=@($lines|Where-Object {$_ -match 'warning (MSB4011|G[0-9A-F]+):'}|ForEach-Object {$_.Trim()})
$compiler=@($lines|Where-Object {$_ -match 'cl.exe.* /MD '})
if($compiler.Count -ne 1){throw 'Expected original Release /MD compiler command not found'}
$receipt=[ordered]@{kind='readonly PE/COFF bytes and retained producer logs; no tool/DLL execution';dll=[ordered]@{path=$dll;bytes=$data.Length;sha256=(Get-FileHash -LiteralPath $dll).Hash;machine='AMD64';PE='PE32+';isDLL=$true;exports=$exports;exportsMatchOriginalDef=$true;imports=$imports};importLibrary=[ordered]@{path=$lib;bytes=$archive.Length;sha256=(Get-FileHash -LiteralPath $lib).Hash;members=$members;imports=$importNames;importsMatchOriginalDef=$true};CRT=[ordered]@{observedCompileFlags='/MD';compilerCommand=$compiler;meaning='This build only; no historical ABI/general mixed CRT acceptance'};warnings=[ordered]@{observed=$warnings;interpretation='MSB4011 duplicate empty props import and three upstream ISPC warning sites retained; performance warning text also exists. No source fix or behavior claim.'};nativeConsumer='NOT_RUN';sourceMutation=$false}
$target=Join-Path $PSScriptRoot 'pe-coff-notice-audit.json';$s=[IO.File]::Open($target,[IO.FileMode]::CreateNew);try{$b=[Text.UTF8Encoding]::new($false).GetBytes(($receipt|ConvertTo-Json -Depth 12)+"`n");$s.Write($b,0,$b.Length)}finally{$s.Dispose()}
[pscustomobject]@{exports=$exports.Count;imports=$imports;importObjectCount=$importNames.Count;receipt=$target;sha256=(Get-FileHash -LiteralPath $target).Hash}|ConvertTo-Json