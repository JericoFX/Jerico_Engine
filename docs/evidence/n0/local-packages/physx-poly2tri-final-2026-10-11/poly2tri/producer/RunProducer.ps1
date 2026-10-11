#Requires -Version 7.0
#Requires -PSEdition Core
. "$PSScriptRoot/CommonExecution.ps1"
BeginRun 'attempt1'
$failure=$null;$status='NATIVE_NOT_RUN';$checks=@()
try {
    if($script:Before.matched -ne $script:InputFreeze.count){throw 'poly2tri producer preflight mismatch'}
    $assignment=Get-Content -LiteralPath "$PSScriptRoot/assignment.json" -Raw|ConvertFrom-Json
    if($assignment.producer.argc -ne 6 -or $assignment.producer.arguments.Count -ne 6){throw 'poly2tri producer requires the assigned six arguments'}
    if(Test-Path -LiteralPath $assignment.outputRoot){throw 'poly2tri output must be fresh'}
    $native=Native 'producer' $script:CMake ([string[]]$assignment.producer.arguments)
    if($native.exitCode -ne 0){throw "poly2tri producer failed: $($native.exitCode)"}
    if(-not [IO.File]::Exists($assignment.archivePath) -or ([IO.FileInfo]::new($assignment.archivePath)).Length -eq 0){throw 'Producer returned without a nonempty archive'}
    $sidecar=$assignment.archivePath+'.SHA256SUMS'
    if(-not [IO.File]::Exists($sidecar)){throw 'Package SHA256SUMS sidecar missing'}
    $hash=GetHash $assignment.archivePath
    $expected=$hash.ToLowerInvariant()+' *'+[IO.Path]::GetFileName($assignment.archivePath)
    if([IO.File]::ReadAllText($sidecar).Trim() -cne $expected){throw 'Archive sidecar does not match measured hash'}
    $checks=@([ordered]@{archive=$assignment.archivePath;bytes=([IO.FileInfo]::new($assignment.archivePath)).Length;sha256=$hash;sidecar=$sidecar;scope='Producer outcome only; tar/COFF/CRT/consumer/engine validation pending'})
    $status='PRODUCER_EXIT_ZERO; PARENT_ACCEPTANCE_PENDING'
}catch{$failure=$_.Exception.Message;$status='PRODUCER_DRIVER_FAILED'}
finally{$result=EndRun $status $failure $checks;$result|ConvertTo-Json -Depth 12}
if($failure){exit 1}
exit 0
