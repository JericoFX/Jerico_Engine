$ErrorActionPreference = 'Stop'
$badStage = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\negative\metadata-stage'
$helper = 'F:\JericoEngine\JericoEngine\cmake\Tools\LocalPackages\JECreateLocalPackage.cmake'
# Exact expression used in the failed attempt:
$broken = @('-DJE_PACKAGE_STAGE='+$badStage,'-DJE_PACKAGE_NAME=probe-metadata','-DJE_PACKAGE_OUTPUT=F:\tmp','-DJE_PACKAGE_VERSION=1','-P',$helper)
# Explicit interpolation preserves one native argument per option:
[ string[] ]$fixed = @("-DJE_PACKAGE_STAGE=$badStage",'-DJE_PACKAGE_NAME=probe-metadata','-DJE_PACKAGE_OUTPUT=F:\tmp','-DJE_PACKAGE_VERSION=1','-P',$helper)
[ordered]@{
    script_path = $PSCommandPath
    broken_count = $broken.Count
    broken_arguments = @($broken | ForEach-Object { $_ })
    corrected_count = $fixed.Count
    corrected_arguments = @($fixed | ForEach-Object { $_ })
    expected_cmake_mode = 'script (-P) only after splatting corrected vector'
    cmake_invoked = $false
} | ConvertTo-Json -Depth 5
