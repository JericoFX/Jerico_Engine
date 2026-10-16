/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Common/RHI/Factory.h>
#include <Common/RHI/Stubs.h>
#include <Common/RPITestFixture.h>
#include <Common/ErrorMessageFinder.h>
#include <Common/ShaderAssetTestUtils.h>
#include <Material/MaterialAssetTestUtils.h>

#include <Atom/RPI.Public/ColorManagement/TransformColor.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <Atom/RPI.Reflect/Material/MaterialAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialNameContext.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    namespace
    {
        class ReloadBindlessImageView final
            : public StubRHI::ImageView
        {
        public:
            AZ_CLASS_ALLOCATOR(ReloadBindlessImageView, AZ::SystemAllocator);

            explicit ReloadBindlessImageView(uint32_t readIndex)
                : m_readIndex(readIndex)
            {
            }

            uint32_t GetBindlessReadIndex() const override
            {
                return m_readIndex;
            }

        private:
            const uint32_t m_readIndex;
        };
    }

    class MaterialTests
        : public RPITestFixture
    {
    protected:
        Data::Asset<ShaderAsset> m_testMaterialShaderAsset;
        MaterialShaderParameterLayout m_testMaterialShaderParameterLayout;
        AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_testMaterialSrgLayout;
        Data::Asset<MaterialTypeAsset> m_testMaterialTypeAsset;
        Data::Asset<MaterialAsset> m_testMaterialAsset;
        Data::Asset<StreamingImageAsset> m_testImageAsset;
        Data::Asset<AttachmentImageAsset> m_testAttachmentImageAsset;
        Data::Instance<StreamingImage> m_testImage;
        Data::Instance<AttachmentImage> m_testAttachmentImage;

        enum class BindlessReloadScenario
        {
            SingleReload,
            RepeatedReload,
            EditBeforeReload,
            EditAfterReload,
            DestroyBeforeTick
        };
        void CheckBindlessImageReload(BindlessReloadScenario scenario);

        Data::Asset<StreamingImageAsset> CreateTestImageAsset() const
        {
            Data::Asset<StreamingImageAsset> testImageAsset;

            Data::Asset<ImageMipChainAsset> mipChainAsset;
            ImageMipChainAssetCreator mipChainCreator;
            mipChainCreator.Begin(Uuid::CreateRandom(), 1, 1);
            mipChainCreator.BeginMip(RHI::GetImageSubresourceLayout(RHI::Size{ 1,1,1 }, RHI::Format::R8_UNORM));
            uint8_t pixel = 0;
            mipChainCreator.AddSubImage(&pixel, 1);
            mipChainCreator.EndMip();
            mipChainCreator.End(mipChainAsset);

            StreamingImageAssetCreator imageCreator;
            imageCreator.Begin(Uuid::CreateRandom());
            imageCreator.AddMipChainAsset(*mipChainAsset.Get());
            imageCreator.SetFlags(StreamingImageFlags::NotStreamable);
            imageCreator.SetPoolAssetId(ImageSystemInterface::Get()->GetSystemStreamingPool()->GetAssetId());
            imageCreator.End(testImageAsset);

            return testImageAsset;
        }

        Data::Asset<AttachmentImageAsset> CreateAttachmentImageAsset() const
        {
            Data::Asset<AttachmentImageAsset> testImageAsset;

            AttachmentImageAssetCreator imageCreator;
            imageCreator.Begin(Uuid::CreateRandom());
            imageCreator.SetPoolAsset({ImageSystemInterface::Get()->GetSystemAttachmentPool()->GetAssetId(), azrtti_typeid<ResourcePoolAsset>()});
            imageCreator.SetName(Name("testAttachmentImageAsset"), true);
            imageCreator.End(testImageAsset);

            return testImageAsset;
        }

        void SetUp() override
        {
            RPITestFixture::SetUp();

            m_testMaterialSrgLayout = CreateCommonTestMaterialSrgLayout();
            m_testMaterialShaderParameterLayout = CreateCommonTestMaterialShaderParameterLayout();
            m_testMaterialShaderParameterLayout.ConnectParametersToSrg(m_testMaterialSrgLayout.get());
            m_testMaterialShaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout);

            MaterialTypeAssetCreator materialTypeCreator;
            materialTypeCreator.Begin(Uuid::CreateRandom());
            materialTypeCreator.AddShader(m_testMaterialShaderAsset);
            materialTypeCreator.SetMaterialShaderParameterLayout(m_testMaterialShaderParameterLayout);
            AddCommonTestMaterialProperties(materialTypeCreator);
            materialTypeCreator.UpdateShaderParameterConnections();
            materialTypeCreator.SetPropertyValue(Name{ "MyFloat2" }, Vector2{ 10.1f, 10.2f });
            materialTypeCreator.SetPropertyValue(Name{ "MyFloat3" }, Vector3{ 11.1f, 11.2f, 11.3f });
            materialTypeCreator.SetPropertyValue(Name{ "MyFloat4" }, Vector4{ 12.1f, 12.2f, 12.3f, 12.4f });
            materialTypeCreator.SetPropertyValue(Name{ "MyColor" }, Color{ 0.1f, 0.2f, 0.3f, 0.4f });
            materialTypeCreator.SetPropertyValue(Name{ "MyInt" }, -12);
            materialTypeCreator.SetPropertyValue(Name{ "MyUInt" }, 112u);
            materialTypeCreator.SetPropertyValue(Name{ "MyFloat" }, 11.5f);
            materialTypeCreator.SetPropertyValue(Name{ "MyEnum" }, 1u);
            materialTypeCreator.SetPropertyValue(Name{ "MySamplerState" }, GetDefaultSamplerState());
            materialTypeCreator.End(m_testMaterialTypeAsset);

            m_testImageAsset = CreateTestImageAsset();
            m_testImage = StreamingImage::FindOrCreate(m_testImageAsset);

            m_testAttachmentImageAsset = CreateAttachmentImageAsset();
            m_testAttachmentImage = AttachmentImage::FindOrCreate(m_testAttachmentImageAsset);

            MaterialAssetCreator materialCreator;
            materialCreator.Begin(Uuid::CreateRandom(), m_testMaterialTypeAsset);
            materialCreator.SetPropertyValue(Name{ "MyFloat2" }, Vector2{ 0.1f, 0.2f });
            materialCreator.SetPropertyValue(Name{ "MyFloat3" }, Vector3{ 1.1f, 1.2f, 1.3f });
            materialCreator.SetPropertyValue(Name{ "MyFloat4" }, Vector4{ 2.1f, 2.2f, 2.3f, 2.4f });
            materialCreator.SetPropertyValue(Name{ "MyColor" }, Color{ 1.0f, 1.0f, 1.0f, 1.0f });
            materialCreator.SetPropertyValue(Name{ "MyInt" }, -2);
            materialCreator.SetPropertyValue(Name{ "MyUInt" }, 12u);
            materialCreator.SetPropertyValue(Name{ "MyFloat" }, 1.5f);
            materialCreator.SetPropertyValue(Name{ "MyBool" }, true);
            materialCreator.SetPropertyValue(Name{ "MyImage" }, Data::Asset<ImageAsset>(m_testImageAsset));
            materialCreator.SetPropertyValue(Name{ "MyEnum" }, 2u);
            materialCreator.SetPropertyValue(Name{ "MyAttachmentImage" }, Data::Asset<ImageAsset>(m_testAttachmentImageAsset));
            materialCreator.SetPropertyValue(Name{ "MySamplerState" }, GetClampSamplerState());

            materialCreator.End(m_testMaterialAsset);
        }

        void TearDown() override
        {
            m_testMaterialShaderAsset.Reset();
            m_testMaterialSrgLayout = nullptr;
            m_testMaterialShaderParameterLayout = {};
            m_testMaterialTypeAsset.Reset();
            m_testMaterialAsset.Reset();
            m_testImageAsset.Reset();
            m_testImage = nullptr;
            m_testAttachmentImageAsset.Reset();
            m_testAttachmentImage = nullptr;

            RPITestFixture::TearDown();
        }

        void ValidateInitialValuesFromMaterialType(Data::Instance<Material> material)
        {
            // clang-format off
            // Test reading the values directly...

            EXPECT_EQ(material->GetPropertyValue<bool>(material->FindPropertyIndex(Name{ "MyBool" })), false);
            EXPECT_EQ(material->GetPropertyValue<int32_t>(material->FindPropertyIndex(Name{ "MyInt" })), -12);
            EXPECT_EQ(material->GetPropertyValue<uint32_t>(material->FindPropertyIndex(Name{ "MyUInt" })), 112u);
            EXPECT_EQ(material->GetPropertyValue<float>(material->FindPropertyIndex(Name{ "MyFloat" })), 11.5f);
            EXPECT_EQ(material->GetPropertyValue<Vector2>(material->FindPropertyIndex(Name{ "MyFloat2" })), Vector2(10.1f, 10.2f));
            EXPECT_EQ(material->GetPropertyValue<Vector3>(material->FindPropertyIndex(Name{ "MyFloat3" })), Vector3(11.1f, 11.2f, 11.3f));
            EXPECT_EQ(material->GetPropertyValue<Vector4>(material->FindPropertyIndex(Name{ "MyFloat4" })), Vector4(12.1f, 12.2f, 12.3f, 12.4f));
            EXPECT_EQ(material->GetPropertyValue<Color>(material->FindPropertyIndex(Name{ "MyColor" })), Color(0.1f, 0.2f, 0.3f, 0.4f));
            EXPECT_EQ(material->GetPropertyValue<Data::Instance<Image>>(material->FindPropertyIndex(Name{ "MyImage" })), nullptr);
            EXPECT_EQ(material->GetPropertyValue<uint32_t>(material->FindPropertyIndex(Name{ "MyEnum" })), 1u);
            EXPECT_EQ(material->GetPropertyValue<RHI::SamplerState>(material->FindPropertyIndex(Name{ "MySamplerState" })), GetDefaultSamplerState());

            // Dig in to the SRG to make sure the values were applied there as well...

            const RHI::ShaderResourceGroup* srg = material->GetRHIShaderResourceGroup();
            const RHI::ShaderResourceGroupData& srgData = srg->GetData();

            EXPECT_EQ(srgData.GetConstant<bool>(srgData.FindShaderInputConstantIndex(Name{ "m_bool" })), false);
            EXPECT_EQ(srgData.GetConstant<int32_t>(srgData.FindShaderInputConstantIndex(Name{ "m_int" })), -12);
            EXPECT_EQ(srgData.GetConstant<uint32_t>(srgData.FindShaderInputConstantIndex(Name{ "m_uint" })), 112u);
            EXPECT_EQ(srgData.GetConstant<float>(srgData.FindShaderInputConstantIndex(Name{ "m_float" })), 11.5f);
            EXPECT_EQ(srgData.GetConstant<Vector2>(srgData.FindShaderInputConstantIndex(Name{ "m_float2" })), Vector2(10.1f, 10.2f));
            // Currently srgData.GetConstant<Vector3> isn't supported so we check the individual floats
            EXPECT_EQ(srgData.GetConstant<float>(srgData.FindShaderInputConstantIndex(Name{ "m_float3" }), 0), 11.1f);
            EXPECT_EQ(srgData.GetConstant<float>(srgData.FindShaderInputConstantIndex(Name{ "m_float3" }), 1), 11.2f);
            EXPECT_EQ(srgData.GetConstant<float>(srgData.FindShaderInputConstantIndex(Name{ "m_float3" }), 2), 11.3f);
            EXPECT_EQ(srgData.GetConstant<Vector4>(srgData.FindShaderInputConstantIndex(Name{ "m_float4" })), Vector4(12.1f, 12.2f, 12.3f, 12.4f));
            EXPECT_EQ(srgData.GetConstant<Color>(srgData.FindShaderInputConstantIndex(Name{ "m_color" })), TransformColor(Color(0.1f, 0.2f, 0.3f, 0.4f), ColorSpaceId::LinearSRGB, ColorSpaceId::ACEScg));

            EXPECT_EQ(srgData.GetImageView(srgData.FindShaderInputImageIndex(Name{ "m_image" }), 0), nullptr);
            EXPECT_EQ(srgData.GetConstant<uint32_t>(srgData.FindShaderInputConstantIndex(Name{ "m_enum" })), 1u);
            EXPECT_EQ(srgData.GetImageView(srgData.FindShaderInputImageIndex(Name{ "m_attachmentImage" }), 0), nullptr);
            
            auto samplerIndex = srgData.GetConstant<uint32_t>(srgData.FindShaderInputConstantIndex(Name{ "m_samplerIndex" }), 0);
            auto samplerState = MaterialInstanceHandlerInterface::Get()->GetRegisteredTextureSampler(material->GetMaterialTypeId(), material->GetMaterialTypeId(), samplerIndex);
            EXPECT_EQ(samplerState, GetDefaultSamplerState());

            // Make sure the values are also in the shader parameter buffer

            auto paramData = material->GetMaterialShaderParameter();
            auto layout = material->GetAsset()->GetMaterialTypeAsset()->GetMaterialShaderParameterLayout();

            EXPECT_EQ(paramData->GetShaderParameterData<bool>(layout.GetParameterIndex("m_bool")), false);
            EXPECT_EQ(paramData->GetShaderParameterData<int32_t>(layout.GetParameterIndex("m_int")), -12);
            EXPECT_EQ(paramData->GetShaderParameterData<uint32_t>(layout.GetParameterIndex("m_uint")), 112u);
            EXPECT_EQ(paramData->GetShaderParameterData<float>(layout.GetParameterIndex("m_float")), 11.5f);
            EXPECT_EQ(paramData->GetShaderParameterData<Vector2>(layout.GetParameterIndex("m_float2")), Vector2(10.1f, 10.2f));
            EXPECT_EQ(paramData->GetShaderParameterData<Vector3>(layout.GetParameterIndex("m_float3")), Vector3(11.1f, 11.2f, 11.3f));
            EXPECT_EQ(
                paramData->GetShaderParameterData<Vector4>(layout.GetParameterIndex("m_float4")), Vector4(12.1f, 12.2f, 12.3f, 12.4f));
            EXPECT_EQ(
                paramData->GetShaderParameterData<Color>(layout.GetParameterIndex("m_color")),
                TransformColor(Color(0.1f, 0.2f, 0.3f, 0.4f), ColorSpaceId::LinearSRGB, ColorSpaceId::ACEScg));

            EXPECT_EQ(paramData->GetShaderParameterData<uint32_t>(layout.GetParameterIndex("m_image")), 0);
            EXPECT_EQ(paramData->GetShaderParameterData<uint32_t>(layout.GetParameterIndex("m_enum")), 1u);
            EXPECT_EQ(paramData->GetShaderParameterData<int32_t>(layout.GetParameterIndex("m_attachmentImage")), 0);
            EXPECT_EQ(paramData->GetShaderParameterData<uint32_t>(layout.GetParameterIndex("m_samplerIndex")), samplerIndex);
            EXPECT_EQ(paramData->GetShaderParameterData<RHI::SamplerState>(layout.GetParameterIndex("m_samplerIndex")), GetDefaultSamplerState());
        }

        void ValidateInitialValuesFromMaterial(Data::Instance<Material> material)
        {
            // Test reading the values directly...

            EXPECT_EQ(material->GetPropertyValue<bool>(material->FindPropertyIndex(Name{ "MyBool" })), true);
            EXPECT_EQ(material->GetPropertyValue<int32_t>(material->FindPropertyIndex(Name{ "MyInt" })), -2);
            EXPECT_EQ(material->GetPropertyValue<uint32_t>(material->FindPropertyIndex(Name{ "MyUInt" })), 12u);
            EXPECT_EQ(material->GetPropertyValue<float>(material->FindPropertyIndex(Name{ "MyFloat" })), 1.5f);
            EXPECT_EQ(material->GetPropertyValue<Vector2>(material->FindPropertyIndex(Name{ "MyFloat2" })), Vector2(0.1f, 0.2f));
            EXPECT_EQ(material->GetPropertyValue<Vector3>(material->FindPropertyIndex(Name{ "MyFloat3" })), Vector3(1.1f, 1.2f, 1.3f));
            EXPECT_EQ(material->GetPropertyValue<Vector4>(material->FindPropertyIndex(Name{ "MyFloat4" })), Vector4(2.1f, 2.2f, 2.3f, 2.4f));
            EXPECT_EQ(material->GetPropertyValue<Color>(material->FindPropertyIndex(Name{ "MyColor" })), Color(1.0f, 1.0f, 1.0f, 1.0f));
            EXPECT_EQ(material->GetPropertyValue<Data::Instance<Image>>(material->FindPropertyIndex(Name{ "MyImage" })), m_testImage);
            EXPECT_EQ(material->GetPropertyValue<uint32_t>(material->FindPropertyIndex(Name{ "MyEnum" })), 2u);
            EXPECT_EQ(material->GetPropertyValue<Data::Instance<Image>>(material->FindPropertyIndex(Name{ "MyAttachmentImage" })), m_testAttachmentImage);
            EXPECT_EQ(material->GetPropertyValue<RHI::SamplerState>(material->FindPropertyIndex(Name{ "MySamplerState" })), GetClampSamplerState());

            // Dig in to the SRG to make sure the values were applied there as well...

            const RHI::ShaderResourceGroup* srg = material->GetRHIShaderResourceGroup();
            const RHI::ShaderResourceGroupData& srgData = srg->GetData();

            EXPECT_EQ(srgData.GetConstant<bool>(srgData.FindShaderInputConstantIndex(Name{ "m_bool" })), true);
            EXPECT_EQ(srgData.GetConstant<int32_t>(srgData.FindShaderInputConstantIndex(Name{ "m_int" })), -2);
            EXPECT_EQ(srgData.GetConstant<uint32_t>(srgData.FindShaderInputConstantIndex(Name{ "m_uint" })), 12u);
            EXPECT_EQ(srgData.GetConstant<float>(srgData.FindShaderInputConstantIndex(Name{ "m_float" })), 1.5f);
            EXPECT_EQ(srgData.GetConstant<Vector2>(srgData.FindShaderInputConstantIndex(Name{ "m_float2" })), Vector2(0.1f, 0.2f));
            // Currently srgData.GetConstant<Vector3> isn't supported so we check the individual floats
            EXPECT_EQ(srgData.GetConstant<float>(srgData.FindShaderInputConstantIndex(Name{ "m_float3" }), 0), 1.1f);
            EXPECT_EQ(srgData.GetConstant<float>(srgData.FindShaderInputConstantIndex(Name{ "m_float3" }), 1), 1.2f);
            EXPECT_EQ(srgData.GetConstant<float>(srgData.FindShaderInputConstantIndex(Name{ "m_float3" }), 2), 1.3f);
            EXPECT_EQ(srgData.GetConstant<Vector4>(srgData.FindShaderInputConstantIndex(Name{ "m_float4" })), Vector4(2.1f, 2.2f, 2.3f, 2.4f));
            EXPECT_EQ(srgData.GetConstant<Color>(srgData.FindShaderInputConstantIndex(Name{ "m_color" })), TransformColor(Color(1.0f, 1.0f, 1.0f, 1.0f), ColorSpaceId::LinearSRGB, ColorSpaceId::ACEScg));

            EXPECT_EQ(srgData.GetImageView(srgData.FindShaderInputImageIndex(Name{ "m_image" }), 0), m_testImage->GetImageView());
            EXPECT_EQ(srgData.GetConstant<uint32_t>(srgData.FindShaderInputConstantIndex(Name{ "m_enum" })), 2u);
            EXPECT_EQ(srgData.GetImageView(srgData.FindShaderInputImageIndex(Name{ "m_attachmentImage" }), 0), m_testAttachmentImage->GetImageView());

            auto samplerIndex = srgData.GetConstant<uint32_t>(srgData.FindShaderInputConstantIndex(Name{ "m_samplerIndex" }), 0);
            auto samplerState = MaterialInstanceHandlerInterface::Get()->GetRegisteredTextureSampler(material->GetMaterialTypeId(), material->GetMaterialTypeId(), samplerIndex);
            EXPECT_EQ(samplerState, GetClampSamplerState());

            // Make sure the values are also in the shader parameter buffer

            auto paramData = material->GetMaterialShaderParameter();
            auto layout = material->GetAsset()->GetMaterialTypeAsset()->GetMaterialShaderParameterLayout();

            EXPECT_EQ(paramData->GetShaderParameterData<bool>(layout.GetParameterIndex("m_bool")), true);
            EXPECT_EQ(paramData->GetShaderParameterData<int32_t>(layout.GetParameterIndex("m_int")), -2);
            EXPECT_EQ(paramData->GetShaderParameterData<uint32_t>(layout.GetParameterIndex("m_uint")), 12u);
            EXPECT_EQ(paramData->GetShaderParameterData<float>(layout.GetParameterIndex("m_float")), 1.5f);
            EXPECT_EQ(paramData->GetShaderParameterData<Vector2>(layout.GetParameterIndex("m_float2")), Vector2(0.1f, 0.2f));
            EXPECT_EQ(paramData->GetShaderParameterData<Vector3>(layout.GetParameterIndex("m_float3")), Vector3(1.1f, 1.2f, 1.3f));
            EXPECT_EQ(paramData->GetShaderParameterData<Vector4>(layout.GetParameterIndex("m_float4")), Vector4(2.1f, 2.2f, 2.3f, 2.4f));
            EXPECT_EQ(
                paramData->GetShaderParameterData<Color>(layout.GetParameterIndex("m_color")),
                TransformColor(Color(1.0f, 1.0f, 1.0f, 1.0f), ColorSpaceId::LinearSRGB, ColorSpaceId::ACEScg));

            EXPECT_EQ(
                paramData->GetShaderParameterData<uint32_t>(layout.GetParameterIndex("m_image")),
                m_testImage->GetImageView()->GetDeviceImageView(0)->GetBindlessReadIndex());
            EXPECT_EQ(paramData->GetShaderParameterData<uint32_t>(layout.GetParameterIndex("m_enum")), 2u);
            EXPECT_EQ(
                paramData->GetShaderParameterData<uint32_t>(layout.GetParameterIndex("m_attachmentImage")),
                m_testAttachmentImage->GetImageView()->GetDeviceImageView(0)->GetBindlessReadIndex());
        
            EXPECT_EQ(paramData->GetShaderParameterData<uint32_t>(layout.GetParameterIndex("m_samplerIndex")), samplerIndex);
            EXPECT_EQ(paramData->GetShaderParameterData<RHI::SamplerState>(layout.GetParameterIndex("m_samplerIndex")), GetClampSamplerState());

        }

        //! Provides write access to private material asset property values, primarily for simulating
        //! MaterialAsset hot reload.
        MaterialPropertyValue& AccessMaterialAssetPropertyValue(Data::Asset<MaterialAsset> materialAsset, Name propertyName)
        {
            return materialAsset->m_propertyValues[materialAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyName).GetIndex()];
        }
    };

    //! Copies the value of a material property to an internal material pipelien property
    class SetInternalPropertyFunctor final
        : public AZ::RPI::MaterialFunctor
    {
        friend class SetInternalPropertyFunctorSourceData;
    public:
        AZ_CLASS_ALLOCATOR(SetInternalPropertyFunctor, SystemAllocator)
        using AZ::RPI::MaterialFunctor::Process;
        void Process(AZ::RPI::MaterialFunctorAPI::RuntimeContext& context) override
        {
            const MaterialPropertyValue value = context.GetMaterialPropertyValue(m_inputPropertyIndex);
            context.SetInternalMaterialPropertyValue(m_outputPropertyName, value);
        }

    private:
        AZ::RPI::MaterialPropertyIndex m_inputPropertyIndex;
        AZ::Name m_outputPropertyName;
    };

    class SetInternalPropertyFunctorSourceData final
        : public AZ::RPI::MaterialFunctorSourceData
    {
    public:
        AZ_CLASS_ALLOCATOR(SetInternalPropertyFunctorSourceData, AZ::SystemAllocator)
        using MaterialFunctorSourceData::CreateFunctor;
        FunctorResult CreateFunctor(const RuntimeContext& context) const override
        {
            Ptr<SetInternalPropertyFunctor> functor = aznew SetInternalPropertyFunctor;

            functor->m_inputPropertyIndex = context.FindMaterialPropertyIndex(Name{m_inputPropertyName});
            functor->m_outputPropertyName = m_outputPropertyName;

            AddMaterialPropertyDependency(functor, functor->m_inputPropertyIndex);

            SetFunctorShaderParameter(functor, GetMaterialShaderParameters(context.GetNameContext()));

            return Success(Ptr<MaterialFunctor>(functor));
        }

        AZ::Name m_inputPropertyName;
        AZ::Name m_outputPropertyName;
    };

    //! Sets the shader enable state based on a bool material property
    class ShaderEnablePipelineFunctor final
        : public AZ::RPI::MaterialFunctor
    {
    public:
        AZ_CLASS_ALLOCATOR(ShaderEnablePipelineFunctor, SystemAllocator)
        using AZ::RPI::MaterialFunctor::Process;
        void Process(AZ::RPI::MaterialFunctorAPI::PipelineRuntimeContext& context) override
        {
            const bool enable = context.GetMaterialPropertyValue(m_enablePropertyIndex).GetValue<bool>();
            context.SetShaderEnabled(m_shaderTag, enable);
        }

        AZ::RPI::MaterialPropertyIndex m_enablePropertyIndex;
        AZ::Name m_shaderTag;
    };

    class ShaderEnablePipelineFunctorSourceData final
        : public AZ::RPI::MaterialFunctorSourceData
    {
    public:
        AZ_CLASS_ALLOCATOR(ShaderEnablePipelineFunctorSourceData, AZ::SystemAllocator)
        using MaterialFunctorSourceData::CreateFunctor;
        FunctorResult CreateFunctor(const RuntimeContext& context) const override
        {
            Ptr<ShaderEnablePipelineFunctor> functor = aznew ShaderEnablePipelineFunctor;

            functor->m_enablePropertyIndex = context.FindMaterialPropertyIndex(Name{m_enablePropertyName});
            functor->m_shaderTag = m_shaderTag;

            AddMaterialPropertyDependency(functor, functor->m_enablePropertyIndex);

            SetFunctorShaderParameter(functor, GetMaterialShaderParameters(context.GetNameContext()));

            return Success(Ptr<MaterialFunctor>(functor));
        }

        AZ::Name m_enablePropertyName;
        AZ::Name m_shaderTag;
    };

    void MaterialTests::CheckBindlessImageReload(BindlessReloadScenario scenario)
    {
        auto& factory = static_cast<StubRHI::Factory&>(RHI::Factory::Get());
        using ImageViewCreator = decltype(factory.m_createImageViewOverride);
        auto previousCreator = AZStd::move(factory.m_createImageViewOverride);
        auto restoreCreator = [&previousCreator](ImageViewCreator* creator)
        {
            *creator = AZStd::move(previousCreator);
        };
        uint32_t nextReadIndex = 37;
        AZStd::unique_ptr<ImageViewCreator, decltype(restoreCreator)> restoreFactory(
            &factory.m_createImageViewOverride, restoreCreator);
        factory.m_createImageViewOverride = [&nextReadIndex]() -> RHI::Ptr<RHI::DeviceImageView>
        {
            return aznew ReloadBindlessImageView(nextReadIndex++);
        };

        const auto createImageAsset = [](const Data::AssetId& assetId)
        {
            Data::Asset<ImageMipChainAsset> mipChain;
            ImageMipChainAssetCreator mipCreator;
            mipCreator.Begin(Uuid::CreateRandom(), 1, 1);
            mipCreator.BeginMip(RHI::GetImageSubresourceLayout(RHI::Size{ 1, 1, 1 }, RHI::Format::R8G8B8A8_UNORM));
            const uint8_t pixel[] = { 32, 64, 128, 255 };
            mipCreator.AddSubImage(pixel, sizeof(pixel));
            mipCreator.EndMip();
            EXPECT_TRUE(mipCreator.End(mipChain));
            StreamingImageAssetCreator creator;
            creator.Begin(assetId);
            creator.SetImageDescriptor(RHI::ImageDescriptor::Create2D(
                RHI::ImageBindFlags::ShaderRead, 1, 1, RHI::Format::R8G8B8A8_UNORM));
            creator.AddMipChainAsset(*mipChain.Get());
            creator.SetFlags(StreamingImageFlags::NotStreamable);
            creator.SetPoolAssetId(ImageSystemInterface::Get()->GetSystemStreamingPool()->GetAssetId());
            Data::Asset<StreamingImageAsset> result;
            EXPECT_TRUE(creator.End(result));
            return result;
        };
        auto imageAsset = createImageAsset(Data::AssetId(Uuid::CreateRandom()));
        ASSERT_TRUE(imageAsset.IsReady());
        auto image = StreamingImage::FindOrCreate(imageAsset);
        ASSERT_NE(image.get(), nullptr);
        auto originalView = image->GetImageView()->GetDeviceImageView(0);
        ASSERT_NE(originalView.get(), nullptr);
        const uint32_t originalIndex = originalView->GetBindlessReadIndex();
        ASSERT_NE(originalIndex, RHI::DeviceImageView::InvalidBindlessIndex);

        auto srgLayout = RHI::ShaderResourceGroupLayout::Create();
        srgLayout->SetName(Name("MaterialSrg"));
        srgLayout->SetBindingSlot(SrgBindingSlot::Material);
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name("m_image"), 0, 4, 0, 0 });
        ASSERT_TRUE(srgLayout->Finalize());
        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), srgLayout);
        ASSERT_TRUE(shaderAsset.IsReady());
        MaterialShaderParameterLayout parameterLayout;
        parameterLayout.AddMaterialParameter<uint32_t>("m_materialType", true);
        parameterLayout.AddMaterialParameter<uint32_t>("m_materialInstance", true);
        parameterLayout.AddMaterialParameter<Data::Asset<Image>>("m_image");
        parameterLayout.FinalizeLayout();
        MaterialTypeAssetCreator typeCreator;
        typeCreator.Begin(Uuid::CreateRandom());
        typeCreator.AddShader(shaderAsset);
        typeCreator.SetMaterialShaderParameterLayout(parameterLayout);
        typeCreator.BeginMaterialProperty(Name("TestImage"), MaterialPropertyDataType::Image);
        typeCreator.ConnectMaterialPropertyToShaderParameter(Name("m_image"));
        typeCreator.EndMaterialProperty();
        typeCreator.UpdateShaderParameterConnections();
        Data::Asset<MaterialTypeAsset> typeAsset;
        ASSERT_TRUE(typeCreator.End(typeAsset));
        MaterialAssetCreator materialCreator;
        materialCreator.Begin(Uuid::CreateRandom(), typeAsset);
        materialCreator.SetPropertyValue(Name("TestImage"), Data::Asset<ImageAsset>(imageAsset));
        Data::Asset<MaterialAsset> materialAsset;
        ASSERT_TRUE(materialCreator.End(materialAsset));
        auto material = Material::FindOrCreate(materialAsset);
        ASSERT_NE(material.get(), nullptr);
        auto parameters = material->GetMaterialShaderParameter();
        ASSERT_NE(parameters, nullptr);
        const auto parameterIndex = typeAsset->GetMaterialShaderParameterLayout().GetParameterIndex("m_image");
        ASSERT_TRUE(parameterIndex.IsValid());
        const auto* descriptor = typeAsset->GetMaterialShaderParameterLayout().GetDescriptor(parameterIndex);
        ASSERT_NE(descriptor, nullptr);
        ASSERT_TRUE(descriptor->m_isBindlessReadIndex);
        ASSERT_TRUE(AZStd::holds_alternative<RHI::ShaderInputConstantIndex>(descriptor->m_srgInputIndex));
        ASSERT_TRUE(material->Compile());
        MaterialInstanceHandlerInterface::Get()->Compile();
        ProcessQueuedSrgCompilations(shaderAsset, srgLayout->GetName());
        auto deviceSrg = material->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(0);
        ASSERT_NE(deviceSrg.get(), nullptr);
        const auto constantIndex = deviceSrg->GetData().FindShaderInputConstantIndex(Name("m_image"));
        ASSERT_TRUE(constantIndex.IsValid());
        ASSERT_EQ(parameters->GetShaderParameterData<uint32_t>(parameterIndex), originalIndex);
        ASSERT_EQ(deviceSrg->GetData().GetConstant<uint32_t>(constantIndex), originalIndex);

        const auto handlersBeforeReload = SystemTickBus::GetTotalNumOfEventHandlers();
        const bool hasManualEdit = scenario == BindlessReloadScenario::EditBeforeReload ||
            scenario == BindlessReloadScenario::EditAfterReload;
        Data::Instance<StreamingImage> manualImage;
        uint32_t manualIndex = RHI::DeviceImageView::InvalidBindlessIndex;
        if (hasManualEdit)
        {
            auto manualAsset = createImageAsset(Data::AssetId(Uuid::CreateRandom()));
            ASSERT_TRUE(manualAsset.IsReady());
            manualImage = StreamingImage::FindOrCreate(manualAsset);
            ASSERT_NE(manualImage.get(), nullptr);
            auto manualView = manualImage->GetImageView()->GetDeviceImageView(0);
            ASSERT_NE(manualView.get(), nullptr);
            manualIndex = manualView->GetBindlessReadIndex();
            ASSERT_NE(manualIndex, RHI::DeviceImageView::InvalidBindlessIndex);
            ASSERT_NE(manualIndex, originalIndex);
            if (scenario == BindlessReloadScenario::EditBeforeReload)
            {
                ASSERT_TRUE(material->SetPropertyValue<Data::Instance<Image>>(
                    material->FindPropertyIndex(Name("TestImage")), manualImage));
                ASSERT_TRUE(material->NeedsCompile());
            }
        }

        auto replacementAsset = createImageAsset(imageAsset.GetId());
        ASSERT_TRUE(replacementAsset.IsReady());
        ASSERT_NE(replacementAsset.Get(), imageAsset.Get());
        auto replacementView = image->GetImageView()->GetDeviceImageView(0);
        ASSERT_NE(replacementView.get(), nullptr);
        ASSERT_NE(replacementView.get(), originalView.get());
        uint32_t replacementIndex = replacementView->GetBindlessReadIndex();
        ASSERT_NE(replacementIndex, RHI::DeviceImageView::InvalidBindlessIndex);
        ASSERT_NE(replacementIndex, originalIndex);
        if (scenario == BindlessReloadScenario::RepeatedReload)
        {
            auto firstReplacementView = replacementView;
            const auto firstReplacementIndex = replacementIndex;
            replacementAsset = createImageAsset(imageAsset.GetId());
            ASSERT_TRUE(replacementAsset.IsReady());
            replacementView = image->GetImageView()->GetDeviceImageView(0);
            ASSERT_NE(replacementView.get(), nullptr);
            ASSERT_NE(replacementView.get(), firstReplacementView.get());
            replacementIndex = replacementView->GetBindlessReadIndex();
            ASSERT_NE(replacementIndex, RHI::DeviceImageView::InvalidBindlessIndex);
            ASSERT_NE(replacementIndex, originalIndex);
            ASSERT_NE(replacementIndex, firstReplacementIndex);
        }
        if (scenario == BindlessReloadScenario::EditAfterReload)
        {
            ASSERT_TRUE(material->SetPropertyValue<Data::Instance<Image>>(
                material->FindPropertyIndex(Name("TestImage")), manualImage));
            ASSERT_TRUE(material->NeedsCompile());
        }
        RHI::ResourceInvalidateBus::ExecuteQueuedEvents();
        if (scenario == BindlessReloadScenario::DestroyBeforeTick)
        {
            EXPECT_EQ(SystemTickBus::GetTotalNumOfEventHandlers(), handlersBeforeReload + 1);
            material = nullptr;
            EXPECT_EQ(SystemTickBus::GetTotalNumOfEventHandlers(), handlersBeforeReload);
            SystemTickBus::Broadcast(&SystemTickBus::Events::OnSystemTick);
            SystemTickBus::Broadcast(&SystemTickBus::Events::OnSystemTick);
            EXPECT_EQ(SystemTickBus::GetTotalNumOfEventHandlers(), handlersBeforeReload);
            EXPECT_EQ(image->GetImageView()->GetDeviceImageView(0).get(), replacementView.get());
            return;
        }
        SystemTickBus::Broadcast(&SystemTickBus::Events::OnSystemTick);
        MaterialInstanceHandlerInterface::Get()->Compile();
        ProcessQueuedSrgCompilations(shaderAsset, srgLayout->GetName());

        if (hasManualEdit)
        {
            EXPECT_TRUE(material->NeedsCompile());
            EXPECT_EQ(material->GetPropertyValue<Data::Instance<Image>>(material->FindPropertyIndex(Name("TestImage"))).get(), manualImage.get());
            EXPECT_EQ(parameters->GetShaderParameterData<uint32_t>(parameterIndex), originalIndex);
            EXPECT_EQ(deviceSrg->GetData().GetConstant<uint32_t>(constantIndex), originalIndex);
            EXPECT_EQ(SystemTickBus::GetTotalNumOfEventHandlers(), handlersBeforeReload);
            ASSERT_TRUE(material->Compile());
            MaterialInstanceHandlerInterface::Get()->Compile();
            ProcessQueuedSrgCompilations(shaderAsset, srgLayout->GetName());
            EXPECT_FALSE(material->NeedsCompile());
            EXPECT_EQ(material->GetPropertyValue<Data::Instance<Image>>(material->FindPropertyIndex(Name("TestImage"))).get(), manualImage.get());
            EXPECT_EQ(parameters->GetShaderParameterData<uint32_t>(parameterIndex), manualIndex);
            EXPECT_EQ(deviceSrg->GetData().GetConstant<uint32_t>(constantIndex), manualIndex);
            return;
        }
        EXPECT_FALSE(material->NeedsCompile());
        EXPECT_EQ(SystemTickBus::GetTotalNumOfEventHandlers(), handlersBeforeReload);
        EXPECT_EQ(parameters->GetShaderParameterData<uint32_t>(parameterIndex), replacementIndex);
        EXPECT_EQ(deviceSrg->GetData().GetConstant<uint32_t>(constantIndex), replacementIndex);
        EXPECT_EQ(material->GetPropertyValue<Data::Instance<Image>>(material->FindPropertyIndex(Name("TestImage"))).get(), image.get());
        EXPECT_TRUE(imageAsset.IsReady());
    }

    TEST_F(MaterialTests, BindlessImageReloadPublishesReplacementIndexWithoutPropertyEdit)
    {
        ASSERT_NO_FATAL_FAILURE(CheckBindlessImageReload(BindlessReloadScenario::SingleReload));
    }

    TEST_F(MaterialTests, BindlessImageDoubleReloadPublishesLatestIndexOnSystemTick)
    {
        ASSERT_NO_FATAL_FAILURE(CheckBindlessImageReload(BindlessReloadScenario::RepeatedReload));
    }

    TEST_F(MaterialTests, ImageReloadPreservesManualEditPendingBeforeReload)
    {
        ASSERT_NO_FATAL_FAILURE(CheckBindlessImageReload(BindlessReloadScenario::EditBeforeReload));
    }

    TEST_F(MaterialTests, ImageReloadPreservesManualEditMadeAfterReload)
    {
        ASSERT_NO_FATAL_FAILURE(CheckBindlessImageReload(BindlessReloadScenario::EditAfterReload));
    }

    TEST_F(MaterialTests, DestroyMaterialWithImageReloadPendingDisconnectsSystemTick)
    {
        ASSERT_NO_FATAL_FAILURE(CheckBindlessImageReload(BindlessReloadScenario::DestroyBeforeTick));
    }

    TEST_F(MaterialTests, MaterialPublicationWaitsForExplicitCompile)
    {
        auto material = Material::FindOrCreate(m_testMaterialAsset);
        ASSERT_NE(material.get(), nullptr);
        ASSERT_TRUE(m_testMaterialAsset.IsReady());
        MaterialInstanceHandlerInterface::Get()->Compile();
        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        auto parameters = material->GetMaterialShaderParameter();
        ASSERT_NE(parameters, nullptr);
        const auto parameterIndex = m_testMaterialTypeAsset->GetMaterialShaderParameterLayout().GetParameterIndex("m_float");
        ASSERT_TRUE(parameterIndex.IsValid());
        auto deviceSrg = material->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(0);
        ASSERT_NE(deviceSrg.get(), nullptr);
        const auto constantIndex = deviceSrg->GetData().FindShaderInputConstantIndex(Name("m_float"));
        ASSERT_TRUE(constantIndex.IsValid());
        ASSERT_EQ(parameters->GetShaderParameterData<float>(parameterIndex), 1.5f);
        ASSERT_EQ(deviceSrg->GetData().GetConstant<float>(constantIndex), 1.5f);
        ASSERT_TRUE(material->SetPropertyValue<float>(material->FindPropertyIndex(Name("MyFloat")), 2.5f));
        ASSERT_TRUE(material->NeedsCompile());

        MaterialInstanceHandlerInterface::Get()->Compile();
        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        EXPECT_TRUE(material->NeedsCompile());
        EXPECT_EQ(parameters->GetShaderParameterData<float>(parameterIndex), 1.5f);
        EXPECT_EQ(deviceSrg->GetData().GetConstant<float>(constantIndex), 1.5f);

        ASSERT_TRUE(material->Compile());
        MaterialInstanceHandlerInterface::Get()->Compile();
        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        EXPECT_FALSE(material->NeedsCompile());
        EXPECT_EQ(parameters->GetShaderParameterData<float>(parameterIndex), 2.5f);
        EXPECT_EQ(deviceSrg->GetData().GetConstant<float>(constantIndex), 2.5f);
    }

    TEST_F(MaterialTests, TestCreateVsFindOrCreate)
    {
        Data::Instance<Material> materialInstance1 = Material::FindOrCreate(m_testMaterialAsset);
        Data::Instance<Material> materialInstance2 = Material::FindOrCreate(m_testMaterialAsset);
        Data::Instance<Material> materialInstance3 = Material::Create(m_testMaterialAsset);
        Data::Instance<Material> materialInstance4 = Material::Create(m_testMaterialAsset);

        EXPECT_TRUE(materialInstance1);
        EXPECT_TRUE(materialInstance2);
        EXPECT_TRUE(materialInstance3);
        EXPECT_TRUE(materialInstance4);

        // Instances created via FindOrCreate should be the same object

        EXPECT_EQ(materialInstance1, materialInstance2);
        EXPECT_NE(materialInstance1, materialInstance3);
        EXPECT_NE(materialInstance1, materialInstance4);
        EXPECT_NE(materialInstance3, materialInstance4);
    }

    TEST_F(MaterialTests, TestInitialValuesFromMaterial)
    {
        Data::Instance<Material> material = Material::FindOrCreate(m_testMaterialAsset);
        MaterialInstanceHandlerInterface::Get()->Compile();
        ValidateInitialValuesFromMaterial(material);
    }

    TEST_F(MaterialTests, TestSetPropertyValue)
    {
        Data::Instance<Material> material = Material::FindOrCreate(m_testMaterialAsset);

        Data::Asset<StreamingImageAsset> otherTestImageAsset;
        Data::Instance<StreamingImage> otherTestImage;
        otherTestImageAsset = CreateTestImageAsset();
        otherTestImage = StreamingImage::FindOrCreate(otherTestImageAsset);

        auto samplerState = AZ::RHI::SamplerState::Create(RHI::FilterMode::Point, RHI::FilterMode::Point, RHI::AddressMode::Border, RHI::BorderColor::OpaqueWhite);

        EXPECT_TRUE(material->SetPropertyValue<bool>(material->FindPropertyIndex(Name{ "MyBool" }), false));
        EXPECT_TRUE(material->SetPropertyValue<int32_t>(material->FindPropertyIndex(Name{ "MyInt" }), -5));
        EXPECT_TRUE(material->SetPropertyValue<uint32_t>(material->FindPropertyIndex(Name{ "MyUInt" }), 123u));
        EXPECT_TRUE(material->SetPropertyValue<float>(material->FindPropertyIndex(Name{ "MyFloat" }), 2.5f));
        EXPECT_TRUE(material->SetPropertyValue<Vector2>(material->FindPropertyIndex(Name{ "MyFloat2" }), Vector2(10.1f, 10.2f)));
        EXPECT_TRUE(material->SetPropertyValue<Vector3>(material->FindPropertyIndex(Name{ "MyFloat3" }), Vector3(11.1f, 11.2f, 11.3f)));
        EXPECT_TRUE(material->SetPropertyValue<Vector4>(material->FindPropertyIndex(Name{ "MyFloat4" }), Vector4(12.1f, 12.2f, 12.3f, 12.4f)));
        EXPECT_TRUE(material->SetPropertyValue<Color>(material->FindPropertyIndex(Name{ "MyColor" }), Color(0.1f, 0.2f, 0.3f, 0.4f)));
        EXPECT_TRUE(material->SetPropertyValue<Data::Instance<Image>>(material->FindPropertyIndex(Name{ "MyImage" }), otherTestImage));
        EXPECT_TRUE(material->SetPropertyValue<uint32_t>(material->FindPropertyIndex(Name{ "MyEnum" }), 3u));
        EXPECT_TRUE(material->SetPropertyValue<RHI::SamplerState>(material->FindPropertyIndex(Name{ "MySamplerState" }), samplerState));

        // Test reading the values directly...

        EXPECT_EQ(material->GetPropertyValue<bool>(material->FindPropertyIndex(Name{ "MyBool" })), false);
        EXPECT_EQ(material->GetPropertyValue<int32_t>(material->FindPropertyIndex(Name{ "MyInt" })), -5);
        EXPECT_EQ(material->GetPropertyValue<uint32_t>(material->FindPropertyIndex(Name{ "MyUInt" })), 123u);
        EXPECT_EQ(material->GetPropertyValue<float>(material->FindPropertyIndex(Name{ "MyFloat" })), 2.5f);
        EXPECT_EQ(material->GetPropertyValue<Vector2>(material->FindPropertyIndex(Name{ "MyFloat2" })), Vector2(10.1f, 10.2f));
        EXPECT_EQ(material->GetPropertyValue<Vector3>(material->FindPropertyIndex(Name{ "MyFloat3" })), Vector3(11.1f, 11.2f, 11.3f));
        EXPECT_EQ(material->GetPropertyValue<Vector4>(material->FindPropertyIndex(Name{ "MyFloat4" })), Vector4(12.1f, 12.2f, 12.3f, 12.4f));
        EXPECT_EQ(material->GetPropertyValue<Color>(material->FindPropertyIndex(Name{ "MyColor" })), Color(0.1f, 0.2f, 0.3f, 0.4f));
        EXPECT_EQ(material->GetPropertyValue<Data::Instance<Image>>(material->FindPropertyIndex(Name{ "MyImage" })), otherTestImage);
        EXPECT_EQ(material->GetPropertyValue<uint32_t>(material->FindPropertyIndex(Name{ "MyEnum" })), 3u);
        EXPECT_EQ(material->GetPropertyValue<RHI::SamplerState>(material->FindPropertyIndex(Name{ "MySamplerState" })), samplerState);

        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        // Dig in to the SRG to make sure the values were applied there as well...

        const RHI::ShaderResourceGroup* srg = material->GetRHIShaderResourceGroup();
        const RHI::ShaderResourceGroupData& srgData = srg->GetData();

        EXPECT_EQ(srgData.GetConstant<bool>(srgData.FindShaderInputConstantIndex(Name{ "m_bool" })), false);
        EXPECT_EQ(srgData.GetConstant<int32_t>(srgData.FindShaderInputConstantIndex(Name{ "m_int" })), -5);
        EXPECT_EQ(srgData.GetConstant<uint32_t>(srgData.FindShaderInputConstantIndex(Name{ "m_uint" })), 123u);
        EXPECT_EQ(srgData.GetConstant<float>(srgData.FindShaderInputConstantIndex(Name{ "m_float" })), 2.5f);
        EXPECT_EQ(srgData.GetConstant<Vector2>(srgData.FindShaderInputConstantIndex(Name{ "m_float2" })), Vector2(10.1f, 10.2f));
        // Currently srgData.GetConstant<Vector3> isn't supported so we check the individual floats
        EXPECT_EQ(srgData.GetConstant<float>(srgData.FindShaderInputConstantIndex(Name{ "m_float3" }), 0), 11.1f);
        EXPECT_EQ(srgData.GetConstant<float>(srgData.FindShaderInputConstantIndex(Name{ "m_float3" }), 1), 11.2f);
        EXPECT_EQ(srgData.GetConstant<float>(srgData.FindShaderInputConstantIndex(Name{ "m_float3" }), 2), 11.3f);
        EXPECT_EQ(srgData.GetConstant<Vector4>(srgData.FindShaderInputConstantIndex(Name{ "m_float4" })), Vector4(12.1f, 12.2f, 12.3f, 12.4f));
        EXPECT_EQ(srgData.GetConstant<Color>(srgData.FindShaderInputConstantIndex(Name{ "m_color" })), TransformColor(Color(0.1f, 0.2f, 0.3f, 0.4f), ColorSpaceId::LinearSRGB, ColorSpaceId::ACEScg));

        EXPECT_EQ(srgData.GetImageView(srgData.FindShaderInputImageIndex(Name{ "m_image" }), 0), otherTestImage->GetImageView());
        EXPECT_EQ(srgData.GetConstant<uint32_t>(srgData.FindShaderInputConstantIndex(Name{ "m_enum" })), 3u);

        auto samplerIndex = srgData.GetConstant<uint32_t>(srgData.FindShaderInputConstantIndex(Name{ "m_samplerIndex" }), 0);
        EXPECT_EQ(samplerIndex, 2);

        auto registeredSamplerState = MaterialInstanceHandlerInterface::Get()->GetRegisteredTextureSampler(material->GetMaterialTypeId(), material->GetMaterialInstanceId(), samplerIndex);
        EXPECT_EQ(registeredSamplerState, samplerState);

        auto paramData = material->GetMaterialShaderParameter();
        auto layout = material->GetAsset()->GetMaterialTypeAsset()->GetMaterialShaderParameterLayout();

        EXPECT_EQ(paramData->GetShaderParameterData<bool>(layout.GetParameterIndex("m_bool")), false);
        EXPECT_EQ(paramData->GetShaderParameterData<int32_t>(layout.GetParameterIndex("m_int")), -5);
        EXPECT_EQ(paramData->GetShaderParameterData<uint32_t>(layout.GetParameterIndex("m_uint")), 123u);
        EXPECT_EQ(paramData->GetShaderParameterData<float>(layout.GetParameterIndex("m_float")), 2.5f);
        EXPECT_EQ(paramData->GetShaderParameterData<Vector2>(layout.GetParameterIndex("m_float2")), Vector2(10.1f, 10.2f));
        EXPECT_EQ(paramData->GetShaderParameterData<Vector3>(layout.GetParameterIndex("m_float3")), Vector3(11.1f, 11.2f, 11.3f));
        EXPECT_EQ(paramData->GetShaderParameterData<Vector4>(layout.GetParameterIndex("m_float4")), Vector4(12.1f, 12.2f, 12.3f, 12.4f));
        EXPECT_EQ(
            paramData->GetShaderParameterData<Color>(layout.GetParameterIndex("m_color")),
            TransformColor(Color(0.1f, 0.2f, 0.3f, 0.4f), ColorSpaceId::LinearSRGB, ColorSpaceId::ACEScg));

        EXPECT_EQ(
            paramData->GetShaderParameterData<int32_t>(layout.GetParameterIndex("m_image")),
            otherTestImage->GetImageView()->GetDeviceImageView(0)->GetBindlessReadIndex());
        EXPECT_EQ(paramData->GetShaderParameterData<uint32_t>(layout.GetParameterIndex("m_enum")), 3u);
        EXPECT_EQ(paramData->GetShaderParameterData<uint32_t>(layout.GetParameterIndex("m_samplerIndex")), samplerIndex);
        EXPECT_EQ(paramData->GetShaderParameterData<RHI::SamplerState>(layout.GetParameterIndex("m_samplerIndex")), samplerState);
    }

    TEST_F(MaterialTests, TestSetPropertyValueToMultipleShaderSettings)
    {
        Data::Asset<MaterialTypeAsset> materialTypeAsset;
        Data::Asset<MaterialAsset> materialAsset;

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.AddShader(m_testMaterialShaderAsset);
        materialTypeCreator.SetMaterialShaderParameterLayout(m_testMaterialShaderParameterLayout);
        materialTypeCreator.BeginMaterialProperty(Name{ "MyInt" }, MaterialPropertyDataType::Int);
        materialTypeCreator.ConnectMaterialPropertyToShaderParameter(Name{ "m_int" });
        materialTypeCreator.ConnectMaterialPropertyToShaderParameter(Name{ "m_uint" });
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.UpdateShaderParameterConnections();
        materialTypeCreator.End(materialTypeAsset);

        MaterialAssetCreator materialAssetCreator;
        materialAssetCreator.Begin(Uuid::CreateRandom(), materialTypeAsset);
        materialAssetCreator.End(materialAsset);

        Data::Instance<Material> material = Material::FindOrCreate(materialAsset);

        EXPECT_TRUE(material->SetPropertyValue<int32_t>(material->FindPropertyIndex(Name{ "MyInt" }), 42));

        // Test reading the value directly...

        EXPECT_EQ(material->GetPropertyValue<int32_t>(material->FindPropertyIndex(Name{ "MyInt" })), 42);

        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        // Dig in to the SRG to make sure the values were applied to both shader constants...

        const RHI::ShaderResourceGroup* srg = material->GetRHIShaderResourceGroup();
        const RHI::ShaderResourceGroupData& srgData = srg->GetData();

        EXPECT_EQ(srgData.GetConstant<int32_t>(srgData.FindShaderInputConstantIndex(Name{ "m_int" })), 42);
        EXPECT_EQ(srgData.GetConstant<uint32_t>(srgData.FindShaderInputConstantIndex(Name{ "m_uint" })), 42u);

        auto paramData = material->GetMaterialShaderParameter();
        auto layout = material->GetAsset()->GetMaterialTypeAsset()->GetMaterialShaderParameterLayout();

        EXPECT_EQ(paramData->GetShaderParameterData<int32_t>(layout.GetParameterIndex("m_int")), 42);
        EXPECT_EQ(paramData->GetShaderParameterData<uint32_t>(layout.GetParameterIndex("m_uint")), 42u);
    }

    TEST_F(MaterialTests, TestSetPropertyValueWhenValueIsUnchanged)
    {
        Data::Instance<Material> material = Material::FindOrCreate(m_testMaterialAsset);
        
        EXPECT_TRUE(material->SetPropertyValue<float>(material->FindPropertyIndex(Name{ "MyFloat" }), 2.5f));

        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        EXPECT_TRUE(material->Compile());
        MaterialInstanceHandlerInterface::Get()->Compile();

        // Taint the SRG so we can check whether it was set by the SetPropertyValue() calls below.
        const RHI::ShaderResourceGroup* srg = material->GetRHIShaderResourceGroup();
        const RHI::ShaderResourceGroupData& srgData = srg->GetData();

        const_cast<RHI::ShaderResourceGroupData*>(&srgData)->SetConstant(
            m_testMaterialSrgLayout->FindShaderInputConstantIndex(Name{ "m_float" }), 0.1f);

        // Also taint the Parameter-Buffer
        auto paramData = material->GetMaterialShaderParameter();
        auto layout = material->GetAsset()->GetMaterialTypeAsset()->GetMaterialShaderParameterLayout();
        paramData->SetParameter(layout.GetParameterIndex("m_float"), 0.1f);

        // Set the properties to the same values as before
        EXPECT_FALSE(material->SetPropertyValue<float>(material->FindPropertyIndex(Name{ "MyFloat" }), 2.5f));

        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        EXPECT_TRUE(material->Compile());
        MaterialInstanceHandlerInterface::Get()->Compile();

        // Make sure the SRG is still tainted, because the SetPropertyValue() functions weren't processed
        EXPECT_EQ(srgData.GetConstant<float>(srgData.FindShaderInputConstantIndex(Name{ "m_float" })), 0.1f);
        EXPECT_EQ(paramData->GetShaderParameterData<float>(layout.GetParameterIndex("m_float")), 0.1f);
    }

    TEST_F(MaterialTests, TestImageNotProvided)
    {
        Data::Asset<MaterialAsset> materialAssetWithEmptyImage;

        MaterialAssetCreator materialCreator;
        materialCreator.Begin(Uuid::CreateRandom(), m_testMaterialTypeAsset);
        materialCreator.SetPropertyValue(Name{"MyFloat2"}, Vector2{0.1f, 0.2f});
        materialCreator.SetPropertyValue(Name{"MyFloat3"}, Vector3{1.1f, 1.2f, 1.3f});
        materialCreator.SetPropertyValue(Name{"MyFloat4"}, Vector4{2.1f, 2.2f, 2.3f, 2.4f});
        materialCreator.SetPropertyValue(Name{"MyColor"}, Color{1.0f, 1.0f, 1.0f, 1.0f});
        materialCreator.SetPropertyValue(Name{"MyInt"}, -2);
        materialCreator.SetPropertyValue(Name{"MyUInt"}, 12u);
        materialCreator.SetPropertyValue(Name{"MyFloat"}, 1.5f);
        materialCreator.SetPropertyValue(Name{"MyBool"}, true);
        // We don't set "MyImage"
        materialCreator.End(materialAssetWithEmptyImage);

        Data::Instance<Material> material = Material::FindOrCreate(materialAssetWithEmptyImage);

        Data::Instance<Image> nullImageInstance;
        Data::Instance<Image> actualImageInstance = material->GetPropertyValue<Data::Instance<Image>>(material->FindPropertyIndex(Name{"MyImage"}));
        EXPECT_EQ(actualImageInstance, nullImageInstance);

        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        const RHI::ShaderResourceGroupData& srgData = material->GetRHIShaderResourceGroup()->GetData();

        EXPECT_EQ(srgData.GetImageView(srgData.FindShaderInputImageIndex(Name{"m_image"}), 0), nullptr);

        // the shader params only store a read-index, and 0 is the default
        auto shaderParamIndex = m_testMaterialTypeAsset->GetMaterialShaderParameterLayout().GetParameterIndex("m_image");
        const auto& imageReadIndex = material->GetMaterialShaderParameter()->GetShaderParameterData<int32_t>(shaderParamIndex);
        EXPECT_EQ(imageReadIndex, -1);
    }

    TEST_F(MaterialTests, TestMaterialWithNoSRGOrProperties)
    {
        // Making a material with no properties and no SRG allows us to create simple shaders
        // that don't need any input, for example a debug shader that just renders surface normals.

        Data::Asset<MaterialTypeAsset> emptyMaterialTypeAsset;
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeCreator.End(emptyMaterialTypeAsset));

        Data::Asset<MaterialAsset> emptyMaterialAsset;
        MaterialAssetCreator materialCreator;
        materialCreator.Begin(Uuid::CreateRandom(), emptyMaterialTypeAsset);
        EXPECT_TRUE(materialCreator.End(emptyMaterialAsset));

        Data::Instance<Material> material = Material::FindOrCreate(emptyMaterialAsset);
        MaterialInstanceHandlerInterface::Get()->Compile();
        EXPECT_TRUE(material);
        EXPECT_FALSE(material->GetRHIShaderResourceGroup());
        EXPECT_FALSE(material->GetMaterialShaderParameter());
    }

    Ptr<ShaderOptionGroupLayout> CreateTestOptionsLayout()
    {
        AZStd::vector<RPI::ShaderOptionValuePair> enumOptionValues = CreateEnumShaderOptionValues({"Low", "Med", "High"});
        AZStd::vector<RPI::ShaderOptionValuePair> boolOptionValues = CreateBoolShaderOptionValues();
        AZStd::vector<RPI::ShaderOptionValuePair> rangeOptionValues = CreateIntRangeShaderOptionValues(1, 10);

        Ptr<ShaderOptionGroupLayout> shaderOptions = ShaderOptionGroupLayout::Create();
        uint32_t order = 0;
        shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_enumA"}, ShaderOptionType::Enumeration, 0, order++, enumOptionValues, Name{"Low"}});
        shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_enumB"}, ShaderOptionType::Enumeration, 2, order++, enumOptionValues, Name{"Low"}});
        shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_enumC"}, ShaderOptionType::Enumeration, 4, order++, enumOptionValues, Name{"Low"}});
        shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_boolA"}, ShaderOptionType::Boolean, 6, order++, boolOptionValues, Name{"False"}});
        shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_boolB"}, ShaderOptionType::Boolean, 7, order++, boolOptionValues, Name{"False"}});
        shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_boolC"}, ShaderOptionType::Boolean, 8, order++, boolOptionValues, Name{"False"}});
        shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_rangeA"}, ShaderOptionType::IntegerRange, 9, order++, rangeOptionValues, Name{"1"}});
        shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_rangeB"}, ShaderOptionType::IntegerRange, 13, order++, rangeOptionValues, Name{"1"}});
        shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_rangeC"}, ShaderOptionType::IntegerRange, 17, order++, rangeOptionValues, Name{"1"}});
        shaderOptions->Finalize();

        return shaderOptions;
    }

    TEST_F(MaterialTests, TestSetPropertyValue_ConnectedToShaderOptions_AllTypes)
    {
        Ptr<ShaderOptionGroupLayout> optionsLayout = CreateTestOptionsLayout();

        Data::Asset<ShaderAsset> shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout, optionsLayout);
        
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.AddShader(shaderAsset);
        materialTypeCreator.SetMaterialShaderParameterLayout(m_testMaterialShaderParameterLayout);
        materialTypeCreator.BeginMaterialProperty(Name{"EnumA"}, MaterialPropertyDataType::Int);
        materialTypeCreator.ConnectMaterialPropertyToShaderOptions(Name{"o_enumA"});
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.BeginMaterialProperty(Name{"EnumB"}, MaterialPropertyDataType::UInt);
        materialTypeCreator.ConnectMaterialPropertyToShaderOptions(Name{"o_enumB"});
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.BeginMaterialProperty(Name{"Bool"}, MaterialPropertyDataType::Bool);
        materialTypeCreator.ConnectMaterialPropertyToShaderOptions(Name{"o_boolA"});
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.BeginMaterialProperty(Name{"RangeA"}, MaterialPropertyDataType::Int);
        materialTypeCreator.ConnectMaterialPropertyToShaderOptions(Name{"o_rangeA"});
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.BeginMaterialProperty(Name{"RangeB"}, MaterialPropertyDataType::UInt);
        materialTypeCreator.ConnectMaterialPropertyToShaderOptions(Name{"o_rangeB"});
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.SetPropertyValue(Name{"EnumA"}, 2);
        materialTypeCreator.SetPropertyValue(Name{"EnumB"}, 1u);
        materialTypeCreator.SetPropertyValue(Name{"Bool"}, true);
        materialTypeCreator.SetPropertyValue(Name{"RangeA"}, 5);
        materialTypeCreator.SetPropertyValue(Name{"RangeB"}, 10u);
        materialTypeCreator.End(m_testMaterialTypeAsset);

        MaterialAssetCreator materialAssetCreator;
        materialAssetCreator.Begin(Uuid::CreateRandom(), m_testMaterialTypeAsset);
        materialAssetCreator.End(m_testMaterialAsset);

        Data::Instance<Material> material = Material::FindOrCreate(m_testMaterialAsset);

        auto& optionEnumA  = optionsLayout->GetShaderOption(optionsLayout->FindShaderOptionIndex(Name{"o_enumA"}));
        auto& optionEnumB  = optionsLayout->GetShaderOption(optionsLayout->FindShaderOptionIndex(Name{"o_enumB"}));
        auto& optionBoolA  = optionsLayout->GetShaderOption(optionsLayout->FindShaderOptionIndex(Name{"o_boolA"}));
        auto& optionRangeA = optionsLayout->GetShaderOption(optionsLayout->FindShaderOptionIndex(Name{"o_rangeA"}));
        auto& optionRangeB = optionsLayout->GetShaderOption(optionsLayout->FindShaderOptionIndex(Name{"o_rangeB"}));

        // Check the values on the properties themselves
        EXPECT_EQ(material->GetPropertyValue<int32_t>(material->FindPropertyIndex(Name{"EnumA"})), 2);
        EXPECT_EQ(material->GetPropertyValue<uint32_t>(material->FindPropertyIndex(Name{"EnumB"})), 1u);
        EXPECT_EQ(material->GetPropertyValue<bool>(material->FindPropertyIndex(Name{"Bool"})), true);
        EXPECT_EQ(material->GetPropertyValue<int32_t>(material->FindPropertyIndex(Name{"RangeA"})), 5);
        EXPECT_EQ(material->GetPropertyValue<uint32_t>(material->FindPropertyIndex(Name{"RangeB"})), 10u);

        // Check the values on the underlying ShaderCollection::Item
        ShaderOptionGroup options{optionsLayout, material->GetGeneralShaderCollection()[0].GetShaderVariantId()};
        EXPECT_EQ(optionEnumA.Get(options).GetIndex(), optionEnumA.FindValue(Name{"High"}).GetIndex());
        EXPECT_EQ(optionEnumB.Get(options).GetIndex(), optionEnumB.FindValue(Name{"Med"}).GetIndex());
        EXPECT_EQ(optionBoolA.Get(options).GetIndex(), optionBoolA.FindValue(Name{"True"}).GetIndex());
        EXPECT_EQ(optionRangeA.Get(options).GetIndex(), 5);
        EXPECT_EQ(optionRangeB.Get(options).GetIndex(), 10);

        // Now call SetPropertyValue to change the values, and check again
        EXPECT_TRUE(material->SetPropertyValue<int32_t>(material->FindPropertyIndex(Name{"EnumA"}), 1));
        EXPECT_TRUE(material->SetPropertyValue<uint32_t>(material->FindPropertyIndex(Name{"EnumB"}), 0u));
        EXPECT_TRUE(material->SetPropertyValue<bool>(material->FindPropertyIndex(Name{"Bool"}), false));
        EXPECT_TRUE(material->SetPropertyValue<int32_t>(material->FindPropertyIndex(Name{"RangeA"}), 3));
        EXPECT_TRUE(material->SetPropertyValue<uint32_t>(material->FindPropertyIndex(Name{"RangeB"}), 7u));

        // Check the values on the properties themselves
        EXPECT_EQ(material->GetPropertyValue<int32_t>(material->FindPropertyIndex(Name{"EnumA"})), 1);
        EXPECT_EQ(material->GetPropertyValue<uint32_t>(material->FindPropertyIndex(Name{"EnumB"})), 0u);
        EXPECT_EQ(material->GetPropertyValue<bool>(material->FindPropertyIndex(Name{"Bool"})), false);
        EXPECT_EQ(material->GetPropertyValue<int32_t>(material->FindPropertyIndex(Name{"RangeA"})), 3);
        EXPECT_EQ(material->GetPropertyValue<uint32_t>(material->FindPropertyIndex(Name{"RangeB"})), 7u);

        ProcessQueuedSrgCompilations(shaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        // Check the values on the underlying ShaderCollection::Item
        ShaderOptionGroup options2{optionsLayout, material->GetGeneralShaderCollection()[0].GetShaderVariantId()};
        EXPECT_EQ(optionEnumA.Get(options2).GetIndex(), optionEnumA.FindValue(Name{"Med"}).GetIndex());
        EXPECT_EQ(optionEnumB.Get(options2).GetIndex(), optionEnumB.FindValue(Name{"Low"}).GetIndex());
        EXPECT_EQ(optionBoolA.Get(options2).GetIndex(), optionBoolA.FindValue(Name{"False"}).GetIndex());
        EXPECT_EQ(optionRangeA.Get(options2).GetIndex(), 3);
        EXPECT_EQ(optionRangeB.Get(options2).GetIndex(), 7);
    }

    TEST_F(MaterialTests, TestSetPropertyValue_ConnectedToShaderOptions_WithMultipleShaders)
    {
        Ptr<ShaderOptionGroupLayout> optionsLayout = CreateTestOptionsLayout();

        Data::Asset<ShaderAsset> shaderAsset =
            CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout, optionsLayout);

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        // Adding more than one shader
        materialTypeCreator.AddShader(shaderAsset);
        materialTypeCreator.AddShader(shaderAsset);
        materialTypeCreator.AddShader(shaderAsset);
        materialTypeCreator.SetMaterialShaderParameterLayout(m_testMaterialShaderParameterLayout);
        materialTypeCreator.BeginMaterialProperty(Name{"Value"}, MaterialPropertyDataType::Int);
        materialTypeCreator.ConnectMaterialPropertyToShaderOptions(Name{"o_rangeB"}); // Applies to all shaders
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.SetPropertyValue(Name{"Value"}, 2);
        materialTypeCreator.End(m_testMaterialTypeAsset);

        MaterialAssetCreator materialAssetCreator;
        materialAssetCreator.Begin(Uuid::CreateRandom(), m_testMaterialTypeAsset);
        materialAssetCreator.End(m_testMaterialAsset);

        Data::Instance<Material> material = Material::FindOrCreate(m_testMaterialAsset);

        auto& optionRangeB = optionsLayout->GetShaderOption(optionsLayout->FindShaderOptionIndex(Name{"o_rangeB"}));

        const ShaderCollection& shaderCollection = material->GetGeneralShaderCollection();

        // Check the values on the underlying ShaderVariantReferences
        {
            ShaderOptionGroup options0{optionsLayout, shaderCollection[0].GetShaderVariantId()};
            ShaderOptionGroup options1{optionsLayout, shaderCollection[1].GetShaderVariantId()};
            ShaderOptionGroup options2{optionsLayout, shaderCollection[2].GetShaderVariantId()};
            EXPECT_EQ(optionRangeB.Get(options0).GetIndex(),  2);
            EXPECT_EQ(optionRangeB.Get(options1).GetIndex(),  2);
            EXPECT_EQ(optionRangeB.Get(options2).GetIndex(),  2);
        }

        // Now call SetPropertyValue to change the values, and check again
        EXPECT_TRUE(material->SetPropertyValue<int32_t>(material->FindPropertyIndex(Name{"Value"}), 5));

        ProcessQueuedSrgCompilations(shaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        // Check the values on the underlying ShaderVariantReferences
        {
            ShaderOptionGroup options0{optionsLayout, shaderCollection[0].GetShaderVariantId()};
            ShaderOptionGroup options1{optionsLayout, shaderCollection[1].GetShaderVariantId()};
            ShaderOptionGroup options2{optionsLayout, shaderCollection[2].GetShaderVariantId()};
            EXPECT_EQ(optionRangeB.Get(options0).GetIndex(),  5);
            EXPECT_EQ(optionRangeB.Get(options1).GetIndex(),  5);
            EXPECT_EQ(optionRangeB.Get(options2).GetIndex(),  5);
        }
    }

    TEST_F(MaterialTests, TestSetPropertyValue_ConnectedToShaderEnabled)
    {
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"one"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"two"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"three"});
        materialTypeCreator.SetMaterialShaderParameterLayout(m_testMaterialShaderParameterLayout);
        materialTypeCreator.BeginMaterialProperty(Name{"EnableSecondShader"}, MaterialPropertyDataType::Bool);
        materialTypeCreator.ConnectMaterialPropertyToShaderEnabled(Name{"two"});
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.End(m_testMaterialTypeAsset);

        MaterialAssetCreator materialAssetCreator;
        materialAssetCreator.Begin(Uuid::CreateRandom(), m_testMaterialTypeAsset);
        materialAssetCreator.End(m_testMaterialAsset);

        Data::Instance<Material> material = Material::FindOrCreate(m_testMaterialAsset);

        MaterialPropertyIndex enableShader = material->FindPropertyIndex(Name{"EnableSecondShader"});

        const ShaderCollection& shaderCollection = material->GetGeneralShaderCollection();

        EXPECT_TRUE(shaderCollection[0].IsEnabled());
        EXPECT_FALSE(shaderCollection[1].IsEnabled());
        EXPECT_TRUE(shaderCollection[2].IsEnabled());

        material->SetPropertyValue(enableShader, true);

        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        EXPECT_TRUE(shaderCollection[0].IsEnabled());
        EXPECT_TRUE(shaderCollection[1].IsEnabled());
        EXPECT_TRUE(shaderCollection[2].IsEnabled());

        material->SetPropertyValue(enableShader, false);

        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        EXPECT_TRUE(shaderCollection[0].IsEnabled());
        EXPECT_FALSE(shaderCollection[1].IsEnabled());
        EXPECT_TRUE(shaderCollection[2].IsEnabled());
    }

    TEST_F(MaterialTests, TestSetPropertyValue_ConnectedToInternalProperty_ConnectedToShaderEnabled)
    {
        // This tests the concept of internal properties used to pass data from the material to a MaterialPipelinePayload.

        // The test will set up a structure like this:
        //   Properties
        //       "general.useSpecialFeature" connects to "EnableSpecialFeature" in each pipeline
        //   Material pipelines
        //       "PipalineA"
        //           Properties
        //               "EnableSpecialFeature" connects to enable the local "special" shader
        //           Shaders
        //               "shader1"
        //               "shader2"
        //               "special"
        //       "PipalineB"
        //           Properties
        //               "EnableSpecialFeature" connects to enable the local "peculiar" shader
        //           Shaders
        //               "shaderA"
        //               "peculiar"
        //               "shaderB"

        using DrawItemType = MaterialTypeAssetCreator::DrawItemType;
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"shader1"}, DrawItemType::Raster, Name{"PipalineA"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"shader2"}, DrawItemType::Raster, Name{"PipalineA"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"special"}, DrawItemType::Raster, Name{"PipalineA"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"shaderA"}, DrawItemType::Raster, Name{"PipalineB"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"peculiar"}, DrawItemType::Raster, Name{"PipalineB"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"shaderB"}, DrawItemType::Raster, Name{"PipalineB"});
        materialTypeCreator.SetMaterialShaderParameterLayout(m_testMaterialShaderParameterLayout);

        // PipalineA's EnableSpecialFeature connects to its "special" shader
        materialTypeCreator.BeginMaterialProperty(Name{"EnableSpecialFeature"}, MaterialPropertyDataType::Bool, Name{"PipalineA"});
        materialTypeCreator.ConnectMaterialPropertyToShaderEnabled(Name{"special"});
        materialTypeCreator.EndMaterialProperty();

        // PipalineB's EnableSpecialFeature connects to its "peculiar" shader
        materialTypeCreator.BeginMaterialProperty(Name{"EnableSpecialFeature"}, MaterialPropertyDataType::Bool, Name{"PipalineB"});
        materialTypeCreator.ConnectMaterialPropertyToShaderEnabled(Name{"peculiar"});
        materialTypeCreator.EndMaterialProperty();

        // The main property connects to EnableSpecialFeature
        materialTypeCreator.BeginMaterialProperty(Name{"general.useSpecialFeature"}, MaterialPropertyDataType::Bool, MaterialPipelineNone);
        materialTypeCreator.ConnectMaterialPropertyToInternalProperty(Name{"EnableSpecialFeature"});
        materialTypeCreator.EndMaterialProperty();

        materialTypeCreator.End(m_testMaterialTypeAsset);

        MaterialAssetCreator materialAssetCreator;
        materialAssetCreator.Begin(Uuid::CreateRandom(), m_testMaterialTypeAsset);
        materialAssetCreator.End(m_testMaterialAsset);

        Data::Instance<Material> material = Material::FindOrCreate(m_testMaterialAsset);

        MaterialPropertyIndex enableShader = material->FindPropertyIndex(Name{"general.useSpecialFeature"});

        const ShaderCollection& shaderCollectionA = material->GetShaderCollection(Name{"PipalineA"});
        const ShaderCollection& shaderCollectionB = material->GetShaderCollection(Name{"PipalineB"});

        EXPECT_TRUE(shaderCollectionA[0].IsEnabled());
        EXPECT_TRUE(shaderCollectionA[1].IsEnabled());
        EXPECT_FALSE(shaderCollectionA[2].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[0].IsEnabled());
        EXPECT_FALSE(shaderCollectionB[1].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[2].IsEnabled());

        material->SetPropertyValue(enableShader, true);

        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        EXPECT_TRUE(shaderCollectionA[0].IsEnabled());
        EXPECT_TRUE(shaderCollectionA[1].IsEnabled());
        EXPECT_TRUE(shaderCollectionA[2].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[0].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[1].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[2].IsEnabled());

        material->SetPropertyValue(enableShader, false);

        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        EXPECT_TRUE(shaderCollectionA[0].IsEnabled());
        EXPECT_TRUE(shaderCollectionA[1].IsEnabled());
        EXPECT_FALSE(shaderCollectionA[2].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[0].IsEnabled());
        EXPECT_FALSE(shaderCollectionB[1].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[2].IsEnabled());
    }

    TEST_F(MaterialTests, TestSetPropertyValue_ConnectedToMaterialFunctor_ConnectedToShaderEnabled)
    {
        using namespace AZ::RPI;

        // This is the same as TestSetPropertyValue_ConnectedToInternalProperty_ConnectedToShaderEnabled, except
        // a functor is used for the first connection instead of a direct connection.

        // The test will set up a structure like this:
        //   Properties
        //       "general.useSpecialFeature" doesn't connect to anything directly
        //   Functors
        //       SetInternalPropertyFunctor connects "general.useSpecialFeature" to "EnableSpecialFeature", which propagates to each pipeline
        //   Material pipelines (This is the same as TestSetPropertyValue_ConnectedToInternalProperty_ConnectedToShaderEnabled)
        //       "PipalineA"
        //           Properties
        //               "EnableSpecialFeature" connects to enable the local "special" shader
        //           Shaders
        //               "shader1"
        //               "shader2"
        //               "special"
        //       "PipalineB"
        //           Properties
        //               "EnableSpecialFeature" connects to enable the local "peculiar" shader
        //           Shaders
        //               "shaderA"
        //               "peculiar"
        //               "shaderB"
        
        using DrawItemType = MaterialTypeAssetCreator::DrawItemType;

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, ShaderVariantId{}, Name{"shader1"}, DrawItemType::Raster, Name{"PipalineA"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, ShaderVariantId{}, Name{"shader2"}, DrawItemType::Raster, Name{"PipalineA"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, ShaderVariantId{}, Name{"special"}, DrawItemType::Raster, Name{"PipalineA"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, ShaderVariantId{}, Name{"shaderA"}, DrawItemType::Raster, Name{"PipalineB"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, ShaderVariantId{}, Name{"peculiar"}, DrawItemType::Raster, Name{"PipalineB"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, ShaderVariantId{}, Name{"shaderB"}, DrawItemType::Raster, Name{"PipalineB"});
        materialTypeCreator.SetMaterialShaderParameterLayout(m_testMaterialShaderParameterLayout);

        // PipalineA's EnableSpecialFeature connects to its "special" shader
        materialTypeCreator.BeginMaterialProperty(Name{"EnableSpecialFeature"}, MaterialPropertyDataType::Bool, Name{"PipalineA"});
        materialTypeCreator.ConnectMaterialPropertyToShaderEnabled(Name{"special"});
        materialTypeCreator.EndMaterialProperty();

        // PipalineB's EnableSpecialFeature connects to its "peculiar" shader
        materialTypeCreator.BeginMaterialProperty(Name{"EnableSpecialFeature"}, MaterialPropertyDataType::Bool, Name{"PipalineB"});
        materialTypeCreator.ConnectMaterialPropertyToShaderEnabled(Name{"peculiar"});
        materialTypeCreator.EndMaterialProperty();

        // The main property doesn't connect directly, because a functor is used
        materialTypeCreator.BeginMaterialProperty(Name{"general.useSpecialFeature"}, MaterialPropertyDataType::Bool, MaterialPipelineNone);
        materialTypeCreator.EndMaterialProperty();

        // This functor makes the connection between general.useSpecialFeature and EnableSpecialFeature
        SetInternalPropertyFunctorSourceData functorCreator;
        functorCreator.m_inputPropertyName = Name{"general.useSpecialFeature"};
        functorCreator.m_outputPropertyName = Name{"EnableSpecialFeature"};
        MaterialNameContext nameContext;
        MaterialFunctorSourceData::FunctorResult result = functorCreator.CreateFunctor(
            MaterialFunctorSourceData::RuntimeContext{ "", materialTypeCreator.GetMaterialPropertiesLayout(), &nameContext });
        materialTypeCreator.AddMaterialFunctor(result.GetValue(), MaterialPipelineNone);

        materialTypeCreator.End(m_testMaterialTypeAsset);

        MaterialAssetCreator materialAssetCreator;
        materialAssetCreator.Begin(Uuid::CreateRandom(), m_testMaterialTypeAsset);
        materialAssetCreator.End(m_testMaterialAsset);

        Data::Instance<Material> material = Material::FindOrCreate(m_testMaterialAsset);

        MaterialPropertyIndex enableShader = material->FindPropertyIndex(Name{"general.useSpecialFeature"});

        const ShaderCollection& shaderCollectionA = material->GetShaderCollection(Name{"PipalineA"});
        const ShaderCollection& shaderCollectionB = material->GetShaderCollection(Name{"PipalineB"});

        EXPECT_TRUE(shaderCollectionA[0].IsEnabled());
        EXPECT_TRUE(shaderCollectionA[1].IsEnabled());
        EXPECT_FALSE(shaderCollectionA[2].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[0].IsEnabled());
        EXPECT_FALSE(shaderCollectionB[1].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[2].IsEnabled());

        material->SetPropertyValue(enableShader, true);

        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        EXPECT_TRUE(shaderCollectionA[0].IsEnabled());
        EXPECT_TRUE(shaderCollectionA[1].IsEnabled());
        EXPECT_TRUE(shaderCollectionA[2].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[0].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[1].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[2].IsEnabled());

        material->SetPropertyValue(enableShader, false);

        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        EXPECT_TRUE(shaderCollectionA[0].IsEnabled());
        EXPECT_TRUE(shaderCollectionA[1].IsEnabled());
        EXPECT_FALSE(shaderCollectionA[2].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[0].IsEnabled());
        EXPECT_FALSE(shaderCollectionB[1].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[2].IsEnabled());
    }

    TEST_F(MaterialTests, TestSetPropertyValue_ConnectedToInternalProperty_ConnectedToMaterialPipelineFunctor)
    {
        // This is the same as TestSetPropertyValue_ConnectedToInternalProperty_ConnectedToShaderEnabled, except
        // a functor is used for the final ShaderEnable connection instead of a direct connection.

        // The test will set up a structure like this:
        //   Properties
        //       "general.useSpecialFeature" connects to "EnableSpecialFeature" in each pipeline
        //   Material pipelines
        //       "PipalineA"
        //           Properties
        //               "EnableSpecialFeature" doesn't connect to anything directly
        //           Shaders
        //               "shader1"
        //               "shader2"
        //               "special"
        //           Functors
        //               ShaderEnablePipelineFunctor uses "EnableSpecialFeature" to enable/disable the "special" shader
        //       "PipalineB"
        //           Properties
        //               "EnableSpecialFeature" doesn't connect to anything directly
        //           Shaders
        //               "shaderA"
        //               "peculiar"
        //               "shaderB"
        //           Functors
        //               ShaderEnablePipelineFunctor uses "EnableSpecialFeature" to enable/disable the "peculiar" shader
        
        using DrawItemType = MaterialTypeAssetCreator::DrawItemType;

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"shader1"}, DrawItemType::Raster, Name{"PipalineA"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"shader2"}, DrawItemType::Raster, Name{"PipalineA"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"special"}, DrawItemType::Raster, Name{"PipalineA"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"shaderA"}, DrawItemType::Raster, Name{"PipalineB"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"peculiar"}, DrawItemType::Raster, Name{"PipalineB"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"shaderB"}, DrawItemType::Raster, Name{"PipalineB"});
        materialTypeCreator.SetMaterialShaderParameterLayout(m_testMaterialShaderParameterLayout);

        // PipalineA's EnableSpecialFeature connects to its "special" shader
        materialTypeCreator.BeginMaterialProperty(Name{"EnableSpecialFeature"}, MaterialPropertyDataType::Bool, Name{"PipalineA"});
        materialTypeCreator.EndMaterialProperty();

        // This functor makes the connection between EnableSpecialFeature and PipelineA's "special" shader
        ShaderEnablePipelineFunctorSourceData shaderEnableFunctorCreator;
        shaderEnableFunctorCreator.m_enablePropertyName = Name{"EnableSpecialFeature"};
        shaderEnableFunctorCreator.m_shaderTag = Name{"special"};
        MaterialNameContext defaultNameContext;
        MaterialFunctorSourceData::FunctorResult createFunctorResult =
            shaderEnableFunctorCreator.CreateFunctor(MaterialFunctorSourceData::RuntimeContext{
                "", materialTypeCreator.GetMaterialPropertiesLayout(Name{ "PipalineA" }), &defaultNameContext });
        materialTypeCreator.AddMaterialFunctor(createFunctorResult.GetValue(), Name{"PipalineA"});

        // PipalineB's EnableSpecialFeature connects to its "peculiar" shader
        materialTypeCreator.BeginMaterialProperty(Name{"EnableSpecialFeature"}, MaterialPropertyDataType::Bool, Name{"PipalineB"});
        materialTypeCreator.EndMaterialProperty();

        // This functor makes the connection between EnableSpecialFeature and PipelineB's "peculiar" shader
        shaderEnableFunctorCreator.m_enablePropertyName = Name{"EnableSpecialFeature"};
        shaderEnableFunctorCreator.m_shaderTag = Name{"peculiar"};
        createFunctorResult = shaderEnableFunctorCreator.CreateFunctor(MaterialFunctorSourceData::RuntimeContext{
            "", materialTypeCreator.GetMaterialPropertiesLayout(Name{ "PipalineB" }), &defaultNameContext });
        materialTypeCreator.AddMaterialFunctor(createFunctorResult.GetValue(), Name{"PipalineB"});

        // The main property connects to EnableSpecialFeature
        materialTypeCreator.BeginMaterialProperty(Name{"general.useSpecialFeature"}, MaterialPropertyDataType::Bool, MaterialPipelineNone);
        materialTypeCreator.ConnectMaterialPropertyToInternalProperty(Name{"EnableSpecialFeature"});
        materialTypeCreator.EndMaterialProperty();

        materialTypeCreator.End(m_testMaterialTypeAsset);

        MaterialAssetCreator materialAssetCreator;
        materialAssetCreator.Begin(Uuid::CreateRandom(), m_testMaterialTypeAsset);
        materialAssetCreator.End(m_testMaterialAsset);

        Data::Instance<Material> material = Material::FindOrCreate(m_testMaterialAsset);

        MaterialPropertyIndex enableShader = material->FindPropertyIndex(Name{"general.useSpecialFeature"});

        const ShaderCollection& shaderCollectionA = material->GetShaderCollection(Name{"PipalineA"});
        const ShaderCollection& shaderCollectionB = material->GetShaderCollection(Name{"PipalineB"});

        EXPECT_TRUE(shaderCollectionA[0].IsEnabled());
        EXPECT_TRUE(shaderCollectionA[1].IsEnabled());
        EXPECT_FALSE(shaderCollectionA[2].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[0].IsEnabled());
        EXPECT_FALSE(shaderCollectionB[1].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[2].IsEnabled());

        material->SetPropertyValue(enableShader, true);

        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        EXPECT_TRUE(shaderCollectionA[0].IsEnabled());
        EXPECT_TRUE(shaderCollectionA[1].IsEnabled());
        EXPECT_TRUE(shaderCollectionA[2].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[0].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[1].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[2].IsEnabled());

        material->SetPropertyValue(enableShader, false);

        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        EXPECT_TRUE(shaderCollectionA[0].IsEnabled());
        EXPECT_TRUE(shaderCollectionA[1].IsEnabled());
        EXPECT_FALSE(shaderCollectionA[2].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[0].IsEnabled());
        EXPECT_FALSE(shaderCollectionB[1].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[2].IsEnabled());
    }

    TEST_F(MaterialTests, TestSetPropertyValue_ConnectedToMaterialFunctor_ConnectedToMaterialPipelineFunctor)
    {
        // This is the same as TestSetPropertyValue_ConnectedToInternalProperty_ConnectedToShaderEnabled, except
        // functors are used to connect everything instead of direct connections.

        // The test will set up a structure like this:
        //   Properties
        //       "general.useSpecialFeature" doesn't connect to anything directly
        //   Functors
        //       SetInternalPropertyFunctor connects "general.useSpecialFeature" to "EnableSpecialFeature", which propagates to each pipeline
        //   Material pipelines
        //       "PipalineA"
        //           Properties
        //               "EnableSpecialFeature" doesn't connect to anything directly
        //           Shaders
        //               "shader1"
        //               "shader2"
        //               "special"
        //           Functors
        //               ShaderEnablePipelineFunctor uses "EnableSpecialFeature" to enable/disable the "special" shader
        //       "PipalineB"
        //           Properties
        //               "EnableSpecialFeature" doesn't connect to anything directly
        //           Shaders
        //               "shaderA"
        //               "peculiar"
        //               "shaderB"
        //           Functors
        //               ShaderEnablePipelineFunctor uses "EnableSpecialFeature" to enable/disable the "peculiar" shader

        using DrawItemType = MaterialTypeAssetCreator::DrawItemType;

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"shader1"}, DrawItemType::Raster, Name{"PipalineA"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"shader2"}, DrawItemType::Raster, Name{"PipalineA"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"special"}, DrawItemType::Raster, Name{"PipalineA"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"shaderA"}, DrawItemType::Raster, Name{"PipalineB"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"peculiar"}, DrawItemType::Raster, Name{"PipalineB"});
        materialTypeCreator.AddShader(m_testMaterialShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"shaderB"}, DrawItemType::Raster, Name{"PipalineB"});
        materialTypeCreator.SetMaterialShaderParameterLayout(m_testMaterialShaderParameterLayout);

        // PipalineA's EnableSpecialFeature connects to its "special" shader
        materialTypeCreator.BeginMaterialProperty(Name{"EnableSpecialFeature"}, MaterialPropertyDataType::Bool, Name{"PipalineA"});
        materialTypeCreator.EndMaterialProperty();

        // This functor makes the connection between EnableSpecialFeature and PipelineA's "special" shader
        ShaderEnablePipelineFunctorSourceData shaderEnableFunctorCreator;
        shaderEnableFunctorCreator.m_enablePropertyName = Name{"EnableSpecialFeature"};
        shaderEnableFunctorCreator.m_shaderTag = Name{"special"};
        MaterialNameContext defaultNameContext;
        MaterialFunctorSourceData::FunctorResult createFunctorResult =
            shaderEnableFunctorCreator.CreateFunctor(MaterialFunctorSourceData::RuntimeContext{
                "", materialTypeCreator.GetMaterialPropertiesLayout(Name{ "PipalineA" }), &defaultNameContext });
        materialTypeCreator.AddMaterialFunctor(createFunctorResult.GetValue(), Name{"PipalineA"});

        // PipalineB's EnableSpecialFeature connects to its "peculiar" shader
        materialTypeCreator.BeginMaterialProperty(Name{"EnableSpecialFeature"}, MaterialPropertyDataType::Bool, Name{"PipalineB"});
        materialTypeCreator.EndMaterialProperty();

        // This functor makes the connection between EnableSpecialFeature and PipelineB's "peculiar" shader
        shaderEnableFunctorCreator.m_enablePropertyName = Name{"EnableSpecialFeature"};
        shaderEnableFunctorCreator.m_shaderTag = Name{"peculiar"};
        createFunctorResult = shaderEnableFunctorCreator.CreateFunctor(MaterialFunctorSourceData::RuntimeContext{
            "", materialTypeCreator.GetMaterialPropertiesLayout(Name{ "PipalineB" }), &defaultNameContext });
        materialTypeCreator.AddMaterialFunctor(createFunctorResult.GetValue(), Name{"PipalineB"});

        // The main property doesn't connect directly, because a functor is used
        materialTypeCreator.BeginMaterialProperty(Name{"general.useSpecialFeature"}, MaterialPropertyDataType::Bool, MaterialPipelineNone);
        materialTypeCreator.EndMaterialProperty();

        // This functor makes the connection between general.useSpecialFeature and EnableSpecialFeature
        SetInternalPropertyFunctorSourceData setInternalPropertyFunctorCreator;
        setInternalPropertyFunctorCreator.m_inputPropertyName = Name{"general.useSpecialFeature"};
        setInternalPropertyFunctorCreator.m_outputPropertyName = Name{"EnableSpecialFeature"};
        MaterialFunctorSourceData::FunctorResult result = setInternalPropertyFunctorCreator.CreateFunctor(
            MaterialFunctorSourceData::RuntimeContext{ "", materialTypeCreator.GetMaterialPropertiesLayout(), &defaultNameContext });
        materialTypeCreator.AddMaterialFunctor(result.GetValue(), MaterialPipelineNone);

        materialTypeCreator.End(m_testMaterialTypeAsset);

        MaterialAssetCreator materialAssetCreator;
        materialAssetCreator.Begin(Uuid::CreateRandom(), m_testMaterialTypeAsset);
        materialAssetCreator.End(m_testMaterialAsset);

        Data::Instance<Material> material = Material::FindOrCreate(m_testMaterialAsset);

        MaterialPropertyIndex enableShader = material->FindPropertyIndex(Name{"general.useSpecialFeature"});

        const ShaderCollection& shaderCollectionA = material->GetShaderCollection(Name{"PipalineA"});
        const ShaderCollection& shaderCollectionB = material->GetShaderCollection(Name{"PipalineB"});

        EXPECT_TRUE(shaderCollectionA[0].IsEnabled());
        EXPECT_TRUE(shaderCollectionA[1].IsEnabled());
        EXPECT_FALSE(shaderCollectionA[2].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[0].IsEnabled());
        EXPECT_FALSE(shaderCollectionB[1].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[2].IsEnabled());

        material->SetPropertyValue(enableShader, true);

        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        EXPECT_TRUE(shaderCollectionA[0].IsEnabled());
        EXPECT_TRUE(shaderCollectionA[1].IsEnabled());
        EXPECT_TRUE(shaderCollectionA[2].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[0].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[1].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[2].IsEnabled());

        material->SetPropertyValue(enableShader, false);

        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        EXPECT_TRUE(shaderCollectionA[0].IsEnabled());
        EXPECT_TRUE(shaderCollectionA[1].IsEnabled());
        EXPECT_FALSE(shaderCollectionA[2].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[0].IsEnabled());
        EXPECT_FALSE(shaderCollectionB[1].IsEnabled());
        EXPECT_TRUE(shaderCollectionB[2].IsEnabled());
    }

    TEST_F(MaterialTests, TestSetSystemShaderOption)
    {
        Ptr<ShaderOptionGroupLayout> optionsLayout = CreateTestOptionsLayout();

        Data::Asset<ShaderAsset> shaderAsset =
            CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout, optionsLayout);

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.AddShader(shaderAsset);
        materialTypeCreator.AddShader(shaderAsset);
        materialTypeCreator.AddShader(shaderAsset);
        materialTypeCreator.SetMaterialShaderParameterLayout(m_testMaterialShaderParameterLayout);
        materialTypeCreator.BeginMaterialProperty(Name{"BoolValue"}, MaterialPropertyDataType::Bool);
        materialTypeCreator.ConnectMaterialPropertyToShaderOptions(Name{"o_boolA"}); // Applies to all shaders
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.ClaimShaderOptionOwnership(Name{"o_boolB"});
        materialTypeCreator.End(m_testMaterialTypeAsset);

        MaterialAssetCreator materialAssetCreator;
        materialAssetCreator.Begin(Uuid::CreateRandom(), m_testMaterialTypeAsset);
        materialAssetCreator.End(m_testMaterialAsset);

        Data::Instance<Material> material = Material::FindOrCreate(m_testMaterialAsset);

        EXPECT_EQ(3, material->SetSystemShaderOption(Name{"o_enumA"}, ShaderOptionValue{0}).GetValue());
        EXPECT_EQ(3, material->SetSystemShaderOption(Name{"o_enumB"}, ShaderOptionValue{1}).GetValue());
        EXPECT_EQ(3, material->SetSystemShaderOption(Name{"o_enumC"}, ShaderOptionValue{2}).GetValue());
        EXPECT_FALSE(material->SetSystemShaderOption(Name{"o_boolA"}, ShaderOptionValue{1}).IsSuccess());
        EXPECT_FALSE(material->SetSystemShaderOption(Name{"o_boolB"}, ShaderOptionValue{1}).IsSuccess());
        EXPECT_EQ(3, material->SetSystemShaderOption(Name{"o_boolC"}, ShaderOptionValue{1}).GetValue());
        EXPECT_EQ(3, material->SetSystemShaderOption(Name{"o_rangeA"}, ShaderOptionValue{3}).GetValue());
        EXPECT_EQ(3, material->SetSystemShaderOption(Name{"o_rangeB"}, ShaderOptionValue{4}).GetValue());
        EXPECT_EQ(3, material->SetSystemShaderOption(Name{"o_rangeC"}, ShaderOptionValue{5}).GetValue());

        // Try setting a shader option that does not exist in this material
        auto result = material->SetSystemShaderOption(Name{"o_someOtherOption"}, ShaderOptionValue{1});
        EXPECT_TRUE(result.IsSuccess());
        EXPECT_EQ(0, result.GetValue());

        for (size_t i = 0; i < material->GetGeneralShaderCollection().size(); ++i)
        {
            auto& shaderItem = material->GetGeneralShaderCollection()[i];

            EXPECT_EQ(0, shaderItem.GetShaderOptions()->GetValue(Name{"o_enumA"}).GetIndex());
            EXPECT_EQ(1, shaderItem.GetShaderOptions()->GetValue(Name{"o_enumB"}).GetIndex());
            EXPECT_EQ(2, shaderItem.GetShaderOptions()->GetValue(Name{"o_enumC"}).GetIndex());
            EXPECT_EQ(1, shaderItem.GetShaderOptions()->GetValue(Name{"o_boolC"}).GetIndex());
            EXPECT_EQ(3, shaderItem.GetShaderOptions()->GetValue(Name{"o_rangeA"}).GetIndex());
            EXPECT_EQ(4, shaderItem.GetShaderOptions()->GetValue(Name{"o_rangeB"}).GetIndex());
            EXPECT_EQ(5, shaderItem.GetShaderOptions()->GetValue(Name{"o_rangeC"}).GetIndex());

            // We don't care whether a material-owned shader option is unspecified or is initialized to its default state.
            // The important thing is that it did not change from its default value.
            auto checkValueNotChanged = [&shaderItem](const Name& name, ShaderOptionValue expectedValue)
            {
                ShaderOptionValue value = shaderItem.GetShaderOptions()->GetValue(name);
                if (value.IsValid())
                {
                    EXPECT_EQ(expectedValue.GetIndex(), value.GetIndex());
                }
            };

            checkValueNotChanged(Name{"o_boolA"}, ShaderOptionValue{0});
            checkValueNotChanged(Name{"o_boolB"}, ShaderOptionValue{0});
        }
    }

    TEST_F(MaterialTests, Error_InvalidShaderOptionValue)
    {
        Ptr<ShaderOptionGroupLayout> optionsLayout = CreateTestOptionsLayout();

        Data::Asset<ShaderAsset> shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout, optionsLayout);

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.AddShader(shaderAsset);
        materialTypeCreator.SetMaterialShaderParameterLayout(m_testMaterialShaderParameterLayout);
        materialTypeCreator.BeginMaterialProperty(Name{"Value"}, MaterialPropertyDataType::Int);
        materialTypeCreator.ConnectMaterialPropertyToShaderOptions(Name{"o_rangeA"});
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.SetPropertyValue(Name{"Value"}, 1);
        materialTypeCreator.End(m_testMaterialTypeAsset);

        MaterialAssetCreator materialAssetCreator;
        materialAssetCreator.Begin(Uuid::CreateRandom(), m_testMaterialTypeAsset);
        materialAssetCreator.End(m_testMaterialAsset);

        Data::Instance<Material> material = Material::FindOrCreate(m_testMaterialAsset);

        ErrorMessageFinder errorMessageFinder("ShaderOption value [100] is out of range");

        // Depending on implementation, the error message might not actually be reported until Compile() is called.
        // In that case, SetPropertyValue won't know about any issue and just return true after setting the property value.
        // If we want SetPropertyValue to return false, then we would have to change the implementation to process
        // property connections immediately instead of in Compile(), but there's little point to that as lua functors
        // can't be processed immediately, they have to wait for Compile(). So we might as well keep all our back-end
        // error checking in Compile().

        material->SetPropertyValue<int32_t>(material->FindPropertyIndex(Name{"Value"}), 100); // 100 is out of range, max is 10
        ProcessQueuedSrgCompilations(shaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();

        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(MaterialTests, Error_ImageNotFound)
    {
        Data::Asset<MaterialAsset> materialAsset;
        MaterialAssetCreator materialCreator;
        materialCreator.Begin(Uuid::CreateRandom(), m_testMaterialTypeAsset);
        materialCreator.SetPropertyValue(Name{ "MyFloat2" }, Vector2{ 0.1f, 0.2f });
        materialCreator.SetPropertyValue(Name{ "MyFloat3" }, Vector3{ 1.1f, 1.2f, 1.3f });
        materialCreator.SetPropertyValue(Name{ "MyFloat4" }, Vector4{ 2.1f, 2.2f, 2.3f, 2.4f });
        materialCreator.SetPropertyValue(Name{ "MyColor" }, Color{ 1.0f, 1.0f, 1.0f, 1.0f });
        materialCreator.SetPropertyValue(Name{ "MyInt" }, -2);
        materialCreator.SetPropertyValue(Name{ "MyUInt" }, 12u);
        materialCreator.SetPropertyValue(Name{ "MyFloat" }, 1.5f);
        materialCreator.SetPropertyValue(Name{ "MyBool" }, true);
        // Set the image to an empty asset handle that isn't associated with any actual data. StreamingImage::FindOrCreate will fail.
        materialCreator.SetPropertyValue(Name{ "MyImage" }, Data::Asset<ImageAsset>(Uuid::CreateRandom(), azrtti_typeid<StreamingImageAsset>()));
        materialCreator.End(materialAsset);

        ErrorMessageFinder errorMessageFinder{"Image asset could not be loaded"};
        // The material may trigger a blocking load of the image asset, but there is no catalog in unit tests.
        errorMessageFinder.AddIgnoredErrorMessage("this type doesn't have a catalog", true);
        errorMessageFinder.AddIgnoredErrorMessage("Failed to retrieve required information for asset", true);
        errorMessageFinder.AddIgnoredErrorMessage("GetAsset called for asset which does not exist", true);
        errorMessageFinder.AddIgnoredErrorMessage(
            "Unable to find product asset 'shaders/scenematerialsrg.azshader'. Has the source asset finished building?");
        errorMessageFinder.AddIgnoredErrorMessage("Unable to locate the Material SRG shader asset, try again");

        Data::Instance<Material> material = Material::FindOrCreate(materialAsset);

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(nullptr, material);
    }

    TEST_F(MaterialTests, Error_AccessInvalidProperty)
    {
        Data::Instance<Material> material = Material::FindOrCreate(m_testMaterialAsset);

        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(material->SetPropertyValue<float>(MaterialPropertyIndex(), 0.0f));
        material->GetPropertyValue<float>(MaterialPropertyIndex());
        AZ_TEST_STOP_ASSERTTEST(2);
    }

    TEST_F(MaterialTests, Error_SetPropertyValue_WrongDataType)
    {
        Data::Instance<Material> material = Material::FindOrCreate(m_testMaterialAsset);

        {
            ErrorMessageFinder finder;
            finder.AddExpectedErrorMessage("Accessed as type", 9);
            finder.AddExpectedErrorMessage("but is type", 9);

            EXPECT_FALSE(material->SetPropertyValue<bool>(material->FindPropertyIndex(Name{"MyImage"}), false));
            EXPECT_FALSE(material->SetPropertyValue<int32_t>(material->FindPropertyIndex(Name{"MyBool"}), -5));
            EXPECT_FALSE(material->SetPropertyValue<uint32_t>(material->FindPropertyIndex(Name{"MyInt"}), 123u));
            EXPECT_FALSE(material->SetPropertyValue<float>(material->FindPropertyIndex(Name{"MyUInt"}), 2.5f));
            EXPECT_FALSE(material->SetPropertyValue<Vector2>(material->FindPropertyIndex(Name{"MyFloat"}), Vector2(10.1f, 10.2f)));
            EXPECT_FALSE(material->SetPropertyValue<Vector3>(material->FindPropertyIndex(Name{"MyFloat2"}), Vector3(11.1f, 11.2f, 11.3f)));
            EXPECT_FALSE(material->SetPropertyValue<Vector4>(material->FindPropertyIndex(Name{"MyFloat3"}), Vector4(12.1f, 12.2f, 12.3f, 12.4f)));
            EXPECT_FALSE(material->SetPropertyValue<Color>(material->FindPropertyIndex(Name{"MyFloat4"}), Color(0.1f, 0.2f, 0.3f, 0.4f)));
            EXPECT_FALSE(material->SetPropertyValue<Data::Instance<Image>>(material->FindPropertyIndex(Name{"MyColor"}), m_testImage));

            finder.CheckExpectedErrorsFound();
        }

        // Make sure the values have not changed

        ProcessQueuedSrgCompilations(m_testMaterialShaderAsset, m_testMaterialSrgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        ValidateInitialValuesFromMaterial(material);
    }

    TEST_F(MaterialTests, Error_GetPropertyValue_WrongDataType)
    {
        Data::Instance<Material> material = Material::FindOrCreate(m_testMaterialAsset);

        ErrorMessageFinder finder;
        finder.AddExpectedErrorMessage("Accessed as type", 9);
        finder.AddExpectedErrorMessage("but is type", 9);

        material->GetPropertyValue<bool>(material->FindPropertyIndex(Name{ "MyImage" }));
        material->GetPropertyValue<int32_t>(material->FindPropertyIndex(Name{ "MyBool" }));
        material->GetPropertyValue<uint32_t>(material->FindPropertyIndex(Name{ "MyInt" }));
        material->GetPropertyValue<float>(material->FindPropertyIndex(Name{ "MyUInt" }));
        material->GetPropertyValue<Vector2>(material->FindPropertyIndex(Name{ "MyFloat" }));
        material->GetPropertyValue<Vector3>(material->FindPropertyIndex(Name{ "MyFloat2" }));
        material->GetPropertyValue<Vector4>(material->FindPropertyIndex(Name{ "MyFloat3" }));
        material->GetPropertyValue<Color>(material->FindPropertyIndex(Name{ "MyFloat4" }));
        material->GetPropertyValue<Data::Instance<Image>>(material->FindPropertyIndex(Name{ "MyColor" }));

        finder.CheckExpectedErrorsFound();
    }

    TEST_F(MaterialTests, ColorPropertyCanMapToFloat3)
    {
        Data::Asset<MaterialTypeAsset> materialTypeAsset;
        Data::Asset<MaterialAsset> materialAsset;

        RHI::Ptr<RHI::ShaderResourceGroupLayout> srgLayout = RHI::ShaderResourceGroupLayout::Create();
        srgLayout->SetName(Name("MaterialSrg"));
        srgLayout->SetBindingSlot(SrgBindingSlot::Material);
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{"m_color"}, 0, 12, 0, 0});
        ASSERT_TRUE(srgLayout->Finalize());

        Data::Asset<ShaderAsset> shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), srgLayout);

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.AddShader(shaderAsset);
        materialTypeCreator.BeginMaterialProperty(Name{ "MyColor" }, MaterialPropertyDataType::Color);
        materialTypeCreator.ConnectMaterialPropertyToShaderParameter(Name{ "m_color" });
        materialTypeCreator.EndMaterialProperty();

        // create the parameter layout from the properties we have so far (e.g. my_color)
        auto paramsLayout = materialTypeCreator.CreateMaterialShaderParameterLayout();
        // connect the layout to the srg variable
        paramsLayout.ConnectParametersToSrg(srgLayout.get());
        // store the params layout in the MaterialTypeAsset
        materialTypeCreator.SetMaterialShaderParameterLayout(paramsLayout);
        // now try to connect the properties with a shader parameter connection to the layout entries
        materialTypeCreator.UpdateShaderParameterConnections();

        materialTypeCreator.End(materialTypeAsset);

        MaterialAssetCreator materialAssetCreator;
        materialAssetCreator.Begin(Uuid::CreateRandom(), materialTypeAsset);
        materialAssetCreator.End(materialAsset);

        Data::Instance<Material> material = Material::FindOrCreate(materialAsset);
        
        const AZ::Color inputColor{1.0f, 2.0f, 3.0f, 0.0f};

        MaterialPropertyIndex colorProperty{0};
        material->SetPropertyValue(colorProperty, inputColor);

        ProcessQueuedSrgCompilations(shaderAsset, srgLayout->GetName());
        material->Compile();
        MaterialInstanceHandlerInterface::Get()->Compile();

        AZ::Color colorFromMaterial = material->GetPropertyValue<AZ::Color>(colorProperty);

        RHI::ShaderInputConstantIndex colorConstant{0};
        AZ::Color colorFromSrg;
        const float* floatsFromSrg = reinterpret_cast<const float*>(material->GetRHIShaderResourceGroup()->GetData().GetConstantRaw(colorConstant).data());
        colorFromSrg = AZ::Color::CreateFromVector3(AZ::Vector3::CreateFromFloat3(floatsFromSrg));

        auto acesInputColor = TransformColor(inputColor, ColorSpaceId::LinearSRGB, ColorSpaceId::ACEScg);

        for (int i = 0; i < 3; ++i)
        {
            EXPECT_EQ((float)acesInputColor.GetElement(i), (float)colorFromSrg.GetElement(i));
            EXPECT_EQ((float)inputColor.GetElement(i), (float)colorFromMaterial.GetElement(i));
        }
    }

    TEST_F(MaterialTests, TestFindPropertyIndexUsingOldName)
    {
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.AddShader(m_testMaterialShaderAsset);
        materialTypeCreator.SetMaterialShaderParameterLayout(m_testMaterialShaderParameterLayout);
        AddCommonTestMaterialProperties(materialTypeCreator);
        materialTypeCreator.UpdateShaderParameterConnections();
        materialTypeCreator.SetVersion(2);
        MaterialVersionUpdate versionUpdate(2);
        versionUpdate.AddAction(MaterialVersionUpdate::Action(Name("rename"),
                {
                    { Name{ "from" }, AZStd::string("OldName") },
                    { Name{ "to"   }, AZStd::string("MyInt")   }
                } ));

        materialTypeCreator.AddVersionUpdate(versionUpdate);
        materialTypeCreator.End(m_testMaterialTypeAsset);

        MaterialAssetCreator materialCreator;
        materialCreator.Begin(Uuid::CreateRandom(), m_testMaterialTypeAsset);
        materialCreator.End(m_testMaterialAsset);

        Data::Instance<Material> material = Material::FindOrCreate(m_testMaterialAsset);

        bool wasRenamed = false;
        Name newName;
        MaterialPropertyIndex indexFromOldName = material->FindPropertyIndex(Name{"OldName"}, &wasRenamed, &newName);
        EXPECT_TRUE(wasRenamed);
        EXPECT_EQ(newName, Name{"MyInt"});
        
        MaterialPropertyIndex indexFromNewName = material->FindPropertyIndex(Name{"MyInt"}, &wasRenamed, &newName);
        EXPECT_FALSE(wasRenamed);

        EXPECT_EQ(indexFromOldName, indexFromNewName);
    }

    template<typename T>
    void CheckPropertyValueRoundTrip(const T& value)
    {
        AZ::RPI::MaterialPropertyValue materialPropertyValue{value};
        AZStd::any anyValue{value};
        AZ::RPI::MaterialPropertyValue materialPropertyValueFromAny = MaterialPropertyValue::FromAny(anyValue);
        AZ::RPI::MaterialPropertyValue materialPropertyValueFromRoundTrip = MaterialPropertyValue::FromAny(MaterialPropertyValue::ToAny(materialPropertyValue));
        
        EXPECT_EQ(materialPropertyValue, materialPropertyValueFromAny);
        EXPECT_EQ(materialPropertyValue, materialPropertyValueFromRoundTrip);

        if (materialPropertyValue.Is<Data::Asset<ImageAsset>>())
        {
            EXPECT_EQ(materialPropertyValue.GetValue<Data::Asset<ImageAsset>>().GetHint(), materialPropertyValueFromAny.GetValue<Data::Asset<ImageAsset>>().GetHint());
            EXPECT_EQ(materialPropertyValue.GetValue<Data::Asset<ImageAsset>>().GetHint(), materialPropertyValueFromRoundTrip.GetValue<Data::Asset<ImageAsset>>().GetHint());
        }
    }

    TEST_F(MaterialTests, TestMaterialPropertyValueAsAny)
    {
        CheckPropertyValueRoundTrip(true);
        CheckPropertyValueRoundTrip(false);
        CheckPropertyValueRoundTrip(7);
        CheckPropertyValueRoundTrip(8u);
        CheckPropertyValueRoundTrip(9.0f);
        CheckPropertyValueRoundTrip(AZ::Vector2(1.0f, 2.0f));
        CheckPropertyValueRoundTrip(AZ::Vector3(1.0f, 2.0f, 3.0f));
        CheckPropertyValueRoundTrip(AZ::Vector4(1.0f, 2.0f, 3.0f, 4.0f));
        CheckPropertyValueRoundTrip(AZ::Color(1.0f, 2.0f, 3.0f, 4.0f));
        CheckPropertyValueRoundTrip(Data::Asset<Data::AssetData>{});
        CheckPropertyValueRoundTrip(Data::Asset<ImageAsset>{});
        CheckPropertyValueRoundTrip(Data::Asset<StreamingImageAsset>{});
        CheckPropertyValueRoundTrip(Data::Asset<AttachmentImageAsset>{});
        CheckPropertyValueRoundTrip(Data::Asset<Data::AssetData>{Uuid::CreateRandom(), azrtti_typeid<AZ::RPI::StreamingImageAsset>(), "TestAssetPath.png"});
        CheckPropertyValueRoundTrip(Data::Asset<Data::AssetData>{Uuid::CreateRandom(), azrtti_typeid<AZ::RPI::AttachmentImageAsset>(), "TestAssetPath.attimage"});
        CheckPropertyValueRoundTrip(Data::Asset<ImageAsset>{Uuid::CreateRandom(), azrtti_typeid<AZ::RPI::StreamingImageAsset>(), "TestAssetPath.png"});
        CheckPropertyValueRoundTrip(Data::Asset<ImageAsset>{Uuid::CreateRandom(), azrtti_typeid<AZ::RPI::AttachmentImageAsset>(), "TestAssetPath.attimage"});
        CheckPropertyValueRoundTrip(Data::Asset<StreamingImageAsset>{Uuid::CreateRandom(), azrtti_typeid<AZ::RPI::StreamingImageAsset>(), "TestAssetPath.png"});
        CheckPropertyValueRoundTrip(Data::Asset<AttachmentImageAsset>{Uuid::CreateRandom(), azrtti_typeid<AZ::RPI::AttachmentImageAsset>(), "TestAssetPath.attimage"});
        CheckPropertyValueRoundTrip(m_testImageAsset);
        CheckPropertyValueRoundTrip(m_testAttachmentImageAsset);
        CheckPropertyValueRoundTrip(Data::Instance<Image>{m_testImage});
        CheckPropertyValueRoundTrip(Data::Instance<Image>{m_testAttachmentImage});
        CheckPropertyValueRoundTrip(AZStd::string{"hello"});
    }
}
