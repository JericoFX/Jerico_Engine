/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Materials/Base.h>
#include <AzCore/EBus/EBus.h>
#include <JE/Render/RPI/Public/Material/Material.h>
#include <JE/Render/RPI/Reflect/Material/MaterialAsset.h>

namespace JE::Render
{
    namespace RPI
    {
        class MaterialSourceData;
        class MaterialTypeSourceData;
    } // namespace RPI
} // namespace AZ

namespace JE::Editor
{
    //! UVs are processed in a property group but will be handled differently.
    static constexpr const char UvGroupName[] = "uvSets";

    class JE_EDITOR_MATERIALS_API MaterialDocumentRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        //! Get material asset created by MaterialDocument
        virtual AZ::Data::Asset<JE::Render::RPI::MaterialAsset> GetAsset() const = 0;

        //! Get material instance created from asset loaded by MaterialDocument
        virtual AZ::Data::Instance<JE::Render::RPI::Material> GetInstance() const = 0;

        //! Get the internal material source data
        virtual const JE::Render::RPI::MaterialSourceData* GetMaterialSourceData() const = 0;

        //! Get the internal material type source data
        virtual const JE::Render::RPI::MaterialTypeSourceData* GetMaterialTypeSourceData() const = 0;

        //! Modify property value
        virtual void SetPropertyValue(const AZStd::string& propertyFullName, const AZStd::any& value) = 0;

        //! Return property value
        //! If the document is not open or the id can't be found, an invalid value is returned instead.
        virtual const AZStd::any& GetPropertyValue(const AZStd::string& propertyFullName) const = 0;
    };

    using MaterialDocumentRequestBus = AZ::EBus<MaterialDocumentRequests>;
} // namespace JE::Editor

AZ_DECLARE_EBUS_MULTI_ADDRESS(JE_EDITOR_MATERIALS_API, JE::Editor::MaterialDocumentRequests);
