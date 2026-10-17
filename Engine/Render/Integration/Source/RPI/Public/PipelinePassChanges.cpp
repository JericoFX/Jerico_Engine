/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/PipelinePassChanges.h>

namespace JE::Render::RPI
{
    bool PipelineNeedsRebuild(AZ::u32 flags)
    {
        return flags & (PipelinePassChanges::PassAssetHotReloaded |
                        PipelinePassChanges::MultisampleStateChanged |
                        PipelinePassChanges::PipelineChangedByFeatureProcessor);
    }

    bool PipelineViewsNeedRebuild(AZ::u32 flags)
    {
        return flags & (PipelinePassChanges::PipelineViewTagChanged |
                        PipelinePassChanges::PassesAdded |
                        PipelinePassChanges::PassesRemoved);
    }

    bool PipelineStateLookupNeedsRebuild(AZ::u32 flags)
    {
        return flags & (PipelinePassChanges::PipelineViewTagChanged |
                        PipelinePassChanges::MultisampleStateChanged |
                        PipelinePassChanges::PassesAdded |
                        PipelinePassChanges::PassesRemoved);
    }
}
