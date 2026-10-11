$ErrorActionPreference = 'Continue'
$moduleBefore = @(Get-Module -Name Microsoft.PowerShell.Utility | Select-Object Name,Version,Path)
$available = @(Get-Module -ListAvailable -Name Microsoft.PowerShell.Utility | Select-Object Name,Version,Path)
$autoloadVar = Get-Variable -Name PSModuleAutoLoadingPreference -Scope Global -ErrorAction SilentlyContinue
$cmd = Get-Command -Name Get-FileHash -ErrorAction SilentlyContinue
$hashResult = $null
$hashError = $null
if ($cmd) {
  try { $hashResult = (Get-FileHash -LiteralPath 'F:\JericoEngine\JericoEngine\cmake\Tools\LocalPackages\Python\JERequireLocalPath.cmake' -Algorithm SHA256).Hash }
  catch { $hashError = $_.ToString() }
}
$expected = '4B74565484AC8EA233D6D899E74D3128E1023B92F20DA3BE2CCB540EFFA05143'
$dotnetHash = $null
$dotnetError = $null
$stream = $null
$sha = $null
try {
  $sha = [Security.Cryptography.SHA256]::Create()
  $stream = [IO.File]::OpenRead('F:\JericoEngine\JericoEngine\cmake\Tools\LocalPackages\Python\JERequireLocalPath.cmake')
  $bytes = $sha.ComputeHash($stream)
  $dotnetHash = ([BitConverter]::ToString($bytes)).Replace('-', '')
} catch { $dotnetError = $_.ToString() }
finally { if ($stream) { $stream.Dispose() }; if ($sha) { $sha.Dispose() } }
$result = [ordered]@{
  version=$PSVersionTable.PSVersion.ToString()
  edition=$PSVersionTable.PSEdition
  ps_module_path=$env:PSModulePath
  autoload_preference_defined=($null -ne $autoloadVar)
  autoload_preference=if ($null -eq $autoloadVar) { 'unset (PowerShell default applies)' } else { [string]$autoloadVar.Value }
  utility_module_available=$available
  utility_module_loaded_before=$moduleBefore
  get_file_hash_command_found=($null -ne $cmd)
  get_file_hash_command=if ($cmd) { [ordered]@{name=$cmd.Name;command_type=[string]$cmd.CommandType;source=$cmd.Source;module_name=$cmd.ModuleName;version=[string]$cmd.Version;definition=$cmd.Definition} } else { $null }
  get_file_hash_sha256=$hashResult
  get_file_hash_matches_expected=($hashResult -ieq $expected)
  get_file_hash_error=$hashError
  dotnet_sha256=$dotnetHash
  dotnet_matches_expected=($dotnetHash -ieq $expected)
  dotnet_error=$dotnetError
  utility_module_loaded_after=@(Get-Module -Name Microsoft.PowerShell.Utility | Select-Object Name,Version,Path)
}
$result | ConvertTo-Json -Depth 6
