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

#include <hintids.hxx>
#include <cmdid.h>

#include <com/sun/star/beans/XMultiPropertySet.hpp>
#include <com/sun/star/beans/XPropertiesChangeListener.hpp>
#include <com/sun/star/container/XHierarchicalNameAccess.hpp>
#include <cppuhelper/implbase.hxx>
#include <officecfg/Office/Common.hxx>
#include <rtl/ustring.hxx>
#include <sal/log.hxx>
#include <vcl/commandevent.hxx>
#include <vcl/event.hxx>
#include <vcl/svapp.hxx>
#include <vcl/textview.hxx>
#include <vcl/ptrstyle.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/viewfrm.hxx>
#include <svtools/htmltokn.h>
#include <vcl/txtattr.hxx>
#include <vcl/settings.hxx>
#include <svtools/colorcfg.hxx>
#include <editeng/flstitem.hxx>
#include <vcl/metric.hxx>
#include <svtools/ctrltool.hxx>
#include <svtools/scrolladaptor.hxx>
#include <tools/time.hxx>
#include <swmodule.hxx>
#include <docsh.hxx>
#include <srcview.hxx>
#include <helpids.h>
#include <vector>

namespace
{

struct TextPortion
{
    sal_uInt16 nStart, nEnd;
    svtools::ColorConfigEntry eType;
};

}

#define MAX_SYNTAX_HIGHLIGHT 20
#define MAX_HIGHLIGHTTIME 200

typedef std::vector<TextPortion> TextPortions;

static void lcl_Highlight(std::u16string_view aSource, TextPortions& aPortionList)
{
    const sal_Unicode cOpenBracket = '<';
    const sal_Unicode cCloseBracket= '>';
    const sal_Unicode cSlash        = '/';
    const sal_Unicode cExclamation = '!';
    const sal_Unicode cMinus        = '-';
    const sal_Unicode cSpace        = ' ';
    const sal_Unicode cTab          = 0x09;
    const sal_Unicode cLF          = 0x0a;
    const sal_Unicode cCR          = 0x0d;

    const sal_Int32 nStrLen = aSource.size();
    sal_Int32 nInsert = 0;         // number of inserted portions
    sal_Int32 nActPos = 0;         // position, where '<' was found
    sal_Int32 nPortStart = SAL_MAX_INT32;  // for the TextPortion
    sal_Int32 nPortEnd  =  0;
    TextPortion aText;
    while(nActPos < nStrLen)
    {
        if((nActPos < nStrLen - 2) && (aSource[nActPos] == cOpenBracket))
        {
            svtools::ColorConfigEntry eFoundType = svtools::HTMLUNKNOWN;
            // insert 'empty' portion
            if(nPortEnd < nActPos - 1 )
            {
                // don't move at the beginning
                aText.nStart = nPortEnd;
                if(nInsert)
                    aText.nStart += 1;
                aText.nEnd = nActPos - 1;
                aText.eType = svtools::HTMLUNKNOWN;
                aPortionList.push_back( aText );
                nInsert++;
            }
            sal_Unicode cFollowFirst = aSource[nActPos + 1];
            sal_Unicode cFollowNext = aSource[nActPos + 2];
            if(cExclamation == cFollowFirst)
            {
                // "<!" SGML or comment
                if(cMinus == cFollowNext &&
                    nActPos < nStrLen - 3 && cMinus == aSource[nActPos + 3])
                {
                    eFoundType = svtools::HTMLCOMMENT;
                }
                else
                    eFoundType = svtools::HTMLSGML;
                nPortStart = nActPos;
                nPortEnd = nActPos + 1;
            }
            else if(cSlash == cFollowFirst)
            {
                // "</" ignore slash
                nPortStart = nActPos;
                nActPos++;
            }
            if(svtools::HTMLUNKNOWN == eFoundType)
            {
                // now here a keyword could follow
                sal_uInt16 nSrchPos = nActPos;
                while(++nSrchPos < nStrLen - 1)
                {
                    sal_Unicode cNext = aSource[nSrchPos];
                    if( cNext == cSpace ||
                        cNext == cTab   ||
                        cNext == cLF    ||
                        cNext == cCR)
                        break;
                    else if(cNext == cCloseBracket)
                    {
                        break;
                    }
                }
                if(nSrchPos > nActPos + 1)
                {
                    // some string was found
                    OUString sToken( aSource.substr(nActPos + 1, nSrchPos - nActPos - 1 ) );
                    sToken = sToken.toAsciiUpperCase();
                    HtmlTokenId nToken = ::GetHTMLToken(sToken);
                    if(nToken != HtmlTokenId::NONE)
                    {
                        eFoundType = svtools::HTMLKEYWORD;
                        nPortEnd = nSrchPos;
                        nPortStart = nActPos;
                    }
                    else
                        SAL_WARN("sw", "HTML token " << sToken << " not recognised!");
                }
            }
            // now we still have to look for '>'
            if(svtools::HTMLUNKNOWN != eFoundType)
            {
                bool bFound = false;
                for(sal_Int32 i = nPortEnd; i < nStrLen; i++)
                    if(cCloseBracket == aSource[i])
                    {
                        bFound = true;
                        nPortEnd = i;
                        break;
                    }
                if(!bFound && (eFoundType == svtools::HTMLCOMMENT))
                {
                    // comment without ending in this line
                    bFound  = true;
                    nPortEnd = nStrLen - 1;
                }

                if(bFound ||(eFoundType == svtools::HTMLCOMMENT))
                {
                    TextPortion aTextPortion;
                    aTextPortion.nStart = nPortStart + 1;
                    aTextPortion.nEnd = nPortEnd;
                    aTextPortion.eType = eFoundType;
                    aPortionList.push_back( aTextPortion );
                    nInsert++;
                }

            }
        }
        nActPos++;
    }
    if(nInsert && nPortEnd < nActPos - 1)
    {
        aText.nStart = nPortEnd + 1;
        aText.nEnd = nActPos - 1;
        aText.eType = svtools::HTMLUNKNOWN;
        aPortionList.push_back( aText );
        nInsert++;
    }
}

class SwSrcEditWindow::ChangesListener:
    public cppu::WeakImplHelper< css::beans::XPropertiesChangeListener >
{
public:
    explicit ChangesListener(SwSrcEditWindow & editor): m_Editor(editor) {}

private:
    virtual ~ChangesListener() override {}

    virtual void SAL_CALL disposing(css::lang::EventObject const &) override
    {
        std::unique_lock g(m_Editor.mutex_);
        m_Editor.m_xNotifier.clear();
    }

    virtual void SAL_CALL propertiesChange(
        css::uno::Sequence< css::beans::PropertyChangeEvent > const &) override
    {
        SolarMutexGuard g;
        m_Editor.SetFont();
    }

    SwSrcEditWindow & m_Editor;
};

SwSrcEditWindow::SwSrcEditWindow( vcl::Window* pParent, SwSrcView* pParentView ) :
    Window( pParent, WB_BORDER|WB_CLIPCHILDREN ),

    m_pOutWin(nullptr),
    m_pHScrollbar(nullptr),
    m_pVScrollbar(nullptr),

    m_pSrcView(pParentView),

    m_nCurTextWidth(0),
    m_nStartLine(USHRT_MAX),
    m_eSourceEncoding(osl_getThreadTextEncoding()),
    m_bReadonly(false),
    m_bHighlighting(false),
    m_aSyntaxIdle("sw uibase SwSrcEditWindow Syntax")
{
    SetHelpId(HID_SOURCE_EDITWIN);
    CreateTextEngine();

    // Using "this" in ctor is a little fishy, but should work here at least as
    // long as there are no derivations:
    m_xListener = new ChangesListener(*this);
    css::uno::Reference< css::beans::XMultiPropertySet > n(
        officecfg::Office::Common::Font::SourceViewFont::get(),
        css::uno::UNO_QUERY_THROW);
    {
        std::unique_lock g(mutex_);
        m_xNotifier = n;
    }
    n->addPropertiesChangeListener({ u"FontHeight"_ustr, u"FontName"_ustr }, m_xListener);
}

SwSrcEditWindow::~SwSrcEditWindow()
{
    disposeOnce();
}

void SwSrcEditWindow::dispose()
{
    css::uno::Reference< css::beans::XMultiPropertySet > n;
    {
        std::unique_lock g(mutex_);
        n = m_xNotifier;
    }
    if (n.is()) {
        n->removePropertiesChangeListener(m_xListener);
    }
    m_aSyntaxIdle.Stop();
    if ( m_pOutWin )
        m_pOutWin->SetTextView( nullptr );

    if ( m_pTextEngine )
    {
        EndListening( *m_pTextEngine );
        m_pTextEngine->RemoveView( m_pTextView.get() );

        m_pTextView.reset();
        m_pTextEngine.reset();
    }
    m_pHScrollbar.disposeAndClear();
    m_pVScrollbar.disposeAndClear();
    m_pOutWin.disposeAndClear();
    vcl::Window::dispose();
}

void SwSrcEditWindow::DataChanged( const DataChangedEvent& rDCEvt )
{
    Window::DataChanged( rDCEvt );

    switch ( rDCEvt.GetType() )
    {
    case DataChangedEventType::SETTINGS:
        // newly rearrange ScrollBars or trigger Resize, because
        // ScrollBar size could have changed. For this, in the
        // Resize handler the size of ScrollBars has to be queried
        // from the settings as well.
        if( rDCEvt.GetFlags() & AllSettingsFlags::STYLE )
            Resize();
        break;
    default: break;
    }
}

void  SwSrcEditWindow::Resize()
{
    // ScrollBars, etc. happens in Adjust...
    if ( !m_pTextView )
        return;

    tools::Long nVisY = m_pTextView->GetStartDocPos().Y();
    m_pTextView->ShowCursor();
    Size aOutSz( GetOutputSizePixel() );
    tools::Long nMaxVisAreaStart = m_pTextView->GetTextEngine()->GetTextHeight() - aOutSz.Height();
    if ( nMaxVisAreaStart < 0 )
        nMaxVisAreaStart = 0;
    if ( m_pTextView->GetStartDocPos().Y() > nMaxVisAreaStart )
    {
        Point aStartDocPos( m_pTextView->GetStartDocPos() );
        aStartDocPos.setY( nMaxVisAreaStart );
        m_pTextView->SetStartDocPos( aStartDocPos );
        m_pTextView->ShowCursor();
    }
    tools::Long nScrollStd = GetSettings().GetStyleSettings().GetScrollBarSize();
    Size aScrollSz(aOutSz.Width() - nScrollStd, nScrollStd );
    Point aScrollPos(0, aOutSz.Height() - nScrollStd);

    m_pHScrollbar->SetPosSizePixel( aScrollPos, aScrollSz);

    aScrollSz.setWidth( aScrollSz.Height() );
    aScrollSz.setHeight( aOutSz.Height() );
    aScrollPos = Point(aOutSz.Width() - nScrollStd, 0);

    m_pVScrollbar->SetPosSizePixel( aScrollPos, aScrollSz);
    aOutSz.AdjustWidth( -nScrollStd );
    aOutSz.AdjustHeight( -nScrollStd );
    m_pOutWin->SetOutputSizePixel(aOutSz);
    InitScrollBars();

    // set line in first Resize
    if(USHRT_MAX != m_nStartLine)
    {
        if(m_nStartLine < m_pTextEngine->GetParagraphCount())
        {
            TextSelection aSel(TextPaM( m_nStartLine, 0 ), TextPaM( m_nStartLine, 0x0 ));
            m_pTextView->SetSelection(aSel);
            m_pTextView->ShowCursor();
        }
        m_nStartLine = USHRT_MAX;
    }

    if ( nVisY != m_pTextView->GetStartDocPos().Y() )
        Invalidate();


}

void TextViewOutWin::DataChanged( const DataChangedEvent& rDCEvt )
{
    Window::DataChanged( rDCEvt );

    switch( rDCEvt.GetType() )
    {
    case DataChangedEventType::SETTINGS:
        // query settings
        if( rDCEvt.GetFlags() & AllSettingsFlags::STYLE )
        {
            const Color &rCol = GetSettings().GetStyleSettings().GetWindowColor();
            SetBackground( rCol );
            vcl::Font aFont( m_pTextView->GetTextEngine()->GetFont() );
            aFont.SetFillColor( rCol );
            m_pTextView->GetTextEngine()->SetFont( aFont );
        }
        break;
    default: break;
    }
}

void  TextViewOutWin::MouseMove( const MouseEvent &rEvt )
{
    if ( m_pTextView )
        m_pTextView->MouseMove( rEvt );
}

void  TextViewOutWin::MouseButtonUp( const MouseEvent &rEvt )
{
    if ( m_pTextView )
    {
        m_pTextView->MouseButtonUp( rEvt );
        SfxViewFrame& rFrame = static_cast<SwSrcEditWindow*>(GetParent())->GetSrcView()->GetViewFrame();
        SfxBindings& rBindings = rFrame.GetBindings();
        rBindings.Invalidate( SID_TABLE_CELL );
        rBindings.Invalidate( SID_CUT );
        rBindings.Invalidate( SID_COPY );
    }
}

void  TextViewOutWin::MouseButtonDown( const MouseEvent &rEvt )
{
    GrabFocus();
    if ( m_pTextView )
        m_pTextView->MouseButtonDown( rEvt );
}

void  TextViewOutWin::Command( const CommandEvent& rCEvt )
{
    switch(rCEvt.GetCommand())
    {
        case CommandEventId::ContextMenu:
            SfxDispatcher::ExecutePopup();
        break;
        case CommandEventId::Wheel:
        case CommandEventId::StartAutoScroll:
        case CommandEventId::AutoScroll:
        {
            const CommandWheelData* pWData = rCEvt.GetWheelData();
            if( !pWData || CommandWheelMode::ZOOM != pWData->GetMode() )
            {
                static_cast<SwSrcEditWindow*>(GetParent())->HandleWheelCommand( rCEvt );
            }
        }
        break;

        default:
            if ( m_pTextView )
                m_pTextView->Command( rCEvt );
            else
                Window::Command(rCEvt);
    }
}

void  TextViewOutWin::KeyInput( const KeyEvent& rKEvt )
{
    bool bDone = false;
    SwSrcEditWindow* pSrcEditWin = static_cast<SwSrcEditWindow*>(GetParent());
    bool bChange = !pSrcEditWin->IsReadonly() || !TextEngine::DoesKeyChangeText( rKEvt );
    if(bChange)
        bDone = m_pTextView->KeyInput( rKEvt );

    SfxBindings& rBindings = static_cast<SwSrcEditWindow*>(GetParent())->GetSrcView()->GetViewFrame().GetBindings();
    if ( !bDone )
    {
        if ( !SfxViewShell::Current()->KeyInput( rKEvt ) )
            Window::KeyInput( rKEvt );
    }
    else
    {
        rBindings.Invalidate( SID_TABLE_CELL );
        if ( rKEvt.GetKeyCode().GetGroup() == KEYGROUP_CURSOR )
            rBindings.Update( SID_BASICIDE_STAT_POS );
        if (pSrcEditWin->GetTextEngine()->IsModified() )
        {
            rBindings.Invalidate( SID_SAVEDOC );
            rBindings.Invalidate( SID_DOC_MODIFIED );
        }
        if( rKEvt.GetKeyCode().GetCode() == KEY_INSERT )
            rBindings.Invalidate( SID_ATTR_INSERT );
    }

    rBindings.Invalidate( SID_CUT );
    rBindings.Invalidate( SID_COPY );

    SwDocShell* pDocShell = pSrcEditWin->GetSrcView()->GetDocShell();
    if(pSrcEditWin->GetTextEngine()->IsModified())
    {
        pDocShell->SetModified();
    }
}

void TextViewOutWin::Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle& rRect)
{
    m_pTextView->Paint(rRenderContext, rRect);
}

void SwSrcEditWindow::CreateTextEngine()
{
    // FIXME RenderContext

    const Color &rCol = GetSettings().GetStyleSettings().GetWindowColor();
    m_pOutWin = VclPtr<TextViewOutWin>::Create(this, 0);
    m_pOutWin->SetBackground(Wallpaper(rCol));
    m_pOutWin->SetPointer(PointerStyle::Text);
    m_pOutWin->Show();

    // create Scrollbars
    m_pHScrollbar = VclPtr<ScrollAdaptor>::Create(this, true);
    m_pHScrollbar->EnableRTL( false );
    m_pHScrollbar->SetScrollHdl(LINK(this, SwSrcEditWindow, HorzScrollHdl));
    m_pHScrollbar->Show();

    m_pVScrollbar = VclPtr<ScrollAdaptor>::Create(this, false);
    m_pVScrollbar->EnableRTL( false );
    m_pVScrollbar->SetScrollHdl(LINK(this, SwSrcEditWindow, VertScrollHdl));
    m_pVScrollbar->Show();

    m_pTextEngine.reset(new ExtTextEngine);
    m_pTextView.reset(new TextView( m_pTextEngine.get(), m_pOutWin ));
    m_pTextView->SetAutoIndentMode(true);
    m_pOutWin->SetTextView(m_pTextView.get());

    m_pTextEngine->SetUpdateMode( false );
    m_pTextEngine->InsertView( m_pTextView.get() );

    vcl::Font aFont;
    aFont.SetTransparent( false );
    aFont.SetFillColor( rCol );
    SetPointFont(*GetOutDev(), aFont);
    aFont = GetFont();
    aFont.SetFillColor( rCol );
    m_pOutWin->SetFont( aFont );
    m_pTextEngine->SetFont( aFont );

    m_aSyntaxIdle.SetInvokeHandler( LINK( this, SwSrcEditWindow, SyntaxTimerHdl ) );

    m_pTextEngine->EnableUndo( true );
    m_pTextEngine->SetUpdateMode( true );

    m_pTextView->ShowCursor();
    InitScrollBars();
    StartListening( *m_pTextEngine );

    SfxBindings& rBind = GetSrcView()->GetViewFrame().GetBindings();
    rBind.Invalidate( SID_TABLE_CELL );
}

void SwSrcEditWindow::SetScrollBarRanges()
{
    // Extra method, not InitScrollBars, because also for TextEngine events.

    m_pHScrollbar->SetRange( Range( 0, m_nCurTextWidth-1 ) );
    m_pVScrollbar->SetRange( Range(0, m_pTextEngine->GetTextHeight()-1) );
}

void SwSrcEditWindow::InitScrollBars()
{
    SetScrollBarRanges();

    Size aOutSz( m_pOutWin->GetOutputSizePixel() );
    m_pVScrollbar->SetVisibleSize( aOutSz.Height() );
    m_pVScrollbar->SetPageSize(  aOutSz.Height() * 8 / 10 );
    m_pVScrollbar->SetLineSize( m_pOutWin->GetTextHeight() );
    m_pVScrollbar->SetThumbPos( m_pTextView->GetStartDocPos().Y() );
    m_pHScrollbar->SetVisibleSize( aOutSz.Width() );
    m_pHScrollbar->SetPageSize( aOutSz.Width() * 8 / 10 );
    m_pHScrollbar->SetLineSize( m_pOutWin->GetTextWidth(OUString('x')) );
    m_pHScrollbar->SetThumbPos( m_pTextView->GetStartDocPos().X() );

}

IMPL_LINK_NOARG(SwSrcEditWindow, HorzScrollHdl, weld::Scrollbar&, void)
{
    tools::Long nDiff = m_pTextView->GetStartDocPos().X() - m_pHScrollbar->GetThumbPos();
    GetTextView()->Scroll( nDiff, 0 );
    m_pTextView->ShowCursor( false );
    m_pHScrollbar->SetThumbPos( m_pTextView->GetStartDocPos().X() );
    GetSrcView()->GetViewFrame().GetBindings().Invalidate( SID_TABLE_CELL );
}

IMPL_LINK_NOARG(SwSrcEditWindow, VertScrollHdl, weld::Scrollbar&, void)
{
    tools::Long nDiff = m_pTextView->GetStartDocPos().Y() - m_pVScrollbar->GetThumbPos();
    GetTextView()->Scroll( 0, nDiff );
    m_pTextView->ShowCursor( false );
    m_pVScrollbar->SetThumbPos( m_pTextView->GetStartDocPos().Y() );
    GetSrcView()->GetViewFrame().GetBindings().Invalidate( SID_TABLE_CELL );
}

IMPL_LINK( SwSrcEditWindow, SyntaxTimerHdl, Timer*, pIdle, void )
{
    tools::Time aSyntaxCheckStart( tools::Time::SYSTEM );
    SAL_WARN_IF(m_pTextView == nullptr, "sw", "No View yet, but syntax highlighting?!");

    m_bHighlighting = true;
    sal_uInt16 nCount  = 0;
    // at first the region around the cursor is processed
    TextSelection aSel = m_pTextView->GetSelection();
    sal_uInt16 nCur = o3tl::narrowing<sal_uInt16>(aSel.GetStart().GetPara());
    if(nCur > 40)
        nCur -= 40;
    else
        nCur = 0;
    if(!m_aSyntaxLineTable.empty())
        for(sal_uInt16 i = 0; i < 80 && nCount < 40; i++, nCur++)
        {
            if(m_aSyntaxLineTable.find(nCur) != m_aSyntaxLineTable.end())
            {
                DoSyntaxHighlight( nCur );
                m_aSyntaxLineTable.erase( nCur );
                nCount++;
                if(m_aSyntaxLineTable.empty())
                    break;
                if((tools::Time( tools::Time::SYSTEM ).GetTime() - aSyntaxCheckStart.GetTime()) > MAX_HIGHLIGHTTIME )
                {
                    break;
                }
            }
        }

    // when there is still anything left by then, go on from the beginning
    while ( !m_aSyntaxLineTable.empty() && nCount < MAX_SYNTAX_HIGHLIGHT)
    {
        sal_uInt16 nLine = *m_aSyntaxLineTable.begin();
        DoSyntaxHighlight( nLine );
        m_aSyntaxLineTable.erase(nLine);
        nCount ++;
        if(tools::Time( tools::Time::SYSTEM ).GetTime() - aSyntaxCheckStart.GetTime() > MAX_HIGHLIGHTTIME)
        {
            break;
        }
    }

    if(!m_aSyntaxLineTable.empty() && !pIdle->IsActive())
        pIdle->Start();
    // SyntaxTimerHdl is called when text changed
    // => good opportunity to determine text width!
    tools::Long nPrevTextWidth = m_nCurTextWidth;
    m_nCurTextWidth = m_pTextEngine->CalcTextWidth() + 25;  // small tolerance
    if ( m_nCurTextWidth != nPrevTextWidth )
        SetScrollBarRanges();
    m_bHighlighting = false;
}

void SwSrcEditWindow::DoSyntaxHighlight( sal_uInt16 nPara )
{
    // Because of DelayedSyntaxHighlight it could happen,
    // that the line doesn't exist anymore!
    if ( nPara >= m_pTextEngine->GetParagraphCount() )
        return;

    bool bTempModified = IsModified();
    m_pTextEngine->RemoveAttribs( nPara );
    OUString aSource( m_pTextEngine->GetText( nPara ) );
    m_pTextEngine->SetUpdateMode( false );
    ImpDoHighlight( aSource, nPara );
    TextView* pTmp = m_pTextEngine->GetActiveView();
    pTmp->SetAutoScroll(false);
    m_pTextEngine->SetActiveView(nullptr);
    m_pTextEngine->SetUpdateMode( true );
    m_pTextEngine->SetActiveView(pTmp);
    pTmp->SetAutoScroll(true);
    pTmp->ShowCursor( false/*pTmp->IsAutoScroll()*/ );

    if(!bTempModified)
        ClearModifyFlag();

}

void SwSrcEditWindow::ImpDoHighlight( std::u16string_view aSource, sal_uInt16 nLineOff )
{
    TextPortions aPortionList;
    lcl_Highlight(aSource, aPortionList);

    size_t nCount = aPortionList.size();
    if ( !nCount )
        return;

    TextPortion& rLast = aPortionList[nCount-1];
    if ( rLast.nStart > rLast.nEnd )    // Only until Bug from MD is resolved
    {
        nCount--;
        aPortionList.pop_back();
        if ( !nCount )
            return;
    }

    {
        // Only blanks and tabs have to be attributed along.
        // When two identical attributes are placed consecutively,
        // it optimises the TextEngine.
        sal_uInt16 nLastEnd = 0;

        for ( size_t i = 0; i < nCount; i++ )
        {
            TextPortion& r = aPortionList[i];
            if ( r.nStart > r.nEnd )    // only until Bug from MD is resolved
                continue;

            if ( r.nStart > nLastEnd )
            {
                // Can I rely on the fact that all except blank and tab
                // are being highlighted?!
                r.nStart = nLastEnd;
            }
            nLastEnd = r.nEnd+1;
            if ( ( i == (nCount-1) ) && ( r.nEnd < aSource.size() ) )
                r.nEnd = aSource.size();
        }
    }

    for (TextPortion & r : aPortionList)
    {
        if ( r.nStart > r.nEnd )    // only until Bug from MD is resolved
            continue;
        if(r.eType !=  svtools::HTMLSGML    &&
            r.eType != svtools::HTMLCOMMENT &&
            r.eType != svtools::HTMLKEYWORD &&
            r.eType != svtools::HTMLUNKNOWN)
                r.eType = svtools::HTMLUNKNOWN;
        Color aColor(SwModule::get()->GetColorConfig().GetColorValue(r.eType).nColor);
        m_pTextEngine->SetAttrib( TextAttribFontColor( aColor ), nLineOff, r.nStart, r.nEnd+1 );
    }
}

void SwSrcEditWindow::Notify( SfxBroadcaster& /*rBC*/, const SfxHint& rHint )
{
    switch (rHint.GetId())
    {
        case SfxHintId::TextViewScrolled:
            m_pHScrollbar->SetThumbPos( m_pTextView->GetStartDocPos().X() );
            m_pVScrollbar->SetThumbPos( m_pTextView->GetStartDocPos().Y() );
            break;

        case SfxHintId::TextHeightChanged:
            if ( m_pTextEngine->GetTextHeight() < m_pOutWin->GetOutputSizePixel().Height() )
                m_pTextView->Scroll( 0, m_pTextView->GetStartDocPos().Y() );
            m_pVScrollbar->SetThumbPos( m_pTextView->GetStartDocPos().Y() );
            SetScrollBarRanges();
            break;

        case SfxHintId::TextParaInserted:
        case SfxHintId::TextParaContentChanged:
            if ( !m_bHighlighting )
            {
                auto pTextHint = static_cast<const TextHint*>(&rHint);
                m_aSyntaxLineTable.insert( o3tl::narrowing<sal_uInt16>(pTextHint->GetValue()) );
                m_aSyntaxIdle.Start();
            }
            break;
        default: break;
    }
}

void SwSrcEditWindow::ImplInvalidate(const vcl::Region* pRegion, InvalidateFlags nFlags)
{
    m_pOutWin->Invalidate();
    Window::ImplInvalidate(pRegion, nFlags);
}

void SwSrcEditWindow::Command( const CommandEvent& rCEvt )
{
    switch(rCEvt.GetCommand())
    {
        case CommandEventId::Wheel:
        case CommandEventId::StartAutoScroll:
        case CommandEventId::AutoScroll:
        {
            const CommandWheelData* pWData = rCEvt.GetWheelData();
            if( !pWData || CommandWheelMode::ZOOM != pWData->GetMode() )
                HandleScrollCommand( rCEvt, m_pHScrollbar, m_pVScrollbar );
        }
        break;
        default:
            Window::Command(rCEvt);
    }
}

void SwSrcEditWindow::HandleWheelCommand( const CommandEvent& rCEvt )
{
    m_pTextView->Command(rCEvt);
    HandleScrollCommand( rCEvt, m_pHScrollbar, m_pVScrollbar );
}

void SwSrcEditWindow::GetFocus()
{
    if (m_pOutWin)
        m_pOutWin->GrabFocus();
}

static bool lcl_GetLanguagesForEncoding(rtl_TextEncoding eEnc, LanguageType aLanguages[])
{
    switch(eEnc)
    {
        case RTL_TEXTENCODING_UTF7             :
        case RTL_TEXTENCODING_UTF8             :
            // don#t fill - all LANGUAGE_SYSTEM means unicode font has to be used
        break;

        case RTL_TEXTENCODING_ISO_8859_3:
        case RTL_TEXTENCODING_ISO_8859_1  :
        case RTL_TEXTENCODING_MS_1252     :
        case RTL_TEXTENCODING_APPLE_ROMAN :
        case RTL_TEXTENCODING_IBM_850     :
        case RTL_TEXTENCODING_ISO_8859_14 :
        case RTL_TEXTENCODING_ISO_8859_15 :
            //fill with western languages
            aLanguages[0] = LANGUAGE_GERMAN;
            aLanguages[1] = LANGUAGE_FRENCH;
            aLanguages[2] = LANGUAGE_ITALIAN;
            aLanguages[3] = LANGUAGE_SPANISH;
        break;

        case RTL_TEXTENCODING_IBM_865     :
            //scandinavian
            aLanguages[0] = LANGUAGE_FINNISH;
            aLanguages[1] = LANGUAGE_NORWEGIAN;
            aLanguages[2] = LANGUAGE_SWEDISH;
            aLanguages[3] = LANGUAGE_DANISH;
        break;

        case RTL_TEXTENCODING_ISO_8859_10      :
        case RTL_TEXTENCODING_ISO_8859_13      :
        case RTL_TEXTENCODING_ISO_8859_2  :
        case RTL_TEXTENCODING_IBM_852     :
        case RTL_TEXTENCODING_MS_1250     :
        case RTL_TEXTENCODING_APPLE_CENTEURO   :
            aLanguages[0] = LANGUAGE_POLISH;
            aLanguages[1] = LANGUAGE_CZECH;
            aLanguages[2] = LANGUAGE_HUNGARIAN;
            aLanguages[3] = LANGUAGE_SLOVAK;
        break;

        case RTL_TEXTENCODING_ISO_8859_4  :
        case RTL_TEXTENCODING_IBM_775     :
        case RTL_TEXTENCODING_MS_1257          :
            aLanguages[0] = LANGUAGE_LATVIAN   ;
            aLanguages[1] = LANGUAGE_LITHUANIAN;
            aLanguages[2] = LANGUAGE_ESTONIAN  ;
        break;

        case RTL_TEXTENCODING_IBM_863       : aLanguages[0] = LANGUAGE_FRENCH_CANADIAN; break;
        case RTL_TEXTENCODING_APPLE_FARSI   : aLanguages[0] = LANGUAGE_FARSI; break;
        case RTL_TEXTENCODING_APPLE_ROMANIAN:aLanguages[0] = LANGUAGE_ROMANIAN; break;

        case RTL_TEXTENCODING_IBM_861     :
        case RTL_TEXTENCODING_APPLE_ICELAND    :
            aLanguages[0] = LANGUAGE_ICELANDIC;
        break;

        case RTL_TEXTENCODING_APPLE_CROATIAN:aLanguages[0] = LANGUAGE_CROATIAN; break;

        case RTL_TEXTENCODING_IBM_437     :
        case RTL_TEXTENCODING_ASCII_US    : aLanguages[0] = LANGUAGE_ENGLISH; break;

        case RTL_TEXTENCODING_IBM_862     :
        case RTL_TEXTENCODING_MS_1255     :
        case RTL_TEXTENCODING_APPLE_HEBREW     :
        case RTL_TEXTENCODING_ISO_8859_8  :
            aLanguages[0] = LANGUAGE_HEBREW;
        break;

        case RTL_TEXTENCODING_IBM_857     :
        case RTL_TEXTENCODING_MS_1254     :
        case RTL_TEXTENCODING_APPLE_TURKISH:
        case RTL_TEXTENCODING_ISO_8859_9  :
            aLanguages[0] = LANGUAGE_TURKISH;
        break;

        case RTL_TEXTENCODING_IBM_860     :
            aLanguages[0] = LANGUAGE_PORTUGUESE;
        break;

        case RTL_TEXTENCODING_IBM_869     :
        case RTL_TEXTENCODING_MS_1253     :
        case RTL_TEXTENCODING_APPLE_GREEK :
        case RTL_TEXTENCODING_ISO_8859_7  :
        case RTL_TEXTENCODING_IBM_737     :
            aLanguages[0] = LANGUAGE_GREEK;
        break;

        case RTL_TEXTENCODING_KOI8_R      :
        case RTL_TEXTENCODING_ISO_8859_5  :
        case RTL_TEXTENCODING_IBM_855     :
        case RTL_TEXTENCODING_MS_1251     :
        case RTL_TEXTENCODING_IBM_866     :
        case RTL_TEXTENCODING_APPLE_CYRILLIC   :
            aLanguages[0] = LANGUAGE_RUSSIAN;
        break;

        case RTL_TEXTENCODING_APPLE_UKRAINIAN:
        case RTL_TEXTENCODING_KOI8_U:
            aLanguages[0] = LANGUAGE_UKRAINIAN;
            break;

        case RTL_TEXTENCODING_IBM_864     :
        case RTL_TEXTENCODING_MS_1256          :
        case RTL_TEXTENCODING_ISO_8859_6  :
        case RTL_TEXTENCODING_APPLE_ARABIC :
            aLanguages[0] = LANGUAGE_ARABIC_SAUDI_ARABIA;
         break;

        case RTL_TEXTENCODING_APPLE_CHINTRAD   :
        case RTL_TEXTENCODING_MS_950           :
        case RTL_TEXTENCODING_GBT_12345        :
        case RTL_TEXTENCODING_BIG5             :
        case RTL_TEXTENCODING_EUC_TW           :
        case RTL_TEXTENCODING_BIG5_HKSCS       :
            aLanguages[0] = LANGUAGE_CHINESE_TRADITIONAL;
        break;

        case RTL_TEXTENCODING_EUC_JP           :
        case RTL_TEXTENCODING_ISO_2022_JP      :
        case RTL_TEXTENCODING_JIS_X_0201       :
        case RTL_TEXTENCODING_JIS_X_0208       :
        case RTL_TEXTENCODING_JIS_X_0212       :
        case RTL_TEXTENCODING_APPLE_JAPANESE   :
        case RTL_TEXTENCODING_MS_932           :
        case RTL_TEXTENCODING_SHIFT_JIS        :
            aLanguages[0] = LANGUAGE_JAPANESE;
        break;

        case RTL_TEXTENCODING_GB_2312          :
        case RTL_TEXTENCODING_MS_936           :
        case RTL_TEXTENCODING_GBK              :
        case RTL_TEXTENCODING_GB_18030         :
        case RTL_TEXTENCODING_APPLE_CHINSIMP   :
        case RTL_TEXTENCODING_EUC_CN           :
        case RTL_TEXTENCODING_ISO_2022_CN      :
            aLanguages[0] = LANGUAGE_CHINESE_SIMPLIFIED;
        break;

        case RTL_TEXTENCODING_APPLE_KOREAN     :
        case RTL_TEXTENCODING_MS_949           :
        case RTL_TEXTENCODING_EUC_KR           :
        case RTL_TEXTENCODING_ISO_2022_KR      :
        case RTL_TEXTENCODING_MS_1361          :
            aLanguages[0] = LANGUAGE_KOREAN;
        break;

        case RTL_TEXTENCODING_APPLE_THAI       :
        case RTL_TEXTENCODING_MS_874      :
        case RTL_TEXTENCODING_TIS_620          :
            aLanguages[0] = LANGUAGE_THAI;
        break;
        default: aLanguages[0] = Application::GetSettings().GetUILanguageTag().getLanguageType();
    }
    return aLanguages[0] != LANGUAGE_SYSTEM;
}
void SwSrcEditWindow::SetFont()
{
    OUString sFontName(
        officecfg::Office::Common::Font::SourceViewFont::FontName::get().
        value_or(OUString()));
    if(sFontName.isEmpty())
    {
        LanguageType aLanguages[5] =
        {
            LANGUAGE_SYSTEM, LANGUAGE_SYSTEM, LANGUAGE_SYSTEM, LANGUAGE_SYSTEM, LANGUAGE_SYSTEM
        };
        vcl::Font aFont;
        if(lcl_GetLanguagesForEncoding(m_eSourceEncoding, aLanguages))
        {
            //TODO: check for multiple languages
            aFont = OutputDevice::GetDefaultFont(DefaultFontType::FIXED, aLanguages[0], GetDefaultFontFlags::NONE, GetOutDev());
        }
        else
            aFont = OutputDevice::GetDefaultFont(DefaultFontType::SANS_UNICODE,
                        Application::GetSettings().GetLanguageTag().getLanguageType(), GetDefaultFontFlags::NONE, GetOutDev());
        sFontName = aFont.GetFamilyName();
    }
    const SvxFontListItem* pFontListItem =
        static_cast<const SvxFontListItem* >(m_pSrcView->GetDocShell()->GetItem( SID_ATTR_CHAR_FONTLIST ));
    const FontList* pList = pFontListItem->GetFontList();
    vcl::Font aFont(pList->Get(sFontName, WEIGHT_NORMAL, ITALIC_NONE));

    const vcl::Font& rFont = GetTextEngine()->GetFont();
    Size aSize(rFont.GetFontSize());
    //font height is stored in point and set in twip
    aSize.setHeight(
        officecfg::Office::Common::Font::SourceViewFont::FontHeight::get() * 20 );
    aFont.SetFontSize(m_pOutWin->LogicToPixel(aSize, MapMode(MapUnit::MapTwip)));
    GetTextEngine()->SetFont( aFont );
    m_pOutWin->SetFont(aFont);
}

void SwSrcEditWindow::SetTextEncoding(rtl_TextEncoding eEncoding)
{
    m_eSourceEncoding = eEncoding;
    SetFont();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
