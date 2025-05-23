/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#pragma once

#include <sal/config.h>

#include <map>

#include <com/sun/star/uno/Type.hxx>
#include <rtl/ustring.hxx>
#include <sal/types.h>
#include <xmloff/xmlnamespace.hxx>
#include <xmloff/xmltoken.hxx>
#include <o3tl/typed_flags_set.hxx>

template<typename EnumT>
struct SvXMLEnumMapEntry;

    // flags for common control attributes
enum class CCAFlags {
    NONE                  = 0x00000000,
    Name                  = 0x00000001,
    ServiceName           = 0x00000002,
    ButtonType            = 0x00000004,
    ControlId             = 0x00000008,
    CurrentSelected       = 0x00000010,
    CurrentValue          = 0x00000020,
    Disabled              = 0x00000040,
    Dropdown              = 0x00000080,
    For                   = 0x00000100,
    ImageData             = 0x00000200,
    Label                 = 0x00000400,
    MaxLength             = 0x00000800,
    Printable             = 0x00001000,
    ReadOnly              = 0x00002000,
    Selected              = 0x00004000,
    Size                  = 0x00008000,
    TabIndex              = 0x00010000,
    TargetFrame           = 0x00020000,
    TargetLocation        = 0x00040000,
    TabStop               = 0x00080000,
    Title                 = 0x00100000,
    Value                 = 0x00200000,
    Orientation           = 0x00400000,
    VisualEffect          = 0x00800000,
    EnableVisible         = 0x01000000,
};
namespace o3tl {
    template<> struct typed_flags<CCAFlags> : is_typed_flags<CCAFlags, 0x01ffffff> {};
}

    // flags for database control attributes
enum class DAFlags {
    NONE                  = 0x0000,
    BoundColumn           = 0x0001,
    ConvertEmpty          = 0x0002,
    DataField             = 0x0004,
    ListSource            = 0x0008,
    ListSource_TYPE       = 0x0010,
    InputRequired         = 0x0020,
};
namespace o3tl {
    template<> struct typed_flags<DAFlags> : is_typed_flags<DAFlags, 0x003f> {};
}

    // flags for binding related control attributes
enum class BAFlags {
    NONE                  = 0x0000,
    LinkedCell            = 0x0001,
    ListLinkingType       = 0x0002,
    ListCellRange         = 0x0004,
    XFormsBind            = 0x0008,
    XFormsListBind        = 0x0010,
    XFormsSubmission      = 0x0020
};
namespace o3tl {
    template<> struct typed_flags<BAFlags> : is_typed_flags<BAFlags, 0x003f> {};
}

    // flags for event attributes
enum class EAFlags {
    NONE                  = 0x0000,
    ControlEvents         = 0x0001,
    OnChange              = 0x0002,
    OnClick               = 0x0004,
    OnDoubleClick         = 0x0008,
    OnSelect              = 0x0010
};
namespace o3tl {
    template<> struct typed_flags<EAFlags> : is_typed_flags<EAFlags, 0x001f> {};
}

    // any other attributes, which are special to some control types
enum class SCAFlags {
    NONE                  = 0x000000,
    EchoChar              = 0x000001,
    MaxValue              = 0x000002,
    MinValue              = 0x000004,
    Validation            = 0x000008,
    GroupName             = 0x000010,
    MultiLine             = 0x000020,
    AutoCompletion        = 0x000080,
    Multiple              = 0x000100,
    DefaultButton         = 0x000200,
    CurrentState          = 0x000400,
    IsTristate            = 0x000800,
    State                 = 0x001000,
    ColumnStyleName       = 0x002000,
    StepSize              = 0x004000,
    PageStepSize          = 0x008000,
    RepeatDelay           = 0x010000,
    Toggle                = 0x020000,
    FocusOnClick          = 0x040000,
    ImagePosition         = 0x080000
};
namespace o3tl {
    template<> struct typed_flags<SCAFlags> : is_typed_flags<SCAFlags, 0x0fffbf> {};
}


namespace xmloff
{

    /// attributes in the xml tag representing a form
    enum FormAttributes
    {
        faName,
        faAction,
        faEnctype,
        faMethod,
        faAllowDeletes,
        faAllowInserts,
        faAllowUpdates,
        faApplyFilter,
        faCommand,
        faCommandType,
        faEscapeProcessing,
        faDatasource,
        faDetailFields,
        faFilter,
        faIgnoreResult,
        faMasterFields,
        faNavigationMode,
        faOrder,
        faTabbingCycle
    };

    // attributes of the office:forms element
    enum OfficeFormsAttributes
    {
        ofaAutomaticFocus,
        ofaApplyDesignMode
    };

    //= OAttributeMetaData
    /** allows the translation of attribute ids into strings.

        <p>This class does not allow to connect xml attributes to property names or
        something like that, it only deals with the xml side</p>
    */
    class OAttributeMetaData
    {
    public:
        /** calculates the xml attribute representation of a common control attribute.
            @param _nId
                the id of the attribute. Has to be one of the CCA_* constants.
        */
        static OUString getCommonControlAttributeName(CCAFlags _nId);

        /** calculates the xml attribute representation of a common control attribute.
            @param _nId
                the id of the attribute. Has to be one of the CCA_* constants.
        */
        static sal_Int32 getCommonControlAttributeToken(CCAFlags _nId);

        /** calculates the xml namespace key to use for a common control attribute
            @param _nId
                the id of the attribute. Has to be one of the CCA_* constants.
        */
        static sal_uInt16 getCommonControlAttributeNamespace(CCAFlags _nId);

        /** retrieves the name of an attribute of a form xml representation
            @param  _eAttrib
                enum value specifying the attribute
        */
        static OUString getFormAttributeName(FormAttributes _eAttrib);

        /** retrieves the name of an attribute of a form xml representation
            @param  _eAttrib
                enum value specifying the attribute
        */
        static sal_Int32 getFormAttributeToken(FormAttributes _eAttrib);

        /** calculates the xml namespace key to use for an attribute of a form xml representation
            @param  _eAttrib
                enum value specifying the attribute
        */
        static sal_uInt16 getFormAttributeNamespace(FormAttributes _eAttrib);

        /** calculates the xml attribute representation of a database attribute.
            @param _nId
                the id of the attribute. Has to be one of the DA_* constants.
        */
        static OUString getDatabaseAttributeName(DAFlags _nId);

        /** calculates the xml attribute representation of a database attribute.
            @param _nId
                the id of the attribute. Has to be one of the DA_* constants.
        */
        static sal_Int32 getDatabaseAttributeToken(DAFlags _nId);

        /** calculates the xml namespace key to use for a database attribute.
            @param _nId
                the id of the attribute. Has to be one of the DA_* constants.
        */
        static sal_uInt16 getDatabaseAttributeNamespace()
        {
            // nothing special here
            return XML_NAMESPACE_FORM;
        }

        /** calculates the xml attribute representation of a special attribute.
            @param _nId
                the id of the attribute. Has to be one of the SCA_* constants.
        */
        static OUString getSpecialAttributeName(SCAFlags _nId);

        /** calculates the xml attribute representation of a special attribute.
            @param _nId
                the id of the attribute. Has to be one of the SCA_* constants.
        */
        static sal_Int32 getSpecialAttributeToken(SCAFlags _nId);

        /** calculates the xml attribute representation of a binding attribute.
            @param _nId
                the id of the attribute. Has to be one of the BA_* constants.
        */
        static OUString getBindingAttributeName(BAFlags _nId);

        /** calculates the xml attribute representation of a binding attribute.
            @param _nId
                the id of the attribute. Has to be one of the BA_* constants.
        */
        static sal_Int32 getBindingAttributeToken(BAFlags _nId);

        /** calculates the xml namespace key to use for a binding attribute.
            @param _nId
                the id of the attribute. Has to be one of the BA_* constants.
        */
        static sal_uInt16 getBindingAttributeNamespace()
        {
            // nothing special here
            return XML_NAMESPACE_FORM;
        }

        /** calculates the xml namespace key to use for a special attribute.
            @param _nId
                the id of the attribute. Has to be one of the SCA_* constants.
        */
        static sal_uInt16 getSpecialAttributeNamespace(SCAFlags _nId);

        /** calculates the xml attribute representation of an attribute of the office:forms element
            @param _nId
                the id of the attribute
        */
        static OUString getOfficeFormsAttributeName(OfficeFormsAttributes _eAttrib);
        static xmloff::token::XMLTokenEnum getOfficeFormsAttributeToken(OfficeFormsAttributes _eAttrib);

        /** calculates the xml namedspace key of an attribute of the office:forms element
            @param _nId
                the id of the attribute
        */
        static sal_uInt16 getOfficeFormsAttributeNamespace()
        { // nothing special here
          return XML_NAMESPACE_FORM;
        }
    };

    //= OAttribute2Property
    /** some kind of opposite to the OAttributeMetaData class. Able to translate
        attributes into property names/types

        <p>The construction of this class is rather expensive (or at least it's initialization from outside),
        so it should be shared</p>
    */
    class OAttribute2Property final
    {
    public:
        // TODO: maybe the following struct should be used for exports, too. In this case we would not need to
        // store it's instances in a map, but in a vector for faster access.
        struct AttributeAssignment
        {
            OUString                 sPropertyName;          // the property name
            css::uno::Type           aPropertyType;          // the property type

            // entries which are special to some value types
            const SvXMLEnumMapEntry<sal_uInt16>*
                                     pEnumMap;               // the enum map, if applicable
            bool                     bInverseSemantics;      // for booleans: attribute and property value have the same or an inverse semantics?

            AttributeAssignment() : pEnumMap(nullptr), bInverseSemantics(false) { }
        };

    private:
        std::map<sal_Int32, AttributeAssignment> m_aKnownProperties;

    public:
        OAttribute2Property();
        ~OAttribute2Property();

        /** return the AttributeAssignment which corresponds to the given attribute

            @return
                a pointer to the <type>AttributeAssignment</type> structure as requested, NULL if the attribute
                does not represent a property.
        */
        const AttributeAssignment* getAttributeTranslation(sal_Int32 nAttributeToken);

        /** add an attribute assignment referring to a string property to the map
            @param _pAttributeName
                the name of the attribute
            @param _rPropertyName
                the name of the property assigned to the attribute
        */
        void    addStringProperty(
            sal_Int32 nAttributeToken, const OUString& _rPropertyName);

        /** add an attribute assignment referring to a boolean property to the map

            @param _pAttributeName
                the name of the attribute
            @param _rPropertyName
                the name of the property assigned to the attribute
            @param _bAttributeDefault
                the default value for the attribute.
            @param _bInverseSemantics
                if <TRUE/>, an attribute value of <TRUE/> means a property value of <FALSE/> and vice verse.<br/>
                if <FALSE/>, the attribute value is used as property value directly
        */
        void    addBooleanProperty(
            sal_Int32 nAttributeToken, const OUString& _rPropertyName,
            const bool _bAttributeDefault, const bool _bInverseSemantics = false);

        /** add an attribute assignment referring to an int16 property to the map

            @param _pAttributeName
                the name of the attribute
            @param _rPropertyName
                the name of the property assigned to the attribute
        */
        void    addInt16Property(
            sal_Int32 nAttributeToken, const OUString& _rPropertyName);

        /** add an attribute assignment referring to an int32 property to the map

            @param _pAttributeName
                the name of the attribute
            @param _rPropertyName
                the name of the property assigned to the attribute
        */
        void    addInt32Property(
            sal_Int32 nAttributeToken, const OUString& _rPropertyName );

        /** add an attribute assignment referring to an enum property to the map

            @param _pAttributeName
                the name of the attribute
            @param _rPropertyName
                the name of the property assigned to the attribute
            @param _pValueMap
                the map to translate strings into enum values
            @param _pType
                the type of the property. May be NULL, in this case 32bit integer is assumed.
        */
        template<typename EnumT>
        void    addEnumProperty(
            sal_Int32 nAttributeToken, const OUString& _rPropertyName,
            const SvXMLEnumMapEntry<EnumT>* _pValueMap,
            const css::uno::Type* _pType = nullptr)
        {
            addEnumPropertyImpl(nAttributeToken, _rPropertyName,
                                reinterpret_cast<const SvXMLEnumMapEntry<sal_uInt16>*>(_pValueMap), _pType);
        }

    private:
        void addEnumPropertyImpl(
            sal_Int32 nAttributeToken, const OUString& _rPropertyName,
            const SvXMLEnumMapEntry<sal_uInt16>* _pValueMap,
            const css::uno::Type* _pType);
        /// some common code for the various add*Property methods
        AttributeAssignment& implAdd(
            sal_Int32 nAttributeToken, const OUString& _rPropertyName,
            const css::uno::Type& _rType);
    };
}   // namespace xmloff

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
