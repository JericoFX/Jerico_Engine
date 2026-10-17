/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/Model/ModelLod.h>

#include <JE/Render/RPI/Reflect/Model/ModelLodIndex.h>

namespace AZ
{
    class Transform;

    }

namespace JE::Render::RPI
{
        class View;
        class Model;

        namespace ModelLodUtils
        {
            JE_RENDER_INTEGRATION_API ModelLodIndex SelectLod(const View* view, const AZ::Transform& entityTransform, const Model& model, ModelLodIndex lodOverride = {});
            JE_RENDER_INTEGRATION_API ModelLodIndex SelectLod(const View* view, const AZ::Vector3& position, const Model& model, ModelLodIndex lodOverride = {});
            JE_RENDER_INTEGRATION_API uint8_t SelectLodFromBoundingSphere(const AZ::Vector3 center, const float radius, uint8_t numLods, const AZ::Matrix4x4& worldtoView, const AZ::Matrix4x4& viewToClip);

            //! Gets the approximate screen percentage coverage from a bounding sphere (used in LOD calculations).
            //! cameraPosition is typically viewToWorld.GetTranslation() or worldToView.GetInverse().GetTranslation()
            //! yScale is the [1][1] element from a projection matrix, i.e.: viewToClip.GetRow(1).GetY(),
            //!   which is generally cot(FovY/2) or 1.0/tan(FovY/2) or 2*nearPlaneDistance/nearPlaneHeight for perspective frustum and
            //!   2/(top-bottom) for orthogonal frustum.
            //!   We only use the vertical scale for two reasons: speed and for more consistent behavior with ultra-widescreen views
            JE_RENDER_INTEGRATION_API float ApproxScreenPercentage(const AZ::Vector3& center, float radius, const AZ::Vector3& cameraPosition, float yScale, bool isPerspective);
        } // namespace ModelLodUtils
    }

namespace AZ
{ // namespace RPI
} // namespace AZ
