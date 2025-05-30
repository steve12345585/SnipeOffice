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

#include "tablename.hxx"
#include <core_resource.hxx>
#include <strings.hrc>
#include <strings.hxx>

#include <com/sun/star/sdb/tools/CompositionType.hpp>
#include <com/sun/star/sdbcx/XTablesSupplier.hpp>

#include <connectivity/dbtools.hxx>
#include <comphelper/diagnose_ex.hxx>

namespace sdbtools
{

    using ::com::sun::star::uno::Reference;
    using ::com::sun::star::sdbc::XConnection;
    using ::com::sun::star::uno::RuntimeException;
    using ::com::sun::star::lang::IllegalArgumentException;
    using ::com::sun::star::beans::XPropertySet;
    using ::com::sun::star::container::NoSuchElementException;
    using ::com::sun::star::sdbcx::XTablesSupplier;
    using ::com::sun::star::container::XNameAccess;
    using ::com::sun::star::uno::UNO_QUERY_THROW;
    using ::com::sun::star::lang::WrappedTargetException;
    using ::com::sun::star::uno::Exception;
    using ::com::sun::star::uno::UNO_QUERY;
    using ::com::sun::star::beans::XPropertySetInfo;
    using ::com::sun::star::uno::XComponentContext;

    namespace CompositionType = ::com::sun::star::sdb::tools::CompositionType;

    using namespace ::dbtools;

    // TableName
    TableName::TableName( const Reference<XComponentContext>& _rContext, const Reference< XConnection >& _rxConnection )
        :ConnectionDependentComponent( _rContext )
    {
        setWeakConnection( _rxConnection );
    }

    TableName::~TableName()
    {
    }

    OUString SAL_CALL TableName::getCatalogName()
    {
        EntryGuard aGuard( *this );
        return msCatalog;
    }

    void SAL_CALL TableName::setCatalogName( const OUString& _catalogName )
    {
        EntryGuard aGuard( *this );
        msCatalog = _catalogName;
    }

    OUString SAL_CALL TableName::getSchemaName()
    {
        EntryGuard aGuard( *this );
        return msSchema;
    }

    void SAL_CALL TableName::setSchemaName( const OUString& _schemaName )
    {
        EntryGuard aGuard( *this );
        msSchema = _schemaName;
    }

    OUString SAL_CALL TableName::getTableName()
    {
        EntryGuard aGuard( *this );
        return msName;
    }

    void SAL_CALL TableName::setTableName( const OUString& _tableName )
    {
        EntryGuard aGuard( *this );
        msName = _tableName;
    }

    OUString SAL_CALL TableName::getNameForSelect()
    {
        EntryGuard aGuard( *this );
        return composeTableNameForSelect( getConnection(), msCatalog, msSchema, msName );
    }

    Reference< XPropertySet > SAL_CALL TableName::getTable()
    {
        EntryGuard aGuard( *this );

        Reference< XTablesSupplier > xSuppTables( getConnection(), UNO_QUERY_THROW );
        Reference< XNameAccess > xTables( xSuppTables->getTables(), css::uno::UNO_SET_THROW );

        Reference< XPropertySet > xTable;
        try
        {
            xTable.set( xTables->getByName( getComposedName( CompositionType::Complete, false ) ), UNO_QUERY_THROW );
        }
        catch( const WrappedTargetException& )
        {
            throw NoSuchElementException();
        }
        catch( const RuntimeException& ) { throw; }
        catch( const NoSuchElementException& ) { throw; }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("dbaccess");
            throw NoSuchElementException();
        }

        return xTable;
    }

    void SAL_CALL TableName::setTable( const Reference< XPropertySet >& _table )
    {
        EntryGuard aGuard( *this );

        Reference< XPropertySetInfo > xPSI( _table, UNO_QUERY );
        if  (   !xPSI.is()
            ||  !xPSI->hasPropertyByName( PROPERTY_CATALOGNAME )
            ||  !xPSI->hasPropertyByName( PROPERTY_SCHEMANAME )
            ||  !xPSI->hasPropertyByName( PROPERTY_NAME )
            )
            throw IllegalArgumentException(
                DBA_RES( STR_NO_TABLE_OBJECT ),
                *this,
                0
            );

        try
        {
            OSL_VERIFY( _table->getPropertyValue( PROPERTY_CATALOGNAME ) >>= msCatalog );
            OSL_VERIFY( _table->getPropertyValue( PROPERTY_SCHEMANAME ) >>= msSchema );
            OSL_VERIFY( _table->getPropertyValue( PROPERTY_NAME ) >>= msName );
        }
        catch( const RuntimeException& ) { throw; }
        catch( const Exception& e )
        {
            throw IllegalArgumentException( e.Message, e.Context, 0 );
        }
    }

    namespace
    {
        /** translates a CompositionType into an EComposeRule
            @throws IllegalArgumentException
                if the given value does not denote a valid CompositionType
        */
        EComposeRule lcl_translateCompositionType_throw( sal_Int32 _nType )
        {
            static const struct
            {
                sal_Int32       nCompositionType;
                EComposeRule    eComposeRule;
            }   TypeTable[] =
            {
                { CompositionType::ForTableDefinitions,      EComposeRule::InTableDefinitions },
                { CompositionType::ForIndexDefinitions,      EComposeRule::InIndexDefinitions },
                { CompositionType::ForDataManipulation,      EComposeRule::InDataManipulation },
                { CompositionType::ForProcedureCalls,        EComposeRule::InProcedureCalls },
                { CompositionType::ForPrivilegeDefinitions,  EComposeRule::InPrivilegeDefinitions },
                { CompositionType::Complete,                 EComposeRule::Complete }
            };

            auto const found = std::find_if(std::begin(TypeTable), std::end(TypeTable)
                                            , [_nType](auto const & type){ return type.nCompositionType == _nType; });
            if (found == std::end(TypeTable))
                throw IllegalArgumentException(
                    DBA_RES( STR_INVALID_COMPOSITION_TYPE ),
                    nullptr,
                    0
                );

            return found->eComposeRule;
        }
    }

    OUString SAL_CALL TableName::getComposedName( ::sal_Int32 Type, sal_Bool Quote )
    {
        EntryGuard aGuard( *this );

        return composeTableName(
            getConnection()->getMetaData(),
            msCatalog, msSchema, msName, Quote,
            lcl_translateCompositionType_throw( Type ) );
    }

    void SAL_CALL TableName::setComposedName( const OUString& ComposedName, ::sal_Int32 Type )
    {
        EntryGuard aGuard( *this );

        qualifiedNameComponents(
            getConnection()->getMetaData(),
            ComposedName,
            msCatalog, msSchema, msName,
            lcl_translateCompositionType_throw( Type ) );
    }

} // namespace sdbtools

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
