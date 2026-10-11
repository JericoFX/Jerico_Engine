#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = [IO.Path]::GetFullPath($PSScriptRoot)
if ($root -ne 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\dxc-rev1-negatives') { throw 'Assigned root differs' }
$encoding = [Text.UTF8Encoding]::new($false)
function Write-New([string] $path,[string] $text) { if(Test-Path -LiteralPath $path){throw "Evidence exists: $path"}; [IO.File]::WriteAllText($path,$text,$encoding) }
function Entry([string] $path) { @{ path = [IO.Path]::GetFullPath($path); bytes = (Get-Item -LiteralPath $path).Length; sha256 = (Get-FileHash -LiteralPath $path).Hash } }
$driverPath = Join-Path $root 'RunDxcNegativesRev2.ps1'
$assignmentPath = Join-Path $root 'negative-assignment-rev2.json'
$assignment = Get-Content -LiteralPath $assignmentPath -Raw | ConvertFrom-Json
if ((Get-FileHash -LiteralPath $driverPath).Hash -ne 'ECBF9321FFCCE2BCEC73DBD60F9C0916AD235969BEBAE381BCAC107ADFAF845F' -or (Get-FileHash -LiteralPath $assignmentPath).Hash -ne 'D0AF36852D02984ED2F645ADDD29395923768DBA8B8838A1A5DD3F88B6FC8D7C') { throw 'Rev2 candidates changed' }
$attempt = Join-Path $root 'attempt-rev2'
if (Test-Path -LiteralPath $attempt) { throw 'Fixtures must not have been created' }
$checks = @(foreach($case in $assignment.cases) {
    if(Test-Path -LiteralPath $case.secondaryMissingPath){throw 'Secondary guard file exists'}
    if ($case.scope -eq 'PRODUCER') {
        $cl = @($case.argv | Where-Object {$_ -like '-DJE_DXC_CL=*'})
        if($cl.Count -ne 1 -or $cl[0] -ne "-DJE_DXC_CL=$(Join-Path $attempt 'fixtures/missing-secondary-cl.exe')"){throw 'Producer variant has no absent CL backstop'}
        if($case.argv[-1] -notlike '*BuildDirectXShaderCompilerDxcPackage.cmake'){throw 'Producer entrypoint changed'}
    } elseif($case.scope -eq 'FUNCTION-GUARD') {
        if($case.argv.Count -ne 2 -or $case.argv[0] -ne '-P' -or $case.argv[1] -ne $assignment.helperFixture.path){throw 'Function case must not invoke producer'}
        if($assignment.helperFixture.content -notmatch 'JE_python_verify_file' -or $assignment.helperFixture.content -match 'execute_process|BuildDirectXShaderCompilerDxcPackage'){throw 'Function fixture differs'}
    } elseif($case.scope -ne 'PRIVATE-OVERLAY'){throw 'Unknown scope'}
    @{ label = $case.label; scope = $case.scope; specificDiagnostic = $case.expectedError; secondaryMissingFile = $case.secondaryMissingPath; noOutputPaths = @($case.mustRemainAbsent); staticOnly = $true }
})
if(@($checks|Where-Object scope -eq 'PRODUCER').Count -ne 16 -or @($checks|Where-Object scope -eq 'FUNCTION-GUARD').Count -ne 1 -or @($checks|Where-Object scope -eq 'PRIVATE-OVERLAY').Count -ne 1){throw 'Scope counts differ'}
$astResults = @(foreach($path in @($driverPath,(Join-Path $root 'PrepareNegativeAssignmentRev2.ps1'),$PSCommandPath)) {
    $tokens=$null; $errors=$null
    [void][Management.Automation.Language.Parser]::ParseFile($path,[ref]$tokens,[ref]$errors)
    if($errors.Count){throw 'PowerShell parser error'}
    @{ path = $path; parserErrors = $errors.Count }
})
$oldLines = [IO.File]::ReadAllLines((Join-Path $root 'RunDxcNegatives.ps1'))
$newLines = [IO.File]::ReadAllLines($driverPath)
$stride = $newLines.Length+1
$matrix = [int[]]::new(($oldLines.Length+1)*$stride)
for($i=$oldLines.Length-1;$i -ge 0;$i--){for($j=$newLines.Length-1;$j -ge 0;$j--){
    $at=$i*$stride+$j
    if($oldLines[$i] -ceq $newLines[$j]){$matrix[$at]=1+$matrix[(($i+1)*$stride+$j+1)]}
    else{$matrix[$at]=[Math]::Max($matrix[(($i+1)*$stride+$j)],$matrix[($i*$stride+$j+1)])}
}}
$ops=[Collections.Generic.List[object]]::new(); $i=0; $j=0
while($i -lt $oldLines.Length -or $j -lt $newLines.Length){
    if($i -lt $oldLines.Length -and $j -lt $newLines.Length -and $oldLines[$i] -ceq $newLines[$j]){$ops.Add(@{tag=' '; text=$oldLines[$i]; old=$i+1; new=$j+1});$i++;$j++}
    elseif($j -lt $newLines.Length -and ($i -eq $oldLines.Length -or $matrix[($i*$stride+$j+1)] -ge $matrix[(($i+1)*$stride+$j)])){$ops.Add(@{tag='+';text=$newLines[$j];old=$i+1;new=$j+1});$j++}
    else{$ops.Add(@{tag='-';text=$oldLines[$i];old=$i+1;new=$j+1});$i++}
}
$ranges=[Collections.Generic.List[object]]::new()
for($k=0;$k -lt $ops.Count;$k++){if($ops[$k].tag -ne ' '){
    $start=[Math]::Max(0,$k-3);$end=[Math]::Min($ops.Count-1,$k+3)
    if($ranges.Count -and $start -le $ranges[-1].end+1){$ranges[-1].end=$end}else{$ranges.Add(@{start=$start;end=$end})}
}}
$diff=[Text.StringBuilder]::new()
[void]$diff.AppendLine('--- RunDxcNegatives.ps1 (historical rev1)')
[void]$diff.AppendLine('+++ RunDxcNegativesRev2.ps1 (prepared only)')
foreach($range in $ranges){
    $removed=0;$added=0
    for($k=$range.start;$k -le $range.end;$k++){if($ops[$k].tag -ne '+'){$removed++};if($ops[$k].tag -ne '-'){$added++}}
    [void]$diff.AppendLine("@@ -$($ops[$range.start].old),$removed +$($ops[$range.start].new),$added @@")
    for($k=$range.start;$k -le $range.end;$k++){[void]$diff.AppendLine($ops[$k].tag+$ops[$k].text)}
}
$diffPath=Join-Path $root 'driver-source-diff-rev2.patch'
Write-New $diffPath ($diff.ToString().Replace("`r`n","`n"))
$receiptPath=Join-Path $root 'negative-static-review-rev2.json'
$receipt=@{status='STATIC PREPARATION ONLY; NATIVE PENDING'; nativeExecuted=$false; acceptance=$false; cases=$checks; scopeCounts=$assignment.scopeCounts; powershell=$astResults; attemptExists=$false; fixturesCreated=0; noPositiveProducerVariant=$true; secondaryGuardOrder='Producer: archives -> PYTHON -> NINJA -> absent CL -> notices -> output creation. Python SHA/missing-tool additionally use absent Ninja. FUNCTION-GUARD: real verifier wrong CL hash -> absent file. PRIVATE-OVERLAY: wrong first file -> absent next version.inc.'; productionArchiveSHA256=$assignment.productionWitnesses[0].sha256; driverDiff=(Entry $diffPath)}
Write-New $receiptPath (($receipt|ConvertTo-Json -Depth 16)+"`n")
$setupPath=Join-Path $root 'negative-setup-freeze-rev2.json'
$setup=Get-Content -LiteralPath $setupPath -Raw|ConvertFrom-Json
$entries=@($setup.files)+@((Entry $setupPath),(Entry $diffPath),(Entry $receiptPath),(Entry $PSCommandPath))
foreach($file in $entries){if((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $file.path).Hash -ne $file.sha256){throw 'Review input changed'}}
$reviewFreeze=Join-Path $root 'negative-review-freeze-rev2.json'
Write-New $reviewFreeze ((@{status='PENDING PARENT REVIEW / LUNA EXECUTION';fileCount=$entries.Count;files=$entries; sourceFreeze=$assignment.sourceFreeze;caseCount=18;scopeCounts=$assignment.scopeCounts;nativeExecuted=$false;acceptance=$false}|ConvertTo-Json -Depth 16)+"`n")
@{driver=(Entry $driverPath); assignment=(Entry $assignmentPath); diff=(Entry $diffPath); freeze=(Entry $reviewFreeze); receipt=(Entry $receiptPath); freezeFiles=$entries.Count; cases=$checks.Count; ASTerrors=0; nativeExecuted=$false}|ConvertTo-Json -Depth 4
