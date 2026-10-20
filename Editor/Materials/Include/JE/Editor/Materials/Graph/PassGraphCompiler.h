/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Materials/Base.h>
#include <JE/Editor/Tools/Graph/GraphCompiler.h>

namespace JE::Editor::Materials
{
    //! Validates pass graph compilation inputs. Source generation is not implemented, so valid graphs fail
    //! with a diagnostic and produce no pass or shader files.
    class JE_EDITOR_MATERIALS_API PassGraphCompiler : public JE::Editor::Tools::GraphCompiler
    {
    public:
        AZ_RTTI(PassGraphCompiler, "{4D9407B1-195A-404A-B97A-E2BA22207C87}", JE::Editor::Tools::GraphCompiler);
        AZ_CLASS_ALLOCATOR(PassGraphCompiler, AZ::SystemAllocator);
        AZ_DISABLE_COPY_MOVE(PassGraphCompiler);

        static void Reflect(AZ::ReflectContext* context);

        PassGraphCompiler() = default;
        PassGraphCompiler(const AZ::Crc32& toolId);
        virtual ~PassGraphCompiler();

        // AtomToolsFramework::GraphCompiler overrides...
        AZStd::string GetGraphPath() const;
        bool CompileGraph(GraphModel::GraphPtr graph, const AZStd::string& graphName, const AZStd::string& graphPath) override;
    };
} // namespace JE::Editor::Materials
