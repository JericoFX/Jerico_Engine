/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <JE/Render/RPI/Public/FeatureProcessor.h>

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName};

    using ${SanitizedCppName}Handle = AZStd::shared_ptr<${SanitizedCppName}>;

    // ${Name}FeatureProcessorInterface provides an interface to the feature processor for code outside of Atom
    class ${SanitizedCppName}FeatureProcessorInterface
        : public JE::Render::RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(${SanitizedCppName}FeatureProcessorInterface, "{${Random_Uuid}}", JE::Render::RPI::FeatureProcessor);

    };
}
