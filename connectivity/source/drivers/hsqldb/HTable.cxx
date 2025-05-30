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

#include <hsqldb/HTable.hxx>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/sdbcx/Privilege.hpp>
#include <comphelper/property.hxx>
#include <comphelper/servicehelper.hxx>
#include <comphelper/types.hxx>
#include <connectivity/dbtools.hxx>
#include <connectivity/TKeys.hxx>
#include <connectivity/TIndexes.hxx>
#include <hsqldb/HColumns.hxx>
#include <TConnection.hxx>

#include <comphelper/diagnose_ex.hxx>


using namespace ::comphelper;
using namespace connectivity::hsqldb;
using namespace connectivity::sdbcx;
using namespace connectivity;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::sdbcx;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::lang;

OHSQLTable::OHSQLTable( sdbcx::OCollection* _pTables,
                           const Reference< XConnection >& _xConnection)
    :OTableHelper(_pTables,_xConnection,true)
{
    // we create a new table here, so we should have all the rights or ;-)
    m_nPrivileges = Privilege::DROP         |
                    Privilege::REFERENCE    |
                    Privilege::ALTER        |
                    Privilege::CREATE       |
                    Privilege::READ         |
                    Privilege::DELETE       |
                    Privilege::UPDATE       |
                    Privilege::INSERT       |
                    Privilege::SELECT;
    construct();
}

OHSQLTable::OHSQLTable( sdbcx::OCollection* _pTables,
                           const Reference< XConnection >& _xConnection,
                    const OUString& Name,
                    const OUString& Type,
                    const OUString& Description ,
                    const OUString& SchemaName,
                    const OUString& CatalogName,
                    sal_Int32 _nPrivileges
                ) : OTableHelper(   _pTables,
                                    _xConnection,
                                    true,
                                    Name,
                                    Type,
                                    Description,
                                    SchemaName,
                                    CatalogName)
 , m_nPrivileges(_nPrivileges)
{
    construct();
}

void OHSQLTable::construct()
{
    OTableHelper::construct();
    if ( !isNew() )
        registerProperty(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_PRIVILEGES),  PROPERTY_ID_PRIVILEGES,PropertyAttribute::READONLY,&m_nPrivileges,  cppu::UnoType<decltype(m_nPrivileges)>::get());
}

::cppu::IPropertyArrayHelper* OHSQLTable::createArrayHelper( sal_Int32 /*_nId*/ ) const
{
    return doCreateArrayHelper();
}

::cppu::IPropertyArrayHelper & OHSQLTable::getInfoHelper()
{
    return *static_cast<OHSQLTable_PROP*>(this)->getArrayHelper(isNew() ? 1 : 0);
}

sdbcx::OCollection* OHSQLTable::createColumns(const ::std::vector< OUString>& _rNames)
{
    OHSQLColumns* pColumns = new OHSQLColumns(*this,m_aMutex,_rNames);
    pColumns->setParent(this);
    return pColumns;
}

sdbcx::OCollection* OHSQLTable::createKeys(const ::std::vector< OUString>& _rNames)
{
    return new OKeysHelper(this,m_aMutex,_rNames);
}

sdbcx::OCollection* OHSQLTable::createIndexes(const ::std::vector< OUString>& _rNames)
{
    return new OIndexesHelper(this,m_aMutex,_rNames);
}


// XAlterTable
void SAL_CALL OHSQLTable::alterColumnByName( const OUString& colName, const Reference< XPropertySet >& descriptor )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(
#ifdef __GNUC__
        ::connectivity::sdbcx::OTableDescriptor_BASE::rBHelper.bDisposed
#else
        rBHelper.bDisposed
#endif
        );

    if ( !m_xColumns || !m_xColumns->hasByName(colName) )
        throw NoSuchElementException(colName,*this);


    if ( !isNew() )
    {
        // first we have to check what should be altered
        Reference<XPropertySet> xProp;
        m_xColumns->getByName(colName) >>= xProp;
        // first check the types
        sal_Int32 nOldType = 0,nNewType = 0,nOldPrec = 0,nNewPrec = 0,nOldScale = 0,nNewScale = 0;
        OUString sOldTypeName, sNewTypeName;

        ::dbtools::OPropertyMap& rProp = OMetaConnection::getPropMap();

        // type/typename
        xProp->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_TYPE))         >>= nOldType;
        descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_TYPE))    >>= nNewType;
        xProp->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_TYPENAME))     >>= sOldTypeName;
        descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_TYPENAME))>>= sNewTypeName;

        // and precision and scale
        xProp->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_PRECISION))    >>= nOldPrec;
        descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_PRECISION))>>= nNewPrec;
        xProp->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_SCALE))        >>= nOldScale;
        descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_SCALE))   >>= nNewScale;

        // second: check the "is nullable" value
        sal_Int32 nOldNullable = 0,nNewNullable = 0;
        xProp->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_ISNULLABLE))       >>= nOldNullable;
        descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_ISNULLABLE))  >>= nNewNullable;

        // check also the auto_increment
        bool bOldAutoIncrement = false,bAutoIncrement = false;
        xProp->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_ISAUTOINCREMENT))      >>= bOldAutoIncrement;
        descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_ISAUTOINCREMENT)) >>= bAutoIncrement;

        // now we should look if the name of the column changed
        OUString sNewColumnName;
        descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_NAME)) >>= sNewColumnName;
        if ( sNewColumnName != colName )
        {
            const OUString sQuote = getMetaData()->getIdentifierQuoteString(  );

            OUString sSql = getAlterTableColumnPart() +
                " ALTER COLUMN " +
                ::dbtools::quoteName(sQuote,colName) +
                " RENAME TO " +
                ::dbtools::quoteName(sQuote,sNewColumnName);

            executeStatement(sSql);
        }

        if  (   nOldType != nNewType
            ||  sOldTypeName != sNewTypeName
            ||  nOldPrec != nNewPrec
            ||  nOldScale != nNewScale
            ||  nNewNullable != nOldNullable
            ||  bOldAutoIncrement != bAutoIncrement )
        {
            // special handling because they change the type names to distinguish
            // if a column should be an auto_increment one
            if ( bOldAutoIncrement != bAutoIncrement )
            {
                /// TODO: insert special handling for auto increment "IDENTITY" and primary key
            }
            alterColumnType(nNewType,sNewColumnName,descriptor);
        }

        // third: check the default values
        OUString sNewDefault,sOldDefault;
        xProp->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_DEFAULTVALUE))     >>= sOldDefault;
        descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_DEFAULTVALUE)) >>= sNewDefault;

        if(!sOldDefault.isEmpty())
        {
            dropDefaultValue(colName);
            if(!sNewDefault.isEmpty() && sOldDefault != sNewDefault)
                alterDefaultValue(sNewDefault,sNewColumnName);
        }
        else if(sOldDefault.isEmpty() && !sNewDefault.isEmpty())
            alterDefaultValue(sNewDefault,sNewColumnName);

        m_xColumns->refresh();
    }
    else
    {
        if(m_xColumns)
        {
            m_xColumns->dropByName(colName);
            m_xColumns->appendByDescriptor(descriptor);
        }
    }

}

void OHSQLTable::alterColumnType(sal_Int32 nNewType,const OUString& _rColName, const Reference<XPropertySet>& _xDescriptor)
{
    OUString sSql = getAlterTableColumnPart() + " ALTER COLUMN ";
#if OSL_DEBUG_LEVEL > 0
    try
    {
        OUString sDescriptorName;
        OSL_ENSURE( _xDescriptor.is()
                &&  ( _xDescriptor->getPropertyValue( OMetaConnection::getPropMap().getNameByIndex( PROPERTY_ID_NAME ) ) >>= sDescriptorName )
                &&  ( sDescriptorName == _rColName ),
                "OHSQLTable::alterColumnType: unexpected column name!" );
    }
    catch( const Exception& )
    {
        DBG_UNHANDLED_EXCEPTION("connectivity.hsqldb");
    }
#else
    (void)_rColName;
#endif

    rtl::Reference<OHSQLColumn> pColumn = new OHSQLColumn;
    ::comphelper::copyProperties(_xDescriptor,pColumn);
    pColumn->setPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_TYPE),Any(nNewType));

    sSql += ::dbtools::createStandardColumnPart(pColumn,getConnection());
    executeStatement(sSql);
}

void OHSQLTable::alterDefaultValue(std::u16string_view _sNewDefault,const OUString& _rColName)
{
    const OUString sQuote = getMetaData()->getIdentifierQuoteString(  );
    OUString sSql = getAlterTableColumnPart() +
        " ALTER COLUMN " +
        ::dbtools::quoteName(sQuote,_rColName) +
        " SET DEFAULT '" + _sNewDefault + "'";

    executeStatement(sSql);
}

void OHSQLTable::dropDefaultValue(const OUString& _rColName)
{
    const OUString sQuote = getMetaData()->getIdentifierQuoteString(  );
    OUString sSql = getAlterTableColumnPart() +
        " ALTER COLUMN " +
        ::dbtools::quoteName(sQuote,_rColName) +
        " DROP DEFAULT";

    executeStatement(sSql);
}

OUString OHSQLTable::getAlterTableColumnPart() const
{
    OUString sSql(  u"ALTER TABLE "_ustr );

    OUString sComposedName( ::dbtools::composeTableName( getMetaData(), m_CatalogName, m_SchemaName, m_Name, true, ::dbtools::EComposeRule::InTableDefinitions ) );
    sSql += sComposedName;

    return sSql;
}

void OHSQLTable::executeStatement(const OUString& _rStatement )
{
    OUString sSQL = _rStatement;
    if(sSQL.endsWith(","))
        sSQL = sSQL.replaceAt(sSQL.getLength()-1, 1, u")");

    Reference< XStatement > xStmt = getConnection()->createStatement(  );
    if ( xStmt.is() )
    {
        try { xStmt->execute(sSQL); }
        catch( const Exception& )
        {
            ::comphelper::disposeComponent(xStmt);
            throw;
        }
        ::comphelper::disposeComponent(xStmt);
    }
}

Sequence< Type > SAL_CALL OHSQLTable::getTypes(  )
{
    if ( m_Type == "VIEW" )
    {
        Sequence< Type > aTypes = OTableHelper::getTypes();
        std::vector<Type> aOwnTypes;
        aOwnTypes.reserve(aTypes.getLength());
        const Type* pIter = aTypes.getConstArray();
        const Type* pEnd = pIter + aTypes.getLength();
        for(;pIter != pEnd;++pIter)
        {
            if( *pIter != cppu::UnoType<XRename>::get())
            {
                aOwnTypes.push_back(*pIter);
            }
        }
        return Sequence< Type >(aOwnTypes.data(), aOwnTypes.size());
    }
    return OTableHelper::getTypes();
}

// XRename
void SAL_CALL OHSQLTable::rename( const OUString& newName )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(
#ifdef __GNUC__
        ::connectivity::sdbcx::OTableDescriptor_BASE::rBHelper.bDisposed
#else
        rBHelper.bDisposed
#endif
        );

    if(!isNew())
    {
        OUString sSql = u"ALTER "_ustr;
        if ( m_Type == "VIEW" )
            sSql += " VIEW ";
        else
            sSql += " TABLE ";

        OUString sCatalog,sSchema,sTable;
        ::dbtools::qualifiedNameComponents(getMetaData(),newName,sCatalog,sSchema,sTable,::dbtools::EComposeRule::InDataManipulation);

        sSql +=
            ::dbtools::composeTableName( getMetaData(), m_CatalogName, m_SchemaName, m_Name, true, ::dbtools::EComposeRule::InDataManipulation )
            + " RENAME TO "
            + ::dbtools::composeTableName( getMetaData(), sCatalog, sSchema, sTable, true, ::dbtools::EComposeRule::InDataManipulation );

        executeStatement(sSql);

        ::connectivity::OTable_TYPEDEF::rename(newName);
    }
    else
        ::dbtools::qualifiedNameComponents(getMetaData(),newName,m_CatalogName,m_SchemaName,m_Name,::dbtools::EComposeRule::InTableDefinitions);
}


Any SAL_CALL OHSQLTable::queryInterface( const Type & rType )
{
    if( m_Type == "VIEW" && rType == cppu::UnoType<XRename>::get())
        return Any();

    return OTableHelper::queryInterface(rType);
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
