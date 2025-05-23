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

#include <com/sun/star/sdb/XParametersSupplier.hpp>
#include <com/sun/star/sdbcx/XTablesSupplier.hpp>
#include <com/sun/star/sdbcx/XColumnsSupplier.hpp>
#include <com/sun/star/sdb/XSingleSelectQueryComposer.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/script/XTypeConverter.hpp>
#include <cppuhelper/implbase5.hxx>
#include <connectivity/sqliterator.hxx>
#include <connectivity/sqlparse.hxx>
#include <apitools.hxx>
#include <comphelper/broadcasthelper.hxx>
#include <comphelper/uno3.hxx>
#include <comphelper/proparrhlp.hxx>
#include <comphelper/propertycontainer.hxx>
#include <svx/ParseContext.hxx>

namespace com::sun::star::util {
    class XNumberFormatsSupplier;
    class XNumberFormatter;
}

namespace dbaccess
{
    typedef ::cppu::ImplHelper5<    css::sdb::XSingleSelectQueryComposer,
                                    css::sdb::XParametersSupplier,
                                    css::sdbcx::XColumnsSupplier,
                                    css::sdbcx::XTablesSupplier,
                                    css::lang::XServiceInfo    > OSingleSelectQueryComposer_BASE;

    class OPrivateColumns;
    class OPrivateTables;

    class OSingleSelectQueryComposer :   public ::comphelper::OMutexAndBroadcastHelper
                                        ,public OSubComponent
                                        ,public ::comphelper::OPropertyContainer
                                        ,public ::comphelper::OPropertyArrayUsageHelper < OSingleSelectQueryComposer >
                                        ,public OSingleSelectQueryComposer_BASE
    {
        enum SQLPart
        {
            Where = 0,      // the 0 is important, as it will be used as index into arrays
            Group,
            Having,
            Order,

            SQLPartCount
        };
        static void incSQLPart( SQLPart& e ) { e = static_cast<SQLPart>(1 + static_cast<size_t>(e)); }
        enum EColumnType
        {
            SelectColumns       = 0,
            GroupByColumns      = 1,
            OrderColumns        = 2,
            ParameterColumns    = 3
        };
        typedef std::function<const ::connectivity::OSQLParseNode*(::connectivity::OSQLParseTreeIterator *)>
                                                TGetParseNode;
        ::svxform::OSystemParseContext          m_aParseContext;
        ::svxform::ONeutralParseContext         m_aNeutralContext;
        ::connectivity::OSQLParser              m_aSqlParser;
        ::connectivity::OSQLParseTreeIterator   m_aSqlIterator;         // the iterator for the complete statement
        ::connectivity::OSQLParseTreeIterator   m_aAdditiveIterator;    // the iterator for the "additive statement" (means without the clauses of the elementary statement)
        std::vector<std::unique_ptr<OPrivateColumns>>
                                                m_aColumnsCollection;   // used for columns and parameters of old queries
        std::vector<std::unique_ptr<OPrivateTables>>
                                                m_aTablesCollection;

        std::vector< OUString >        m_aElementaryParts;     // the filter/groupby/having/order of the elementary statement

        css::uno::Reference< css::sdbc::XConnection>              m_xConnection;
        css::uno::Reference< css::sdbc::XDatabaseMetaData>        m_xMetaData;
        css::uno::Reference< css::container::XNameAccess>         m_xConnectionTables;
        css::uno::Reference< css::container::XNameAccess>         m_xConnectionQueries;
        css::uno::Reference< css::util::XNumberFormatsSupplier >  m_xNumberFormatsSupplier;
        css::uno::Reference< css::uno::XComponentContext>         m_aContext;
        css::uno::Reference< css::script::XTypeConverter >        m_xTypeConverter;

        std::vector<std::unique_ptr<OPrivateColumns>>         m_aCurrentColumns;
        std::unique_ptr<OPrivateTables>                       m_pTables;      // currently used tables

        OUString                                m_aPureSelectSQL;   // the pure select statement, without filter/order/groupby/having
        OUString                                m_sDecimalSep;
        OUString                                m_sCommand;
        css::lang::Locale                       m_aLocale;
        sal_Int32                               m_nBoolCompareMode; // how to compare bool values
        sal_Int32                               m_nCommandType;

        // <properties>
        OUString                         m_sOriginal;
        // </properties>


        bool setORCriteria(::connectivity::OSQLParseNode const * pCondition, ::connectivity::OSQLParseTreeIterator& _rIterator,
            std::vector< std::vector < css::beans::PropertyValue > >& rFilters, const css::uno::Reference< css::util::XNumberFormatter > & xFormatter) const;
        bool setANDCriteria(::connectivity::OSQLParseNode const * pCondition, ::connectivity::OSQLParseTreeIterator& _rIterator,
            std::vector < css::beans::PropertyValue > & rFilters, const css::uno::Reference< css::util::XNumberFormatter > & xFormatter) const;
        bool setLikePredicate(::connectivity::OSQLParseNode const * pCondition, ::connectivity::OSQLParseTreeIterator const & _rIterator,
            std::vector < css::beans::PropertyValue > & rFilters, const css::uno::Reference< css::util::XNumberFormatter > & xFormatter) const;
        bool setComparisonPredicate(::connectivity::OSQLParseNode const * pCondition, ::connectivity::OSQLParseTreeIterator const & _rIterator,
            std::vector < css::beans::PropertyValue > & rFilters, const css::uno::Reference< css::util::XNumberFormatter > & xFormatter) const;

        static OUString getColumnName(::connectivity::OSQLParseNode const * pColumnRef, ::connectivity::OSQLParseTreeIterator const & _rIterator);
        OUString getTableAlias(const css::uno::Reference< css::beans::XPropertySet >& column ) const;
        static sal_Int32 getPredicateType(::connectivity::OSQLParseNode const * _pPredicate);
        // clears all Columns,Parameters and tables and insert it to their vectors
        void clearCurrentCollections();
        // clears the columns collection given by EColumnType
        void clearColumns( const EColumnType _eType );

        /** retrieves a particular part of a statement
            @param _rIterator
                the iterator to use.
        */
        OUString getStatementPart( TGetParseNode const & _aGetFunctor, ::connectivity::OSQLParseTreeIterator& _rIterator );
        void setQuery_Impl( const OUString& command );

        void setConditionByColumn( const css::uno::Reference< css::beans::XPropertySet >& column
                                , bool andCriteria
                                , std::function<bool(OSingleSelectQueryComposer *, const OUString&)> const & _aSetFunctor
                                ,sal_Int32 filterOperator);

        /** getStructuredCondition returns the structured condition for the where or having clause
            @param  _aGetFunctor
                A member function to get the correct parse node.

            @return
                The structured filter
        */
        css::uno::Sequence< css::uno::Sequence< css::beans::PropertyValue > >
                    getStructuredCondition( TGetParseNode const & _aGetFunctor );

        css::uno::Reference< css::container::XIndexAccess >
                    setCurrentColumns( EColumnType _eType, const ::rtl::Reference< ::connectivity::OSQLColumns >& _rCols );

        //helper methods for mem_fun_t
        bool implSetFilter(const OUString& _sFilter) { setFilter(_sFilter); return true;}
        bool implSetHavingClause(const OUString& _sFilter) { setHavingClause(_sFilter); return true;}

        /** returns the part of the select statement
            @param  _ePart
                Which part should be returned.
            @param  _bWithKeyword
                If <TRUE/> the keyword will be added too. Otherwise not.
            @param _rIterator
                The iterator to use.

            @return
                The part of the select statement.
        */
        OUString getSQLPart( SQLPart _ePart, ::connectivity::OSQLParseTreeIterator& _rIterator, bool _bWithKeyword );

        /** retrieves the keyword for the given SQLPart
        */
        static OUString getKeyword( SQLPart _ePart );

        /** sets a single "additive" clause, means a filter/groupby/having/order clause
        */
        void setSingleAdditiveClause( SQLPart _ePart, const OUString& _rClause );

        /** composes a statement from m_aPureSelectSQL and the 4 usual clauses
        */
        OUString composeStatementFromParts( const std::vector< OUString >& _rParts );

        /** return the name of the column in the *source* *table*.

            That is, for (SELECT a AS b FROM t), it returns A or "t"."A", as appropriate.

            Use e.g. for WHERE, GROUP BY and HAVING clauses.

            @param bGroupBy: for GROUP BY clause? In that case, throw exception if trying to use an unrelated column and the database does not support that.
        */
        OUString impl_getColumnRealName_throw(const css::uno::Reference< css::beans::XPropertySet >& column, bool bGroupBy);

        /** return the name of the column in the *query* for ORDER BY clause.

            That is, for (SELECT a AS b FROM t), it returns "b"

            Throws exception if trying to use an unrelated column and the database does not support that.
        */
        OUString impl_getColumnNameOrderBy_throw(const css::uno::Reference< css::beans::XPropertySet >& column);

    protected:
        virtual ~OSingleSelectQueryComposer() override;
    public:

        OSingleSelectQueryComposer( const css::uno::Reference< css::container::XNameAccess>& _xTableSupplier,
                        const css::uno::Reference< css::sdbc::XConnection>& _xConnection,
                        const css::uno::Reference< css::uno::XComponentContext>& _rContext);


        void SAL_CALL disposing() override;

        virtual css::uno::Sequence<css::uno::Type> SAL_CALL getTypes() override;
        virtual css::uno::Sequence<sal_Int8> SAL_CALL getImplementationId() override;

        // css::uno::XInterface
        DECLARE_XINTERFACE( )

        // XServiceInfo
        DECLARE_SERVICE_INFO();

        virtual css::uno::Reference< css::beans::XPropertySetInfo>  SAL_CALL getPropertySetInfo() override;
        virtual ::cppu::IPropertyArrayHelper& SAL_CALL getInfoHelper() override;
        virtual ::cppu::IPropertyArrayHelper* createArrayHelper() const override;


        // css::sdb::XSingleSelectQueryComposer
        virtual OUString SAL_CALL getElementaryQuery() override;
        virtual void SAL_CALL setElementaryQuery( const OUString& _rElementary ) override;
        virtual void SAL_CALL setFilter( const OUString& filter ) override;
        virtual void SAL_CALL setStructuredFilter( const css::uno::Sequence< css::uno::Sequence< css::beans::PropertyValue > >& filter ) override;
        virtual void SAL_CALL appendFilterByColumn( const css::uno::Reference< css::beans::XPropertySet >& column, sal_Bool andCriteria,sal_Int32 filterOperator ) override;
        virtual void SAL_CALL appendGroupByColumn( const css::uno::Reference< css::beans::XPropertySet >& column ) override;
        virtual void SAL_CALL setGroup( const OUString& group ) override;
        virtual void SAL_CALL setHavingClause( const OUString& filter ) override;
        virtual void SAL_CALL setStructuredHavingClause( const css::uno::Sequence< css::uno::Sequence< css::beans::PropertyValue > >& filter ) override;
        virtual void SAL_CALL appendHavingClauseByColumn( const css::uno::Reference< css::beans::XPropertySet >& column, sal_Bool andCriteria,sal_Int32 filterOperator ) override;
        virtual void SAL_CALL appendOrderByColumn( const css::uno::Reference< css::beans::XPropertySet >& column, sal_Bool ascending ) override;
        virtual void SAL_CALL setOrder( const OUString& order ) override;

        // XSingleSelectQueryAnalyzer
        virtual OUString SAL_CALL getQuery(  ) override;
        virtual void SAL_CALL setQuery( const OUString& command ) override;
        virtual void SAL_CALL setCommand( const OUString& command,sal_Int32 CommandType ) override;
        virtual OUString SAL_CALL getFilter(  ) override;
        virtual css::uno::Sequence< css::uno::Sequence< css::beans::PropertyValue > > SAL_CALL getStructuredFilter(  ) override;
        virtual OUString SAL_CALL getGroup(  ) override;
        virtual css::uno::Reference< css::container::XIndexAccess > SAL_CALL getGroupColumns(  ) override;
        virtual OUString SAL_CALL getHavingClause(  ) override;
        virtual css::uno::Sequence< css::uno::Sequence< css::beans::PropertyValue > > SAL_CALL getStructuredHavingClause(  ) override;
        virtual OUString SAL_CALL getOrder(  ) override;
        virtual css::uno::Reference< css::container::XIndexAccess > SAL_CALL getOrderColumns(  ) override;
        virtual OUString SAL_CALL getQueryWithSubstitution(  ) override;

        // XColumnsSupplier
        virtual css::uno::Reference< css::container::XNameAccess > SAL_CALL getColumns(  ) override;
        // XTablesSupplier
        virtual css::uno::Reference< css::container::XNameAccess > SAL_CALL getTables(  ) override;
        // XParametersSupplier
        virtual css::uno::Reference< css::container::XIndexAccess > SAL_CALL getParameters(  ) override;
    };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
