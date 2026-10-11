# Execute only after explicit parent dispatch to Luna. No compiler is expected.
. (Join-Path $PSScriptRoot 'CommonExecution.ps1')
$authority='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/ispc-texcomp-rev1-audit/validation-arguments-rev2.json'
if((GetHash $authority) -cne '35B6A7D77269F60E2A067B401980E01C3F4576E2F755D0DE09CFB3B1851127F4'){throw 'Guard authority changed'}
$plan=Get-Content -LiteralPath $authority -Raw|ConvertFrom-Json
if($plan.guards.Count -ne 9 -or $plan.producer.arguments.Count -ne 11){throw 'Guard/producer count differs'}
$fixtures='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/ispc-texcomp-rev1-guards'
if(Test-Path -LiteralPath $fixtures){throw 'Guard fixtures must be fresh'}
BeginRun 'guards-run'
$failure=$null;$checks=@();$status='FAIL_OR_INCOMPLETE'
try{
    [IO.Directory]::CreateDirectory($fixtures)|Out-Null
    [IO.File]::WriteAllText((Join-Path $fixtures 'not-archive.txt'),'Guard-owned deliberately invalid data',[Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $fixtures 'not-manifest.json'),'{}',[Text.UTF8Encoding]::new($false))
    foreach($guard in $plan.guards){
        $arguments=[string[]]@($plan.producer.arguments)
        $output="$fixtures/output-$($guard.name)"
        for($i=0;$i -lt $arguments.Count;$i++){
            if($arguments[$i].StartsWith('-DJE_ISPC_OUTPUT=',[StringComparison]::Ordinal)){$arguments[$i]='-DJE_ISPC_OUTPUT='+$output}
            if($arguments[$i].StartsWith('-D'+$guard.argument+'=',[StringComparison]::Ordinal)){$arguments[$i]='-D'+$guard.argument+'='+$guard.value}
        }
        if($guard.argument -ceq 'JE_ISPC_OUTPUT'){$output=$guard.value}
        $beforeExists=Test-Path -LiteralPath $output
        if($guard.name -ceq 'old-output'){if(-not $beforeExists){throw 'Old-output guard requires retained producer output'}}elseif($beforeExists){throw 'Guard output must be absent before its rejection'}
        $phase=Native ('guard-'+$guard.name) $script:CMake $arguments
        $diagnostic=[IO.File]::ReadAllText($phase.stderr)+[IO.File]::ReadAllText($phase.stdout)
        $afterExists=Test-Path -LiteralPath $output
        $ok=$phase.exitCode -ne 0 -and $diagnostic.Contains($guard.expected) -and $afterExists -eq $beforeExists
        $checks+=[pscustomobject]@{name=$guard.name;expectedDiagnostic=$guard.expected;exitCode=$phase.exitCode;diagnosticPresent=$diagnostic.Contains($guard.expected);output=$output;beforeExists=$beforeExists;afterExists=$afterExists;expectedRejection=$ok}
        if(-not $ok){throw "Guard $($guard.name) did not demonstrate the expected pre-output rejection; no retry"}
    }
    $status='NINE_EXPECTED_GUARD_REJECTIONS_ONLY'
}catch{$failure=$_.Exception.ToString()}
$result=EndRun $status $failure $checks
if($failure){exit 1}
exit 0