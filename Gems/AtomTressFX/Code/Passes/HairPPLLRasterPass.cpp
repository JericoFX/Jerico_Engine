/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

//#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/DevicePipelineState.h>

#include <JE/Render/RPI/Public/View.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/RPISystemInterface.h>
#include <JE/Render/RPI/Public/Pass/PassUtils.h>
#include <JE/Render/RPI/Public/Scene.h>

#include <JE/Render/RPI/Reflect/Asset/AssetUtils.h>
#include <JE/Render/RPI/Reflect/Pass/RasterPassData.h>
#include <JE/Render/RPI/Reflect/Pass/PassTemplate.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderAsset.h>

#include <Passes/HairPPLLRasterPass.h>
#include <Rendering/HairRenderObject.h>
#include <Rendering/HairFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            HairPPLLRasterPass::HairPPLLRasterPass(const JE::Render::RPI::PassDescriptor& descriptor)
                : HairGeometryRasterPass(descriptor)
            {
                SetShaderPath("Shaders/hairrenderingfillppll.azshader");  
            }

            JE::Render::RPI::Ptr<HairPPLLRasterPass> HairPPLLRasterPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
            {
                JE::Render::RPI::Ptr<HairPPLLRasterPass> pass = aznew HairPPLLRasterPass(descriptor);
                return pass;
            }

            //! This method is used for attaching the PPLL data buffer which is a transient buffer.
            //! It is done this ways because Atom doesn't support transient structured buffers declaration
            //! via Pass yet.
            //! Once supported, this will be done via data driven code and the method can be removed.
            void HairPPLLRasterPass::BuildInternal()
            {
                HairGeometryRasterPass::BuildInternal(); // change this to call parent if the method exists

                if (!AcquireFeatureProcessor())
                {
                    return;
                }

                // Output
                AttachBufferToSlot(Name{ "PerPixelLinkedList" }, m_featureProcessor->GetPerPixelListBuffer());
            }

            void HairPPLLRasterPass::InitializeInternal()
            {
                if (!LoadShaderAndPipelineState())
                {
                    return;
                }
                HairGeometryRasterPass::InitializeInternal();
            }

        } // namespace Hair
    }   // namespace Render
}   // namespace AZ
