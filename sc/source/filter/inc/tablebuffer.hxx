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

#include "autofilterbuffer.hxx"
#include "tablecolumnsbuffer.hxx"
#include "workbookhelper.hxx"

namespace oox::xls {

struct TableModel
{
    ScRange             maRange;            /// Original (unchecked) range of the table.
    OUString            maProgName;         /// Programmatical name.
    OUString            maDisplayName;      /// Display name.
    sal_Int32           mnId;               /// Unique table identifier.
    sal_Int32           mnType;             /// Table type (worksheet, query, etc.).
    sal_Int32           mnHeaderRows;       /// Number of header rows.
    sal_Int32           mnTotalsRows;       /// Number of totals rows.

    explicit            TableModel();
};

class Table : public WorkbookHelper
{
public:
    explicit            Table( const WorkbookHelper& rHelper );

    /** Imports a table definition from the passed attributes. */
    void                importTable( const AttributeList& rAttribs, sal_Int16 nSheet );
    /** Imports a table definition from a TABLE record. */
    void                importTable( SequenceInputStream& rStrm, sal_Int16 nSheet );
    /** Creates a new auto filter and stores it internally. */
    AutoFilter&  createAutoFilter() { return maAutoFilters.createAutoFilter(); }
    /** Creates a new tableColumns handler and stores it internally. */
    TableColumns&  createTableColumns() { return maTableColumns.createTableColumns(); }

    /** Creates a database range from this tables. */
    void                finalizeImport();
    void                applyAutoFilters();
    void                applyTableColumns();

    /** Returns the unique table identifier. */
    sal_Int32    getTableId() const { return maModel.mnId; }
    /** Returns the token index used in API token arrays (com.sun.star.sheet.FormulaToken). */
    sal_Int32    getTokenIndex() const { return mnTokenIndex; }
    /** Returns the original display name of the table. */
    const OUString& getDisplayName() const { return maModel.maDisplayName; }

    /** Returns the original (unchecked) total range of the table. */
    const ScRange& getOriginalRange() const { return maModel.maRange; }
    /** Returns the cell range of this table. */
    const ScRange& getRange() const { return maDestRange; }
    /** Returns the number of columns of this table. */
    SCCOL        getWidth() const { return maDestRange.aEnd.Col() - maDestRange.aStart.Col() + 1; }
    /** Returns the number of rows of this table. */
    SCROW        getHeight() const { return maDestRange.aEnd.Row() - maDestRange.aStart.Row() + 1; }
    /** Returns the number of header rows in the table range. */
    sal_Int32    getHeaderRows() const { return maModel.mnHeaderRows; }
    /** Returns the number of totals rows in the table range. */
    sal_Int32    getTotalsRows() const { return maModel.mnTotalsRows; }

private:
    TableModel          maModel;
    AutoFilterBuffer    maAutoFilters;      /// Filter settings for this table.
    TableColumnsBuffer  maTableColumns;     /// Column names of this table.
    OUString            maDBRangeName;      /// Name of the database range in the Calc document.
    ScRange             maDestRange;        /// Validated range of the table in the worksheet.
    sal_Int32           mnTokenIndex;       /// Token index used in API token array.
};

typedef std::shared_ptr< Table > TableRef;

class TableBuffer : public WorkbookHelper
{
public:
    explicit            TableBuffer( const WorkbookHelper& rHelper );

    /** Creates a new empty table. */
    Table&              createTable();

    /** Creates database ranges from all imported tables. */
    void                finalizeImport();
    /** Applies autofilters from created database range ( requires finalizeImport to have run before being called */
    void                applyAutoFilters();
    /** Applies columns names from created database range ( requires finalizeImport to have run before being called */
    void                applyTableColumns();
    /** Returns a table by its identifier. */
    TableRef            getTable( sal_Int32 nTableId ) const;
    /** Returns a table by its display name. */
    TableRef            getTable( const OUString& rDispName ) const;

private:
    /** Inserts the passed table into the maps according to its identifier and name. */
    void                insertTableToMaps( const TableRef& rxTable );

private:
    typedef RefVector< Table >           TableVector;

    TableVector         maTables;
    RefMap< sal_Int32, Table >  maIdTables;
    RefMap< OUString, Table >   maNameTables;
};

} // namespace oox::xls

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
