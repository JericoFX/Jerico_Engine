param([Parameter(Mandatory)][string]$AzslcPath,[Parameter(Mandatory)][string]$EvidencePath)
$ErrorActionPreference='Stop'
$repo='F:/JericoEngine/JericoEngine'
foreach($path in @($AzslcPath,$EvidencePath)){if(-not [IO.Path]::IsPathFullyQualified($path) -or $path -match '(^[\\/]{2})|://'){throw 'explicit absolute local paths required'}}
$exe=[IO.Path]::GetFullPath($AzslcPath)
$evidence=[IO.Path]::GetFullPath($EvidencePath)
if(-not $evidence.StartsWith([IO.Path]::GetFullPath((Join-Path $repo 'build'))+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'evidence must be beneath checkout/build'}
if(-not(Test-Path -LiteralPath $exe -PathType Leaf) -or (Test-Path -LiteralPath $evidence)){throw 'built executable missing or evidence exists'}
$fixture=Join-Path $repo 'Code/Tools/ShaderCompiler/Tests/Advanced/compute-entries.azsl'
[IO.Directory]::CreateDirectory($evidence) | Out-Null
function Invoke-Case([string]$name,[string]$inputPath){
    $arguments=@('--ia',$inputPath)
    [ordered]@{executable=$exe;executableSHA256=(Get-FileHash -Algorithm SHA256 -LiteralPath $exe).Hash;arguments=$arguments;inputSHA256=(Get-FileHash -Algorithm SHA256 -LiteralPath $inputPath).Hash;workingDirectory=$evidence} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence "$name-argv.json") -Encoding utf8
    & $exe @arguments 1> (Join-Path $evidence "$name.stdout.json") 2> (Join-Path $evidence "$name.stderr.log")
    $nativeExit=$LASTEXITCODE
    $nativeExit | Set-Content -LiteralPath (Join-Path $evidence "$name.exitcode") -Encoding utf8
    return $nativeExit
}
Push-Location $evidence
try{
    if((Invoke-Case 'compute-entries' $fixture) -ne 0){throw 'real Azslc reflection failed'}
    $json=Get-Content -Raw -LiteralPath (Join-Path $evidence 'compute-entries.stdout.json') | ConvertFrom-Json
    if($json.meta -cne 'Shader entries exported by AZSLc' -or $json.inputLayouts.Count -ne 5){throw 'unexpected real reflection schema/entry count'}
    $expected=@{MainCS1=@(1,1,1);MainCS2=@(8,1,1);MainCS3=@(16,4,1);MainCS4=@(1,1,64)}
    foreach($name in $expected.Keys){$entry=@($json.inputLayouts | Where-Object entry -eq $name);if($entry.Count -ne 1 -or ($entry[0].numthreads -join ',') -ne ($expected[$name] -join ',')){throw "thread reflection mismatch: $name"}}
    if(@($json.inputLayouts | Where-Object entry -eq 'MainVS1').Count -ne 1){throw 'vertex entry absent'}
    $bad=Join-Path $evidence 'invalid.azsl'
    [IO.File]::WriteAllText($bad,'float4 Broken(float4 position) { return position + ; }',[Text.UTF8Encoding]::new($false))
    $badExit=Invoke-Case 'invalid' $bad
    if($badExit -eq 0 -or (Get-Item -LiteralPath (Join-Path $evidence 'invalid.stderr.log')).Length -eq 0){throw 'invalid AZSL was accepted or lacks diagnostics'}
    [ordered]@{status='PASS';originalAzslc=$exe;computeEntries=4;vertexEntries=1;expectedThreadVectors=$expected;invalidExitCode=$badExit;scope='Original Azslc/ANTLR/JSONCpp CPU reflection and malformed-source rejection only; no GPU/Editor/performance acceptance'} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $evidence 'result.json') -Encoding utf8
}finally{Pop-Location}
