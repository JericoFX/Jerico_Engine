# Execute only after explicit parent dispatch to Luna. Six unexecuted guards only.
. (Join-Path $PSScriptRoot 'CommonExecution.ps1')
$authority='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev2-audit-v2/validation-arguments-rev2.json'
if((GetHash $authority) -cne 'BA3B3D3E25CB765D1B978ED147F8AA0546A2BA6F0D9B3D85910E6C9EA14AAAF0'){throw 'Guard authority changed'}
$plan=Get-Content -LiteralPath $authority -Raw|ConvertFrom-Json
$remainingNames=[string[]]@('missing-archive','archive-hash','missing-manifest','manifest-hash','external-output','old-output')
if($plan.guards.Count -ne 8 -or $plan.producer.arguments.Count -ne 5){throw 'Original guard/producer count differs'}
$remaining=@($plan.guards|Where-Object {$_.name -cin $remainingNames})
if($remaining.Count -ne 6){throw 'Remaining guard count differs'}
for($i=0;$i -lt 6;$i++){if($remaining[$i].name -cne $remainingNames[$i]){throw 'Remaining guard order differs'}}
# Existing guard fixtures are frozen inputs, never regenerated or rewritten.
$fixtures='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev2-guards'
if(-not [IO.Directory]::Exists($fixtures)){throw 'Retained guard fixture directory is missing'}
BeginRun 'remaining-guards-run'
$failure=$null;$checks=@();$status='FAIL_OR_INCOMPLETE'
try{
    if($script:Before.matched -ne $script:InputFreeze.count){throw 'Preflight changed; no native phase dispatched'}
    foreach($guard in $remaining){
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
        # CMake wraps prose; only whitespace is normalized for matching. Raw logs
        # and their byte hashes are retained by Native exactly as emitted.
        $normalizedDiagnostic=[regex]::Replace($diagnostic,'\s+',' ').Trim()
        $normalizedExpected=[regex]::Replace($guard.expectedDiagnostic,'\s+',' ').Trim()
        $diagnosticPresent=$normalizedDiagnostic.Contains($normalizedExpected)
        $afterExists=Test-Path -LiteralPath $output
        $ok=$phase.exitCode -ne 0 -and $diagnosticPresent -and $afterExists -eq $beforeExists
        $checks+=[pscustomobject]@{name=$guard.name;expectedDiagnostic=$guard.expectedDiagnostic;normalizedExpectedDiagnostic=$normalizedExpected;exitCode=$phase.exitCode;diagnosticPresent=$diagnosticPresent;diagnosticMatching='Whitespace-only normalization; case/text requirements preserved; raw logs hashed unchanged';output=$output;beforeExists=$beforeExists;afterExists=$afterExists;expectedRejection=$ok;oldOutputScope='Existence of retained directory only; no full private old-build manifest claim'}
        if(-not $ok){throw "Guard $($guard.name) did not demonstrate expected pre-output rejection; no retry"}
    }
    $status='SIX_REMAINING_EXPECTED_GUARD_REJECTIONS_ONLY'
}catch{$failure=$_.Exception.ToString()}
$result=EndRun $status $failure $checks
if($failure){exit 1}
exit 0
