param([Parameter(Mandatory=$true)][ValidateSet('vulkan','dx12')][string]$Backend, [ValidateRange(1,99)][int]$Attempt=1)
$ErrorActionPreference='Stop'
$root='F:\JericoEngine\JericoEngine'
$out=Join-Path $PSScriptRoot ($Backend+'-attempt'+$Attempt)
if(Test-Path $out){throw "Attempt already exists: $out"}
New-Item -ItemType Directory -Path $out | Out-Null
$exe="$root\build\n0-validation\original-engine\bin\profile\JEEditorFixture.GameLauncher.exe"
$project="$root\Projects\JEEditorFixture"
if(Get-Process -Name JEEditorFixture.GameLauncher,AssetProcessor,Editor -ErrorAction SilentlyContinue){throw 'Other product process active'}
$launchArgs=@('--project-path',('"'+$project+'"'),('--rhi='+$Backend),'--bg_ConnectToAssetProcessor=false','--regset=/Amazon/AzCore/Bootstrap/wait_for_connect=0','--regset=/Amazon/AzCore/Bootstrap/remote_filesystem=0')
$prefab="$project\Levels\JEEditorFixture\JEEditorFixture.prefab"
$start=[DateTime]::UtcNow
$p=Start-Process -FilePath $exe -ArgumentList $launchArgs -WorkingDirectory $project -PassThru
@{pid=$p.Id;utc=$start.ToString('o');exe=$exe;exeSha256=(Get-FileHash $exe).Hash;crySystemSha256=(Get-FileHash (Join-Path (Split-Path $exe) 'CrySystem.dll')).Hash;arguments=$launchArgs;prefabSha256=(Get-FileHash $prefab).Hash} | ConvertTo-Json -Depth 4 | Set-Content "$out\launch.json"
$samples=@()
for($i=0;$i -lt 20;$i++){
 $live=Get-Process -Id $p.Id -ErrorAction Stop
 $tcp=@(Get-NetTCPConnection | Where-Object {$_.OwningProcess -eq $p.Id} | Select-Object State,LocalAddress,LocalPort,RemoteAddress,RemotePort)
 $udp=@(Get-NetUDPEndpoint | Where-Object {$_.OwningProcess -eq $p.Id} | Select-Object LocalAddress,LocalPort)
 $tools=@(Get-Process -Name AssetProcessor,Editor -ErrorAction SilentlyContinue | Select-Object Id,ProcessName)
 $samples+=@{elapsedSeconds=([DateTime]::UtcNow-$start).TotalSeconds;tcp=$tcp;udp=$udp;tools=$tools;workingSetBytes=$live.WorkingSet64;privateBytes=$live.PrivateMemorySize64}
 Start-Sleep -Milliseconds 1000
}
ConvertTo-Json -InputObject $samples -Depth 6 | Set-Content "$out\startup-samples.json"
(Get-Process -Id $p.Id).Modules | Select-Object ModuleName,FileName,ModuleMemorySize | ConvertTo-Json -Depth 4 | Set-Content "$out\loaded-modules.json"
$bad=@($samples | Where-Object {$_.tcp.Count -ne 0 -or $_.udp.Count -ne 0 -or $_.tools.Count -ne 0})
@{pid=$p.Id;sampleCount=$samples.Count;badSamples=$bad.Count;durationSeconds=([DateTime]::UtcNow-$start).TotalSeconds;prefabUnchanged=((Get-FileHash $prefab).Hash -eq 'FEE55849A032D29059D4369112047DC27E281D39631D0F0DF87D8BCF5EF517AA');scope='Sampled TCP/UDP endpoints and Editor/AP absence; parent must still inspect visible render. Process left running for UI verification.'} | ConvertTo-Json | Set-Content "$out\network-result.json"
Get-Content "$out\network-result.json"
if($bad.Count -ne 0){exit 1}