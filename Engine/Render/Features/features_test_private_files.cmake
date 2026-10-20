# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

include(${CMAKE_CURRENT_LIST_DIR}/features_private_files.cmake)
# Editor SourceData consumes these exported bodies from the single shared provider.
list(REMOVE_ITEM FILES
    Source/Material/SubsurfaceTransmissionParameterFunctor.cpp
    Source/Material/Transform2DFunctor.cpp
    Source/Material/UseTextureFunctor.cpp
)

# DPG modes tests use private bodies; the editor imports these five bodies from the shared provider.
set(features_existing_private_files ${FILES})
include(${CMAKE_CURRENT_LIST_DIR}/features_diffuse_probe_grid_files.cmake)
list(REMOVE_ITEM FILES
    Source/DiffuseProbeGrid/Components/DiffuseGlobalIlluminationComponent.cpp
    Source/DiffuseProbeGrid/Components/DiffuseGlobalIlluminationComponentConfig.cpp
    Source/DiffuseProbeGrid/Components/DiffuseGlobalIlluminationComponentController.cpp
    Source/DiffuseProbeGrid/Components/DiffuseProbeGridComponent.cpp
    Source/DiffuseProbeGrid/Components/DiffuseProbeGridComponentController.cpp
)
list(PREPEND FILES ${features_existing_private_files})
unset(features_existing_private_files)
