/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzFramework/Windowing/NativeWindow.h>
#include <AzFramework/Scene/Scene.h>

#include <Integration/Assets/ActorAsset.h>
#include <JE/Render/RPI/Public/Base.h>
#include <JE/Render/RPI/Public/ViewportContext.h>
#include <JE/Render/Features/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <JE/Render/Features/CoreLights/DirectionalLightFeatureProcessorInterface.h>
#include <JE/Render/Features/Utils/LightingPreset.h>

namespace JE::Render::Features
{
    class DisplayMapperFeatureProcessorInterface;
    class DirectionalLightFeatureProcessorInterface;
    class MeshFeatureProcessorInterface;
}

namespace AZ
{
    class Entity;
    class Component;

    namespace Render
    {



    }

    }

namespace JE::Render::RPI
{
        class WindowContext;
    }

namespace AZ
{
}

namespace EMStudio
{
    class RenderOptions;

    class AnimViewportRenderer
    {
    public:
        AZ_CLASS_ALLOCATOR(AnimViewportRenderer, AZ::SystemAllocator);

        AnimViewportRenderer(JE::Render::RPI::ViewportContextPtr viewportContext, const RenderOptions* renderOptions);
        ~AnimViewportRenderer();

        void Reinit();

        //! Return the center position of the existing objects.
        AZ::Vector3 GetCharacterCenter() const;

        void UpdateActorRenderFlag(EMotionFX::ActorRenderFlags renderFlags);
        AZStd::shared_ptr<AzFramework::Scene> GetFrameworkScene() const;
        AZ::EntityId GetEntityId() const;
        AzFramework::EntityContextId GetEntityContextId() const;

        //! Moves the groundplane along with the character.
        void UpdateGroundplane();

        //! Apply the identity transform to the actor entities.
        void MoveActorEntitiesToOrigin();

        //! Get the ID for the render scene.
        JE::Render::RPI::SceneId GetRenderSceneId() const;

        //! Get a list of actor entities in the animation viewport.
        const AZStd::vector<AZ::Entity*>& GetActorEntities() const;
    private:

        // This function resets the light, camera and other environment settings.
        void ResetEnvironment();

        // This function creates in-editor entities for all actor assets stored in the actor manager,
        // and deletes all the actor entities that no longer has an actor asset in the actor manager.
        // Those entities are used in atom render viewport to visualize actors in animation editor.
        void ReinitActorEntities();

        AZ::Entity* CreateActorEntity(AZ::Data::Asset<EMotionFX::Integration::ActorAsset> actorAsset);

        AZ::Entity* FindActorEntity(AZ::Data::Asset<EMotionFX::Integration::ActorAsset> actorAsset) const;
        void SetLightingPreset(const JE::Render::Features::LightingPreset* preset);

        AZStd::shared_ptr<JE::Render::RPI::WindowContext> m_windowContext;
        AZStd::unique_ptr<AzFramework::EntityContext> m_entityContext;
        AZStd::shared_ptr<AzFramework::Scene> m_frameworkScene;
        JE::Render::RPI::ScenePtr m_scene;
        JE::Render::RPI::RenderPipelinePtr m_renderPipeline;
        JE::Render::Features::DirectionalLightFeatureProcessorInterface* m_directionalLightFeatureProcessor = nullptr;
        JE::Render::Features::DisplayMapperFeatureProcessorInterface* m_displayMapperFeatureProcessor = nullptr;
        JE::Render::Features::SkyBoxFeatureProcessorInterface* m_skyboxFeatureProcessor = nullptr;
        JE::Render::Features::MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;

        AZ::Entity* m_postProcessEntity = nullptr;
        AZ::Entity* m_iblEntity = nullptr;
        AZ::Entity* m_groundEntity = nullptr;
        AZStd::vector<AZ::Entity*> m_actorEntities;
        const RenderOptions* m_renderOptions;

        const float DefaultFrustumDimension = 128.0f;
        const float TileSize = 1.0f;
        AZStd::vector<JE::Render::Features::DirectionalLightFeatureProcessorInterface::LightHandle> m_lightHandles;
    };
}
