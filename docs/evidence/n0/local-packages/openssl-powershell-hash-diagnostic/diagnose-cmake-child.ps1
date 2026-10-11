param([Parameter(Mandatory=$true)][string]$Root,[Parameter(Mandatory=$true)][string]$Manifest)
$ErrorActionPreference='Continue'
$before=@(Get-Module -Name Microsoft.PowerShell.Utility|Select-Object Name,Version,Path)
$available=@(Get-Module -ListAvailable -Name Microsoft.PowerShell.Utility|Select-Object Name,Version,Path)
$pref=Get-Variable -Name PSModuleAutoLoadingPreference -Scope Global -ErrorAction SilentlyContinue
$cmd=Get-Command Get-FileHash -ErrorAction SilentlyContinue
$hash=$null;$hashError=$null
if($cmd){try{$hash=(Get-FileHash -LiteralPath 'F:\JericoEngine\JericoEngine\cmake\Tools\LocalPackages\Python\JERequireLocalPath.cmake' -Algorithm SHA256).Hash}catch{$hashError=$_.ToString()}}
$sha=$null;$stream=$null;$dotnet=$null;$dotnetError=$null
try{$sha=[Security.Cryptography.SHA256]::Create();$stream=[IO.File]::OpenRead('F:\JericoEngine\JericoEngine\cmake\Tools\LocalPackages\Python\JERequireLocalPath.cmake');$dotnet=([BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-','')}catch{$dotnetError=$_.ToString()}finally{if($stream){$stream.Dispose()};if($sha){$sha.Dispose()}}
[ordered]@{version=$PSVersionTable.PSVersion.ToString();edition=$PSVersionTable.PSEdition;ps_module_path=$env:PSModulePath;autoload_defined=($null-ne $pref);autoload=if($null-eq $pref){'unset/default'}else{[string]$pref.Value};utility_available=$available;utility_loaded_before=$before;get_file_hash_found=($null-ne $cmd);command_type=if($cmd){[string]$cmd.CommandType}else{$null};module_name=if($cmd){$cmd.ModuleName}else{$null};get_file_hash_sha256=$hash;get_file_hash_error=$hashError;dotnet_sha256=$dotnet;dotnet_error=$dotnetError;root_arg=$Root;manifest_arg=$Manifest}|ConvertTo-Json -Depth 5
