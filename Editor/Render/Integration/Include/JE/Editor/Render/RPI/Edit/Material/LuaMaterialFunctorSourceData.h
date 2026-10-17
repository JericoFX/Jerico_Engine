/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Render/RPI/Edit/Configuration.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialFunctorSourceData.h>

namespace UnitTest
{
    class LuaMaterialFunctorTests;
}

namespace JE::Render
{
    namespace RPI
    {
        //! Builds a LuaMaterialFunctor.
        //! Materials can use this functor to create custom scripted operations.
        class JE_EDITOR_RENDER_INTEGRATION_API LuaMaterialFunctorSourceData final
            : public JE::Render::RPI::MaterialFunctorSourceData
        {
            friend class UnitTest::LuaMaterialFunctorTests;
        public:
            AZ_CLASS_ALLOCATOR(LuaMaterialFunctorSourceData, AZ::SystemAllocator)
            AZ_RTTI_WITH_NAME(JE::Render::RPI::LuaMaterialFunctorSourceData, "AZ::RPI::LuaMaterialFunctorSourceData", "{E6F6D022-340C-47E3-A0BA-4EFE79C0CD1A}", JE::Render::RPI::MaterialFunctorSourceData);

            static void Reflect(AZ::ReflectContext* context);

            FunctorResult CreateFunctor(const RuntimeContext& context) const override;
            FunctorResult CreateFunctor(const EditorContext& context) const override;

            AZStd::vector<AssetDependency> GetAssetDependencies() const override;

            virtual AZStd::vector<AZ::Name> GetShaderOptionDependencies() const override { return m_shaderOptionDependencies; }

        private:
            // Calls a lua function that returns a list of strings.
            AZ::Outcome<AZStd::vector<AZ::Name>, void> GetNameListFromLuaScript(AZ::ScriptContext& scriptContext, const char* luaFunctionName) const;

            FunctorResult CreateFunctor(
                const AZStd::string& materialTypeSourceFilePath,
                const MaterialPropertiesLayout* propertiesLayout,
                const MaterialNameContext* materialNameContext) const;

            // Only one of these should have data
            AZStd::string m_luaSourceFile;
            AZStd::string m_luaScript;

            // These are prefix strings that will be applied to every name lookup in the lua functor.
            // This allows the lua script to be reused in different contexts.
            AZStd::string m_propertyNamePrefix;
            AZStd::string m_srgNamePrefix;
            AZStd::string m_optionsNamePrefix;

            // This is mutable because it gets initialized in CreateFunctor which is const. This is okay because CreateFunctor() is called before GetShaderOptionDependencies()
            mutable AZStd::vector<AZ::Name> m_shaderOptionDependencies;
        };

    } // namespace Render
} // namespace AZ
