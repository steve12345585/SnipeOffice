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

#include <core_resource.hxx>
#include <datasourceconnector.hxx>
#include <osl/diagnose.h>
#include <com/sun/star/sdbc/XWarningsSupplier.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/sdb/XCompletedConnection.hpp>
#include <com/sun/star/task/InteractionHandler.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/sdbc/SQLWarning.hpp>
#include <comphelper/namedvaluecollection.hxx>
#include <connectivity/dbexception.hxx>
#include <com/sun/star/sdbc/XDataSource.hpp>
#include <UITools.hxx>
#include <utility>
#include <vcl/mnemonic.hxx>
#include <vcl/outdev.hxx>
#include <vcl/stdtext.hxx>
#include <vcl/weld.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <cppuhelper/exc_hlp.hxx>
#include <strings.hrc>
#include <strings.hxx>

namespace dbaui
{

    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::lang;
    using namespace ::com::sun::star::sdb;
    using namespace ::com::sun::star::sdbc;
    using namespace ::com::sun::star::task;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::container;
    using namespace ::com::sun::star::frame;
    using namespace ::dbtools;

    // ODatasourceConnector
    ODatasourceConnector::ODatasourceConnector(const Reference< XComponentContext >& _rxContext, weld::Window* _pMessageParent)
        :m_pErrorMessageParent(_pMessageParent)
        ,m_xContext(_rxContext)
    {
    }

    ODatasourceConnector::ODatasourceConnector( const Reference< XComponentContext >& _rxContext, weld::Window* _pMessageParent,
        OUString _sContextInformation )
        :m_pErrorMessageParent(_pMessageParent)
        ,m_xContext(_rxContext)
        ,m_sContextInformation(std::move( _sContextInformation ))
    {
    }

    Reference< XConnection > ODatasourceConnector::connect( const OUString& _rDataSourceName,
        ::dbtools::SQLExceptionInfo* _pErrorInfo ) const
    {
        Reference< XConnection > xConnection;

        OSL_ENSURE(isValid(), "ODatasourceConnector::connect: invalid object!");
        if (!isValid())
            return xConnection;

        // get the data source
        Reference< XDataSource > xDatasource =
            getDataSourceByName( _rDataSourceName, m_pErrorMessageParent, m_xContext, _pErrorInfo );

        if ( xDatasource.is() )
            xConnection = connect( xDatasource, _pErrorInfo );
        return xConnection;
    }

    Reference< XConnection > ODatasourceConnector::connect(const Reference< XDataSource>& _xDataSource,
        ::dbtools::SQLExceptionInfo* _pErrorInfo ) const
    {
        Reference< XConnection > xConnection;

        OSL_ENSURE( isValid() && _xDataSource.is(), "ODatasourceConnector::connect: invalid object or argument!" );
        if ( !isValid() || !_xDataSource.is() )
            return xConnection;

        // get user/password
        OUString sPassword, sUser;
        bool bPwdRequired = false;
        Reference<XPropertySet> xProp(_xDataSource,UNO_QUERY);
        try
        {
            xProp->getPropertyValue(PROPERTY_PASSWORD) >>= sPassword;
            xProp->getPropertyValue(PROPERTY_ISPASSWORDREQUIRED) >>= bPwdRequired;
            xProp->getPropertyValue(PROPERTY_USER) >>= sUser;
        }
        catch(Exception&)
        {
            DBG_UNHANDLED_EXCEPTION("dbaccess");
        }

        // try to connect
        SQLExceptionInfo aInfo;
        try
        {
            if (bPwdRequired && sPassword.isEmpty())
            {   // password required, but empty -> connect using an interaction handler
                Reference< XCompletedConnection > xConnectionCompletion( _xDataSource, UNO_QUERY_THROW );

                Reference< XModel > xModel( getDataSourceOrModel( _xDataSource ), UNO_QUERY_THROW );
                ::comphelper::NamedValueCollection aArgs( xModel->getArgs() );
                Reference< XInteractionHandler > xHandler( aArgs.getOrDefault( u"InteractionHandler"_ustr, Reference< XInteractionHandler >() ) );

                if ( !xHandler.is() )
                {
                    // instantiate the default SDB interaction handler
                    xHandler = InteractionHandler::createWithParent(m_xContext, m_pErrorMessageParent ? m_pErrorMessageParent->GetXWindow() : nullptr);
                }

                xConnection = xConnectionCompletion->connectWithCompletion(xHandler);
            }
            else
            {
                xConnection = _xDataSource->getConnection(sUser, sPassword);
            }
        }
        catch( const SQLException& )
        {
            aInfo = ::cppu::getCaughtException();
        }
        catch(const Exception&)
        {
            DBG_UNHANDLED_EXCEPTION("dbaccess");
        }

        if ( !aInfo.isValid() )
        {
            // there was no error during connecting, but perhaps a warning?
            Reference< XWarningsSupplier > xConnectionWarnings( xConnection, UNO_QUERY );
            if ( xConnectionWarnings.is() )
            {
                try
                {
                    Any aWarnings( xConnectionWarnings->getWarnings() );
                    if ( aWarnings.hasValue() )
                    {
                        OUString sMessage( DBA_RES( STR_WARNINGS_DURING_CONNECT ) );
                        sMessage = sMessage.replaceFirst( "$buttontext$", GetStandardText( StandardButtonType::More ) );
                        sMessage = removeMnemonicFromString( sMessage );

                        SQLWarning aContext(sMessage, {}, {}, 0, aWarnings);
                        aInfo = aContext;
                    }
                    xConnectionWarnings->clearWarnings();
                }
                catch( const Exception& )
                {
                    DBG_UNHANDLED_EXCEPTION("dbaccess");
                }
            }
        }
        else
        {
            if ( !m_sContextInformation.isEmpty() )
            {
                SQLException aError(m_sContextInformation, {}, {}, 0, aInfo.get());
                aInfo = aError;
            }
        }

        // was there an error?
        if ( aInfo.isValid() )
        {
            if ( _pErrorInfo )
            {
                *_pErrorInfo = std::move(aInfo);
            }
            else
            {
                showError(aInfo, m_pErrorMessageParent ? m_pErrorMessageParent->GetXWindow() : nullptr, m_xContext);
            }
        }
        return xConnection;
    }

}   // namespace dbaui

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
