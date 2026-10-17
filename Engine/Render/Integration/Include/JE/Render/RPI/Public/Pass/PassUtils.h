/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// This header file is for declaring types used for RPI System classes to avoid recursive includes
 
#include <JE/Render/RPI/Public/Base.h>
#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/Pass/Pass.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>

#include <JE/Render/RPI/Reflect/Pass/PassData.h>
#include <JE/Render/RPI/Reflect/Pass/PassDescriptor.h>
#include <JE/Render/RPI/Reflect/Pass/PassRequest.h>
#include <JE/Render/RPI/Reflect/Pass/PassTemplate.h>

namespace JE::Render
{
    namespace RPI
    {
        namespace PassUtils
        {
            //! Function for applying shader data mappings from a PassDescriptor to a shader resource group
            JE_RENDER_INTEGRATION_API bool BindDataMappingsToSrg(const PassDescriptor& descriptor, ShaderResourceGroup* shaderResourceGroup);

            //! Retrieves PassData from a PassDescriptor
            JE_RENDER_INTEGRATION_API const PassData* GetPassData(const PassDescriptor& descriptor);

            //! Finds all PipelineGlobalConnections in the descriptor and adds them to the provided list
            JE_RENDER_INTEGRATION_API void ExtractPipelineGlobalConnections(const AZStd::shared_ptr<PassData>& passData, PipelineGlobalConnectionList& outList);

            //! Retrieves PassData from a PassDescriptor
            JE_RENDER_INTEGRATION_API AZStd::shared_ptr<PassData> GetPassDataPtr(const PassDescriptor& descriptor);

            //! Templated function for retrieving specific data types from a PassDescriptor
            template<typename PassDataType>
            const PassDataType* GetPassData(const PassDescriptor& descriptor)
            {
                const PassDataType* passData = nullptr;

                // Try custom data from PassRequest
                if (descriptor.m_passRequest != nullptr)
                {
                    passData = azrtti_cast<const PassDataType*>(descriptor.m_passRequest->m_passData.get());
                }

                // Try custom data from PassTemplate
                if (passData == nullptr && descriptor.m_passTemplate != nullptr)
                {
                    passData = azrtti_cast<const PassDataType*>(descriptor.m_passTemplate->m_passData.get());
                }

                if (passData == nullptr)
                {
                    passData = azrtti_cast<const PassDataType*>(descriptor.m_passData.get());
                }

                return passData;
            }

            JE_RENDER_INTEGRATION_API void SortPassListAscending(AZStd::vector< Ptr<Pass> >& passList);
            JE_RENDER_INTEGRATION_API void SortPassListDescending(AZStd::vector< Ptr<Pass> >& passList);
        }
    }
}

