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
#include <defaultobjectnamecheck.hxx>

#include <strings.hrc>

#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/sdb/CommandType.hpp>
#include <com/sun/star/sdbc/SQLException.hpp>

#include <connectivity/dbexception.hxx>
#include <connectivity/dbmetadata.hxx>

#include <rtl/ustrbuf.hxx>

#include <comphelper/diagnose_ex.hxx>
#include <cppuhelper/exc_hlp.hxx>

#include <memory>
#include <string_view>

namespace dbaui
{

    using ::com::sun::star::uno::Reference;
    using ::com::sun::star::lang::IllegalArgumentException;
    using ::com::sun::star::container::XHierarchicalNameAccess;
    using ::com::sun::star::sdbc::SQLException;
    using ::com::sun::star::uno::Exception;
    using ::com::sun::star::sdbc::XConnection;
    using ::com::sun::star::sdb::tools::XConnectionTools;
    using ::com::sun::star::uno::UNO_QUERY;

    using namespace dbtools;

    namespace CommandType = ::com::sun::star::sdb::CommandType;

    // helper
    namespace
    {
        void lcl_fillNameExistsError( std::u16string_view _rObjectName, SQLExceptionInfo& _out_rErrorToDisplay )
        {
            OUString sErrorMessage = DBA_RES(STR_NAMED_OBJECT_ALREADY_EXISTS);
            SQLException aError(sErrorMessage.replaceAll("$#$", _rObjectName), {}, {}, 0, {});
            _out_rErrorToDisplay = aError;
        }

    }

    // HierarchicalNameCheck
    HierarchicalNameCheck::HierarchicalNameCheck( const Reference< XHierarchicalNameAccess >& _rxNames, const OUString& _rRelativeRoot )
    {
        mxHierarchicalNames = _rxNames;
        msRelativeRoot = _rRelativeRoot;

        if ( !mxHierarchicalNames.is() )
            throw IllegalArgumentException();
    }

    HierarchicalNameCheck::~HierarchicalNameCheck()
    {
    }

    bool HierarchicalNameCheck::isNameValid( const OUString& _rObjectName, SQLExceptionInfo& _out_rErrorToDisplay ) const
    {
        try
        {
            OUStringBuffer aCompleteName;
            if ( !msRelativeRoot.isEmpty() )
            {
                aCompleteName.append( msRelativeRoot + "/" );
            }
            aCompleteName.append( _rObjectName );

            OUString sCompleteName( aCompleteName.makeStringAndClear() );
            if ( !mxHierarchicalNames->hasByHierarchicalName( sCompleteName ) )
                return true;
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("dbaccess");
        }

        lcl_fillNameExistsError( _rObjectName, _out_rErrorToDisplay );
        return false;
    }

    // DynamicTableOrQueryNameCheck
    DynamicTableOrQueryNameCheck::DynamicTableOrQueryNameCheck( const Reference< XConnection >& _rxSdbLevelConnection, sal_Int32 _nCommandType )
    {
        Reference< XConnectionTools > xConnTools( _rxSdbLevelConnection, UNO_QUERY );
        if ( xConnTools.is() )
            mxObjectNames.set( xConnTools->getObjectNames() );
        if ( !mxObjectNames.is() )
            throw IllegalArgumentException();

        if ( ( _nCommandType != CommandType::QUERY ) && ( _nCommandType != CommandType::TABLE ) )
            throw IllegalArgumentException();
        mnCommandType = _nCommandType;
    }

    DynamicTableOrQueryNameCheck::~DynamicTableOrQueryNameCheck()
    {
    }

    bool DynamicTableOrQueryNameCheck::isNameValid( const OUString& _rObjectName, ::dbtools::SQLExceptionInfo& _out_rErrorToDisplay ) const
    {
        try
        {
            mxObjectNames->checkNameForCreate( mnCommandType, _rObjectName );
            return true;
        }
        catch( const SQLException& )
        {
            _out_rErrorToDisplay = ::dbtools::SQLExceptionInfo( ::cppu::getCaughtException() );
        }
        return false;
    }

} // namespace dbaui

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
