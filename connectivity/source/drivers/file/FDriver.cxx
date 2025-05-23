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

#include <file/FDriver.hxx>
#include <file/FConnection.hxx>
#include <file/fcode.hxx>
#include <comphelper/servicehelper.hxx>
#include <comphelper/types.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <connectivity/dbexception.hxx>
#include <strings.hrc>
#include <resource/sharedresources.hxx>
#include <utility>


using namespace connectivity::file;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::sdbc;
using namespace com::sun::star::sdbcx;
using namespace com::sun::star::container;

OFileDriver::OFileDriver(css::uno::Reference< css::uno::XComponentContext > _xContext)
    : ODriver_BASE(m_aMutex)
    ,m_xContext(std::move(_xContext))
{
}

void OFileDriver::disposing()
{
    ::osl::MutexGuard aGuard(m_aMutex);


    for (auto const& connection : m_xConnections)
    {
        rtl::Reference< OConnection > xComp(connection);
        if (xComp.is())
            xComp->dispose();
    }
    m_xConnections.clear();

    ODriver_BASE::disposing();
}

// XServiceInfo

OUString SAL_CALL OFileDriver::getImplementationName(  )
{
    return u"com.sun.star.sdbc.driver.file.Driver"_ustr;
}

sal_Bool SAL_CALL OFileDriver::supportsService( const OUString& _rServiceName )
{
    return cppu::supportsService(this, _rServiceName);
}


Sequence< OUString > SAL_CALL OFileDriver::getSupportedServiceNames(  )
{
    return { u"com.sun.star.sdbc.Driver"_ustr, u"com.sun.star.sdbcx.Driver"_ustr };
}


Reference< XConnection > SAL_CALL OFileDriver::connect( const OUString& url, const Sequence< PropertyValue >& info )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(ODriver_BASE::rBHelper.bDisposed);

    rtl::Reference<OConnection> pCon = new OConnection(this);
    pCon->construct(url,info);
    m_xConnections.push_back(pCon);

    return pCon;
}

sal_Bool SAL_CALL OFileDriver::acceptsURL( const OUString& url )
{
    return url.startsWith("sdbc:file:");
}

Sequence< DriverPropertyInfo > SAL_CALL OFileDriver::getPropertyInfo( const OUString& url, const Sequence< PropertyValue >& /*info*/ )
{
    if ( acceptsURL(url) )
    {

        Sequence< OUString > aBoolean { u"0"_ustr, u"1"_ustr };

        return
        {
             {
                u"CharSet"_ustr
                ,u"CharSet of the database."_ustr
                ,false
                ,{}
                ,{}
             },
             {
                u"Extension"_ustr
                ,u"Extension of the file format."_ustr
                ,false
                ,u".*"_ustr
                ,{}
             },
             {
                u"ShowDeleted"_ustr
                ,u"Display inactive records."_ustr
                ,false
                ,u"0"_ustr
                ,aBoolean
             },
             {
                u"EnableSQL92Check"_ustr
                ,u"Use SQL92 naming constraints."_ustr
                ,false
                ,u"0"_ustr
                ,aBoolean
             },
             {
                u"UseRelativePath"_ustr
                ,u"Handle the connection url as relative path."_ustr
                ,false
                ,u"0"_ustr
                ,aBoolean
             },
             {
                u"URL"_ustr
                ,u"The URL of the database document which is used to create an absolute path."_ustr
                ,false
                ,{}
                ,{}
             }
        };
    } // if ( acceptsURL(url) )
    {
        ::connectivity::SharedResources aResources;
        const OUString sMessage = aResources.getResourceString(STR_URI_SYNTAX_ERROR);
        ::dbtools::throwGenericSQLException(sMessage ,*this);
    } // if ( ! acceptsURL(url) )
}

sal_Int32 SAL_CALL OFileDriver::getMajorVersion(  )
{
    return 1;
}

sal_Int32 SAL_CALL OFileDriver::getMinorVersion(  )
{
    return 0;
}


// XDataDefinitionSupplier
Reference< XTablesSupplier > SAL_CALL OFileDriver::getDataDefinitionByConnection( const Reference< css::sdbc::XConnection >& connection )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(ODriver_BASE::rBHelper.bDisposed);

    if (OConnection* pSearchConnection = comphelper::getFromUnoTunnel<OConnection>(connection))
    {
        for (auto const& elem : m_xConnections)
        {
            if (elem.get().get() == pSearchConnection)
                return pSearchConnection->createCatalog();
        }
    }
    return {};
}


Reference< XTablesSupplier > SAL_CALL OFileDriver::getDataDefinitionByURL( const OUString& url, const Sequence< PropertyValue >& info )
{
    if ( ! acceptsURL(url) )
    {
        ::connectivity::SharedResources aResources;
        const OUString sMessage = aResources.getResourceString(STR_URI_SYNTAX_ERROR);
        ::dbtools::throwGenericSQLException(sMessage ,*this);
    }
    return getDataDefinitionByConnection(connect(url,info));
}


OOperandAttr::OOperandAttr(sal_uInt16 _nPos,const Reference< XPropertySet>& _xColumn)
    : OOperandRow(_nPos,::comphelper::getINT32(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_TYPE))))
{
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
