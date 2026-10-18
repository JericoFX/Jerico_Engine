// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#include <JE/Render/Features/Mesh/MeshFeatureProcessorInterface.h>

namespace JE::Render::Features
{
    AZ_CVAR_API(JE_RENDER_FEATURES_API,bool,
            r_enablePerMeshShaderOptionFlags,
            false,
            nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "Enable allowing systems to set shader options on a per-mesh basis."
        );
    AZ_CVAR_API(JE_RENDER_FEATURES_API,
            bool,
            r_meshInstancingEnabled,
            false,
            nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "Enable instanced draw calls in the MeshFeatureProcessor.");
    AZ_CVAR_API(JE_RENDER_FEATURES_API,
            bool,
            r_meshInstancingEnabledForTransparentObjects,
            false,
            nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "Enable instanced draw calls for transparent objects in the MeshFeatureProcessor. Use this only if you have many instances of the same "
            "transparent object, but don't have multiple different transparent objects mixed together. See documentation for details.");
    AZ_CVAR_API(JE_RENDER_FEATURES_API,
            size_t,
            r_meshInstancingBucketSortScatterBatchSize,
            512,
            nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "Batch size for the first stage of the mesh instancing bucket sort. "
            "Can be modified to find optimal load balancing for the multi-threaded tasks.");
    AZ_CVAR_API(JE_RENDER_FEATURES_API,
            bool,
            r_meshInstancingDebugForceUniqueObjectsForProfiling,
            false,
            nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "Enable instanced draw calls in the MeshFeatureProcessor, but force one object per draw call. "
            "This is helpful for simulating the worst case scenario for instancing for profiling performance.");
}
