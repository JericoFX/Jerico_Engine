// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#include <JE/Editor/Render/Scene/SceneModule.h>
#include <SceneModuleInternal.h>

#include <EditorCommonFeaturesSystemComponent.h>
#include <PostProcess/EditorPostFxSystemComponent.h>
#include <CoreLights/EditorAreaLightComponent.h>
#include <CoreLights/EditorDirectionalLightComponent.h>
#include <CubeMapCapture/EditorCubeMapCaptureComponent.h>
#include <Debug/RayTracingDebugEditorComponent.h>
#include <Debug/RenderDebugEditorComponent.h>
#include <Decals/EditorDecalComponent.h>
#include <Grid/EditorGridComponent.h>
#include <ImageBasedLights/EditorImageBasedLightComponent.h>
#include <Mesh/EditorMeshComponent.h>
#include <Mesh/EditorMeshSystemComponent.h>
#include <ReflectionProbe/EditorReflectionProbeComponent.h>
#include <SpecularReflections/EditorSpecularReflectionsComponent.h>
#include <OcclusionCullingPlane/EditorOcclusionCullingPlaneComponent.h>
#include <PostProcess/EditorPostFxLayerComponent.h>
#include <PostProcess/Bloom/EditorBloomComponent.h>
#include <PostProcess/ColorGrading/EditorHDRColorGradingComponent.h>
#include <PostProcess/DepthOfField/EditorDepthOfFieldComponent.h>
#include <PostProcess/DisplayMapper/EditorDisplayMapperComponent.h>
#include <PostProcess/ExposureControl/EditorExposureControlComponent.h>
#include <PostProcess/AmbientOcclusion/EditorAoComponent.h>
#include <PostProcess/LookModification/EditorLookModificationComponent.h>
#include <PostProcess/RadiusWeightModifier/EditorRadiusWeightModifierComponent.h>
#include <PostProcess/ShapeWeightModifier/EditorShapeWeightModifierComponent.h>
#include <PostProcess/GradientWeightModifier/EditorGradientWeightModifierComponent.h>
#include <PostProcess/ChromaticAberration/EditorChromaticAberrationComponent.h>
#include <PostProcess/MotionBlur/EditorMotionBlurComponent.h>
#include <PostProcess/PaniniProjection/EditorPaniniProjectionComponent.h>
#include <PostProcess/FilmGrain/EditorFilmGrainComponent.h>
#include <PostProcess/WhiteBalance/EditorWhiteBalanceComponent.h>
#include <PostProcess/Vignette/EditorVignetteComponent.h>
#include <ScreenSpace/EditorDeferredFogComponent.h>
#include <SkyBox/EditorHDRiSkyboxComponent.h>
#include <SkyBox/EditorPhysicalSkyComponent.h>
#include <Scripting/EditorEntityReferenceComponent.h>
#include <SurfaceData/EditorSurfaceDataMeshComponent.h>
#include <Animation/EditorAttachmentComponent.h>

namespace JE::Editor::Render::Scene
{
    class SceneModule final : public AZ::Module
    {
    public:
        AZ_RTTI_WITH_NAME(SceneModule, "AtomLyIntegrationCommonFeaturesModule", "{E6FF4862-9355-4B23-AE00-B936F0C6E6C9}", AZ::Module);
        AZ_CLASS_ALLOCATOR(SceneModule, AZ::SystemAllocator);

        SceneModule()
        {
            JE::Render::Scene::AppendRuntimeComponentDescriptors(m_descriptors);
            m_descriptors.insert(m_descriptors.end(), {
                JE::Editor::Render::Scene::EditorAreaLightComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorCommonFeaturesSystemComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorPostFxSystemComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorDecalComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorDirectionalLightComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorBloomComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorHDRColorGradingComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorDepthOfFieldComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorDisplayMapperComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorExposureControlComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorAoComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorLookModificationComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorGridComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorHDRiSkyboxComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorImageBasedLightComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorMeshSystemComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorMeshComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorPhysicalSkyComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorPostFxLayerComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorReflectionProbeComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorSpecularReflectionsComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::RayTracingDebugEditorComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::RenderDebugEditorComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorRadiusWeightModifierComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorShapeWeightModifierComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorEntityReferenceComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorGradientWeightModifierComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorDeferredFogComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorSurfaceDataMeshComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorAttachmentComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorOcclusionCullingPlaneComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorChromaticAberrationComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorMotionBlurComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorPaniniProjectionComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorFilmGrainComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorWhiteBalanceComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorVignetteComponent::CreateDescriptor(),
                JE::Editor::Render::Scene::EditorCubeMapCaptureComponent::CreateDescriptor(),
            });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            auto required = JE::Render::Scene::GetRuntimeRequiredSystemComponents();
            required.insert(required.end(), {
                azrtti_typeid<EditorMeshSystemComponent>(),
                azrtti_typeid<EditorCommonFeaturesSystemComponent>(),
                azrtti_typeid<EditorPostFxSystemComponent>()
            });
            return required;
        }
    };

    AZ::Module* CreateModule()
    {
        return aznew SceneModule;
    }
}
