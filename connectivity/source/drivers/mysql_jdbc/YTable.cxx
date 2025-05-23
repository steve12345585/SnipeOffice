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

#include <mysql/YTable.hxx>
#include <mysql/YTables.hxx>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/sdbcx/Privilege.hpp>
#include <comphelper/property.hxx>
#include <comphelper/servicehelper.hxx>
#include <comphelper/types.hxx>
#include <connectivity/dbtools.hxx>
#include <connectivity/sdbcx/VColumn.hxx>
#include <connectivity/TKeys.hxx>
#include <connectivity/TIndexes.hxx>
#include <mysql/YColumns.hxx>
#include <TConnection.hxx>

using namespace ::comphelper;
using namespace connectivity::mysql;
using namespace connectivity::sdbcx;
using namespace connectivity;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::sdbcx;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::lang;
namespace connectivity::mysql
{
namespace
{
class OMySQLKeysHelper : public OKeysHelper
{
protected:
    virtual OUString getDropForeignKey() const override { return u" DROP FOREIGN KEY "_ustr; }

public:
    OMySQLKeysHelper(OTableHelper* _pTable, ::osl::Mutex& _rMutex,
                     const ::std::vector<OUString>& _rVector)
        : OKeysHelper(_pTable, _rMutex, _rVector)
    {
    }
};
}
}

OMySQLTable::OMySQLTable(sdbcx::OCollection* _pTables, const Reference<XConnection>& _xConnection)
    : OTableHelper(_pTables, _xConnection, true)
{
    // we create a new table here, so we should have all the rights or ;-)
    m_nPrivileges = Privilege::DROP | Privilege::REFERENCE | Privilege::ALTER | Privilege::CREATE
                    | Privilege::READ | Privilege::DELETE | Privilege::UPDATE | Privilege::INSERT
                    | Privilege::SELECT;
    construct();
}

OMySQLTable::OMySQLTable(sdbcx::OCollection* _pTables, const Reference<XConnection>& _xConnection,
                         const OUString& Name, const OUString& Type, const OUString& Description,
                         const OUString& SchemaName, const OUString& CatalogName,
                         sal_Int32 _nPrivileges)
    : OTableHelper(_pTables, _xConnection, true, Name, Type, Description, SchemaName, CatalogName)
    , m_nPrivileges(_nPrivileges)
{
    construct();
}

void OMySQLTable::construct()
{
    OTableHelper::construct();
    if (!isNew())
        registerProperty(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_PRIVILEGES),
                         PROPERTY_ID_PRIVILEGES, PropertyAttribute::READONLY, &m_nPrivileges,
                         cppu::UnoType<decltype(m_nPrivileges)>::get());
}

::cppu::IPropertyArrayHelper* OMySQLTable::createArrayHelper(sal_Int32 /*_nId*/) const
{
    return doCreateArrayHelper();
}

::cppu::IPropertyArrayHelper& OMySQLTable::getInfoHelper()
{
    return *static_cast<OMySQLTable_PROP*>(this)->getArrayHelper(isNew() ? 1 : 0);
}

sdbcx::OCollection* OMySQLTable::createColumns(const ::std::vector<OUString>& _rNames)
{
    OMySQLColumns* pColumns = new OMySQLColumns(*this, m_aMutex, _rNames);
    pColumns->setParent(this);
    return pColumns;
}

sdbcx::OCollection* OMySQLTable::createKeys(const ::std::vector<OUString>& _rNames)
{
    return new OMySQLKeysHelper(this, m_aMutex, _rNames);
}

sdbcx::OCollection* OMySQLTable::createIndexes(const ::std::vector<OUString>& _rNames)
{
    return new OIndexesHelper(this, m_aMutex, _rNames);
}

// XAlterTable
void SAL_CALL OMySQLTable::alterColumnByName(const OUString& colName,
                                             const Reference<XPropertySet>& descriptor)
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(
#ifdef __GNUC__
        ::connectivity::sdbcx::OTableDescriptor_BASE::rBHelper.bDisposed
#else
        rBHelper.bDisposed
#endif
    );

    if (!m_xColumns || !m_xColumns->hasByName(colName))
        throw NoSuchElementException(colName, *this);

    if (!isNew())
    {
        // first we have to check what should be altered
        Reference<XPropertySet> xProp;
        m_xColumns->getByName(colName) >>= xProp;
        // first check the types
        sal_Int32 nOldType = 0, nNewType = 0, nOldPrec = 0, nNewPrec = 0, nOldScale = 0,
                  nNewScale = 0;

        ::dbtools::OPropertyMap& rProp = OMetaConnection::getPropMap();
        xProp->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_TYPE)) >>= nOldType;
        descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_TYPE)) >>= nNewType;
        // and precisions and scale
        xProp->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_PRECISION)) >>= nOldPrec;
        descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_PRECISION)) >>= nNewPrec;
        xProp->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_SCALE)) >>= nOldScale;
        descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_SCALE)) >>= nNewScale;
        // second: check the "is nullable" value
        sal_Int32 nOldNullable = 0, nNewNullable = 0;
        xProp->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_ISNULLABLE)) >>= nOldNullable;
        descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_ISNULLABLE)) >>= nNewNullable;

        // check also the auto_increment
        bool bOldAutoIncrement = false, bAutoIncrement = false;
        xProp->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_ISAUTOINCREMENT))
            >>= bOldAutoIncrement;
        descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_ISAUTOINCREMENT))
            >>= bAutoIncrement;
        bool bColumnNameChanged = false;
        OUString sOldDesc, sNewDesc;
        xProp->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_DESCRIPTION)) >>= sOldDesc;
        descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_DESCRIPTION)) >>= sNewDesc;

        if (nOldType != nNewType || nOldPrec != nNewPrec || nOldScale != nNewScale
            || nNewNullable != nOldNullable || bOldAutoIncrement != bAutoIncrement
            || sOldDesc != sNewDesc)
        {
            // special handling because they changed the type names to distinguish
            // if a column should be an auto_increment one
            if (bOldAutoIncrement != bAutoIncrement)
            {
                OUString sTypeName;
                descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_TYPENAME))
                    >>= sTypeName;

                static const char s_sAutoIncrement[] = "auto_increment";
                if (bAutoIncrement)
                {
                    if (sTypeName.indexOf(s_sAutoIncrement) == -1)
                    {
                        sTypeName += OUString::Concat(" ") + s_sAutoIncrement;
                        descriptor->setPropertyValue(rProp.getNameByIndex(PROPERTY_ID_TYPENAME),
                                                     Any(sTypeName));
                    }
                }
                else
                {
                    if (!sTypeName.isEmpty())
                    {
                        sal_Int32 nIndex = sTypeName.indexOf(s_sAutoIncrement);
                        if (nIndex != -1)
                        {
                            sTypeName = sTypeName.copy(0, nIndex);
                            descriptor->setPropertyValue(rProp.getNameByIndex(PROPERTY_ID_TYPENAME),
                                                         Any(sTypeName));
                        }
                    }
                }
            }
            alterColumnType(nNewType, colName, descriptor);
            bColumnNameChanged = true;
        }

        // third: check the default values
        OUString sNewDefault, sOldDefault;
        xProp->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_DEFAULTVALUE)) >>= sOldDefault;
        descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_DEFAULTVALUE))
            >>= sNewDefault;

        if (!sOldDefault.isEmpty())
        {
            dropDefaultValue(colName);
            if (!sNewDefault.isEmpty() && sOldDefault != sNewDefault)
                alterDefaultValue(sNewDefault, colName);
        }
        else if (!sNewDefault.isEmpty())
            alterDefaultValue(sNewDefault, colName);

        // now we should look if the name of the column changed
        OUString sNewColumnName;
        descriptor->getPropertyValue(rProp.getNameByIndex(PROPERTY_ID_NAME)) >>= sNewColumnName;
        if (!sNewColumnName.equalsIgnoreAsciiCase(colName) && !bColumnNameChanged)
        {
            const OUString sQuote = getMetaData()->getIdentifierQuoteString();
            OUString sSql = getAlterTableColumnPart() + " CHANGE "
                            + ::dbtools::quoteName(sQuote, colName) + " "
                            + OTables::adjustSQL(::dbtools::createStandardColumnPart(
                                  descriptor, getConnection(), static_cast<OTables*>(m_pTables),
                                  getTypeCreatePattern()));
            executeStatement(sSql);
        }
        m_xColumns->refresh();
    }
    else
    {
        if (m_xColumns)
        {
            m_xColumns->dropByName(colName);
            m_xColumns->appendByDescriptor(descriptor);
        }
    }
}

void OMySQLTable::alterColumnType(sal_Int32 nNewType, const OUString& _rColName,
                                  const Reference<XPropertySet>& _xDescriptor)
{
    const OUString sQuote = getMetaData()->getIdentifierQuoteString();
    OUString sSql
        = getAlterTableColumnPart() + " CHANGE " + ::dbtools::quoteName(sQuote, _rColName) + " ";

    rtl::Reference<OColumn> pColumn = new OColumn(true);
    ::comphelper::copyProperties(_xDescriptor, pColumn);
    pColumn->setPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_TYPE),
                              Any(nNewType));

    sSql += OTables::adjustSQL(::dbtools::createStandardColumnPart(
        pColumn, getConnection(), static_cast<OTables*>(m_pTables), getTypeCreatePattern()));
    executeStatement(sSql);
}

OUString OMySQLTable::getTypeCreatePattern() const { return u"(M,D)"_ustr; }

void OMySQLTable::alterDefaultValue(std::u16string_view _sNewDefault, const OUString& _rColName)
{
    const OUString sQuote = getMetaData()->getIdentifierQuoteString();
    OUString sSql = getAlterTableColumnPart() + " ALTER " + ::dbtools::quoteName(sQuote, _rColName)
                    + " SET DEFAULT '" + _sNewDefault + "'";

    executeStatement(sSql);
}

void OMySQLTable::dropDefaultValue(const OUString& _rColName)
{
    const OUString sQuote = getMetaData()->getIdentifierQuoteString();
    OUString sSql = getAlterTableColumnPart() + " ALTER " + ::dbtools::quoteName(sQuote, _rColName)
                    + " DROP DEFAULT";

    executeStatement(sSql);
}

OUString OMySQLTable::getAlterTableColumnPart() const
{
    OUString sSql(u"ALTER TABLE "_ustr);

    OUString sComposedName(
        ::dbtools::composeTableName(getMetaData(), m_CatalogName, m_SchemaName, m_Name, true,
                                    ::dbtools::EComposeRule::InTableDefinitions));
    sSql += sComposedName;

    return sSql;
}

void OMySQLTable::executeStatement(const OUString& _rStatement)
{
    OUString sSQL = _rStatement;
    if (sSQL.endsWith(","))
        sSQL = sSQL.replaceAt(sSQL.getLength() - 1, 1, u")");

    Reference<XStatement> xStmt = getConnection()->createStatement();
    if (xStmt.is())
    {
        xStmt->execute(sSQL);
        ::comphelper::disposeComponent(xStmt);
    }
}

OUString OMySQLTable::getRenameStart() const { return u"RENAME TABLE "_ustr; }

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
