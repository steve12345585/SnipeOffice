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

#include <memory>
#include <osl/diagnose.h>
#include <unotools/charclass.hxx>
#include <rangeutl.hxx>
#include <document.hxx>
#include <global.hxx>
#include <dbdata.hxx>
#include <rangenam.hxx>
#include <convuno.hxx>
#include <externalrefmgr.hxx>
#include <compiler.hxx>
#include <refupdatecontext.hxx>

using ::formula::FormulaGrammar;
using namespace ::com::sun::star;

bool ScRangeUtil::MakeArea( const OUString&   rAreaStr,
                            ScArea&         rArea,
                            const ScDocument& rDoc,
                            SCTAB           nTab,
                            ScAddress::Details const & rDetails )
{
    // Input in rAreaStr: "$Tabelle1.$A1:$D17"

    // BROKEN BROKEN BROKEN
    // but it is only used in the consolidate dialog.  Ignore for now.

    bool        bSuccess    = false;
    sal_Int32       nPointPos   = rAreaStr.indexOf('.');
    sal_Int32       nColonPos   = rAreaStr.indexOf(':');
    OUString      aStrArea( rAreaStr );
    ScRefAddress    startPos;
    ScRefAddress    endPos;

    if ( nColonPos == -1  && nPointPos != -1 )
    {
        aStrArea += OUString::Concat(":") + rAreaStr.subView( nPointPos+1 ); // do not include '.' in copy
    }

    bSuccess = ConvertDoubleRef( rDoc, aStrArea, nTab, startPos, endPos, rDetails );

    if ( bSuccess )
        rArea = ScArea( startPos.Tab(),
                        startPos.Col(), startPos.Row(),
                        endPos.Col(),   endPos.Row() );

    return bSuccess;
}

void ScRangeUtil::CutPosString( const OUString&   theAreaStr,
                                OUString&         thePosStr )
{
    OUString  aPosStr;
    // BROKEN BROKEN BROKEN
    // but it is only used in the consolidate dialog.  Ignore for now.

    sal_Int32  nColonPos = theAreaStr.indexOf(':');

    if ( nColonPos != -1 )
        aPosStr = theAreaStr.copy( 0, nColonPos ); // do not include ':' in copy
    else
        aPosStr = theAreaStr;

    thePosStr = aPosStr;
}

bool ScRangeUtil::IsAbsTabArea( const OUString&   rAreaStr,
                                const ScDocument* pDoc,
                                std::unique_ptr<ScArea[]>*  ppAreas,
                                sal_uInt16*         pAreaCount,
                                bool            /* bAcceptCellRef */,
                                ScAddress::Details const & rDetails )
{
    OSL_ENSURE( pDoc, "No document given!" );
    if ( !pDoc )
        return false;

    // BROKEN BROKEN BROKEN
    // but it is only used in the consolidate dialog.  Ignore for now.

    /*
     * Expects strings like:
     *      "$Tabelle1.$A$1:$Tabelle3.$D$17"
     * If bAcceptCellRef == sal_True then also accept strings like:
     *      "$Tabelle1.$A$1"
     *
     * as result a ScArea-Array is created,
     * which is published via ppAreas and also has to be deleted this route.
     */

    bool    bStrOk = false;
    OUString  aTempAreaStr(rAreaStr);

    if ( -1 == aTempAreaStr.indexOf(':') )
    {
        aTempAreaStr += ":" + rAreaStr;
    }

    sal_Int32   nColonPos = aTempAreaStr.indexOf(':');

    if (   -1 != nColonPos
        && -1 != aTempAreaStr.indexOf('.') )
    {
        ScRefAddress    aStartPos;

        OUString aStartPosStr = aTempAreaStr.copy( 0,           nColonPos  );
        OUString aEndPosStr   = aTempAreaStr.copy( nColonPos+1 );

        if ( ConvertSingleRef( *pDoc, aStartPosStr, 0, aStartPos, rDetails ) )
        {
            ScRefAddress aEndPos;
            if ( ConvertSingleRef( *pDoc, aEndPosStr, aStartPos.Tab(), aEndPos, rDetails ) )
            {
                aStartPos.SetRelCol( false );
                aStartPos.SetRelRow( false );
                aStartPos.SetRelTab( false );
                aEndPos.SetRelCol( false );
                aEndPos.SetRelRow( false );
                aEndPos.SetRelTab( false );

                bStrOk = true;

                if ( ppAreas && pAreaCount ) // Array returned ?
                {
                    SCTAB       nStartTab   = aStartPos.Tab();
                    SCTAB       nEndTab     = aEndPos.Tab();
                    sal_uInt16      nTabCount   = static_cast<sal_uInt16>(nEndTab-nStartTab+1);
                    ppAreas->reset(new ScArea[nTabCount]);
                    SCTAB       nTab        = 0;
                    sal_uInt16      i           = 0;
                    ScArea      theArea( 0, aStartPos.Col(), aStartPos.Row(),
                                            aEndPos.Col(), aEndPos.Row() );

                    nTab = nStartTab;
                    for ( i=0; i<nTabCount; i++ )
                    {
                        (*ppAreas)[i] = theArea;
                        (*ppAreas)[i].nTab = nTab;
                        nTab++;
                    }
                    *pAreaCount = nTabCount;
                }
            }
        }
    }

    return bStrOk;
}

bool ScRangeUtil::IsAbsArea( const OUString&  rAreaStr,
                             const ScDocument& rDoc,
                             SCTAB          nTab,
                             OUString*      pCompleteStr,
                             ScRefAddress*  pStartPos,
                             ScRefAddress*  pEndPos,
                             ScAddress::Details const & rDetails )
{
    ScRefAddress    startPos;
    ScRefAddress    endPos;

    bool bIsAbsArea = ConvertDoubleRef( rDoc, rAreaStr, nTab, startPos, endPos, rDetails );

    if ( bIsAbsArea )
    {
        startPos.SetRelCol( false );
        startPos.SetRelRow( false );
        startPos.SetRelTab( false );
        endPos  .SetRelCol( false );
        endPos  .SetRelRow( false );
        endPos  .SetRelTab( false );

        if ( pCompleteStr )
        {
            *pCompleteStr  = startPos.GetRefString( rDoc, MAXTAB+1, rDetails );
            *pCompleteStr += ":";
            *pCompleteStr += endPos.GetRefString( rDoc, nTab, rDetails );
        }

        if ( pStartPos && pEndPos )
        {
            *pStartPos = startPos;
            *pEndPos   = endPos;
        }
    }

    return bIsAbsArea;
}

bool ScRangeUtil::IsAbsPos( const OUString&   rPosStr,
                            const ScDocument& rDoc,
                            SCTAB           nTab,
                            OUString*       pCompleteStr,
                            ScRefAddress*   pPosTripel,
                            ScAddress::Details const & rDetails )
{
    ScRefAddress    thePos;

    bool bIsAbsPos = ConvertSingleRef( rDoc, rPosStr, nTab, thePos, rDetails );
    thePos.SetRelCol( false );
    thePos.SetRelRow( false );
    thePos.SetRelTab( false );

    if ( bIsAbsPos )
    {
        if ( pPosTripel )
            *pPosTripel = thePos;
        if ( pCompleteStr )
            *pCompleteStr = thePos.GetRefString( rDoc, MAXTAB+1, rDetails );
    }

    return bIsAbsPos;
}

bool ScRangeUtil::MakeRangeFromName (
    const OUString& rName,
    const ScDocument& rDoc,
    SCTAB           nCurTab,
    ScRange&        rRange,
    RutlNameScope   eScope,
    ScAddress::Details const & rDetails,
    bool bUseDetailsPos )
{
    bool bResult = false;
    if (rName.isEmpty())
        return bResult;

    SCTAB nTab = 0;
    SCCOL nColStart = 0;
    SCCOL nColEnd = 0;
    SCROW nRowStart = 0;
    SCROW nRowEnd = 0;

    if (eScope == RUTL_NAMES || eScope == RUTL_NAMES_LOCAL || eScope == RUTL_NAMES_GLOBAL)
    {
        OUString aName(rName);
        SCTAB nTable = nCurTab;

        if (eScope != RUTL_NAMES_GLOBAL)
        {
            // First handle UI names like "local1 (Sheet1)", which point to a
            // local range name.
            const sal_Int32 nEndPos = aName.getLength() - 1;
            if (rName[nEndPos] == ')')
            {
                const sal_Int32 nStartPos = aName.indexOf(" (");
                if (nStartPos != -1)
                {
                    OUString aSheetName = aName.copy(nStartPos+2, nEndPos-nStartPos-2);
                    if (rDoc.GetTable(aSheetName, nTable))
                    {
                        aName = aName.copy(0, nStartPos);
                        eScope = RUTL_NAMES_LOCAL;
                    }
                    else
                        nTable = nCurTab;
                }
            }
        }

        aName = ScGlobal::getCharClass().uppercase(aName);
        ScRangeData* pData = nullptr;
        if (eScope != RUTL_NAMES_GLOBAL)
        {
            // Check for local range names.
            ScRangeName* pRangeNames = rDoc.GetRangeName( nTable );
            if ( pRangeNames )
                pData = pRangeNames->findByUpperName(aName);
        }
        if (!pData && eScope != RUTL_NAMES_LOCAL)
            pData = rDoc.GetRangeName()->findByUpperName(aName);
        if (pData)
        {
            OUString         aStrArea;
            ScRefAddress     aStartPos;
            ScRefAddress     aEndPos;

            // tdf#138646: use the current grammar of the document and passed
            // address convention.
            // tdf#145077: create range string according to current cell cursor
            // position if expression has relative references and details say so.
            if (bUseDetailsPos)
                aStrArea = pData->GetSymbol( ScAddress( rDetails.nCol, rDetails.nRow, nCurTab),
                        FormulaGrammar::mergeToGrammar(rDoc.GetGrammar(), rDetails.eConv));
            else
                aStrArea = pData->GetSymbol(
                        FormulaGrammar::mergeToGrammar(rDoc.GetGrammar(), rDetails.eConv));

            if ( IsAbsArea( aStrArea, rDoc, nTable,
                            nullptr, &aStartPos, &aEndPos, rDetails ) )
            {
                nTab       = aStartPos.Tab();
                nColStart  = aStartPos.Col();
                nRowStart  = aStartPos.Row();
                nColEnd    = aEndPos.Col();
                nRowEnd    = aEndPos.Row();
                bResult    = true;
            }
            else
            {
                CutPosString( aStrArea, aStrArea );

                if ( IsAbsPos( aStrArea, rDoc, nTable,
                                          nullptr, &aStartPos, rDetails ) )
                {
                    nTab       = aStartPos.Tab();
                    nColStart  = nColEnd = aStartPos.Col();
                    nRowStart  = nRowEnd = aStartPos.Row();
                    bResult    = true;
                }
            }
        }
    }
    else if( eScope==RUTL_DBASE )
    {
        ScDBCollection::NamedDBs& rDbNames = rDoc.GetDBCollection()->getNamedDBs();
        ScDBData* pData = rDbNames.findByUpperName(ScGlobal::getCharClass().uppercase(rName));
        if (pData)
        {
            pData->GetArea(nTab, nColStart, nRowStart, nColEnd, nRowEnd);
            bResult = true;
        }
    }
    else
    {
        OSL_FAIL( "ScRangeUtil::MakeRangeFromName" );
    }

    if( bResult )
    {
        rRange = ScRange( nColStart, nRowStart, nTab, nColEnd, nRowEnd, nTab );
    }

    return bResult;
}

void ScRangeStringConverter::AssignString(
        OUString& rString,
        const OUString& rNewStr,
        bool bAppendStr,
        sal_Unicode cSeparator)
{
    if( bAppendStr )
    {
        if( !rNewStr.isEmpty() )
        {
            if( !rString.isEmpty() )
                rString += OUStringChar(cSeparator);
            rString += rNewStr;
        }
    }
    else
        rString = rNewStr;
}

sal_Int32 ScRangeStringConverter::IndexOf(
        std::u16string_view rString,
        sal_Unicode cSearchChar,
        sal_Int32 nOffset,
        sal_Unicode cQuote )
{
    sal_Int32       nLength     = rString.size();
    sal_Int32       nIndex      = nOffset;
    bool            bQuoted     = false;
    bool        bExitLoop   = false;

    while( !bExitLoop && (nIndex >= 0 && nIndex < nLength) )
    {
        sal_Unicode cCode = rString[ nIndex ];
        bExitLoop = (cCode == cSearchChar) && !bQuoted;
        bQuoted = (bQuoted != (cCode == cQuote));
        if( !bExitLoop )
            nIndex++;
    }
    return (nIndex < nLength) ? nIndex : -1;
}

sal_Int32 ScRangeStringConverter::IndexOfDifferent(
        std::u16string_view rString,
        sal_Unicode cSearchChar,
        sal_Int32 nOffset )
{
    sal_Int32       nLength     = rString.size();
    sal_Int32       nIndex      = nOffset;
    bool        bExitLoop   = false;

    while( !bExitLoop && (nIndex >= 0 && nIndex < nLength) )
    {
        bExitLoop = (rString[ nIndex ] != cSearchChar);
        if( !bExitLoop )
            nIndex++;
    }
    return (nIndex < nLength) ? nIndex : -1;
}

void ScRangeStringConverter::GetTokenByOffset(
        OUString& rToken,
        std::u16string_view rString,
        sal_Int32& nOffset,
        sal_Unicode cSeparator,
        sal_Unicode cQuote)
{
    sal_Int32 nLength = rString.size();
    if( nOffset == -1 || nOffset >= nLength )
    {
        rToken.clear();
        nOffset = -1;
    }
    else
    {
        sal_Int32 nTokenEnd = IndexOf( rString, cSeparator, nOffset, cQuote );
        if( nTokenEnd < 0 )
            nTokenEnd = nLength;
        rToken = rString.substr( nOffset, nTokenEnd - nOffset );

        sal_Int32 nNextBegin = IndexOfDifferent( rString, cSeparator, nTokenEnd );
        nOffset = (nNextBegin < 0) ? nLength : nNextBegin;
    }
}

void ScRangeStringConverter::AppendTableName(OUStringBuffer& rBuf, const OUString& rTabName)
{
    // quote character is always "'"
    OUString aQuotedTab(rTabName);
    ScCompiler::CheckTabQuotes(aQuotedTab);
    rBuf.append(aQuotedTab);
}

sal_Int32 ScRangeStringConverter::GetTokenCount( std::u16string_view rString, sal_Unicode cSeparator )
{
    OUString    sToken;
    sal_Int32   nCount = 0;
    sal_Int32   nOffset = 0;
    while( nOffset >= 0 )
    {
        GetTokenByOffset( sToken, rString, nOffset, '\'', cSeparator );
        if( nOffset >= 0 )
            nCount++;
    }
    return nCount;
}

bool ScRangeStringConverter::GetAddressFromString(
        ScAddress& rAddress,
        std::u16string_view rAddressStr,
        const ScDocument& rDocument,
        FormulaGrammar::AddressConvention eConv,
        sal_Int32& nOffset,
        sal_Unicode cSeparator,
        sal_Unicode cQuote )
{
    OUString sToken;
    GetTokenByOffset( sToken, rAddressStr, nOffset, cSeparator, cQuote );
    if( nOffset >= 0 )
    {
        if ((rAddress.Parse( sToken, rDocument, eConv ) & ScRefFlags::VALID) == ScRefFlags::VALID)
            return true;
        ::formula::FormulaGrammar::AddressConvention eConvUI = rDocument.GetAddressConvention();
        if (eConv != eConvUI)
            return ((rAddress.Parse(sToken, rDocument, eConvUI) & ScRefFlags::VALID) == ScRefFlags::VALID);
    }
    return false;
}

bool ScRangeStringConverter::GetRangeFromString(
        ScRange& rRange,
        std::u16string_view rRangeStr,
        const ScDocument& rDocument,
        FormulaGrammar::AddressConvention eConv,
        sal_Int32& nOffset,
        sal_Unicode cSeparator,
        sal_Unicode cQuote )
{
    OUString sToken;
    bool bResult(false);
    GetTokenByOffset( sToken, rRangeStr, nOffset, cSeparator, cQuote );
    if( nOffset >= 0 )
    {
        sal_Int32 nIndex = IndexOf( sToken, ':', 0, cQuote );
        OUString aUIString(sToken);

        if( nIndex < 0 )
        {
            if ( aUIString[0] == '.' )
                aUIString = aUIString.copy( 1 );
            bResult = (rRange.aStart.Parse( aUIString, rDocument, eConv) & ScRefFlags::VALID) ==
                                                                                                     ScRefFlags::VALID;
            ::formula::FormulaGrammar::AddressConvention eConvUI = rDocument.GetAddressConvention();
            if (!bResult && eConv != eConvUI)
                bResult = (rRange.aStart.Parse(aUIString, rDocument, eConvUI) & ScRefFlags::VALID) ==
                                                                                                         ScRefFlags::VALID;
            rRange.aEnd = rRange.aStart;
        }
        else
        {
            if ( aUIString[0] == '.' )
            {
                aUIString = aUIString.copy( 1 );
                --nIndex;
            }

            if ( nIndex < aUIString.getLength() - 1 &&
                    aUIString[ nIndex + 1 ] == '.' )
                aUIString = aUIString.replaceAt( nIndex + 1, 1, u"" );

            bResult = ((rRange.Parse(aUIString, rDocument, eConv) & ScRefFlags::VALID) ==
                                                                                              ScRefFlags::VALID);

            // #i77703# chart ranges in the file format contain both sheet names, even for an external reference sheet.
            // This isn't parsed by ScRange, so try to parse the two Addresses then.
            if (!bResult)
            {
                bResult = ((rRange.aStart.Parse( aUIString.copy(0, nIndex), rDocument, eConv)
                               & ScRefFlags::VALID) == ScRefFlags::VALID)
                          &&
                          ((rRange.aEnd.Parse( aUIString.copy(nIndex+1), rDocument, eConv)
                               & ScRefFlags::VALID) == ScRefFlags::VALID);

                ::formula::FormulaGrammar::AddressConvention eConvUI = rDocument.GetAddressConvention();
                if (!bResult && eConv != eConvUI)
                {
                    bResult = ((rRange.aStart.Parse( aUIString.copy(0, nIndex), rDocument, eConvUI)
                                   & ScRefFlags::VALID) == ScRefFlags::VALID)
                              &&
                              ((rRange.aEnd.Parse( aUIString.copy(nIndex+1), rDocument, eConvUI)
                                   & ScRefFlags::VALID) == ScRefFlags::VALID);
                }
            }
        }
    }
    return bResult;
}

bool ScRangeStringConverter::GetRangeListFromString(
        ScRangeList& rRangeList,
        std::u16string_view rRangeListStr,
        const ScDocument& rDocument,
        FormulaGrammar::AddressConvention eConv,
        sal_Unicode cSeparator,
        sal_Unicode cQuote )
{
    bool bRet = true;
    OSL_ENSURE( !rRangeListStr.empty(), "ScXMLConverter::GetRangeListFromString - empty string!" );
    sal_Int32 nOffset = 0;
    while( nOffset >= 0 )
    {
        ScRange aRange;
        if (
             GetRangeFromString( aRange, rRangeListStr, rDocument, eConv, nOffset, cSeparator, cQuote ) &&
             (nOffset >= 0)
           )
        {
            rRangeList.push_back( aRange );
        }
        else if (nOffset > -1)
            bRet = false;
    }
    return bRet;
}

bool ScRangeStringConverter::GetAreaFromString(
        ScArea& rArea,
        std::u16string_view rRangeStr,
        const ScDocument& rDocument,
        FormulaGrammar::AddressConvention eConv,
        sal_Int32& nOffset,
        sal_Unicode cSeparator )
{
    ScRange aScRange;
    bool bResult(false);
    if( GetRangeFromString( aScRange, rRangeStr, rDocument, eConv, nOffset, cSeparator ) && (nOffset >= 0) )
    {
        rArea.nTab = aScRange.aStart.Tab();
        rArea.nColStart = aScRange.aStart.Col();
        rArea.nRowStart = aScRange.aStart.Row();
        rArea.nColEnd = aScRange.aEnd.Col();
        rArea.nRowEnd = aScRange.aEnd.Row();
        bResult = true;
    }
    return bResult;
}

bool ScRangeStringConverter::GetRangeFromString(
        table::CellRangeAddress& rRange,
        std::u16string_view rRangeStr,
        const ScDocument& rDocument,
        FormulaGrammar::AddressConvention eConv,
        sal_Int32& nOffset,
        sal_Unicode cSeparator )
{
    ScRange aScRange;
    bool bResult(false);
    if( GetRangeFromString( aScRange, rRangeStr, rDocument, eConv, nOffset, cSeparator ) && (nOffset >= 0) )
    {
        ScUnoConversion::FillApiRange( rRange, aScRange );
        bResult = true;
    }
    return bResult;
}

void ScRangeStringConverter::GetStringFromAddress(
        OUString& rString,
        const ScAddress& rAddress,
        const ScDocument* pDocument,
        FormulaGrammar::AddressConvention eConv,
        sal_Unicode cSeparator,
        bool bAppendStr,
        ScRefFlags nFormatFlags )
{
    if (pDocument && pDocument->HasTable(rAddress.Tab()))
    {
        OUString sAddress(rAddress.Format(nFormatFlags, pDocument, eConv));
        AssignString( rString, sAddress, bAppendStr, cSeparator );
    }
}

void ScRangeStringConverter::GetStringFromRange(
        OUString& rString,
        const ScRange& rRange,
        const ScDocument* pDocument,
        FormulaGrammar::AddressConvention eConv,
        sal_Unicode cSeparator,
        bool bAppendStr,
        ScRefFlags nFormatFlags )
{
    if (pDocument && pDocument->HasTable(rRange.aStart.Tab()))
    {
        ScAddress aStartAddress( rRange.aStart );
        ScAddress aEndAddress( rRange.aEnd );
        OUString sStartAddress(aStartAddress.Format(nFormatFlags, pDocument, eConv));
        OUString sEndAddress(aEndAddress.Format(nFormatFlags, pDocument, eConv));
        AssignString(
            rString, sStartAddress + ":" + sEndAddress, bAppendStr, cSeparator);
    }
}

void ScRangeStringConverter::GetStringFromRangeList(
        OUString& rString,
        const ScRangeList* pRangeList,
        const ScDocument* pDocument,
        FormulaGrammar::AddressConvention eConv,
        sal_Unicode cSeparator )
{
    OUString sRangeListStr;
    if( pRangeList )
    {
        for( size_t nIndex = 0, nCount = pRangeList->size(); nIndex < nCount; nIndex++ )
        {
            const ScRange & rRange = (*pRangeList)[nIndex];
            GetStringFromRange( sRangeListStr, rRange, pDocument, eConv, cSeparator, true );
        }
    }
    rString = sRangeListStr;
}

void ScRangeStringConverter::GetStringFromArea(
        OUString& rString,
        const ScArea& rArea,
        const ScDocument* pDocument,
        FormulaGrammar::AddressConvention eConv,
        sal_Unicode cSeparator,
        bool bAppendStr,
        ScRefFlags nFormatFlags )
{
    ScRange aRange( rArea.nColStart, rArea.nRowStart, rArea.nTab, rArea.nColEnd, rArea.nRowEnd, rArea.nTab );
    GetStringFromRange( rString, aRange, pDocument, eConv, cSeparator, bAppendStr, nFormatFlags );
}

void ScRangeStringConverter::GetStringFromAddress(
        OUString& rString,
        const table::CellAddress& rAddress,
        const ScDocument* pDocument,
        FormulaGrammar::AddressConvention eConv,
        sal_Unicode cSeparator,
        bool bAppendStr )
{
    ScAddress aScAddress( static_cast<SCCOL>(rAddress.Column), static_cast<SCROW>(rAddress.Row), rAddress.Sheet );
    GetStringFromAddress( rString, aScAddress, pDocument, eConv, cSeparator, bAppendStr );
}

void ScRangeStringConverter::GetStringFromRange(
        OUString& rString,
        const table::CellRangeAddress& rRange,
        const ScDocument* pDocument,
        FormulaGrammar::AddressConvention eConv,
        sal_Unicode cSeparator,
        bool bAppendStr,
        ScRefFlags nFormatFlags )
{
    ScRange aScRange( static_cast<SCCOL>(rRange.StartColumn), static_cast<SCROW>(rRange.StartRow), rRange.Sheet,
        static_cast<SCCOL>(rRange.EndColumn), static_cast<SCROW>(rRange.EndRow), rRange.Sheet );
    GetStringFromRange( rString, aScRange, pDocument, eConv, cSeparator, bAppendStr, nFormatFlags );
}

void ScRangeStringConverter::GetStringFromRangeList(
        OUString& rString,
        const uno::Sequence< table::CellRangeAddress >& rRangeSeq,
        const ScDocument* pDocument,
        FormulaGrammar::AddressConvention eConv,
        sal_Unicode cSeparator )
{
    OUString sRangeListStr;
    for( const table::CellRangeAddress& rRange : rRangeSeq )
    {
        GetStringFromRange( sRangeListStr, rRange, pDocument, eConv, cSeparator, true );
    }
    rString = sRangeListStr;
}

static void lcl_appendCellAddress(
    OUStringBuffer& rBuf, const ScDocument& rDoc, const ScAddress& rCell,
    const ScAddress::ExternalInfo& rExtInfo)
{
    if (rExtInfo.mbExternal)
    {
        ScExternalRefManager* pRefMgr = rDoc.GetExternalRefManager();
        const OUString* pFilePath = pRefMgr->getExternalFileName(rExtInfo.mnFileId, true);
        if (!pFilePath)
            return;

        sal_Unicode cQuote = '\'';
        rBuf.append(cQuote);
        rBuf.append(*pFilePath);
        rBuf.append(cQuote);
        rBuf.append('#');
        rBuf.append('$');
        ScRangeStringConverter::AppendTableName(rBuf, rExtInfo.maTabName);
        rBuf.append('.');

        OUString aAddr(rCell.Format(ScRefFlags::ADDR_ABS, nullptr, rDoc.GetAddressConvention()));
        rBuf.append(aAddr);
    }
    else
    {
        OUString aAddr(rCell.Format(ScRefFlags::ADDR_ABS_3D, &rDoc, rDoc.GetAddressConvention()));
        rBuf.append(aAddr);
    }
}

static void lcl_appendCellRangeAddress(
    OUStringBuffer& rBuf, const ScDocument& rDoc, const ScAddress& rCell1, const ScAddress& rCell2,
    const ScAddress::ExternalInfo& rExtInfo1, const ScAddress::ExternalInfo& rExtInfo2)
{
    if (rExtInfo1.mbExternal)
    {
        OSL_ENSURE(rExtInfo2.mbExternal, "2nd address is not external!?");
        OSL_ENSURE(rExtInfo1.mnFileId == rExtInfo2.mnFileId, "File IDs do not match between 1st and 2nd addresses.");

        ScExternalRefManager* pRefMgr = rDoc.GetExternalRefManager();
        const OUString* pFilePath = pRefMgr->getExternalFileName(rExtInfo1.mnFileId, true);
        if (!pFilePath)
            return;

        sal_Unicode cQuote = '\'';
        rBuf.append(cQuote);
        rBuf.append(*pFilePath);
        rBuf.append(cQuote);
        rBuf.append('#');
        rBuf.append('$');
        ScRangeStringConverter::AppendTableName(rBuf, rExtInfo1.maTabName);
        rBuf.append('.');

        OUString aAddr(rCell1.Format(ScRefFlags::ADDR_ABS, nullptr, rDoc.GetAddressConvention()));
        rBuf.append(aAddr);

        rBuf.append(":");

        if (rExtInfo1.maTabName != rExtInfo2.maTabName)
        {
            rBuf.append('$');
            ScRangeStringConverter::AppendTableName(rBuf, rExtInfo2.maTabName);
            rBuf.append('.');
        }

        aAddr = rCell2.Format(ScRefFlags::ADDR_ABS, nullptr, rDoc.GetAddressConvention());
        rBuf.append(aAddr);
    }
    else
    {
        ScRange aRange;
        aRange.aStart = rCell1;
        aRange.aEnd   = rCell2;
        OUString aAddr(aRange.Format(rDoc, ScRefFlags::RANGE_ABS_3D, rDoc.GetAddressConvention()));
        rBuf.append(aAddr);
    }
}

void ScRangeStringConverter::GetStringFromXMLRangeString( OUString& rString, std::u16string_view rXMLRange, const ScDocument& rDoc )
{
    FormulaGrammar::AddressConvention eConv = rDoc.GetAddressConvention();
    const sal_Unicode cSepNew = ScCompiler::GetNativeSymbolChar(ocSep);

    OUStringBuffer aRetStr;
    sal_Int32 nOffset = 0;
    bool bFirst = true;

    while (nOffset >= 0)
    {
        OUString aToken;
        GetTokenByOffset(aToken, rXMLRange, nOffset);
        if (nOffset < 0)
            break;

        sal_Int32 nSepPos = IndexOf(aToken, ':', 0);
        if (nSepPos >= 0)
        {
            // Cell range
            OUString aBeginCell = aToken.copy(0, nSepPos);
            OUString aEndCell   = aToken.copy(nSepPos+1);

            if (aBeginCell.isEmpty() || aEndCell.isEmpty())
                // both cell addresses must exist for this to work.
                continue;

            sal_Int32 nEndCellDotPos = aEndCell.indexOf('.');
            if (nEndCellDotPos <= 0)
            {
                // initialize buffer with table name...
                sal_Int32 nDotPos = IndexOf(aBeginCell, '.', 0);
                OUStringBuffer aBuf(aBeginCell.subView(0, nDotPos));

                if (nEndCellDotPos == 0)
                {
                    // workaround for old syntax (probably pre-chart2 age?)
                    // e.g. Sheet1.A1:.B2
                    aBuf.append(aEndCell);
                }
                else if (nEndCellDotPos < 0)
                {
                    // sheet name in the end cell is omitted (e.g. Sheet2.A1:B2).
                    aBuf.append("." + aEndCell);
                }
                aEndCell = aBuf.makeStringAndClear();
            }

            ScAddress::ExternalInfo aExtInfo1, aExtInfo2;
            ScAddress aCell1, aCell2;
            ScRefFlags nRet = aCell1.Parse(aBeginCell, rDoc, FormulaGrammar::CONV_OOO, &aExtInfo1);
            if ((nRet & ScRefFlags::VALID) == ScRefFlags::ZERO)
            {
                // first cell is invalid.
                if (eConv == FormulaGrammar::CONV_OOO)
                    continue;

                nRet = aCell1.Parse(aBeginCell, rDoc, eConv, &aExtInfo1);
                if ((nRet & ScRefFlags::VALID) == ScRefFlags::ZERO)
                    // first cell is really invalid.
                    continue;
            }

            nRet = aCell2.Parse(aEndCell, rDoc, FormulaGrammar::CONV_OOO, &aExtInfo2);
            if ((nRet & ScRefFlags::VALID) == ScRefFlags::ZERO)
            {
                // second cell is invalid.
                if (eConv == FormulaGrammar::CONV_OOO)
                    continue;

                nRet = aCell2.Parse(aEndCell, rDoc, eConv, &aExtInfo2);
                if ((nRet & ScRefFlags::VALID) == ScRefFlags::ZERO)
                    // second cell is really invalid.
                    continue;
            }

            if (aExtInfo1.mnFileId != aExtInfo2.mnFileId || aExtInfo1.mbExternal != aExtInfo2.mbExternal)
                // external info inconsistency.
                continue;

            // All looks good!

            if (bFirst)
                bFirst = false;
            else
                aRetStr.append(cSepNew);

            lcl_appendCellRangeAddress(aRetStr, rDoc, aCell1, aCell2, aExtInfo1, aExtInfo2);
        }
        else
        {
            // Chart always saves ranges using CONV_OOO convention.
            ScAddress::ExternalInfo aExtInfo;
            ScAddress aCell;
            ScRefFlags nRet = aCell.Parse(aToken, rDoc, ::formula::FormulaGrammar::CONV_OOO, &aExtInfo);
            if ((nRet & ScRefFlags::VALID) == ScRefFlags::ZERO )
            {
                nRet = aCell.Parse(aToken, rDoc, eConv, &aExtInfo);
                if ((nRet & ScRefFlags::VALID) == ScRefFlags::ZERO)
                    continue;
            }

            // Looks good!

            if (bFirst)
                bFirst = false;
            else
                aRetStr.append(cSepNew);

            lcl_appendCellAddress(aRetStr, rDoc, aCell, aExtInfo);
        }
    }

    rString = aRetStr.makeStringAndClear();
}

ScRangeData* ScRangeStringConverter::GetRangeDataFromString( const OUString& rString, const SCTAB nTab,
        const ScDocument& rDoc, formula::FormulaGrammar::AddressConvention eConv )
{
    // This may be called with an external 'doc'#name but wouldn't find any.

    // Dot '.' is not allowed in range names, if present only lookup if it's a
    // sheet-local name. Same for '!' Excel syntax.
    // If eConv == FormulaGrammar::CONV_A1_XL_A1 then try both, first our own.
    sal_Int32 nIndex = -1;
    if (eConv == FormulaGrammar::CONV_OOO || eConv == FormulaGrammar::CONV_A1_XL_A1)
        nIndex = ScGlobal::FindUnquoted( rString, '.');
    if (nIndex < 0 && (eConv == FormulaGrammar::CONV_A1_XL_A1
                || eConv == FormulaGrammar::CONV_XL_A1
                || eConv == FormulaGrammar::CONV_XL_R1C1
                || eConv == FormulaGrammar::CONV_XL_OOX))
        nIndex = ScGlobal::FindUnquoted( rString, '!');

    if (nIndex >= 0)
    {
        if (nIndex == 0)
            return nullptr;     // Can't be a name.

        OUString aTab( rString.copy( 0, nIndex));
        ScGlobal::EraseQuotes( aTab, '\'');
        SCTAB nLocalTab;
        if (!rDoc.GetTable( aTab, nLocalTab))
            return nullptr;

        ScRangeName* pLocalRangeName = rDoc.GetRangeName(nLocalTab);
        if (!pLocalRangeName)
            return nullptr;

        const OUString aName( rString.copy( nIndex+1));
        return pLocalRangeName->findByUpperName( ScGlobal::getCharClass().uppercase( aName));
    }

    ScRangeName* pLocalRangeName = rDoc.GetRangeName(nTab);
    ScRangeData* pData = nullptr;
    OUString aUpperName = ScGlobal::getCharClass().uppercase(rString);
    if(pLocalRangeName)
    {
        pData = pLocalRangeName->findByUpperName(aUpperName);
    }
    if (!pData)
    {
        ScRangeName* pGlobalRangeName = rDoc.GetRangeName();
        if (pGlobalRangeName)
        {
            pData = pGlobalRangeName->findByUpperName(aUpperName);
        }
    }
    return pData;
}

ScArea::ScArea( SCTAB tab,
                SCCOL colStart, SCROW rowStart,
                SCCOL colEnd,   SCROW rowEnd ) :
        nTab     ( tab ),
        nColStart( colStart ),  nRowStart( rowStart ),
        nColEnd  ( colEnd ),    nRowEnd  ( rowEnd )
{
}

bool ScArea::operator==( const ScArea& r ) const
{
    return (   (nTab        == r.nTab)
            && (nColStart   == r.nColStart)
            && (nRowStart   == r.nRowStart)
            && (nColEnd     == r.nColEnd)
            && (nRowEnd     == r.nRowEnd) );
}

ScAreaNameIterator::ScAreaNameIterator( const ScDocument& rDoc ) :
    pRangeName(rDoc.GetRangeName()),
    pDBCollection(rDoc.GetDBCollection()),
    bFirstPass(true)
{
    if (pRangeName)
    {
        maRNPos = pRangeName->begin();
        maRNEnd = pRangeName->end();
    }
}

bool ScAreaNameIterator::Next( OUString& rName, ScRange& rRange )
{
    for (;;)
    {
        if ( bFirstPass )                                   // first the area names
        {
            if ( pRangeName && maRNPos != maRNEnd )
            {
                const ScRangeData& rData = *maRNPos->second;
                ++maRNPos;
                bool bValid = rData.IsValidReference(rRange);
                if (bValid)
                {
                    rName = rData.GetName();
                    return true;                            // found
                }
            }
            else
            {
                bFirstPass = false;
                if (pDBCollection)
                {
                    const ScDBCollection::NamedDBs& rDBs = pDBCollection->getNamedDBs();
                    maDBPos = rDBs.begin();
                    maDBEnd = rDBs.end();
                }
            }
        }

        if ( !bFirstPass )                                  // then the DB areas
        {
            if (pDBCollection && maDBPos != maDBEnd)
            {
                const ScDBData& rData = **maDBPos;
                ++maDBPos;
                rData.GetArea(rRange);
                rName = rData.GetName();
                return true;                                // found
            }
            else
                return false;                               // nothing left
        }
    }
}

void ScRangeUpdater::UpdateInsertTab(ScAddress& rAddr, const sc::RefUpdateInsertTabContext& rCxt)
{
    if (rCxt.mnInsertPos <= rAddr.Tab())
    {
        rAddr.IncTab(rCxt.mnSheets);
    }
}

void ScRangeUpdater::UpdateDeleteTab(ScAddress& rAddr, const sc::RefUpdateDeleteTabContext& rCxt)
{
    if (rCxt.mnDeletePos <= rAddr.Tab())
    {
        rAddr.SetTab( std::max<SCTAB>(0, rAddr.Tab() - rCxt.mnSheets));
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
