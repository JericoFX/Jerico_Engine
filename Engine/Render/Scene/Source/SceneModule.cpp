/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>

#include <SceneSystemComponent.h>
#include <CoreLights/AreaLightComponent.h>
#include <CoreLights/DirectionalLightComponent.h>
#include <CubeMapCapture/CubeMapCaptureComponent.h>
#include <Debug/RayTracingDebugComponent.h>
#include <Debug/RenderDebugComponent.h>
#include <Decals/DecalComponent.h>
#include <Grid/GridComponent.h>
#include <ImageBasedLights/ImageBasedLightComponent.h>
#include <Mesh/MeshComponent.h>
#include <ReflectionProbe/ReflectionProbeComponent.h>
#include <SpecularReflections/SpecularReflectionsComponent.h>
#include <OcclusionCullingPlane/OcclusionCullingPlaneComponent.h>
#include <PostProcess/PostFxLayerComponent.h>
#include <PostProcess/Bloom/BloomComponent.h>
#include <PostProcess/ColorGrading/HDRColorGradingComponent.h>
#include <PostProcess/DepthOfField/DepthOfFieldComponent.h>
#include <PostProcess/DisplayMapper/DisplayMapperComponent.h>
#include <PostProcess/ExposureControl/ExposureControlComponent.h>
#include <PostProcess/AmbientOcclusion/AoComponent.h>
#include <PostProcess/LookModification/LookModificationComponent.h>
#include <PostProcess/RadiusWeightModifier/RadiusWeightModifierComponent.h>
#include <PostProcess/ShapeWeightModifier/ShapeWeightModifierComponent.h>
#include <PostProcess/GradientWeightModifier/GradientWeightModifierComponent.h>
#include <PostProcess/ChromaticAberration/ChromaticAberrationComponent.h>
#include <PostProcess/MotionBlur/MotionBlurComponent.h>
#include <PostProcess/PaniniProjection/PaniniProjectionComponent.h>
#include <PostProcess/FilmGrain/FilmGrainComponent.h>
#include <PostProcess/WhiteBalance/WhiteBalanceComponent.h>
#include <PostProcess/Vignette/VignetteComponent.h>
#include <ScreenSpace/DeferredFogComponent.h>
#include <SkyBox/HDRiSkyboxComponent.h>
#include <SkyBox/PhysicalSkyComponent.h>
#include <Scripting/EntityReferenceComponent.h>
#include <SurfaceData/SurfaceDataMeshComponent.h>
#include <Animation/AttachmentComponent.h>

#include <JE/Render/Scene/SceneModule.h>
#include <SceneModuleInternal.h>

namespace JE::Render::Scene
{
    void AppendRuntimeComponentDescriptors(AZStd::list<AZ::ComponentDescriptor*>& descriptors)
    {
        descriptors.insert(descriptors.end(), {
            SceneSystemComponent::CreateDescriptor(),
            AreaLightComponent::CreateDescriptor(),
            DecalComponent::CreateDescriptor(),
            DirectionalLightComponent::CreateDescriptor(),
            BloomComponent::CreateDescriptor(),
            HDRColorGradingComponent::CreateDescriptor(),
            DisplayMapperComponent::CreateDescriptor(),
            DepthOfFieldComponent::CreateDescriptor(),
            ExposureControlComponent::CreateDescriptor(),
            AoComponent::CreateDescriptor(),
            LookModificationComponent::CreateDescriptor(),
            GridComponent::CreateDescriptor(),
            HDRiSkyboxComponent::CreateDescriptor(),
            ImageBasedLightComponent::CreateDescriptor(),
            MeshComponent::CreateDescriptor(),
            PhysicalSkyComponent::CreateDescriptor(),
            PostFxLayerComponent::CreateDescriptor(),
            ReflectionProbeComponent::CreateDescriptor(),
            SpecularReflectionsComponent::CreateDescriptor(),
            RayTracingDebugComponent::CreateDescriptor(),
            RenderDebugComponent::CreateDescriptor(),
            RadiusWeightModifierComponent::CreateDescriptor(),
            ShapeWeightModifierComponent::CreateDescriptor(),
            EntityReferenceComponent::CreateDescriptor(),
            GradientWeightModifierComponent::CreateDescriptor(),
            DeferredFogComponent::CreateDescriptor(),
            JE::Render::Scene::SurfaceDataMeshComponent::CreateDescriptor(),
            AttachmentComponent::CreateDescriptor(),
            OcclusionCullingPlaneComponent::CreateDescriptor(),
            ChromaticAberrationComponent::CreateDescriptor(),
            MotionBlurComponent::CreateDescriptor(),
            PaniniProjectionComponent::CreateDescriptor(),
            FilmGrainComponent::CreateDescriptor(),
            WhiteBalanceComponent::CreateDescriptor(),
            VignetteComponent::CreateDescriptor(),
            CubeMapCaptureComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList GetRuntimeRequiredSystemComponents()
    {
        return { azrtti_typeid<SceneSystemComponent>() };
    }

    class SceneModule final : public AZ::Module
    {
    public:
        AZ_RTTI_WITH_NAME(SceneModule, "AtomLyIntegrationCommonFeaturesModule", "{E6FF4862-9355-4B23-AE00-B936F0C6E6C9}", AZ::Module);
        AZ_CLASS_ALLOCATOR(SceneModule, AZ::SystemAllocator);

        SceneModule()
        {
            AppendRuntimeComponentDescriptors(m_descriptors);
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return GetRuntimeRequiredSystemComponents();
        }
    };

    AZ::Module* CreateModule()
    {
        return aznew SceneModule;
    }
}
