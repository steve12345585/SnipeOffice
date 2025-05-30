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

#include "lotfilter.hxx"
#include <lotimpop.hxx>
#include <osl/mutex.hxx>
#include <sal/log.hxx>

#include <document.hxx>
#include <formulacell.hxx>
#include <global.hxx>

#include <lotfntbf.hxx>
#include <lotform.hxx>
#include <tool.h>
#include <namebuff.hxx>
#include <lotattr.hxx>
#include <stringutil.hxx>
#include <config_fuzzers.h>


static osl::Mutex aLotImpSemaphore;

ImportLotus::ImportLotus(LotusContext &rContext, SvStream& aStream, rtl_TextEncoding eQ)
    : ImportTyp(rContext.rDoc, eQ)
    , pIn(&aStream)
    , aConv(rContext, *pIn, rContext.rDoc.GetSharedStringPool(), eQ, false)
    , nTab(0)
    , nExtTab(0)
{
    // good point to start locking of import lotus
    aLotImpSemaphore.acquire();
}

ImportLotus::~ImportLotus()
{
    // no need 4 pLotusRoot anymore
    aLotImpSemaphore.release();
}

void ImportLotus::Bof()
{
    sal_uInt16  nFileCode, nFileSub, nSaveCnt;
    sal_uInt8   nMajorId, nMinorId, nFlags;

    Read( nFileCode );
    Read( nFileSub );
    LotusContext &rContext = aConv.getContext();
    Read( rContext.aActRange );
    Read( nSaveCnt );
    Read( nMajorId );
    Read( nMinorId );
    Skip( 1 );
    Read( nFlags );

    if (!pIn->good())
        return;

    if( nFileSub == 0x0004 )
    {
        if( nFileCode == 0x1000 )
        {// <= WK3
            rContext.eFirstType = rContext.eActType = Lotus123Typ::WK3;
        }
        else if( nFileCode == 0x1002 )
        {// WK4
            rContext.eFirstType = rContext.eActType = Lotus123Typ::WK4;
        }
    }
}

bool ImportLotus::BofFm3()
{
    sal_uInt16 nFileCode(0), nFileSub(0);

    Read( nFileCode );
    Read( nFileSub );

    return ( nFileCode == 0x8007 && ( nFileSub == 0x0000 || nFileSub == 0x00001 ) );
}

void ImportLotus::Columnwidth( sal_uInt16 nRecLen )
{
    SAL_WARN_IF( nRecLen < 4, "sc.filter", "*ImportLotus::Columnwidth(): Record too short!" );

    sal_uInt16 nCnt = (nRecLen < 4) ? 0 : ( nRecLen - 4 ) / 2;

    sal_uInt8 nLTab(0), nWindow2(0);
    Read( nLTab );
    Read( nWindow2 );

    if( !rD.HasTable( static_cast<SCTAB> (nLTab) ) )
        rD.MakeTable( static_cast<SCTAB> (nLTab) );

    if( nWindow2 )
        return;

    Skip( 2 );

    while (nCnt && pIn->good())
    {
        sal_uInt8 nCol(0), nSpaces(0);
        Read( nCol );
        Read( nSpaces );
        // Attention: ambiguous Correction factor!
        rD.SetColWidth( static_cast<SCCOL> (nCol), static_cast<SCTAB> (nLTab), static_cast<sal_uInt16>( TWIPS_PER_CHAR * 1.28 * nSpaces ) );

        nCnt--;
    }

    SAL_WARN_IF(!pIn->good(), "sc.filter", "*ImportLotus::Columnwidth(): short read");
}

void ImportLotus::Hiddencolumn( sal_uInt16 nRecLen )
{
    SAL_WARN_IF( nRecLen < 4, "sc.filter", "*ImportLotus::Hiddencolumn(): Record too short!" );

    sal_uInt16 nCnt = (nRecLen < 4) ? 0 : ( nRecLen - 4 ) / 2;

    sal_uInt8 nLTab(0), nWindow2(0);
    Read( nLTab );
    Read( nWindow2 );

    if( nWindow2 )
        return;

    Skip( 2 );

    while (nCnt && pIn->good())
    {
        sal_uInt8 nCol(0);
        Read( nCol );

        rD.SetColHidden(static_cast<SCCOL>(nCol), static_cast<SCCOL>(nCol), static_cast<SCTAB>(nLTab), true);
        nCnt--;
    }

    SAL_WARN_IF(!pIn->good(), "sc.filter", "*ImportLotus::Hiddencolumn(): short read");
}

void ImportLotus::Userrange()
{
    sal_uInt16      nRangeType;
    ScRange     aScRange;

    Read( nRangeType );

    char aBuffer[ 17 ];
    aBuffer[pIn->ReadBytes(aBuffer, 16)] = 0;
    OUString aName(aBuffer, strlen(aBuffer), eQuellChar);

    Read(aScRange);

    if (!pIn->good())
    {
        SAL_WARN("sc.filter", "invalid range");
        return;
    }

    LotusContext &rContext = aConv.getContext();
    rContext.pRngNmBffWK3->Add( rContext.rDoc, aName, aScRange );
}

void ImportLotus::Errcell()
{
    ScAddress   aA;

    Read( aA );

    if (!pIn->good() || !rD.ValidAddress(aA))
    {
        SAL_WARN("sc.filter", "invalid address");
        return;
    }

    ScSetStringParam aParam;
    aParam.setTextInput();
    rD.EnsureTable(aA.Tab());
    // coverity[tainted_data : FALSE] - ValidAddress has sanitized aA
    rD.SetString(aA, u"#ERR!"_ustr, &aParam);
}

void ImportLotus::Nacell()
{
    ScAddress   aA;

    Read( aA );

    if (!pIn->good() || !rD.ValidAddress(aA))
    {
        SAL_WARN("sc.filter", "invalid address");
        return;
    }

    ScSetStringParam aParam;
    aParam.setTextInput();
    rD.EnsureTable(aA.Tab());
    // coverity[tainted_data : FALSE] - ValidAddress has sanitized aA
    rD.SetString(aA, u"#NA!"_ustr, &aParam);
}

void ImportLotus::Labelcell()
{
    ScAddress   aA;
    OUString    aLabel;
    char        cAlign;

    Read( aA );
    Read( cAlign );
    Read( aLabel );

    if (!pIn->good() || !rD.ValidAddress(aA))
    {
        SAL_WARN("sc.filter", "invalid address");
        return;
    }

    ScSetStringParam aParam;
    aParam.setTextInput();
    rD.EnsureTable(aA.Tab());
    // coverity[tainted_data : FALSE] - ValidAddress has sanitized aA
    rD.SetString(aA, aLabel, &aParam);
}

void ImportLotus::Numbercell()
{
    ScAddress   aAddr;
    double      fVal;

    Read( aAddr );
    Read( fVal );

    if (!pIn->good() || !rD.ValidAddress(aAddr))
    {
        SAL_WARN("sc.filter", "invalid address");
        return;
    }

    rD.EnsureTable(aAddr.Tab());
    // coverity[tainted_data : FALSE] - ValidAddress has sanitized aAddr
    rD.SetValue(aAddr, fVal);
}

void ImportLotus::Smallnumcell()
{
    ScAddress   aAddr;
    sal_Int16       nVal;

    Read( aAddr );
    Read( nVal );

    if (!pIn->good() || !rD.ValidAddress(aAddr))
    {
        SAL_WARN("sc.filter", "invalid address");
        return;
    }

    rD.EnsureTable(aAddr.Tab());
    // coverity[tainted_data : FALSE] - ValidAddress has sanitized aAddr
    rD.SetValue(aAddr, SnumToDouble(nVal));
}

void ImportLotus::Formulacell( sal_uInt16 n )
{
    SAL_WARN_IF( !pIn, "sc.filter", "-ImportLotus::Formulacell(): Null-Stream!" );

    ScAddress           aAddr;

    Read( aAddr );
    Skip( 10 );

    n -= std::min<sal_uInt16>(n, 14);

    std::unique_ptr<ScTokenArray> pErg;
    sal_Int32 nRest = n;

    aConv.Reset( aAddr );
    aConv.SetWK3();
    aConv.Convert( pErg, nRest );
    if (!aConv.good())
        return;

    if (!pIn->good() || !rD.ValidAddress(aAddr))
    {
        SAL_WARN("sc.filter", "invalid address");
        return;
    }

    ScFormulaCell* pCell = pErg ? new ScFormulaCell(rD, aAddr, std::move(pErg)) : new ScFormulaCell(rD, aAddr);
    pCell->AddRecalcMode( ScRecalcMode::ONLOAD_ONCE );
    rD.EnsureTable(aAddr.Tab());
    // coverity[tainted_data : FALSE] - ValidAddress has sanitized aAddr
    rD.SetFormulaCell(aAddr, pCell);
}

void ImportLotus::Read( OUString &r )
{
    ScfTools::AppendCString( *pIn, r, eQuellChar );
}

void ImportLotus::RowPresentation( sal_uInt16 nRecLen )
{
    SAL_WARN_IF( nRecLen < 5, "sc.filter", "*ImportLotus::RowPresentation(): Record too short!" );

    sal_uInt16 nCnt = (nRecLen < 4) ? 0 : ( nRecLen - 4 ) / 8;

    sal_uInt8 nLTab(0);
    Read( nLTab );
    Skip( 1 );

    while (nCnt && pIn->good())
    {
        sal_uInt16 nRow(0);
        Read( nRow );
        sal_uInt16 nHeight(0);
        Read( nHeight );
        Skip( 2 );
        sal_uInt8 nFlags(0);
        Read( nFlags );
        Skip( 1 );

        if( nFlags & 0x02 )     // Fixed / Stretch to fit fonts
        {   // fixed
            // Height in Lotus in 1/32 Points
            nHeight *= 20;  // -> 32 * TWIPS
            nHeight /= 32;  // -> TWIPS

            rD.SetRowFlags( static_cast<SCROW> (nRow), static_cast<SCTAB> (nLTab), rD.GetRowFlags( static_cast<SCROW> (nRow), static_cast<SCTAB> (nLTab) ) | CRFlags::ManualSize );

            rD.SetRowHeight( static_cast<SCROW> (nRow), static_cast<SCTAB> (nLTab), nHeight );
        }

        nCnt--;
    }
}

void ImportLotus::NamedSheet()
{
    sal_uInt16 nTmpTab(0);
    Read(nTmpTab);
    OUString aName;
    Read(aName);

    SCTAB nLTab(SanitizeTab(static_cast<SCTAB>(nTmpTab)));
#if ENABLE_FUZZERS
    //ofz#14167 arbitrary sheet limit to make fuzzing useful
    if (nLTab > 5)
        nLTab = 5;
#endif

    if (rD.HasTable(nLTab))
        rD.RenameTab(nLTab, aName);
    else
        rD.InsertTab(nLTab, aName);
}

void ImportLotus::Font_Face()
{
    sal_uInt8   nNum;
    OUString    aName;

    Read( nNum );

    if( nNum >= LotusFontBuffer::nSize )
        return;     // nonsense

    Read( aName );

    LotusContext &rContext = aConv.getContext();
    rContext.maFontBuff.SetName( nNum, aName );
}

void ImportLotus::Font_Type()
{
    LotusContext &rContext = aConv.getContext();
    for( sal_uInt16 nCnt = 0 ; nCnt < LotusFontBuffer::nSize ; nCnt++ )
    {
        sal_uInt16 nType;
        Read( nType );
        rContext.maFontBuff.SetType( nCnt, nType );
    }
}

void ImportLotus::Font_Ysize()
{
    LotusContext &rContext = aConv.getContext();
    for( sal_uInt16 nCnt = 0 ; nCnt < LotusFontBuffer::nSize ; nCnt++ )
    {
        sal_uInt16 nSize;
        Read( nSize );
        rContext.maFontBuff.SetHeight( nCnt, nSize );
    }
}

void ImportLotus::Row_( const sal_uInt16 nRecLen )
{
    SAL_WARN_IF( nExtTab < 0, "sc.filter", "*ImportLotus::Row_(): not possible!" );

    sal_uInt16            nCntDwn = (nRecLen < 4) ? 0 : ( nRecLen - 4 ) / 5;
    SCCOL           nColCnt = 0;
    sal_uInt8           nRepeats;
    LotAttrWK3      aAttr;

    bool            bCenter = false;
    SCCOL           nCenterStart = 0, nCenterEnd = 0;
    LotusContext &rContext = aConv.getContext();

    sal_uInt16 nTmpRow(0);
    Read(nTmpRow);
    SCROW nRow(rContext.rDoc.SanitizeRow(static_cast<SCROW>(nTmpRow)));
    sal_uInt16 nHeight(0);
    Read(nHeight);

    nHeight &= 0x0FFF;
    nHeight *= 22;

    SCTAB nDestTab(static_cast<SCTAB>(nExtTab));

    if( nHeight )
        rD.SetRowHeight(nRow, nDestTab, nHeight);

    while( nCntDwn )
    {
        Read( aAttr );
        Read( nRepeats );

        if( aAttr.HasStyles() )
            rContext.maAttrTable.SetAttr(
                rContext, nColCnt, static_cast<SCCOL> ( nColCnt + nRepeats ), nRow, aAttr );

        // Do this here and NOT in class LotAttrTable, as we only add attributes if the other
        // attributes are set
        //  -> for Center-Attribute default is centered
        if( aAttr.IsCentered() )
        {
            if( bCenter )
            {
                if (rD.HasData(nColCnt, nRow, nDestTab))
                {
                    // new Center after previous Center
                    rD.DoMerge( nCenterStart, nRow, nCenterEnd, nRow, nDestTab);
                    nCenterStart = nColCnt;
                }
            }
            else
            {
                // fully new Center
                bCenter = true;
                nCenterStart = nColCnt;
            }
            nCenterEnd = nColCnt + static_cast<SCCOL>(nRepeats);
        }
        else
        {
            if( bCenter )
            {
                // possibly reset old Center
                rD.DoMerge( nCenterStart, nRow, nCenterEnd, nRow, nDestTab );
                bCenter = false;
            }
        }

        nColCnt = nColCnt + static_cast<SCCOL>(nRepeats);
        nColCnt++;

        nCntDwn--;
    }

    if( bCenter )
        // possibly reset old Center
        rD.DoMerge( nCenterStart, nRow, nCenterEnd, nRow, nDestTab );
}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
