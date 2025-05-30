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

#include <crsrsh.hxx>
#include <layfrm.hxx>
#include <cntfrm.hxx>
#include <swcrsr.hxx>
#include <viscrs.hxx>
#include "callnk.hxx"

SwLayoutFrame* GetCurrColumn( const SwLayoutFrame* pLayFrame )
{
    while( pLayFrame && !pLayFrame->IsColumnFrame() )
        pLayFrame = pLayFrame->GetUpper();
    return const_cast<SwLayoutFrame*>(pLayFrame);
}

SwLayoutFrame* GetNextColumn( const SwLayoutFrame* pLayFrame )
{
    SwLayoutFrame* pActCol = GetCurrColumn( pLayFrame );
    return pActCol ? static_cast<SwLayoutFrame*>(pActCol->GetNext()) : nullptr;
}

SwLayoutFrame* GetPrevColumn( const SwLayoutFrame* pLayFrame )
{
    SwLayoutFrame* pActCol = GetCurrColumn( pLayFrame );
    return pActCol ? static_cast<SwLayoutFrame*>(pActCol->GetPrev()) : nullptr;
}

SwContentFrame* GetColumnStt( const SwLayoutFrame* pColFrame )
{
    return pColFrame ? const_cast<SwContentFrame*>(pColFrame->ContainsContent()) : nullptr;
}

SwContentFrame* GetColumnEnd( const SwLayoutFrame* pColFrame )
{
    SwContentFrame *pRet = GetColumnStt( pColFrame );
    if( !pRet )
        return nullptr;

    SwContentFrame *pNxt = pRet->GetNextContentFrame();
    while( pNxt && pColFrame->IsAnLower( pNxt ) )
    {
        pRet = pNxt;
        pNxt = pNxt->GetNextContentFrame();
    }
    return pRet;
}

void SwCursorShell::MoveColumn( SwWhichColumn fnWhichCol, SwPosColumn fnPosCol )
{
    if( m_pTableCursor )
        return;
    SwLayoutFrame* pLayFrame = GetCurrFrame()->GetUpper();
    if( !pLayFrame )
        return;
    pLayFrame = (*fnWhichCol)( pLayFrame );
    if(  !pLayFrame )
        return;

    SwContentFrame* pCnt = (*fnPosCol)( pLayFrame );
    if( !pCnt )
        return;

    CurrShell aCurr( this );
    SwCallLink aLk( *this ); // watch Cursor-Moves; call Link if needed
    SwCursorSaveState aSaveState( *m_pCurrentCursor );

    pCnt->Calc(GetOut());

    Point aPt( pCnt->getFrameArea().Pos() + pCnt->getFramePrintArea().Pos() );
    if( fnPosCol == GetColumnEnd )
    {
        aPt.setX(aPt.getX() + pCnt->getFramePrintArea().Width());
        aPt.setY(aPt.getY() + pCnt->getFramePrintArea().Height());
    }

    pCnt->GetModelPositionForViewPoint( m_pCurrentCursor->GetPoint(), aPt );

    if( !m_pCurrentCursor->IsInProtectTable( true ) &&
        !m_pCurrentCursor->IsSelOvr() )
    {
        UpdateCursor();
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
