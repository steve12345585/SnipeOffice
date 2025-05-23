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

#include <com/sun/star/sdbc/FetchDirection.hpp>
#include <com/sun/star/sdbc/XResultSet.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/sdbc/XResultSetMetaDataSupplier.hpp>
#include <com/sun/star/sdbc/XCloseable.hpp>
#include <com/sun/star/sdbc/XColumnLocate.hpp>
#include <com/sun/star/util/XCancellable.hpp>
#include <com/sun/star/sdbc/XWarningsSupplier.hpp>
#include <com/sun/star/sdbc/XResultSetUpdate.hpp>
#include <com/sun/star/sdbc/XRowUpdate.hpp>
#include <com/sun/star/sdbcx/XRowLocate.hpp>
#include <com/sun/star/sdbcx/XDeleteRows.hpp>
#include <cppuhelper/compbase.hxx>
#include <comphelper/proparrhlp.hxx>
#include <odbc/OFunctions.hxx>
#include <odbc/OStatement.hxx>
#include <odbc/odbcbasedllapi.hxx>
#include <connectivity/CommonTools.hxx>
#include <connectivity/FValue.hxx>
#include <TSkipDeletedSet.hxx>
#include <memory>
#include "OResultSetMetaData.hxx"

namespace connectivity::odbc
{
    class OResultSetMetaData;

    /*
    **  java_sql_ResultSet
    */
    typedef ::cppu::WeakComponentImplHelper<      css::sdbc::XResultSet,
                                                  css::sdbc::XRow,
                                                  css::sdbc::XResultSetMetaDataSupplier,
                                                  css::util::XCancellable,
                                                  css::sdbc::XWarningsSupplier,
                                                  css::sdbc::XResultSetUpdate,
                                                  css::sdbc::XRowUpdate,
                                                  css::sdbcx::XRowLocate,
                                                  css::sdbcx::XDeleteRows,
                                                  css::sdbc::XCloseable,
                                                  css::sdbc::XColumnLocate,
                                                  css::lang::XServiceInfo> OResultSet_BASE;

    class BindData;

    /// Functor object for class ZZ returntype is void
    struct OOO_DLLPUBLIC_ODBCBASE TBookmarkPosMapCompare
    {
        bool operator()( const css::uno::Sequence<sal_Int8>& _rLH,
                                const css::uno::Sequence<sal_Int8>& _rRH) const
        {
            if(_rLH.getLength() == _rRH.getLength())
            {
                sal_Int32 nCount = _rLH.getLength();
                if(nCount != 4)
                {
                    const sal_Int8* pLHBack = _rLH.getConstArray() + nCount - 1;
                    const sal_Int8* pRHBack = _rRH.getConstArray() + nCount - 1;

                    sal_Int32 i;
                    for(i=0;i < nCount;++i,--pLHBack,--pRHBack)
                    {
                        if(!(*pLHBack) && *pRHBack)
                            return true;
                        else if(*pLHBack && !(*pRHBack))
                            return false;
                    }
                    for(i=0,++pLHBack,++pRHBack;i < nCount;++pLHBack,++pRHBack,++i)
                        if(*pLHBack < *pRHBack)
                            return true;
                    return false;
                }
                else
                    return *reinterpret_cast<const sal_Int32*>(_rLH.getConstArray()) < *reinterpret_cast<const sal_Int32*>(_rRH.getConstArray());

            }
            else
                return _rLH.getLength() < _rRH.getLength();
        }
    };

    typedef std::map< css::uno::Sequence<sal_Int8>, sal_Int32,TBookmarkPosMapCompare > TBookmarkPosMap;

    class OResultSet :
                    public  cppu::BaseMutex,
                    public  ::connectivity::IResultSetHelper,
                    public  OResultSet_BASE,
                    public  ::cppu::OPropertySetHelper,
                    public  ::comphelper::OPropertyArrayUsageHelper<OResultSet>
    {
    protected:
        TBookmarkPosMap                             m_aPosToBookmarks;
        // used top hold the information about the value and the datatype to save calls to metadata
        typedef std::vector<ORowSetValue>         TDataRow;

        std::vector<std::unique_ptr<BindData>>    m_aBindVector;
        std::vector<SQLLEN>                       m_aLengthVector;
        std::map<sal_Int32,SWORD>                 m_aODBCColumnTypes;

        // In baseline ODBC, SQLGetData can only be called on monotonically increasing column numbers.
        // additionally, any variable-length data can be fetched only once (possibly in parts);
        // after that, SQLGetData returns SQL_NO_DATA.
        // In order to insulate our callers from these restrictions,
        // we cache the current row in m_aRow.
        // If the driver claims to support the GD_ANY_ORDER extension,
        // we read and cache only the columns requested by a caller.
        // Else, we read and cache all columns whose number is <= a requested column.
        // m_aRow[colNumber].getBound() says if it contains an up-to-date value or not.
        TDataRow                                    m_aRow;
        bool                                        m_bFetchDataInOrder;
        SQLHANDLE                                   m_aStatementHandle;
        SQLHANDLE                                   m_aConnectionHandle;
        OStatement_Base*                            m_pStatement;
        std::unique_ptr<OSkipDeletedSet>            m_pSkipDeletedSet;
        css::uno::Reference< css::uno::XInterface>    m_xStatement;
        rtl::Reference< OResultSetMetaData>        m_xMetaData;
        std::unique_ptr<SQLUSMALLINT[]>             m_pRowStatusArray;
        rtl_TextEncoding                            m_nTextEncoding;
        sal_Int32                                   m_nRowPos;
        mutable sal_uInt32                          m_nUseBookmarks;
        SQLRETURN                                   m_nCurrentFetchState;
        bool                                    m_bWasNull;
        bool                                    m_bEOF;                 // after last record
        bool                                    m_bRowInserted;
        bool                                    m_bRowDeleted;
        bool                                    m_bUseFetchScroll;

        bool      isBookmarkable()          const;
        sal_Int32 getResultSetConcurrency() const;
        sal_Int32 getResultSetType()        const;
        static sal_Int32 getFetchDirection() { return css::sdbc::FetchDirection::FORWARD; }
        sal_Int32 getFetchSize()            const;
        OUString getCursorName()     const;
        template < typename T, SQLINTEGER BufferLength > T getStmtOption (SQLINTEGER fOption) const;

        void setFetchDirection(sal_Int32 _par0);
        void setFetchSize(sal_Int32 _par0);
        template < typename T, SQLINTEGER BufferLength > SQLRETURN setStmtOption (SQLINTEGER fOption, T value) const;


        void ensureCacheForColumn(sal_Int32 columnIndex);
        void invalidateCache();
        void fillColumn(sal_Int32 _nToColumn);
        void allocBuffer();
        void releaseBuffer();
        /// @throws css::sdbc::SQLException
        /// @throws css::uno::RuntimeException
        void updateValue(sal_Int32 columnIndex, SQLSMALLINT _nType, void const * _pValue);
        void fillNeededData(SQLRETURN _nRet);
        bool moveImpl(IResultSetHelper::Movement _eCursorPosition, sal_Int32 _nOffset);
        SQLRETURN unbind(bool _bUnbindHandle = true);
        SWORD impl_getColumnType_nothrow(sal_Int32 columnIndex);

        // helper to implement XRow::getXXX in simple cases
        template < typename T > T getValue( sal_Int32 columnIndex );
        // impl_getXXX are the functions that do the actual fetching from ODBC, ignoring the cache
        // for simple cases
        template < typename T > T impl_getValue( const sal_Int32 _nColumnIndex, SQLSMALLINT nType );
        // these cases need some special treatment
        /// @throws css::sdbc::SQLException
        /// @throws css::uno::RuntimeException
        bool impl_getBoolean( sal_Int32 columnIndex );
        /// @throws css::sdbc::SQLException
        /// @throws css::uno::RuntimeException
        css::uno::Sequence< sal_Int8 > impl_getBytes( sal_Int32 columnIndex );
        /// @throws css::sdbc::SQLException
        /// @throws css::uno::RuntimeException
        css::util::Date impl_getDate( sal_Int32 columnIndex );
        /// @throws css::sdbc::SQLException
        /// @throws css::uno::RuntimeException
        css::util::Time impl_getTime( sal_Int32 columnIndex );
        /// @throws css::sdbc::SQLException
        /// @throws css::uno::RuntimeException
        css::util::DateTime impl_getTimestamp( sal_Int32 columnIndex );
        /// @throws css::sdbc::SQLException
        /// @throws css::uno::RuntimeException
        sal_Int64 impl_getLong( sal_Int32 columnIndex );
        /// @throws css::sdbc::SQLException
        /// @throws css::uno::RuntimeException
        OUString impl_getString( sal_Int32 columnIndex );
        /// @throws css::sdbc::SQLException
        /// @throws css::uno::RuntimeException
        css::uno::Sequence<sal_Int8> impl_getBookmark(  );


        // OPropertyArrayUsageHelper
        virtual ::cppu::IPropertyArrayHelper* createArrayHelper( ) const override;
        // OPropertySetHelper
        virtual ::cppu::IPropertyArrayHelper & SAL_CALL getInfoHelper() override;

        virtual sal_Bool SAL_CALL convertFastPropertyValue(
                            css::uno::Any & rConvertedValue,
                            css::uno::Any & rOldValue,
                            sal_Int32 nHandle,
                            const css::uno::Any& rValue ) override;
        virtual void SAL_CALL setFastPropertyValue_NoBroadcast(
                                sal_Int32 nHandle,
                                const css::uno::Any& rValue
                                 ) override;
        virtual void SAL_CALL getFastPropertyValue(
                                css::uno::Any& rValue,
                                sal_Int32 nHandle
                                     ) const override;
    public:
        DECLARE_SERVICE_INFO();
        // A ctor that is needed for returning the object
        OResultSet( SQLHANDLE _pStatementHandle,OStatement_Base* pStmt);
        virtual ~OResultSet() override;

        void construct();

        const Functions& functions() const { return m_pStatement->functions(); }

        css::uno::Reference< css::uno::XInterface > operator *()
        {
            return css::uno::Reference< css::uno::XInterface >(*static_cast<OResultSet_BASE*>(this));
        }

        void setMetaData(const rtl::Reference<OResultSetMetaData>& _xMetaData) { m_xMetaData = _xMetaData;}

        // ::cppu::OComponentHelper
        virtual void SAL_CALL disposing() override;
        // XInterface
        virtual css::uno::Any SAL_CALL queryInterface( const css::uno::Type & rType ) override;
        virtual void SAL_CALL acquire() noexcept override;
        virtual void SAL_CALL release() noexcept override;
        //XTypeProvider
        virtual css::uno::Sequence< css::uno::Type > SAL_CALL getTypes(  ) override;
        // XPropertySet
        virtual css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL getPropertySetInfo(  ) override;
        // XResultSet
        virtual sal_Bool SAL_CALL next(  ) override;
        virtual sal_Bool SAL_CALL isBeforeFirst(  ) override;
        virtual sal_Bool SAL_CALL isAfterLast(  ) override;
        virtual sal_Bool SAL_CALL isFirst(  ) override;
        virtual sal_Bool SAL_CALL isLast(  ) override;
        virtual void SAL_CALL beforeFirst(  ) override;
        virtual void SAL_CALL afterLast(  ) override;
        virtual sal_Bool SAL_CALL first(  ) override;
        virtual sal_Bool SAL_CALL last(  ) override;
        virtual sal_Int32 SAL_CALL getRow(  ) override;
        virtual sal_Bool SAL_CALL absolute( sal_Int32 row ) override;
        virtual sal_Bool SAL_CALL relative( sal_Int32 rows ) override;
        virtual sal_Bool SAL_CALL previous(  ) override;
        virtual void SAL_CALL refreshRow(  ) override;
        virtual sal_Bool SAL_CALL rowUpdated(  ) override;
        virtual sal_Bool SAL_CALL rowInserted(  ) override;
        virtual sal_Bool SAL_CALL rowDeleted(  ) override;
        virtual css::uno::Reference< css::uno::XInterface > SAL_CALL getStatement(  ) override;
        // XRow
        virtual sal_Bool SAL_CALL wasNull(  ) override;
        virtual OUString SAL_CALL getString( sal_Int32 columnIndex ) override;
        virtual sal_Bool SAL_CALL getBoolean( sal_Int32 columnIndex ) override;
        virtual sal_Int8 SAL_CALL getByte( sal_Int32 columnIndex ) override;
        virtual sal_Int16 SAL_CALL getShort( sal_Int32 columnIndex ) override;
        virtual sal_Int32 SAL_CALL getInt( sal_Int32 columnIndex ) override;
        virtual sal_Int64 SAL_CALL getLong( sal_Int32 columnIndex ) override;
        virtual float SAL_CALL getFloat( sal_Int32 columnIndex ) override;
        virtual double SAL_CALL getDouble( sal_Int32 columnIndex ) override;
        virtual css::uno::Sequence< sal_Int8 > SAL_CALL getBytes( sal_Int32 columnIndex ) override;
        virtual css::util::Date SAL_CALL getDate( sal_Int32 columnIndex ) override;
        virtual css::util::Time SAL_CALL getTime( sal_Int32 columnIndex ) override;
        virtual css::util::DateTime SAL_CALL getTimestamp( sal_Int32 columnIndex ) override;
        virtual css::uno::Reference< css::io::XInputStream > SAL_CALL getBinaryStream( sal_Int32 columnIndex ) override;
        virtual css::uno::Reference< css::io::XInputStream > SAL_CALL getCharacterStream( sal_Int32 columnIndex ) override;
        virtual css::uno::Any SAL_CALL getObject( sal_Int32 columnIndex, const css::uno::Reference< css::container::XNameAccess >& typeMap ) override;
        virtual css::uno::Reference< css::sdbc::XRef > SAL_CALL getRef( sal_Int32 columnIndex ) override;
        virtual css::uno::Reference< css::sdbc::XBlob > SAL_CALL getBlob( sal_Int32 columnIndex ) override;
        virtual css::uno::Reference< css::sdbc::XClob > SAL_CALL getClob( sal_Int32 columnIndex ) override;
        virtual css::uno::Reference< css::sdbc::XArray > SAL_CALL getArray( sal_Int32 columnIndex ) override;
        // XResultSetMetaDataSupplier
        virtual css::uno::Reference< css::sdbc::XResultSetMetaData > SAL_CALL getMetaData(  ) override;
        // XCancellable
        virtual void SAL_CALL cancel(  ) override;
        // XCloseable
        virtual void SAL_CALL close(  ) override;
        // XWarningsSupplier
        virtual css::uno::Any SAL_CALL getWarnings(  ) override;
        virtual void SAL_CALL clearWarnings(  ) override;
        // XResultSetUpdate
        virtual void SAL_CALL insertRow(  ) override;
        virtual void SAL_CALL updateRow(  ) override;
        virtual void SAL_CALL deleteRow(  ) override;
        virtual void SAL_CALL cancelRowUpdates(  ) override;
        virtual void SAL_CALL moveToInsertRow(  ) override;
        virtual void SAL_CALL moveToCurrentRow(  ) override;
        // XRowUpdate
        virtual void SAL_CALL updateNull( sal_Int32 columnIndex ) override;
        virtual void SAL_CALL updateBoolean( sal_Int32 columnIndex, sal_Bool x ) override;
        virtual void SAL_CALL updateByte( sal_Int32 columnIndex, sal_Int8 x ) override;
        virtual void SAL_CALL updateShort( sal_Int32 columnIndex, sal_Int16 x ) override;
        virtual void SAL_CALL updateInt( sal_Int32 columnIndex, sal_Int32 x ) override;
        virtual void SAL_CALL updateLong( sal_Int32 columnIndex, sal_Int64 x ) override;
        virtual void SAL_CALL updateFloat( sal_Int32 columnIndex, float x ) override;
        virtual void SAL_CALL updateDouble( sal_Int32 columnIndex, double x ) override;
        virtual void SAL_CALL updateString( sal_Int32 columnIndex, const OUString& x ) override;
        virtual void SAL_CALL updateBytes( sal_Int32 columnIndex, const css::uno::Sequence< sal_Int8 >& x ) override;
        virtual void SAL_CALL updateDate( sal_Int32 columnIndex, const css::util::Date& x ) override;
        virtual void SAL_CALL updateTime( sal_Int32 columnIndex, const css::util::Time& x ) override;
        virtual void SAL_CALL updateTimestamp( sal_Int32 columnIndex, const css::util::DateTime& x ) override;
        virtual void SAL_CALL updateBinaryStream( sal_Int32 columnIndex, const css::uno::Reference< css::io::XInputStream >& x, sal_Int32 length ) override;
        virtual void SAL_CALL updateCharacterStream( sal_Int32 columnIndex, const css::uno::Reference< css::io::XInputStream >& x, sal_Int32 length ) override;
        virtual void SAL_CALL updateObject( sal_Int32 columnIndex, const css::uno::Any& x ) override;
        virtual void SAL_CALL updateNumericObject( sal_Int32 columnIndex, const css::uno::Any& x, sal_Int32 scale ) override;
        // XColumnLocate
        virtual sal_Int32 SAL_CALL findColumn( const OUString& columnName ) override;
        // XRowLocate
        virtual css::uno::Any SAL_CALL getBookmark(  ) override;
        virtual sal_Bool SAL_CALL moveToBookmark( const css::uno::Any& bookmark ) override;
        virtual sal_Bool SAL_CALL moveRelativeToBookmark( const css::uno::Any& bookmark, sal_Int32 rows ) override;
        virtual sal_Int32 SAL_CALL compareBookmarks( const css::uno::Any& first, const css::uno::Any& second ) override;
        virtual sal_Bool SAL_CALL hasOrderedBookmarks(  ) override;
        virtual sal_Int32 SAL_CALL hashBookmark( const css::uno::Any& bookmark ) override;
        // XDeleteRows
        virtual css::uno::Sequence< sal_Int32 > SAL_CALL deleteRows( const css::uno::Sequence< css::uno::Any >& rows ) override;

        // IResultSetHelper
        virtual bool move(IResultSetHelper::Movement _eCursorPosition, sal_Int32 _nOffset, bool _bRetrieveData) override;
        virtual sal_Int32 getDriverPos() const override;
        virtual bool isRowDeleted() const override;

    protected:
        using OPropertySetHelper::getFastPropertyValue;
    };

}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
