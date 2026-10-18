# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

include(${CMAKE_CURRENT_LIST_DIR}/features_private_files.cmake)
# Editor SourceData consumes these exported bodies from the single shared provider.
list(REMOVE_ITEM FILES
    Source/Material/SubsurfaceTransmissionParameterFunctor.cpp
    Source/Material/Transform2DFunctor.cpp
    Source/Material/UseTextureFunctor.cpp
)
