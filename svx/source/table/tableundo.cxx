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


#include <sdr/properties/cellproperties.hxx>
#include <editeng/outlobj.hxx>

#include <cell.hxx>
#include "tableundo.hxx"
#include <svx/svdotable.hxx>
#include "tablerow.hxx"
#include "tablecolumn.hxx"


using namespace ::com::sun::star::uno;


namespace sdr::table {

CellUndo::CellUndo( SdrObject* pObjRef, const CellRef& xCell )
:   SdrUndoAction(xCell->GetObject().getSdrModelFromSdrObject())
    ,mxObjRef( pObjRef )
    ,mxCell( xCell )
    ,mbUndo( true )
{
    if( mxCell.is() && pObjRef )
    {
        getDataFromCell( maUndoData );
        pObjRef->AddObjectUser( *this );
    }
}

CellUndo::~CellUndo()
{
    if( auto pObj = mxObjRef.get() )
        pObj->RemoveObjectUser( *this );
    dispose();
}

void CellUndo::dispose()
{
    mxCell.clear();
    maUndoData.mxProperties.reset();
    maRedoData.mxProperties.reset();
    maUndoData.mpOutlinerParaObject.reset();
    maRedoData.mpOutlinerParaObject.reset();
}

void CellUndo::ObjectInDestruction(const SdrObject& )
{
    dispose();
}

void CellUndo::Undo()
{
    if( mxCell.is() && mbUndo )
    {
        if( !maRedoData.mxProperties )
            getDataFromCell( maRedoData );

        setDataToCell( maUndoData );
        mbUndo = false;
    }
}

void CellUndo::Redo()
{
    if( mxCell.is() && !mbUndo )
    {
        setDataToCell( maRedoData );
        mbUndo = true;
    }
}

bool CellUndo::Merge( SfxUndoAction *pNextAction )
{
    CellUndo* pNext = dynamic_cast< CellUndo* >( pNextAction );
    return pNext && pNext->mxCell.get() == mxCell.get();
}

void CellUndo::setDataToCell( const Data& rData )
{
    if( rData.mxProperties )
        mxCell->mpProperties.reset(new properties::CellProperties( *rData.mxProperties, *mxObjRef.get(), mxCell.get() ));
    else
        mxCell->mpProperties.reset();

    if( rData.mpOutlinerParaObject )
        mxCell->SetOutlinerParaObject( *rData.mpOutlinerParaObject );
    else
        mxCell->RemoveOutlinerParaObject();

    mxCell->msFormula = rData.msFormula;
    mxCell->mfValue = rData.mfValue;
    mxCell->mnError = rData.mnError;
    mxCell->mbMerged = rData.mbMerged;
    mxCell->mnRowSpan = rData.mnRowSpan;
    mxCell->mnColSpan = rData.mnColSpan;

    if(auto pObj = mxObjRef.get())
    {
        // #i120201# ActionChanged is not enough, we need to trigger TableLayouter::UpdateBorderLayout()
        // and this is done best using ReformatText() for table objects
        pObj->ActionChanged();
        pObj->NbcReformatText();
    }
}

void CellUndo::getDataFromCell( Data& rData )
{
    if( !(mxObjRef.get().is() && mxCell.is()) )
        return;

    if( mxCell->mpProperties )
        rData.mxProperties.reset( mxCell->CloneProperties( *mxObjRef.get(), *mxCell) );

    if( mxCell->GetOutlinerParaObject() )
        rData.mpOutlinerParaObject = *mxCell->GetOutlinerParaObject();
    else
        rData.mpOutlinerParaObject.reset();

    rData.msFormula = mxCell->msFormula;
    rData.mfValue = mxCell->mfValue;
    rData.mnError = mxCell->mnError;
    rData.mbMerged = mxCell->mbMerged;
    rData.mnRowSpan = mxCell->mnRowSpan;
    rData.mnColSpan = mxCell->mnColSpan;
}


// class InsertRowUndo : public SdrUndoAction


static void Dispose( RowVector& rRows )
{
    for( auto& rpRow : rRows )
        rpRow->dispose();
}


InsertRowUndo::InsertRowUndo( const TableModelRef& xTable, sal_Int32 nIndex, RowVector& aNewRows )
:   SdrUndoAction(xTable->getSdrTableObj()->getSdrModelFromSdrObject())
    ,mxTable( xTable )
    ,mnIndex( nIndex )
    ,mbUndo( true )
{
    maRows.swap( aNewRows );
}


InsertRowUndo::~InsertRowUndo()
{
    if( !mbUndo )
        Dispose( maRows );
}


void InsertRowUndo::Undo()
{
    if( mxTable.is() )
    {
        mxTable->UndoInsertRows( mnIndex, sal::static_int_cast< sal_Int32 >( maRows.size() ) );
        mbUndo = false;
    }
}


void InsertRowUndo::Redo()
{
    if( mxTable.is() )
    {
        mxTable->UndoRemoveRows( mnIndex, maRows );
        mbUndo = true;
    }
}


// class RemoveRowUndo : public SdrUndoAction


RemoveRowUndo::RemoveRowUndo( const TableModelRef& xTable, sal_Int32 nIndex, RowVector& aRemovedRows )
:   SdrUndoAction(xTable->getSdrTableObj()->getSdrModelFromSdrObject())
    ,mxTable( xTable )
    ,mnIndex( nIndex )
    ,mbUndo( true )
{
    maRows.swap( aRemovedRows );
}


RemoveRowUndo::~RemoveRowUndo()
{
    if( mbUndo )
        Dispose( maRows );
}


void RemoveRowUndo::Undo()
{
    if( mxTable.is() )
    {
        mxTable->UndoRemoveRows( mnIndex, maRows );
        mbUndo = false;
    }
}


void RemoveRowUndo::Redo()
{
    if( mxTable.is() )
    {
        mxTable->UndoInsertRows( mnIndex, sal::static_int_cast< sal_Int32 >( maRows.size() ) );
        mbUndo = true;
    }
}


// class InsertColUndo : public SdrUndoAction


static void Dispose( ColumnVector& rCols )
{
    for( auto& rpCol : rCols )
        rpCol->dispose();
}


static void Dispose( CellVector& rCells )
{
    for( auto& rpCell : rCells )
        rpCell->dispose();
}


InsertColUndo::InsertColUndo( const TableModelRef& xTable, sal_Int32 nIndex, ColumnVector& aNewCols, CellVector& aCells  )
:   SdrUndoAction(xTable->getSdrTableObj()->getSdrModelFromSdrObject())
    ,mxTable( xTable )
    ,mnIndex( nIndex )
    ,mbUndo( true )
{
    maColumns.swap( aNewCols );
    maCells.swap( aCells );
}


InsertColUndo::~InsertColUndo()
{
    if( !mbUndo )
    {
        Dispose( maColumns );
        Dispose( maCells );
    }
}


void InsertColUndo::Undo()
{
    if( mxTable.is() )
    {
        mxTable->UndoInsertColumns( mnIndex, sal::static_int_cast< sal_Int32 >( maColumns.size() ) );
        mbUndo = false;
    }
}


void InsertColUndo::Redo()
{
    if( mxTable.is() )
    {
        mxTable->UndoRemoveColumns( mnIndex, maColumns, maCells );
        mbUndo = true;
    }
}


// class RemoveColUndo : public SdrUndoAction


RemoveColUndo::RemoveColUndo( const TableModelRef& xTable, sal_Int32 nIndex, ColumnVector& aNewCols, CellVector& aCells )
:   SdrUndoAction(xTable->getSdrTableObj()->getSdrModelFromSdrObject())
    ,mxTable( xTable )
    ,mnIndex( nIndex )
    ,mbUndo( true )
{
    maColumns.swap( aNewCols );
    maCells.swap( aCells );
}


RemoveColUndo::~RemoveColUndo()
{
    if( mbUndo )
    {
        Dispose( maColumns );
        Dispose( maCells );
    }
}


void RemoveColUndo::Undo()
{
    if( mxTable.is() )
    {
        mxTable->UndoRemoveColumns( mnIndex, maColumns, maCells );
        mbUndo = false;
    }
}


void RemoveColUndo::Redo()
{
    if( mxTable.is() )
    {
        mxTable->UndoInsertColumns( mnIndex, sal::static_int_cast< sal_Int32 >( maColumns.size() ) );
        mbUndo = true;
    }
}


// class TableColumnUndo : public SdrUndoAction


TableColumnUndo::TableColumnUndo( const TableColumnRef& xCol )
:   SdrUndoAction(xCol->mxTableModel->getSdrTableObj()->getSdrModelFromSdrObject())
    ,mxCol( xCol )
    ,mbHasRedoData( false )
{
    getData( maUndoData );
}


TableColumnUndo::~TableColumnUndo()
{
}


void TableColumnUndo::Undo()
{
    if( !mbHasRedoData )
    {
        getData( maRedoData );
        mbHasRedoData = true;
    }
    setData( maUndoData );
}


void TableColumnUndo::Redo()
{
    setData( maRedoData );
}


bool TableColumnUndo::Merge( SfxUndoAction *pNextAction )
{
    TableColumnUndo* pNext = dynamic_cast< TableColumnUndo* >( pNextAction );
    return pNext && pNext->mxCol == mxCol;
}


void TableColumnUndo::setData( const Data& rData )
{
    mxCol->mnColumn = rData.mnColumn;
    mxCol->mnWidth = rData.mnWidth;
    mxCol->mbOptimalWidth = rData.mbOptimalWidth;
    mxCol->mbIsVisible = rData.mbIsVisible;
    mxCol->mbIsStartOfNewPage = rData.mbIsStartOfNewPage;
    mxCol->maName = rData.maName;

    // Trigger re-layout of the table.
    mxCol->getModel()->setModified(true);
}


void TableColumnUndo::getData( Data& rData )
{
    rData.mnColumn = mxCol->mnColumn;
    rData.mnWidth = mxCol->mnWidth;
    rData.mbOptimalWidth = mxCol->mbOptimalWidth;
    rData.mbIsVisible = mxCol->mbIsVisible;
    rData.mbIsStartOfNewPage = mxCol->mbIsStartOfNewPage;
    rData.maName = mxCol->maName;
}


// class TableRowUndo : public SdrUndoAction


TableRowUndo::TableRowUndo( const TableRowRef& xRow )
:   SdrUndoAction(xRow->mxTableModel->getSdrTableObj()->getSdrModelFromSdrObject())
    , mxRow( xRow )
    , mbHasRedoData( false )
{
    getData( maUndoData );
}


TableRowUndo::~TableRowUndo()
{
}


void TableRowUndo::Undo()
{
    if( !mbHasRedoData )
    {
        getData( maRedoData );
        mbHasRedoData = true;
    }
    setData( maUndoData );
}


void TableRowUndo::Redo()
{
    setData( maRedoData );
}


bool TableRowUndo::Merge( SfxUndoAction *pNextAction )
{
    TableRowUndo* pNext = dynamic_cast< TableRowUndo* >( pNextAction );
    return pNext && pNext->mxRow == mxRow;
}


void TableRowUndo::setData( const Data& rData )
{
    mxRow->mnRow = rData.mnRow;
    mxRow->mnHeight = rData.mnHeight;
    mxRow->mbOptimalHeight = rData.mbOptimalHeight;
    mxRow->mbIsVisible = rData.mbIsVisible;
    mxRow->mbIsStartOfNewPage = rData.mbIsStartOfNewPage;
    mxRow->maName = rData.maName;

    // Trigger re-layout of the table.
    mxRow->getModel()->setModified(true);
 }


void TableRowUndo::getData( Data& rData )
{
    rData.mnRow = mxRow->mnRow;
    rData.mnHeight = mxRow->mnHeight;
    rData.mbOptimalHeight = mxRow->mbOptimalHeight;
    rData.mbIsVisible = mxRow->mbIsVisible;
    rData.mbIsStartOfNewPage = mxRow->mbIsStartOfNewPage;
    rData.maName = mxRow->maName;
}


TableStyleUndo::TableStyleUndo( const SdrTableObj& rTableObj )
:   SdrUndoAction(rTableObj.getSdrModelFromSdrObject())
    ,mxObjRef( const_cast< sdr::table::SdrTableObj*>( &rTableObj ) )
    ,mbHasRedoData(false)
{
    getData( maUndoData );
}

void TableStyleUndo::Undo()
{
    if( !mbHasRedoData )
    {
        getData( maRedoData );
        mbHasRedoData = true;
    }
    setData( maUndoData );
}

void TableStyleUndo::Redo()
{
    setData( maRedoData );
}

void TableStyleUndo::setData( const Data& rData )
{
    rtl::Reference<SdrTableObj> pTableObj = mxObjRef.get();
    if( pTableObj )
    {
        pTableObj->setTableStyle( rData.mxTableStyle );
        pTableObj->setTableStyleSettings( rData.maSettings );
    }
}

void TableStyleUndo::getData( Data& rData )
{
    rtl::Reference<SdrTableObj> pTableObj = mxObjRef.get();
    if( pTableObj )
    {
        rData.maSettings = pTableObj->getTableStyleSettings();
        rData.mxTableStyle = pTableObj->getTableStyle();
    }
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
