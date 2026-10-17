/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Reflect/Base.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderAsset.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderVariantKey.h>

#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroupPool.h>
#include <JE/Render/RPI/Reflect/Image/Image.h>
#include <JE/Render/RPI/Public/Buffer/Buffer.h>

#include <Atom/RHI.Reflect/ShaderInputNameIndex.h>
#include <Atom/RHI/ShaderResourceGroup.h>

#include <AzCore/Instance/InstanceData.h>
#include <AzCore/Instance/InstanceId.h>
#include <AzCore/std/containers/span.h>

namespace AZ
{
    namespace RHI
    {
        struct ShaderDataMappings;
    }

    }

namespace JE::Render::RPI
{
        /**
         * This class is an RPI extension to the RHI shader resource group class. It provides support
         * for instantiation from an asset, as well as assignment of RPI resource types.
         *
         * This class supports assignment of both RPI and RHI types. If an RPI resource is bound at
         * a specific location, the class will hold *both* the RPI and RHI references. On the other
         * hand, if an RHI resource is bound, any previously held RPI resource is *cleared*. Therefore,
         * it's possible that querying for an RPI resource will return null while querying the same
         * location for an RHI resource will return a valid entry.
         *
         * If RHI validation is enabled, the class will perform error checking. If a setter method fails
         * an error is emitted and the call returns false without performing the requested operation.
         * Likewise, if a getter method fails, an error is emitted and an empty value or empty array
         * is returned. If validation is disabled, the operation is always performed.
         */
        class JE_RENDER_INTEGRATION_API ShaderResourceGroup final
            : public AZ::Data::InstanceData
        {
            friend class ShaderSystem;
            friend class ShaderResourceGroupPool;

        public:
            AZ_INSTANCE_DATA(ShaderResourceGroup, "{88B52D0C-9CBF-4B4D-B9E2-180BA602E1EA}");
            AZ_CLASS_ALLOCATOR(ShaderResourceGroup, AZ::SystemAllocator);

            /// Instantiates a unique shader resource group instance using its paired asset but with a random InstanceId.
            /// This version uses the system-wide supervariant (if available) specified by the ShaderSystem or the Default supervariant.
            static AZ::Data::Instance<ShaderResourceGroup> Create(
                const AZ::Data::Asset<ShaderAsset>& shaderAsset, const AZ::Name& srgName);

            /// Instantiates a unique shader resource group instance using its paired asset but with a random InstanceId.
            /// This version uses the supervariant specified by the caller.
            static AZ::Data::Instance<ShaderResourceGroup> Create(
                const AZ::Data::Asset<ShaderAsset>& shaderAsset, const SupervariantIndex& supervariantIndex, const AZ::Name& srgName);

            /// Queues a request that the underlying hardware shader resource group be compiled.
            void Compile();

            /// Returns whether the group is currently queued for compilation.
            bool IsQueuedForCompile() const;

            bool IsInitialized() const;

            /// Finds the shader input index from the shader input name for each type of resource.
            AZ::RHI::ShaderInputBufferIndex    FindShaderInputBufferIndex(const AZ::Name& name) const;
            AZ::RHI::ShaderInputImageIndex     FindShaderInputImageIndex(const AZ::Name& name) const;
            AZ::RHI::ShaderInputSamplerIndex   FindShaderInputSamplerIndex(const AZ::Name& name) const;
            AZ::RHI::ShaderInputConstantIndex  FindShaderInputConstantIndex(const AZ::Name& name) const;

            AZ::RHI::ShaderInputBufferUnboundedArrayIndex FindShaderInputBufferUnboundedArrayIndex(const AZ::Name& name) const;
            AZ::RHI::ShaderInputImageUnboundedArrayIndex  FindShaderInputImageUnboundedArrayIndex(const AZ::Name& name) const;

            /// Returns the parent shader shader asset where the SRG layout data came from.
            /// const Data::Asset<ShaderAsset>& GetAsset() const;

            /// Returns the RHI shader resource group layout.
            const AZ::RHI::ShaderResourceGroupLayout* GetLayout() const;

            /// Returns the underlying RHI shader resource group.
            AZ::RHI::ShaderResourceGroup* GetRHIShaderResourceGroup();

            //////////////////////////////////////////////////////////////////////////
            // Methods for assignment / access of RPI Image types.

            /// Sets the ShaderVariantKey value as constant data. Returns false if this SRG is not designated as fallback.
            bool SetShaderVariantKeyFallbackValue(const ShaderVariantKey& shaderKey);

            /// Returns true if the ShaderResourceGroup has been designated as a ShaderVariantKey fallback
            bool HasShaderVariantKeyFallbackEntry() const;

            /// Sets one RPI image for the given shader input index.
            bool SetImage(AZ::RHI::ShaderInputNameIndex& inputIndex, const AZ::Data::Instance<Image>& image, uint32_t arrayIndex = 0);
            bool SetImage(AZ::RHI::ShaderInputImageIndex inputIndex, const AZ::Data::Instance<Image>& image, uint32_t arrayIndex = 0);

            /// Sets multiple RPI images for the given shader input index.
            bool SetImageArray(AZ::RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const AZ::Data::Instance<Image>> images, uint32_t arrayIndex = 0);
            bool SetImageArray(AZ::RHI::ShaderInputImageIndex inputIndex, AZStd::span<const AZ::Data::Instance<Image>> images, uint32_t arrayIndex = 0);

            /// Returns a single RPI image associated with the image shader input index and array offset.
            const AZ::Data::Instance<Image>& GetImage(AZ::RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex = 0) const;
            const AZ::Data::Instance<Image>& GetImage(AZ::RHI::ShaderInputImageIndex inputIndex, uint32_t arrayIndex = 0) const;

            /// Returns a span of RPI images associated with the image shader input index.
            AZStd::span<const AZ::Data::Instance<Image>> GetImageArray(AZ::RHI::ShaderInputNameIndex& inputIndex) const;
            AZStd::span<const AZ::Data::Instance<Image>> GetImageArray(AZ::RHI::ShaderInputImageIndex inputIndex) const;

            //////////////////////////////////////////////////////////////////////////
            // Methods for assignment / access of RPI Buffer types.

            /// Sets one RPI buffer for the given shader input index.
            bool SetBuffer(AZ::RHI::ShaderInputNameIndex& inputIndex, const AZ::Data::Instance<Buffer>& buffer, uint32_t arrayIndex = 0);
            bool SetBuffer(AZ::RHI::ShaderInputBufferIndex inputIndex, const AZ::Data::Instance<Buffer>& buffer, uint32_t arrayIndex = 0);

            /// Sets multiple RPI buffers for the given shader input index.
            bool SetBufferArray(AZ::RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const AZ::Data::Instance<Buffer>> buffers, uint32_t arrayIndex = 0);
            bool SetBufferArray(AZ::RHI::ShaderInputBufferIndex inputIndex, AZStd::span<const AZ::Data::Instance<Buffer>> buffers, uint32_t arrayIndex = 0);

            /// Returns a single RPI buffer associated with the buffer shader input index and array offset.
            const AZ::Data::Instance<Buffer>& GetBuffer(AZ::RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex = 0) const;
            const AZ::Data::Instance<Buffer>& GetBuffer(AZ::RHI::ShaderInputBufferIndex inputIndex, uint32_t arrayIndex = 0) const;

            /// Returns a span of RPI buffers associated with the buffer shader input index.
            AZStd::span<const AZ::Data::Instance<Buffer>> GetBufferArray(AZ::RHI::ShaderInputNameIndex& inputIndex) const;
            AZStd::span<const AZ::Data::Instance<Buffer>> GetBufferArray(AZ::RHI::ShaderInputBufferIndex inputIndex) const;

            //! Reset image and buffer views so that it won't hold references for any RHI resources
            void ResetViews();

            //////////////////////////////////////////////////////////////////////////
            // Methods for assignment / access of RHI Image types.

            /// Sets one image view for the given shader input index.
            bool SetImageView(AZ::RHI::ShaderInputNameIndex& inputIndex, const AZ::RHI::ImageView *imageView, uint32_t arrayIndex = 0);
            bool SetImageView(AZ::RHI::ShaderInputImageIndex inputIndex, const AZ::RHI::ImageView* imageView, uint32_t arrayIndex = 0);

            /// Sets an array of image view for the given shader input index.
            bool SetImageViewArray(AZ::RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const AZ::RHI::ImageView* const> imageViews, uint32_t arrayIndex = 0);
            bool SetImageViewArray(AZ::RHI::ShaderInputImageIndex inputIndex, AZStd::span<const AZ::RHI::ImageView* const> imageViews, uint32_t arrayIndex = 0);

            /// Sets an unbounded array of image views for the given shader input index.
            bool SetImageViewUnboundedArray(AZ::RHI::ShaderInputImageUnboundedArrayIndex inputIndex, AZStd::span<const AZ::RHI::ImageView * const> imageViews);

            /// Update the indirect buffer view with the indices of all the image views which reside in the global gpu heap.
            void SetBindlessViews(
                AZ::RHI::ShaderInputBufferIndex indirectResourceBufferIndex,
                const AZ::RHI::BufferView* indirectResourceBuffer,
                AZStd::span<const AZ::RHI::ImageView* const> imageViews,
                AZStd::unordered_map<int, uint32_t*> outIndices,
                AZStd::span<bool> isViewReadOnly,
                uint32_t arrayIndex = 0);

            /// Returns a single image view associated with the image shader input index and array offset.
            const AZ::RHI::ConstPtr<AZ::RHI::ImageView>& GetImageView(AZ::RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex = 0) const;
            const AZ::RHI::ConstPtr<AZ::RHI::ImageView>& GetImageView(AZ::RHI::ShaderInputImageIndex inputIndex, uint32_t arrayIndex = 0) const;

            /// Returns a span of image views associated with the given image shader input index.
            AZStd::span<const AZ::RHI::ConstPtr<AZ::RHI::ImageView>> GetImageViewArray(AZ::RHI::ShaderInputNameIndex& inputIndex) const;
            AZStd::span<const AZ::RHI::ConstPtr<AZ::RHI::ImageView>> GetImageViewArray(AZ::RHI::ShaderInputImageIndex inputIndex) const;

            //////////////////////////////////////////////////////////////////////////
            // Methods for assignment / access of RHI Buffer types.

            /// Sets one buffer view for the given shader input index.
            bool SetBufferView(AZ::RHI::ShaderInputNameIndex& inputIndex, const AZ::RHI::BufferView* bufferView, uint32_t arrayIndex = 0);
            bool SetBufferView(AZ::RHI::ShaderInputBufferIndex inputIndex, const AZ::RHI::BufferView* bufferView, uint32_t arrayIndex = 0);

            /// Sets an array of buffer view for the given shader input index.
            bool SetBufferViewArray(AZ::RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const AZ::RHI::BufferView * const> bufferViews, uint32_t arrayIndex = 0);
            bool SetBufferViewArray(AZ::RHI::ShaderInputBufferIndex inputIndex, AZStd::span<const AZ::RHI::BufferView* const> bufferViews, uint32_t arrayIndex = 0);

            /// Sets an unbounded array of buffer views for the given shader input index.
            bool SetBufferViewUnboundedArray(AZ::RHI::ShaderInputBufferUnboundedArrayIndex inputIndex, AZStd::span<const AZ::RHI::BufferView* const> bufferViews);

            /// Update the indirect buffer view with the indices of all the buffer views which reside in the global gpu heap.
            void SetBindlessViews(
                AZ::RHI::ShaderInputBufferIndex indirectResourceBufferIndex,
                const AZ::RHI::BufferView* indirectResourceBuffer,
                AZStd::span<const AZ::RHI::BufferView* const> bufferViews,
                AZStd::unordered_map<int, uint32_t*> outIndices,
                AZStd::span<bool> isViewReadOnly,
                uint32_t arrayIndex = 0);

            /// Returns a single buffer view associated with the buffer shader input index and array offset.
            const AZ::RHI::ConstPtr<AZ::RHI::BufferView>& GetBufferView(AZ::RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex = 0) const;
            const AZ::RHI::ConstPtr<AZ::RHI::BufferView>& GetBufferView(AZ::RHI::ShaderInputBufferIndex inputIndex, uint32_t arrayIndex = 0) const;

            /// Returns a span of buffer views associated with the given buffer shader input index.
            AZStd::span<const AZ::RHI::ConstPtr<AZ::RHI::BufferView>> GetBufferViewArray(AZ::RHI::ShaderInputNameIndex& inputIndex) const;
            AZStd::span<const AZ::RHI::ConstPtr<AZ::RHI::BufferView>> GetBufferViewArray(AZ::RHI::ShaderInputBufferIndex inputIndex) const;

            //////////////////////////////////////////////////////////////////////////
            // Methods for assignment / access of RHI Sampler types.

            /// Sets one sampler for the given shader input index, using the bindingIndex as the key.
            bool SetSampler(AZ::RHI::ShaderInputNameIndex& inputIndex, const AZ::RHI::SamplerState& sampler, uint32_t arrayIndex = 0);
            bool SetSampler(AZ::RHI::ShaderInputSamplerIndex inputIndex, const AZ::RHI::SamplerState& sampler, uint32_t arrayIndex = 0);

            /// Sets an array of samplers for the given shader input index.
            bool SetSamplerArray(AZ::RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const AZ::RHI::SamplerState> samplers, uint32_t arrayIndex = 0);
            bool SetSamplerArray(AZ::RHI::ShaderInputSamplerIndex inputIndex, AZStd::span<const AZ::RHI::SamplerState> samplers, uint32_t arrayIndex = 0);

            /// Returns a single sampler associated with the sampler shader input index and array offset.
            const AZ::RHI::SamplerState& GetSampler(AZ::RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex) const;
            const AZ::RHI::SamplerState& GetSampler(AZ::RHI::ShaderInputSamplerIndex inputIndex, uint32_t arrayIndex) const;

            /// Returns a span of samplers associated with the sampler shader input index.
            AZStd::span<const AZ::RHI::SamplerState> GetSamplerArray(AZ::RHI::ShaderInputNameIndex& inputIndex) const;
            AZStd::span<const AZ::RHI::SamplerState> GetSamplerArray(AZ::RHI::ShaderInputSamplerIndex inputIndex) const;

            //////////////////////////////////////////////////////////////////////////
            // Methods for assignment / access SRG constants.

            /// Assigns constant data for the given constant shader input index.
            bool SetConstantRaw(AZ::RHI::ShaderInputNameIndex& inputIndex, const void* bytes, uint32_t byteCount);
            bool SetConstantRaw(AZ::RHI::ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteCount);
            bool SetConstantRaw(AZ::RHI::ShaderInputNameIndex& inputIndex, const void* bytes, uint32_t byteOffset, uint32_t byteCount);
            bool SetConstantRaw(AZ::RHI::ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteOffset, uint32_t byteCount);

            /// Assigns a value of type T to the constant shader input.
            template <typename T>
            bool SetConstant(AZ::RHI::ShaderInputNameIndex& inputIndex, const T& value);
            template <typename T>
            bool SetConstant(AZ::RHI::ShaderInputConstantIndex inputIndex, const T& value);

            /// Assign a device-specific value of type T to the constant shader input. Note that the corresponding GetConstant() - function
            /// returns only the value of one device.
            template<typename T>
            bool SetConstant(AZ::RHI::ShaderInputNameIndex& inputIndex, const AZStd::unordered_map<int, T>& values);

            template<typename T>
            bool SetConstant(AZ::RHI::ShaderInputConstantIndex& inputIndex, const AZStd::unordered_map<int, T>& values);

            /// Assigns the specified number of rows from a Matrix
            template <typename T>
            bool SetConstantMatrixRows(AZ::RHI::ShaderInputNameIndex& inputIndex, const T& value, uint32_t rowCount);
            template <typename T>
            bool SetConstantMatrixRows(AZ::RHI::ShaderInputConstantIndex inputIndex, const T& value, uint32_t rowCount);

            /// Assigns a value of type T to the constant shader input, at an array offset.
            template <typename T>
            bool SetConstant(AZ::RHI::ShaderInputNameIndex& inputIndex, const T& value, uint32_t arrayIndex);
            template <typename T>
            bool SetConstant(AZ::RHI::ShaderInputConstantIndex inputIndex, const T& value, uint32_t arrayIndex);

            /// Assigns a span of type T to the constant shader input.
            template <typename T>
            bool SetConstantArray(AZ::RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const T> values);
            template <typename T>
            bool SetConstantArray(AZ::RHI::ShaderInputConstantIndex inputIndex, AZStd::span<const T> values);

            /// Assigns an array of type T to the constant shader input.
            template <typename T, size_t N>
            bool SetConstantArray(AZ::RHI::ShaderInputNameIndex& inputIndex, const AZStd::array<T, N>& values);
            template <typename T, size_t N>
            bool SetConstantArray(AZ::RHI::ShaderInputConstantIndex inputIndex, const AZStd::array<T, N>& values);

            /// Assigns an array of shader data mappings of type T
            template<typename T>
            bool ApplyDataMappingArray(const AZStd::vector<T>& mappings);

            /// Assigns a group of shader data mapping arrays
            bool ApplyDataMappings(const AZ::RHI::ShaderDataMappings& mappings);

            /**
             * Returns constant data for the given shader input index as a template type.
             * The stride of T must match the size of the constant input region. The number
             * of elements in the returned array is the number of evenly divisible elements.
             * If the strides do not match, an empty array is returned.
             */
            template <typename T>
            AZStd::span<const T> GetConstantArray(AZ::RHI::ShaderInputNameIndex& inputIndex) const;
            template <typename T>
            AZStd::span<const T> GetConstantArray(AZ::RHI::ShaderInputConstantIndex inputIndex) const;

            /**
             * Returns the constant data as type 'T' returned by value. The size of the constant region
             * must match the size of T exactly. Otherwise, an empty instance is returned.
             */
            template <typename T>
            T GetConstant(AZ::RHI::ShaderInputNameIndex& inputIndex) const;
            template <typename T>
            T GetConstant(AZ::RHI::ShaderInputConstantIndex inputIndex) const;

            /**
             * Treats the constant input as an array of type T, returning the element by value at the
             * specified array index. The size of the constant region must equally partition into an
             * array of type T. Otherwise, an empty instance is returned.
             */
            template <typename T>
            T GetConstant(AZ::RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex) const;
            template <typename T>
            T GetConstant(AZ::RHI::ShaderInputConstantIndex inputIndex, uint32_t arrayIndex) const;

            /// Returns constant data for the given shader input index as a span of bytes.
            AZStd::span<const uint8_t> GetConstantRaw(AZ::RHI::ShaderInputNameIndex& inputIndex) const;
            AZStd::span<const uint8_t> GetConstantRaw(AZ::RHI::ShaderInputConstantIndex inputIndex) const;

            /**
            * Copies the Shader Resource Group Data from another ShaderResourceGroup. It only copies the
            * Shader Inputs that are present in both ShaderResourceGroups.
            * Return true if all values from the ShaderResourceGroup were updated. False means that some
            * shader inputs were not present in the "other" ShaderResourceGroup.
            */
            bool CopyShaderResourceGroupData(const ShaderResourceGroup& other);

            /**
            * Returns the ShaderAsset that this ShaderResourceGroup is using.
            */
            const AZ::Data::Asset<ShaderAsset>& GetShaderAsset() const;

            /**
            * Returns the SuperVariantIndex that this ShaderResourceGroup is using.
            */
            SupervariantIndex GetSupervariantIndex() const;

        private:
            ShaderResourceGroup() = default;

            //! Will be passed as AZStd::any to CreateInternal.
            //! The data comes from the same parameters of ::Create() or ::FindOrCreate().
            struct SrgInitParams
            {
                AZ_TYPE_INFO(SrgInitParams, "{FDBDDB75-3DE6-4383-8D19-C0092246A411}");
                SupervariantIndex m_supervariantIndex;
                AZ::Name m_srgName;
            };

            //! Usually subclasses of AZ::Data::InstanceData leverage the AssetId of the given asset as a means to define
            //! the AZ::Data::InstanceId. This works well when there's a one-to-one relationship between the Asset and the InstanceData.
            //!
            //! ShaderResourceGroup & ShaderResourceGroupPool are different because one ShaderAsset can have several
            //! ShaderResourceGroupLayouts defined in it. This means that using only the AssetId is not sufficient.
            //!
            //! This function searches the ShaderResourceGroupLayout of the given @srgName in the @shaderAsset. If it finds such
            //! ShaderResourceGroupLayout it makes an InstanceId based on:
            //! - The azsl file of origin where the ShaderResourceGroup was defined.
            //! - The supervariant index.
            //! - The name of the srg.
            //! @param shaderAsset: The shader asset where the ShaderResourceGroupLayout will be searched.
            //! @param supervariantIndex: The supervariant index in @shaderAsset where the search will be conducted.
            //! @param srgName: Name of the ShaderResourceGroup as it was declared in the azsl file of origin.
            static AZ::Data::InstanceId MakeSrgPoolInstanceId(const AZ::Data::Asset<ShaderAsset>& shaderAsset, const SupervariantIndex& supervariantIndex, const AZ::Name& srgName);

            AZ::RHI::ResultCode Init(ShaderAsset& shaderAsset, const SupervariantIndex& supervariantIndex, const AZ::Name& srgName);

            static AZ::Data::Instance<ShaderResourceGroup> CreateInternal(ShaderAsset& shaderAsset, const AZStd::any* srgInitParams);

            /// A name to be used in error messages
            static constexpr const char* s_traceCategoryName = "ShaderResourceGroup";

            /// Allows us to return const& to a null Image
            static const AZ::Data::Instance<Image> s_nullImage;

            /// Allows us to return const& to a null Buffer
            static const AZ::Data::Instance<Buffer> s_nullBuffer;

            /// If true, Init() was called and was successful.
            bool m_isInitialized = false;

            /// Pool for allocating RHI::ShaderResourceGroup objects
            AZ::Data::Instance<ShaderResourceGroupPool> m_pool;

            /// The shader resource group data that is manipulated by this class
            AZ::RHI::ShaderResourceGroupData m_data;

            /// The shader resource group that can be submitted to the renderer
            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroup> m_shaderResourceGroup;

            /// A reference to the SRG asset used to initialize and manipulate this group.
            AZ::Data::Asset<ShaderAsset> m_asset;

            /// A pointer to the layout inside of m_srgAsset
            const AZ::RHI::ShaderResourceGroupLayout* m_layout = nullptr;

            /**
             * The set of images currently bound. The shader resource group maintains these references to
             * keep the hardware resource in memory, manage streaming operations, and support reload operations.
             * However, entries remain null when RHI image views are bound.
             */
            AZStd::vector<AZ::Data::Instance<Image>> m_imageGroup;

            /**
             * The set of buffers currently bound. The shader resource group maintains these references to
             * keep the hardware resource in memory, manage streaming operations, and support reload operations.
             * However, entries remain null when RHI buffer views are bound.
             */
            AZStd::vector<AZ::Data::Instance<Buffer>> m_bufferGroup;

            /// SupervariantIndex used for initializing the SRG.
            SupervariantIndex m_supervariantIndex;
        };

        template<typename T>
        bool ShaderResourceGroup::ApplyDataMappingArray(const AZStd::vector<T>& mappings)
        {
            bool success = true;
            for (const auto& mapping : mappings)
            {
                auto index = m_layout->FindShaderInputConstantIndex(mapping.m_name);
                success = success && SetConstant(index, mapping.m_value);
            }
            return success;
        }

        template <typename T>
        bool ShaderResourceGroup::SetConstant(AZ::RHI::ShaderInputConstantIndex inputIndex, const T& value)
        {
            return m_data.SetConstant(inputIndex, value);
        }

        template <typename T>
        bool ShaderResourceGroup::SetConstant(AZ::RHI::ShaderInputNameIndex& inputIndex, const T& value)
        {
            if (inputIndex.ValidateOrFindConstantIndex(GetLayout()))
            {
                return SetConstant(inputIndex.GetConstantIndex(), value);
            }
            return false;
        }

        template<typename T>
        bool ShaderResourceGroup::SetConstant(AZ::RHI::ShaderInputConstantIndex& inputIndex, const AZStd::unordered_map<int, T>& values)
        {
            return m_data.SetConstant(inputIndex, values);
        }

        template<typename T>
        bool ShaderResourceGroup::SetConstant(AZ::RHI::ShaderInputNameIndex& inputIndex, const AZStd::unordered_map<int, T>& values)
        {
            if (inputIndex.ValidateOrFindConstantIndex(GetLayout()))
            {
                return SetConstant(inputIndex.GetConstantIndex(), values);
            }
            return false;
        }

        template <typename T>
        bool ShaderResourceGroup::SetConstantMatrixRows(AZ::RHI::ShaderInputConstantIndex inputIndex, const T& value, uint32_t rowCount)
        {
            return m_data.SetConstantMatrixRows(inputIndex, value, rowCount);
        }

        template <typename T>
        bool ShaderResourceGroup::SetConstantMatrixRows(AZ::RHI::ShaderInputNameIndex& inputIndex, const T& value, uint32_t rowCount)
        {
            if (inputIndex.ValidateOrFindConstantIndex(GetLayout()))
            {
                return SetConstantMatrixRows(inputIndex.GetConstantIndex(), value, rowCount);
            }
            return false;
        }

        template <typename T>
        bool ShaderResourceGroup::SetConstant(AZ::RHI::ShaderInputConstantIndex inputIndex, const T& value, uint32_t arrayIndex)
        {
            return m_data.SetConstant(inputIndex, value, arrayIndex);
        }

        template <typename T>
        bool ShaderResourceGroup::SetConstant(AZ::RHI::ShaderInputNameIndex& inputIndex, const T& value, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindConstantIndex(GetLayout()))
            {
                return SetConstant(inputIndex.GetConstantIndex(), value, arrayIndex);
            }
            return false;
        }

        template <typename T>
        bool ShaderResourceGroup::SetConstantArray(AZ::RHI::ShaderInputConstantIndex inputIndex, AZStd::span<const T> values)
        {
            return m_data.SetConstantArray(inputIndex, values);
        }

        template <typename T>
        bool ShaderResourceGroup::SetConstantArray(AZ::RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const T> values)
        {
            if (inputIndex.ValidateOrFindConstantIndex(GetLayout()))
            {
                return SetConstantArray(inputIndex.GetConstantIndex(), values);
            }
            return false;
        }

        template <typename T, size_t N>
        bool ShaderResourceGroup::SetConstantArray(AZ::RHI::ShaderInputConstantIndex inputIndex, const AZStd::array<T, N>& values)
        {
            return SetConstantArray(inputIndex, AZStd::span<const T>(values));
        }

        template <typename T, size_t N>
        bool ShaderResourceGroup::SetConstantArray(AZ::RHI::ShaderInputNameIndex& inputIndex, const AZStd::array<T, N>& values)
        {
            if (inputIndex.ValidateOrFindConstantIndex(GetLayout()))
            {
                return SetConstantArray(inputIndex.GetConstantIndex(), AZStd::span<const T>(values));
            }
            return false;
        }

        template <typename T>
        AZStd::span<const T> ShaderResourceGroup::GetConstantArray(AZ::RHI::ShaderInputConstantIndex inputIndex) const
        {
            return m_data.GetConstantArray<T>(inputIndex);
        }

        template <typename T>
        AZStd::span<const T> ShaderResourceGroup::GetConstantArray(AZ::RHI::ShaderInputNameIndex& inputIndex) const
        {
            if (inputIndex.ValidateOrFindConstantIndex(GetLayout()))
            {
                return GetConstantArray<T>(inputIndex.GetConstantIndex());
            }
            return false;
        }

        template <typename T>
        T ShaderResourceGroup::GetConstant(AZ::RHI::ShaderInputConstantIndex inputIndex) const
        {
            return m_data.GetConstant<T>(inputIndex);
        }

        template <typename T>
        T ShaderResourceGroup::GetConstant(AZ::RHI::ShaderInputNameIndex& inputIndex) const
        {
            if (inputIndex.ValidateOrFindConstantIndex(GetLayout()))
            {
                return GetConstant<T>(inputIndex.GetConstantIndex());
            }
            return false;
        }

        template <typename T>
        T ShaderResourceGroup::GetConstant(AZ::RHI::ShaderInputConstantIndex inputIndex, uint32_t arrayIndex) const
        {
            return m_data.GetConstant<T>(inputIndex, arrayIndex);
        }

        template <typename T>
        T ShaderResourceGroup::GetConstant(AZ::RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex) const
        {
            if (inputIndex.ValidateOrFindConstantIndex(GetLayout()))
            {
                return GetConstant<T>(inputIndex.GetConstantIndex(), arrayIndex);
            }
            return false;
        }
    }

namespace AZ
{ // namespace RPI
} // namespace AZ
