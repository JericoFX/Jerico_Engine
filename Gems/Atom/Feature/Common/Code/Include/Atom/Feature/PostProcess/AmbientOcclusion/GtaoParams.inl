/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Macros below are of the form:
// PARAM(NAME, MEMBER_NAME, DEFAULT_VALUE, ...)

// --- GTAO COMPUTE ---

// The strength multiplier for the GTAO effect
AZ_GFX_FLOAT_PARAM(GtaoStrength, m_gtaoStrength, AZ::Render::Ao::DefaultGtaoStrength)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, GtaoStrength, m_gtaoStrength)

// The power for the GTAO effect
AZ_GFX_FLOAT_PARAM(GtaoPower, m_gtaoPower, AZ::Render::Ao::DefaultGtaoPower)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, GtaoPower, m_gtaoPower)

// The world radius for the GTAO effect
AZ_GFX_FLOAT_PARAM(GtaoWorldRadius, m_gtaoWorldRadius, AZ::Render::Ao::DefaultGtaoWorldRadius)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, GtaoWorldRadius, m_gtaoWorldRadius)

// The max depth for the GTAO effect
AZ_GFX_FLOAT_PARAM(GtaoMaxDepth, m_gtaoMaxDepth, AZ::Render::Ao::DefaultGtaoMaxDepth)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, GtaoMaxDepth, m_gtaoMaxDepth)

// The thickness blend factor for the GTAO effect
AZ_GFX_FLOAT_PARAM(GtaoThicknessBlend, m_gtaoThicknessBlend, AZ::Render::Ao::DefaultGtaoThicknessBlend)
AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(float, GtaoThicknessBlend, m_gtaoThicknessBlend)

// Quality level of the effect
AZ_GFX_COMMON_PARAM(AZ::Render::Ao::GtaoQualityLevel, GtaoQuality, m_gtaoQuality, AZ::Render::Ao::GtaoQualityLevel::Medium)
AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(AZ::Render::Ao::GtaoQualityLevel, GtaoQuality, m_gtaoQuality)
