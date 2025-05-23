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

#include <TableDeco.hxx>
#include <definitioncolumn.hxx>
#include <stringconstants.hxx>
#include <strings.hxx>
#include <core_resource.hxx>
#include <strings.hrc>
#include <osl/diagnose.h>
#include <sal/log.hxx>

#include <comphelper/property.hxx>
#include <comphelper/servicehelper.hxx>
#include <com/sun/star/sdbc/SQLException.hpp>
#include <com/sun/star/sdbc/XConnection.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <connectivity/dbtools.hxx>
#include <connectivity/dbexception.hxx>
#include <ContainerMediator.hxx>

using namespace dbaccess;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::sdbcx;
using namespace ::com::sun::star::container;
using namespace ::osl;
using namespace ::comphelper;
using namespace ::dbtools;
using namespace ::cppu;

// ODBTableDecorator

ODBTableDecorator::ODBTableDecorator( const Reference< XConnection >& _rxConnection, const Reference< XColumnsSupplier >& _rxNewTable,
        const Reference< XNumberFormatsSupplier >& _rxNumberFormats, const Reference< XNameAccess >& _xColumnDefinitions )
    :OTableDescriptor_BASE(m_aMutex)
    ,ODataSettings(OTableDescriptor_BASE::rBHelper)
    ,m_xTable(_rxNewTable)
    ,m_xColumnDefinitions(_xColumnDefinitions)
    ,m_xConnection( _rxConnection )
    ,m_xMetaData( _rxConnection.is() ? _rxConnection->getMetaData() : Reference< XDatabaseMetaData >() )
    ,m_xNumberFormats( _rxNumberFormats )
    ,m_nPrivileges(-1)
{
    ODataSettings::registerPropertiesFor(this);
}

ODBTableDecorator::~ODBTableDecorator()
{
}

Sequence< sal_Int8 > ODBTableDecorator::getImplementationId()
{
    return css::uno::Sequence<sal_Int8>();
}

// OComponentHelper
void SAL_CALL ODBTableDecorator::disposing()
{
    OPropertySetHelper::disposing();
    OTableDescriptor_BASE::disposing();

    MutexGuard aGuard(m_aMutex);
    m_xTable        = nullptr;
    m_xMetaData     = nullptr;
    m_xColumnDefinitions = nullptr;
    m_xNumberFormats = nullptr;
    if ( m_pColumns )
        m_pColumns->disposing();
    m_xColumnMediator = nullptr;
}

sal_Bool SAL_CALL ODBTableDecorator::convertFastPropertyValue(
                            Any & rConvertedValue,
                            Any & rOldValue,
                            sal_Int32 nHandle,
                            const Any& rValue )
{
    bool bRet = true;
    switch(nHandle)
    {
        case PROPERTY_ID_PRIVILEGES:
        case PROPERTY_ID_FILTER:
        case PROPERTY_ID_ORDER:
        case PROPERTY_ID_APPLYFILTER:
        case PROPERTY_ID_FONT:
        case PROPERTY_ID_ROW_HEIGHT:
        case PROPERTY_ID_AUTOGROW:
        case PROPERTY_ID_TEXTCOLOR:
        case PROPERTY_ID_TEXTLINECOLOR:
        case PROPERTY_ID_TEXTEMPHASIS:
        case PROPERTY_ID_TEXTRELIEF:
        case PROPERTY_ID_FONTCHARWIDTH:
        case PROPERTY_ID_FONTCHARSET:
        case PROPERTY_ID_FONTFAMILY:
        case PROPERTY_ID_FONTHEIGHT:
        case PROPERTY_ID_FONTKERNING:
        case PROPERTY_ID_FONTNAME:
        case PROPERTY_ID_FONTORIENTATION:
        case PROPERTY_ID_FONTPITCH:
        case PROPERTY_ID_FONTSLANT:
        case PROPERTY_ID_FONTSTRIKEOUT:
        case PROPERTY_ID_FONTSTYLENAME:
        case PROPERTY_ID_FONTUNDERLINE:
        case PROPERTY_ID_FONTWEIGHT:
        case PROPERTY_ID_FONTWIDTH:
        case PROPERTY_ID_FONTWORDLINEMODE:
        case PROPERTY_ID_FONTTYPE:
            bRet = ODataSettings::convertFastPropertyValue(rConvertedValue, rOldValue,nHandle,rValue);
            break;

        default:
            {
                Any aValue;
                getFastPropertyValue(aValue,nHandle);
                bRet = ::comphelper::tryPropertyValue(rConvertedValue,rOldValue,rValue,aValue,::cppu::UnoType<OUString>::get());
            }
            break; // we assume that it works
    }
    return bRet;
}

void ODBTableDecorator::setFastPropertyValue_NoBroadcast(sal_Int32 _nHandle, const Any& _rValue)
{
    switch(_nHandle)
    {
        case PROPERTY_ID_PRIVILEGES:
            SAL_WARN("dbaccess", "Property is readonly!");
            [[fallthrough]];
        case PROPERTY_ID_FILTER:
        case PROPERTY_ID_ORDER:
        case PROPERTY_ID_APPLYFILTER:
        case PROPERTY_ID_FONT:
        case PROPERTY_ID_ROW_HEIGHT:
        case PROPERTY_ID_AUTOGROW:
        case PROPERTY_ID_TEXTCOLOR:
        case PROPERTY_ID_TEXTLINECOLOR:
        case PROPERTY_ID_TEXTEMPHASIS:
        case PROPERTY_ID_TEXTRELIEF:
        case PROPERTY_ID_FONTCHARWIDTH:
        case PROPERTY_ID_FONTCHARSET:
        case PROPERTY_ID_FONTFAMILY:
        case PROPERTY_ID_FONTHEIGHT:
        case PROPERTY_ID_FONTKERNING:
        case PROPERTY_ID_FONTNAME:
        case PROPERTY_ID_FONTORIENTATION:
        case PROPERTY_ID_FONTPITCH:
        case PROPERTY_ID_FONTSLANT:
        case PROPERTY_ID_FONTSTRIKEOUT:
        case PROPERTY_ID_FONTSTYLENAME:
        case PROPERTY_ID_FONTUNDERLINE:
        case PROPERTY_ID_FONTWEIGHT:
        case PROPERTY_ID_FONTWIDTH:
        case PROPERTY_ID_FONTWORDLINEMODE:
        case PROPERTY_ID_FONTTYPE:

            ODataSettings::setFastPropertyValue_NoBroadcast(_nHandle, _rValue);
            break;
        case PROPERTY_ID_CATALOGNAME:
            {
                Reference<XPropertySet> xProp(m_xTable,UNO_QUERY);
                xProp->setPropertyValue(PROPERTY_CATALOGNAME,_rValue);
            }
            break;
        case PROPERTY_ID_SCHEMANAME:
            {
                Reference<XPropertySet> xProp(m_xTable,UNO_QUERY);
                xProp->setPropertyValue(PROPERTY_SCHEMANAME,_rValue);
            }
            break;
        case PROPERTY_ID_NAME:
            {
                Reference<XPropertySet> xProp(m_xTable,UNO_QUERY);
                xProp->setPropertyValue(PROPERTY_NAME,_rValue);
            }
            break;
        case PROPERTY_ID_DESCRIPTION:
            {
                Reference<XPropertySet> xProp(m_xTable,UNO_QUERY);
                xProp->setPropertyValue(PROPERTY_DESCRIPTION,_rValue);
            }
            break;
        case PROPERTY_ID_TYPE:
            {
                Reference<XPropertySet> xProp(m_xTable,UNO_QUERY);
                xProp->setPropertyValue(PROPERTY_TYPE,_rValue);
            }
            break;
    }
}

void ODBTableDecorator::getFastPropertyValue(Any& _rValue, sal_Int32 _nHandle) const
{

    switch(_nHandle)
    {
        case PROPERTY_ID_PRIVILEGES:
            {
                if ( -1 == m_nPrivileges )
                    fillPrivileges();
                Reference<XPropertySet> xProp(m_xTable,UNO_QUERY);
                Reference<XPropertySetInfo> xInfo = xProp->getPropertySetInfo();
                if ( xInfo->hasPropertyByName(PROPERTY_PRIVILEGES) )
                {
                    _rValue <<= m_nPrivileges;
                    break;
                }
            }
            [[fallthrough]];

        case PROPERTY_ID_FILTER:
        case PROPERTY_ID_ORDER:
        case PROPERTY_ID_APPLYFILTER:
        case PROPERTY_ID_FONT:
        case PROPERTY_ID_ROW_HEIGHT:
        case PROPERTY_ID_AUTOGROW:
        case PROPERTY_ID_TEXTCOLOR:
        case PROPERTY_ID_TEXTLINECOLOR:
        case PROPERTY_ID_TEXTEMPHASIS:
        case PROPERTY_ID_TEXTRELIEF:
        case PROPERTY_ID_FONTCHARWIDTH:
        case PROPERTY_ID_FONTCHARSET:
        case PROPERTY_ID_FONTFAMILY:
        case PROPERTY_ID_FONTHEIGHT:
        case PROPERTY_ID_FONTKERNING:
        case PROPERTY_ID_FONTNAME:
        case PROPERTY_ID_FONTORIENTATION:
        case PROPERTY_ID_FONTPITCH:
        case PROPERTY_ID_FONTSLANT:
        case PROPERTY_ID_FONTSTRIKEOUT:
        case PROPERTY_ID_FONTSTYLENAME:
        case PROPERTY_ID_FONTUNDERLINE:
        case PROPERTY_ID_FONTWEIGHT:
        case PROPERTY_ID_FONTWIDTH:
        case PROPERTY_ID_FONTWORDLINEMODE:
        case PROPERTY_ID_FONTTYPE:
            ODataSettings::getFastPropertyValue(_rValue, _nHandle);
            break;
        case PROPERTY_ID_CATALOGNAME:
            {
                Reference<XPropertySet> xProp(m_xTable,UNO_QUERY);
                _rValue = xProp->getPropertyValue(PROPERTY_CATALOGNAME);
            }
            break;
        case PROPERTY_ID_SCHEMANAME:
            {
                Reference<XPropertySet> xProp(m_xTable,UNO_QUERY);
                _rValue = xProp->getPropertyValue(PROPERTY_SCHEMANAME);
            }
            break;
        case PROPERTY_ID_NAME:
            {
                Reference<XPropertySet> xProp(m_xTable,UNO_QUERY);
                _rValue = xProp->getPropertyValue(PROPERTY_NAME);
            }
            break;
        case PROPERTY_ID_DESCRIPTION:
            {
                Reference<XPropertySet> xProp(m_xTable,UNO_QUERY);
                _rValue = xProp->getPropertyValue(PROPERTY_DESCRIPTION);
            }
            break;
        case PROPERTY_ID_TYPE:
            {
                Reference<XPropertySet> xProp(m_xTable,UNO_QUERY);
                _rValue = xProp->getPropertyValue(PROPERTY_TYPE);
            }
            break;
        default:
            SAL_WARN("dbaccess", "Invalid Handle for table");
    }
}

void ODBTableDecorator::construct()
{
    bool bNotFound = true;
    Reference<XPropertySet> xProp(m_xTable,UNO_QUERY);
    if ( xProp.is() )
    {
        Reference<XPropertySetInfo> xInfo = xProp->getPropertySetInfo();
        bNotFound = !xInfo->hasPropertyByName(PROPERTY_PRIVILEGES);
    }
    if ( bNotFound )
        registerProperty(PROPERTY_PRIVILEGES, PROPERTY_ID_PRIVILEGES, PropertyAttribute::BOUND  | PropertyAttribute::READONLY,
                        &m_nPrivileges, ::cppu::UnoType<sal_Int32>::get());
}

::cppu::IPropertyArrayHelper* ODBTableDecorator::createArrayHelper(sal_Int32 /*_nId*/) const
{
    Reference<XPropertySet> xProp(m_xTable,UNO_QUERY);
    Reference<XPropertySetInfo> xInfo = xProp->getPropertySetInfo();

    Sequence< Property > aTableProps = xInfo->getProperties();
    for (Property & prop : asNonConstRange(aTableProps))
    {
        if (prop.Name == PROPERTY_CATALOGNAME)
            prop.Handle = PROPERTY_ID_CATALOGNAME;
        else if (prop.Name == PROPERTY_SCHEMANAME)
            prop.Handle = PROPERTY_ID_SCHEMANAME;
        else if (prop.Name == PROPERTY_NAME)
            prop.Handle = PROPERTY_ID_NAME;
        else if (prop.Name == PROPERTY_DESCRIPTION)
            prop.Handle = PROPERTY_ID_DESCRIPTION;
        else if (prop.Name == PROPERTY_TYPE)
            prop.Handle = PROPERTY_ID_TYPE;
        else if (prop.Name == PROPERTY_PRIVILEGES)
            prop.Handle = PROPERTY_ID_PRIVILEGES;
    }

    describeProperties(aTableProps);

    return new ::cppu::OPropertyArrayHelper(aTableProps);
}

::cppu::IPropertyArrayHelper & SAL_CALL ODBTableDecorator::getInfoHelper()
{
    Reference<XPropertySet> xProp(m_xTable,UNO_QUERY);

    Reference<XPropertySetInfo> xInfo = xProp->getPropertySetInfo();
    bool bIsDescriptor = (xInfo->getPropertyByName(PROPERTY_NAME).Attributes & PropertyAttribute::READONLY) == 0;

    return *ODBTableDecorator_PROP::getArrayHelper( bIsDescriptor ? 0 : 1 );

    // TODO: this is a HACK, and prone to errors
    // The OIdPropertyArrayUsageHelper is intended for classes where there exists a known, limited
    // number of different property set infos (distinguished by the ID), all implemented by this very
    // same class.
    // However, in this case here we have an unknown, potentially unlimited number of different
    // property set infos: Depending on the table for which we act as decorator, different property
    // sets might exist.
}

// XServiceInfo
OUString SAL_CALL ODBTableDecorator::getImplementationName()
    {
        return u"com.sun.star.sdb.dbaccess.ODBTableDecorator"_ustr;
    }
sal_Bool SAL_CALL ODBTableDecorator::supportsService(const OUString& _rServiceName)
    {
        const css::uno::Sequence< OUString > aSupported(getSupportedServiceNames());
        for (const OUString& s : aSupported)
            if (s == _rServiceName)
                return true;

        return false;
    }
css::uno::Sequence< OUString > SAL_CALL ODBTableDecorator::getSupportedServiceNames()
{
    return { SERVICE_SDBCX_TABLE };
}


Any SAL_CALL ODBTableDecorator::queryInterface( const Type & rType )
{
    Any aRet;
    if(m_xTable.is())
    {
        aRet = m_xTable->queryInterface(rType);
        if(aRet.hasValue())
        {   // now we know that our table supports this type so we return ourself
            aRet = OTableDescriptor_BASE::queryInterface(rType);
            if(!aRet.hasValue())
                aRet = ODataSettings::queryInterface(rType);
        }
    }

    return aRet;
}

Sequence< Type > SAL_CALL ODBTableDecorator::getTypes(  )
{
    Reference<XTypeProvider> xTypes(m_xTable,UNO_QUERY);
    OSL_ENSURE(xTypes.is(),"Table must be a TypeProvider!");
    return xTypes->getTypes();
}

// XRename,
void SAL_CALL ODBTableDecorator::rename( const OUString& _rNewName )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    ::connectivity::checkDisposed(OTableDescriptor_BASE::rBHelper.bDisposed);
    Reference<XRename> xRename(m_xTable,UNO_QUERY);
    if(!xRename.is())
        throw SQLException(DBA_RES(RID_STR_NO_TABLE_RENAME),*this,SQLSTATE_GENERAL,1000,Any() );
    // not supported
    xRename->rename(_rNewName);
}

// XAlterTable,
void SAL_CALL ODBTableDecorator::alterColumnByName( const OUString& _rName, const Reference< XPropertySet >& _rxDescriptor )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    ::connectivity::checkDisposed(OTableDescriptor_BASE::rBHelper.bDisposed);
    Reference<XAlterTable> xAlter(m_xTable,UNO_QUERY);
    if(!xAlter.is())
        throw SQLException(DBA_RES(RID_STR_COLUMN_ALTER_BY_NAME),*this,SQLSTATE_GENERAL,1000,Any() );
    xAlter->alterColumnByName(_rName,_rxDescriptor);
    if(m_pColumns)
        m_pColumns->refresh();
}

void SAL_CALL ODBTableDecorator::alterColumnByIndex( sal_Int32 _nIndex, const Reference< XPropertySet >& _rxDescriptor )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    ::connectivity::checkDisposed(OTableDescriptor_BASE::rBHelper.bDisposed);
    Reference<XAlterTable> xAlter(m_xTable,UNO_QUERY);
    if(!xAlter.is())
        throw SQLException(DBA_RES(RID_STR_COLUMN_ALTER_BY_INDEX),*this,SQLSTATE_GENERAL,1000,Any() );
    // not supported
    xAlter->alterColumnByIndex(_nIndex,_rxDescriptor);
    if(m_pColumns)
        m_pColumns->refresh();
}

Reference< XNameAccess> ODBTableDecorator::getIndexes()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    ::connectivity::checkDisposed(OTableDescriptor_BASE::rBHelper.bDisposed);
    return Reference< XIndexesSupplier>(m_xTable,UNO_QUERY_THROW)->getIndexes();
}

Reference< XIndexAccess> ODBTableDecorator::getKeys()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    ::connectivity::checkDisposed(OTableDescriptor_BASE::rBHelper.bDisposed);
    return Reference< XKeysSupplier>(m_xTable,UNO_QUERY_THROW)->getKeys();
}

Reference< XNameAccess> ODBTableDecorator::getColumns()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    ::connectivity::checkDisposed(OTableDescriptor_BASE::rBHelper.bDisposed);

    if(!m_pColumns)
        refreshColumns();

    return m_pColumns.get();
}

OUString SAL_CALL ODBTableDecorator::getName()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    ::connectivity::checkDisposed(OTableDescriptor_BASE::rBHelper.bDisposed);
    Reference<XNamed> xName(m_xTable,UNO_QUERY);
    OSL_ENSURE(xName.is(),"Table should support the XNamed interface");
    return xName->getName();
}

sal_Int64 SAL_CALL ODBTableDecorator::getSomething( const Sequence< sal_Int8 >& rId )
{
    if (comphelper::isUnoTunnelId<ODBTableDecorator>(rId))
        return comphelper::getSomething_cast(this);

    sal_Int64 nRet = 0;
    Reference<XUnoTunnel> xTunnel(m_xTable,UNO_QUERY);
    if(xTunnel.is())
        nRet = xTunnel->getSomething(rId);
    return nRet;
}

const Sequence< sal_Int8 > & ODBTableDecorator::getUnoTunnelId()
{
    static const comphelper::UnoIdInit implId;
    return implId.getSeq();
}

void ODBTableDecorator::fillPrivileges() const
{
    // somebody is asking for the privileges and we do not know them, yet
    m_nPrivileges = 0;
    try
    {
        Reference<XPropertySet> xProp(m_xTable,UNO_QUERY);
        if ( xProp.is() )
        {
            if ( xProp->getPropertySetInfo()->hasPropertyByName(PROPERTY_PRIVILEGES) )
            {
                xProp->getPropertyValue(PROPERTY_PRIVILEGES) >>= m_nPrivileges;
            }
            if ( m_nPrivileges == 0 ) // second chance
            {
                OUString sCatalog,sSchema,sName;
                xProp->getPropertyValue(PROPERTY_CATALOGNAME)   >>= sCatalog;
                xProp->getPropertyValue(PROPERTY_SCHEMANAME)    >>= sSchema;
                xProp->getPropertyValue(PROPERTY_NAME)          >>= sName;
                m_nPrivileges = ::dbtools::getTablePrivileges(m_xMetaData, sCatalog,sSchema, sName);
            }
        }
    }
    catch(const SQLException&)
    {
        SAL_WARN("dbaccess", "ODBTableDecorator::ODBTableDecorator : could not collect the privileges !");
    }
}

Reference< XPropertySet > SAL_CALL ODBTableDecorator::createDataDescriptor(  )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    ::connectivity::checkDisposed(OTableDescriptor_BASE::rBHelper.bDisposed);

    Reference< XDataDescriptorFactory > xFactory( m_xTable, UNO_QUERY );
    OSL_ENSURE( xFactory.is(), "ODBTableDecorator::createDataDescriptor: invalid table!" );
    Reference< XColumnsSupplier > xColsSupp;
    if ( xFactory.is() )
        xColsSupp.set(xFactory->createDataDescriptor(), css::uno::UNO_QUERY);

    return new ODBTableDecorator(
        m_xConnection,
        xColsSupp,
        m_xNumberFormats,
        nullptr
    );
}

Reference< css::beans::XPropertySetInfo > SAL_CALL ODBTableDecorator::getPropertySetInfo(  )
{
    return ::cppu::OPropertySetHelper::createPropertySetInfo(getInfoHelper());
}

void ODBTableDecorator::refreshColumns()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    ::connectivity::checkDisposed(OTableDescriptor_BASE::rBHelper.bDisposed);

    std::vector< OUString> aVector;

    Reference<XNameAccess> xNames;
    if(m_xTable.is())
    {
        xNames = m_xTable->getColumns();
        if(xNames.is())
        {
            const Sequence< OUString> aNames = xNames->getElementNames();
            aVector.insert(aVector.end(), aNames.begin(), aNames.end());
        }
    }
    if(!m_pColumns)
    {
        OColumns* pCol = new OColumns(*this,m_aMutex,xNames,m_xMetaData.is() && m_xMetaData->supportsMixedCaseQuotedIdentifiers(),aVector,
                                    this,this,
                                    m_xMetaData.is() && m_xMetaData->supportsAlterTableWithAddColumn(),
                                    m_xMetaData.is() && m_xMetaData->supportsAlterTableWithDropColumn());

        pCol->setParent(*this);
        rtl::Reference<OContainerMediator> pMediator = new OContainerMediator( pCol, m_xColumnDefinitions );
        m_xColumnMediator = pMediator;
        pCol->setMediator( pMediator.get() );
        m_pColumns.reset(pCol);
    }
    else
        m_pColumns->reFill(aVector);
}

rtl::Reference<OColumn> ODBTableDecorator::createColumn(const OUString& _rName) const
{
    rtl::Reference<OColumn> pReturn;

    Reference<XNameAccess> xNames;
    if ( m_xTable.is() )
    {
        xNames = m_xTable->getColumns();

        if ( xNames.is() && xNames->hasByName(_rName) )
        {
            Reference<XPropertySet> xProp(xNames->getByName(_rName),UNO_QUERY);

            Reference<XPropertySet> xColumnDefinition;
            if ( m_xColumnDefinitions.is() && m_xColumnDefinitions->hasByName(_rName))
                xColumnDefinition.set(m_xColumnDefinitions->getByName(_rName),UNO_QUERY);

            pReturn = new OTableColumnWrapper( xProp, xColumnDefinition, false );
        }
    }
    return pReturn;
}

void ODBTableDecorator::columnAppended( const Reference< XPropertySet >& /*_rxSourceDescriptor*/ )
{
    // not interested in
}

void ODBTableDecorator::columnDropped(const OUString& _sName)
{
    Reference<XDrop> xDrop(m_xColumnDefinitions,UNO_QUERY);
    if ( xDrop.is() && m_xColumnDefinitions->hasByName(_sName) )
        xDrop->dropByName(_sName);
}

Reference< XPropertySet > ODBTableDecorator::createColumnDescriptor()
{
    if(!m_xTable.is())
        return nullptr;
    Reference<XDataDescriptorFactory> xNames(m_xTable->getColumns(),UNO_QUERY);
    if ( !xNames.is() )
        return nullptr;
    return new OTableColumnDescriptorWrapper( xNames->createDataDescriptor(), false, true );
}

void SAL_CALL ODBTableDecorator::acquire() noexcept
{
    OTableDescriptor_BASE::acquire();
}

void SAL_CALL ODBTableDecorator::release() noexcept
{
    OTableDescriptor_BASE::release();
}

void SAL_CALL ODBTableDecorator::setName( const OUString& /*aName*/ )
{
    throwFunctionNotSupportedRuntimeException( u"XNamed::setName"_ustr, *this );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
