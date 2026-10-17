/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Render/RPI/Edit/Material/MaterialFunctorSourceDataRegistration.h>
#include <AzCore/Interface/Interface.h>

namespace JE::Render
{
    namespace RPI
    {
        MaterialFunctorSourceDataRegistration* MaterialFunctorSourceDataRegistration::Get()
        {
            return AZ::Interface<MaterialFunctorSourceDataRegistration>::Get();
        }

        void MaterialFunctorSourceDataRegistration::Init()
        {
            AZ::Interface<MaterialFunctorSourceDataRegistration>::Register(this);
        }

        void MaterialFunctorSourceDataRegistration::Shutdown()
        {
            m_materialfunctorMap.clear();
            m_inverseMap.clear();
            AZ::Interface<MaterialFunctorSourceDataRegistration>::Unregister(this);
        }

        void MaterialFunctorSourceDataRegistration::RegisterMaterialFunctor(const AZStd::string& functorName, const AZ::Uuid& typeId)
        {
            auto iter = m_materialfunctorMap.find(functorName);
            if (iter != m_materialfunctorMap.end())
            {
                if (iter->second != typeId)
                {
                    AZ_Error("Material Functor", false, "Material functor name \"%s\" was already registered with a different type Id!", functorName.c_str());
                }
                else
                {
                    AZ_Warning("Material Functor", false, "Material functor name \"%s\" was already registered!", functorName.c_str());
                }
                return;
            }

            m_materialfunctorMap[functorName] = typeId;
            m_inverseMap[typeId] = functorName;
        }

        AZ::Uuid MaterialFunctorSourceDataRegistration::FindMaterialFunctorTypeIdByName(const AZStd::string& functorName)
        {
            auto iter = m_materialfunctorMap.find(functorName);
            if (iter == m_materialfunctorMap.end())
            {
                AZ_Warning("Material Functor", false, "Material functor name \"%s\" was NOT registered!", functorName.c_str());
                return {};
            }
            return iter->second;
        }

        const AZStd::string MaterialFunctorSourceDataRegistration::FindMaterialFunctorNameByTypeId(const AZ::Uuid& typeId)
        {
            auto iter = m_inverseMap.find(typeId);
            if (iter == m_inverseMap.end())
            {
                AZ_Warning("Material Functor", false, "Material functor type \"%s\" was NOT registered!", typeId.ToString<AZStd::string>().c_str());
                return AZStd::string();
            }
            return iter->second;
        }
    }
}
