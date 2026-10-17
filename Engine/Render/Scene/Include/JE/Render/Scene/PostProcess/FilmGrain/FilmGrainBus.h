/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/FilmGrain/FilmGrainConstants.h>
#include <AzCore/Component/Component.h>

namespace JE::Render
{
    namespace Scene
    {
        class FilmGrainRequests : public AZ::ComponentBus
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Scene::FilmGrainRequests, "AZ::Render::FilmGrainRequests", "{8072BEF6-22A2-45F0-A887-0CB840B6EE45}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            virtual ~FilmGrainRequests()
            {
            }

            // Auto-gen virtual getters/setters...
#include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
#include <Atom/Feature/PostProcess/FilmGrain/FilmGrainParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
        };

        typedef AZ::EBus<FilmGrainRequests> FilmGrainRequestBus;
    } // namespace Render
} // namespace AZ
