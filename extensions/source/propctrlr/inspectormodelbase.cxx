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

#include <memory>
#include "inspectormodelbase.hxx"

#include <com/sun/star/beans/PropertyAttribute.hpp>

#include <comphelper/propertycontainerhelper.hxx>
#include <cppuhelper/supportsservice.hxx>


namespace pcr
{
   namespace
   {
        enum class ModelPropertyId
        {
            HAS_HELP_SECTION    = 2000,
            MIN_HELP_TEXT_LINES = 2001,
            MAX_HELP_TEXT_LINES = 2002,
            IS_READ_ONLY        = 2003
        };
   };
    using ::com::sun::star::uno::Reference;
    using ::com::sun::star::beans::XPropertySetInfo;
    using ::com::sun::star::uno::Any;
    using ::com::sun::star::uno::Sequence;
    using ::com::sun::star::beans::Property;

    namespace PropertyAttribute = ::com::sun::star::beans::PropertyAttribute;


    //= InspectorModelProperties

    /** helper class for implementing the property set related functionality
        of an ImplInspectorModel
    */
    class InspectorModelProperties : public ::comphelper::OPropertyContainerHelper
    {
    private:
        ::osl::Mutex&           m_rMutex;
        bool                m_bHasHelpSection;
        sal_Int32               m_nMinHelpTextLines;
        sal_Int32               m_nMaxHelpTextLines;
        bool                m_bIsReadOnly;
        std::unique_ptr< ::cppu::IPropertyArrayHelper >
                                m_pPropertyInfo;

    public:
        explicit InspectorModelProperties( ::osl::Mutex& _rMutex );

        using ::comphelper::OPropertyContainerHelper::convertFastPropertyValue;
        using ::comphelper::OPropertyContainerHelper::setFastPropertyValue;
        using ::comphelper::OPropertyContainerHelper::getFastPropertyValue;

    public:
        bool    hasHelpSection() const { return m_bHasHelpSection; }
        bool    isReadOnly() const { return m_bIsReadOnly; }
        sal_Int32   getMinHelpTextLines() const { return m_nMinHelpTextLines; }
        sal_Int32   getMaxHelpTextLines() const { return m_nMaxHelpTextLines; }

        css::uno::Reference< css::beans::XPropertySetInfo >
                            getPropertySetInfo();
        ::cppu::IPropertyArrayHelper&
                            getInfoHelper();

        void    constructWithHelpSection( sal_Int32 _nMinHelpTextLines, sal_Int32 _nMaxHelpTextLines );
    };


    //= InspectorModelProperties


    InspectorModelProperties::InspectorModelProperties( ::osl::Mutex& _rMutex )
        :m_rMutex( _rMutex )
        ,m_bHasHelpSection( false )
        ,m_nMinHelpTextLines( 3 )
        ,m_nMaxHelpTextLines( 8 )
        ,m_bIsReadOnly( false )
    {
        registerProperty(
            u"HasHelpSection"_ustr,
            static_cast<sal_Int32>(ModelPropertyId::HAS_HELP_SECTION),
            PropertyAttribute::READONLY,
            &m_bHasHelpSection, cppu::UnoType<decltype(m_bHasHelpSection)>::get()
        );
        registerProperty(
            u"MinHelpTextLines"_ustr,
            static_cast<sal_Int32>(ModelPropertyId::MIN_HELP_TEXT_LINES),
            PropertyAttribute::READONLY,
            &m_nMinHelpTextLines, cppu::UnoType<decltype(m_nMinHelpTextLines)>::get()
        );
        registerProperty(
            u"MaxHelpTextLines"_ustr,
            static_cast<sal_Int32>(ModelPropertyId::MAX_HELP_TEXT_LINES),
            PropertyAttribute::READONLY,
            &m_nMaxHelpTextLines, cppu::UnoType<decltype(m_nMaxHelpTextLines)>::get()
        );
        registerProperty(
            u"IsReadOnly"_ustr,
            static_cast<sal_Int32>(ModelPropertyId::IS_READ_ONLY),
            PropertyAttribute::BOUND,
            &m_bIsReadOnly, cppu::UnoType<decltype(m_bIsReadOnly)>::get()
        );
    }


    void InspectorModelProperties::constructWithHelpSection( sal_Int32 _nMinHelpTextLines, sal_Int32 _nMaxHelpTextLines )
    {
        m_bHasHelpSection = true;
        m_nMinHelpTextLines = _nMinHelpTextLines;
        m_nMaxHelpTextLines = _nMaxHelpTextLines;
        // no need to notify this, those properties are not bound. Also, the method should
        // only be used during construction phase, where we don't expect to have any listeners.
    }


    ::cppu::IPropertyArrayHelper& InspectorModelProperties::getInfoHelper()
    {
        ::osl::MutexGuard aGuard( m_rMutex );
        if (m_pPropertyInfo == nullptr)
        {
            Sequence< Property > aProperties;
            describeProperties( aProperties );

            m_pPropertyInfo.reset( new ::cppu::OPropertyArrayHelper( aProperties ) );
        }
        return *m_pPropertyInfo;
    }


    Reference< XPropertySetInfo > InspectorModelProperties::getPropertySetInfo()
    {
        return ::cppu::OPropertySetHelper::createPropertySetInfo( getInfoHelper() );
    }


    //= ImplInspectorModel

    ImplInspectorModel::ImplInspectorModel()
        :ImplInspectorModel_PBase( GetBroadcastHelper() )
        ,m_pProperties( new InspectorModelProperties( m_aMutex ) )
    {
    }


    ImplInspectorModel::~ImplInspectorModel()
    {
    }


    IMPLEMENT_FORWARD_XINTERFACE2( ImplInspectorModel, ImplInspectorModel_Base, ImplInspectorModel_PBase )


    IMPLEMENT_FORWARD_XTYPEPROVIDER2( ImplInspectorModel, ImplInspectorModel_Base, ImplInspectorModel_PBase )


    Reference< XPropertySetInfo > SAL_CALL ImplInspectorModel::getPropertySetInfo(  )
    {
        return m_pProperties->getPropertySetInfo();
    }


    ::cppu::IPropertyArrayHelper& SAL_CALL ImplInspectorModel::getInfoHelper()
    {
        return m_pProperties->getInfoHelper();
    }


    sal_Bool SAL_CALL ImplInspectorModel::convertFastPropertyValue( Any & rConvertedValue, Any & rOldValue, sal_Int32 nHandle, const Any& rValue )
    {
        return m_pProperties->convertFastPropertyValue( rConvertedValue, rOldValue, nHandle, rValue );
    }


    void SAL_CALL ImplInspectorModel::setFastPropertyValue_NoBroadcast( sal_Int32 nHandle, const Any& rValue )
    {
        m_pProperties->setFastPropertyValue( nHandle, rValue );
    }


    void SAL_CALL ImplInspectorModel::getFastPropertyValue( Any& rValue, sal_Int32 nHandle ) const
    {
        m_pProperties->getFastPropertyValue( rValue, nHandle );
    }


    sal_Bool SAL_CALL ImplInspectorModel::getHasHelpSection()
    {
        return m_pProperties->hasHelpSection();
    }


    ::sal_Int32 SAL_CALL ImplInspectorModel::getMinHelpTextLines()
    {
        return m_pProperties->getMinHelpTextLines();
    }


    ::sal_Int32 SAL_CALL ImplInspectorModel::getMaxHelpTextLines()
    {
        return m_pProperties->getMaxHelpTextLines();
    }


    sal_Bool SAL_CALL ImplInspectorModel::getIsReadOnly()
    {
        return m_pProperties->isReadOnly();
    }


    void SAL_CALL ImplInspectorModel::setIsReadOnly( sal_Bool IsReadOnly )
    {
        setFastPropertyValue( static_cast<sal_Int32>(ModelPropertyId::IS_READ_ONLY), Any( IsReadOnly ) );
    }

    sal_Bool SAL_CALL ImplInspectorModel::supportsService( const OUString& ServiceName )
    {
        return cppu::supportsService(this, ServiceName);
    }


    void ImplInspectorModel::enableHelpSectionProperties( sal_Int32 _nMinHelpTextLines, sal_Int32 _nMaxHelpTextLines )
    {
        m_pProperties->constructWithHelpSection( _nMinHelpTextLines, _nMaxHelpTextLines );
    }


} // namespace pcr


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
