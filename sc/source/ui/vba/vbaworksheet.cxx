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

#include <sal/config.h>

#include <string_view>

#include "vbaworksheet.hxx"
#include "vbanames.hxx"

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XIntrospectionAccess.hpp>
#include <com/sun/star/container/XNamed.hpp>
#include <com/sun/star/util/XProtectable.hpp>
#include <com/sun/star/table/XCellRange.hpp>
#include <com/sun/star/sheet/XSpreadsheetView.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/sheet/XCalculatable.hpp>
#include <com/sun/star/sheet/XCellRangeAddressable.hpp>
#include <com/sun/star/sheet/XSheetCellRange.hpp>
#include <com/sun/star/sheet/XSheetCellCursor.hpp>
#include <com/sun/star/sheet/XSheetAnnotationsSupplier.hpp>
#include <com/sun/star/sheet/XUsedAreaCursor.hpp>
#include <com/sun/star/sheet/XSpreadsheets.hpp>
#include <com/sun/star/sheet/XSheetOutline.hpp>
#include <com/sun/star/sheet/XSheetPageBreak.hpp>
#include <com/sun/star/sheet/XDataPilotTablesSupplier.hpp>
#include <com/sun/star/sheet/XNamedRanges.hpp>
#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/table/XTableChartsSupplier.hpp>
#include <com/sun/star/drawing/XDrawPageSupplier.hpp>
#include <com/sun/star/drawing/XControlShape.hpp>
#include <com/sun/star/form/XFormsSupplier.hpp>
#include <ooo/vba/excel/XApplication.hpp>
#include <ooo/vba/excel/XlEnableSelection.hpp>
#include <ooo/vba/excel/XlSheetVisibility.hpp>
#include <ooo/vba/XControlProvider.hpp>

#include <basic/sberrors.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/servicehelper.hxx>
#include <utility>
#include <vbahelper/vbashapes.hxx>

//zhangyun showdataform
#include <scabstdlg.hxx>
#include <tabvwsh.hxx>

#include <tabprotection.hxx>
#include "excelvbahelper.hxx"
#include "vbaoutline.hxx"
#include "vbarange.hxx"
#include "vbacomments.hxx"
#include "vbachartobjects.hxx"
#include "vbapivottables.hxx"
#include "vbaoleobjects.hxx"
#include "vbapagesetup.hxx"
#include "vbapagebreaks.hxx"
#include "vbaworksheets.hxx"
#include "vbahyperlinks.hxx"
#include "vbasheetobjects.hxx"
#include <dbdata.hxx>

#include <attrib.hxx>

#define STANDARDWIDTH 2267
#define STANDARDHEIGHT 427

using namespace com::sun::star;
using namespace ooo::vba;

static void getNewSpreadsheetName (OUString &aNewName, std::u16string_view aOldName, const uno::Reference <sheet::XSpreadsheetDocument>& xSpreadDoc )
{
    if (!xSpreadDoc.is())
        throw lang::IllegalArgumentException( u"getNewSpreadsheetName() xSpreadDoc is null"_ustr, uno::Reference< uno::XInterface  >(), 1 );
    static const char aUnderScore[] =  "_";
    int currentNum =2;
    aNewName = OUString::Concat(aOldName) + aUnderScore + OUString::number(currentNum) ;
    SCTAB nTab = 0;
    while ( ScVbaWorksheets::nameExists(xSpreadDoc,aNewName, nTab ) )
    {
        aNewName = OUString::Concat(aOldName) + aUnderScore + OUString::number(++currentNum);
    }
}

static void removeAllSheets( const uno::Reference <sheet::XSpreadsheetDocument>& xSpreadDoc, const OUString& aSheetName)
{
    if (!xSpreadDoc.is())
        throw lang::IllegalArgumentException( u"removeAllSheets() xSpreadDoc is null"_ustr, uno::Reference< uno::XInterface  >(), 1 );
    uno::Reference<sheet::XSpreadsheets> xSheets = xSpreadDoc->getSheets();
    uno::Reference <container::XIndexAccess> xIndex( xSheets, uno::UNO_QUERY );

    if ( !xIndex.is() )
        return;

    uno::Reference<container::XNameContainer> xNameContainer(xSheets,uno::UNO_QUERY_THROW);
    for (sal_Int32 i = xIndex->getCount() -1; i>= 1; i--)
    {
        uno::Reference< sheet::XSpreadsheet > xSheet(xIndex->getByIndex(i), uno::UNO_QUERY);
        uno::Reference< container::XNamed > xNamed( xSheet, uno::UNO_QUERY_THROW );
        xNameContainer->removeByName(xNamed->getName());
    }

    uno::Reference< sheet::XSpreadsheet > xSheet(xIndex->getByIndex(0), uno::UNO_QUERY);
    uno::Reference< container::XNamed > xNamed( xSheet, uno::UNO_QUERY_THROW );
    xNamed->setName(aSheetName);
}

static uno::Reference<frame::XModel>
openNewDoc(const OUString& aSheetName )
{
    uno::Reference<frame::XModel> xModel;
    try
    {
        const uno::Reference< uno::XComponentContext >& xContext(
            comphelper::getProcessComponentContext() );

        uno::Reference <frame::XDesktop2 > xComponentLoader = frame::Desktop::create(xContext);

        uno::Reference<lang::XComponent > xComponent( xComponentLoader->loadComponentFromURL(
                u"private:factory/scalc"_ustr,
                u"_blank"_ustr, 0,
                uno::Sequence < css::beans::PropertyValue >() ) );
        uno::Reference <sheet::XSpreadsheetDocument> xSpreadDoc( xComponent, uno::UNO_QUERY_THROW );
        removeAllSheets(xSpreadDoc,aSheetName);
        xModel.set(xSpreadDoc,uno::UNO_QUERY_THROW);
    }
    catch ( uno::Exception & /*e*/ )
    {
    }
    return xModel;
}

ScVbaWorksheet::ScVbaWorksheet(const uno::Reference< XHelperInterface >& xParent, const uno::Reference< uno::XComponentContext >& xContext,
        uno::Reference< sheet::XSpreadsheet > xSheet,
        uno::Reference< frame::XModel > xModel ) : WorksheetImpl_BASE( xParent, xContext ), mxSheet(std::move( xSheet )), mxModel(std::move(xModel)), mbVeryHidden( false )
{
}

ScVbaWorksheet::ScVbaWorksheet( uno::Sequence< uno::Any> const & args,
    uno::Reference< uno::XComponentContext> const & xContext ) :  WorksheetImpl_BASE( getXSomethingFromArgs< XHelperInterface >( args, 0 ), xContext ), mxModel( getXSomethingFromArgs< frame::XModel >( args, 1 ) ), mbVeryHidden( false )
{
    if ( args.getLength() < 3 )
        throw lang::IllegalArgumentException();

    OUString sSheetName;
    args[2] >>= sSheetName;

    uno::Reference< sheet::XSpreadsheetDocument > xSpreadDoc( mxModel, uno::UNO_QUERY_THROW );
    uno::Reference< container::XNameAccess > xNameAccess( xSpreadDoc->getSheets(), uno::UNO_QUERY_THROW );
    mxSheet.set( xNameAccess->getByName( sSheetName ), uno::UNO_QUERY_THROW );
}

ScVbaWorksheet::~ScVbaWorksheet()
{
}

const uno::Sequence<sal_Int8>& ScVbaWorksheet::getUnoTunnelId()
{
    static const comphelper::UnoIdInit theScVbaWorksheetUnoTunnelId;
    return theScVbaWorksheetUnoTunnelId.getSeq();
}

uno::Reference< ov::excel::XWorksheet >
ScVbaWorksheet::createSheetCopyInNewDoc(const OUString& aCurrSheetName)
{
    uno::Reference< sheet::XSheetCellCursor > xSheetCellCursor = getSheet()->createCursor( );
    uno::Reference<sheet::XUsedAreaCursor> xUsedCursor(xSheetCellCursor,uno::UNO_QUERY_THROW);
    uno::Reference<excel::XRange> xRange =  new ScVbaRange( this, mxContext, xSheetCellCursor);
    if (xRange.is())
        xRange->Select();
    excel::implnCopy(mxModel);
    uno::Reference<frame::XModel> xModel = openNewDoc(aCurrSheetName);
    if (xModel.is())
    {
        excel::implnPaste(xModel);
    }
    uno::Reference <sheet::XSpreadsheetDocument> xSpreadDoc( xModel, uno::UNO_QUERY_THROW );
    excel::setUpDocumentModules(xSpreadDoc);
    uno::Reference <sheet::XSpreadsheets> xSheets( xSpreadDoc->getSheets(), uno::UNO_SET_THROW );
    uno::Reference <container::XIndexAccess> xIndex( xSheets, uno::UNO_QUERY_THROW );
    uno::Reference< sheet::XSpreadsheet > xSheet(xIndex->getByIndex(0), uno::UNO_QUERY_THROW);

    ScDocShell* pShell = excel::getDocShell( xModel );
    OUString aCodeName;
    if (pShell)
        pShell->GetDocument().GetCodeName( 0, aCodeName );
    return uno::Reference< excel::XWorksheet >( getUnoDocModule( aCodeName, pShell ), uno::UNO_QUERY_THROW );
}

css::uno::Reference< ov::excel::XWorksheet >
ScVbaWorksheet::createSheetCopy(uno::Reference<excel::XWorksheet> const & xSheet, bool bAfter)
{
    OUString aCurrSheetName = getName();
    ScVbaWorksheet* pDestSheet = excel::getImplFromDocModuleWrapper<ScVbaWorksheet>( xSheet );

    uno::Reference <sheet::XSpreadsheetDocument> xDestDoc( pDestSheet->getModel(), uno::UNO_QUERY );
    uno::Reference <sheet::XSpreadsheetDocument> xSrcDoc( getModel(), uno::UNO_QUERY );

    SCTAB nDest = 0;
    SCTAB nSrc = 0;
    OUString aSheetName = xSheet->getName();
    bool bSameDoc = ( pDestSheet->getModel() == getModel() );
    bool bDestSheetExists = ScVbaWorksheets::nameExists (xDestDoc, aSheetName, nDest );
    bool bSheetExists = ScVbaWorksheets::nameExists (xSrcDoc, aCurrSheetName, nSrc );

    // set sheet name to be newSheet name
    aSheetName = aCurrSheetName;
    if ( bSheetExists && bDestSheetExists )
    {
        SCTAB nDummy=0;
        if(bAfter)
              nDest++;
        uno::Reference<sheet::XSpreadsheets> xSheets = xDestDoc->getSheets();
        if ( bSameDoc || ScVbaWorksheets::nameExists( xDestDoc, aCurrSheetName, nDummy ) )
            getNewSpreadsheetName(aSheetName,aCurrSheetName,xDestDoc);
        if ( bSameDoc )
            xSheets->copyByName(aCurrSheetName,aSheetName,nDest);
        else
        {
            ScDocShell* pDestDocShell = excel::getDocShell( pDestSheet->getModel() );
            ScDocShell* pSrcDocShell = excel::getDocShell( getModel() );
            if ( pDestDocShell && pSrcDocShell )
                pDestDocShell->TransferTab( *pSrcDocShell, nSrc, nDest, true, true );
        }
    }
    // return new sheet
    uno::Reference< excel::XApplication > xApplication( Application(), uno::UNO_QUERY_THROW );
    uno::Reference< excel::XWorksheet > xNewSheet( xApplication->Worksheets( uno::Any( aSheetName ) ), uno::UNO_QUERY_THROW );
    return xNewSheet;
}

OUString
ScVbaWorksheet::getName()
{
    uno::Reference< container::XNamed > xNamed( getSheet(), uno::UNO_QUERY_THROW );
    return xNamed->getName();
}

void
ScVbaWorksheet::setName(const OUString &rName )
{
    uno::Reference< container::XNamed > xNamed( getSheet(), uno::UNO_QUERY_THROW );
    xNamed->setName( rName );
}

sal_Int32
ScVbaWorksheet::getVisible()
{
    uno::Reference< beans::XPropertySet > xProps( getSheet(), uno::UNO_QUERY_THROW );
    bool bVisible = false;
    xProps->getPropertyValue( u"IsVisible"_ustr ) >>= bVisible;
    using namespace ::ooo::vba::excel::XlSheetVisibility;
    return bVisible ? xlSheetVisible : (mbVeryHidden ? xlSheetVeryHidden : xlSheetHidden);
}

void
ScVbaWorksheet::setVisible( sal_Int32 nVisible )
{
    using namespace ::ooo::vba::excel::XlSheetVisibility;
    bool bVisible = true;
    switch( nVisible )
    {
        case xlSheetVisible: case 1:  // Excel accepts -1 and 1 for visible sheets
            bVisible = true;
            mbVeryHidden = false;
        break;
        case xlSheetHidden:
            bVisible = false;
            mbVeryHidden = false;
        break;
        case xlSheetVeryHidden:
            bVisible = false;
            mbVeryHidden = true;
        break;
        default:
            throw uno::RuntimeException();
    }
    uno::Reference< beans::XPropertySet > xProps( getSheet(), uno::UNO_QUERY_THROW );
    xProps->setPropertyValue( u"IsVisible"_ustr, uno::Any( bVisible ) );
}

sal_Int16
ScVbaWorksheet::getIndex()
{
    return getSheetID() + 1;
}

sal_Int32
ScVbaWorksheet::getEnableSelection()
{
    uno::Reference <sheet::XSpreadsheetDocument> xSpreadDoc( getModel(), uno::UNO_QUERY_THROW );
    SCTAB nTab = 0;
    if ( !ScVbaWorksheets::nameExists(xSpreadDoc, getName(), nTab) )
        throw uno::RuntimeException(u"Sheet Name does not exist."_ustr );

    if ( ScDocShell* pShell = excel::getDocShell( getModel() ))
    {
        ScDocument& rDoc = pShell->GetDocument();
        const ScTableProtection* pProtect = rDoc.GetTabProtection(nTab);
        bool bLockedCells = false;
        bool bUnlockedCells = false;
        if( pProtect )
        {
            bLockedCells   = pProtect->isOptionEnabled(ScTableProtection::SELECT_LOCKED_CELLS);
            bUnlockedCells = pProtect->isOptionEnabled(ScTableProtection::SELECT_UNLOCKED_CELLS);
        }
        if( bLockedCells )
            return excel::XlEnableSelection::xlNoRestrictions;
        if( bUnlockedCells )
            return excel::XlEnableSelection::xlUnlockedCells;
    }
    return excel::XlEnableSelection::xlNoSelection;

}

void
ScVbaWorksheet::setEnableSelection( sal_Int32 nSelection )
{
    if( (nSelection != excel::XlEnableSelection::xlNoRestrictions) &&
        (nSelection != excel::XlEnableSelection::xlUnlockedCells) &&
        (nSelection != excel::XlEnableSelection::xlNoSelection) )
    {
        DebugHelper::runtimeexception(ERRCODE_BASIC_BAD_PARAMETER);
    }

    uno::Reference <sheet::XSpreadsheetDocument> xSpreadDoc( getModel(), uno::UNO_QUERY_THROW );
    SCTAB nTab = 0;
    if ( !ScVbaWorksheets::nameExists(xSpreadDoc, getName(), nTab) )
        throw uno::RuntimeException(u"Sheet Name does not exist."_ustr );

    if ( ScDocShell* pShell = excel::getDocShell( getModel() ))
    {
        ScDocument& rDoc = pShell->GetDocument();
        const ScTableProtection* pProtect = rDoc.GetTabProtection(nTab);
        // default is xlNoSelection
        bool bLockedCells = false;
        bool bUnlockedCells = false;
        if( nSelection == excel::XlEnableSelection::xlNoRestrictions )
        {
            bLockedCells = true;
            bUnlockedCells = true;
        }
        else if( nSelection == excel::XlEnableSelection::xlUnlockedCells )
        {
            bUnlockedCells = true;
        }
        if( pProtect )
        {
            ScTableProtection aNewProtect(*pProtect);
            aNewProtect.setOption(ScTableProtection::SELECT_LOCKED_CELLS, bLockedCells);
            aNewProtect.setOption(ScTableProtection::SELECT_UNLOCKED_CELLS, bUnlockedCells);
            rDoc.SetTabProtection(nTab, &aNewProtect);
        }
    }
}

sal_Bool SAL_CALL ScVbaWorksheet::getAutoFilterMode()
{
    if ( ScDocShell* pShell = excel::getDocShell( getModel() ))
    {
        ScDocument& rDoc = pShell->GetDocument();
        ScDBData* pDBData = rDoc.GetAnonymousDBData(getSheetID());
        if (pDBData)
            return pDBData->HasAutoFilter();
    }
    return false;
}

void SAL_CALL ScVbaWorksheet::setAutoFilterMode( sal_Bool bAutoFilterMode )
{
    ScDocShell* pDocShell = excel::getDocShell( getModel() );
    if (!pDocShell)
        return;
    ScDocument& rDoc = pDocShell->GetDocument();
    ScDBData* pDBData = rDoc.GetAnonymousDBData(getSheetID());
    if (!pDBData)
        return;

    pDBData->SetAutoFilter(bAutoFilterMode);
    ScRange aRange;
    pDBData->GetArea(aRange);
    if (bAutoFilterMode)
        rDoc.ApplyFlagsTab( aRange.aStart.Col(), aRange.aStart.Row(),
                                aRange.aEnd.Col(), aRange.aStart.Row(),
                                aRange.aStart.Tab(), ScMF::Auto );
    else if (!bAutoFilterMode)
        rDoc.RemoveFlagsTab(aRange.aStart.Col(), aRange.aStart.Row(),
                                aRange.aEnd.Col(), aRange.aStart.Row(),
                                aRange.aStart.Tab(), ScMF::Auto );
    ScRange aPaintRange(aRange.aStart, aRange.aEnd);
    aPaintRange.aEnd.SetRow(aPaintRange.aStart.Row());
    pDocShell->PostPaint(aPaintRange, PaintPartFlags::Grid);
}

uno::Reference< excel::XRange >
ScVbaWorksheet::getUsedRange()
{
    uno::Reference< sheet::XSheetCellRange > xSheetCellRange(getSheet(), uno::UNO_QUERY_THROW );
    uno::Reference< sheet::XSheetCellCursor > xSheetCellCursor( getSheet()->createCursorByRange( xSheetCellRange ), uno::UNO_SET_THROW );
    uno::Reference<sheet::XUsedAreaCursor> xUsedCursor(xSheetCellCursor,uno::UNO_QUERY_THROW);
    xUsedCursor->gotoStartOfUsedArea( false );
    xUsedCursor->gotoEndOfUsedArea( true );
    return new ScVbaRange(this, mxContext, xSheetCellCursor);
}

uno::Reference< excel::XOutline >
ScVbaWorksheet::Outline( )
{
    uno::Reference<sheet::XSheetOutline> xOutline(getSheet(),uno::UNO_QUERY_THROW);
    return new ScVbaOutline( this, mxContext, xOutline);
}

uno::Reference< excel::XPageSetup >
ScVbaWorksheet::PageSetup( )
{
    return new ScVbaPageSetup( this, mxContext, getSheet(), getModel() );
}

uno::Any
ScVbaWorksheet::HPageBreaks( const uno::Any& aIndex )
{
    uno::Reference< sheet::XSheetPageBreak > xSheetPageBreak(getSheet(),uno::UNO_QUERY_THROW);
    uno::Reference< excel::XHPageBreaks > xHPageBreaks( new ScVbaHPageBreaks( this, mxContext, xSheetPageBreak));
    if ( aIndex.hasValue() )
        return xHPageBreaks->Item( aIndex, uno::Any());
    return uno::Any( xHPageBreaks );
}

uno::Any
ScVbaWorksheet::VPageBreaks( const uno::Any& aIndex )
{
    uno::Reference< sheet::XSheetPageBreak > xSheetPageBreak( getSheet(), uno::UNO_QUERY_THROW );
    uno::Reference< excel::XVPageBreaks > xVPageBreaks( new ScVbaVPageBreaks( this, mxContext, xSheetPageBreak ) );
    if( aIndex.hasValue() )
        return xVPageBreaks->Item( aIndex, uno::Any());
    return uno::Any( xVPageBreaks );
}

sal_Int32
ScVbaWorksheet::getStandardWidth()
{
    return STANDARDWIDTH ;
}

sal_Int32
ScVbaWorksheet::getStandardHeight()
{
    return STANDARDHEIGHT;
}

sal_Bool
ScVbaWorksheet::getProtectionMode()
{
    return false;
}

sal_Bool
ScVbaWorksheet::getProtectContents()
{
    uno::Reference<util::XProtectable > xProtectable(getSheet(), uno::UNO_QUERY_THROW);
    return xProtectable->isProtected();
}

sal_Bool
ScVbaWorksheet::getProtectDrawingObjects()
{
    SCTAB nTab = 0;
    OUString aSheetName = getName();
    uno::Reference <sheet::XSpreadsheetDocument> xSpreadDoc( getModel(), uno::UNO_QUERY_THROW );
    bool bSheetExists = ScVbaWorksheets::nameExists (xSpreadDoc, aSheetName, nTab);
    if ( bSheetExists )
    {
        if ( ScDocShell* pShell = excel::getDocShell( getModel() ))
        {
            ScDocument& rDoc = pShell->GetDocument();
            const ScTableProtection* pProtect = rDoc.GetTabProtection(nTab);
            if ( pProtect )
                return pProtect->isOptionEnabled( ScTableProtection::OBJECTS );
        }
    }
    return false;
}

sal_Bool
ScVbaWorksheet::getProtectScenarios()
{
    return false;
}

void
ScVbaWorksheet::Activate()
{
    uno::Reference< sheet::XSpreadsheetView > xSpreadsheet(
            getModel()->getCurrentController(), uno::UNO_QUERY_THROW );
    xSpreadsheet->setActiveSheet(getSheet());
}

void
ScVbaWorksheet::Select()
{
    Activate();
}

void
ScVbaWorksheet::Move( const uno::Any& Before, const uno::Any& After )
{
    uno::Reference<excel::XWorksheet> xSheet;
    OUString aCurrSheetName = getName();

    if (!(Before >>= xSheet) && !(After >>=xSheet)&& !(Before.hasValue()) && !(After.hasValue()))
    {
        uno::Reference< sheet::XSheetCellCursor > xSheetCellCursor = getSheet()->createCursor( );
        uno::Reference<sheet::XUsedAreaCursor> xUsedCursor(xSheetCellCursor,uno::UNO_QUERY_THROW);
        // #FIXME needs worksheet as parent
        uno::Reference<excel::XRange> xRange =  new ScVbaRange( this, mxContext, xSheetCellCursor);
        if (xRange.is())
            xRange->Select();
        excel::implnCopy(mxModel);
        uno::Reference<frame::XModel> xModel = openNewDoc(aCurrSheetName);
        if (xModel.is())
        {
            excel::implnPaste(xModel);
            Delete();
        }
        return ;
    }

    uno::Reference <sheet::XSpreadsheetDocument> xSpreadDoc( getModel(), uno::UNO_QUERY_THROW );
    SCTAB nDest = 0;
    if ( ScVbaWorksheets::nameExists (xSpreadDoc, xSheet->getName(), nDest) )
    {
        bool bAfter = After.hasValue();
        if (bAfter)
            nDest++;
        uno::Reference<sheet::XSpreadsheets> xSheets = xSpreadDoc->getSheets();
        xSheets->moveByName(aCurrSheetName,nDest);
    }
}

void
ScVbaWorksheet::Copy( const uno::Any& Before, const uno::Any& After )
{
    uno::Reference<excel::XWorksheet> xSheet;
    if (!(Before >>= xSheet) && !(After >>=xSheet)&& !(Before.hasValue()) && !(After.hasValue()))
    {
        createSheetCopyInNewDoc(getName());
        return;
    }

    uno::Reference<excel::XWorksheet> xNewSheet = createSheetCopy(xSheet, After.hasValue());
    xNewSheet->Activate();
}

void
ScVbaWorksheet::Paste( const uno::Any& Destination, const uno::Any& /*Link*/ )
{
    // #TODO# #FIXME# Link is not used
    uno::Reference<excel::XRange> xRange( Destination, uno::UNO_QUERY );
    if ( xRange.is() )
        xRange->Select();
    excel::implnPaste( mxModel );
}

void
ScVbaWorksheet::Delete()
{
    uno::Reference <sheet::XSpreadsheetDocument> xSpreadDoc( getModel(), uno::UNO_QUERY_THROW );
    OUString aSheetName = getName();
    SCTAB nTab = 0;
    if (!ScVbaWorksheets::nameExists(xSpreadDoc, aSheetName, nTab ))
    {
        return;
    }
    uno::Reference<sheet::XSpreadsheets> xSheets = xSpreadDoc->getSheets();
    uno::Reference<container::XNameContainer> xNameContainer(xSheets,uno::UNO_QUERY_THROW);
    xNameContainer->removeByName(aSheetName);
    mxSheet.clear();
}

uno::Reference< excel::XWorksheet >
ScVbaWorksheet::getSheetAtOffset(SCTAB offset)
{
    uno::Reference <sheet::XSpreadsheetDocument> xSpreadDoc( getModel(), uno::UNO_QUERY_THROW );
    uno::Reference <sheet::XSpreadsheets> xSheets( xSpreadDoc->getSheets(), uno::UNO_SET_THROW );
    uno::Reference <container::XIndexAccess> xIndex( xSheets, uno::UNO_QUERY_THROW );

    SCTAB nIdx = 0;
    if ( !ScVbaWorksheets::nameExists (xSpreadDoc, getName(), nIdx ) )
        return uno::Reference< excel::XWorksheet >();
    nIdx = nIdx + offset;
    uno::Reference< sheet::XSpreadsheet > xSheet(xIndex->getByIndex(nIdx), uno::UNO_QUERY_THROW);
    // parent will be the parent of 'this' worksheet
    return new ScVbaWorksheet (getParent(), mxContext, xSheet, getModel());
}

uno::Reference< excel::XWorksheet >
ScVbaWorksheet::getNext()
{
    return getSheetAtOffset(static_cast<SCTAB>(1));
}

uno::Reference< excel::XWorksheet >
ScVbaWorksheet::getPrevious()
{
    return getSheetAtOffset(-1);
}

void
ScVbaWorksheet::Protect( const uno::Any& Password, const uno::Any& /*DrawingObjects*/, const uno::Any& /*Contents*/, const uno::Any& /*Scenarios*/, const uno::Any& /*UserInterfaceOnly*/ )
{
    // #TODO# #FIXME# is there anything we can do with the unused param
    // can the implementation use anything else here
    uno::Reference<util::XProtectable > xProtectable(getSheet(), uno::UNO_QUERY_THROW);
    OUString aPasswd;
    Password >>= aPasswd;
    xProtectable->protect( aPasswd );
}

void
ScVbaWorksheet::Unprotect( const uno::Any& Password )
{
    uno::Reference<util::XProtectable > xProtectable(getSheet(), uno::UNO_QUERY_THROW);
    OUString aPasswd;
    Password >>= aPasswd;
    xProtectable->unprotect( aPasswd );
}

void
ScVbaWorksheet::Calculate()
{
    uno::Reference <sheet::XCalculatable> xReCalculate(getModel(), uno::UNO_QUERY_THROW);
    xReCalculate->calculate();
}

uno::Reference< excel::XRange >
ScVbaWorksheet::Range( const ::uno::Any& Cell1, const ::uno::Any& Cell2 )
{
    uno::Reference< excel::XRange > xSheetRange( new ScVbaRange( this, mxContext
, uno::Reference< table::XCellRange >( getSheet(), uno::UNO_QUERY_THROW ) ) );
    return xSheetRange->Range( Cell1, Cell2 );
}

void
ScVbaWorksheet::CheckSpelling( const uno::Any& /*CustomDictionary*/,const uno::Any& /*IgnoreUppercase*/,const uno::Any& /*AlwaysSuggest*/, const uno::Any& /*SpellingLang*/ )
{
    // #TODO# #FIXME# unused params above, can we do anything with those
    uno::Reference< frame::XModel > xModel( getModel() );
    dispatchRequests(xModel,u".uno:SpellDialog"_ustr);
}

uno::Reference< excel::XRange >
ScVbaWorksheet::getSheetRange()
{
    uno::Reference< table::XCellRange > xRange( getSheet(),uno::UNO_QUERY_THROW );
    return uno::Reference< excel::XRange >( new ScVbaRange( this, mxContext, xRange ) );
}

// These are hacks - we prolly (somehow) need to inherit
// the vbarange functionality here ...
uno::Reference< excel::XRange >
ScVbaWorksheet::Cells( const ::uno::Any &nRow, const ::uno::Any &nCol )
{
    // Performance optimization for often-called Cells method:
    // Use a common helper method instead of creating a new ScVbaRange object
    uno::Reference< table::XCellRange > xRange( getSheet(), uno::UNO_QUERY_THROW );
    uno::Reference< frame::XModel > xModel( getModel(), uno::UNO_SET_THROW );
    if(ScDocShell* pShell = excel::getDocShell( xModel ))
        return ScVbaRange::CellsHelper(pShell->GetDocument(), this, mxContext, xRange, nRow, nCol );
    throw uno::RuntimeException();
}

uno::Reference< excel::XRange >
ScVbaWorksheet::Rows(const uno::Any& aIndex )
{
    return getSheetRange()->Rows( aIndex );
}

uno::Reference< excel::XRange >
ScVbaWorksheet::Columns( const uno::Any& aIndex )
{
    return getSheetRange()->Columns( aIndex );
}

uno::Any SAL_CALL
ScVbaWorksheet::ChartObjects( const uno::Any& Index )
{
    if ( !mxCharts.is() )
    {
        uno::Reference< table::XTableChartsSupplier > xChartSupplier( getSheet(), uno::UNO_QUERY_THROW );
        uno::Reference< table::XTableCharts > xTableCharts = xChartSupplier->getCharts();

        uno::Reference< drawing::XDrawPageSupplier > xDrawPageSupplier( mxSheet, uno::UNO_QUERY_THROW );
        mxCharts = new ScVbaChartObjects(  this, mxContext, xTableCharts, xDrawPageSupplier );
    }
    if ( Index.hasValue() )
    {
        return mxCharts->Item( Index, uno::Any() );
    }
    else
        return uno::Any( uno::Reference<ov::excel::XChartObjects>(mxCharts) );

}

uno::Any SAL_CALL
ScVbaWorksheet::PivotTables( const uno::Any& Index )
{
    uno::Reference< css::sheet::XSpreadsheet > xSheet = getSheet();
    uno::Reference< sheet::XDataPilotTablesSupplier > xTables(xSheet, uno::UNO_QUERY_THROW ) ;
    uno::Reference< container::XIndexAccess > xIndexAccess( xTables->getDataPilotTables(), uno::UNO_QUERY_THROW );

    uno::Reference< XCollection > xColl(  new ScVbaPivotTables( this, mxContext, xIndexAccess ) );
    if ( Index.hasValue() )
        return xColl->Item( Index, uno::Any() );
    return uno::Any( xColl );
}

uno::Any SAL_CALL
ScVbaWorksheet::Comments( const uno::Any& Index )
{
    uno::Reference< css::sheet::XSpreadsheet > xSheet = getSheet();
    uno::Reference< sheet::XSheetAnnotationsSupplier > xAnnosSupp( xSheet, uno::UNO_QUERY_THROW );
    uno::Reference< sheet::XSheetAnnotations > xAnnos( xAnnosSupp->getAnnotations(), uno::UNO_SET_THROW );
    uno::Reference< container::XIndexAccess > xIndexAccess( xAnnos, uno::UNO_QUERY_THROW );
    uno::Reference< XCollection > xColl(  new ScVbaComments( this, mxContext, mxModel, xIndexAccess ) );
    if ( Index.hasValue() )
        return xColl->Item( Index, uno::Any() );
    return uno::Any( xColl );
}

uno::Any SAL_CALL
ScVbaWorksheet::Hyperlinks( const uno::Any& aIndex )
{
    /*  The worksheet always returns the same Hyperlinks object.
        See vbahyperlinks.hxx for more details. */
    if( !mxHlinks.is() )
        mxHlinks.set( new ScVbaHyperlinks( this, mxContext ) );
    if( aIndex.hasValue() )
        return uno::Reference< XCollection >( mxHlinks, uno::UNO_QUERY_THROW )->Item( aIndex, uno::Any() );
    return uno::Any( mxHlinks );
}

uno::Any SAL_CALL
ScVbaWorksheet::Names( const css::uno::Any& aIndex )
{
    css::uno::Reference<css::beans::XPropertySet> xProps(getSheet(), css::uno::UNO_QUERY_THROW);
    uno::Reference< sheet::XNamedRanges > xNamedRanges(  xProps->getPropertyValue(u"NamedRanges"_ustr), uno::UNO_QUERY_THROW );
    uno::Reference< XCollection > xNames( new ScVbaNames( this, mxContext, xNamedRanges, mxModel ) );
    if ( aIndex.hasValue() )
        return xNames->Item( aIndex, uno::Any() );
    return uno::Any( xNames );
}

uno::Any SAL_CALL
ScVbaWorksheet::OLEObjects( const uno::Any& Index )
{
    uno::Reference< sheet::XSpreadsheet > xSpreadsheet( getSheet(), uno::UNO_SET_THROW );
    uno::Reference< drawing::XDrawPageSupplier > xDrawPageSupplier( xSpreadsheet, uno::UNO_QUERY_THROW );
    uno::Reference< drawing::XDrawPage > xDrawPage( xDrawPageSupplier->getDrawPage(), uno::UNO_SET_THROW );
    uno::Reference< container::XIndexAccess > xIndexAccess( xDrawPage, uno::UNO_QUERY_THROW );

    uno::Reference< excel::XOLEObjects >xOleObjects( new ScVbaOLEObjects( this, mxContext, xIndexAccess ) );
    if( Index.hasValue() )
        return xOleObjects->Item( Index, uno::Any() );
    return uno::Any( xOleObjects );
}

uno::Any SAL_CALL
ScVbaWorksheet::Shapes( const uno::Any& aIndex )
{
    uno::Reference< sheet::XSpreadsheet > xSpreadsheet( getSheet(), uno::UNO_SET_THROW );
    uno::Reference< drawing::XDrawPageSupplier > xDrawPageSupplier( xSpreadsheet, uno::UNO_QUERY_THROW );
    uno::Reference< drawing::XShapes > xShapes( xDrawPageSupplier->getDrawPage(), uno::UNO_QUERY_THROW );
    uno::Reference< container::XIndexAccess > xIndexAccess( xShapes, uno::UNO_QUERY_THROW );

    uno::Reference< msforms::XShapes> xVbaShapes( new ScVbaShapes( this, mxContext, xIndexAccess, getModel() ) );
    if ( aIndex.hasValue() )
        return xVbaShapes->Item( aIndex, uno::Any() );
    return uno::Any( xVbaShapes );
}

uno::Any
ScVbaWorksheet::getButtons( const uno::Any &rIndex, bool bOptionButtons )
{
    ::rtl::Reference< ScVbaSheetObjectsBase > &rxButtons = bOptionButtons ? mxButtons[0] : mxButtons[1];

    if( !rxButtons.is() )
        rxButtons.set( new ScVbaButtons( this, mxContext, mxModel, mxSheet, bOptionButtons ) );
    else
        rxButtons->collectShapes();
    if( rIndex.hasValue() )
        return rxButtons->Item( rIndex, uno::Any() );
    return uno::Any( uno::Reference< XCollection >( rxButtons ) );
}

uno::Any SAL_CALL
ScVbaWorksheet::Buttons( const uno::Any& rIndex )
{
    return getButtons( rIndex, false );
}

uno::Any SAL_CALL
ScVbaWorksheet::CheckBoxes( const uno::Any& /*rIndex*/ )
{
    throw uno::RuntimeException();
}

uno::Any SAL_CALL
ScVbaWorksheet::DropDowns( const uno::Any& /*rIndex*/ )
{
    throw uno::RuntimeException();
}

uno::Any SAL_CALL
ScVbaWorksheet::GroupBoxes( const uno::Any& /*rIndex*/ )
{
    throw uno::RuntimeException();
}

uno::Any SAL_CALL
ScVbaWorksheet::Labels( const uno::Any& /*rIndex*/ )
{
    throw uno::RuntimeException();
}

uno::Any SAL_CALL
ScVbaWorksheet::ListBoxes( const uno::Any& /*rIndex*/ )
{
    throw uno::RuntimeException();
}

uno::Any SAL_CALL
ScVbaWorksheet::OptionButtons( const uno::Any& rIndex )
{
    return getButtons( rIndex, true );
}

uno::Any SAL_CALL
ScVbaWorksheet::ScrollBars( const uno::Any& /*rIndex*/ )
{
    throw uno::RuntimeException();
}

uno::Any SAL_CALL
ScVbaWorksheet::Spinners( const uno::Any& /*rIndex*/ )
{
    throw uno::RuntimeException();
}

void SAL_CALL
ScVbaWorksheet::ShowDataForm( )
{
    uno::Reference< frame::XModel > xModel( getModel(), uno::UNO_SET_THROW );
    if (ScTabViewShell* pTabViewShell = excel::getBestViewShell( xModel ))
    {
        ScAbstractDialogFactory* pFact = ScAbstractDialogFactory::Create();
        ScopedVclPtr<AbstractScDataFormDlg> pDlg(pFact->CreateScDataFormDlg(pTabViewShell->GetFrameWeld(),
                                                                        pTabViewShell));
        pDlg->Execute();
    }
}

uno::Any SAL_CALL
ScVbaWorksheet::Evaluate( const OUString& Name )
{
    // #TODO Evaluate allows other things to be evaluated, e.g. functions
    // I think ( like SIN(3) etc. ) need to investigate that
    // named Ranges also? e.g. [MyRange] if so need a list of named ranges
    uno::Any aVoid;
    return uno::Any( Range( uno::Any( Name ), aVoid ) );
}

uno::Reference< beans::XIntrospectionAccess > SAL_CALL
ScVbaWorksheet::getIntrospection(  )
{
    return uno::Reference< beans::XIntrospectionAccess >();
}

uno::Any SAL_CALL
ScVbaWorksheet::invoke( const OUString& /*aFunctionName*/, const uno::Sequence< uno::Any >& /*aParams*/, uno::Sequence< ::sal_Int16 >& /*aOutParamIndex*/, uno::Sequence< uno::Any >& /*aOutParam*/ )
{
    throw uno::RuntimeException(u"Unsupported"_ustr); // unsupported operation
}

void SAL_CALL
ScVbaWorksheet::setValue( const OUString& aPropertyName, const uno::Any& aValue )
{
    setDefaultPropByIntrospection( getValue( aPropertyName ), aValue );
}
uno::Any SAL_CALL
ScVbaWorksheet::getValue( const OUString& aPropertyName )
{
    uno::Reference< drawing::XControlShape > xControlShape( getControlShape( aPropertyName ), uno::UNO_QUERY_THROW );

    uno::Reference<lang::XMultiComponentFactory > xServiceManager( mxContext->getServiceManager(), uno::UNO_SET_THROW );
    uno::Reference< XControlProvider > xControlProvider( xServiceManager->createInstanceWithContext(u"ooo.vba.ControlProvider"_ustr, mxContext ), uno::UNO_QUERY_THROW );
    uno::Reference< msforms::XControl > xControl( xControlProvider->createControl(  xControlShape, getModel() ) );
    return uno::Any( xControl );
}

sal_Bool SAL_CALL
ScVbaWorksheet::hasMethod( const OUString& /*aName*/ )
{
    return false;
}

uno::Reference< container::XNameAccess >
ScVbaWorksheet::getFormControls() const
{
    uno::Reference< container::XNameAccess > xFormControls;
    try
    {
        uno::Reference< sheet::XSpreadsheet > xSpreadsheet( getSheet(), uno::UNO_SET_THROW );
        uno::Reference< drawing::XDrawPageSupplier > xDrawPageSupplier( xSpreadsheet, uno::UNO_QUERY_THROW );
        uno::Reference< form::XFormsSupplier >  xFormSupplier( xDrawPageSupplier->getDrawPage(), uno::UNO_QUERY_THROW );
        uno::Reference< container::XIndexAccess > xIndexAccess( xFormSupplier->getForms(), uno::UNO_QUERY_THROW );
        // get the www-standard container ( maybe we should access the
        // 'www-standard' by name rather than index, this seems an
        // implementation detail
        if( xIndexAccess->hasElements() )
            xFormControls.set( xIndexAccess->getByIndex(0), uno::UNO_QUERY );

    }
    catch( uno::Exception& )
    {
    }
    return xFormControls;

                }
sal_Bool SAL_CALL
ScVbaWorksheet::hasProperty( const OUString& aName )
{
    uno::Reference< container::XNameAccess > xFormControls( getFormControls() );
    if ( xFormControls.is() )
        return xFormControls->hasByName( aName );
    return false;
}

uno::Any
ScVbaWorksheet::getControlShape( std::u16string_view sName )
{
    // ideally we would get an XControl object but it appears an XControl
    // implementation only exists for a Control implementation obtained from the
    // view ( e.g. in basic you would get this from
    // thiscomponent.currentcontroller.getControl( controlModel ) )
    // and the thing to realise is that it is only possible to get an XControl
    // for a currently displayed control :-( often we would want to modify
    // a control not on the active sheet. But... you can always access the
    // XControlShape from the DrawPage whether that is the active drawpage or not

    uno::Reference< drawing::XDrawPageSupplier > xDrawPageSupplier( getSheet(), uno::UNO_QUERY_THROW );
    uno::Reference< container::XIndexAccess > xIndexAccess( xDrawPageSupplier->getDrawPage(), uno::UNO_QUERY_THROW );

    sal_Int32 nCount = xIndexAccess->getCount();
    for( int index = 0; index < nCount; index++ )
    {
        uno::Any aUnoObj =  xIndexAccess->getByIndex( index );
         // It seems there are some drawing objects that can not query into Control shapes?
        uno::Reference< drawing::XControlShape > xControlShape( aUnoObj, uno::UNO_QUERY );
        if( xControlShape.is() )
        {
            uno::Reference< container::XNamed > xNamed( xControlShape->getControl(), uno::UNO_QUERY_THROW );
            if( sName == xNamed->getName() )
            {
                return aUnoObj;
            }
        }
    }
    return uno::Any();
}

OUString
ScVbaWorksheet::getServiceImplName()
{
    return u"ScVbaWorksheet"_ustr;
}

void SAL_CALL
ScVbaWorksheet::setEnableCalculation( sal_Bool bEnableCalculation )
{
    uno::Reference <sheet::XCalculatable> xCalculatable(getModel(), uno::UNO_QUERY_THROW);
    xCalculatable->enableAutomaticCalculation( bEnableCalculation);
}
sal_Bool SAL_CALL
ScVbaWorksheet::getEnableCalculation(  )
{
    uno::Reference <sheet::XCalculatable> xCalculatable(getModel(), uno::UNO_QUERY_THROW);
    return xCalculatable->isAutomaticCalculationEnabled();
}

uno::Sequence< OUString >
ScVbaWorksheet::getServiceNames()
{
    static uno::Sequence< OUString > const aServiceNames
    {
        u"ooo.vba.excel.Worksheet"_ustr
    };
    return aServiceNames;
}

OUString SAL_CALL
ScVbaWorksheet::getCodeName()
{
    uno::Reference< beans::XPropertySet > xSheetProp( mxSheet, uno::UNO_QUERY_THROW );
    return xSheetProp->getPropertyValue(u"CodeName"_ustr).get< OUString >();
}

sal_Int16
ScVbaWorksheet::getSheetID() const
{
    uno::Reference< sheet::XCellRangeAddressable > xAddressable( mxSheet, uno::UNO_QUERY_THROW ); // if ActiveSheet, mxSheet is null.
    return xAddressable->getRangeAddress().Sheet;
}

void SAL_CALL
ScVbaWorksheet::PrintOut( const uno::Any& From, const uno::Any& To, const uno::Any& Copies, const uno::Any& Preview, const uno::Any& ActivePrinter, const uno::Any& PrintToFile, const uno::Any& Collate, const uno::Any& PrToFileName, const uno::Any& )
{
    sal_Int32 nTo = 0;
    sal_Int32 nFrom = 0;
    bool bSelection = false;
    From >>= nFrom;
    To >>= nTo;

    if ( !( nFrom || nTo ) )
        bSelection = true;

    uno::Reference< frame::XModel > xModel( getModel(), uno::UNO_SET_THROW );
    PrintOutHelper( excel::getBestViewShell( xModel ), From, To, Copies, Preview, ActivePrinter, PrintToFile, Collate, PrToFileName, bSelection );
}

void SAL_CALL
ScVbaWorksheet::ExportAsFixedFormat(const css::uno::Any& Type, const css::uno::Any& FileName, const css::uno::Any& Quality,
    const css::uno::Any& IncludeDocProperties, const css::uno::Any& /*IgnorePrintAreas*/, const css::uno::Any& From,
    const css::uno::Any& To, const css::uno::Any& OpenAfterPublish, const css::uno::Any& /*FixedFormatExtClassPtr*/)
{
    uno::Reference< frame::XModel > xModel(getModel(), uno::UNO_SET_THROW);
    uno::Reference< excel::XApplication > xApplication(Application(), uno::UNO_QUERY_THROW);

    excel::ExportAsFixedFormatHelper(xModel, xApplication, Type, FileName, Quality,
        IncludeDocProperties, From, To, OpenAfterPublish);
}

sal_Int64 SAL_CALL
ScVbaWorksheet::getSomething(const uno::Sequence<sal_Int8 > & rId)
{
    return comphelper::getSomethingImpl(rId, this);
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
Calc_ScVbaWorksheet_get_implementation(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const& args)
{
    return cppu::acquire(new ScVbaWorksheet(args, context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
