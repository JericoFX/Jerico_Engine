#Requires -Version 7.0
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$taskRepo='F:/JericoEngine/JericoEngine'
$taskBase="$taskRepo/build/n0-validation/local-packages"
$taskOutput="$PSScriptRoot/parent-archive-review-rev1.json"
if(Test-Path -LiteralPath $taskOutput){throw 'Preserve existing parent review'}
function Get-TaskHash([string]$path){
    $stream=[IO.File]::OpenRead($path)
    try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}
}
function Assert-TaskFile([string]$path,[long]$bytes,[string]$hash){
    if(-not [IO.File]::Exists($path) -or [IO.FileInfo]::new($path).Length -ne $bytes -or (Get-TaskHash $path) -ne $hash){throw "Byte witness changed: $path"}
    if([IO.File]::GetAttributes($path) -band [IO.FileAttributes]::ReparsePoint){throw 'Reparse byte witness rejected'}
}
$taskAudit=Get-Content -LiteralPath "$PSScriptRoot/archive-audit-before-extraction.json" -Raw | ConvertFrom-Json
$taskExtract=Get-Content -LiteralPath "$PSScriptRoot/extracted-package-manifest.json" -Raw | ConvertFrom-Json
$taskPe=Get-Content -LiteralPath "$PSScriptRoot/pe-coff-notice-audit.json" -Raw | ConvertFrom-Json
$taskReader=Get-Content -LiteralPath "$PSScriptRoot/audit-reader-receipt.json" -Raw | ConvertFrom-Json
Assert-TaskFile $taskAudit.archive 491564 '8308677642330694AB1C63C74A7AAABF4A0A9633234F998B12E838CDA64008DF'
if($taskAudit.files -ne 21 -or $taskAudit.entries.Count -ne 21 -or $taskAudit.contentManifestCovered -ne 20 -or $taskAudit.links -ne 0 -or $taskAudit.caseCollisions -ne 0 -or -not $taskAudit.allRegular -or -not $taskAudit.producerPayloadByteExact -or $taskAudit.licenseAndSourceNoticesExact -ne 9 -or $taskAudit.toolExecutableRedistributed){throw 'Archive audit closure differs'}
if($taskExtract.files -ne 21 -or $taskExtract.entries.Count -ne 21){throw 'Extracted closure differs'}
$taskPayload="$taskBase/ispc-texcomp-rev1-build/packages/ISPCTexComp-36b80aa-je-rev1-windows.contents"
foreach($taskRoot in @($taskPayload,$taskExtract.root)){
    $taskActual=@(Get-ChildItem -LiteralPath $taskRoot -Recurse -File -Force)
    if($taskActual.Count -ne 21){throw 'Actual package membership differs'}
    foreach($entry in $taskAudit.entries){Assert-TaskFile (Join-Path $taskRoot $entry.path) $entry.bytes $entry.sha256}
}
$taskOriginal='F:/JericoEngine/Dependencies/ispc-texcomp-36b80aac50ea401a9adde44d86712c2241b2acfd'
$taskTool='F:/JericoEngine/Dependencies/tools/ispc-v1.16.1-windows'
$taskPairs=@(
    @('ISPCTexComp/license.txt',"$taskOriginal/license.txt"),
    @('ISPCTexComp/include/ISPC/ispc_texcomp.h',"$taskOriginal/ispc_texcomp/ispc_texcomp.h"),
    @('LICENSES/ISPC-build-tool/LICENSE.txt',"$taskTool/LICENSE.txt"),
    @('LICENSES/ISPC-build-tool/third-party-programs.txt',"$taskTool/third-party-programs.txt")
)
foreach($name in @('ispc_texcomp.cpp','ispc_texcomp.h','ispc_texcomp_astc.cpp','kernel.ispc','kernel_astc.ispc')){$taskPairs+=,@("LICENSES/ISPCTexComp/source/$name","$taskOriginal/ispc_texcomp/$name")}
foreach($pair in $taskPairs){
    $taskTarget=Join-Path $taskExtract.root $pair[0]
    Assert-TaskFile $pair[1] ([IO.FileInfo]::new($taskTarget).Length) (Get-TaskHash $taskTarget)
}
$taskExpected=@(Get-Content -LiteralPath "$taskOriginal/ispc_texcomp/ispc_texcomp.def" | Select-Object -Skip 2 | ForEach-Object {$_.Trim()} | Where-Object {$_})
foreach($names in @($taskPe.dll.exports,$taskPe.importLibrary.imports)){
    if($names.Count -ne 26 -or @(Compare-Object ($names | Sort-Object) ($taskExpected | Sort-Object)).Count){throw 'PE/COFF export witnesses differ'}
}
if($taskPe.dll.machine -ne 'AMD64' -or $taskPe.dll.PE -ne 'PE32+' -or -not $taskPe.dll.isDLL -or $taskPe.CRT.observedCompileFlags -ne '/MD' -or $taskPe.nativeConsumer -ne 'NOT_RUN'){throw 'Static PE scope differs'}
foreach($member in $taskPe.importLibrary.members){if($member.kind -ne 'linker table' -and $member.machine -ne 34404){throw 'COFF machine witness differs'}}
if(@($taskPe.dll.imports | Where-Object {$_ -match 'python|ispc|gcc|mingw'}).Count){throw 'Unexpected imported product/tool runtime'}
Assert-TaskFile $taskReader.reader ([IO.FileInfo]::new($taskReader.reader).Length) '6235ABE2248C14AD86DFE00DBB7BC2396EB4B91F663412C211905E279BCE9D6E'
if($taskReader.exitCode -ne 0 -or $taskReader.arguments.Count -ne 3 -or $taskReader.arguments[0] -ne '-I' -or $taskReader.arguments[1] -ne '-B'){throw 'External data reader receipt differs'}
$taskReceipt=[ordered]@{
    status='PARENT ARCHIVE DATA REVIEW PASS; NATIVE CONSUMER PENDING'
    archiveBytes=491564;archiveSHA256=$taskAudit.sha256;regularFiles=21;contentManifest=20
    actualPayloadAndExtractedFilesRehashed=42;originalNoticeApiSourcePairs=9;exports=26
    archiveAuditSHA256=(Get-TaskHash "$PSScriptRoot/archive-audit-before-extraction.json")
    extractedManifestSHA256=(Get-TaskHash "$PSScriptRoot/extracted-package-manifest.json")
    peCoffAuditSHA256=(Get-TaskHash "$PSScriptRoot/pe-coff-notice-audit.json")
    readerReceiptSHA256=(Get-TaskHash "$PSScriptRoot/audit-reader-receipt.json")
    readerScriptSHA256=(Get-TaskHash "$PSScriptRoot/AuditPackage.py")
    peScriptSHA256=(Get-TaskHash "$PSScriptRoot/AuditPE.ps1")
    limitation='Static PE/COFF and byte review only; no loaded DLL, codec behavior, engine acceptance or runtime footprint claim. Upstream ISPC performance warnings preserved.'
    engineAccepted=$false;catalogIntegrated=$false
}
$taskStream=[IO.File]::Open($taskOutput,[IO.FileMode]::CreateNew)
try{$taskBytes=[Text.UTF8Encoding]::new($false).GetBytes(($taskReceipt | ConvertTo-Json -Depth 8)+"`n");$taskStream.Write($taskBytes)}finally{$taskStream.Dispose()}
[ordered]@{status=$taskReceipt.status;sha256=(Get-TaskHash $taskOutput);files=21} | ConvertTo-Json
