/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/DiffuseGlobalIlluminationComponentConfig.h>
#include <Components/DiffuseProbeGridComponentController.h>
#include <Render/DiffuseProbeGrid.h>

#include <Common/AssetManagerTestFixture.h>
#include <Common/RPITestFixture.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Visibility/OctreeSystemComponent.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <JE/Render/RPI/Public/Image/AttachmentImage.h>
#include <JE/Render/RPI/Public/Image/ImageSystemInterface.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImageAsset.h>

namespace UnitTest
{
    class DiffuseProbeGridReflectionTests : public AssetManagerTestFixture
    {
    protected:
        void SetUp() override
        {
            AssetManagerTestFixture::SetUp();
            GetSerializeContext()->Class<AZ::ComponentConfig>();
            JE::Render::Features::DiffuseProbeGridComponentConfig::Reflect(GetSerializeContext());
            JE::Render::Features::DiffuseGlobalIlluminationComponentConfig::Reflect(GetSerializeContext());
        }
    };

    TEST_F(DiffuseProbeGridReflectionTests, ConfigPreservesOriginalIdentityVersionAndAllTwentyTwoFields)
    {
        using Config = JE::Render::Features::DiffuseProbeGridComponentConfig;
        EXPECT_EQ(azrtti_typeid<Config>(), AZ::Uuid("{BF190F2A-D7F7-453B-9D42-5CE940180DCE}"));
        EXPECT_STREQ(Config::RTTI_TypeName(), "AZ::Render::DiffuseProbeGridComponentConfig");

        const auto* data = GetSerializeContext()->FindClassData(azrtti_typeid<Config>());
        ASSERT_NE(data, nullptr);
        EXPECT_EQ(data->m_version, 6);
        const char* expected[] = {
            "ProbeSpacing", "Extents", "AmbientMultiplier", "ViewBias", "NormalBias", "NumRaysPerProbe",
            "Scrolling", "EdgeBlendIbl", "FrameUpdateCount", "TransparencyMode", "EmissiveMultiplier",
            "EditorMode", "RuntimeMode", "BakedIrradianceTextureRelativePath", "BakedDistanceTextureRelativePath",
            "BakedProbeDataTextureRelativePath", "BakedIrradianceTextureAsset", "BakedDistanceTextureAsset",
            "BakedProbeDataTextureAsset", "VisualizationEnabled", "VisualizationShowInactiveProbes", "VisualizationSphereRadius"
        };
        ASSERT_EQ(data->m_elements.size(), AZ_ARRAY_SIZE(expected));
        for (size_t index = 0; index < AZ_ARRAY_SIZE(expected); ++index)
        {
            EXPECT_STREQ(data->m_elements[index].m_name, expected[index]);
        }

        const Config defaults;
        EXPECT_EQ(defaults.m_editorMode, JE::Render::Features::DiffuseProbeGridMode::RealTime);
        EXPECT_EQ(defaults.m_runtimeMode, JE::Render::Features::DiffuseProbeGridMode::RealTime);
    }

    TEST_F(DiffuseProbeGridReflectionTests, ConfigRoundTripPreservesAuthoredModesParametersAndBakedReferences)
    {
        JE::Render::Features::DiffuseProbeGridComponentConfig original;
        original.m_probeSpacing = AZ::Vector3(1.5f, 2.5f, 3.5f);
        original.m_extents = AZ::Vector3(12.0f, 14.0f, 16.0f);
        original.m_ambientMultiplier = 2.25f;
        original.m_viewBias = 0.75f;
        original.m_normalBias = 0.25f;
        original.m_numRaysPerProbe = JE::Render::Features::DiffuseProbeGridNumRaysPerProbe::NumRaysPerProbe_576;
        original.m_scrolling = true;
        original.m_edgeBlendIbl = false;
        original.m_frameUpdateCount = 3;
        original.m_transparencyMode = JE::Render::Features::DiffuseProbeGridTransparencyMode::Full;
        original.m_emissiveMultiplier = 3.0f;
        original.m_editorMode = JE::Render::Features::DiffuseProbeGridMode::Baked;
        original.m_runtimeMode = JE::Render::Features::DiffuseProbeGridMode::AutoSelect;
        original.m_bakedIrradianceTextureRelativePath = "textures/dpg/irradiance.streamingimage";
        original.m_bakedDistanceTextureRelativePath = "textures/dpg/distance.streamingimage";
        original.m_bakedProbeDataTextureRelativePath = "textures/dpg/probedata.streamingimage";
        using ImageAsset = AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>;
        const auto imageType = azrtti_typeid<JE::Render::RPI::StreamingImageAsset>();
        original.m_bakedIrradianceTextureAsset = ImageAsset(
            AZ::Data::AssetId("{DB655DF8-CFC2-4D3F-8EB8-A77303D09811}", 1), imageType);
        original.m_bakedDistanceTextureAsset = ImageAsset(
            AZ::Data::AssetId("{E1513BF7-52CB-405B-84E7-D79183F54B32}", 2), imageType);
        original.m_bakedProbeDataTextureAsset = ImageAsset(
            AZ::Data::AssetId("{7278700A-C7E3-4E17-9C5D-BA123054C253}", 3), imageType);
        original.m_bakedIrradianceTextureAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::NoLoad);
        original.m_bakedDistanceTextureAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::NoLoad);
        original.m_bakedProbeDataTextureAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::NoLoad);
        original.m_visualizationEnabled = true;
        original.m_visualizationShowInactiveProbes = true;
        original.m_visualizationSphereRadius = 1.25f;
        original.m_entityId = 123;

        AZStd::vector<char> bytes;
        AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&bytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_XML, &original, GetSerializeContext()));
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        JE::Render::Features::DiffuseProbeGridComponentConfig restored;
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(stream, restored, GetSerializeContext()));

        EXPECT_EQ(restored.m_probeSpacing, original.m_probeSpacing);
        EXPECT_EQ(restored.m_extents, original.m_extents);
        EXPECT_EQ(restored.m_ambientMultiplier, original.m_ambientMultiplier);
        EXPECT_EQ(restored.m_viewBias, original.m_viewBias);
        EXPECT_EQ(restored.m_normalBias, original.m_normalBias);
        EXPECT_EQ(restored.m_numRaysPerProbe, original.m_numRaysPerProbe);
        EXPECT_EQ(restored.m_scrolling, original.m_scrolling);
        EXPECT_EQ(restored.m_edgeBlendIbl, original.m_edgeBlendIbl);
        EXPECT_EQ(restored.m_frameUpdateCount, original.m_frameUpdateCount);
        EXPECT_EQ(restored.m_transparencyMode, original.m_transparencyMode);
        EXPECT_EQ(restored.m_emissiveMultiplier, original.m_emissiveMultiplier);
        EXPECT_EQ(restored.m_editorMode, original.m_editorMode);
        EXPECT_EQ(restored.m_runtimeMode, original.m_runtimeMode);
        EXPECT_EQ(restored.m_bakedIrradianceTextureRelativePath, original.m_bakedIrradianceTextureRelativePath);
        EXPECT_EQ(restored.m_bakedDistanceTextureRelativePath, original.m_bakedDistanceTextureRelativePath);
        EXPECT_EQ(restored.m_bakedProbeDataTextureRelativePath, original.m_bakedProbeDataTextureRelativePath);
        EXPECT_EQ(restored.m_bakedIrradianceTextureAsset.GetId(), original.m_bakedIrradianceTextureAsset.GetId());
        EXPECT_EQ(restored.m_bakedDistanceTextureAsset.GetId(), original.m_bakedDistanceTextureAsset.GetId());
        EXPECT_EQ(restored.m_bakedProbeDataTextureAsset.GetId(), original.m_bakedProbeDataTextureAsset.GetId());
        EXPECT_TRUE(restored.m_bakedIrradianceTextureAsset.GetId().IsValid());
        EXPECT_TRUE(restored.m_bakedDistanceTextureAsset.GetId().IsValid());
        EXPECT_TRUE(restored.m_bakedProbeDataTextureAsset.GetId().IsValid());
        EXPECT_EQ(restored.m_visualizationEnabled, original.m_visualizationEnabled);
        EXPECT_EQ(restored.m_visualizationShowInactiveProbes, original.m_visualizationShowInactiveProbes);
        EXPECT_EQ(restored.m_visualizationSphereRadius, original.m_visualizationSphereRadius);
        EXPECT_EQ(restored.m_entityId, AZ::EntityId::InvalidEntityId);
    }

    TEST_F(DiffuseProbeGridReflectionTests, GlobalIlluminationKeepsQualityLevelVersionOneAndRoundTrips)
    {
        using Config = JE::Render::Features::DiffuseGlobalIlluminationComponentConfig;
        EXPECT_EQ(azrtti_typeid<Config>(), AZ::Uuid("{0D0835D6-6094-4EF8-BEAC-5FF8A4E4C119}"));
        EXPECT_STREQ(Config::RTTI_TypeName(), "DiffuseGlobalIlluminationComponentConfig");
        const auto* data = GetSerializeContext()->FindClassData(azrtti_typeid<Config>());
        ASSERT_NE(data, nullptr);
        EXPECT_EQ(data->m_version, 1);
        size_t fields = 0;
        for (const auto& element : data->m_elements)
        {
            if ((element.m_flags & AZ::SerializeContext::ClassElement::FLG_BASE_CLASS) == 0)
            {
                EXPECT_STREQ(element.m_name, "QualityLevel");
                ++fields;
            }
        }
        EXPECT_EQ(fields, 1);
        Config original;
        EXPECT_EQ(original.m_qualityLevel, JE::Render::Features::DiffuseGlobalIlluminationQualityLevel::Low);
        original.m_qualityLevel = JE::Render::Features::DiffuseGlobalIlluminationQualityLevel::High;
        AZStd::vector<char> bytes;
        AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&bytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_XML, &original, GetSerializeContext()));
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        Config restored;
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(stream, restored, GetSerializeContext()));
        EXPECT_EQ(restored.m_qualityLevel, JE::Render::Features::DiffuseGlobalIlluminationQualityLevel::High);
    }

    class DiffuseProbeGridModeTests : public RPITestFixture
    {
    };

    TEST_F(DiffuseProbeGridModeTests, BakedTexturesAndAutoSelectWorkWithNoRayTracingAndNormalDestruction)
    {
        auto* rhi = AZ::RHI::RHISystemInterface::Get();
        ASSERT_NE(rhi, nullptr);
        ASSERT_EQ(rhi->GetRayTracingSupport(), AZ::RHI::MultiDevice::NoDevices);

        // Same visibility provider as the existing SceneTests fixture. It outlives the scene and grid.
        AZStd::unique_ptr<AzFramework::OctreeSystemComponent> visibility;
        if (!AZ::Interface<AzFramework::IVisibilitySystem>::Get())
        {
            visibility = AZStd::make_unique<AzFramework::OctreeSystemComponent>();
        }
        JE::Render::RPI::SceneDescriptor sceneDescriptor;
        sceneDescriptor.m_nameId = AZ::Name("DPG no-RT mode regression");
        auto scene = JE::Render::RPI::Scene::CreateScene(sceneDescriptor);
        ASSERT_NE(scene, nullptr);
        scene->Activate();

        JE::Render::Features::DiffuseProbeGridRenderData renderData;
        AZ::RHI::BufferPoolDescriptor poolDescriptor;
        poolDescriptor.m_bindFlags = AZ::RHI::BufferBindFlags::ShaderReadWrite;
        renderData.m_bufferPool = aznew AZ::RHI::BufferPool;
        ASSERT_EQ(renderData.m_bufferPool->Init(poolDescriptor), AZ::RHI::ResultCode::Success);
        auto* imageSystem = JE::Render::RPI::ImageSystemInterface::Get();
        ASSERT_NE(imageSystem, nullptr);
        const auto& imagePool = imageSystem->GetSystemAttachmentPool();
        ASSERT_NE(imagePool.get(), nullptr);

        {
            JE::Render::Features::DiffuseProbeGrid grid;
            grid.Init(scene.get(), &renderData);
            grid.SetMode(JE::Render::Features::DiffuseProbeGridMode::Baked);
            EXPECT_EQ(grid.GetMode(), JE::Render::Features::DiffuseProbeGridMode::Baked);
            EXPECT_FALSE(grid.HasValidBakedTextures());

            auto createImage = [&imagePool](const char* name, AZ::RHI::Format format)
            {
                JE::Render::RPI::CreateAttachmentImageRequest request;
                request.m_imageName = AZ::Name(name);
                request.m_imagePool = imagePool.get();
                request.m_imageDescriptor = AZ::RHI::ImageDescriptor::Create2D(
                    AZ::RHI::ImageBindFlags::ShaderReadWrite, 1, 1, format);
                return JE::Render::RPI::AttachmentImage::Create(request);
            };
            JE::Render::Features::DiffuseProbeGridBakedTextures textures;
            textures.m_irradianceImage = createImage("DPG baked irradiance", AZ::RHI::Format::R16G16B16A16_FLOAT);
            textures.m_distanceImage = createImage("DPG baked distance", AZ::RHI::Format::R32G32_FLOAT);
            textures.m_probeDataImage = createImage("DPG baked probe data", AZ::RHI::Format::R16G16B16A16_FLOAT);
            ASSERT_NE(textures.m_irradianceImage.get(), nullptr);
            ASSERT_NE(textures.m_distanceImage.get(), nullptr);
            ASSERT_NE(textures.m_probeDataImage.get(), nullptr);
            grid.SetBakedTextures(textures);
            EXPECT_TRUE(grid.HasValidBakedTextures());
            EXPECT_EQ(grid.GetIrradianceImage(), textures.m_irradianceImage->GetRHIImage());
            EXPECT_EQ(grid.GetDistanceImage(), textures.m_distanceImage->GetRHIImage());
            EXPECT_EQ(grid.GetProbeDataImage(), textures.m_probeDataImage->GetRHIImage());

            grid.SetMode(JE::Render::Features::DiffuseProbeGridMode::AutoSelect);
            EXPECT_EQ(grid.GetMode(), JE::Render::Features::DiffuseProbeGridMode::Baked);
            EXPECT_TRUE(grid.HasValidBakedTextures());
            EXPECT_EQ(grid.GetIrradianceImage(), textures.m_irradianceImage->GetRHIImage());
        }
        // The grid unregisters its cullable before scene/Culling and StubRHI teardown.
        scene->Deactivate();
    }
}
