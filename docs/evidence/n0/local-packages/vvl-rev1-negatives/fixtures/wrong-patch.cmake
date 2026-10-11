cmake_minimum_required(VERSION 3.24)
include("F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/Python/JERequireLocalPath.cmake")
include("F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/VulkanValidationLayers/ApplySpirvToolsProvenance.cmake")
JE_vvl_apply_spirv_provenance("F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-negatives/fixtures/wrong-patch")
