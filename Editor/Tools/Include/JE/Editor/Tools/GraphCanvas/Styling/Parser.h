/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>

#include <JE/Editor/Tools/GraphCanvas/Styling/Style.h>

namespace JE::Editor::Tools::GraphCanvas
{
    class StyleManager;

    namespace Styling
    {
        class Parser
        {
        public:
            static void Parse(StyleManager& styleManager, const AZStd::string& json);
            static void Parse(StyleManager& styleManager, const rapidjson::Document& json);

        private:

            static void ParseStyle(StyleManager& styleManager, const rapidjson::Value& value);

            static SelectorVector ParseSelectors(const rapidjson::Value& value);
        };

    } // namespace Styling
} // namespace GraphCanvas
