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

#include <consoli.hxx>
#include <document.hxx>
#include <olinetab.hxx>
#include <subtotal.hxx>
#include <formula/errorcodes.hxx>
#include <formulacell.hxx>
#include <tokenarray.hxx>
#include <osl/diagnose.h>
#include <refdata.hxx>

#include <string.h>
#include <memory>

#define SC_CONS_NOTFOUND    -1

const OpCode eOpCodeTable[] = {      //  order as for enum ScSubTotalFunc
        ocBad,                              //  none
        ocAverage,
        ocCount,
        ocCount2,
        ocMax,
        ocMin,
        ocProduct,
        ocStDev,
        ocStDevP,
        ocSum,
        ocVar,
        ocVarP };

template< typename T >
static void lcl_AddString( ::std::vector<OUString>& rData, T& nCount, const OUString& rInsert )
{
    rData.push_back( rInsert);
    ++nCount;
}

ScConsData::ScConsData() :
    eFunction(SUBTOTAL_FUNC_SUM),
    bReference(false),
    bColByName(false),
    bRowByName(false),
    nColCount(0),
    nRowCount(0),
    nDataCount(0),
    bCornerUsed(false)
{
}

ScConsData::~ScConsData()
{
}

void ScConsData::DeleteData()
{
    ppRefs.reset();
    ppFunctionData.reset();
    ppUsed.reset();
    ppTitlePos.reset();
    ::std::vector<OUString>().swap( maColHeaders);
    ::std::vector<OUString>().swap( maRowHeaders);
    ::std::vector<OUString>().swap( maTitles);
    nDataCount = 0;

    if (bColByName) nColCount = 0;                  // otherwise maColHeaders is wrong
    if (bRowByName) nRowCount = 0;

    bCornerUsed = false;
    aCornerText.clear();
}

void ScConsData::InitData()
{
    if (bReference && nColCount && !ppRefs)
    {
        ppRefs.reset(new std::unique_ptr<ScReferenceList[]>[nColCount]);
        for (SCSIZE i=0; i<nColCount; i++)
            ppRefs[i].reset(new ScReferenceList[nRowCount]);
    }
    else if (nColCount && !ppFunctionData)
    {
        ppFunctionData.reset( new std::unique_ptr<ScFunctionData[]>[nColCount] );
        for (SCSIZE i=0; i<nColCount; i++)
        {
            ppFunctionData[i].reset( new ScFunctionData[nRowCount] );
        }
    }

    if (nColCount && !ppUsed)
    {
        ppUsed.reset( new std::unique_ptr<bool[]>[nColCount] );
        for (SCSIZE i=0; i<nColCount; i++)
        {
            ppUsed[i].reset( new bool[nRowCount] );
            memset( ppUsed[i].get(), 0, nRowCount * sizeof(bool) );
        }
    }

    if (nRowCount && nDataCount && !ppTitlePos)
    {
        ppTitlePos.reset( new std::unique_ptr<SCSIZE[]>[nRowCount] );
        for (SCSIZE i=0; i<nRowCount; i++)
        {
            ppTitlePos[i].reset( new SCSIZE[nDataCount] );
            memset( ppTitlePos[i].get(), 0, nDataCount * sizeof(SCSIZE) );    //TODO: not necessary ?
        }
    }

    //  CornerText: single String
}

void ScConsData::DoneFields()
{
    InitData();
}

void ScConsData::SetSize( SCCOL nCols, SCROW nRows )
{
    DeleteData();
    nColCount = static_cast<SCSIZE>(nCols);
    nRowCount = static_cast<SCSIZE>(nRows);
}

void ScConsData::GetSize( SCCOL& rCols, SCROW& rRows ) const
{
    rCols = static_cast<SCCOL>(nColCount);
    rRows = static_cast<SCROW>(nRowCount);
}

void ScConsData::SetFlags( ScSubTotalFunc eFunc, bool bColName, bool bRowName, bool bRef )
{
    DeleteData();
    bReference = bRef;
    bColByName = bColName;
    if (bColName) nColCount = 0;
    bRowByName = bRowName;
    if (bRowName) nRowCount = 0;
    eFunction = eFunc;
}

void ScConsData::AddFields( const ScDocument* pSrcDoc, SCTAB nTab,
                            SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2 )
{
    ++nDataCount;

    OUString aTitle;

    SCCOL nStartCol = nCol1;
    SCROW nStartRow = nRow1;
    if (bColByName) ++nStartRow;
    if (bRowByName) ++nStartCol;

    if (bColByName)
    {
        for (SCCOL nCol=nStartCol; nCol<=nCol2; nCol++)
        {
            aTitle = pSrcDoc->GetString(nCol, nRow1, nTab);
            if (!aTitle.isEmpty())
            {
                bool bFound = false;
                for (SCSIZE i=0; i<nColCount && !bFound; i++)
                    if ( maColHeaders[i] == aTitle )
                        bFound = true;
                if (!bFound)
                    lcl_AddString( maColHeaders, nColCount, aTitle );
            }
        }
    }

    if (!bRowByName)
        return;

    for (SCROW nRow=nStartRow; nRow<=nRow2; nRow++)
    {
        aTitle = pSrcDoc->GetString(nCol1, nRow, nTab);
        if (!aTitle.isEmpty())
        {
            bool bFound = false;
            for (SCSIZE i=0; i<nRowCount && !bFound; i++)
                if ( maRowHeaders[i] == aTitle )
                    bFound = true;
            if (!bFound)
                lcl_AddString( maRowHeaders, nRowCount, aTitle );
        }
    }
}

void ScConsData::AddName( const OUString& rName )
{
    SCSIZE nArrX;
    SCSIZE nArrY;

    if (!bReference)
        return;

    maTitles.push_back( rName);
    size_t nTitleCount = maTitles.size();

    for (nArrY=0; nArrY<nRowCount; nArrY++)
    {
        //  set all data to same length

        SCSIZE nMax = 0;
        for (nArrX=0; nArrX<nColCount; nArrX++)
            nMax = std::max( nMax, ppRefs[nArrX][nArrY].size() );

        for (nArrX=0; nArrX<nColCount; nArrX++)
        {
            ppUsed[nArrX][nArrY] = true;
            ppRefs[nArrX][nArrY].resize( nMax, { SC_CONS_NOTFOUND, SC_CONS_NOTFOUND, SC_CONS_NOTFOUND });
        }

        //  store positions

        if (ppTitlePos)
            if (nTitleCount < nDataCount)
                ppTitlePos[nArrY][nTitleCount] = nMax;
    }
}

void ScConsData::AddData( ScDocument* pSrcDoc, SCTAB nTab,
                            SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2 )
{
    PutInOrder(nCol1,nCol2);
    PutInOrder(nRow1,nRow2);
    if ( nCol2 >= sal::static_int_cast<SCCOL>(nCol1 + nColCount) && !bColByName )
    {
        OSL_FAIL("range too big");
        nCol2 = sal::static_int_cast<SCCOL>( nCol1 + nColCount - 1 );
    }
    if ( nRow2 >= sal::static_int_cast<SCROW>(nRow1 + nRowCount) && !bRowByName )
    {
        OSL_FAIL("range too big");
        nRow2 = sal::static_int_cast<SCROW>( nRow1 + nRowCount - 1 );
    }

    SCCOL nCol;
    SCROW nRow;

    // left top corner

    if ( bColByName && bRowByName )
    {
        OUString aThisCorner = pSrcDoc->GetString(nCol1, nRow1, nTab);
        if (bCornerUsed)
        {
            if (aCornerText != aThisCorner)
                aCornerText.clear();
        }
        else
        {
            aCornerText = aThisCorner;
            bCornerUsed = true;
        }
    }

    // search title

    SCCOL nStartCol = nCol1;
    SCROW nStartRow = nRow1;
    if (bColByName) ++nStartRow;
    if (bRowByName) ++nStartCol;
    OUString aTitle;
    std::unique_ptr<SCCOL[]> pDestCols;
    std::unique_ptr<SCROW[]> pDestRows;
    if (bColByName)
    {
        pDestCols.reset(new SCCOL[nCol2-nStartCol+1]);
        for (nCol=nStartCol; nCol<=nCol2; nCol++)
        {
            aTitle = pSrcDoc->GetString(nCol, nRow1, nTab);
            SCCOL nPos = SC_CONS_NOTFOUND;
            if (!aTitle.isEmpty())
            {
                bool bFound = false;
                for (SCSIZE i=0; i<nColCount && !bFound; i++)
                    if ( maColHeaders[i] == aTitle )
                    {
                        nPos = static_cast<SCCOL>(i);
                        bFound = true;
                    }
                OSL_ENSURE(bFound, "column not found");
            }
            pDestCols[nCol-nStartCol] = nPos;
        }
    }
    if (bRowByName)
    {
        pDestRows.reset(new SCROW[nRow2-nStartRow+1]);
        for (nRow=nStartRow; nRow<=nRow2; nRow++)
        {
            aTitle = pSrcDoc->GetString(nCol1, nRow, nTab);
            SCROW nPos = SC_CONS_NOTFOUND;
            if (!aTitle.isEmpty())
            {
                bool bFound = false;
                for (SCSIZE i=0; i<nRowCount && !bFound; i++)
                    if ( maRowHeaders[i] == aTitle )
                    {
                        nPos = static_cast<SCROW>(i);
                        bFound = true;
                    }
                OSL_ENSURE(bFound, "row not found");
            }
            pDestRows[nRow-nStartRow] = nPos;
        }
    }
    nCol1 = nStartCol;
    nRow1 = nStartRow;

    // data

    bool bAnyCell = ( eFunction == SUBTOTAL_FUNC_CNT2 );
    for (nCol=nCol1; nCol<=nCol2; nCol++)
    {
        SCCOL nArrX = nCol-nCol1;
        if (bColByName) nArrX = pDestCols[nArrX];
        if (nArrX != SC_CONS_NOTFOUND)
        {
            for (nRow=nRow1; nRow<=nRow2; nRow++)
            {
                SCROW nArrY = nRow-nRow1;
                if (bRowByName) nArrY = pDestRows[nArrY];
                if ( nArrY != SC_CONS_NOTFOUND && (
                        bAnyCell ? pSrcDoc->HasData( nCol, nRow, nTab )
                                 : pSrcDoc->HasValueData( nCol, nRow, nTab ) ) )
                {
                    if (bReference)
                    {
                        ppUsed[nArrX][nArrY] = true;
                        ppRefs[nArrX][nArrY].push_back( { nCol, nRow, nTab } );
                    }
                    else
                    {
                        double nVal = pSrcDoc->GetValue( nCol, nRow, nTab );
                        if (!ppUsed[nArrX][nArrY])
                        {
                            ppUsed[nArrX][nArrY] = true;
                            ppFunctionData[nArrX][nArrY] = ScFunctionData( eFunction);
                        }
                        ppFunctionData[nArrX][nArrY].update( nVal);
                    }
                }
            }
        }
    }
}

// check before, how many rows to insert (for Undo)

SCROW ScConsData::GetInsertCount() const
{
    SCROW nInsert = 0;
    SCSIZE nArrX;
    SCSIZE nArrY;
    if ( ppRefs && ppUsed )
    {
        for (nArrY=0; nArrY<nRowCount; nArrY++)
        {
            SCSIZE nNeeded = 0;
            for (nArrX=0; nArrX<nColCount; nArrX++)
                nNeeded = std::max( nNeeded, ppRefs[nArrX][nArrY].size() );

            nInsert += nNeeded;
        }
    }
    return nInsert;
}

// store completed data to document
//TODO: optimize on columns?

void ScConsData::OutputToDocument( ScDocument& rDestDoc, SCCOL nCol, SCROW nRow, SCTAB nTab )
{
    OpCode eOpCode = eOpCodeTable[eFunction];

    SCSIZE nArrX;
    SCSIZE nArrY;

    // left top corner

    if ( bColByName && bRowByName && !aCornerText.isEmpty() )
        rDestDoc.SetString( nCol, nRow, nTab, aCornerText );

    // title

    SCCOL nStartCol = nCol;
    SCROW nStartRow = nRow;
    if (bColByName) ++nStartRow;
    if (bRowByName) ++nStartCol;

    if (bColByName)
        for (SCSIZE i=0; i<nColCount; i++)
            rDestDoc.SetString( sal::static_int_cast<SCCOL>(nStartCol+i), nRow, nTab, maColHeaders[i] );
    if (bRowByName)
        for (SCSIZE j=0; j<nRowCount; j++)
            rDestDoc.SetString( nCol, sal::static_int_cast<SCROW>(nStartRow+j), nTab, maRowHeaders[j] );

    nCol = nStartCol;
    nRow = nStartRow;

    // data

    if ( ppFunctionData && ppUsed )    // insert values directly
    {
        for (nArrX=0; nArrX<nColCount; nArrX++)
            for (nArrY=0; nArrY<nRowCount; nArrY++)
                if (ppUsed[nArrX][nArrY])
                {
                    double fVal = ppFunctionData[nArrX][nArrY].getResult();
                    if (ppFunctionData[nArrX][nArrY].getError())
                        rDestDoc.SetError( sal::static_int_cast<SCCOL>(nCol+nArrX),
                                           sal::static_int_cast<SCROW>(nRow+nArrY), nTab, FormulaError::NoValue );
                    else
                        rDestDoc.SetValue( sal::static_int_cast<SCCOL>(nCol+nArrX),
                                           sal::static_int_cast<SCROW>(nRow+nArrY), nTab, fVal );
                }
    }

    if ( !(ppRefs && ppUsed) )     // insert Reference
                                return;

                            //TODO: differentiate, if split into categories
    OUString aString;

    ScSingleRefData aSRef;  // data for Reference formula cells
    aSRef.InitFlags();      // this reference is absolute at all times
    aSRef.SetFlag3D(true);

    ScComplexRefData aCRef; // data for Sum cells
    aCRef.InitFlags();
    aCRef.Ref1.SetColRel(true); aCRef.Ref1.SetRowRel(true); aCRef.Ref1.SetTabRel(true);
    aCRef.Ref2.SetColRel(true); aCRef.Ref2.SetRowRel(true); aCRef.Ref2.SetTabRel(true);

    for (nArrY=0; nArrY<nRowCount; nArrY++)
    {
        SCSIZE nNeeded = 0;
        for (nArrX=0; nArrX<nColCount; nArrX++)
            nNeeded = std::max( nNeeded, ppRefs[nArrX][nArrY].size() );

        if (nNeeded)
        {
            rDestDoc.InsertRow( 0,nTab, rDestDoc.MaxCol(),nTab, nRow+nArrY, nNeeded );

            for (nArrX=0; nArrX<nColCount; nArrX++)
                if (ppUsed[nArrX][nArrY])
                {
                    SCSIZE nCount = ppRefs[nArrX][nArrY].size();
                    if (nCount)
                    {
                        for (SCSIZE nPos=0; nPos<nCount; nPos++)
                        {
                            ScReferenceEntry aRef = ppRefs[nArrX][nArrY][nPos];
                            if (aRef.nTab != SC_CONS_NOTFOUND)
                            {
                                // insert reference (absolute, 3d)

                                aSRef.SetAddress(rDestDoc.GetSheetLimits(), ScAddress(aRef.nCol,aRef.nRow,aRef.nTab), ScAddress());

                                ScTokenArray aRefArr(rDestDoc);
                                aRefArr.AddSingleReference(aSRef);
                                aRefArr.AddOpCode(ocStop);
                                ScAddress aDest( sal::static_int_cast<SCCOL>(nCol+nArrX),
                                                 sal::static_int_cast<SCROW>(nRow+nArrY+nPos), nTab );
                                ScFormulaCell* pCell = new ScFormulaCell(rDestDoc, aDest, aRefArr);
                                rDestDoc.SetFormulaCell(aDest, pCell);
                            }
                        }

                        // insert sum (relative, not 3d)

                        ScAddress aDest( sal::static_int_cast<SCCOL>(nCol+nArrX),
                                         sal::static_int_cast<SCROW>(nRow+nArrY+nNeeded), nTab );

                        ScRange aRange(sal::static_int_cast<SCCOL>(nCol+nArrX), nRow+nArrY, nTab);
                        aRange.aEnd.SetRow(nRow+nArrY+nNeeded-1);
                        aCRef.SetRange(rDestDoc.GetSheetLimits(), aRange, aDest);

                        ScTokenArray aArr(rDestDoc);
                        aArr.AddOpCode(eOpCode);            // selected function
                        aArr.AddOpCode(ocOpen);
                        aArr.AddDoubleReference(aCRef);
                        aArr.AddOpCode(ocClose);
                        aArr.AddOpCode(ocStop);
                        ScFormulaCell* pCell = new ScFormulaCell(rDestDoc, aDest, aArr);
                        rDestDoc.SetFormulaCell(aDest, pCell);
                    }
                }

            // insert outline

            ScOutlineArray& rOutArr = rDestDoc.GetOutlineTable( nTab, true )->GetRowArray();
            SCROW nOutStart = nRow+nArrY;
            SCROW nOutEnd = nRow+nArrY+nNeeded-1;
            bool bSize = false;
            rOutArr.Insert( nOutStart, nOutEnd, bSize );
            for (SCROW nOutRow=nOutStart; nOutRow<=nOutEnd; nOutRow++)
                rDestDoc.ShowRow( nOutRow, nTab, false );
            rDestDoc.SetDrawPageSize(nTab);
            rDestDoc.UpdateOutlineRow( nOutStart, nOutEnd, nTab, false );

            // sub title

            if (ppTitlePos && !maTitles.empty() && !maRowHeaders.empty())
            {
                for (SCSIZE nPos=0; nPos<nDataCount; nPos++)
                {
                    SCSIZE nTPos = ppTitlePos[nArrY][nPos];
                    bool bDo = true;
                    if (nPos+1<nDataCount)
                        if (ppTitlePos[nArrY][nPos+1] == nTPos)
                            bDo = false;                                    // empty
                    if ( bDo && nTPos < nNeeded )
                    {
                        aString = maRowHeaders[nArrY] + " / " + maTitles[nPos];
                        rDestDoc.SetString( nCol-1, nRow+nArrY+nTPos, nTab, aString );
                    }
                }
            }

            nRow += nNeeded;
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
