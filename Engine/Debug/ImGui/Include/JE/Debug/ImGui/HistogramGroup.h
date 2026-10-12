/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <JE/Debug/ImGui/ImGuiExports.h>

#ifdef IMGUI_ENABLED

#include <AzCore/Math/Color.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>

#include <imgui/imgui.h>
#include <JE/Debug/ImGui/HistogramContainer.h>

namespace JE::Debug
{
    //! Helper for a group containing several histograms.
    //! The group is shown using collapsible header.
    class JE_DEBUG_IMGUI_API HistogramGroup
    {
    public:
        HistogramGroup() = default;
        HistogramGroup(const char* name, int histogramBinCount);

        void OnImGuiUpdate();
        void PushHistogramValue(const char* valueName, float value, const AZ::Color& color);

        const char* GetName() const { return m_name.c_str(); }
        const AZStd::string& GetNameString() const { return m_name; }
        void SetName(AZStd::string name) { m_name = name; }

        void SetHistogramBinCount(int count) { m_histogramBinCount = count; }

        JE::Debug::HistogramContainer* FindContainerByName(const char* name);

        //! Needs to be public for l-value access for ImGui::MenuItem()
        bool m_show = true;

    private:
        AZStd::string m_name; //< The name shown in the collapsible header.
        int m_histogramBinCount = 100; //< The number of bins in the histogram.

        using HistogramIndexByNames = AZStd::unordered_map<const char*, size_t>;
        HistogramIndexByNames m_histogramIndexByName; //< Look-up table for the histogram index by name.
        AZStd::vector<JE::Debug::HistogramContainer> m_histograms; //< Owns the histogram containers.

        static constexpr float s_histogramHeight = 85.0f;
    };
} // namespace JE::Debug

#endif // IMGUI_ENABLED
