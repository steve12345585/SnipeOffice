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

#include <sdbcoretools.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XChild.hpp>
#include <com/sun/star/util/XModifiable.hpp>
#include <com/sun/star/sdb/XDocumentDataSource.hpp>
#include <com/sun/star/task/InteractionRequestStringResolver.hpp>
#include <com/sun/star/embed/XTransactedObject.hpp>
#include <com/sun/star/embed/ElementModes.hpp>

#include <comphelper/diagnose_ex.hxx>
#include <comphelper/interaction.hxx>
#include <rtl/ref.hxx>

namespace dbaccess
{

    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::util;
    using namespace ::com::sun::star::io;
    using namespace ::com::sun::star::sdb;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::task;
    using namespace ::com::sun::star::embed;
    using namespace ::com::sun::star::container;

    void notifyDataSourceModified(const css::uno::Reference< css::uno::XInterface >& _rxObject)
    {
        Reference< XInterface > xDs = getDataSource( _rxObject );
        Reference<XDocumentDataSource> xDocumentDataSource(xDs,UNO_QUERY);
        if ( xDocumentDataSource.is() )
            xDs = xDocumentDataSource->getDatabaseDocument();
        Reference< XModifiable > xModi( xDs, UNO_QUERY );
        if ( xModi.is() )
            xModi->setModified(true);
    }

    Reference< XInterface > getDataSource( const Reference< XInterface >& _rxDependentObject )
    {
        Reference< XInterface > xParent = _rxDependentObject;
        Reference< XInterface > xReturn;
        while( xParent.is() )
        {
            xReturn = xParent;
            Reference<XChild> xChild(xParent,UNO_QUERY);
            xParent.set(xChild.is() ? xChild->getParent() : Reference< XInterface >(),UNO_QUERY);
        }
        return xReturn;
    }

    OUString extractExceptionMessage( const Reference<XComponentContext> & _rContext, const Any& _rError )
    {
        OUString sDisplayMessage;

        try
        {
            Reference< XInteractionRequestStringResolver > xStringResolver = InteractionRequestStringResolver::create(_rContext);

            ::rtl::Reference pRequest( new ::comphelper::OInteractionRequest( _rError ) );
            ::rtl::Reference pApprove( new ::comphelper::OInteractionApprove );
            pRequest->addContinuation( pApprove );
            Optional< OUString > aMessage = xStringResolver->getStringFromInformationalRequest( pRequest );
            if ( aMessage.IsPresent )
                sDisplayMessage = aMessage.Value;
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("dbaccess");
        }

        if ( sDisplayMessage.isEmpty() )
        {
            Exception aExcept;
            _rError >>= aExcept;

            sDisplayMessage = _rError.getValueTypeName() +
                ":\n" +
                aExcept.Message;
        }

        return sDisplayMessage;
    }

    namespace tools::stor {

    bool storageIsWritable_nothrow( const Reference< XStorage >& _rxStorage )
    {
        if ( !_rxStorage.is() )
            return false;

        sal_Int32 nMode = ElementModes::READ;
        try
        {
            Reference< XPropertySet > xStorageProps( _rxStorage, UNO_QUERY_THROW );
            xStorageProps->getPropertyValue( u"OpenMode"_ustr ) >>= nMode;
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("dbaccess");
        }
        return ( nMode & ElementModes::WRITE ) != 0;
    }

    bool commitStorageIfWriteable( const Reference< XStorage >& _rxStorage )
    {
        bool bSuccess = false;
        Reference< XTransactedObject > xTrans( _rxStorage, UNO_QUERY );
        if ( xTrans.is() )
        {
            if ( storageIsWritable_nothrow( _rxStorage ) )
                xTrans->commit();
            bSuccess = true;
        }
        return bSuccess;
    }

}

}   // namespace dbaccess

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
