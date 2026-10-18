/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/GeometryView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RHI/DevicePipelineState.h>

#include <JE/Render/RPI/Public/FeatureProcessor.h>
#include <JE/Render/RPI/Public/PipelineState.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>

#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <Atom/RHI.Reflect/Limits.h>

#include <AzCore/std/containers/fixed_vector.h>

#include "AuxGeomBase.h"

namespace AZ
{
    namespace RHI
    {
        class DrawPacketBuilder;
    }

    }

namespace JE::Render::RPI
{
        class Scene;
        class Shader;
        class ShaderVariant;
        class ShaderOptionGroup;
    }

namespace JE::Render
{

    namespace Features
    {
        /**
         * FixedShapeProcessor does the feature processor work for fixed shapes such as
         * Sphere, Cone, Cylinder.
         * This class, manages setting up the shape buffers, the stream layout, the shader asset
         * and the pipeline states.
         */
        class FixedShapeProcessor final
        {
        public:
            using StreamBufferViewsForAllStreams = AZStd::fixed_vector<AZ::RHI::StreamBufferView, AZ::RHI::Limits::Pipeline::StreamCountMax>;

            using AuxGeomNormal = AuxGeomPosition;

            AZ_TYPE_INFO(FixedShapeProcessor, "{20A11645-F8B1-4BAC-847D-F8F49FD2E339}");
            AZ_CLASS_ALLOCATOR(FixedShapeProcessor, AZ::SystemAllocator);

            FixedShapeProcessor() = default;
            ~FixedShapeProcessor() = default;

            //! Initialize the FixedShapeProcessor and all its buffers, shaders, stream layouts etc
            bool Initialize(AZ::RHI::MultiDevice::DeviceMask deviceMask, const JE::Render::RPI::Scene* scene);

            //! Releases the FixedShapeProcessor and all buffers
            void Release();

            //! Processes all the fixed shape objects for a frame
            void ProcessObjects(const AuxGeomBufferData* bufferData, const JE::Render::RPI::FeatureProcessor::RenderPacket& fpPacket);

            //! Prepare frame.
            void PrepareFrame();
                        
            //! Do any cleanup after current frame is rendered.
            void FrameEnd();

            //! Notify this FixedShapeProcessor to update its pipeline states
            void SetUpdatePipelineStates();

        private:

            using LodIndex = uint32_t;
            struct ShaderData; // forward declare internal struct;

            //! We store a struct of this type for each fixed object geometry (both shapes and boxes)
            struct ObjectBuffers
            {
                AZ::RHI::GeometryView m_pointGeometryView{ AZ::RHI::MultiDevice::AllDevices };
                AZ::RHI::GeometryView m_lineGeometryView{ AZ::RHI::MultiDevice::AllDevices };
                AZ::RHI::GeometryView m_triangleGeometryView{ AZ::RHI::MultiDevice::AllDevices };

                AZ::RHI::Ptr<AZ::RHI::Buffer> m_pointIndexBuffer;
                AZ::RHI::Ptr<AZ::RHI::Buffer> m_lineIndexBuffer;
                AZ::RHI::Ptr<AZ::RHI::Buffer> m_triangleIndexBuffer;

                AZ::RHI::Ptr<AZ::RHI::Buffer> m_positionBuffer;
                AZ::RHI::Ptr<AZ::RHI::Buffer> m_normalBuffer;
            };

            // This is a temporary structure used when building object meshes. The data is then copied into RHI buffers.
            struct MeshData
            {
                AZStd::vector<uint16_t> m_pointIndices; // Use indices because draws are all indexed.
                AZStd::vector<uint16_t> m_lineIndices;
                AZStd::vector<uint16_t> m_triangleIndices;
                AZStd::vector<AuxGeomPosition> m_positions;
                AZStd::vector<AuxGeomNormal> m_normals;
            };

            struct Shape
            {
                LodIndex m_numLods;
                AZStd::vector<ObjectBuffers> m_lodBuffers;
                AZStd::vector<float> m_lodScreenPercentages;
            };

            struct PipelineStateOptions
            {
                AuxGeomShapePerpectiveType m_perpectiveType = PerspectiveType_ViewProjection;
                AuxGeomBlendMode m_blendMode = BlendMode_Alpha;
                AuxGeomDrawStyle m_drawStyle = DrawStyle_Line;
                AuxGeomDepthReadType m_depthReadType = DepthRead_On;
                AuxGeomDepthWriteType m_depthWriteType = DepthWrite_Off;
                AuxGeomFaceCullMode m_faceCullMode = FaceCull_Back;
            };

        private: // functions

            enum class Facing
            {
                Up,
                Down,
                Both,
            };

            bool CreateSphereBuffersAndViews(AuxGeomShapeType sphereShapeType);
            void CreateSphereMeshData(MeshData& meshData, uint32_t numRings, uint32_t numSections, AuxGeomShapeType sphereShapeType);

            bool CreateQuadBuffersAndViews();
            void CreateQuadMeshDataSide(MeshData& meshData, bool isUp, bool drawLines);
            void CreateQuadMeshData(MeshData& meshData, Facing facing = Facing::Up);

            bool CreateDiskBuffersAndViews();
            void CreateDiskMeshDataSide(MeshData& meshData, uint32_t numSections, bool isUp, float yPosition);
            void CreateDiskMeshData(MeshData& meshData, uint32_t numSections, Facing facing = Facing::Up, float yPosition = 0.0f);

            bool CreateConeBuffersAndViews();
            void CreateConeMeshData(MeshData& meshData, uint32_t numRings, uint32_t numSections);

            bool CreateCylinderBuffersAndViews(AuxGeomShapeType cylinderShapeType);
            void CreateCylinderMeshData(MeshData& meshData, uint32_t numSections, AuxGeomShapeType cylinderShapeType);

            bool CreateBoxBuffersAndViews();
            void CreateBoxMeshData(MeshData& meshData);

            bool CreateBuffersAndViews(ObjectBuffers& objectBuffers, const MeshData& meshData);

            LodIndex GetLodIndexForShape(AuxGeomShapeType shapeType, const JE::Render::RPI::View* view, const AZ::Vector3& worldPosition, const AZ::Vector3& scale);

            void SetupInputStreamLayout(AZ::RHI::InputStreamLayout& inputStreamLayout, AZ::RHI::PrimitiveTopology topology, bool includeNormals);

            void LoadShaders();
            void FillShaderData(AZ::Data::Instance<JE::Render::RPI::Shader>& shader, ShaderData& shaderData);

            void InitPipelineState(const PipelineStateOptions& options);
            JE::Render::RPI::Ptr<JE::Render::RPI::PipelineStateForDraw>& GetPipelineState(const PipelineStateOptions& pipelineStateOptions);

            AZ::RHI::GeometryView* GetGeometryView(ObjectBuffers& objectBuffers, int drawStyle);
            AZ::RHI::GeometryView* GetGeometryView(AuxGeomShapeType shapeType, int drawStyle, LodIndex lodIndex);
            AZ::RHI::GeometryView* GetBoxGeometryView(int drawStyle);

            //! Uses the given drawPacketBuilder to build a draw packet for given shape and state and returns it
            AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> BuildDrawPacketForShape(
                AZ::RHI::DrawPacketBuilder& drawPacketBuilder,
                const ShapeBufferEntry& shape,
                int drawStyle,
                const AZStd::vector<AZ::Matrix4x4>& viewProjOverrides,
                const JE::Render::RPI::Ptr<JE::Render::RPI::PipelineStateForDraw>& pipelineState,
                LodIndex lodIndex,
                AZ::RHI::DrawItemSortKey sortKey = 0);

            //! Uses the given drawPacketBuilder to build a draw packet for given box and state and returns it
            AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> BuildDrawPacketForBox(
                AZ::RHI::DrawPacketBuilder& drawPacketBuilder,
                const BoxBufferEntry& box,
                int drawStyle,
                const AZStd::vector<AZ::Matrix4x4>& overrideViewProjMatrices,
                const JE::Render::RPI::Ptr<JE::Render::RPI::PipelineStateForDraw>& pipelineState,
                AZ::RHI::DrawItemSortKey sortKey = 0);

            //! Uses the given drawPacketBuilder to build a draw packet with the given data
            AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> BuildDrawPacket(
                AZ::RHI::DrawPacketBuilder& drawPacketBuilder,
                AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& srg,
                AZ::RHI::GeometryView* geometryView,
                AZ::RHI::DrawListTag drawListTag,
                const AZ::RHI::PipelineState* pipelineState,
                AZ::RHI::DrawItemSortKey sortKey,
                int drawStyle);

        private: // data

            //! The buffer pool that manages the index and vertex buffers for each shape
            AZ::RHI::Ptr<AZ::RHI::BufferPool> m_bufferPool;

            //! The descriptor for drawing an object of each draw style using predefined streams
            AZ::RHI::InputStreamLayout m_objectStreamLayout[DrawStyle_Count];
            
            //! Array of shape buffers for all shapes
            AZStd::array<Shape, ShapeType_Count> m_shapes;

            ObjectBuffers m_boxBuffers;

            // not sure what the required lifetime of these is
            AZStd::vector<AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>> m_processSrgs;

            // The PSOs generated by this feature processor
            JE::Render::RPI::Ptr<JE::Render::RPI::PipelineStateForDraw> m_pipelineStates[PerspectiveType_Count][BlendMode_Count][DrawStyle_Count][DepthRead_Count][DepthWrite_Count][FaceCull_Count];
            AZStd::list<JE::Render::RPI::Ptr<JE::Render::RPI::PipelineStateForDraw>*> m_createdPipelineStates;
            AZ::Data::Instance<JE::Render::RPI::Shader> m_unlitShader;
            AZ::Data::Instance<JE::Render::RPI::Shader> m_litShader;

            enum ShapeLightingStyle
            {
                ShapeLightingStyle_ConstantColor, // color from srg
                ShapeLightingStyle_Directional, // color from srg * dot product(normal, hard coded direction)

                ShapeLightingStyle_Count
            };

            struct ShaderData
            {
                AZ::Data::Asset<JE::Render::RPI::ShaderAsset> m_shaderAsset; // For @m_perObjectSrgLayout.
                JE::Render::RPI::SupervariantIndex m_supervariantIndex; // For @m_perObjectSrgLayout.
                AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_perObjectSrgLayout; // Comes from @m_shaderAsset
                AZ::RHI::DrawListTag m_drawListTag;
                AZ::RHI::ShaderInputNameIndex m_colorIndex = "m_color";
                AZ::RHI::ShaderInputNameIndex m_modelToWorldIndex = "m_modelToWorld";
                AZ::RHI::ShaderInputNameIndex m_normalMatrixIndex = "m_normalMatrix";
                AZ::RHI::ShaderInputNameIndex m_viewProjectionOverrideIndex = "m_viewProjectionOverride";
                AZ::RHI::ShaderInputNameIndex m_pointSizeIndex = "m_pointSize";
            };
            ShaderData m_perObjectShaderData[ShapeLightingStyle_Count];
            ShaderData& GetShaderDataForDrawStyle(int drawStyle) {return m_perObjectShaderData[drawStyle == DrawStyle_Shaded];}

            AZStd::vector<AZ::RHI::ConstPtr<AZ::RHI::DrawPacket>> m_drawPackets;

            const JE::Render::RPI::Scene* m_scene = nullptr;

            bool m_needUpdatePipelineStates = false;
        };
    } // namespace Features
} // namespace JE::Render
