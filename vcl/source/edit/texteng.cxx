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

#include <tools/stream.hxx>

#include <vcl/texteng.hxx>
#include <vcl/textview.hxx>
#include <vcl/commandevent.hxx>
#include <vcl/inputctx.hxx>
#include "textdoc.hxx"
#include "textdat2.hxx"
#include "textundo.hxx"
#include "textund2.hxx"
#include <svl/ctloptions.hxx>
#include <vcl/window.hxx>
#include <vcl/settings.hxx>
#include <vcl/toolkit/edit.hxx>
#include <vcl/virdev.hxx>
#include <sal/log.hxx>
#include <osl/diagnose.h>

#include "TextLine.hxx"
#include "IdleFormatter.hxx"

#include <com/sun/star/i18n/XBreakIterator.hpp>

#include <com/sun/star/i18n/CharacterIteratorMode.hpp>

#include <com/sun/star/i18n/WordType.hpp>

#include <com/sun/star/i18n/InputSequenceChecker.hpp>
#include <com/sun/star/i18n/InputSequenceCheckMode.hpp>
#include <com/sun/star/i18n/ScriptType.hpp>

#include <comphelper/processfactory.hxx>

#include <unotools/localedatawrapper.hxx>
#include <vcl/unohelp.hxx>

#include <vcl/svapp.hxx>

#include <unicode/ubidi.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <o3tl/sorted_vector.hxx>
#include <string_view>
#include <vector>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;

TextEngine::TextEngine()
    : mpActiveView {nullptr}
    , maTextColor {COL_BLACK}
    , mnMaxTextLen {0}
    , mnMaxTextWidth {0}
    , mnCharHeight {0}
    , mnCurTextWidth {-1}
    , mnCurTextHeight {0}
    , mnDefTab {0}
    , meAlign {TxtAlign::Left}
    , mbIsFormatting {false}
    , mbFormatted {false}
    , mbUpdate {true}
    , mbModified {false}
    , mbUndoEnabled {false}
    , mbIsInUndo {false}
    , mbDowning {false}
    , mbRightToLeft {false}
    , mbHasMultiLineParas {false}
{
    mpViews.reset( new TextViews );

    mpIdleFormatter.reset( new IdleFormatter );
    mpIdleFormatter->SetInvokeHandler( LINK( this, TextEngine, IdleFormatHdl ) );

    mpRefDev = VclPtr<VirtualDevice>::Create();

    ImpInitLayoutMode( mpRefDev );

    ImpInitDoc();

    vcl::Font aFont(mpRefDev->GetFont().GetFamilyName(), Size(0, 0));
    aFont.SetTransparent( false );
    Color aFillColor( aFont.GetFillColor() );
    aFillColor.SetAlpha( 255 );
    aFont.SetFillColor( aFillColor );
    SetFont( aFont );
}

TextEngine::~TextEngine()
{
    mbDowning = true;

    mpIdleFormatter.reset();
    mpDoc.reset();
    mpTEParaPortions.reset();
    mpViews.reset(); // only the list, not the Views
    mpRefDev.disposeAndClear();
    mpUndoManager.reset();
    mpIMEInfos.reset();
    mpLocaleDataWrapper.reset();
}

void TextEngine::InsertView( TextView* pTextView )
{
    mpViews->push_back( pTextView );
    pTextView->SetSelection( TextSelection() );

    if ( !GetActiveView() )
        SetActiveView( pTextView );
}

void TextEngine::RemoveView( TextView* pTextView )
{
    TextViews::iterator it = std::find( mpViews->begin(), mpViews->end(), pTextView );
    if( it != mpViews->end() )
    {
        pTextView->HideCursor();
        mpViews->erase( it );
        if ( pTextView == GetActiveView() )
            SetActiveView( nullptr );
    }
}

sal_uInt16 TextEngine::GetViewCount() const
{
    return mpViews->size();
}

TextView* TextEngine::GetView( sal_uInt16 nView ) const
{
    return (*mpViews)[ nView ];
}


void TextEngine::SetActiveView( TextView* pTextView )
{
    if ( pTextView != mpActiveView )
    {
        if ( mpActiveView )
            mpActiveView->HideSelection();

        mpActiveView = pTextView;

        if ( mpActiveView )
            mpActiveView->ShowSelection();
    }
}

void TextEngine::SetFont( const vcl::Font& rFont )
{
    if ( rFont == maFont )
        return;

    maFont = rFont;
    // #i40221# As the font's color now defaults to transparent (since i35764)
    //  we have to choose a useful textcolor in this case.
    // Otherwise maTextColor and maFont.GetColor() are both transparent...
    if( rFont.GetColor() == COL_TRANSPARENT )
        maTextColor = COL_BLACK;
    else
        maTextColor = rFont.GetColor();

    // Do not allow transparent fonts because of selection
    // (otherwise delete the background in ImplPaint later differently)
    maFont.SetTransparent( false );
    // Tell VCL not to use the font color, use text color from OutputDevice
    maFont.SetColor( COL_TRANSPARENT );
    Color aFillColor( maFont.GetFillColor() );
    aFillColor.SetAlpha( 255 );
    maFont.SetFillColor( aFillColor );

    maFont.SetAlignment( ALIGN_TOP );
    mpRefDev->SetFont( maFont );
    mnDefTab = mpRefDev->GetTextWidth(u"    "_ustr);
    if ( !mnDefTab )
        mnDefTab = mpRefDev->GetTextWidth(u"XXXX"_ustr);
    if ( !mnDefTab )
        mnDefTab = 1;
    mnCharHeight = mpRefDev->GetTextHeight();

    FormatFullDoc();
    UpdateViews();

    for ( auto nView = mpViews->size(); nView; )
    {
        TextView* pView = (*mpViews)[ --nView ];
        pView->GetWindow()->SetInputContext( InputContext( GetFont(), !pView->IsReadOnly() ? InputContextFlags::Text|InputContextFlags::ExtText : InputContextFlags::NONE ) );
    }

}

void TextEngine::SetMaxTextLen( sal_Int32 nLen )
{
    mnMaxTextLen = nLen>=0 ? nLen : EDIT_NOLIMIT;
}

void TextEngine::SetMaxTextWidth( tools::Long nMaxWidth )
{
    if ( nMaxWidth>=0 && nMaxWidth != mnMaxTextWidth )
    {
        mnMaxTextWidth = nMaxWidth;
        FormatFullDoc();
        UpdateViews();
    }
}

const sal_Unicode static_aLFText[] = { '\n', 0 };
const sal_Unicode static_aCRText[] = { '\r', 0 };
const sal_Unicode static_aCRLFText[] = { '\r', '\n', 0 };

static const sal_Unicode* static_getLineEndText( LineEnd aLineEnd )
{
    const sal_Unicode* pRet = nullptr;

    switch( aLineEnd )
    {
        case LINEEND_LF:
            pRet = static_aLFText;
            break;
        case LINEEND_CR:
            pRet = static_aCRText;
            break;
        case LINEEND_CRLF:
            pRet = static_aCRLFText;
            break;
    }
    return pRet;
}

void  TextEngine::ReplaceText(const TextSelection& rSel, const OUString& rText)
{
    ImpInsertText( rSel, rText );
}

OUString TextEngine::GetText( LineEnd aSeparator ) const
{
    return mpDoc->GetText( static_getLineEndText( aSeparator ) );
}

OUString TextEngine::GetTextLines( LineEnd aSeparator ) const
{
    OUStringBuffer aText;
    const sal_uInt32 nParas = mpTEParaPortions->Count();
    const sal_Unicode* pSep = static_getLineEndText( aSeparator );
    for ( sal_uInt32 nP = 0; nP < nParas; ++nP )
    {
        TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nP );

        const size_t nLines = pTEParaPortion->GetLines().size();
        for ( size_t nL = 0; nL < nLines; ++nL )
        {
            TextLine& rLine = pTEParaPortion->GetLines()[nL];
            aText.append( pTEParaPortion->GetNode()->GetText().subView(rLine.GetStart(), rLine.GetEnd() - rLine.GetStart()) );
            if ( pSep && ( ( (nP+1) < nParas ) || ( (nL+1) < nLines ) ) )
                aText.append(pSep);
        }
    }
    return aText.makeStringAndClear();
}

OUString TextEngine::GetText( sal_uInt32 nPara ) const
{
    return mpDoc->GetText( nPara );
}

sal_Int32 TextEngine::GetTextLen() const
{
    return mpDoc->GetTextLen( static_getLineEndText( LINEEND_LF ) );
}

sal_Int32 TextEngine::GetTextLen( const TextSelection& rSel ) const
{
    TextSelection aSel( rSel );
    aSel.Justify();
    ValidateSelection( aSel );
    return mpDoc->GetTextLen( static_getLineEndText( LINEEND_LF ), &aSel );
}

sal_Int32 TextEngine::GetTextLen( const sal_uInt32 nPara ) const
{
    return mpDoc->GetNodes()[ nPara ]->GetText().getLength();
}

void TextEngine::SetUpdateMode( bool bUpdate )
{
    if ( bUpdate != mbUpdate )
    {
        mbUpdate = bUpdate;
        if ( mbUpdate )
        {
            FormatAndUpdate( GetActiveView() );
            if ( GetActiveView() )
                GetActiveView()->ShowCursor();
        }
    }
}

bool TextEngine::DoesKeyChangeText( const KeyEvent& rKeyEvent )
{
    bool bDoesChange = false;

    KeyFuncType eFunc = rKeyEvent.GetKeyCode().GetFunction();
    if ( eFunc != KeyFuncType::DONTKNOW )
    {
        switch ( eFunc )
        {
            case KeyFuncType::UNDO:
            case KeyFuncType::REDO:
            case KeyFuncType::CUT:
            case KeyFuncType::PASTE:
                bDoesChange = true;
                break;
            default:
                // might get handled below
                eFunc = KeyFuncType::DONTKNOW;
        }
    }
    if ( eFunc == KeyFuncType::DONTKNOW )
    {
        switch ( rKeyEvent.GetKeyCode().GetCode() )
        {
            case KEY_DELETE:
            case KEY_BACKSPACE:
                if ( !rKeyEvent.GetKeyCode().IsMod2() )
                    bDoesChange = true;
                break;
            case KEY_RETURN:
            case KEY_TAB:
                if ( !rKeyEvent.GetKeyCode().IsMod1() && !rKeyEvent.GetKeyCode().IsMod2() )
                    bDoesChange = true;
                break;
            default:
                bDoesChange = TextEngine::IsSimpleCharInput( rKeyEvent );
        }
    }
    return bDoesChange;
}

bool TextEngine::IsSimpleCharInput( const KeyEvent& rKeyEvent )
{
    return rKeyEvent.GetCharCode() >= 32 && rKeyEvent.GetCharCode() != 127 &&
        KEY_MOD1 != (rKeyEvent.GetKeyCode().GetModifier() & ~KEY_SHIFT) && // (ssa) #i45714#:
        KEY_MOD2 != (rKeyEvent.GetKeyCode().GetModifier() & ~KEY_SHIFT);  // check for Ctrl and Alt separately
}

void TextEngine::ImpInitDoc()
{
    if ( mpDoc )
        mpDoc->Clear();
    else
        mpDoc.reset( new TextDoc );

    mpTEParaPortions.reset(new TEParaPortions);

    std::unique_ptr<TextNode> pNode(new TextNode( OUString() ));
    mpDoc->GetNodes().insert( mpDoc->GetNodes().begin(), std::move(pNode) );

    TEParaPortion* pIniPortion = new TEParaPortion( mpDoc->GetNodes().begin()->get() );
    mpTEParaPortions->Insert( pIniPortion, 0 );

    mbFormatted = false;

    ImpParagraphRemoved( TEXT_PARA_ALL );
    ImpParagraphInserted( 0 );
}

OUString TextEngine::GetText( const TextSelection& rSel, LineEnd aSeparator ) const
{
    if ( !rSel.HasRange() )
        return OUString();

    TextSelection aSel( rSel );
    aSel.Justify();

    OUStringBuffer aText;
    const sal_uInt32 nStartPara = aSel.GetStart().GetPara();
    const sal_uInt32 nEndPara = aSel.GetEnd().GetPara();
    const sal_Unicode* pSep = static_getLineEndText( aSeparator );
    for ( sal_uInt32 nNode = aSel.GetStart().GetPara(); nNode <= nEndPara; ++nNode )
    {
        TextNode* pNode = mpDoc->GetNodes()[ nNode ].get();

        sal_Int32 nStartPos = 0;
        sal_Int32 nEndPos = pNode->GetText().getLength();
        if ( nNode == nStartPara )
            nStartPos = aSel.GetStart().GetIndex();
        if ( nNode == nEndPara ) // may also be == nStart!
            nEndPos = aSel.GetEnd().GetIndex();

        aText.append(pNode->GetText().subView(nStartPos, nEndPos-nStartPos));
        if ( nNode < nEndPara )
            aText.append(pSep);
    }
    return aText.makeStringAndClear();
}

void TextEngine::ImpRemoveText()
{
    ImpInitDoc();

    const TextSelection aEmptySel;
    for (TextView* pView : *mpViews)
    {
        pView->ImpSetSelection( aEmptySel );
    }
    ResetUndo();
}

void TextEngine::SetText( const OUString& rText )
{
    ImpRemoveText();

    const bool bUndoCurrentlyEnabled = IsUndoEnabled();
    // the manually inserted text cannot be reversed by the user
    EnableUndo( false );

    const TextSelection aEmptySel;

    TextPaM aPaM;
    if ( !rText.isEmpty() )
        aPaM = ImpInsertText( aEmptySel, rText );

    for (TextView* pView : *mpViews)
    {
        pView->ImpSetSelection( aEmptySel );

        // if no text, then no Format&Update => the text remains
        if ( rText.isEmpty() && GetUpdateMode() )
            pView->Invalidate();
    }

    if( rText.isEmpty() )  // otherwise needs invalidation later; !bFormatted is sufficient
        mnCurTextHeight = 0;

    FormatAndUpdate();

    EnableUndo( bUndoCurrentlyEnabled );
    SAL_WARN_IF( HasUndoManager() && GetUndoManager().GetUndoActionCount(), "vcl", "SetText: Undo!" );
}

void TextEngine::CursorMoved( sal_uInt32 nNode )
{
    // delete empty attribute; but only if paragraph is not empty!
    TextNode* pNode = mpDoc->GetNodes()[ nNode ].get();
    if ( pNode && pNode->GetCharAttribs().HasEmptyAttribs() && !pNode->GetText().isEmpty() )
        pNode->GetCharAttribs().DeleteEmptyAttribs();
}

void TextEngine::ImpRemoveChars( const TextPaM& rPaM, sal_Int32 nChars )
{
    SAL_WARN_IF( !nChars, "vcl", "ImpRemoveChars: 0 Chars?!" );
    if ( IsUndoEnabled() && !IsInUndo() )
    {
        // attributes have to be saved for UNDO before RemoveChars!
        TextNode* pNode = mpDoc->GetNodes()[ rPaM.GetPara() ].get();
        OUString aStr( pNode->GetText().copy( rPaM.GetIndex(), nChars ) );

        // check if attributes are being deleted or changed
        const sal_Int32 nStart = rPaM.GetIndex();
        const sal_Int32 nEnd = nStart + nChars;
        for ( sal_uInt16 nAttr = pNode->GetCharAttribs().Count(); nAttr; )
        {
            TextCharAttrib& rAttr = pNode->GetCharAttribs().GetAttrib( --nAttr );
            if ( ( rAttr.GetEnd() >= nStart ) && ( rAttr.GetStart() < nEnd ) )
            {
                break;  // for
            }
        }
        InsertUndo( std::make_unique<TextUndoRemoveChars>( this, rPaM, aStr ) );
    }

    mpDoc->RemoveChars( rPaM, nChars );
    ImpCharsRemoved( rPaM.GetPara(), rPaM.GetIndex(), nChars );
}

TextPaM TextEngine::ImpConnectParagraphs( sal_uInt32 nLeft, sal_uInt32 nRight )
{
    SAL_WARN_IF( nLeft == nRight, "vcl", "ImpConnectParagraphs: connect the very same paragraph ?" );

    TextNode* pLeft = mpDoc->GetNodes()[ nLeft ].get();
    TextNode* pRight = mpDoc->GetNodes()[ nRight ].get();

    if ( IsUndoEnabled() && !IsInUndo() )
        InsertUndo( std::make_unique<TextUndoConnectParas>( this, nLeft, pLeft->GetText().getLength() ) );

    // first lookup Portions, as pRight is gone after ConnectParagraphs
    TEParaPortion* pLeftPortion = mpTEParaPortions->GetObject( nLeft );
    TEParaPortion* pRightPortion = mpTEParaPortions->GetObject( nRight );
    SAL_WARN_IF( !pLeft || !pLeftPortion, "vcl", "ImpConnectParagraphs(1): Hidden Portion" );
    SAL_WARN_IF( !pRight || !pRightPortion, "vcl", "ImpConnectParagraphs(2): Hidden Portion" );

    TextPaM aPaM = mpDoc->ConnectParagraphs( pLeft, pRight );
    ImpParagraphRemoved( nRight );

    pLeftPortion->MarkSelectionInvalid( aPaM.GetIndex() );

    mpTEParaPortions->Remove( nRight );
    // the right Node is deleted by EditDoc::ConnectParagraphs()

    return aPaM;
}

TextPaM TextEngine::ImpDeleteText( const TextSelection& rSel )
{
    if ( !rSel.HasRange() )
        return rSel.GetStart();

    TextSelection aSel( rSel );
    aSel.Justify();
    TextPaM aStartPaM( aSel.GetStart() );
    TextPaM aEndPaM( aSel.GetEnd() );

    CursorMoved( aStartPaM.GetPara() ); // so that newly-adjusted attributes vanish
    CursorMoved( aEndPaM.GetPara() );   // so that newly-adjusted attributes vanish

    SAL_WARN_IF( !mpDoc->IsValidPaM( aStartPaM ), "vcl", "ImpDeleteText(1): bad Index" );
    SAL_WARN_IF( !mpDoc->IsValidPaM( aEndPaM ), "vcl", "ImpDeleteText(2): bad Index" );

    const sal_uInt32 nStartNode = aStartPaM.GetPara();
    sal_uInt32 nEndNode = aEndPaM.GetPara();

    // remove all Nodes inbetween
    for ( sal_uInt32 z = nStartNode+1; z < nEndNode; ++z )
    {
        // always nStartNode+1, because of Remove()!
        ImpRemoveParagraph( nStartNode+1 );
    }

    if ( nStartNode != nEndNode )
    {
        // the remainder of StartNodes...
        TextNode* pLeft = mpDoc->GetNodes()[ nStartNode ].get();
        sal_Int32 nChars = pLeft->GetText().getLength() - aStartPaM.GetIndex();
        if ( nChars )
        {
            ImpRemoveChars( aStartPaM, nChars );
            TEParaPortion* pPortion = mpTEParaPortions->GetObject( nStartNode );
            SAL_WARN_IF( !pPortion, "vcl", "ImpDeleteText(3): bad Index" );
            pPortion->MarkSelectionInvalid( aStartPaM.GetIndex() );
        }

        // the beginning of EndNodes...
        nEndNode = nStartNode+1;    // the other paragraphs were deleted
        nChars = aEndPaM.GetIndex();
        if ( nChars )
        {
            aEndPaM.GetPara() = nEndNode;
            aEndPaM.GetIndex() = 0;
            ImpRemoveChars( aEndPaM, nChars );
            TEParaPortion* pPortion = mpTEParaPortions->GetObject( nEndNode );
            SAL_WARN_IF( !pPortion, "vcl", "ImpDeleteText(4): bad Index" );
            pPortion->MarkSelectionInvalid( 0 );
        }

        // connect...
        aStartPaM = ImpConnectParagraphs( nStartNode, nEndNode );
    }
    else
    {
        const sal_Int32 nChars = aEndPaM.GetIndex() - aStartPaM.GetIndex();
        ImpRemoveChars( aStartPaM, nChars );
        TEParaPortion* pPortion = mpTEParaPortions->GetObject( nStartNode );
        SAL_WARN_IF( !pPortion, "vcl", "ImpDeleteText(5): bad Index" );
        pPortion->MarkInvalid( aEndPaM.GetIndex(), aStartPaM.GetIndex() - aEndPaM.GetIndex() );
    }

//  UpdateSelections();
    TextModified();
    return aStartPaM;
}

void TextEngine::ImpRemoveParagraph( sal_uInt32 nPara )
{
    std::unique_ptr<TextNode> pNode = std::move(mpDoc->GetNodes()[ nPara ]);

    // the Node is handled by Undo and is deleted if appropriate
    mpDoc->GetNodes().erase( mpDoc->GetNodes().begin() + nPara );
    if ( IsUndoEnabled() && !IsInUndo() )
        InsertUndo( std::make_unique<TextUndoDelPara>( this, pNode.release(), nPara ) );

    mpTEParaPortions->Remove( nPara );

    ImpParagraphRemoved( nPara );
}

uno::Reference < i18n::XExtendedInputSequenceChecker > const & TextEngine::GetInputSequenceChecker()
{
    if ( !mxISC.is() )
    {
        mxISC = i18n::InputSequenceChecker::create( ::comphelper::getProcessComponentContext() );
    }
    return mxISC;
}

bool TextEngine::IsInputSequenceCheckingRequired( sal_Unicode c, const TextSelection& rCurSel ) const
{
    // get the index that really is first
    const sal_Int32 nFirstPos = std::min(rCurSel.GetStart().GetIndex(), rCurSel.GetEnd().GetIndex());

    bool bIsSequenceChecking =
        SvtCTLOptions::IsCTLFontEnabled() &&
        SvtCTLOptions::IsCTLSequenceChecking() &&
        nFirstPos != 0; /* first char needs not to be checked */

    if (bIsSequenceChecking)
    {
        uno::Reference< i18n::XBreakIterator > xBI = const_cast<TextEngine *>(this)->GetBreakIterator();
        bIsSequenceChecking = xBI.is() && i18n::ScriptType::COMPLEX == xBI->getScriptType( OUString( c ), 0 );
    }

    return bIsSequenceChecking;
}

TextPaM TextEngine::ImpInsertText( const TextSelection& rCurSel, sal_Unicode c, bool bOverwrite )
{
    return ImpInsertText( c, rCurSel, bOverwrite  );
}

TextPaM TextEngine::ImpInsertText( sal_Unicode c, const TextSelection& rCurSel, bool bOverwrite, bool bIsUserInput )
{
    SAL_WARN_IF( c == '\n', "vcl", "InsertText: NewLine!" );
    SAL_WARN_IF( c == '\r', "vcl", "InsertText: NewLine!" );

    TextPaM aPaM( rCurSel.GetStart() );
    TextNode* pNode = mpDoc->GetNodes()[ aPaM.GetPara() ].get();

    bool bDoOverwrite = bOverwrite && ( aPaM.GetIndex() < pNode->GetText().getLength() );

    bool bUndoAction = rCurSel.HasRange() || bDoOverwrite;

    if ( bUndoAction )
        UndoActionStart();

    if ( rCurSel.HasRange() )
    {
        aPaM = ImpDeleteText( rCurSel );
    }
    else if ( bDoOverwrite )
    {
        // if selection, then don't overwrite a character
        TextSelection aTmpSel( aPaM );
        ++aTmpSel.GetEnd().GetIndex();
        ImpDeleteText( aTmpSel );
    }

    if (bIsUserInput && IsInputSequenceCheckingRequired( c, rCurSel ))
    {
        uno::Reference < i18n::XExtendedInputSequenceChecker > xISC = GetInputSequenceChecker();

        if (xISC.is())
        {
            sal_Int32 nTmpPos = aPaM.GetIndex();
            sal_Int16 nCheckMode = SvtCTLOptions::IsCTLSequenceCheckingRestricted() ?
                    i18n::InputSequenceCheckMode::STRICT : i18n::InputSequenceCheckMode::BASIC;

            // the text that needs to be checked is only the one
            // before the current cursor position
            OUString aOldText( mpDoc->GetText( aPaM.GetPara() ).copy(0, nTmpPos) );
            if (SvtCTLOptions::IsCTLSequenceCheckingTypeAndReplace())
            {
                OUString aNewText( aOldText );
                xISC->correctInputSequence( aNewText, nTmpPos - 1, c, nCheckMode );

                // find position of first character that has changed
                const sal_Int32 nOldLen = aOldText.getLength();
                const sal_Int32 nNewLen = aNewText.getLength();
                const sal_Unicode *pOldTxt = aOldText.getStr();
                const sal_Unicode *pNewTxt = aNewText.getStr();
                sal_Int32 nChgPos = 0;
                while ( nChgPos < nOldLen && nChgPos < nNewLen &&
                        pOldTxt[nChgPos] == pNewTxt[nChgPos] )
                    ++nChgPos;

                OUString aChgText( aNewText.copy( nChgPos ) );

                // select text from first pos to be changed to current pos
                TextSelection aSel( TextPaM( aPaM.GetPara(), nChgPos ), aPaM );

                if (!aChgText.isEmpty())
                    // ImpInsertText implicitly handles undo...
                    return ImpInsertText( aSel, aChgText );
                else
                    return aPaM;
            }
            else
            {
                // should the character be ignored (i.e. not get inserted) ?
                if (!xISC->checkInputSequence( aOldText, nTmpPos - 1, c, nCheckMode ))
                    return aPaM;    // nothing to be done -> no need for undo
            }
        }

        // at this point now we will insert the character 'normally' some lines below...
    }

    if ( IsUndoEnabled() && !IsInUndo() )
    {
        std::unique_ptr<TextUndoInsertChars> pNewUndo(new TextUndoInsertChars( this, aPaM, OUString(c) ));
        bool bTryMerge = !bDoOverwrite && ( c != ' ' );
        InsertUndo( std::move(pNewUndo), bTryMerge );
    }

    TEParaPortion* pPortion = mpTEParaPortions->GetObject( aPaM.GetPara() );
    pPortion->MarkInvalid( aPaM.GetIndex(), 1 );
    if ( c == '\t' )
        pPortion->SetNotSimpleInvalid();
    aPaM = mpDoc->InsertText( aPaM, c );
    ImpCharsInserted( aPaM.GetPara(), aPaM.GetIndex()-1, 1 );

    TextModified();

    if ( bUndoAction )
        UndoActionEnd();

    return aPaM;
}

TextPaM TextEngine::ImpInsertText( const TextSelection& rCurSel, const OUString& rStr )
{
    UndoActionStart();

    TextPaM aPaM;

    if ( rCurSel.HasRange() )
        aPaM = ImpDeleteText( rCurSel );
    else
        aPaM = rCurSel.GetEnd();

    OUString aText(convertLineEnd(rStr, LINEEND_LF));

    sal_Int32 nStart = 0;
    while ( nStart < aText.getLength() )
    {
        sal_Int32 nEnd = aText.indexOf( LINE_SEP, nStart );
        if (nEnd == -1)
            nEnd = aText.getLength(); // do not dereference!

        // Start == End => empty line
        if ( nEnd > nStart )
        {
            OUString aLine(aText.copy(nStart, nEnd-nStart));
            if ( IsUndoEnabled() && !IsInUndo() )
                InsertUndo( std::make_unique<TextUndoInsertChars>( this, aPaM, aLine ) );

            TEParaPortion* pPortion = mpTEParaPortions->GetObject( aPaM.GetPara() );
            pPortion->MarkInvalid( aPaM.GetIndex(), aLine.getLength() );
            if (aLine.indexOf( '\t' ) != -1)
                pPortion->SetNotSimpleInvalid();

            aPaM = mpDoc->InsertText( aPaM, aLine );
            ImpCharsInserted( aPaM.GetPara(), aPaM.GetIndex()-aLine.getLength(), aLine.getLength() );

        }
        if ( nEnd < aText.getLength() )
            aPaM = ImpInsertParaBreak( aPaM );

        if ( nEnd == aText.getLength() )    // #108611# prevent overflow in "nStart = nEnd+1" calculation
            break;

        nStart = nEnd+1;
    }

    UndoActionEnd();

    TextModified();
    return aPaM;
}

TextPaM TextEngine::ImpInsertParaBreak( const TextSelection& rCurSel )
{
    TextPaM aPaM;
    if ( rCurSel.HasRange() )
        aPaM = ImpDeleteText( rCurSel );
    else
        aPaM = rCurSel.GetEnd();

    return ImpInsertParaBreak( aPaM );
}

TextPaM TextEngine::ImpInsertParaBreak( const TextPaM& rPaM )
{
    if ( IsUndoEnabled() && !IsInUndo() )
        InsertUndo( std::make_unique<TextUndoSplitPara>( this, rPaM.GetPara(), rPaM.GetIndex() ) );

    TextNode* pNode = mpDoc->GetNodes()[ rPaM.GetPara() ].get();
    bool bFirstParaContentChanged = rPaM.GetIndex() < pNode->GetText().getLength();

    TextPaM aPaM( mpDoc->InsertParaBreak( rPaM ) );

    TEParaPortion* pPortion = mpTEParaPortions->GetObject( rPaM.GetPara() );
    SAL_WARN_IF( !pPortion, "vcl", "ImpInsertParaBreak: Hidden Portion" );
    pPortion->MarkInvalid( rPaM.GetIndex(), 0 );

    TextNode* pNewNode = mpDoc->GetNodes()[ aPaM.GetPara() ].get();
    TEParaPortion* pNewPortion = new TEParaPortion( pNewNode );
    mpTEParaPortions->Insert( pNewPortion, aPaM.GetPara() );
    ImpParagraphInserted( aPaM.GetPara() );

    CursorMoved( rPaM.GetPara() );  // if empty attribute created
    TextModified();

    if ( bFirstParaContentChanged )
        Broadcast( TextHint( SfxHintId::TextParaContentChanged, rPaM.GetPara() ) );

    return aPaM;
}

tools::Rectangle TextEngine::PaMtoEditCursor( const TextPaM& rPaM, bool bSpecial )
{
    SAL_WARN_IF( !GetUpdateMode(), "vcl", "PaMtoEditCursor: GetUpdateMode()" );

    tools::Rectangle aEditCursor;
    tools::Long nY = 0;

    if ( !mbHasMultiLineParas )
    {
        nY = rPaM.GetPara() * mnCharHeight;
    }
    else
    {
        for ( sal_uInt32 nPortion = 0; nPortion < rPaM.GetPara(); ++nPortion )
        {
            TEParaPortion* pPortion = mpTEParaPortions->GetObject(nPortion);
            nY += pPortion->GetLines().size() * mnCharHeight;
        }
    }

    aEditCursor = GetEditCursor( rPaM, bSpecial );
    aEditCursor.AdjustTop(nY );
    aEditCursor.AdjustBottom(nY );
    return aEditCursor;
}

tools::Rectangle TextEngine::GetEditCursor( const TextPaM& rPaM, bool bSpecial, bool bPreferPortionStart )
{
    if ( !IsFormatted() && !IsFormatting() )
        FormatAndUpdate();

    TEParaPortion* pPortion = mpTEParaPortions->GetObject( rPaM.GetPara() );
    //TextNode* pNode = mpDoc->GetNodes().GetObject( rPaM.GetPara() );

    /*
      bSpecial: If behind the last character of a made up line, stay at the
                  end of the line, not at the start of the next line.
      Purpose:  - really END = > behind the last character
                - to selection...

    */

    tools::Long nY = 0;
    sal_Int32 nCurIndex = 0;
    TextLine* pLine = nullptr;
    for (TextLine & rTmpLine : pPortion->GetLines())
    {
        if ( ( rTmpLine.GetStart() == rPaM.GetIndex() ) || ( rTmpLine.IsIn( rPaM.GetIndex(), bSpecial ) ) )
        {
            pLine = &rTmpLine;
            break;
        }

        nCurIndex = nCurIndex + rTmpLine.GetLen();
        nY += mnCharHeight;
    }
    if ( !pLine )
    {
        // Cursor at end of paragraph
        SAL_WARN_IF( rPaM.GetIndex() != nCurIndex, "vcl", "GetEditCursor: Bad Index!" );

        pLine = & ( pPortion->GetLines().back() );
        nY -= mnCharHeight;
    }

    tools::Rectangle aEditCursor;

    aEditCursor.SetTop( nY );
    nY += mnCharHeight;
    aEditCursor.SetBottom( nY-1 );

    // search within the line
    tools::Long nX = ImpGetXPos( rPaM.GetPara(), pLine, rPaM.GetIndex(), bPreferPortionStart );
    aEditCursor.SetLeft(nX);
    aEditCursor.SetRight(nX);
    return aEditCursor;
}

tools::Long TextEngine::ImpGetXPos( sal_uInt32 nPara, TextLine* pLine, sal_Int32 nIndex, bool bPreferPortionStart )
{
    SAL_WARN_IF( ( nIndex < pLine->GetStart() ) || ( nIndex > pLine->GetEnd() ) , "vcl", "ImpGetXPos: Bad parameters!" );

    bool bDoPreferPortionStart = bPreferPortionStart;
    // Assure that the portion belongs to this line
    if ( nIndex == pLine->GetStart() )
        bDoPreferPortionStart = true;
    else if ( nIndex == pLine->GetEnd() )
        bDoPreferPortionStart = false;

    TEParaPortion* pParaPortion = mpTEParaPortions->GetObject( nPara );

    sal_Int32 nTextPortionStart = 0;
    std::size_t nTextPortion = pParaPortion->GetTextPortions().FindPortion( nIndex, nTextPortionStart, bDoPreferPortionStart );

    SAL_WARN_IF( ( nTextPortion < pLine->GetStartPortion() ) || ( nTextPortion > pLine->GetEndPortion() ), "vcl", "GetXPos: Portion not in current line!" );

    TETextPortion& rPortion = pParaPortion->GetTextPortions()[ nTextPortion ];

    tools::Long nX = ImpGetPortionXOffset( nPara, pLine, nTextPortion );

    tools::Long nPortionTextWidth = rPortion.GetWidth();

    if ( nTextPortionStart != nIndex )
    {
        // Search within portion...
        if ( nIndex == ( nTextPortionStart + rPortion.GetLen() ) )
        {
            // End of Portion
            if ( ( rPortion.GetKind() == PORTIONKIND_TAB ) ||
                 ( !IsRightToLeft() && !rPortion.IsRightToLeft() ) ||
                 ( IsRightToLeft() && rPortion.IsRightToLeft() ) )
            {
                nX += nPortionTextWidth;
                if ( ( rPortion.GetKind() == PORTIONKIND_TAB ) && ( (nTextPortion+1) < pParaPortion->GetTextPortions().size() ) )
                {
                    TETextPortion& rNextPortion = pParaPortion->GetTextPortions()[ nTextPortion+1 ];
                    if (rNextPortion.GetKind() != PORTIONKIND_TAB && IsRightToLeft() != rNextPortion.IsRightToLeft())
                    {
                        // End of the tab portion, use start of next for cursor pos
                        SAL_WARN_IF( bPreferPortionStart, "vcl", "ImpGetXPos: How can we get here!" );
                        nX = ImpGetXPos( nPara, pLine, nIndex, true );
                    }

                }
            }
        }
        else if ( rPortion.GetKind() == PORTIONKIND_TEXT )
        {
            SAL_WARN_IF( nIndex == pLine->GetStart(), "vcl", "ImpGetXPos: Strange behavior" );

            tools::Long nPosInPortion = CalcTextWidth( nPara, nTextPortionStart, nIndex-nTextPortionStart );

            if (IsRightToLeft() == rPortion.IsRightToLeft())
            {
                nX += nPosInPortion;
            }
            else
            {
                nX += nPortionTextWidth - nPosInPortion;
            }
        }
    }
    else // if ( nIndex == pLine->GetStart() )
    {
        if (rPortion.GetKind() != PORTIONKIND_TAB && IsRightToLeft() != rPortion.IsRightToLeft())
        {
            nX += nPortionTextWidth;
        }
    }

    return nX;
}

const TextAttrib* TextEngine::FindAttrib( const TextPaM& rPaM, sal_uInt16 nWhich ) const
{
    const TextAttrib* pAttr = nullptr;
    const TextCharAttrib* pCharAttr = FindCharAttrib( rPaM, nWhich );
    if ( pCharAttr )
        pAttr = &pCharAttr->GetAttr();
    return pAttr;
}

const TextCharAttrib* TextEngine::FindCharAttrib( const TextPaM& rPaM, sal_uInt16 nWhich ) const
{
    const TextCharAttrib* pAttr = nullptr;
    TextNode* pNode = mpDoc->GetNodes()[ rPaM.GetPara() ].get();
    if (pNode && (rPaM.GetIndex() <= pNode->GetText().getLength()))
        pAttr = pNode->GetCharAttribs().FindAttrib( nWhich, rPaM.GetIndex() );
    return pAttr;
}

TextPaM TextEngine::GetPaM( const Point& rDocPos )
{
    SAL_WARN_IF( !GetUpdateMode(), "vcl", "GetPaM: GetUpdateMode()" );

    tools::Long nY = 0;
    for ( sal_uInt32 nPortion = 0; nPortion < mpTEParaPortions->Count(); ++nPortion )
    {
        TEParaPortion* pPortion = mpTEParaPortions->GetObject( nPortion );
        tools::Long nTmpHeight = pPortion->GetLines().size() * mnCharHeight;
        nY += nTmpHeight;
        if ( nY > rDocPos.Y() )
        {
            nY -= nTmpHeight;
            Point aPosInPara( rDocPos );
            aPosInPara.AdjustY( -nY );

            TextPaM aPaM( nPortion, 0 );
            aPaM.GetIndex() = ImpFindIndex( nPortion, aPosInPara );
            return aPaM;
        }
    }

    // not found - go to last visible
    const sal_uInt32 nLastNode = static_cast<sal_uInt32>(mpDoc->GetNodes().size() - 1);
    TextNode* pLast = mpDoc->GetNodes()[ nLastNode ].get();
    return TextPaM( nLastNode, pLast->GetText().getLength() );
}

sal_Int32 TextEngine::ImpFindIndex( sal_uInt32 nPortion, const Point& rPosInPara )
{
    SAL_WARN_IF( !IsFormatted(), "vcl", "GetPaM: Not formatted" );
    TEParaPortion* pPortion = mpTEParaPortions->GetObject( nPortion );

    sal_Int32 nCurIndex = 0;

    tools::Long nY = 0;
    TextLine* pLine = nullptr;
    std::vector<TextLine>::size_type nLine;
    for ( nLine = 0; nLine < pPortion->GetLines().size(); nLine++ )
    {
        TextLine& rmpLine = pPortion->GetLines()[ nLine ];
        nY += mnCharHeight;
        if ( nY > rPosInPara.Y() )  // that's it
        {
            pLine = &rmpLine;
            break;                  // correct Y-Position not needed
        }
    }

    assert(pLine && "ImpFindIndex: pLine ?");

    nCurIndex = GetCharPos( nPortion, nLine, rPosInPara.X() );

    if ( nCurIndex && ( nCurIndex == pLine->GetEnd() ) &&
         ( pLine != &( pPortion->GetLines().back() ) ) )
    {
        uno::Reference < i18n::XBreakIterator > xBI = GetBreakIterator();
        sal_Int32 nCount = 1;
        nCurIndex = xBI->previousCharacters( pPortion->GetNode()->GetText(), nCurIndex, GetLocale(), i18n::CharacterIteratorMode::SKIPCELL, nCount, nCount );
    }
    return nCurIndex;
}

sal_Int32 TextEngine::GetCharPos( sal_uInt32 nPortion, std::vector<TextLine>::size_type nLine, tools::Long nXPos )
{

    TEParaPortion* pPortion = mpTEParaPortions->GetObject( nPortion );
    TextLine& rLine = pPortion->GetLines()[ nLine ];

    sal_Int32 nCurIndex = rLine.GetStart();

    tools::Long nTmpX = rLine.GetStartX();
    if ( nXPos <= nTmpX )
        return nCurIndex;

    for ( std::size_t i = rLine.GetStartPortion(); i <= rLine.GetEndPortion(); i++ )
    {
        TETextPortion& rTextPortion = pPortion->GetTextPortions()[ i ];
        nTmpX += rTextPortion.GetWidth();

        if ( nTmpX > nXPos )
        {
            if( rTextPortion.GetLen() > 1 )
            {
                nTmpX -= rTextPortion.GetWidth();  // position before Portion
                // TODO: Optimize: no GetTextBreak if fixed-width Font
                vcl::Font aFont;
                SeekCursor( nPortion, nCurIndex+1, aFont, nullptr );
                mpRefDev->SetFont( aFont);
                tools::Long nPosInPortion = nXPos-nTmpX;
                if ( IsRightToLeft() != rTextPortion.IsRightToLeft() )
                    nPosInPortion = rTextPortion.GetWidth() - nPosInPortion;
                nCurIndex = mpRefDev->GetTextBreak( pPortion->GetNode()->GetText(), nPosInPortion, nCurIndex );
                // MT: GetTextBreak should assure that we are not within a CTL cell...
            }
            return nCurIndex;
        }
        nCurIndex += rTextPortion.GetLen();
    }
    return nCurIndex;
}

tools::Long TextEngine::GetTextHeight() const
{
    SAL_WARN_IF( !GetUpdateMode(), "vcl", "GetTextHeight: GetUpdateMode()" );

    if ( !IsFormatted() && !IsFormatting() )
        const_cast<TextEngine*>(this)->FormatAndUpdate();

    return mnCurTextHeight;
}

tools::Long TextEngine::GetTextHeight( sal_uInt32 nParagraph ) const
{
    SAL_WARN_IF( !GetUpdateMode(), "vcl", "GetTextHeight: GetUpdateMode()" );

    if ( !IsFormatted() && !IsFormatting() )
        const_cast<TextEngine*>(this)->FormatAndUpdate();

    return CalcParaHeight( nParagraph );
}

tools::Long TextEngine::CalcTextWidth( sal_uInt32 nPara )
{
    tools::Long nParaWidth = 0;
    TEParaPortion* pPortion = mpTEParaPortions->GetObject( nPara );
    for ( auto nLine = pPortion->GetLines().size(); nLine; )
    {
        tools::Long nLineWidth = 0;
        TextLine& rLine = pPortion->GetLines()[ --nLine ];
        for ( std::size_t nTP = rLine.GetStartPortion(); nTP <= rLine.GetEndPortion(); nTP++ )
        {
            TETextPortion& rTextPortion = pPortion->GetTextPortions()[ nTP ];
            nLineWidth += rTextPortion.GetWidth();
        }
        if ( nLineWidth > nParaWidth )
            nParaWidth = nLineWidth;
    }
    return nParaWidth;
}

tools::Long TextEngine::CalcTextWidth()
{
    if ( !IsFormatted() && !IsFormatting() )
        FormatAndUpdate();

    if ( mnCurTextWidth < 0 )
    {
        mnCurTextWidth = 0;
        for ( sal_uInt32 nPara = mpTEParaPortions->Count(); nPara; )
        {
            const tools::Long nParaWidth = CalcTextWidth( --nPara );
            if ( nParaWidth > mnCurTextWidth )
                mnCurTextWidth = nParaWidth;
        }
    }
    return mnCurTextWidth+1;// wider by 1, as CreateLines breaks at >=
}

tools::Long TextEngine::CalcTextHeight() const
{
    SAL_WARN_IF( !GetUpdateMode(), "vcl", "CalcTextHeight: GetUpdateMode()" );

    tools::Long nY = 0;
    for ( auto nPortion = mpTEParaPortions->Count(); nPortion; )
        nY += CalcParaHeight( --nPortion );
    return nY;
}

tools::Long TextEngine::CalcTextWidth( sal_uInt32 nPara, sal_Int32 nPortionStart, sal_Int32 nLen )
{
#ifdef DBG_UTIL
    // within the text there must not be a Portion change (attribute/tab)!
    sal_Int32 nTabPos = mpDoc->GetNodes()[ nPara ]->GetText().indexOf( '\t', nPortionStart );
    SAL_WARN_IF( nTabPos != -1 && nTabPos < (nPortionStart+nLen), "vcl", "CalcTextWidth: Tab!" );
#endif

    vcl::Font aFont;
    SeekCursor( nPara, nPortionStart+1, aFont, nullptr );
    mpRefDev->SetFont( aFont );
    TextNode* pNode = mpDoc->GetNodes()[ nPara ].get();
    tools::Long nWidth = mpRefDev->GetTextWidth( pNode->GetText(), nPortionStart, nLen );
    return nWidth;
}

void TextEngine::GetTextPortionRange(const TextPaM& rPaM, sal_Int32& nStart, sal_Int32& nEnd)
{
    nStart = 0;
    nEnd = 0;
    TEParaPortion* pParaPortion = mpTEParaPortions->GetObject( rPaM.GetPara() );
    for ( std::size_t i = 0; i < pParaPortion->GetTextPortions().size(); ++i )
    {
        TETextPortion& rTextPortion = pParaPortion->GetTextPortions()[ i ];
        if (nStart + rTextPortion.GetLen() > rPaM.GetIndex())
        {
            nEnd = nStart + rTextPortion.GetLen();
            return;
        }
        else
        {
            nStart += rTextPortion.GetLen();
        }
    }
}

sal_uInt16 TextEngine::GetLineCount( sal_uInt32 nParagraph ) const
{
    SAL_WARN_IF( nParagraph >= mpTEParaPortions->Count(), "vcl", "GetLineCount: Out of range" );

    TEParaPortion* pPPortion = mpTEParaPortions->GetObject( nParagraph );
    if ( pPPortion )
        return pPPortion->GetLines().size();

    return 0;
}

sal_Int32 TextEngine::GetLineLen( sal_uInt32 nParagraph, sal_uInt16 nLine ) const
{
    SAL_WARN_IF( nParagraph >= mpTEParaPortions->Count(), "vcl", "GetLineCount: Out of range" );

    TEParaPortion* pPPortion = mpTEParaPortions->GetObject( nParagraph );
    if ( pPPortion && ( nLine < pPPortion->GetLines().size() ) )
    {
        return pPPortion->GetLines()[ nLine ].GetLen();
    }

    return 0;
}

tools::Long TextEngine::CalcParaHeight( sal_uInt32 nParagraph ) const
{
    tools::Long nHeight = 0;

    TEParaPortion* pPPortion = mpTEParaPortions->GetObject( nParagraph );
    SAL_WARN_IF( !pPPortion, "vcl", "GetParaHeight: paragraph not found" );
    if ( pPPortion )
        nHeight = pPPortion->GetLines().size() * mnCharHeight;

    return nHeight;
}

Range TextEngine::GetInvalidYOffsets( sal_uInt32 nPortion )
{
    TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPortion );
    sal_uInt16 nLines = pTEParaPortion->GetLines().size();
    sal_uInt16 nLastInvalid, nFirstInvalid = 0;
    sal_uInt16 nLine;
    for ( nLine = 0; nLine < nLines; nLine++ )
    {
        TextLine& rL = pTEParaPortion->GetLines()[ nLine ];
        if ( rL.IsInvalid() )
        {
            nFirstInvalid = nLine;
            break;
        }
    }

    for ( nLastInvalid = nFirstInvalid; nLastInvalid < nLines; nLastInvalid++ )
    {
        TextLine& rL = pTEParaPortion->GetLines()[ nLine ];
        if ( rL.IsValid() )
            break;
    }

    if ( nLastInvalid >= nLines )
        nLastInvalid = nLines-1;

    return Range( nFirstInvalid*mnCharHeight, ((nLastInvalid+1)*mnCharHeight)-1 );
}

sal_uInt32 TextEngine::GetParagraphCount() const
{
    return static_cast<sal_uInt32>(mpDoc->GetNodes().size());
}

void TextEngine::EnableUndo( bool bEnable )
{
    // delete list when switching mode
    if ( bEnable != IsUndoEnabled() )
        ResetUndo();

    mbUndoEnabled = bEnable;
}

SfxUndoManager& TextEngine::GetUndoManager()
{
    if ( !mpUndoManager )
        mpUndoManager.reset( new TextUndoManager( this ) );
    return *mpUndoManager;
}

void TextEngine::UndoActionStart( sal_uInt16 nId )
{
    if ( IsUndoEnabled() && !IsInUndo() )
    {
        GetUndoManager().EnterListAction( OUString(), OUString(), nId, ViewShellId(-1) );
    }
}

void TextEngine::UndoActionEnd()
{
    if ( IsUndoEnabled() && !IsInUndo() )
        GetUndoManager().LeaveListAction();
}

void TextEngine::InsertUndo( std::unique_ptr<TextUndo> pUndo, bool bTryMerge )
{
    SAL_WARN_IF( IsInUndo(), "vcl", "InsertUndo: in Undo mode!" );
    GetUndoManager().AddUndoAction( std::move(pUndo), bTryMerge );
}

void TextEngine::ResetUndo()
{
    if ( mpUndoManager )
        mpUndoManager->Clear();
}

void TextEngine::InsertContent( std::unique_ptr<TextNode> pNode, sal_uInt32 nPara )
{
    SAL_WARN_IF( !pNode, "vcl", "InsertContent: NULL-Pointer!" );
    SAL_WARN_IF( !IsInUndo(), "vcl", "InsertContent: only in Undo()!" );
    TEParaPortion* pNew = new TEParaPortion( pNode.get() );
    mpTEParaPortions->Insert( pNew, nPara );
    mpDoc->GetNodes().insert( mpDoc->GetNodes().begin() + nPara, std::move(pNode) );
    ImpParagraphInserted( nPara );
}

TextPaM TextEngine::SplitContent( sal_uInt32 nNode, sal_Int32 nSepPos )
{
#ifdef DBG_UTIL
    TextNode* pNode = mpDoc->GetNodes()[ nNode ].get();
    assert(pNode && "SplitContent: Invalid Node!");
    SAL_WARN_IF( !IsInUndo(), "vcl", "SplitContent: only in Undo()!" );
    SAL_WARN_IF( nSepPos > pNode->GetText().getLength(), "vcl", "SplitContent: Bad index" );
#endif
    TextPaM aPaM( nNode, nSepPos );
    return ImpInsertParaBreak( aPaM );
}

TextPaM TextEngine::ConnectContents( sal_uInt32 nLeftNode )
{
    SAL_WARN_IF( !IsInUndo(), "vcl", "ConnectContent: only in Undo()!" );
    return ImpConnectParagraphs( nLeftNode, nLeftNode+1 );
}

void TextEngine::SeekCursor( sal_uInt32 nPara, sal_Int32 nPos, vcl::Font& rFont, OutputDevice* pOutDev )
{
    rFont = maFont;
    if ( pOutDev )
        pOutDev->SetTextColor( maTextColor );

    TextNode* pNode = mpDoc->GetNodes()[ nPara ].get();
    sal_uInt16 nAttribs = pNode->GetCharAttribs().Count();
    for ( sal_uInt16 nAttr = 0; nAttr < nAttribs; nAttr++ )
    {
        TextCharAttrib& rAttrib = pNode->GetCharAttribs().GetAttrib( nAttr );
        if ( rAttrib.GetStart() > nPos )
            break;

        // When seeking don't use Attr that start there!
        // Do not use empty attributes:
        // - If just being setup and empty => no effect on Font
        // - Characters that are setup in an empty paragraph become visible right away.
        if ( ( ( rAttrib.GetStart() < nPos ) && ( rAttrib.GetEnd() >= nPos ) )
                    || pNode->GetText().isEmpty() )
        {
            if ( rAttrib.Which() != TEXTATTR_FONTCOLOR )
            {
                rAttrib.GetAttr().SetFont(rFont);
            }
            else
            {
                if ( pOutDev )
                    pOutDev->SetTextColor( static_cast<const TextAttribFontColor&>(rAttrib.GetAttr()).GetColor() );
            }
        }
    }

    if ( !(mpIMEInfos && mpIMEInfos->pAttribs && ( mpIMEInfos->aPos.GetPara() == nPara ) &&
        ( nPos > mpIMEInfos->aPos.GetIndex() ) && ( nPos <= ( mpIMEInfos->aPos.GetIndex() + mpIMEInfos->nLen ) )) )
        return;

    ExtTextInputAttr nAttr = mpIMEInfos->pAttribs[ nPos - mpIMEInfos->aPos.GetIndex() - 1 ];
    if ( nAttr & ExtTextInputAttr::Underline )
        rFont.SetUnderline( LINESTYLE_SINGLE );
    else if ( nAttr & ExtTextInputAttr::DoubleUnderline )
        rFont.SetUnderline( LINESTYLE_DOUBLE );
    else if ( nAttr & ExtTextInputAttr::BoldUnderline )
        rFont.SetUnderline( LINESTYLE_BOLD );
    else if ( nAttr & ExtTextInputAttr::DottedUnderline )
        rFont.SetUnderline( LINESTYLE_DOTTED );
    else if ( nAttr & ExtTextInputAttr::DashDotUnderline )
        rFont.SetUnderline( LINESTYLE_DOTTED );
    if ( nAttr & ExtTextInputAttr::RedText )
        rFont.SetColor( COL_RED );
    else if ( nAttr & ExtTextInputAttr::HalfToneText )
        rFont.SetColor( COL_LIGHTGRAY );
    if ( nAttr & ExtTextInputAttr::Highlight )
    {
        const StyleSettings& rStyleSettings = Application::GetSettings().GetStyleSettings();
        rFont.SetColor( rStyleSettings.GetHighlightTextColor() );
        rFont.SetFillColor( rStyleSettings.GetHighlightColor() );
        rFont.SetTransparent( false );
    }
    else if ( nAttr & ExtTextInputAttr::GrayWaveline )
    {
        rFont.SetUnderline( LINESTYLE_WAVE );
//          if( pOut )
//              pOut->SetTextLineColor( COL_LIGHTGRAY );
    }
}

void TextEngine::FormatAndUpdate( TextView* pCurView )
{
    if ( mbDowning )
        return;

    if ( IsInUndo() )
        IdleFormatAndUpdate( pCurView );
    else
    {
        FormatDoc();
        UpdateViews( pCurView );
    }
}

void TextEngine::IdleFormatAndUpdate( TextView* pCurView, sal_uInt16 nMaxTimerRestarts )
{
    mpIdleFormatter->DoIdleFormat( pCurView, nMaxTimerRestarts );
}

void TextEngine::TextModified()
{
    mbFormatted = false;
    mbModified = true;
}

void TextEngine::UpdateViews( TextView* pCurView )
{
    if ( !GetUpdateMode() || IsFormatting() || maInvalidRect.IsEmpty() )
        return;

    SAL_WARN_IF( !IsFormatted(), "vcl", "UpdateViews: Doc not formatted!" );

    for (TextView* pView : *mpViews)
    {
        pView->HideCursor();

        tools::Rectangle aClipRect( maInvalidRect );
        const Size aOutSz = pView->GetWindow()->GetOutputSizePixel();
        const tools::Rectangle aVisArea( pView->GetStartDocPos(), aOutSz );
        aClipRect.Intersection( aVisArea );
        if ( !aClipRect.IsEmpty() )
        {
            // translate into window coordinates
            Point aNewPos = pView->GetWindowPos( aClipRect.TopLeft() );
            if ( IsRightToLeft() )
                aNewPos.AdjustX( -(aOutSz.Width() - 1) );
            aClipRect.SetPos( aNewPos );

            pView->GetWindow()->Invalidate( aClipRect );
        }
    }

    if ( pCurView )
    {
        pCurView->ShowCursor( pCurView->IsAutoScroll() );
    }

    maInvalidRect = tools::Rectangle();
}

IMPL_LINK_NOARG(TextEngine, IdleFormatHdl, Timer *, void)
{
    FormatAndUpdate( mpIdleFormatter->GetView() );
}

void TextEngine::CheckIdleFormatter()
{
    mpIdleFormatter->ForceTimeout();
}

void TextEngine::FormatFullDoc()
{
    for ( sal_uInt32 nPortion = 0; nPortion < mpTEParaPortions->Count(); ++nPortion )
    {
        TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPortion );
        pTEParaPortion->MarkSelectionInvalid( 0 );
    }
    mbFormatted = false;
    FormatDoc();
}

void TextEngine::FormatDoc()
{
    if ( IsFormatted() || !GetUpdateMode() || IsFormatting() )
        return;

    mbIsFormatting = true;
    mbHasMultiLineParas = false;

    tools::Long nY = 0;
    bool bGrow = false;

    maInvalidRect = tools::Rectangle(); // clear
    for ( sal_uInt32 nPara = 0; nPara < mpTEParaPortions->Count(); ++nPara )
    {
        TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );
        if ( pTEParaPortion->IsInvalid() )
        {
            const tools::Long nOldParaWidth = mnCurTextWidth >= 0 ? CalcTextWidth( nPara ) : -1;

            Broadcast( TextHint( SfxHintId::TextFormatPara, nPara ) );

            if ( CreateLines( nPara ) )
                bGrow = true;

            // set InvalidRect only once
            if ( maInvalidRect.IsEmpty() )
            {
                // otherwise remains Empty() for Paperwidth 0 (AutoPageSize)
                const tools::Long nWidth = mnMaxTextWidth
                    ? mnMaxTextWidth
                    : std::numeric_limits<tools::Long>::max();
                const Range aInvRange( GetInvalidYOffsets( nPara ) );
                maInvalidRect = tools::Rectangle( Point( 0, nY+aInvRange.Min() ),
                    Size( nWidth, aInvRange.Len() ) );
            }
            else
            {
                maInvalidRect.SetBottom( nY + CalcParaHeight( nPara ) );
            }

            if ( mnCurTextWidth >= 0 )
            {
                const tools::Long nNewParaWidth = CalcTextWidth( nPara );
                if ( nNewParaWidth >= mnCurTextWidth )
                    mnCurTextWidth = nNewParaWidth;
                else if ( nOldParaWidth >= mnCurTextWidth )
                    mnCurTextWidth = -1;
            }
        }
        else if ( bGrow )
        {
            maInvalidRect.SetBottom( nY + CalcParaHeight( nPara ) );
        }
        nY += CalcParaHeight( nPara );
        if ( !mbHasMultiLineParas && pTEParaPortion->GetLines().size() > 1 )
            mbHasMultiLineParas = true;
    }

    if ( !maInvalidRect.IsEmpty() )
    {
        const tools::Long nNewHeight = CalcTextHeight();
        const tools::Long nDiff = nNewHeight - mnCurTextHeight;
        if ( nNewHeight < mnCurTextHeight )
        {
            maInvalidRect.SetBottom( std::max( nNewHeight, mnCurTextHeight ) );
            if ( maInvalidRect.IsEmpty() )
            {
                maInvalidRect.SetTop( 0 );
                // Left and Right are not evaluated, but set because of IsEmpty
                maInvalidRect.SetLeft( 0 );
                maInvalidRect.SetRight( mnMaxTextWidth );
            }
        }

        mnCurTextHeight = nNewHeight;
        if ( nDiff )
        {
            mbFormatted = true;
            Broadcast( TextHint( SfxHintId::TextHeightChanged ) );
        }
    }

    mbIsFormatting = false;
    mbFormatted = true;

    Broadcast( TextHint( SfxHintId::TextFormatted ) );
}

void TextEngine::CreateAndInsertEmptyLine( sal_uInt32 nPara )
{
    TextNode* pNode = mpDoc->GetNodes()[ nPara ].get();
    TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );

    TextLine aTmpLine;
    aTmpLine.SetStart( pNode->GetText().getLength() );
    aTmpLine.SetEnd( aTmpLine.GetStart() );

    if ( ImpGetAlign() == TxtAlign::Center )
        aTmpLine.SetStartX( static_cast<short>(mnMaxTextWidth / 2) );
    else if ( ImpGetAlign() == TxtAlign::Right )
        aTmpLine.SetStartX( static_cast<short>(mnMaxTextWidth) );
    else
        aTmpLine.SetStartX( mpDoc->GetLeftMargin() );

    bool bLineBreak = !pNode->GetText().isEmpty();

    TETextPortion aDummyPortion( 0 );
    aDummyPortion.GetWidth() = 0;
    pTEParaPortion->GetTextPortions().push_back( aDummyPortion );

    if ( bLineBreak )
    {
        // -2: The new one is already inserted.
        const std::size_t nPos = pTEParaPortion->GetTextPortions().size() - 1;
        aTmpLine.SetStartPortion( nPos );
        aTmpLine.SetEndPortion( nPos );
    }
    pTEParaPortion->GetLines().push_back( aTmpLine );
}

void TextEngine::ImpBreakLine( sal_uInt32 nPara, TextLine* pLine, sal_Int32 nPortionStart, tools::Long nRemainingWidth )
{
    TextNode* pNode = mpDoc->GetNodes()[ nPara ].get();

    // Font still should be adjusted
    sal_Int32 nMaxBreakPos = mpRefDev->GetTextBreak( pNode->GetText(), nRemainingWidth, nPortionStart );

    SAL_WARN_IF( nMaxBreakPos >= pNode->GetText().getLength(), "vcl", "ImpBreakLine: Break?!" );

    if ( nMaxBreakPos == -1 )   // GetTextBreak() != GetTextSize()
        nMaxBreakPos = pNode->GetText().getLength() - 1;

    uno::Reference < i18n::XBreakIterator > xBI = GetBreakIterator();
    i18n::LineBreakHyphenationOptions aHyphOptions( nullptr, uno::Sequence< beans::PropertyValue >(), 1 );

    i18n::LineBreakUserOptions aUserOptions;
    aUserOptions.forbiddenBeginCharacters = ImpGetLocaleDataWrapper()->getForbiddenCharacters().beginLine;
    aUserOptions.forbiddenEndCharacters = ImpGetLocaleDataWrapper()->getForbiddenCharacters().endLine;
    aUserOptions.applyForbiddenRules = true;
    aUserOptions.allowPunctuationOutsideMargin = false;
    aUserOptions.allowHyphenateEnglish = false;

    static const css::lang::Locale aDefLocale;
    i18n::LineBreakResults aLBR = xBI->getLineBreak( pNode->GetText(), nMaxBreakPos, aDefLocale, pLine->GetStart(), aHyphOptions, aUserOptions );
    sal_Int32 nBreakPos = aLBR.breakIndex;
    if ( nBreakPos <= pLine->GetStart() )
    {
        nBreakPos = nMaxBreakPos;
        if ( nBreakPos <= pLine->GetStart() )
            nBreakPos = pLine->GetStart() + 1;  // infinite loop otherwise!
    }

    // the damaged Portion is the End Portion
    pLine->SetEnd( nBreakPos );
    const std::size_t nEndPortion = SplitTextPortion( nPara, nBreakPos );

    if ( nBreakPos >= pLine->GetStart() &&
         nBreakPos < pNode->GetText().getLength() &&
         pNode->GetText()[ nBreakPos ] == ' ' )
    {
        // generally suppress blanks at the end of line
        TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );
        TETextPortion& rTP = pTEParaPortion->GetTextPortions()[ nEndPortion ];
        SAL_WARN_IF( nBreakPos <= pLine->GetStart(), "vcl", "ImpBreakLine: SplitTextPortion at beginning of line?" );
        rTP.GetWidth() = CalcTextWidth( nPara, nBreakPos-rTP.GetLen(), rTP.GetLen()-1 );
    }
    pLine->SetEndPortion( nEndPortion );
}

std::size_t TextEngine::SplitTextPortion( sal_uInt32 nPara, sal_Int32 nPos )
{

    // the Portion at nPos is being split, unless there is already a switch at nPos
    if ( nPos == 0 )
        return 0;

    std::size_t nSplitPortion;
    sal_Int32 nTmpPos = 0;
    TETextPortion* pTextPortion = nullptr;
    TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );
    const std::size_t nPortions = pTEParaPortion->GetTextPortions().size();
    for ( nSplitPortion = 0; nSplitPortion < nPortions; nSplitPortion++ )
    {
        TETextPortion& rTP = pTEParaPortion->GetTextPortions()[nSplitPortion];
        nTmpPos += rTP.GetLen();
        if ( nTmpPos >= nPos )
        {
            if ( nTmpPos == nPos )  // nothing needs splitting
                return nSplitPortion;
            pTextPortion = &rTP;
            break;
        }
    }

    assert(pTextPortion && "SplitTextPortion: position outside of region!");

    const sal_Int32 nOverlapp = nTmpPos - nPos;
    pTextPortion->GetLen() -= nOverlapp;
    pTextPortion->GetWidth() = CalcTextWidth( nPara, nPos-pTextPortion->GetLen(), pTextPortion->GetLen() );
    TETextPortion aNewPortion( nOverlapp );
    pTEParaPortion->GetTextPortions().insert( pTEParaPortion->GetTextPortions().begin() + nSplitPortion + 1, aNewPortion );

    return nSplitPortion;
}

void TextEngine::CreateTextPortions( sal_uInt32 nPara, sal_Int32 nStartPos )
{
    TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );
    TextNode* pNode = pTEParaPortion->GetNode();
    SAL_WARN_IF( pNode->GetText().isEmpty(), "vcl", "CreateTextPortions: should not be used for empty paragraphs!" );

    o3tl::sorted_vector<sal_Int32> aPositions;
    o3tl::sorted_vector<sal_Int32>::const_iterator aPositionsIt;
    aPositions.insert(0);

    const sal_uInt16 nAttribs = pNode->GetCharAttribs().Count();
    for ( sal_uInt16 nAttr = 0; nAttr < nAttribs; nAttr++ )
    {
        TextCharAttrib& rAttrib = pNode->GetCharAttribs().GetAttrib( nAttr );

        aPositions.insert( rAttrib.GetStart() );
        aPositions.insert( rAttrib.GetEnd() );
    }
    aPositions.insert( pNode->GetText().getLength() );

    const std::vector<TEWritingDirectionInfo>& rWritingDirections = pTEParaPortion->GetWritingDirectionInfos();
    for ( const auto& rWritingDirection : rWritingDirections )
        aPositions.insert( rWritingDirection.nStartPos );

    if ( mpIMEInfos && mpIMEInfos->pAttribs && ( mpIMEInfos->aPos.GetPara() == nPara ) )
    {
        ExtTextInputAttr nLastAttr = ExtTextInputAttr(0xffff);
        for( sal_Int32 n = 0; n < mpIMEInfos->nLen; n++ )
        {
            if ( mpIMEInfos->pAttribs[n] != nLastAttr )
            {
                aPositions.insert( mpIMEInfos->aPos.GetIndex() + n );
                nLastAttr = mpIMEInfos->pAttribs[n];
            }
        }
    }

    sal_Int32 nTabPos = pNode->GetText().indexOf( '\t' );
    while ( nTabPos != -1 )
    {
        aPositions.insert( nTabPos );
        aPositions.insert( nTabPos + 1 );
        nTabPos = pNode->GetText().indexOf( '\t', nTabPos+1 );
    }

    // Delete starting with...
    // Unfortunately, the number of TextPortions does not have to be
    // equal to aPositions.Count(), because of linebreaks
    sal_Int32 nPortionStart = 0;
    std::size_t nInvPortion = 0;
    std::size_t nP;
    for ( nP = 0; nP < pTEParaPortion->GetTextPortions().size(); nP++ )
    {
        TETextPortion& rTmpPortion = pTEParaPortion->GetTextPortions()[nP];
        nPortionStart += rTmpPortion.GetLen();
        if ( nPortionStart >= nStartPos )
        {
            nPortionStart -= rTmpPortion.GetLen();
            nInvPortion = nP;
            break;
        }
    }
    OSL_ENSURE(nP < pTEParaPortion->GetTextPortions().size()
            || pTEParaPortion->GetTextPortions().empty(),
            "CreateTextPortions: Nothing to delete!");
    if ( nInvPortion && ( nPortionStart+pTEParaPortion->GetTextPortions()[nInvPortion].GetLen() > nStartPos ) )
    {
        // better one before...
        // But only if it was within the Portion; otherwise it might be
        // the only one in the previous line!
        nInvPortion--;
        nPortionStart -= pTEParaPortion->GetTextPortions()[nInvPortion].GetLen();
    }
    pTEParaPortion->GetTextPortions().DeleteFromPortion( nInvPortion );

    // a Portion might have been created by a line break
    aPositions.insert( nPortionStart );

    aPositionsIt = aPositions.find( nPortionStart );
    SAL_WARN_IF( aPositionsIt == aPositions.end(), "vcl", "CreateTextPortions: nPortionStart not found" );

    if ( aPositionsIt != aPositions.end() )
    {
        o3tl::sorted_vector<sal_Int32>::const_iterator nextIt = aPositionsIt;
        for ( ++nextIt; nextIt != aPositions.end(); ++aPositionsIt, ++nextIt )
        {
            TETextPortion aNew( *nextIt - *aPositionsIt );
            pTEParaPortion->GetTextPortions().push_back( aNew );
        }
    }
    OSL_ENSURE(pTEParaPortion->GetTextPortions().size(), "CreateTextPortions: No Portions?!");
}

void TextEngine::RecalcTextPortion( sal_uInt32 nPara, sal_Int32 nStartPos, sal_Int32 nNewChars )
{
    TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );
    OSL_ENSURE(pTEParaPortion->GetTextPortions().size(), "RecalcTextPortion: no Portions!");
    OSL_ENSURE(nNewChars, "RecalcTextPortion: Diff == 0");

    TextNode* const pNode = pTEParaPortion->GetNode();
    if ( nNewChars > 0 )
    {
        // If an Attribute is starting/ending at nStartPos, or there is a tab
        // before nStartPos => a new Portion starts.
        // Otherwise the Portion is extended at nStartPos.
        // Or if at the very beginning ( StartPos 0 ) followed by a tab...
        if ( ( pNode->GetCharAttribs().HasBoundingAttrib( nStartPos ) ) ||
             ( nStartPos && ( pNode->GetText()[ nStartPos - 1 ] == '\t' ) ) ||
             ( !nStartPos && ( nNewChars < pNode->GetText().getLength() ) && pNode->GetText()[ nNewChars ] == '\t' ) )
        {
            std::size_t nNewPortionPos = 0;
            if ( nStartPos )
                nNewPortionPos = SplitTextPortion( nPara, nStartPos ) + 1;

            // Here could be an empty Portion if the paragraph was empty,
            // or a new line was created by a hard line-break.
            if ( ( nNewPortionPos < pTEParaPortion->GetTextPortions().size() ) &&
                    !pTEParaPortion->GetTextPortions()[nNewPortionPos].GetLen() )
            {
                // use the empty Portion
                pTEParaPortion->GetTextPortions()[nNewPortionPos].GetLen() = nNewChars;
            }
            else
            {
                TETextPortion aNewPortion( nNewChars );
                pTEParaPortion->GetTextPortions().insert( pTEParaPortion->GetTextPortions().begin() + nNewPortionPos, aNewPortion );
            }
        }
        else
        {
            sal_Int32 nPortionStart {0};
            const std::size_t nTP = pTEParaPortion->GetTextPortions().FindPortion( nStartPos, nPortionStart );
            TETextPortion& rTP = pTEParaPortion->GetTextPortions()[ nTP ];
            rTP.GetLen() += nNewChars;
            rTP.GetWidth() = -1;
        }
    }
    else
    {
        // Shrink or remove Portion
        // Before calling this function, ensure that no Portions were in the deleted range!

        // There must be no Portion reaching into or starting within,
        // thus: nStartPos <= nPos <= nStartPos - nNewChars(neg.)
        std::size_t nPortion = 0;
        sal_Int32 nPos = 0;
        const sal_Int32 nEnd = nStartPos-nNewChars;
        const std::size_t nPortions = pTEParaPortion->GetTextPortions().size();
        TETextPortion* pTP = nullptr;
        for ( nPortion = 0; nPortion < nPortions; nPortion++ )
        {
            pTP = &pTEParaPortion->GetTextPortions()[ nPortion ];
            if ( ( nPos+pTP->GetLen() ) > nStartPos )
            {
                SAL_WARN_IF( nPos > nStartPos, "vcl", "RecalcTextPortion: Bad Start!" );
                SAL_WARN_IF( nPos+pTP->GetLen() < nEnd, "vcl", "RecalcTextPortion: Bad End!" );
                break;
            }
            nPos += pTP->GetLen();
        }
        SAL_WARN_IF( !pTP, "vcl", "RecalcTextPortion: Portion not found!" );
        if ( ( nPos == nStartPos ) && ( (nPos+pTP->GetLen()) == nEnd ) )
        {
            // remove Portion
            pTEParaPortion->GetTextPortions().erase( pTEParaPortion->GetTextPortions().begin() + nPortion );
        }
        else
        {
            SAL_WARN_IF( pTP->GetLen() <= (-nNewChars), "vcl", "RecalcTextPortion: Portion too small to shrink!" );
            pTP->GetLen() += nNewChars;
        }
        OSL_ENSURE( pTEParaPortion->GetTextPortions().size(),
                "RecalcTextPortion: none are left!" );
    }
}

void TextEngine::ImpPaint( OutputDevice* pOutDev, const Point& rStartPos, tools::Rectangle const* pPaintArea, TextSelection const* pSelection )
{
    if ( !GetUpdateMode() )
        return;

    if ( !IsFormatted() )
        FormatDoc();

    vcl::Window* const pOutWin = pOutDev->GetOwnerWindow();
    const bool bTransparent = (pOutWin && pOutWin->IsPaintTransparent());

    tools::Long nY = rStartPos.Y();

    TextPaM const* pSelStart = nullptr;
    TextPaM const* pSelEnd = nullptr;
    if ( pSelection && pSelection->HasRange() )
    {
        const bool bInvers = pSelection->GetEnd() < pSelection->GetStart();
        pSelStart = !bInvers ? &pSelection->GetStart() : &pSelection->GetEnd();
        pSelEnd = bInvers ? &pSelection->GetStart() : &pSelection->GetEnd();
    }

    const StyleSettings& rStyleSettings = pOutDev->GetSettings().GetStyleSettings();

    // for all paragraphs
    for ( sal_uInt32 nPara = 0; nPara < mpTEParaPortions->Count(); ++nPara )
    {
        TEParaPortion* pPortion = mpTEParaPortions->GetObject( nPara );
        // in case while typing Idle-Formatting, asynchronous Paint
        if ( pPortion->IsInvalid() )
            return;

        const tools::Long nParaHeight = CalcParaHeight( nPara );
        if ( !pPaintArea || ( ( nY + nParaHeight ) > pPaintArea->Top() ) )
        {
            // for all lines of the paragraph
            sal_Int32 nIndex = 0;
            for ( auto & rLine : pPortion->GetLines() )
            {
                Point aTmpPos( rStartPos.X() + rLine.GetStartX(), nY );

                if ( !pPaintArea || ( ( nY + mnCharHeight ) > pPaintArea->Top() ) )
                {
                    // for all Portions of the line
                    nIndex = rLine.GetStart();
                    for ( std::size_t y = rLine.GetStartPortion(); y <= rLine.GetEndPortion(); y++ )
                    {
                        OSL_ENSURE(pPortion->GetTextPortions().size(),
                                "ImpPaint: Line without Textportion!");
                        TETextPortion& rTextPortion = pPortion->GetTextPortions()[ y ];

                        ImpInitLayoutMode( pOutDev /*, pTextPortion->IsRightToLeft() */);

                        const tools::Long nTxtWidth = rTextPortion.GetWidth();
                        aTmpPos.setX( rStartPos.X() + ImpGetOutputOffset( nPara, &rLine, nIndex, nIndex ) );

                        // only print if starting in the visible region
                        if ( ( aTmpPos.X() + nTxtWidth ) >= 0 )
                        {
                            switch ( rTextPortion.GetKind() )
                            {
                                case PORTIONKIND_TEXT:
                                    {
                                        vcl::Font aFont;
                                        SeekCursor( nPara, nIndex+1, aFont, pOutDev );
                                        if( bTransparent )
                                            aFont.SetTransparent( true );
                                        else if ( pSelection )
                                            aFont.SetTransparent( false );
                                        pOutDev->SetFont( aFont );

                                        sal_Int32 nTmpIndex = nIndex;
                                        sal_Int32 nEnd = nTmpIndex + rTextPortion.GetLen();
                                        Point aPos = aTmpPos;

                                        bool bDone = false;
                                        if ( pSelStart )
                                        {
                                            // is a part of it in the selection?
                                            const TextPaM aTextStart( nPara, nTmpIndex );
                                            const TextPaM aTextEnd( nPara, nEnd );
                                            if ( ( aTextStart < *pSelEnd ) && ( aTextEnd > *pSelStart ) )
                                            {
                                                // 1) vcl::Region before Selection
                                                if ( aTextStart < *pSelStart )
                                                {
                                                    const sal_Int32 nL = pSelStart->GetIndex() - nTmpIndex;
                                                    pOutDev->SetFont( aFont);
                                                    pOutDev->SetTextFillColor();
                                                    aPos.setX( rStartPos.X() + ImpGetOutputOffset( nPara, &rLine, nTmpIndex, nTmpIndex+nL ) );
                                                    pOutDev->DrawText( aPos, pPortion->GetNode()->GetText(), nTmpIndex, nL );
                                                    nTmpIndex = nTmpIndex + nL;

                                                }
                                                // 2) vcl::Region with Selection
                                                sal_Int32 nL = nEnd - nTmpIndex;
                                                if ( aTextEnd > *pSelEnd )
                                                    nL = pSelEnd->GetIndex() - nTmpIndex;
                                                if ( nL )
                                                {
                                                    const Color aOldTextColor = pOutDev->GetTextColor();
                                                    pOutDev->SetTextColor( rStyleSettings.GetHighlightTextColor() );
                                                    pOutDev->SetTextFillColor( rStyleSettings.GetHighlightColor() );
                                                    aPos.setX( rStartPos.X() + ImpGetOutputOffset( nPara, &rLine, nTmpIndex, nTmpIndex+nL ) );
                                                    pOutDev->DrawText( aPos, pPortion->GetNode()->GetText(), nTmpIndex, nL );
                                                    pOutDev->SetTextColor( aOldTextColor );
                                                    pOutDev->SetTextFillColor();
                                                    nTmpIndex = nTmpIndex + nL;
                                                }

                                                // 3) vcl::Region after Selection
                                                if ( nTmpIndex < nEnd )
                                                {
                                                    nL = nEnd-nTmpIndex;
                                                    pOutDev->SetTextFillColor();
                                                    aPos.setX( rStartPos.X() + ImpGetOutputOffset( nPara, &rLine, nTmpIndex, nTmpIndex+nL ) );
                                                    pOutDev->DrawText( aPos, pPortion->GetNode()->GetText(), nTmpIndex, nEnd-nTmpIndex );
                                                }
                                                bDone = true;
                                            }
                                        }
                                        if ( !bDone )
                                        {
                                            pOutDev->SetTextFillColor();
                                            aPos.setX( rStartPos.X() + ImpGetOutputOffset( nPara, &rLine, nTmpIndex, nEnd ) );
                                            pOutDev->DrawText( aPos, pPortion->GetNode()->GetText(), nTmpIndex, nEnd-nTmpIndex );
                                        }
                                    }
                                    break;
                                case PORTIONKIND_TAB:
                                    // for HideSelection() only Range, pSelection = 0.
                                    if ( pSelStart ) // also implies pSelEnd
                                    {
                                        const tools::Rectangle aTabArea( aTmpPos, Point( aTmpPos.X()+nTxtWidth, aTmpPos.Y()+mnCharHeight-1 ) );
                                        // is the Tab in the Selection???
                                        const TextPaM aTextStart(nPara, nIndex);
                                        const TextPaM aTextEnd(nPara, nIndex + 1);
                                        if ((aTextStart < *pSelEnd) && (aTextEnd > *pSelStart))
                                        {
                                            pOutDev->Push(vcl::PushFlags::FILLCOLOR);
                                            pOutDev->SetFillColor(
                                                rStyleSettings.GetHighlightColor());
                                            pOutDev->DrawRect(aTabArea);
                                            pOutDev->Pop();
                                        }
                                        else
                                        {
                                            pOutDev->Erase( aTabArea );
                                        }
                                    }
                                    break;
                                default:
                                    OSL_FAIL( "ImpPaint: Unknown Portion-Type !" );
                            }
                        }

                        nIndex += rTextPortion.GetLen();
                    }
                }

                nY += mnCharHeight;

                if ( pPaintArea && ( nY >= pPaintArea->Bottom() ) )
                    break;  // no more visible actions
            }
        }
        else
        {
            nY += nParaHeight;
        }

        if ( pPaintArea && ( nY > pPaintArea->Bottom() ) )
            break;  // no more visible actions
    }
}

bool TextEngine::CreateLines( sal_uInt32 nPara )
{
    // bool: changing Height of Paragraph Yes/No - true/false

    TextNode* pNode = mpDoc->GetNodes()[ nPara ].get();
    TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );
    SAL_WARN_IF( !pTEParaPortion->IsInvalid(), "vcl", "CreateLines: Portion not invalid!" );

    const auto nOldLineCount = pTEParaPortion->GetLines().size();

    // fast special case for empty paragraphs
    if ( pTEParaPortion->GetNode()->GetText().isEmpty() )
    {
        if ( !pTEParaPortion->GetTextPortions().empty() )
            pTEParaPortion->GetTextPortions().Reset();
        pTEParaPortion->GetLines().clear();
        CreateAndInsertEmptyLine( nPara );
        pTEParaPortion->SetValid();
        return nOldLineCount != pTEParaPortion->GetLines().size();
    }

    // initialization
    if ( pTEParaPortion->GetLines().empty() )
    {
        pTEParaPortion->GetLines().emplace_back( );
    }

    const sal_Int32 nInvalidDiff = pTEParaPortion->GetInvalidDiff();
    const sal_Int32 nInvalidStart = pTEParaPortion->GetInvalidPosStart();
    const sal_Int32 nInvalidEnd = nInvalidStart + std::abs( nInvalidDiff );
    bool bQuickFormat = false;

    if ( pTEParaPortion->GetWritingDirectionInfos().empty() )
        ImpInitWritingDirections( nPara );

    if ( pTEParaPortion->GetWritingDirectionInfos().size() == 1 && pTEParaPortion->IsSimpleInvalid() )
    {
        bQuickFormat = nInvalidDiff != 0;
        if ( nInvalidDiff < 0 )
        {
            // check if deleting across Portion border
            sal_Int32 nPos = 0;
            for ( const auto & rTP : pTEParaPortion->GetTextPortions() )
            {
                // there must be no Start/End in the deleted region
                nPos += rTP.GetLen();
                if ( nPos > nInvalidStart && nPos < nInvalidEnd )
                {
                    bQuickFormat = false;
                    break;
                }
            }
        }
    }

    if ( bQuickFormat )
        RecalcTextPortion( nPara, nInvalidStart, nInvalidDiff );
    else
        CreateTextPortions( nPara, nInvalidStart );

    // search for line with InvalidPos; start a line prior
    // flag lines => do not remove!

    sal_uInt16 nLine = pTEParaPortion->GetLines().size()-1;
    for ( sal_uInt16 nL = 0; nL <= nLine; nL++ )
    {
        TextLine& rLine = pTEParaPortion->GetLines()[ nL ];
        if ( rLine.GetEnd() > nInvalidStart )
        {
            nLine = nL;
            break;
        }
        rLine.SetValid();
    }
    // start a line before...
    // if typing at the end, the line before cannot change
    if ( nLine && ( !pTEParaPortion->IsSimpleInvalid() || ( nInvalidEnd < pNode->GetText().getLength() ) || ( nInvalidDiff <= 0 ) ) )
        nLine--;

    TextLine* pLine =  &( pTEParaPortion->GetLines()[ nLine ] );

    // format all lines starting here
    std::size_t nDelFromLine = TETextPortionList::npos;

    sal_Int32 nIndex = pLine->GetStart();
    TextLine aSaveLine( *pLine );

    while ( nIndex < pNode->GetText().getLength() )
    {
        bool bEOL = false;
        sal_Int32 nPortionStart = 0;
        sal_Int32 nPortionEnd = 0;

        sal_Int32 nTmpPos = nIndex;
        std::size_t nTmpPortion = pLine->GetStartPortion();
        tools::Long nTmpWidth = mpDoc->GetLeftMargin();
        // do not subtract margin; it is included in TmpWidth
        tools::Long nXWidth = std::max(
            mnMaxTextWidth ? mnMaxTextWidth : std::numeric_limits<tools::Long>::max(), nTmpWidth);

        // search for Portion that does not fit anymore into line
        TETextPortion* pPortion = nullptr;
        bool bBrokenLine = false;

        while ( ( nTmpWidth <= nXWidth ) && !bEOL && ( nTmpPortion < pTEParaPortion->GetTextPortions().size() ) )
        {
            nPortionStart = nTmpPos;
            pPortion = &pTEParaPortion->GetTextPortions()[ nTmpPortion ];
            SAL_WARN_IF( !pPortion->GetLen(), "vcl", "CreateLines: Empty Portion!" );
            if ( pNode->GetText()[ nTmpPos ] == '\t' )
            {
                tools::Long nCurPos = nTmpWidth-mpDoc->GetLeftMargin();
                nTmpWidth = ((nCurPos/mnDefTab)+1)*mnDefTab+mpDoc->GetLeftMargin();
                pPortion->GetWidth() = nTmpWidth - nCurPos - mpDoc->GetLeftMargin();
                // infinite loop, if this is the first token of the line and nTmpWidth > aPaperSize.Width !!!
                if ( ( nTmpWidth >= nXWidth ) && ( nTmpPortion == pLine->GetStartPortion() ) )
                {
                    // adjust Tab
                    pPortion->GetWidth() = nXWidth-1;
                    nTmpWidth = pPortion->GetWidth();
                    bEOL = true;
                    bBrokenLine = true;
                }
                pPortion->GetKind() = PORTIONKIND_TAB;
            }
            else
            {

                pPortion->GetWidth() = CalcTextWidth( nPara, nTmpPos, pPortion->GetLen() );
                nTmpWidth += pPortion->GetWidth();

                pPortion->SetRightToLeft( ImpGetRightToLeft( nPara, nTmpPos+1 ) );
                pPortion->GetKind() = PORTIONKIND_TEXT;
            }

            nTmpPos += pPortion->GetLen();
            nPortionEnd = nTmpPos;
            nTmpPortion++;
        }

        // this was perhaps one Portion too far
        bool bFixedEnd = false;
        if ( nTmpWidth > nXWidth )
        {
            assert(pPortion);

            nPortionEnd = nTmpPos;
            nTmpPos -= pPortion->GetLen();
            nPortionStart = nTmpPos;
            nTmpPortion--;
            bEOL = false;

            nTmpWidth -= pPortion->GetWidth();
            if ( pPortion->GetKind() == PORTIONKIND_TAB )
            {
                bEOL = true;
                bFixedEnd = true;
            }
        }
        else
        {
            bEOL = true;
            pLine->SetEnd( nPortionEnd );
            OSL_ENSURE(pTEParaPortion->GetTextPortions().size(),
                    "CreateLines: No TextPortions?");
            pLine->SetEndPortion( pTEParaPortion->GetTextPortions().size() - 1 );
        }

        if ( bFixedEnd )
        {
            pLine->SetEnd( nPortionStart );
            pLine->SetEndPortion( nTmpPortion-1 );
        }
        else if ( bBrokenLine )
        {
            pLine->SetEnd( nPortionStart+1 );
            pLine->SetEndPortion( nTmpPortion-1 );
        }
        else if ( !bEOL )
        {
            SAL_WARN_IF( (nPortionEnd-nPortionStart) != pPortion->GetLen(), "vcl", "CreateLines: There is a Portion after all?!" );
            const tools::Long nRemainingWidth = mnMaxTextWidth - nTmpWidth;
            ImpBreakLine( nPara, pLine, nPortionStart, nRemainingWidth );
        }

        if ( ( ImpGetAlign() == TxtAlign::Center ) || ( ImpGetAlign() == TxtAlign::Right ) )
        {
            // adjust
            tools::Long nTextWidth = 0;
            for ( std::size_t nTP = pLine->GetStartPortion(); nTP <= pLine->GetEndPortion(); nTP++ )
            {
                TETextPortion& rTextPortion = pTEParaPortion->GetTextPortions()[ nTP ];
                nTextWidth += rTextPortion.GetWidth();
            }
            const tools::Long nSpace = mnMaxTextWidth - nTextWidth;
            if ( nSpace > 0 )
            {
                if ( ImpGetAlign() == TxtAlign::Center )
                    pLine->SetStartX( static_cast<sal_uInt16>(nSpace / 2) );
                else    // TxtAlign::Right
                    pLine->SetStartX( static_cast<sal_uInt16>(nSpace) );
            }
        }
        else
        {
            pLine->SetStartX( mpDoc->GetLeftMargin() );
        }

         // check if the line has to be printed again
        pLine->SetInvalid();

        if ( pTEParaPortion->IsSimpleInvalid() )
        {
            // Change due to simple TextChange...
            // Do not abort formatting, as Portions might have to be split!
            // Once it is ok to abort, then validate the following lines!
            // But mark as valid, thus reduce printing...
            if ( pLine->GetEnd() < nInvalidStart )
            {
                if ( *pLine == aSaveLine )
                {
                    pLine->SetValid();
                }
            }
            else
            {
                const sal_Int32 nStart = pLine->GetStart();
                const sal_Int32 nEnd = pLine->GetEnd();

                if ( nStart > nInvalidEnd )
                {
                    if ( ( ( nStart-nInvalidDiff ) == aSaveLine.GetStart() ) &&
                            ( ( nEnd-nInvalidDiff ) == aSaveLine.GetEnd() ) )
                    {
                        pLine->SetValid();
                        if ( bQuickFormat )
                        {
                            pTEParaPortion->CorrectValuesBehindLastFormattedLine( nLine );
                            break;
                        }
                    }
                }
                else if ( bQuickFormat && ( nEnd > nInvalidEnd) )
                {
                    // If the invalid line ends such that the next line starts
                    // at the 'same' position as before (no change in line breaks),
                    // the text width does not have to be recalculated.
                    if ( nEnd == ( aSaveLine.GetEnd() + nInvalidDiff ) )
                    {
                        pTEParaPortion->CorrectValuesBehindLastFormattedLine( nLine );
                        break;
                    }
                }
            }
        }

        nIndex = pLine->GetEnd();   // next line Start = previous line End
                                    // because nEnd is past the last char!

        const std::size_t nEndPortion = pLine->GetEndPortion();

        // next line or new line
        pLine = nullptr;
        if ( nLine < pTEParaPortion->GetLines().size()-1 )
            pLine = &( pTEParaPortion->GetLines()[ ++nLine ] );
        if ( pLine && ( nIndex >= pNode->GetText().getLength() ) )
        {
            nDelFromLine = nLine;
            break;
        }
        if ( !pLine )
        {
            if ( nIndex < pNode->GetText().getLength() )
            {
                ++nLine;
                pTEParaPortion->GetLines().insert( pTEParaPortion->GetLines().begin() + nLine, TextLine() );
                pLine = &pTEParaPortion->GetLines()[nLine];
            }
            else
            {
                break;
            }
        }
        aSaveLine = *pLine;
        pLine->SetStart( nIndex );
        pLine->SetEnd( nIndex );
        pLine->SetStartPortion( nEndPortion+1 );
        pLine->SetEndPortion( nEndPortion+1 );

    }   // while ( Index < Len )

    if (nDelFromLine != TETextPortionList::npos)
    {
        pTEParaPortion->GetLines().erase( pTEParaPortion->GetLines().begin() + nDelFromLine,
                                          pTEParaPortion->GetLines().end() );
    }

    SAL_WARN_IF( pTEParaPortion->GetLines().empty(), "vcl", "CreateLines: No Line!" );

    pTEParaPortion->SetValid();

    return nOldLineCount != pTEParaPortion->GetLines().size();
}

OUString TextEngine::GetWord( const TextPaM& rCursorPos, TextPaM* pStartOfWord, TextPaM* pEndOfWord )
{
    OUString aWord;
    if ( rCursorPos.GetPara() < mpDoc->GetNodes().size() )
    {
        TextSelection aSel( rCursorPos );
        TextNode* pNode = mpDoc->GetNodes()[ rCursorPos.GetPara() ].get();
        uno::Reference < i18n::XBreakIterator > xBI = GetBreakIterator();
        i18n::Boundary aBoundary = xBI->getWordBoundary( pNode->GetText(), rCursorPos.GetIndex(), GetLocale(), i18n::WordType::ANYWORD_IGNOREWHITESPACES, true );
        // tdf#57879 - expand selection to the left to include connector punctuation and search for additional word boundaries
        if (aBoundary.startPos > 0 && aBoundary.startPos < pNode->GetText().getLength() && u_charType(pNode->GetText()[aBoundary.startPos]) == U_CONNECTOR_PUNCTUATION)
        {
            aBoundary.startPos = xBI->getWordBoundary(pNode->GetText(), aBoundary.startPos - 1,
                GetLocale(), css::i18n::WordType::ANYWORD_IGNOREWHITESPACES, true).startPos;
        }
        while (aBoundary.startPos > 0 && u_charType(pNode->GetText()[aBoundary.startPos - 1]) == U_CONNECTOR_PUNCTUATION)
        {
            aBoundary.startPos = std::min(aBoundary.startPos,
                xBI->getWordBoundary( pNode->GetText(), aBoundary.startPos - 2,
                    GetLocale(), css::i18n::WordType::ANYWORD_IGNOREWHITESPACES, true).startPos);
        }
        // tdf#57879 - expand selection to the right to include connector punctuation and search for additional word boundaries
        if (aBoundary.endPos > 0 && aBoundary.endPos < pNode->GetText().getLength() && u_charType(pNode->GetText()[aBoundary.endPos - 1]) == U_CONNECTOR_PUNCTUATION)
        {
            aBoundary.endPos = xBI->getWordBoundary(pNode->GetText(), aBoundary.endPos,
                GetLocale(), css::i18n::WordType::ANYWORD_IGNOREWHITESPACES, true).endPos;
        }
        while (aBoundary.endPos < pNode->GetText().getLength() && u_charType(pNode->GetText()[aBoundary.endPos]) == U_CONNECTOR_PUNCTUATION)
        {
            aBoundary.endPos = xBI->getWordBoundary(pNode->GetText(), aBoundary.endPos + 1,
                GetLocale(), css::i18n::WordType::ANYWORD_IGNOREWHITESPACES, true).endPos;
        }
        aSel.GetStart().GetIndex() = aBoundary.startPos;
        aSel.GetEnd().GetIndex() = aBoundary.endPos;
        aWord = pNode->GetText().copy( aSel.GetStart().GetIndex(), aSel.GetEnd().GetIndex() - aSel.GetStart().GetIndex() );
        if ( pStartOfWord )
            *pStartOfWord = aSel.GetStart();
        if (pEndOfWord)
            *pEndOfWord = aSel.GetEnd();
    }
    return aWord;
}

bool TextEngine::Read( SvStream& rInput, const TextSelection* pSel )
{
    const bool bUpdate = GetUpdateMode();
    SetUpdateMode( false );

    UndoActionStart();
    TextSelection aSel;
    if ( pSel )
        aSel = *pSel;
    else
    {
        const sal_uInt32 nParas = static_cast<sal_uInt32>(mpDoc->GetNodes().size());
        TextNode* pNode = mpDoc->GetNodes()[ nParas - 1 ].get();
        aSel = TextPaM( nParas-1 , pNode->GetText().getLength() );
    }

    if ( aSel.HasRange() )
        aSel = ImpDeleteText( aSel );

    OStringBuffer aLine;
    bool bDone = rInput.ReadLine( aLine );
    OUString aTmpStr(OStringToOUString(aLine, rInput.GetStreamCharSet()));
    while ( bDone )
    {
        aSel = ImpInsertText( aSel, aTmpStr );
        bDone = rInput.ReadLine( aLine );
        aTmpStr = OStringToOUString(aLine, rInput.GetStreamCharSet());
        if ( bDone )
            aSel = ImpInsertParaBreak( aSel.GetEnd() );
    }

    UndoActionEnd();

    const TextSelection aNewSel( aSel.GetEnd(), aSel.GetEnd() );

    // so that FormatAndUpdate does not access the invalid selection
    if ( GetActiveView() )
        GetActiveView()->ImpSetSelection( aNewSel );

    SetUpdateMode( bUpdate );
    FormatAndUpdate( GetActiveView() );

    return rInput.GetError() == ERRCODE_NONE;
}

void TextEngine::Write( SvStream& rOutput )
{
    TextSelection aSel;
    const sal_uInt32 nParas = static_cast<sal_uInt32>(mpDoc->GetNodes().size());
    TextNode* pSelNode = mpDoc->GetNodes()[ nParas - 1 ].get();
    aSel.GetStart() = TextPaM( 0, 0 );
    aSel.GetEnd() = TextPaM( nParas-1, pSelNode->GetText().getLength() );

    for ( sal_uInt32 nPara = aSel.GetStart().GetPara(); nPara <= aSel.GetEnd().GetPara(); ++nPara  )
    {
        TextNode* pNode = mpDoc->GetNodes()[ nPara ].get();

        const sal_Int32 nStartPos = nPara == aSel.GetStart().GetPara()
            ? aSel.GetStart().GetIndex() : 0;
        const sal_Int32 nEndPos = nPara == aSel.GetEnd().GetPara()
            ? aSel.GetEnd().GetIndex() : pNode->GetText().getLength();

        const OUString aText = pNode->GetText().copy( nStartPos, nEndPos-nStartPos );
        rOutput.WriteLine(OUStringToOString(aText, rOutput.GetStreamCharSet()));
    }
}

void TextEngine::RemoveAttribs( sal_uInt32 nPara )
{
    if ( nPara >= mpDoc->GetNodes().size() )
        return;

    TextNode* pNode = mpDoc->GetNodes()[ nPara ].get();
    if ( pNode->GetCharAttribs().Count() )
    {
        pNode->GetCharAttribs().Clear();

        TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );
        pTEParaPortion->MarkSelectionInvalid( 0 );

        mbFormatted = false;

        IdleFormatAndUpdate( nullptr, 0xFFFF );
    }
}

void TextEngine::SetAttrib( const TextAttrib& rAttr, sal_uInt32 nPara, sal_Int32 nStart, sal_Int32 nEnd )
{

    // For now do not check if Attributes overlap!
    // This function is for TextEditors that want to _quickly_ generate the Syntax-Highlight

    // As TextEngine is currently intended only for TextEditors, there is no Undo for Attributes!

    if ( nPara >= mpDoc->GetNodes().size() )
        return;

    TextNode* pNode = mpDoc->GetNodes()[ nPara ].get();
    TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );

    const sal_Int32 nMax = pNode->GetText().getLength();
    if ( nStart > nMax )
        nStart = nMax;
    if ( nEnd > nMax )
        nEnd = nMax;

    pNode->GetCharAttribs().InsertAttrib( std::make_unique<TextCharAttrib>( rAttr, nStart, nEnd ) );
    pTEParaPortion->MarkSelectionInvalid( nStart );

    mbFormatted = false;
    IdleFormatAndUpdate( nullptr, 0xFFFF );
}

void TextEngine::SetTextAlign( TxtAlign eAlign )
{
    if ( eAlign != meAlign )
    {
        meAlign = eAlign;
        FormatFullDoc();
        UpdateViews();
    }
}

void TextEngine::ValidateSelection( TextSelection& rSel ) const
{
    ValidatePaM( rSel.GetStart() );
    ValidatePaM( rSel.GetEnd() );
}

void TextEngine::ValidatePaM( TextPaM& rPaM ) const
{
    const sal_uInt32 nParas = static_cast<sal_uInt32>(mpDoc->GetNodes().size());
    if ( rPaM.GetPara() >= nParas )
    {
        rPaM.GetPara() = nParas ? nParas-1 : 0;
        rPaM.GetIndex() = TEXT_INDEX_ALL;
    }

    const sal_Int32 nMaxIndex = GetTextLen( rPaM.GetPara() );
    if ( rPaM.GetIndex() > nMaxIndex )
        rPaM.GetIndex() = nMaxIndex;
}

// adjust State & Selection

void TextEngine::ImpParagraphInserted( sal_uInt32 nPara )
{
    // No adjustment needed for the active View;
    // but for all passive Views the Selection needs adjusting.
    if ( mpViews->size() > 1 )
    {
        for ( auto nView = mpViews->size(); nView; )
        {
            TextView* pView = (*mpViews)[ --nView ];
            if ( pView != GetActiveView() )
            {
                for ( int n = 0; n <= 1; n++ )
                {
                    TextPaM& rPaM = n ? pView->GetSelection().GetStart(): pView->GetSelection().GetEnd();
                    if ( rPaM.GetPara() >= nPara )
                        rPaM.GetPara()++;
                }
            }
        }
    }
    Broadcast( TextHint( SfxHintId::TextParaInserted, nPara ) );
}

void TextEngine::ImpParagraphRemoved( sal_uInt32 nPara )
{
    if ( mpViews->size() > 1 )
    {
        for ( auto nView = mpViews->size(); nView; )
        {
            TextView* pView = (*mpViews)[ --nView ];
            if ( pView != GetActiveView() )
            {
                const sal_uInt32 nParas = static_cast<sal_uInt32>(mpDoc->GetNodes().size());
                for ( int n = 0; n <= 1; n++ )
                {
                    TextPaM& rPaM = n ? pView->GetSelection().GetStart(): pView->GetSelection().GetEnd();
                    if ( rPaM.GetPara() > nPara )
                        rPaM.GetPara()--;
                    else if ( rPaM.GetPara() == nPara )
                    {
                        rPaM.GetIndex() = 0;
                        if ( rPaM.GetPara() >= nParas )
                            rPaM.GetPara()--;
                    }
                }
            }
        }
    }
    Broadcast( TextHint( SfxHintId::TextParaRemoved, nPara ) );
}

void TextEngine::ImpCharsRemoved( sal_uInt32 nPara, sal_Int32 nPos, sal_Int32 nChars )
{
    if ( mpViews->size() > 1 )
    {
        for ( auto nView = mpViews->size(); nView; )
        {
            TextView* pView = (*mpViews)[ --nView ];
            if ( pView != GetActiveView() )
            {
                const sal_Int32 nEnd = nPos + nChars;
                for ( int n = 0; n <= 1; n++ )
                {
                    TextPaM& rPaM = n ? pView->GetSelection().GetStart(): pView->GetSelection().GetEnd();
                    if ( rPaM.GetPara() == nPara )
                    {
                        if ( rPaM.GetIndex() > nEnd )
                            rPaM.GetIndex() = rPaM.GetIndex() - nChars;
                        else if ( rPaM.GetIndex() > nPos )
                            rPaM.GetIndex() = nPos;
                    }
                }
            }
        }
    }
    Broadcast( TextHint( SfxHintId::TextParaContentChanged, nPara ) );
}

void TextEngine::ImpCharsInserted( sal_uInt32 nPara, sal_Int32 nPos, sal_Int32 nChars )
{
    if ( mpViews->size() > 1 )
    {
        for ( auto nView = mpViews->size(); nView; )
        {
            TextView* pView = (*mpViews)[ --nView ];
            if ( pView != GetActiveView() )
            {
                for ( int n = 0; n <= 1; n++ )
                {
                    TextPaM& rPaM = n ? pView->GetSelection().GetStart(): pView->GetSelection().GetEnd();
                    if ( rPaM.GetPara() == nPara )
                    {
                        if ( rPaM.GetIndex() >= nPos )
                            rPaM.GetIndex() += nChars;
                    }
                }
            }
        }
    }
    Broadcast( TextHint( SfxHintId::TextParaContentChanged, nPara ) );
}

void TextEngine::Draw( OutputDevice* pDev, const Point& rPos )
{
    ImpPaint( pDev, rPos, nullptr );
}

void TextEngine::SetLeftMargin( sal_uInt16 n )
{
    mpDoc->SetLeftMargin( n );
}

uno::Reference< i18n::XBreakIterator > const & TextEngine::GetBreakIterator()
{
    if ( !mxBreakIterator.is() )
        mxBreakIterator = vcl::unohelper::CreateBreakIterator();
    SAL_WARN_IF( !mxBreakIterator.is(), "vcl", "BreakIterator: Failed to create!" );
    return mxBreakIterator;
}

void TextEngine::SetLocale( const css::lang::Locale& rLocale )
{
    maLocale = rLocale;
    mpLocaleDataWrapper.reset();
}

css::lang::Locale const & TextEngine::GetLocale()
{
    if ( maLocale.Language.isEmpty() )
    {
        maLocale = Application::GetSettings().GetUILanguageTag().getLocale();   // TODO: why UI locale?
    }
    return maLocale;
}

LocaleDataWrapper* TextEngine::ImpGetLocaleDataWrapper()
{
    if ( !mpLocaleDataWrapper )
        mpLocaleDataWrapper.reset( new LocaleDataWrapper( LanguageTag( GetLocale()) ) );

    return mpLocaleDataWrapper.get();
}

void TextEngine::SetRightToLeft( bool bR2L )
{
    if ( mbRightToLeft != bR2L )
    {
        mbRightToLeft = bR2L;
        meAlign = bR2L ? TxtAlign::Right : TxtAlign::Left;
        FormatFullDoc();
        UpdateViews();
    }
}

void TextEngine::ImpInitWritingDirections( sal_uInt32 nPara )
{
    TEParaPortion* pParaPortion = mpTEParaPortions->GetObject( nPara );
    std::vector<TEWritingDirectionInfo>& rInfos = pParaPortion->GetWritingDirectionInfos();
    rInfos.clear();

    if ( !pParaPortion->GetNode()->GetText().isEmpty() )
    {
        const UBiDiLevel nBidiLevel = IsRightToLeft() ? 1 /*RTL*/ : 0 /*LTR*/;
        OUString aText( pParaPortion->GetNode()->GetText() );

        // Bidi functions from icu 2.0

        UErrorCode nError = U_ZERO_ERROR;
        UBiDi* pBidi = ubidi_openSized( aText.getLength(), 0, &nError );
        nError = U_ZERO_ERROR;

        ubidi_setPara( pBidi, reinterpret_cast<const UChar *>(aText.getStr()), aText.getLength(), nBidiLevel, nullptr, &nError );
        nError = U_ZERO_ERROR;

        tools::Long nCount = ubidi_countRuns( pBidi, &nError );

        int32_t nStart = 0;
        int32_t nEnd;
        UBiDiLevel nCurrDir;

        for ( tools::Long nIdx = 0; nIdx < nCount; ++nIdx )
        {
            ubidi_getLogicalRun( pBidi, nStart, &nEnd, &nCurrDir );
            // bit 0 of nCurrDir indicates direction
            rInfos.emplace_back( /*bLeftToRight*/ nCurrDir % 2 == 0, nStart, nEnd );
            nStart = nEnd;
        }

        ubidi_close( pBidi );
    }

    // No infos mean no CTL and default dir is L2R...
    if ( rInfos.empty() )
        rInfos.emplace_back( 0, 0, pParaPortion->GetNode()->GetText().getLength() );

}

bool TextEngine::ImpGetRightToLeft( sal_uInt32 nPara, sal_Int32 nPos )
{
    bool bRightToLeft = false;

    TextNode* pNode = mpDoc->GetNodes()[ nPara ].get();
    if ( pNode && !pNode->GetText().isEmpty() )
    {
        TEParaPortion* pParaPortion = mpTEParaPortions->GetObject( nPara );
        if ( pParaPortion->GetWritingDirectionInfos().empty() )
            ImpInitWritingDirections( nPara );

        std::vector<TEWritingDirectionInfo>& rDirInfos = pParaPortion->GetWritingDirectionInfos();
        for ( const auto& rWritingDirectionInfo : rDirInfos )
        {
            if ( rWritingDirectionInfo.nStartPos <= nPos && rWritingDirectionInfo.nEndPos >= nPos )
            {
                bRightToLeft = !rWritingDirectionInfo.bLeftToRight;
                break;
            }
        }
    }
    return bRightToLeft;
}

tools::Long TextEngine::ImpGetPortionXOffset( sal_uInt32 nPara, TextLine const * pLine, std::size_t nTextPortion )
{
    tools::Long nX = pLine->GetStartX();

    TEParaPortion* pParaPortion = mpTEParaPortions->GetObject( nPara );

    for ( std::size_t i = pLine->GetStartPortion(); i < nTextPortion; i++ )
    {
        TETextPortion& rPortion = pParaPortion->GetTextPortions()[ i ];
        nX += rPortion.GetWidth();
    }

    TETextPortion& rDestPortion = pParaPortion->GetTextPortions()[ nTextPortion ];
    if ( rDestPortion.GetKind() != PORTIONKIND_TAB )
    {
        if ( !IsRightToLeft() && rDestPortion.IsRightToLeft() )
        {
            // Portions behind must be added, visual before this portion
            std::size_t nTmpPortion = nTextPortion+1;
            while ( nTmpPortion <= pLine->GetEndPortion() )
            {
                TETextPortion& rNextTextPortion = pParaPortion->GetTextPortions()[ nTmpPortion ];
                if ( rNextTextPortion.IsRightToLeft() && ( rNextTextPortion.GetKind() != PORTIONKIND_TAB ) )
                    nX += rNextTextPortion.GetWidth();
                else
                    break;
                nTmpPortion++;
            }
            // Portions before must be removed, visual behind this portion
            nTmpPortion = nTextPortion;
            while ( nTmpPortion > pLine->GetStartPortion() )
            {
                --nTmpPortion;
                TETextPortion& rPrevTextPortion = pParaPortion->GetTextPortions()[ nTmpPortion ];
                if ( rPrevTextPortion.IsRightToLeft() && ( rPrevTextPortion.GetKind() != PORTIONKIND_TAB ) )
                    nX -= rPrevTextPortion.GetWidth();
                else
                    break;
            }
        }
        else if ( IsRightToLeft() && !rDestPortion.IsRightToLeft() )
        {
            // Portions behind must be removed, visual behind this portion
            std::size_t nTmpPortion = nTextPortion+1;
            while ( nTmpPortion <= pLine->GetEndPortion() )
            {
                TETextPortion& rNextTextPortion = pParaPortion->GetTextPortions()[ nTmpPortion ];
                if ( !rNextTextPortion.IsRightToLeft() && ( rNextTextPortion.GetKind() != PORTIONKIND_TAB ) )
                    nX += rNextTextPortion.GetWidth();
                else
                    break;
                nTmpPortion++;
            }
            // Portions before must be added, visual before this portion
            nTmpPortion = nTextPortion;
            while ( nTmpPortion > pLine->GetStartPortion() )
            {
                --nTmpPortion;
                TETextPortion& rPrevTextPortion = pParaPortion->GetTextPortions()[ nTmpPortion ];
                if ( !rPrevTextPortion.IsRightToLeft() && ( rPrevTextPortion.GetKind() != PORTIONKIND_TAB ) )
                    nX -= rPrevTextPortion.GetWidth();
                else
                    break;
            }
        }
    }

    return nX;
}

void TextEngine::ImpInitLayoutMode( OutputDevice* pOutDev )
{
    vcl::text::ComplexTextLayoutFlags nLayoutMode = pOutDev->GetLayoutMode();

    nLayoutMode &= ~vcl::text::ComplexTextLayoutFlags(vcl::text::ComplexTextLayoutFlags::BiDiRtl | vcl::text::ComplexTextLayoutFlags::BiDiStrong );

    pOutDev->SetLayoutMode( nLayoutMode );
}

TxtAlign TextEngine::ImpGetAlign() const
{
    TxtAlign eAlign = meAlign;
    if ( IsRightToLeft() )
    {
        if ( eAlign == TxtAlign::Left )
            eAlign = TxtAlign::Right;
        else if ( eAlign == TxtAlign::Right )
            eAlign = TxtAlign::Left;
    }
    return eAlign;
}

tools::Long TextEngine::ImpGetOutputOffset( sal_uInt32 nPara, TextLine* pLine, sal_Int32 nIndex, sal_Int32 nIndex2 )
{
    TEParaPortion* pPortion = mpTEParaPortions->GetObject( nPara );

    sal_Int32 nPortionStart {0};
    const std::size_t nPortion = pPortion->GetTextPortions().FindPortion( nIndex, nPortionStart, true );

    TETextPortion& rTextPortion = pPortion->GetTextPortions()[ nPortion ];

    tools::Long nX;

    if ( ( nIndex == nPortionStart ) && ( nIndex == nIndex2 )  )
    {
        // Output of full portion, so we need portion x offset.
        // Use ImpGetPortionXOffset, because GetXPos may deliver left or right position from portion, depending on R2L, L2R
        nX = ImpGetPortionXOffset( nPara, pLine, nPortion );
        if ( IsRightToLeft() )
        {
            nX = -nX - rTextPortion.GetWidth();
        }
    }
    else
    {
        nX = ImpGetXPos( nPara, pLine, nIndex, nIndex == nPortionStart );
        if ( nIndex2 != nIndex )
        {
            const tools::Long nX2 = ImpGetXPos( nPara, pLine, nIndex2 );
            if ( ( !IsRightToLeft() && ( nX2 < nX ) ) ||
                 ( IsRightToLeft() && ( nX2 > nX ) ) )
            {
                nX = nX2;
            }
        }
        if ( IsRightToLeft() )
        {
            nX = -nX;
        }
    }

    return nX;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
