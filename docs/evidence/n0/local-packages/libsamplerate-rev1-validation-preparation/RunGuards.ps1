# Execute only after explicit parent dispatch to Luna. No compiler is expected.
. (Join-Path $PSScriptRoot 'CommonExecution.ps1')
$authority='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev1-audit/validation-arguments-rev1.json'
if((GetHash $authority) -cne 'B623FE1D877D8F83A22E9D5D124C13E3DDEF875AD0A9BDC0D06FD821093BC585'){throw 'Guard authority changed'}
$plan=Get-Content -LiteralPath $authority -Raw|ConvertFrom-Json
if($plan.guards.Count -ne 8 -or $plan.producer.arguments.Count -ne 5){throw 'Guard/producer count differs'}
$fixtures='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev1-guards'
if(Test-Path -LiteralPath $fixtures){throw 'Guard fixtures must be fresh'}
BeginRun 'guards-run'
$failure=$null;$checks=@();$status='FAIL_OR_INCOMPLETE'
try{
    if($script:Before.matched -ne $script:InputFreeze.count){throw 'Preflight changed; no native phase dispatched'}
    [IO.Directory]::CreateDirectory($fixtures)|Out-Null
    foreach($name in @('wrong.tar.gz','wrong.json')){
        $stream=[IO.File]::Open((Join-Path $fixtures $name),[IO.FileMode]::CreateNew)
        try{$data=[Text.UTF8Encoding]::new($false).GetBytes('Guard-owned deliberately invalid input');$stream.Write($data,0,$data.Length)}finally{$stream.Dispose()}
    }
    foreach($guard in $plan.guards){
        $arguments=[string[]]@($plan.producer.arguments)
        $output="$fixtures/output-$($guard.name)"
        for($i=0;$i -lt $arguments.Count;$i++){
            if($arguments[$i].StartsWith('-DJE_SAMPLERATE_OUTPUT=',[StringComparison]::Ordinal)){$arguments[$i]='-DJE_SAMPLERATE_OUTPUT='+$output}
            if($arguments[$i].StartsWith('-D'+$guard.replaceVariable+'=',[StringComparison]::Ordinal)){$arguments[$i]='-D'+$guard.replaceVariable+'='+$guard.value}
        }
        if($guard.replaceVariable -ceq 'JE_SAMPLERATE_OUTPUT'){$output=$guard.value}
        $beforeExists=Test-Path -LiteralPath $output
        if($guard.name -ceq 'old-output'){if(-not $beforeExists){throw 'Old-output guard requires retained producer output'}}elseif($beforeExists){throw 'Guard output must be absent before its rejection'}
        $phase=Native ('guard-'+$guard.name) $script:CMake $arguments
        $diagnostic=[IO.File]::ReadAllText($phase.stderr)+[IO.File]::ReadAllText($phase.stdout)
        $afterExists=Test-Path -LiteralPath $output
        $ok=$phase.exitCode -ne 0 -and $diagnostic.Contains($guard.expectedDiagnostic) -and $afterExists -eq $beforeExists
        $checks+=[pscustomobject]@{name=$guard.name;expectedDiagnostic=$guard.expectedDiagnostic;exitCode=$phase.exitCode;diagnosticPresent=$diagnostic.Contains($guard.expectedDiagnostic);output=$output;beforeExists=$beforeExists;afterExists=$afterExists;expectedRejection=$ok;oldOutputScope='Existence of retained directory only; no full private old-build manifest claim'}
        if(-not $ok){throw "Guard $($guard.name) did not demonstrate expected pre-output rejection; no retry"}
    }
    $status='EIGHT_EXPECTED_GUARD_REJECTIONS_ONLY'
}catch{$failure=$_.Exception.ToString()}
$result=EndRun $status $failure $checks
if($failure){exit 1}
exit 0