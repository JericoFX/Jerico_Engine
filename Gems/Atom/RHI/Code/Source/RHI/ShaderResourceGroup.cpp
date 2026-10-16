/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI/ShaderResourceGroupPool.h>

namespace AZ::RHI
{
    void ShaderResourceGroup::Compile(
        const ShaderResourceGroupData& groupData, CompileMode compileMode /*= CompileMode::Async*/)
    {
        m_data = groupData;

        // Reload can replace the device images while the multi-device image views stay bound.
        for (uint32_t imageInputIndex = 0; imageInputIndex < m_data.GetLayout()->GetShaderInputListForImages().size(); ++imageInputIndex)
        {
            const ShaderInputImageIndex inputIndex(imageInputIndex);
            const auto imageViews = m_data.GetImageViewArray(inputIndex);
            for (uint32_t arrayIndex = 0; arrayIndex < imageViews.size(); ++arrayIndex)
            {
                const ImageView* imageView = imageViews[arrayIndex].get();
                bool needsRefresh = false;
                IterateObjects<DeviceShaderResourceGroup>([&](auto deviceIndex, [[maybe_unused]] auto deviceShaderResourceGroup)
                {
                    const DeviceImageView* currentView = imageView && imageView->GetImage()->IsDeviceSet(deviceIndex)
                        ? imageView->GetDeviceImageView(deviceIndex).get()
                        : nullptr;
                    needsRefresh |= m_data.GetDeviceShaderResourceGroupData(deviceIndex).GetImageView(inputIndex, arrayIndex).get()
                        != currentView;
                });
                if (needsRefresh)
                {
                    m_data.SetImageView(inputIndex, imageView, arrayIndex);
                }
            }
        }

        IterateObjects<DeviceShaderResourceGroup>([this, compileMode](auto deviceIndex, auto deviceShaderResourceGroup)
        {
            deviceShaderResourceGroup->Compile(m_data.GetDeviceShaderResourceGroupData(deviceIndex), compileMode);
        });
    }

    uint32_t ShaderResourceGroup::GetBindingSlot() const
    {
        return m_bindingSlot;
    }

    bool ShaderResourceGroup::IsQueuedForCompile() const
    {
        return IterateObjects<DeviceShaderResourceGroup>([]([[maybe_unused]] auto deviceIndex, auto deviceShaderResourceGroup)
        {
            if (deviceShaderResourceGroup->IsQueuedForCompile())
            {
                return ResultCode::Fail;
            }

            return ResultCode::Success;
        }) == ResultCode::Fail;
    }

    const ShaderResourceGroupPool* ShaderResourceGroup::GetPool() const
    {
        return static_cast<const ShaderResourceGroupPool*>(Resource::GetPool());
    }

    ShaderResourceGroupPool* ShaderResourceGroup::GetPool()
    {
        return static_cast<ShaderResourceGroupPool*>(Resource::GetPool());
    }

    const ShaderResourceGroupData& ShaderResourceGroup::GetData() const
    {
        return m_data;
    }

    void ShaderResourceGroup::Shutdown()
    {
        Resource::Shutdown();
    }
} // namespace AZ::RHI
