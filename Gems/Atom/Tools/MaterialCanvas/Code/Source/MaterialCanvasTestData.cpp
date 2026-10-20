/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Render/RPI/Edit/Common/AssetUtils.h>
#include <JE/Editor/Tools/Graph/DynamicNode/DynamicNodeConfig.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/algorithm.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace MaterialCanvas
{
    AZStd::string GetTestDataPathForNodeConfig(const JE::Editor::Tools::DynamicNodeConfig& nodeConfig)
    {
        AZStd::string baseName = JE::Render::RPI::AssetUtils::SanitizeFileName(nodeConfig.m_title);
        AZStd::to_lower(baseName.begin(), baseName.end());
        return AZStd::string::format(
            "@engroot@/Editor/Materials/Assets/MaterialCanvas/GraphData/Nodes/Functions/%s.materialgraphnode", baseName.c_str());
    }

    void CreateTestNodes()
    {
        const AZ::Vector4 defaultValue = AZ::Vector4::CreateZero();

        JE::Editor::Tools::DynamicNodeConfig nodeConfig;
        nodeConfig.m_category = "Math Operations";
        nodeConfig.m_title = "Combine";
        nodeConfig.m_inputSlots.emplace_back("inX", "X", "X", AZStd::any(defaultValue.GetX()), "color|float[1-4]?", JE::Editor::Tools::DynamicNodeSettingsMap());
        nodeConfig.m_inputSlots.emplace_back("inY", "Y", "Y", AZStd::any(defaultValue.GetY()), "color|float[1-4]?", JE::Editor::Tools::DynamicNodeSettingsMap());
        nodeConfig.m_inputSlots.emplace_back("inZ", "Z", "Z", AZStd::any(defaultValue.GetZ()), "color|float[1-4]?", JE::Editor::Tools::DynamicNodeSettingsMap());
        nodeConfig.m_inputSlots.emplace_back("inW", "W", "W", AZStd::any(defaultValue.GetW()), "color|float[1-4]?", JE::Editor::Tools::DynamicNodeSettingsMap());
        nodeConfig.m_outputSlots.emplace_back("outValue", "Value", "Value", AZStd::any(""), "float4", JE::Editor::Tools::DynamicNodeSettingsMap());
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));

        nodeConfig = {};
        nodeConfig.m_category = "Math Operations";
        nodeConfig.m_title = "Split";
        nodeConfig.m_outputSlots.emplace_back("outX", "X", "X", AZStd::any(defaultValue.GetX()), "color|float[1-4]?", JE::Editor::Tools::DynamicNodeSettingsMap());
        nodeConfig.m_outputSlots.emplace_back("outY", "Y", "Y", AZStd::any(defaultValue.GetY()), "color|float[1-4]?", JE::Editor::Tools::DynamicNodeSettingsMap());
        nodeConfig.m_outputSlots.emplace_back("outZ", "Z", "Z", AZStd::any(defaultValue.GetZ()), "color|float[1-4]?", JE::Editor::Tools::DynamicNodeSettingsMap());
        nodeConfig.m_outputSlots.emplace_back("outW", "W", "W", AZStd::any(defaultValue.GetW()), "color|float[1-4]?", JE::Editor::Tools::DynamicNodeSettingsMap());
        nodeConfig.m_inputSlots.emplace_back("inValue", "Value", "Value", AZStd::any(defaultValue), "color|float[1-4]?", JE::Editor::Tools::DynamicNodeSettingsMap());
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));

        nodeConfig = {};
        nodeConfig.m_category = "Math Operations";
        nodeConfig.m_title = "Add";
        nodeConfig.m_inputSlots.emplace_back("inValue1", "Value1", "Value1", AZStd::any(defaultValue), "color|float[1-4]?", JE::Editor::Tools::DynamicNodeSettingsMap());
        nodeConfig.m_inputSlots.emplace_back("inValue2", "Value2", "Value2", AZStd::any(defaultValue), "color|float[1-4]?", JE::Editor::Tools::DynamicNodeSettingsMap());
        nodeConfig.m_outputSlots.emplace_back("outValue", "Value", "Value", AZStd::any(defaultValue), "float4", JE::Editor::Tools::DynamicNodeSettingsMap());
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));

        nodeConfig.m_title = "Subtract";
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));

        nodeConfig.m_title = "Multiply";
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));

        nodeConfig.m_title = "Divide";
        nodeConfig.Save(GetTestDataPathForNodeConfig(nodeConfig));
    }
} // namespace MaterialCanvas
