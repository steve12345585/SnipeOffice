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

#include <o3tl/sorted_vector.hxx>

#include <xmloff/xmlictxt.hxx>
#include "formattributes.hxx"
#include <rtl/ref.hxx>
#include <com/sun/star/beans/PropertyValue.hpp>
#include "layerimport.hxx"

namespace com::sun::star::util {
    struct Time;
    struct Date;
}

namespace xmloff
{

    //= PropertyConversion
    class PropertyConversion
    {
    public:
        template<typename EnumT>
        static css::uno::Any convertString(
            const css::uno::Type& _rExpectedType,
            const OUString& _rReadCharacters,
            const SvXMLEnumMapEntry<EnumT>* _pEnumMap = nullptr
        )
        {
            return convertString(_rExpectedType, _rReadCharacters,
                    reinterpret_cast<const SvXMLEnumMapEntry<sal_uInt16>*>(_pEnumMap), /*_bInvertBoolean*/false);
        }
        static css::uno::Any convertString(
            const css::uno::Type& _rExpectedType,
            const OUString& _rReadCharacters,
            const SvXMLEnumMapEntry<sal_uInt16>* _pEnumMap = nullptr,
            const bool _bInvertBoolean = false
        );

        static css::uno::Type xmlTypeToUnoType( const OUString& _rType );
    };

    class OFormLayerXMLImport_Impl;
    //= OPropertyImport
    /** Helper class for importing property values

        <p>This class imports properties which are stored as attributes as well as properties which
        are stored in </em>&lt;form:properties&gt;</em> elements.</p>
    */
    class OPropertyImport : public SvXMLImportContext
    {
        friend class OSinglePropertyContext;
        friend class OListPropertyContext;

    protected:
        typedef ::std::vector< css::beans::PropertyValue > PropertyValueArray;
        PropertyValueArray          m_aValues;
        PropertyValueArray          m_aGenericValues;
            // the values which the instance collects between StartElement and EndElement

        o3tl::sorted_vector<sal_Int32>  m_aEncounteredAttributes;

        OFormLayerXMLImport_Impl&       m_rContext;

        bool                    m_bTrackAttributes;

        // TODO: think about the restriction that the class does not know anything about the object it is importing.
        // Perhaps this object should be known to the class, so setting the properties ('normal' ones as well as
        // style properties) can be done in our own EndElement instead of letting derived classes do this.

    public:
        OPropertyImport(OFormLayerXMLImport_Impl& _rImport);

        virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext(
            sal_Int32 nElement,
            const css::uno::Reference< css::xml::sax::XFastAttributeList >& AttrList ) override;

        virtual void SAL_CALL startFastElement(
            sal_Int32 nElement,
            const css::uno::Reference< css::xml::sax::XFastAttributeList >& _rxAttrList) override;
        virtual void SAL_CALL characters(const OUString& _rChars) override;

    protected:
        /** handle one single attribute.

            <p>This is called for every attribute of the element. This class' implementation checks if the attribute
            describes a property, if so, it is added to <member>m_aValues</member>.</p>

            <p>All non-property attributes should be handled in derived classes.</p>

            @param _nNamespaceKey
                key of the namespace used in the attribute
            @param _rLocalName
                local (relative to the namespace) attribute name
            @param _rValue
                attribute value
        */
        virtual bool handleAttribute(sal_Int32 nElement, const OUString& _rValue);

        /** determine if the element imported by the object had a given attribute.
            <p>Please be aware of the fact that the name given must be a local name, i.e. not contain a namespace.
            All form relevant attributes are in the same namespace, so this would be a redundant information.</p>
        */
        bool    encounteredAttribute(sal_Int32 nElement) const;

        /** enables the tracking of the encountered attributes
            <p>The tracking will raise the import costs a little bit, but it's cheaper than
            derived classes tracking this themself.</p>
        */
        void        enableTrackAttributes() { m_bTrackAttributes = true; }

        void implPushBackPropertyValue(const css::beans::PropertyValue& _rProp)
        {
            m_aValues.push_back(_rProp);
        }

        void implPushBackPropertyValue( const OUString& _rName, const css::uno::Any& _rValue )
        {
            m_aValues.push_back( css::beans::PropertyValue(
                _rName, -1, _rValue, css::beans::PropertyState_DIRECT_VALUE ) );
        }

        void implPushBackGenericPropertyValue(const css::beans::PropertyValue& _rProp)
        {
            m_aGenericValues.push_back(_rProp);
        }
    };
    typedef rtl::Reference<OPropertyImport> OPropertyImportRef;

    //= OPropertyElementsContext
    /** helper class for importing the &lt;form:properties&gt; element
    */
    class OPropertyElementsContext : public SvXMLImportContext
    {
        OPropertyImportRef  m_xPropertyImporter;    // to add the properties

    public:
        OPropertyElementsContext(SvXMLImport& _rImport,
                OPropertyImportRef _xPropertyImporter);

        virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext(
            sal_Int32 nElement, const css::uno::Reference< css::xml::sax::XFastAttributeList >& AttrList ) override;

#if OSL_DEBUG_LEVEL > 0
        virtual void SAL_CALL startFastElement(
            sal_Int32 nElement,
            const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList ) override;
        virtual void SAL_CALL characters(const OUString& _rChars) override;
#endif
    };

    //= OSinglePropertyContext
    /** helper class for importing a single &lt;form:property&gt; element
    */
    class OSinglePropertyContext : public SvXMLImportContext
    {
        OPropertyImportRef          m_xPropertyImporter;    // to add the properties

    public:
        OSinglePropertyContext(SvXMLImport& _rImport,
                OPropertyImportRef _xPropertyImporter);

        virtual void SAL_CALL startFastElement(
            sal_Int32 nElement,
            const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList ) override;
    };

    //= OListPropertyContext
    class OListPropertyContext : public SvXMLImportContext
    {
        OPropertyImportRef                  m_xPropertyImporter;
        OUString                     m_sPropertyName;
        OUString                     m_sPropertyType;
        ::std::vector< OUString >    m_aListValues;

    public:
        OListPropertyContext( SvXMLImport& _rImport,
                OPropertyImportRef _xPropertyImporter );

        virtual void SAL_CALL startFastElement(
            sal_Int32 nElement,
            const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList ) override;

        virtual void SAL_CALL endFastElement(sal_Int32 nElement) override;

        virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext(
            sal_Int32 nElement, const css::uno::Reference< css::xml::sax::XFastAttributeList >& AttrList ) override;
    };

    //= OListValueContext
    class OListValueContext : public SvXMLImportContext
    {
        OUString& m_rListValueHolder;

    public:
        OListValueContext( SvXMLImport& _rImport, OUString& _rListValueHolder );

        virtual void SAL_CALL startFastElement(
            sal_Int32 nElement,
            const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList ) override;
    };

}   // namespace xmloff

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
