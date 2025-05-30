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

#include <comphelper/fileformat.h>
#include <comphelper/string.hxx>
#include <osl/thread.h>
#include <sot/exchange.hxx>
#include <sfx2/linkmgr.hxx>
#include <sfx2/bindings.hxx>
#include <svl/numformat.hxx>
#include <svl/sharedstringpool.hxx>
#include <o3tl/string_view.hxx>

#include <ddelink.hxx>
#include <brdcst.hxx>
#include <document.hxx>
#include <scmatrix.hxx>
#include <patattr.hxx>
#include <rechead.hxx>
#include <rangeseq.hxx>
#include <sc.hrc>
#include <hints.hxx>
#include <utility>


bool ScDdeLink::bIsInUpdate = false;

ScDdeLink::ScDdeLink( ScDocument& rD, OUString aA, OUString aT, OUString aI,
                        sal_uInt8 nM ) :
    ::sfx2::SvBaseLink(SfxLinkUpdateMode::ALWAYS,SotClipboardFormatId::STRING),
    rDoc( rD ),
    aAppl(std::move( aA )),
    aTopic(std::move( aT )),
    aItem(std::move( aI )),
    nMode( nM ),
    bNeedUpdate( false ),
    pResult( nullptr )
{
}

ScDdeLink::~ScDdeLink()
{
    // cancel connection

    // pResult is refcounted
}

ScDdeLink::ScDdeLink( ScDocument& rD, const ScDdeLink& rOther ) :
    ::sfx2::SvBaseLink(SfxLinkUpdateMode::ALWAYS,SotClipboardFormatId::STRING),
    rDoc    ( rD ),
    aAppl   ( rOther.aAppl ),
    aTopic  ( rOther.aTopic ),
    aItem   ( rOther.aItem ),
    nMode   ( rOther.nMode ),
    bNeedUpdate( false ),
    pResult ( nullptr )
{
    if (rOther.pResult)
        pResult = rOther.pResult->Clone();
}

ScDdeLink::ScDdeLink( ScDocument& rD, SvStream& rStream, ScMultipleReadHeader& rHdr ) :
    ::sfx2::SvBaseLink(SfxLinkUpdateMode::ALWAYS,SotClipboardFormatId::STRING),
    rDoc( rD ),
    bNeedUpdate( false ),
    pResult( nullptr )
{
    rHdr.StartEntry();

    rtl_TextEncoding eCharSet = rStream.GetStreamCharSet();
    aAppl = rStream.ReadUniOrByteString( eCharSet );
    aTopic = rStream.ReadUniOrByteString( eCharSet );
    aItem = rStream.ReadUniOrByteString( eCharSet );

    bool bHasValue;
    rStream.ReadCharAsBool( bHasValue );
    if ( bHasValue )
        pResult = new ScMatrix(0, 0);

    if (rHdr.BytesLeft())       // new in 388b and the 364w (RealTime Client) version
        rStream.ReadUChar( nMode );
    else
        nMode = SC_DDE_DEFAULT;

    rHdr.EndEntry();
}

void ScDdeLink::Store( SvStream& rStream, ScMultipleWriteHeader& rHdr ) const
{
    rHdr.StartEntry();

    rtl_TextEncoding eCharSet = rStream.GetStreamCharSet();
    rStream.WriteUniOrByteString( aAppl, eCharSet );
    rStream.WriteUniOrByteString( aTopic, eCharSet );
    rStream.WriteUniOrByteString( aItem, eCharSet );

    bool bHasValue = ( pResult != nullptr );
    rStream.WriteBool( bHasValue );

    if( rStream.GetVersion() > SOFFICE_FILEFORMAT_40 )      // not with 4.0 Export
        rStream.WriteUChar( nMode );                                   // since 388b

    //  links with Mode != SC_DDE_DEFAULT are completely omitted in 4.0 Export
    //  (from ScDocument::SaveDdeLinks)

    rHdr.EndEntry();
}

sfx2::SvBaseLink::UpdateResult ScDdeLink::DataChanged(
    const OUString& rMimeType, const css::uno::Any & rValue )
{
    //  we only master strings...
    if ( SotClipboardFormatId::STRING != SotExchange::GetFormatIdFromMimeType( rMimeType ))
        return SUCCESS;

    OUString aLinkStr;
    ScByteSequenceToString::GetString( aLinkStr, rValue );
    aLinkStr = convertLineEnd(aLinkStr, LINEEND_LF);

    //  if string ends with line end, discard:

    sal_Int32 nLen = aLinkStr.getLength();
    if (nLen && aLinkStr[nLen-1] == '\n')
        aLinkStr = aLinkStr.copy(0, nLen-1);

    SCSIZE nCols = 1;       // empty string -> an empty line
    SCSIZE nRows = 1;
    if (!aLinkStr.isEmpty())
    {
        nRows = static_cast<SCSIZE>(comphelper::string::getTokenCount(aLinkStr, '\n'));
        std::u16string_view aLine = o3tl::getToken(aLinkStr, 0, '\n' );
        if (!aLine.empty())
            nCols = static_cast<SCSIZE>(comphelper::string::getTokenCount(aLine, '\t'));
    }

    if (!nRows || !nCols)               // no data
    {
        pResult.reset();
    }
    else                                // split data
    {
        //  always newly re-create matrix, so that bIsString doesn't get mixed up
        pResult = new ScMatrix(nCols, nRows, 0.0);

        SvNumberFormatter* pFormatter = rDoc.GetFormatTable();
        svl::SharedStringPool& rPool = rDoc.GetSharedStringPool();

        //  nMode determines how the text is interpreted (#44455#/#49783#):
        //  SC_DDE_DEFAULT - number format from cell template "Standard"
        //  SC_DDE_ENGLISH - standard number format for English/US
        //  SC_DDE_TEXT    - without NumberFormatter directly as string
        sal_uInt32 nStdFormat = 0;
        if ( nMode == SC_DDE_DEFAULT )
        {
            nStdFormat = rDoc.getCellAttributeHelper().getDefaultCellAttribute().GetNumberFormat( pFormatter );
        }
        else if ( nMode == SC_DDE_ENGLISH )
            nStdFormat = pFormatter->GetStandardIndex(LANGUAGE_ENGLISH_US);

        for (SCSIZE nR=0; nR<nRows; nR++)
        {
            std::u16string_view aLine = o3tl::getToken(aLinkStr, static_cast<sal_Int32>(nR), '\n' );
            for (SCSIZE nC=0; nC<nCols; nC++)
            {
                OUString aEntry( o3tl::getToken(aLine, static_cast<sal_Int32>(nC), '\t' ) );
                sal_uInt32 nIndex = nStdFormat;
                double fVal = double();
                if ( nMode != SC_DDE_TEXT && pFormatter->IsNumberFormat( aEntry, nIndex, fVal ) )
                    pResult->PutDouble( fVal, nC, nR );
                else if (aEntry.isEmpty())
                    // empty cell
                    pResult->PutEmpty(nC, nR);
                else
                    pResult->PutString(rPool.intern(aEntry), nC, nR);
            }
        }
    }

    //  Something happened...

    if (HasListeners())
    {
        Broadcast(ScHint(SfxHintId::ScDataChanged, ScAddress()));
        rDoc.TrackFormulas();      // must happen immediately
        rDoc.StartTrackTimer();

        //  StartTrackTimer asynchronously calls TrackFormulas, Broadcast(FID_DATACHANGED),
        //  ResetChanged, SetModified and Invalidate(SID_SAVEDOC/SID_DOC_MODIFIED)
        //  TrackFormulas additionally once again immediately, so that, e.g., a formula still
        //  located in the FormulaTrack doesn't get calculated by IdleCalc (#61676#)

        //  notify Uno objects (for XRefreshListener)
        //  must be after TrackFormulas
        //TODO: do this asynchronously?
        ScLinkRefreshedHint aHint;
        aHint.SetDdeLink( aAppl, aTopic, aItem );
        rDoc.BroadcastUno( aHint );
    }

    return SUCCESS;
}

void ScDdeLink::ListenersGone()
{
    bool bWas = bIsInUpdate;
    bIsInUpdate = true;             // Remove() can trigger reschedule??!?

    ScDocument& rStackDoc = rDoc;   // member rDoc can't be used after removing the link

    sfx2::LinkManager* pLinkMgr = rDoc.GetLinkManager();
    pLinkMgr->Remove( this);        // deletes this

    if ( pLinkMgr->GetLinks().empty() )            // deleted the last one ?
    {
        SfxBindings* pBindings = rStackDoc.GetViewBindings();      // don't use member rDoc!
        if (pBindings)
            pBindings->Invalidate( SID_LINKS );
    }

    bIsInUpdate = bWas;
}

const ScMatrix* ScDdeLink::GetResult() const
{
    return pResult.get();
}

void ScDdeLink::SetResult( const ScMatrixRef& pRes )
{
    pResult = pRes;
}

void ScDdeLink::TryUpdate()
{
    if (bIsInUpdate)
        bNeedUpdate = true;         // cannot be executed now
    else
    {
        bIsInUpdate = true;
        rDoc.IncInDdeLinkUpdate();
        Update();
        rDoc.DecInDdeLinkUpdate();
        bIsInUpdate = false;
        bNeedUpdate = false;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
