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

#include <algorithm>
#include <memory>
#include <config_java.h>

#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>
#include <com/sun/star/document/XDocumentProperties.hpp>
#include <com/sun/star/i18n/ScriptType.hpp>
#include <com/sun/star/i18n/XBreakIterator.hpp>
#include <comphelper/string.hxx>
#include <o3tl/safeint.hxx>
#include <rtl/ustrbuf.hxx>
#include <svx/svxids.hrc>
#include <svx/svdotext.hxx>
#if OSL_DEBUG_LEVEL > 0
#include <stdlib.h>
#endif
#include <hintids.hxx>

#include <utility>
#include <vcl/errinf.hxx>
#include <svl/stritem.hxx>
#include <svl/itemiter.hxx>
#include <vcl/imap.hxx>
#include <svtools/htmltokn.h>
#include <svtools/htmlkywd.hxx>
#include <svtools/ctrltool.hxx>
#include <unotools/pathoptions.hxx>
#include <vcl/svapp.hxx>
#include <sfx2/event.hxx>
#include <sfx2/docfile.hxx>

#include <sfx2/linkmgr.hxx>
#include <editeng/kernitem.hxx>
#include <editeng/boxitem.hxx>
#include <editeng/fhgtitem.hxx>
#include <editeng/formatbreakitem.hxx>
#include <editeng/postitem.hxx>
#include <editeng/wghtitem.hxx>
#include <editeng/crossedoutitem.hxx>
#include <editeng/udlnitem.hxx>
#include <editeng/escapementitem.hxx>
#include <editeng/blinkitem.hxx>
#include <editeng/ulspitem.hxx>
#include <editeng/colritem.hxx>
#include <editeng/fontitem.hxx>
#include <editeng/adjustitem.hxx>
#include <editeng/lrspitem.hxx>
#include <editeng/protitem.hxx>
#include <editeng/flstitem.hxx>
#include <svx/unobrushitemhelper.hxx>

#include <frmatr.hxx>
#include <charatr.hxx>
#include <fmtfld.hxx>
#include <fmtpdsc.hxx>
#include <fmtanchr.hxx>
#include <fmtsrnd.hxx>
#include <fmtfsize.hxx>
#include <fmtclds.hxx>
#include <fchrfmt.hxx>
#include <fmtinfmt.hxx>
#include <fmtfollowtextflow.hxx>
#include <fmtornt.hxx>
#include <doc.hxx>
#include <IDocumentUndoRedo.hxx>
#include <IDocumentSettingAccess.hxx>
#include <IDocumentLayoutAccess.hxx>
#include <IDocumentLinksAdministration.hxx>
#include <IDocumentRedlineAccess.hxx>
#include <IDocumentFieldsAccess.hxx>
#include <IDocumentStylePoolAccess.hxx>
#include <IDocumentStatistics.hxx>
#include <IDocumentState.hxx>
#include <pam.hxx>
#include <ndtxt.hxx>
#include <mdiexp.hxx>
#include <poolfmt.hxx>
#include <pagedesc.hxx>
#include <IMark.hxx>
#include <docsh.hxx>
#include <editsh.hxx>
#include <docufld.hxx>
#include "swcss1.hxx"
#include <fltini.hxx>
#include <htmltbl.hxx>
#include "htmlnum.hxx"
#include "swhtml.hxx"
#include "wrthtml.hxx"
#include <linkenum.hxx>
#include <breakit.hxx>
#include <SwAppletImpl.hxx>
#include <swdll.hxx>
#include <txatbase.hxx>

#include <sfx2/viewfrm.hxx>
#include <svx/svdobj.hxx>
#include <officecfg/Office/Writer.hxx>
#include <comphelper/sequenceashashmap.hxx>
#include <comphelper/sequence.hxx>
#include <officecfg/Office/Common.hxx>

#include <swerror.h>
#include <ndole.hxx>
#include <unoframe.hxx>
#include "css1atr.hxx"
#include <frameformats.hxx>

#define FONTSIZE_MASK           7

#define HTML_ESC_PROP 80
#define HTML_ESC_SUPER DFLT_ESC_SUPER
#define HTML_ESC_SUB DFLT_ESC_SUB

#define HTML_SPTYPE_BLOCK 1
#define HTML_SPTYPE_HORI 2
#define HTML_SPTYPE_VERT 3

using editeng::SvxBorderLine;
using namespace ::com::sun::star;

// <P ALIGN=xxx>, <Hn ALIGN=xxx>, <TD ALIGN=xxx> etc.
HTMLOptionEnum<SvxAdjust> const aHTMLPAlignTable[] =
{
    { OOO_STRING_SVTOOLS_HTML_AL_left,    SvxAdjust::Left     },
    { OOO_STRING_SVTOOLS_HTML_AL_center,  SvxAdjust::Center   },
    { OOO_STRING_SVTOOLS_HTML_AL_middle,  SvxAdjust::Center   }, // Netscape
    { OOO_STRING_SVTOOLS_HTML_AL_right,   SvxAdjust::Right    },
    { OOO_STRING_SVTOOLS_HTML_AL_justify, SvxAdjust::Block    },
    { OOO_STRING_SVTOOLS_HTML_AL_char,    SvxAdjust::Left     },
    { nullptr,                            SvxAdjust(0)        }
};

// <SPACER TYPE=...>
HTMLOptionEnum<sal_uInt16> const aHTMLSpacerTypeTable[] =
{
    { OOO_STRING_SVTOOLS_HTML_SPTYPE_block,      HTML_SPTYPE_BLOCK       },
    { OOO_STRING_SVTOOLS_HTML_SPTYPE_horizontal, HTML_SPTYPE_HORI        },
    { OOO_STRING_SVTOOLS_HTML_SPTYPE_vertical,   HTML_SPTYPE_VERT        },
    { nullptr,                                   0                       }
};

HTMLReader::HTMLReader()
{
    m_bTemplateBrowseMode = true;
}

OUString HTMLReader::GetTemplateName(SwDoc& rDoc) const
{
    if (!rDoc.getIDocumentSettingAccess().get(DocumentSettingId::HTML_MODE))
        // HTML import into Writer, avoid loading the Writer/Web template.
        return OUString();

    static constexpr OUString sTemplateWithoutExt(u"internal/html"_ustr);
    SvtPathOptions aPathOpt;

    // first search for OpenDocument Writer/Web template
    // OpenDocument Writer/Web template (extension .oth)
    OUString sTemplate( sTemplateWithoutExt + ".oth" );
    if (aPathOpt.SearchFile( sTemplate, SvtPathOptions::Paths::Template ))
        return sTemplate;

        // no OpenDocument Writer/Web template found.
        // search for OpenOffice.org Writer/Web template
    sTemplate = sTemplateWithoutExt + ".stw";
    if (aPathOpt.SearchFile( sTemplate, SvtPathOptions::Paths::Template ))
        return sTemplate;

    OSL_ENSURE( false, "The default HTML template cannot be found in the defined template directories!");

    return OUString();
}

bool HTMLReader::SetStrmStgPtr()
{
    assert(m_pMedium && "Where is the medium??");

    if( m_pMedium->IsRemote() || !m_pMedium->IsStorage() )
    {
        m_pStream = m_pMedium->GetInStream();
        return true;
    }
    return false;

}

// Call for the general Reader-Interface
ErrCodeMsg HTMLReader::Read( SwDoc &rDoc, const OUString& rBaseURL, SwPaM &rPam, const OUString & rName )
{
    SetupFilterOptions();

    if( !m_pStream )
    {
        OSL_ENSURE( m_pStream, "HTML-Read without stream" );
        return ERR_SWG_READ_ERROR;
    }

    if( !m_bInsertMode )
    {
        Reader::ResetFrameFormats( rDoc );

        // Set the HTML page style, when it isn't a HTML document,
        // otherwise it's already set.
        if( !rDoc.getIDocumentSettingAccess().get(DocumentSettingId::HTML_MODE) && m_aNamespace != "reqif-xhtml" )
        {
            rDoc.getIDocumentContentOperations().InsertPoolItem( rPam, SwFormatPageDesc(
                rDoc.getIDocumentStylePoolAccess().GetPageDescFromPool( RES_POOLPAGE_HTML, false )) );
        }
    }

    // so nobody steals the document!
    rtl::Reference<SwDoc> xHoldAlive(&rDoc);
    ErrCodeMsg nRet = ERRCODE_NONE;
    tools::SvRef<SwHTMLParser> xParser = new SwHTMLParser( &rDoc, rPam, *m_pStream,
                                            rName, rBaseURL, !m_bInsertMode, m_pMedium,
                                            IsReadUTF8(),
                                            m_bIgnoreHTMLComments, m_aNamespace );

    SvParserState eState = xParser->CallParser();

    if( SvParserState::Pending == eState )
        m_pStream->ResetError();
    else if( SvParserState::Accepted != eState )
    {
        const OUString sErr(OUString::number(static_cast<sal_Int32>(xParser->GetLineNr()))
            + "," + OUString::number(static_cast<sal_Int32>(xParser->GetLinePos())));

        // use the stream as transport for error number
        nRet = ErrCodeMsg( ERR_FORMAT_ROWCOL, sErr,
                                    DialogMask::ButtonsOk | DialogMask::MessageError );
    }

    return nRet;
}

SwHTMLParser::SwHTMLParser( SwDoc* pD, SwPaM& rCursor, SvStream& rIn,
                            OUString aPath,
                            OUString aBaseURL,
                            bool bReadNewDoc,
                            SfxMedium* pMed, bool bReadUTF8,
                            bool bNoHTMLComments,
                            const OUString& rNamespace )
    : SfxHTMLParser( rIn, bReadNewDoc, pMed ),
    m_aPathToFile(std::move( aPath )),
    m_sBaseURL(std::move( aBaseURL )),
    m_xAttrTab(std::make_shared<HTMLAttrTable>()),
    m_pNumRuleInfo( new SwHTMLNumRuleInfo ),
    m_xDoc( pD ),
    m_pActionViewShell( nullptr ),
    m_pSttNdIdx( nullptr ),
    m_pFormImpl( nullptr ),
    m_pImageMap( nullptr ),
    m_nBaseFontStMin( 0 ),
    m_nFontStMin( 0 ),
    m_nDefListDeep( 0 ),
    m_nFontStHeadStart( 0 ),
    m_nSBModuleCnt( 0 ),
    m_nMissingImgMaps( 0 ),
    m_nParaCnt( 5 ),
    // #i83625#
    m_nContextStMin( 0 ),
    m_nContextStAttrMin( 0 ),
    m_nSelectEntryCnt( 0 ),
    m_nOpenParaToken( HtmlTokenId::NONE ),
    m_eJumpTo( JumpToMarks::NONE ),
#ifdef DBG_UTIL
    m_nContinue( 0 ),
#endif
    m_eParaAdjust( SvxAdjust::End ),
    m_bDocInitialized( false ),
    m_bSetModEnabled( false ),
    m_bInFloatingFrame( false ),
    m_bInField( false ),
    m_bKeepUnknown( false ),
    m_bCallNextToken( false ),
    m_bIgnoreRawData( false ),
    m_bLBEntrySelected ( false ),
    m_bTAIgnoreNewPara ( false ),
    m_bFixMarqueeWidth ( false ),
    m_bNoParSpace( false ),
    m_bInNoEmbed( false ),
    m_bInTitle( false ),
    m_bUpdateDocStat( false ),
    m_bFixSelectWidth( false ),
    m_bTextArea( false ),
    m_bSelect( false ),
    m_bInFootEndNoteAnchor( false ),
    m_bInFootEndNoteSymbol( false ),
    m_bIgnoreHTMLComments( bNoHTMLComments ),
    m_bRemoveHidden( false ),
    m_bBodySeen( false ),
    m_bReadingHeaderOrFooter( false ),
    m_bNotifyMacroEventRead( false ),
    m_isInTableStructure(false),
    m_nTableDepth( 0 ),
    m_nFloatingFrames( 0 ),
    m_nListItems( 0 ),
    m_pTempViewFrame(nullptr)
{
    // If requested explicitly, then force ignoring of comments (don't create postits for them).
    if (!bFuzzing)
    {
        if (officecfg::Office::Writer::Filter::Import::HTML::IgnoreComments::get())
            m_bIgnoreHTMLComments = true;
        m_bKeepUnknown = officecfg::Office::Common::Filter::HTML::Import::UnknownTag::get();
    }

    m_nEventId = nullptr;
    m_bUpperSpace = m_bViewCreated = m_bChkJumpMark = false;

    m_eScriptLang = HTMLScriptLanguage::Unknown;

    rCursor.DeleteMark();
    m_pPam = &rCursor; // re-use existing cursor: avoids spurious ~SwContentIndexReg assert
    memset(m_xAttrTab.get(), 0, sizeof(HTMLAttrTable));

    // Read the font sizes 1-7 from the INI file
    if (!bFuzzing)
    {
        m_aFontHeights[0] = officecfg::Office::Common::Filter::HTML::Import::FontSize::Size_1::get() * 20;
        m_aFontHeights[1] = officecfg::Office::Common::Filter::HTML::Import::FontSize::Size_2::get() * 20;
        m_aFontHeights[2] = officecfg::Office::Common::Filter::HTML::Import::FontSize::Size_3::get() * 20;
        m_aFontHeights[3] = officecfg::Office::Common::Filter::HTML::Import::FontSize::Size_4::get() * 20;
        m_aFontHeights[4] = officecfg::Office::Common::Filter::HTML::Import::FontSize::Size_5::get() * 20;
        m_aFontHeights[5] = officecfg::Office::Common::Filter::HTML::Import::FontSize::Size_6::get() * 20;
        m_aFontHeights[6] = officecfg::Office::Common::Filter::HTML::Import::FontSize::Size_7::get() * 20;
    }
    else
    {
        m_aFontHeights[0] = m_aFontHeights[1] = m_aFontHeights[2] = m_aFontHeights[3] =
        m_aFontHeights[4] = m_aFontHeights[5] = m_aFontHeights[6] = 12 * 20;
    }

    if(bReadNewDoc)
    {
        //CJK has different defaults, so a different object should be used for this
        //RES_CHARTR_CJK_FONTSIZE is a valid value
        SvxFontHeightItem aFontHeight(m_aFontHeights[2], 100, RES_CHRATR_FONTSIZE);
        m_xDoc->SetDefault( aFontHeight );
        SvxFontHeightItem aFontHeightCJK(m_aFontHeights[2], 100, RES_CHRATR_CJK_FONTSIZE);
        m_xDoc->SetDefault( aFontHeightCJK );
        SvxFontHeightItem aFontHeightCTL(m_aFontHeights[2], 100, RES_CHRATR_CTL_FONTSIZE);
        m_xDoc->SetDefault( aFontHeightCTL );

        // #i18732# - adjust default of option 'FollowTextFlow'
        // TODO: not sure what the appropriate default for HTML should be?
        m_xDoc->SetDefault( SwFormatFollowTextFlow(true) );
    }

    // Change to HTML mode during the import, so that the right styles are created
    m_bOldIsHTMLMode = m_xDoc->getIDocumentSettingAccess().get(DocumentSettingId::HTML_MODE);
    m_xDoc->getIDocumentSettingAccess().set(DocumentSettingId::HTML_MODE, true);

    m_pCSS1Parser.reset(new SwCSS1Parser(m_xDoc.get(), *this, m_aFontHeights, m_sBaseURL, IsNewDoc()));
    if (!bFuzzing)
        m_pCSS1Parser->SetIgnoreFontFamily( officecfg::Office::Common::Filter::HTML::Import::FontSetting::get() );

    if( bReadUTF8 )
    {
        SetSrcEncoding( RTL_TEXTENCODING_UTF8 );
    }
    else
    {
        SwDocShell *pDocSh = m_xDoc->GetDocShell();
        SvKeyValueIterator *pHeaderAttrs =
            pDocSh->GetHeaderAttributes();
        if( pHeaderAttrs )
            SetEncodingByHTTPHeader( pHeaderAttrs );
    }
    m_pCSS1Parser->SetDfltEncoding( osl_getThreadTextEncoding() );

    SwDocShell* pDocSh = m_xDoc->GetDocShell();
    if( pDocSh )
    {
        m_bViewCreated = true;  // not, load synchronous

        // a jump mark is present

        if( pMed )
        {
            m_sJmpMark = pMed->GetURLObject().GetMark();
            if( !m_sJmpMark.isEmpty() )
            {
                m_eJumpTo = JumpToMarks::Mark;
                sal_Int32 nLastPos = m_sJmpMark.lastIndexOf( cMarkSeparator );
                sal_Int32 nPos =  nLastPos != -1 ? nLastPos : 0;

                OUString sCmp;
                if (nPos)
                {
                    sCmp = m_sJmpMark.copy(nPos + 1).replaceAll(" ", "");
                }

                if( !sCmp.isEmpty() )
                {
                    sCmp = sCmp.toAsciiLowerCase();
                    if( sCmp == "region" )
                        m_eJumpTo = JumpToMarks::Region;
                    else if( sCmp == "table" )
                        m_eJumpTo = JumpToMarks::Table;
                    else if( sCmp == "graphic" )
                        m_eJumpTo = JumpToMarks::Graphic;
                    else if( sCmp == "outline" ||
                            sCmp == "text" ||
                            sCmp == "frame" )
                        m_eJumpTo = JumpToMarks::NONE;  // this is nothing valid!
                    else
                        // otherwise this is a normal (book)mark
                        nPos = -1;
                }
                else
                    nPos = -1;

                if( nPos != -1 )
                    m_sJmpMark = m_sJmpMark.copy( 0, nPos );
                if( m_sJmpMark.isEmpty() )
                    m_eJumpTo = JumpToMarks::NONE;
            }
        }
    }

    if (!rNamespace.isEmpty())
    {
        SetNamespace(rNamespace);
        m_bXHTML = true;
        if (rNamespace == "reqif-xhtml")
            m_bReqIF = true;
    }

    // Extract load parameters which are specific to this filter.
    if (!pMed)
    {
        return;
    }

    comphelper::SequenceAsHashMap aLoadMap(pMed->GetArgs());
    auto it = aLoadMap.find(u"AllowedRTFOLEMimeTypes"_ustr);
    if (it == aLoadMap.end())
    {
        return;
    }

    uno::Sequence<OUString> aTypes;
    it->second >>= aTypes;
    m_aAllowedRTFOLEMimeTypes = comphelper::sequenceToContainer<std::set<OUString>>(aTypes);
}

SwHTMLParser::~SwHTMLParser()
{
#ifdef DBG_UTIL
    OSL_ENSURE( !m_nContinue, "DTOR in continue!" );
#endif

    OSL_ENSURE(m_aContexts.empty(), "There are still contexts on the stack");
    OSL_ENSURE(!m_nContextStMin, "There are protected contexts");
    m_nContextStMin = 0;
    while (!m_aContexts.empty())
    {
        std::unique_ptr<HTMLAttrContext> xCntxt(PopContext());
        ClearContext(xCntxt.get());
    }

    bool bAsync = m_xDoc->IsInLoadAsynchron();
    m_xDoc->SetInLoadAsynchron( false );
    m_xDoc->getIDocumentSettingAccess().set(DocumentSettingId::HTML_MODE, m_bOldIsHTMLMode);

    if( m_xDoc->GetDocShell() && m_nEventId )
        Application::RemoveUserEvent( m_nEventId );

    // the DocumentDetected maybe can delete the DocShells, therefore fetch again
    if (SwDocShell* pShell = m_xDoc->GetDocShell())
    {
        // update linked sections
        sal_uInt16 nLinkMode = m_xDoc->getIDocumentSettingAccess().getLinkUpdateMode( true );
        if( nLinkMode != NEVER && bAsync &&
            SfxObjectCreateMode::INTERNAL != pShell->GetCreateMode() )
        {
            SfxMedium * medium = pShell->GetMedium();
            m_xDoc->getIDocumentLinksAdministration().GetLinkManager().UpdateAllLinks( nLinkMode == MANUAL, false, nullptr, medium == nullptr ? OUString() : medium->GetName() );
        }

        if ( pShell->IsLoading() )
        {
            // #i59688#
            pShell->LoadingFinished();
        }
    }

    delete m_pSttNdIdx;

    if( !m_aSetAttrTab.empty() )
    {
        OSL_ENSURE( m_aSetAttrTab.empty(),"There are still attributes on the stack" );
        for ( const auto& rpAttr : m_aSetAttrTab )
            delete rpAttr;
        m_aSetAttrTab.clear();
    }

    m_pCSS1Parser.reset();
    m_pNumRuleInfo.reset();
    DeleteFormImpl();
    m_pFootEndNoteImpl.reset();

    OSL_ENSURE(!m_xTable, "It exists still an open table");
    m_pImageMaps.reset();

    OSL_ENSURE( m_vPendingStack.empty(),
            "SwHTMLParser::~SwHTMLParser: Here should not be Pending-Stack anymore" );
    m_vPendingStack.clear();

    m_xDoc.clear();

    if ( m_pTempViewFrame )
    {
        m_pTempViewFrame->DoClose();

        // the temporary view frame is hidden, so the hidden flag might need to be removed
        SwDocShell* pShell = m_xDoc.is() ? m_xDoc->GetDocShell() : nullptr;
        if ( m_bRemoveHidden && pShell && pShell->GetMedium() )
            pShell->GetMedium()->GetItemSet().ClearItem( SID_HIDDEN );
    }
}

IMPL_LINK_NOARG( SwHTMLParser, AsyncCallback, void*, void )
{
    m_nEventId=nullptr;

    // #i47907# - If the document has already been destructed,
    // the parser should be aware of this:
    SwDocShell* pShell = m_xDoc->GetDocShell();
    if( ( pShell && pShell->IsAbortingImport() )
        || 1 == m_xDoc->getReferenceCount() )
    {
        // was the import aborted by SFX?
        eState = SvParserState::Error;
    }

    GetAsynchCallLink().Call(nullptr);
}

SvParserState SwHTMLParser::CallParser()
{
    // create temporary index on position 0, so it won't be moved!
    m_pSttNdIdx = new SwNodeIndex( m_xDoc->GetNodes() );
    if( !IsNewDoc() )       // insert into existing document ?
    {
        const SwPosition* pPos = m_pPam->GetPoint();

        m_xDoc->getIDocumentContentOperations().SplitNode( *pPos, false );

        *m_pSttNdIdx = pPos->GetNodeIndex()-1;
        m_xDoc->getIDocumentContentOperations().SplitNode( *pPos, false );

        SwPaM aInsertionRangePam( *pPos );

        m_pPam->Move( fnMoveBackward );

        // split any redline over the insertion point
        aInsertionRangePam.SetMark();
        *aInsertionRangePam.GetPoint() = *m_pPam->GetPoint();
        aInsertionRangePam.Move( fnMoveBackward );
        m_xDoc->getIDocumentRedlineAccess().SplitRedline( aInsertionRangePam );

        ::std::unique_ptr<SfxItemSet> pSet{new SfxItemSet(m_xDoc->GetAttrPool(),
            svl::Items< RES_CHRATR_BEGIN, RES_CHRATR_END-1,
                        RES_PARATR_BEGIN, RES_PARATR_NUMRULE-1,
                        RES_PARATR_NUMRULE+1, RES_PARATR_CONNECT_BORDER,
                        RES_LR_SPACE, RES_UL_SPACE,
                        XATTR_FILL_FIRST, XATTR_FILL_LAST>)};
        if (pPos->GetNode().GetTextNode()->GetParaAttr(*pSet, 0, 0, false, false))
        {
            m_pTargetCharAttrs = std::move(pSet);
        }

        m_xDoc->SetTextFormatColl( *m_pPam,
                m_pCSS1Parser->GetTextCollFromPool( RES_POOLCOLL_STANDARD ));
    }

    if( GetMedium() )
    {
        if( !m_bViewCreated )
        {
            m_nEventId = Application::PostUserEvent( LINK( this, SwHTMLParser, AsyncCallback ) );
        }
        else
        {
            m_nEventId = nullptr;
        }
    }
    else // show progress bar
    {
        rInput.Seek(STREAM_SEEK_TO_END);
        rInput.ResetError();

        m_xProgress.reset(new ImportProgress(m_xDoc->GetDocShell(), 0, rInput.Tell()));

        rInput.Seek(STREAM_SEEK_TO_BEGIN);
        rInput.ResetError();
    }

    StartListening(m_xDoc->GetPageDesc( 0 ).GetNotifier());

    SvParserState eRet = HTMLParser::CallParser();
    return eRet;
}

bool SwHTMLParser::CanRemoveNode(SwNodeOffset nNodeIdx) const
{
    const SwNode *pPrev = m_xDoc->GetNodes()[nNodeIdx - 1];
    return pPrev->IsContentNode() || (pPrev->IsEndNode() && pPrev->StartOfSectionNode()->IsSectionNode());
}

void SwHTMLParser::Continue( HtmlTokenId nToken )
{
#ifdef DBG_UTIL
    OSL_ENSURE(!m_nContinue, "Continue in Continue - not supposed to happen");
    m_nContinue++;
#endif

    // When the import (of SFX) is aborted, an error will be set but
    // we still continue, so that we clean up properly.
    OSL_ENSURE( SvParserState::Error!=eState,
            "SwHTMLParser::Continue: already set an error" );
    SwDocShell* pShell = m_xDoc->GetDocShell();
    if( pShell && pShell->IsAbortingImport() )
        eState = SvParserState::Error;

    // Fetch SwViewShell from document, save it and set as current.
    SwViewShell *pInitVSh = CallStartAction();

    if( SvParserState::Error != eState && GetMedium() && !m_bViewCreated )
    {
        // At first call first return, show document and wait for callback
        // time.
        // At this point in CallParser only one digit was read and
        // a SaveState(0) was called.
        eState = SvParserState::Pending;
        m_bViewCreated = true;
        m_xDoc->SetInLoadAsynchron( true );

#ifdef DBG_UTIL
        m_nContinue--;
#endif

        return;
    }

    m_bSetModEnabled = false;
    if( pShell )
    {
        m_bSetModEnabled = pShell->IsEnableSetModified();
        if( m_bSetModEnabled )
        {
            pShell->EnableSetModified( false );
        }
    }

    // during import don't call OLE-Modified
    Link<bool,void> aOLELink( m_xDoc->GetOle2Link() );
    m_xDoc->SetOle2Link( Link<bool,void>() );

    bool bModified = m_xDoc->getIDocumentState().IsModified();
    bool const bWasUndo = m_xDoc->GetIDocumentUndoRedo().DoesUndo();
    m_xDoc->GetIDocumentUndoRedo().DoUndo(false);

    // When the import will be aborted, don't call Continue anymore.
    // If a Pending-Stack exists make sure the stack is ended with a call
    // of NextToken.
    if( SvParserState::Error == eState )
    {
        OSL_ENSURE( m_vPendingStack.empty() || m_vPendingStack.back().nToken != HtmlTokenId::NONE,
                "SwHTMLParser::Continue: Pending-Stack without Token" );
        if( !m_vPendingStack.empty() && m_vPendingStack.back().nToken != HtmlTokenId::NONE )
            NextToken( m_vPendingStack.back().nToken );
        OSL_ENSURE( m_vPendingStack.empty(),
                "SwHTMLParser::Continue: There is again a Pending-Stack" );
    }
    else
    {
        HTMLParser::Continue( !m_vPendingStack.empty() ? m_vPendingStack.back().nToken : nToken );
    }

    // disable progress bar again
    m_xProgress.reset();

    bool bLFStripped = false;
    if( SvParserState::Pending != GetStatus() )
    {
        // set the last attributes yet
        {
            if( !m_aScriptSource.isEmpty() )
            {
                SwScriptFieldType *pType =
                    static_cast<SwScriptFieldType*>(m_xDoc->getIDocumentFieldsAccess().GetSysFieldType( SwFieldIds::Script ));

                SwScriptField aField( pType, m_aScriptType, m_aScriptSource,
                                    false );
                InsertAttr( SwFormatField( aField ), false );
            }

            if( m_pAppletImpl )
            {
                if( m_pAppletImpl->GetApplet().is() )
                    EndApplet();
                else
                    EndObject();
            }

            // maybe remove an existing LF after the last paragraph
            if( IsNewDoc() )
                bLFStripped = StripTrailingLF() > 0;

            // close still open numbering
            while( GetNumInfo().GetNumRule() )
                EndNumberBulletList();

            OSL_ENSURE( !m_nContextStMin, "There are protected contexts" );
            // try this twice, first normally to let m_nContextStMin decrease
            // naturally and get contexts popped in desired order, and if that
            // fails force it
            for (int i = 0; i < 2; ++i)
            {
                while (m_aContexts.size() > m_nContextStMin)
                {
                    std::unique_ptr<HTMLAttrContext> xCntxt(PopContext());
                    if (xCntxt)
                        EndContext(xCntxt.get());
                }
                if (!m_nContextStMin)
                    break;
                OSL_ENSURE(!m_nContextStMin, "There are still protected contexts");
                m_nContextStMin = 0;
            }

            m_aParaAttrs.clear();

            SetAttr( false );

            // set the first delayed styles
            m_pCSS1Parser->SetDelayedStyles();
        }

        // again correct the start
        if( !IsNewDoc() && m_pSttNdIdx->GetIndex() )
        {
            SwTextNode* pTextNode = m_pSttNdIdx->GetNode().GetTextNode();
            SwNodeIndex aNxtIdx( *m_pSttNdIdx );
            if( pTextNode && pTextNode->CanJoinNext( &aNxtIdx ))
            {
                const sal_Int32 nStt = pTextNode->GetText().getLength();
                // when the cursor is still in the node, then set him at the end
                if( m_pPam->GetPoint()->GetNode() == aNxtIdx.GetNode() )
                {
                    m_pPam->GetPoint()->Assign( *pTextNode, nStt );
                }

#if OSL_DEBUG_LEVEL > 0
// !!! shouldn't be possible, or ??
                OSL_ENSURE( m_pSttNdIdx->GetIndex()+1 != m_pPam->GetBound().GetNodeIndex(),
                    "Pam.Bound1 is still in the node" );
                OSL_ENSURE( m_pSttNdIdx->GetIndex()+1 != m_pPam->GetBound( false ).GetNodeIndex(),
                    "Pam.Bound2 is still in the node" );

                if( m_pSttNdIdx->GetIndex()+1 == m_pPam->GetBound().GetNodeIndex() )
                {
                    const sal_Int32 nCntPos = m_pPam->GetBound().GetContentIndex();
                    m_pPam->GetBound().SetContent(
                                    pTextNode->GetText().getLength() + nCntPos );
                }
                if( m_pSttNdIdx->GetIndex()+1 == m_pPam->GetBound( false ).GetNodeIndex() )
                {
                    const sal_Int32 nCntPos = m_pPam->GetBound( false ).GetContentIndex();
                    m_pPam->GetBound( false ).SetContent(
                                    pTextNode->GetText().getLength() + nCntPos );
                }
#endif
                // Keep character attribute!
                SwTextNode* pDelNd = aNxtIdx.GetNode().GetTextNode();
                if (pTextNode->GetText().getLength())
                    pDelNd->FormatToTextAttr( pTextNode );
                else
                    pTextNode->ChgFormatColl( pDelNd->GetTextColl() );
                pTextNode->JoinNext();
            }
        }
    }

    if( SvParserState::Accepted == eState )
    {
        if( m_nMissingImgMaps )
        {
            // Some Image-Map relations are still missing.
            // Maybe now the Image-Maps are there?
            ConnectImageMaps();
        }

        // now remove the last useless paragraph
        SwPosition* pPos = m_pPam->GetPoint();
        if( !pPos->GetContentIndex() && !bLFStripped )
        {
            SwTextNode* pCurrentNd;
            SwNodeOffset nNodeIdx = pPos->GetNodeIndex();

            bool bHasFlysOrMarks =
                HasCurrentParaFlys() || HasCurrentParaBookmarks( true );

            if( IsNewDoc() )
            {
                if (!m_pPam->GetPoint()->GetContentIndex() && CanRemoveNode(nNodeIdx))
                {
                    SwContentNode* pCNd = m_pPam->GetPointContentNode();
                    if( pCNd && pCNd->StartOfSectionIndex()+2 <
                        pCNd->EndOfSectionIndex() && !bHasFlysOrMarks )
                    {
                        SwViewShell *pVSh = CheckActionViewShell();
                        SwCursorShell *pCursorSh = dynamic_cast<SwCursorShell *>( pVSh );
                        if( pCursorSh &&
                            pCursorSh->GetCursor()->GetPoint()
                                   ->GetNodeIndex() == nNodeIdx )
                        {
                            pCursorSh->MovePara(GoPrevPara, fnParaEnd );
                            pCursorSh->SetMark();
                            pCursorSh->ClearMark();
                        }
                        SwNode& rDelNode = m_pPam->GetPoint()->GetNode();
                        // move so we don't have a dangling SwContentIndex to the deleted node
                        m_pPam->GetPoint()->Adjust(SwNodeOffset(1));
                        if (m_pPam->HasMark())
                            m_pPam->GetMark()->Adjust(SwNodeOffset(1));
                        m_xDoc->GetNodes().Delete( rDelNode );
                    }
                }
            }
            else if( nullptr != ( pCurrentNd = m_xDoc->GetNodes()[ nNodeIdx ]->GetTextNode()) && !bHasFlysOrMarks )
            {
                if( pCurrentNd->CanJoinNext( pPos ))
                {
                    SwTextNode* pNextNd = pPos->GetNode().GetTextNode();
                    m_pPam->SetMark(); m_pPam->DeleteMark();
                    pNextNd->JoinPrev();
                }
                else if (pCurrentNd->GetText().isEmpty())
                {
                    m_pPam->SetMark(); m_pPam->DeleteMark();
                    SwNode& rDelNode = pPos->GetNode();
                    // move so we don't have a dangling SwContentIndex to the deleted node
                    m_pPam->GetPoint()->Adjust(SwNodeOffset(+1));
                    m_xDoc->GetNodes().Delete( rDelNode );
                    m_pPam->Move( fnMoveBackward );
                }
            }
        }

        // annul the SplitNode from the beginning
        else if( !IsNewDoc() )
        {
            if( pPos->GetContentIndex() )                 // then there was no <p> at the end
                m_pPam->Move( fnMoveForward, GoInNode );    // therefore to the next
            SwTextNode* pTextNode = pPos->GetNode().GetTextNode();
            SwNodeIndex aPrvIdx( pPos->GetNode() );
            if( pTextNode && pTextNode->CanJoinPrev( &aPrvIdx ) &&
                *m_pSttNdIdx <= aPrvIdx )
            {
                // Normally here should take place a JoinNext, but all cursors and
                // so are registered in pTextNode, so that it MUST remain.

                // Convert paragraph to character attribute, from Prev adopt
                // the paragraph attribute and the template!
                SwTextNode* pPrev = aPrvIdx.GetNode().GetTextNode();
                pTextNode->ChgFormatColl( pPrev->GetTextColl() );
                pTextNode->FormatToTextAttr( pPrev );
                pTextNode->ResetAllAttr();

                if( pPrev->HasSwAttrSet() )
                    pTextNode->SetAttr( *pPrev->GetpSwAttrSet() );

                if( &m_pPam->GetBound().GetNode() == pPrev )
                    m_pPam->GetBound().nContent.Assign( pTextNode, 0 );
                if( &m_pPam->GetBound(false).GetNode() == pPrev )
                    m_pPam->GetBound(false).nContent.Assign( pTextNode, 0 );

                pTextNode->JoinPrev();
            }
        }

        // adjust AutoLoad in DocumentProperties
        if (!bFuzzing && IsNewDoc())
        {
            SwDocShell *pDocShell(m_xDoc->GetDocShell());
            OSL_ENSURE(pDocShell, "no SwDocShell");
            if (pDocShell) {
                uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
                    pDocShell->GetModel(), uno::UNO_QUERY_THROW);
                uno::Reference<document::XDocumentProperties> xDocProps(
                    xDPS->getDocumentProperties());
                OSL_ENSURE(xDocProps.is(), "DocumentProperties is null");
                if ( xDocProps.is() && (xDocProps->getAutoloadSecs() > 0) &&
                     (xDocProps->getAutoloadURL().isEmpty()) )
                {
                    xDocProps->setAutoloadURL(m_aPathToFile);
                }
            }
        }

        if( m_bUpdateDocStat )
        {
            m_xDoc->getIDocumentStatistics().UpdateDocStat( false, true );
        }
    }

    if( SvParserState::Pending != GetStatus() )
    {
        delete m_pSttNdIdx;
        m_pSttNdIdx = nullptr;
    }

    // should the parser be the last one who hold the document, then nothing
    // has to be done anymore, document will be destroyed shortly!
    if( 1 < m_xDoc->getReferenceCount() )
    {
        if( bWasUndo )
        {
            m_xDoc->GetIDocumentUndoRedo().DelAllUndoObj();
            m_xDoc->GetIDocumentUndoRedo().DoUndo(true);
        }
        else if( !pInitVSh )
        {
            // When at the beginning of Continue no Shell was available,
            // it's possible in the meantime one was created.
            // In that case the bWasUndo flag is wrong and we must
            // enable Undo.
            SwViewShell *pTmpVSh = CheckActionViewShell();
            if( pTmpVSh )
            {
                m_xDoc->GetIDocumentUndoRedo().DoUndo(true);
            }
        }

        m_xDoc->SetOle2Link( aOLELink );
        if( !bModified )
            m_xDoc->getIDocumentState().ResetModified();
        if( m_bSetModEnabled && pShell )
        {
            pShell->EnableSetModified();
            m_bSetModEnabled = false; // this is unnecessary here
        }
    }

    // When the Document-SwVievShell still exists and an Action is open
    // (doesn't have to be by abort), end the Action, disconnect from Shell
    // and finally reconstruct the old Shell.
    CallEndAction( true );

#ifdef DBG_UTIL
    m_nContinue--;
#endif
}

void SwHTMLParser::Notify(const SfxHint& rHint)
{
    if(rHint.GetId() == SfxHintId::Dying)
    {
        EndListeningAll();
        ReleaseRef();
    }
}

void SwHTMLParser::DocumentDetected()
{
    OSL_ENSURE( !m_bDocInitialized, "DocumentDetected called multiple times" );
    m_bDocInitialized = true;
    if( IsNewDoc() )
    {
        if( IsInHeader() )
            FinishHeader();

        CallEndAction( true );

        m_xDoc->GetIDocumentUndoRedo().DoUndo(false);
        // For DocumentDetected in general a SwViewShell is created.
        // But it also can be created later, in case the UI is captured.
        CallStartAction();
    }
}

// is called for every token that is recognised in CallParser
void SwHTMLParser::NextToken( HtmlTokenId nToken )
{
    SwDocShell* pShell = m_xDoc->GetDocShell();
    if( ( pShell && pShell->IsAbortingImport() )
        || 1 == m_xDoc->getReferenceCount() )
    {
        // Was the import cancelled by SFX? If a pending stack
        // exists, clean it.
        eState = SvParserState::Error;
        OSL_ENSURE( m_vPendingStack.empty() || m_vPendingStack.back().nToken != HtmlTokenId::NONE,
                "SwHTMLParser::NextToken: Pending-Stack without token" );
        if( 1 == m_xDoc->getReferenceCount() || m_vPendingStack.empty() )
            return ;
    }

#if OSL_DEBUG_LEVEL > 0
    if( !m_vPendingStack.empty() )
    {
        switch( nToken )
        {
            // tables are read by recursive method calls
        case HtmlTokenId::TABLE_ON:
            // For CSS declarations we might have to wait
            // for a file download to finish
        case HtmlTokenId::LINK:
            // For controls we might have to set the size.
        case HtmlTokenId::INPUT:
        case HtmlTokenId::TEXTAREA_ON:
        case HtmlTokenId::SELECT_ON:
        case HtmlTokenId::SELECT_OFF:
            break;
        default:
            OSL_ENSURE( m_vPendingStack.empty(), "Unknown token for Pending-Stack" );
            break;
        }
    }
#endif

    // The following special cases have to be treated before the
    // filter detection, because Netscape doesn't reference the content
    // of the title for filter detection either.
    if( m_vPendingStack.empty() )
    {
        if( m_bInTitle )
        {
            switch( nToken )
            {
            case HtmlTokenId::TITLE_OFF:
            {
                OUString sTitle = m_sTitle.makeStringAndClear();
                if( IsNewDoc() && !sTitle.isEmpty() )
                {
                    if (pShell)
                    {
                        uno::Reference<document::XDocumentPropertiesSupplier>
                            xDPS(pShell->GetModel(),
                            uno::UNO_QUERY_THROW);
                        uno::Reference<document::XDocumentProperties> xDocProps(
                            xDPS->getDocumentProperties());
                        OSL_ENSURE(xDocProps.is(), "no DocumentProperties");
                        if (xDocProps.is()) {
                            xDocProps->setTitle(sTitle);
                        }

                        pShell->SetTitle(sTitle);
                    }
                }
                m_bInTitle = false;
                break;
            }

            case HtmlTokenId::NONBREAKSPACE:
                m_sTitle.append(" ");
                break;

            case HtmlTokenId::SOFTHYPH:
                m_sTitle.append("-");
                break;

            case HtmlTokenId::TEXTTOKEN:
                m_sTitle.append(aToken);
                break;

            default:
                m_sTitle.append("<");
                if( (nToken >= HtmlTokenId::ONOFF_START) && isOffToken(nToken) )
                    m_sTitle.append("/");
                m_sTitle.append(sSaveToken);
                if( !aToken.isEmpty() )
                {
                    m_sTitle.append(" ");
                    m_sTitle.append(aToken);
                }
                m_sTitle.append(">");
                break;
            }

            return;
        }
    }

    // Find out what type of document it is if we don't know already.
    // For Controls this has to be finished before the control is inserted
    // because for inserting a View is needed.
    if( !m_bDocInitialized )
        DocumentDetected();

    bool bGetIDOption = false, bInsertUnknown = false;
    bool bUpperSpaceSave = m_bUpperSpace;
    m_bUpperSpace = false;

    // The following special cases may or have to be treated after the
    // filter detection
    if( m_vPendingStack.empty() )
    {
        if( m_bInFloatingFrame )
        {
            // <SCRIPT> is ignored here (from us), because it is ignored in
            // Applets as well
            if( HtmlTokenId::IFRAME_OFF == nToken )
            {
                m_bCallNextToken = false;
                m_bInFloatingFrame = false;
            }

            return;
        }
        else if( m_bInNoEmbed )
        {
            switch( nToken )
            {
            case HtmlTokenId::NOEMBED_OFF:
                m_aContents = convertLineEnd(m_aContents, GetSystemLineEnd());
                InsertComment( m_aContents, OOO_STRING_SVTOOLS_HTML_noembed );
                m_aContents.clear();
                m_bCallNextToken = false;
                m_bInNoEmbed = false;
                break;

            case HtmlTokenId::RAWDATA:
                InsertCommentText( OOO_STRING_SVTOOLS_HTML_noembed );
                break;

            default:
                OSL_ENSURE( false, "SwHTMLParser::NextToken: invalid tag" );
                break;
            }

            return;
        }
        else if( m_pAppletImpl )
        {
            // in an applet only <PARAM> tags and the </APPLET> tag
            // are of interest for us (for the moment)
            // <SCRIPT> is ignored here (from Netscape)!

            switch( nToken )
            {
            case HtmlTokenId::APPLET_OFF:
                m_bCallNextToken = false;
                EndApplet();
                break;
            case HtmlTokenId::OBJECT_OFF:
                m_bCallNextToken = false;
                EndObject();
                break;
            case HtmlTokenId::PARAM:
                InsertParam();
                break;
            default: break;
            }

            return;
        }
        else if( m_bTextArea )
        {
            // in a TextArea everything up to </TEXTAREA> is inserted as text.
            // <SCRIPT> is ignored here (from Netscape)!

            switch( nToken )
            {
            case HtmlTokenId::TEXTAREA_OFF:
                m_bCallNextToken = false;
                EndTextArea();
                break;

            default:
                InsertTextAreaText( nToken );
                break;
            }

            return;
        }
        else if( m_bSelect )
        {
            // HAS to be treated after bNoScript!
            switch( nToken )
            {
            case HtmlTokenId::SELECT_OFF:
                m_bCallNextToken = false;
                EndSelect();
                return;

            case HtmlTokenId::OPTION:
                InsertSelectOption();
                return;

            case HtmlTokenId::TEXTTOKEN:
                InsertSelectText();
                return;

            case HtmlTokenId::INPUT:
            case HtmlTokenId::SCRIPT_ON:
            case HtmlTokenId::SCRIPT_OFF:
            case HtmlTokenId::NOSCRIPT_ON:
            case HtmlTokenId::NOSCRIPT_OFF:
            case HtmlTokenId::RAWDATA:
                // treat in normal switch
                break;

            default:
                // ignore
                return;
            }
        }
        else if( m_pMarquee )
        {
            // in a TextArea everything up to </TEXTAREA> is inserted as text.
            // The <SCRIPT> tags are ignored from MS-IE, we ignore the whole
            // script.
            switch( nToken )
            {
            case HtmlTokenId::MARQUEE_OFF:
                m_bCallNextToken = false;
                EndMarquee();
                break;

            case HtmlTokenId::TEXTTOKEN:
                InsertMarqueeText();
                break;
            default: break;
            }

            return;
        }
        else if( m_bInField )
        {
            switch( nToken )
            {
            case HtmlTokenId::SDFIELD_OFF:
                m_bCallNextToken = false;
                EndField();
                break;

            case HtmlTokenId::TEXTTOKEN:
                InsertFieldText();
                break;
            default: break;
            }

            return;
        }
        else if( m_bInFootEndNoteAnchor || m_bInFootEndNoteSymbol )
        {
            switch( nToken )
            {
            case HtmlTokenId::ANCHOR_OFF:
                EndAnchor();
                m_bCallNextToken = false;
                break;

            case HtmlTokenId::TEXTTOKEN:
                InsertFootEndNoteText();
                break;
            default: break;
            }
            return;
        }
        else if( !m_aUnknownToken.isEmpty() )
        {
            // Paste content of unknown tags.
            // (but surely if we are not in the header section) fdo#36080 fdo#34666
            if (!aToken.isEmpty() && !IsInHeader() )
            {
                if( !m_bDocInitialized )
                    DocumentDetected();
                m_xDoc->getIDocumentContentOperations().InsertString( *m_pPam, aToken.toString());

                // if there are temporary paragraph attributes and the
                // paragraph isn't empty then the paragraph attributes
                // are final.
                m_aParaAttrs.clear();

                SetAttr();
            }

            // Unknown token in the header are only closed by a matching
            // end-token, </HEAD> or <BODY>. Text inside is ignored.
            switch( nToken )
            {
            case HtmlTokenId::UNKNOWNCONTROL_OFF:
                if( m_aUnknownToken != sSaveToken )
                    return;
                [[fallthrough]];
            case HtmlTokenId::FRAMESET_ON:
            case HtmlTokenId::HEAD_OFF:
            case HtmlTokenId::BODY_ON:
            case HtmlTokenId::IMAGE:        // Don't know why Netscape acts this way.
                m_aUnknownToken.clear();
                break;
            case HtmlTokenId::TEXTTOKEN:
                return;
            default:
                m_aUnknownToken.clear();
                break;
            }
        }
    }

    switch( nToken )
    {
    case HtmlTokenId::BODY_ON:
        if (!m_bBodySeen)
        {
            m_bBodySeen = true;
            if( !m_aStyleSource.isEmpty() )
            {
                m_pCSS1Parser->ParseStyleSheet( m_aStyleSource );
                m_aStyleSource.clear();
            }
            if( IsNewDoc() )
            {
                InsertBodyOptions();
                // If there is a template for the first or the right page,
                // it is set here.
                const SwPageDesc *pPageDesc = nullptr;
                if( m_pCSS1Parser->IsSetFirstPageDesc() )
                    pPageDesc = m_pCSS1Parser->GetFirstPageDesc();
                else if( m_pCSS1Parser->IsSetRightPageDesc() )
                    pPageDesc = m_pCSS1Parser->GetRightPageDesc();

                if( pPageDesc )
                {
                    m_xDoc->getIDocumentContentOperations().InsertPoolItem( *m_pPam, SwFormatPageDesc( pPageDesc ) );
                }
            }
        }
        break;

    case HtmlTokenId::LINK:
        InsertLink();
        break;

    case HtmlTokenId::BASE:
        {
            const HTMLOptions& rHTMLOptions = GetOptions();
            for (size_t i = rHTMLOptions.size(); i; )
            {
                const HTMLOption& rOption = rHTMLOptions[--i];
                switch( rOption.GetToken() )
                {
                case HtmlOptionId::HREF:
                    m_sBaseURL = rOption.GetString();
                    break;
                case HtmlOptionId::TARGET:
                    if( IsNewDoc() )
                    {
                        SwDocShell *pDocShell(m_xDoc->GetDocShell());
                        OSL_ENSURE(pDocShell, "no SwDocShell");
                        if (pDocShell) {
                            uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
                                pDocShell->GetModel(), uno::UNO_QUERY_THROW);
                            uno::Reference<document::XDocumentProperties>
                                xDocProps(xDPS->getDocumentProperties());
                            OSL_ENSURE(xDocProps.is(),"no DocumentProperties");
                            if (xDocProps.is()) {
                                xDocProps->setDefaultTarget(
                                    rOption.GetString());
                            }
                        }
                    }
                    break;
                default: break;
                }
            }
        }
        break;

    case HtmlTokenId::META:
        {
            SvKeyValueIterator *pHTTPHeader = nullptr;
            if( IsNewDoc() )
            {
                SwDocShell *pDocSh = m_xDoc->GetDocShell();
                if( pDocSh )
                    pHTTPHeader = pDocSh->GetHeaderAttributes();
            }
            SwDocShell *pDocShell(m_xDoc->GetDocShell());
            OSL_ENSURE(pDocShell, "no SwDocShell");
            if (pDocShell)
            {
                uno::Reference<document::XDocumentProperties> xDocProps;
                if (IsNewDoc())
                {
                    const uno::Reference<document::XDocumentPropertiesSupplier>
                        xDPS( pDocShell->GetModel(), uno::UNO_QUERY_THROW );
                    xDocProps = xDPS->getDocumentProperties();
                    OSL_ENSURE(xDocProps.is(), "DocumentProperties is null");
                }
                ParseMetaOptions( xDocProps, pHTTPHeader );
            }
        }
        break;

    case HtmlTokenId::TITLE_ON:
        m_bInTitle = true;
        break;

    case HtmlTokenId::SCRIPT_ON:
        NewScript();
        break;

    case HtmlTokenId::SCRIPT_OFF:
        EndScript();
        break;

    case HtmlTokenId::NOSCRIPT_ON:
    case HtmlTokenId::NOSCRIPT_OFF:
        bInsertUnknown = true;
        break;

    case HtmlTokenId::STYLE_ON:
        NewStyle();
        break;

    case HtmlTokenId::STYLE_OFF:
        EndStyle();
        break;

    case HtmlTokenId::RAWDATA:
        if( !m_bIgnoreRawData )
        {
            if( IsReadScript() )
            {
                AddScriptSource();
            }
            else if( IsReadStyle() )
            {
                if( !m_aStyleSource.isEmpty() )
                    m_aStyleSource += "\n";
                m_aStyleSource += aToken;
            }
        }
        break;

    case HtmlTokenId::OBJECT_ON:
        if (m_bXHTML)
        {
            if (!InsertEmbed())
                InsertImage();
            break;
        }
#if HAVE_FEATURE_JAVA
        NewObject();
        m_bCallNextToken = m_pAppletImpl!=nullptr && m_xTable;
#endif
        break;

    case HtmlTokenId::OBJECT_OFF:
        if (!m_aEmbeds.empty())
            m_aEmbeds.pop();
        break;

    case HtmlTokenId::APPLET_ON:
#if HAVE_FEATURE_JAVA
        InsertApplet();
        m_bCallNextToken = m_pAppletImpl!=nullptr && m_xTable;
#endif
        break;

    case HtmlTokenId::IFRAME_ON:
        if (bFuzzing && m_nFloatingFrames > 64)
            SAL_WARN("sw.html", "Not importing any more FloatingFrames for fuzzing performance");
        else
        {
            InsertFloatingFrame();
            m_bCallNextToken = m_bInFloatingFrame && m_xTable;
        }
        break;

    case HtmlTokenId::LINEBREAK:
        if( !IsReadPRE() )
        {
            InsertLineBreak();
            break;
        }
        else
            bGetIDOption = true;
            // <BR>s in <PRE> resemble true LFs, hence no break
        [[fallthrough]];

    case HtmlTokenId::NEWPARA:
        // CR in PRE/LISTING/XMP
        {
            if( HtmlTokenId::NEWPARA==nToken ||
                m_pPam->GetPoint()->GetContentIndex() )
            {
                AppendTextNode(); // there is no LF at this place
                                 // therefore it will cause no problems
                SetTextCollAttrs();
            }
            // progress bar
            if (m_xProgress)
                m_xProgress->Update(rInput.Tell());
        }
        break;

    case HtmlTokenId::NONBREAKSPACE:
        m_xDoc->getIDocumentContentOperations().InsertString( *m_pPam, OUString(CHAR_HARDBLANK) );
        break;

    case HtmlTokenId::SOFTHYPH:
        m_xDoc->getIDocumentContentOperations().InsertString( *m_pPam, OUString(CHAR_SOFTHYPHEN) );
        break;

    case HtmlTokenId::LINEFEEDCHAR:
        if( m_pPam->GetPoint()->GetContentIndex() )
            AppendTextNode();
        if (!m_xTable && !m_xDoc->IsInHeaderFooter(m_pPam->GetPoint()->GetNode()))
        {
            NewAttr(m_xAttrTab, &m_xAttrTab->pBreak, SvxFormatBreakItem(SvxBreak::PageBefore, RES_BREAK));
            EndAttr( m_xAttrTab->pBreak, false );
        }
        break;

    case HtmlTokenId::TEXTTOKEN:
    case HtmlTokenId::CDATA:
        // insert string without spanning attributes at the end.
        if (!aToken.isEmpty() && ' ' == aToken[0] && !IsReadPRE() && !GetPreserveSpaces())
        {
            sal_Int32 nPos = m_pPam->GetPoint()->GetContentIndex();
            const SwTextNode* pTextNode = nPos ? m_pPam->GetPoint()->GetNode().GetTextNode() : nullptr;
            if (pTextNode)
            {
                const OUString& rText = pTextNode->GetText();
                sal_Unicode cLast = rText[--nPos];
                if( ' ' == cLast || '\x0a' == cLast)
                    aToken.remove(0, 1);
            }
            else
                aToken.remove(0, 1);

            if( aToken.isEmpty() )
            {
                m_bUpperSpace = bUpperSpaceSave;
                break;
            }
        }

        if( !aToken.isEmpty() )
        {
            if( !m_bDocInitialized )
                DocumentDetected();

            if (!m_aEmbeds.empty())
            {
                // The text token is inside an OLE object, which means
                // alternate text.
                SwOLENode* pOLENode = m_aEmbeds.top();
                if (!pOLENode)
                {
                    // <object> is mapped to an image -> ignore.
                    break;
                }

                if (SwFlyFrameFormat* pFormat
                    = dynamic_cast<SwFlyFrameFormat*>(pOLENode->GetFlyFormat()))
                {
                    if (SdrObject* pObject = SwXFrame::GetOrCreateSdrObject(*pFormat))
                    {
                        pObject->SetTitle(pObject->GetTitle() + aToken);
                        break;
                    }
                }
            }

            m_xDoc->getIDocumentContentOperations().InsertString( *m_pPam, aToken.toString());

            // if there are temporary paragraph attributes and the
            // paragraph isn't empty then the paragraph attributes
            // are final.
            m_aParaAttrs.clear();

            SetAttr();
        }
        break;

    case HtmlTokenId::HORZRULE:
        InsertHorzRule();
        break;

    case HtmlTokenId::IMAGE:
        InsertImage();
        // if only the parser references the doc, we can break and set
        // an error code
        if( 1 == m_xDoc->getReferenceCount() )
        {
            eState = SvParserState::Error;
        }
        break;

    case HtmlTokenId::SPACER:
        InsertSpacer();
        break;

    case HtmlTokenId::EMBED:
        InsertEmbed();
        break;

    case HtmlTokenId::NOEMBED_ON:
        m_bInNoEmbed = true;
        m_bCallNextToken = bool(m_xTable);
        ReadRawData( u"" OOO_STRING_SVTOOLS_HTML_noembed ""_ustr );
        break;

    case HtmlTokenId::DEFLIST_ON:
        if( m_nOpenParaToken != HtmlTokenId::NONE )
            EndPara();
        NewDefList();
        break;
    case HtmlTokenId::DEFLIST_OFF:
        if( m_nOpenParaToken != HtmlTokenId::NONE )
            EndPara();
        EndDefListItem( HtmlTokenId::NONE );
        EndDefList();
        break;

    case HtmlTokenId::DD_ON:
    case HtmlTokenId::DT_ON:
        if( m_nOpenParaToken != HtmlTokenId::NONE )
            EndPara();
        EndDefListItem();// close <DD>/<DT> and set no template
        NewDefListItem( nToken );
        break;

    case HtmlTokenId::DD_OFF:
    case HtmlTokenId::DT_OFF:
        // c.f. HtmlTokenId::LI_OFF
        // Actually we should close a DD/DT now.
        // But neither Netscape nor Microsoft do this and so don't we.
        EndDefListItem( nToken );
        break;

    // divisions
    case HtmlTokenId::DIVISION_ON:
    case HtmlTokenId::CENTER_ON:
        if (!m_isInTableStructure)
        {
            if (m_nOpenParaToken != HtmlTokenId::NONE)
            {
                if (IsReadPRE())
                    m_nOpenParaToken = HtmlTokenId::NONE;
                else
                    EndPara();
            }
            NewDivision( nToken );
        }
        break;

    case HtmlTokenId::DIVISION_OFF:
    case HtmlTokenId::CENTER_OFF:
        if (!m_isInTableStructure)
        {
            if (m_nOpenParaToken != HtmlTokenId::NONE)
            {
                if (IsReadPRE())
                    m_nOpenParaToken = HtmlTokenId::NONE;
                else
                    EndPara();
            }
            EndDivision();
        }
        break;

    case HtmlTokenId::MULTICOL_ON:
        if( m_nOpenParaToken != HtmlTokenId::NONE )
            EndPara();
        NewMultiCol();
        break;

    case HtmlTokenId::MULTICOL_OFF:
        if( m_nOpenParaToken != HtmlTokenId::NONE )
            EndPara();
        EndTag( HtmlTokenId::MULTICOL_ON );
        break;

    case HtmlTokenId::MARQUEE_ON:
        NewMarquee();
        m_bCallNextToken = m_pMarquee!=nullptr && m_xTable;
        break;

    case HtmlTokenId::FORM_ON:
        NewForm();
        break;
    case HtmlTokenId::FORM_OFF:
        EndForm();
        break;

    // templates
    case HtmlTokenId::PARABREAK_ON:
        if( m_nOpenParaToken != HtmlTokenId::NONE )
            EndPara( true );
        NewPara();
        break;

    case HtmlTokenId::PARABREAK_OFF:
        EndPara( true );
        break;

    case HtmlTokenId::ADDRESS_ON:
        if( m_nOpenParaToken != HtmlTokenId::NONE )
            EndPara();
        NewTextFormatColl(HtmlTokenId::ADDRESS_ON, RES_POOLCOLL_SEND_ADDRESS);
        break;

    case HtmlTokenId::ADDRESS_OFF:
        if( m_nOpenParaToken != HtmlTokenId::NONE )
            EndPara();
        EndTextFormatColl( HtmlTokenId::ADDRESS_OFF );
        break;

    case HtmlTokenId::BLOCKQUOTE_ON:
    case HtmlTokenId::BLOCKQUOTE30_ON:
        if( m_nOpenParaToken != HtmlTokenId::NONE )
            EndPara();
        NewTextFormatColl( HtmlTokenId::BLOCKQUOTE_ON, RES_POOLCOLL_HTML_BLOCKQUOTE );
        break;

    case HtmlTokenId::BLOCKQUOTE_OFF:
    case HtmlTokenId::BLOCKQUOTE30_OFF:
        if( m_nOpenParaToken != HtmlTokenId::NONE )
            EndPara();
        EndTextFormatColl( HtmlTokenId::BLOCKQUOTE_ON );
        break;

    case HtmlTokenId::PREFORMTXT_ON:
    case HtmlTokenId::LISTING_ON:
    case HtmlTokenId::XMP_ON:
        if( m_nOpenParaToken != HtmlTokenId::NONE )
            EndPara();
        NewTextFormatColl( nToken, RES_POOLCOLL_HTML_PRE );
        break;

    case HtmlTokenId::PREFORMTXT_OFF:
        m_bNoParSpace = true; // the last PRE-paragraph gets a spacing
        EndTextFormatColl( HtmlTokenId::PREFORMTXT_OFF );
        break;

    case HtmlTokenId::LISTING_OFF:
    case HtmlTokenId::XMP_OFF:
        EndTextFormatColl( nToken );
        break;

    case HtmlTokenId::HEAD1_ON:
    case HtmlTokenId::HEAD2_ON:
    case HtmlTokenId::HEAD3_ON:
    case HtmlTokenId::HEAD4_ON:
    case HtmlTokenId::HEAD5_ON:
    case HtmlTokenId::HEAD6_ON:
        if( m_nOpenParaToken != HtmlTokenId::NONE )
        {
            if( IsReadPRE() )
                m_nOpenParaToken = HtmlTokenId::NONE;
            else
                EndPara();
        }
        NewHeading( nToken );
        break;

    case HtmlTokenId::HEAD1_OFF:
    case HtmlTokenId::HEAD2_OFF:
    case HtmlTokenId::HEAD3_OFF:
    case HtmlTokenId::HEAD4_OFF:
    case HtmlTokenId::HEAD5_OFF:
    case HtmlTokenId::HEAD6_OFF:
        EndHeading();
        break;

    case HtmlTokenId::TABLE_ON:
        if( !m_vPendingStack.empty() )
            BuildTable( SvxAdjust::End );
        else
        {
            if( m_nOpenParaToken != HtmlTokenId::NONE )
                EndPara();
            OSL_ENSURE(!m_xTable, "table in table not allowed here");
            if( !m_xTable && (IsNewDoc() || !m_pPam->GetPointNode().FindTableNode()) &&
                (m_pPam->GetPoint()->GetNodeIndex() >
                            m_xDoc->GetNodes().GetEndOfExtras().GetIndex() ||
                !m_pPam->GetPointNode().FindFootnoteStartNode() ) )
            {
                if ( m_nParaCnt < 5 )
                    Show();     // show what we have up to here

                SvxAdjust eAdjust = m_xAttrTab->pAdjust
                    ? static_cast<const SvxAdjustItem&>(m_xAttrTab->pAdjust->GetItem()).
                                             GetAdjust()
                    : SvxAdjust::End;
                BuildTable( eAdjust );
            }
            else
                bInsertUnknown = m_bKeepUnknown;
        }
        break;

    // lists
    case HtmlTokenId::DIRLIST_ON:
    case HtmlTokenId::MENULIST_ON:
    case HtmlTokenId::ORDERLIST_ON:
    case HtmlTokenId::UNORDERLIST_ON:
        if( m_nOpenParaToken != HtmlTokenId::NONE )
            EndPara();
        NewNumberBulletList( nToken );
        break;

    case HtmlTokenId::DIRLIST_OFF:
    case HtmlTokenId::MENULIST_OFF:
    case HtmlTokenId::ORDERLIST_OFF:
    case HtmlTokenId::UNORDERLIST_OFF:
        if( m_nOpenParaToken != HtmlTokenId::NONE )
            EndPara();
        EndNumberBulletListItem( HtmlTokenId::NONE, true );
        EndNumberBulletList( nToken );
        break;

    case HtmlTokenId::LI_ON:
    case HtmlTokenId::LISTHEADER_ON:
        if( m_nOpenParaToken != HtmlTokenId::NONE &&
            (m_pPam->GetPoint()->GetContentIndex()
            || HtmlTokenId::PARABREAK_ON==m_nOpenParaToken) )
        {
            // only finish paragraph for <P><LI>, not for <DD><LI>
            EndPara();
        }

        if (bFuzzing && m_nListItems > 1024)
        {
            SAL_WARN("sw.html", "skipping remaining bullet import for performance during fuzzing");
        }
        else
        {
            EndNumberBulletListItem( HtmlTokenId::NONE, false );// close <LI>/<LH> and don't set a template
            NewNumberBulletListItem( nToken );
        }

        ++m_nListItems;

        break;
    case HtmlTokenId::LI_OFF:
    case HtmlTokenId::LISTHEADER_OFF:
        EndNumberBulletListItem( nToken, false );
        break;

    // Attribute :
    case HtmlTokenId::ITALIC_ON:
        {
            SvxPostureItem aPosture( ITALIC_NORMAL, RES_CHRATR_POSTURE );
            SvxPostureItem aPostureCJK( ITALIC_NORMAL, RES_CHRATR_CJK_POSTURE );
            SvxPostureItem aPostureCTL( ITALIC_NORMAL, RES_CHRATR_CTL_POSTURE );
            NewStdAttr( HtmlTokenId::ITALIC_ON,
                           &m_xAttrTab->pItalic, aPosture,
                           &m_xAttrTab->pItalicCJK, &aPostureCJK,
                           &m_xAttrTab->pItalicCTL, &aPostureCTL );
        }
        break;

    case HtmlTokenId::BOLD_ON:
        {
            SvxWeightItem aWeight( WEIGHT_BOLD, RES_CHRATR_WEIGHT );
            SvxWeightItem aWeightCJK( WEIGHT_BOLD, RES_CHRATR_CJK_WEIGHT );
            SvxWeightItem aWeightCTL( WEIGHT_BOLD, RES_CHRATR_CTL_WEIGHT );
            NewStdAttr( HtmlTokenId::BOLD_ON,
                        &m_xAttrTab->pBold, aWeight,
                        &m_xAttrTab->pBoldCJK, &aWeightCJK,
                        &m_xAttrTab->pBoldCTL, &aWeightCTL );
        }
        break;

    case HtmlTokenId::STRIKE_ON:
    case HtmlTokenId::STRIKETHROUGH_ON:
    case HtmlTokenId::DELETEDTEXT_ON:
        {
            NewStdAttr( HtmlTokenId::STRIKE_ON, &m_xAttrTab->pStrike,
                        SvxCrossedOutItem(STRIKEOUT_SINGLE, RES_CHRATR_CROSSEDOUT) );
        }
        break;

    case HtmlTokenId::UNDERLINE_ON:
    case HtmlTokenId::INSERTEDTEXT_ON:
        {
            NewStdAttr( HtmlTokenId::UNDERLINE_ON, &m_xAttrTab->pUnderline,
                        SvxUnderlineItem(LINESTYLE_SINGLE, RES_CHRATR_UNDERLINE) );
        }
        break;

    case HtmlTokenId::SUPERSCRIPT_ON:
        {
            NewStdAttr( HtmlTokenId::SUPERSCRIPT_ON, &m_xAttrTab->pEscapement,
                        SvxEscapementItem(HTML_ESC_SUPER,HTML_ESC_PROP, RES_CHRATR_ESCAPEMENT) );
        }
        break;

    case HtmlTokenId::SUBSCRIPT_ON:
        {
            NewStdAttr( HtmlTokenId::SUBSCRIPT_ON, &m_xAttrTab->pEscapement,
                        SvxEscapementItem(HTML_ESC_SUB,HTML_ESC_PROP, RES_CHRATR_ESCAPEMENT) );
        }
        break;

    case HtmlTokenId::BLINK_ON:
        {
            NewStdAttr( HtmlTokenId::BLINK_ON, &m_xAttrTab->pBlink,
                        SvxBlinkItem( true, RES_CHRATR_BLINK ) );
        }
        break;

    case HtmlTokenId::SPAN_ON:
        NewStdAttr( HtmlTokenId::SPAN_ON );
        break;

    case HtmlTokenId::ITALIC_OFF:
    case HtmlTokenId::BOLD_OFF:
    case HtmlTokenId::STRIKE_OFF:
    case HtmlTokenId::UNDERLINE_OFF:
    case HtmlTokenId::SUPERSCRIPT_OFF:
    case HtmlTokenId::SUBSCRIPT_OFF:
    case HtmlTokenId::BLINK_OFF:
    case HtmlTokenId::SPAN_OFF:
        EndTag( nToken );
        break;

    case HtmlTokenId::STRIKETHROUGH_OFF:
        EndTag( HtmlTokenId::STRIKE_OFF );
        break;

    case HtmlTokenId::BASEFONT_ON:
        NewBasefontAttr();
        break;
    case HtmlTokenId::BASEFONT_OFF:
        EndBasefontAttr();
        break;
    case HtmlTokenId::FONT_ON:
    case HtmlTokenId::BIGPRINT_ON:
    case HtmlTokenId::SMALLPRINT_ON:
        NewFontAttr( nToken );
        break;
    case HtmlTokenId::FONT_OFF:
    case HtmlTokenId::BIGPRINT_OFF:
    case HtmlTokenId::SMALLPRINT_OFF:
        EndFontAttr( nToken );
        break;

    case HtmlTokenId::EMPHASIS_ON:
    case HtmlTokenId::CITATION_ON:
    case HtmlTokenId::STRONG_ON:
    case HtmlTokenId::CODE_ON:
    case HtmlTokenId::SAMPLE_ON:
    case HtmlTokenId::KEYBOARD_ON:
    case HtmlTokenId::VARIABLE_ON:
    case HtmlTokenId::DEFINSTANCE_ON:
    case HtmlTokenId::SHORTQUOTE_ON:
    case HtmlTokenId::LANGUAGE_ON:
    case HtmlTokenId::AUTHOR_ON:
    case HtmlTokenId::PERSON_ON:
    case HtmlTokenId::ACRONYM_ON:
    case HtmlTokenId::ABBREVIATION_ON:

    case HtmlTokenId::TELETYPE_ON:
        NewCharFormat( nToken );
        break;

    case HtmlTokenId::SDFIELD_ON:
        NewField();
        m_bCallNextToken = m_bInField && m_xTable;
        break;

    case HtmlTokenId::EMPHASIS_OFF:
    case HtmlTokenId::CITATION_OFF:
    case HtmlTokenId::STRONG_OFF:
    case HtmlTokenId::CODE_OFF:
    case HtmlTokenId::SAMPLE_OFF:
    case HtmlTokenId::KEYBOARD_OFF:
    case HtmlTokenId::VARIABLE_OFF:
    case HtmlTokenId::DEFINSTANCE_OFF:
    case HtmlTokenId::SHORTQUOTE_OFF:
    case HtmlTokenId::LANGUAGE_OFF:
    case HtmlTokenId::AUTHOR_OFF:
    case HtmlTokenId::PERSON_OFF:
    case HtmlTokenId::ACRONYM_OFF:
    case HtmlTokenId::ABBREVIATION_OFF:
    case HtmlTokenId::INSERTEDTEXT_OFF:
    case HtmlTokenId::DELETEDTEXT_OFF:

    case HtmlTokenId::TELETYPE_OFF:
        EndTag( nToken );
        break;

    case HtmlTokenId::HEAD_OFF:
        if( !m_aStyleSource.isEmpty() )
        {
            m_pCSS1Parser->ParseStyleSheet( m_aStyleSource );
            m_aStyleSource.clear();
        }
        break;

    case HtmlTokenId::DOCTYPE:
    case HtmlTokenId::BODY_OFF:
    case HtmlTokenId::HTML_OFF:
    case HtmlTokenId::HEAD_ON:
    case HtmlTokenId::TITLE_OFF:
        break;      // don't evaluate further???
    case HtmlTokenId::HTML_ON:
        {
            const HTMLOptions& rHTMLOptions = GetOptions();
            for (size_t i = rHTMLOptions.size(); i; )
            {
                const HTMLOption& rOption = rHTMLOptions[--i];
                if( HtmlOptionId::DIR == rOption.GetToken() )
                {
                    const OUString& rDir = rOption.GetString();
                    SfxItemSet aItemSet( m_xDoc->GetAttrPool(),
                                         m_pCSS1Parser->GetWhichMap() );
                    SvxCSS1PropertyInfo aPropInfo;
                    OUString aDummy;
                    ParseStyleOptions( aDummy, aDummy, aDummy, aItemSet,
                                       aPropInfo, nullptr, &rDir );

                    m_pCSS1Parser->SetPageDescAttrs( nullptr, &aItemSet );
                    break;
                }
            }
        }
        break;

    case HtmlTokenId::INPUT:
        InsertInput();
        break;

    case HtmlTokenId::TEXTAREA_ON:
        NewTextArea();
        m_bCallNextToken = m_bTextArea && m_xTable;
        break;

    case HtmlTokenId::SELECT_ON:
        NewSelect();
        m_bCallNextToken = m_bSelect && m_xTable;
        break;

    case HtmlTokenId::ANCHOR_ON:
        NewAnchor();
        break;

    case HtmlTokenId::ANCHOR_OFF:
        EndAnchor();
        break;

    case HtmlTokenId::COMMENT:
        if( ( aToken.getLength() > 5 ) && ( ! m_bIgnoreHTMLComments ) )
        {
            // insert as Post-It
            // If there are no space characters right behind
            // the <!-- and on front of the -->, leave the comment untouched.
            if( ' ' == aToken[ 3 ] &&
                ' ' == aToken[ aToken.getLength()-3 ] )
            {
                std::u16string_view aComment( aToken.subView( 3, aToken.getLength()-5 ) );
                InsertComment(OUString(comphelper::string::strip(aComment, ' ')));
            }
            else
            {
                OUString aComment = "<" + aToken + ">";
                InsertComment( aComment );
            }
        }
        break;

    case HtmlTokenId::MAP_ON:
        // Image Maps are read asynchronously: At first only an image map is created
        // Areas are processed later. Nevertheless the
        // ImageMap is inserted into the IMap-Array, because it might be used
        // already.
        m_pImageMap = new ImageMap;
        if( ParseMapOptions( m_pImageMap) )
        {
            if (!m_pImageMaps)
                m_pImageMaps.reset( new ImageMaps );
            m_pImageMaps->push_back(std::unique_ptr<ImageMap>(m_pImageMap));
        }
        else
        {
            delete m_pImageMap;
            m_pImageMap = nullptr;
        }
        break;

    case HtmlTokenId::MAP_OFF:
        // there is no ImageMap anymore (don't delete IMap, because it's
        // already contained in the array!)
        m_pImageMap = nullptr;
        break;

    case HtmlTokenId::AREA:
        if( m_pImageMap )
            ParseAreaOptions( m_pImageMap, m_sBaseURL, SvMacroItemId::OnMouseOver,
                                         SvMacroItemId::OnMouseOut );
        break;

    case HtmlTokenId::FRAMESET_ON:
        bInsertUnknown = m_bKeepUnknown;
        break;

    case HtmlTokenId::NOFRAMES_ON:
        if( IsInHeader() )
            FinishHeader();
        bInsertUnknown = m_bKeepUnknown;
        break;

    case HtmlTokenId::UNKNOWNCONTROL_ON:
        // Ignore content of unknown token in the header, if the token
        // does not start with a '!'.
        // (but judging from the code, also if does not start with a '%')
        // (and also if we're not somewhere we consider PRE)
        if( IsInHeader() && !IsReadPRE() && m_aUnknownToken.isEmpty() &&
            !sSaveToken.isEmpty() && '!' != sSaveToken[0] &&
            '%' != sSaveToken[0] )
            m_aUnknownToken = sSaveToken;
        [[fallthrough]];

    default:
        bInsertUnknown = m_bKeepUnknown;
        break;
    }

    if( bGetIDOption )
        InsertIDOption();

    if( bInsertUnknown )
    {
        OUStringBuffer aComment("HTML: <");
        if( (nToken >= HtmlTokenId::ONOFF_START) && isOffToken(nToken) )
            aComment.append("/");
        aComment.append(sSaveToken);
        if( !aToken.isEmpty() )
        {
            UnescapeToken();
            aComment.append(" " + aToken);
        }
        aComment.append(">");
        InsertComment( aComment.makeStringAndClear() );
    }

    // if there are temporary paragraph attributes and the
    // paragraph isn't empty then the paragraph attributes are final.
    if( !m_aParaAttrs.empty() && m_pPam->GetPoint()->GetContentIndex() )
        m_aParaAttrs.clear();
}

static void lcl_swhtml_getItemInfo( const HTMLAttr& rAttr,
                                 bool& rScriptDependent,
                                 sal_uInt16& rScriptType )
{
    switch( rAttr.GetItem().Which() )
    {
    case RES_CHRATR_FONT:
    case RES_CHRATR_FONTSIZE:
    case RES_CHRATR_LANGUAGE:
    case RES_CHRATR_POSTURE:
    case RES_CHRATR_WEIGHT:
        rScriptType = i18n::ScriptType::LATIN;
        rScriptDependent = true;
        break;
    case RES_CHRATR_CJK_FONT:
    case RES_CHRATR_CJK_FONTSIZE:
    case RES_CHRATR_CJK_LANGUAGE:
    case RES_CHRATR_CJK_POSTURE:
    case RES_CHRATR_CJK_WEIGHT:
        rScriptType = i18n::ScriptType::ASIAN;
        rScriptDependent = true;
        break;
    case RES_CHRATR_CTL_FONT:
    case RES_CHRATR_CTL_FONTSIZE:
    case RES_CHRATR_CTL_LANGUAGE:
    case RES_CHRATR_CTL_POSTURE:
    case RES_CHRATR_CTL_WEIGHT:
        rScriptType = i18n::ScriptType::COMPLEX;
        rScriptDependent = true;
        break;
    default:
        rScriptDependent = false;
        break;
    }
}

bool SwHTMLParser::AppendTextNode( SwHTMLAppendMode eMode, bool bUpdateNum )
{
    // A hard line break at the end always must be removed.
    // A second one we replace with paragraph spacing.
    sal_Int32 nLFStripped = StripTrailingLF();
    if( (AM_NOSPACE==eMode || AM_SOFTNOSPACE==eMode) && nLFStripped > 1 )
        eMode = AM_SPACE;

    // the hard attributes of this paragraph will never be invalid again
    m_aParaAttrs.clear();

    SwTextNode *pTextNode = (AM_SPACE==eMode || AM_NOSPACE==eMode) ?
        m_pPam->GetPoint()->GetNode().GetTextNode() : nullptr;

    if (pTextNode)
    {
        const SvxULSpaceItem& rULSpace =
            pTextNode->SwContentNode::GetAttr( RES_UL_SPACE );

        bool bChange = AM_NOSPACE==eMode ? rULSpace.GetLower() > 0
                                         : rULSpace.GetLower() == 0;

        if( bChange )
        {
            const SvxULSpaceItem& rCollULSpace =
                pTextNode->GetAnyFormatColl().GetULSpace();

            bool bMayReset = AM_NOSPACE==eMode ? rCollULSpace.GetLower() == 0
                                               : rCollULSpace.GetLower() > 0;

            if( bMayReset &&
                rCollULSpace.GetUpper() == rULSpace.GetUpper() )
            {
                pTextNode->ResetAttr( RES_UL_SPACE );
            }
            else
            {
                pTextNode->SetAttr(
                    SvxULSpaceItem( rULSpace.GetUpper(),
                         AM_NOSPACE==eMode ? 0 : HTML_PARSPACE, RES_UL_SPACE ) );
            }
        }
    }
    m_bNoParSpace = AM_NOSPACE==eMode || AM_SOFTNOSPACE==eMode;

    SwPosition aOldPos( *m_pPam->GetPoint() );

    bool bRet = m_xDoc->getIDocumentContentOperations().AppendTextNode( *m_pPam->GetPoint() );

    // split character attributes and maybe set none,
    // which are set for the whole paragraph
    const sal_Int32 nEndCnt = aOldPos.GetContentIndex();
    const SwPosition& rPos = *m_pPam->GetPoint();

    HTMLAttr** pHTMLAttributes = reinterpret_cast<HTMLAttr**>(m_xAttrTab.get());
    for (auto nCnt = sizeof(HTMLAttrTable) / sizeof(HTMLAttr*); nCnt--; ++pHTMLAttributes)
    {
        HTMLAttr *pAttr = *pHTMLAttributes;
        if( pAttr && pAttr->GetItem().Which() < RES_PARATR_BEGIN )
        {
            bool bWholePara = false;

            while( pAttr )
            {
                HTMLAttr *pNext = pAttr->GetNext();
                if( pAttr->GetStartParagraphIdx() < aOldPos.GetNodeIndex() ||
                    (!bWholePara &&
                     pAttr->GetStartParagraph() == aOldPos.GetNode() &&
                     pAttr->GetStartContent() != nEndCnt) )
                {
                    bWholePara =
                        pAttr->GetStartParagraph() == aOldPos.GetNode() &&
                        pAttr->GetStartContent() == 0;

                    sal_Int32 nStt = pAttr->m_nStartContent;
                    bool bScript = false;
                    sal_uInt16 nScriptItem;
                    bool bInsert = true;
                    lcl_swhtml_getItemInfo( *pAttr, bScript,
                                            nScriptItem );
                        // set previous part
                    if( bScript )
                    {
                        const SwTextNode *pTextNd =
                            pAttr->GetStartParagraph().GetNode().GetTextNode();
                        OSL_ENSURE( pTextNd, "No text node" );
                        if( pTextNd )
                        {
                            const OUString& rText = pTextNd->GetText();
                            sal_uInt16 nScriptText =
                                g_pBreakIt->GetBreakIter()->getScriptType(
                                            rText, pAttr->GetStartContent() );
                            sal_Int32 nScriptEnd = g_pBreakIt->GetBreakIter()
                                    ->endOfScript( rText, nStt, nScriptText );
                            while (nScriptEnd < nEndCnt && nScriptEnd != -1)
                            {
                                if( nScriptItem == nScriptText )
                                {
                                    HTMLAttr *pSetAttr =
                                        pAttr->Clone( aOldPos.GetNode(), nScriptEnd );
                                    pSetAttr->m_nStartContent = nStt;
                                    pSetAttr->ClearPrev();
                                    if( !pNext || bWholePara )
                                    {
                                        if (pSetAttr->m_bInsAtStart)
                                            m_aSetAttrTab.push_front( pSetAttr );
                                        else
                                            m_aSetAttrTab.push_back( pSetAttr );
                                    }
                                    else
                                        pNext->InsertPrev( pSetAttr );
                                }
                                nStt = nScriptEnd;
                                nScriptText = g_pBreakIt->GetBreakIter()->getScriptType(
                                                rText, nStt );
                                nScriptEnd = g_pBreakIt->GetBreakIter()
                                    ->endOfScript( rText, nStt, nScriptText );
                            }
                            bInsert = nScriptItem == nScriptText;
                        }
                    }
                    if( bInsert )
                    {
                        HTMLAttr *pSetAttr =
                            pAttr->Clone( aOldPos.GetNode(), nEndCnt );
                        pSetAttr->m_nStartContent = nStt;

                        // When the attribute is for the whole paragraph, the outer
                        // attributes aren't effective anymore. Hence it may not be inserted
                        // in the Prev-List of an outer attribute, because that won't be
                        // set. That leads to shifting when fields are used.
                        if( !pNext || bWholePara )
                        {
                            if (pSetAttr->m_bInsAtStart)
                                m_aSetAttrTab.push_front( pSetAttr );
                            else
                                m_aSetAttrTab.push_back( pSetAttr );
                        }
                        else
                            pNext->InsertPrev( pSetAttr );
                    }
                    else
                    {
                        HTMLAttr *pPrev = pAttr->GetPrev();
                        if( pPrev )
                        {
                            // the previous attributes must be set anyway
                            if( !pNext || bWholePara )
                            {
                                if (pPrev->m_bInsAtStart)
                                    m_aSetAttrTab.push_front( pPrev );
                                else
                                    m_aSetAttrTab.push_back( pPrev );
                            }
                            else
                                pNext->InsertPrev( pPrev );
                        }
                    }
                    pAttr->ClearPrev();
                }

                pAttr->SetStart( rPos );
                pAttr = pNext;
            }
        }
    }

    if( bUpdateNum )
    {
        if( GetNumInfo().GetDepth() )
        {
            sal_uInt8 nLvl = GetNumInfo().GetLevel();
            SetNodeNum( nLvl );
        }
        else
            m_pPam->GetPointNode().GetTextNode()->ResetAttr( RES_PARATR_NUMRULE );
    }

    // We must set the attribute of the paragraph before now (because of JavaScript)
    SetAttr();

    // Now it is time to get rid of all script dependent hints that are
    // equal to the settings in the style
    SwTextNode *pTextNd = aOldPos.GetNode().GetTextNode();
    OSL_ENSURE( pTextNd, "There is the txt node" );
    size_t nCntAttr = (pTextNd  && pTextNd->GetpSwpHints())
                            ? pTextNd->GetSwpHints().Count() : 0;
    if( nCntAttr )
    {
        // These are the end position of all script dependent hints.
        // If we find a hint that starts before the current end position,
        // we have to set it. If we find a hint that start behind or at
        // that position, we have to take the hint value into account.
        // If it is equal to the style, or in fact the paragraph value
        // for that hint, the hint is removed. Otherwise its end position
        // is remembered.
        sal_Int32 aEndPos[15] =
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        SwpHints& rHints = pTextNd->GetSwpHints();
        for( size_t i=0; i < nCntAttr; i++ )
        {
            SwTextAttr *pHt = rHints.Get( i );
            sal_uInt16 nWhich = pHt->Which();
            sal_Int16 nIdx = 0;
            bool bFont = false;
            switch( nWhich )
            {
                case RES_CHRATR_FONT:
                    nIdx = 0;
                    bFont = true;
                    break;
                case RES_CHRATR_FONTSIZE:
                    nIdx = 1;
                    break;
                case RES_CHRATR_LANGUAGE:
                    nIdx = 2;
                    break;
                case RES_CHRATR_POSTURE:
                    nIdx = 3;
                    break;
                case RES_CHRATR_WEIGHT:
                    nIdx = 4;
                    break;
                case RES_CHRATR_CJK_FONT:
                    nIdx = 5;
                    bFont = true;
                    break;
                case RES_CHRATR_CJK_FONTSIZE:
                    nIdx = 6;
                    break;
                case RES_CHRATR_CJK_LANGUAGE:
                    nIdx = 7;
                    break;
                case RES_CHRATR_CJK_POSTURE:
                    nIdx = 8;
                    break;
                case RES_CHRATR_CJK_WEIGHT:
                    nIdx = 9;
                    break;
                case RES_CHRATR_CTL_FONT:
                    nIdx = 10;
                    bFont = true;
                    break;
                case RES_CHRATR_CTL_FONTSIZE:
                    nIdx = 11;
                    break;
                case RES_CHRATR_CTL_LANGUAGE:
                    nIdx = 12;
                    break;
                case RES_CHRATR_CTL_POSTURE:
                    nIdx = 13;
                    break;
                case RES_CHRATR_CTL_WEIGHT:
                    nIdx = 14;
                    break;
                default:
                    // Skip to next attribute
                    continue;
            }
            const sal_Int32 nStt = pHt->GetStart();
            if( nStt >= aEndPos[nIdx] )
            {
                const SfxPoolItem& rItem =
                    static_cast<const SwContentNode *>(pTextNd)->GetAttr( nWhich );
                if( bFont ? swhtml_css1atr_equalFontItems(rItem,pHt->GetAttr())
                            : rItem == pHt->GetAttr() )
                {
                    // The hint is the same as set in the paragraph and
                    // therefore, it can be deleted
                    // CAUTION!!! This WILL delete the hint and it MAY
                    // also delete the SwpHints!!! To avoid any trouble
                    // we leave the loop immediately if this is the last
                    // hint.
                    pTextNd->DeleteAttribute( pHt );
                    if( 1 == nCntAttr )
                        break;
                    i--;
                    nCntAttr--;
                }
                else
                {
                    // The hint is different. Therefore all hints within that
                    // hint have to be ignored.
                    aEndPos[nIdx] = pHt->GetEnd() ? *pHt->GetEnd() : nStt;
                }
            }
            else
            {
                // The hint starts before another one ends.
                // The hint in this case is not deleted
                OSL_ENSURE( pHt->GetEnd() && *pHt->GetEnd() <= aEndPos[nIdx],
                        "hints aren't nested properly!" );
            }
        }
    }

    if (!m_xTable && !--m_nParaCnt)
        Show();

    return bRet;
}

void SwHTMLParser::AddParSpace()
{
    //If it already has ParSpace, return
    if( !m_bNoParSpace )
        return;

    m_bNoParSpace = false;

    SwNodeOffset nNdIdx = m_pPam->GetPoint()->GetNodeIndex() - 1;

    SwTextNode *pTextNode = m_xDoc->GetNodes()[nNdIdx]->GetTextNode();
    if( !pTextNode )
        return;

    SvxULSpaceItem rULSpace =
        pTextNode->SwContentNode::GetAttr( RES_UL_SPACE );
    if( rULSpace.GetLower() )
        return;

    const SvxULSpaceItem& rCollULSpace =
        pTextNode->GetAnyFormatColl().GetULSpace();
    if( rCollULSpace.GetLower() &&
        rCollULSpace.GetUpper() == rULSpace.GetUpper() )
    {
        pTextNode->ResetAttr( RES_UL_SPACE );
    }
    else
    {
        //What I do here, is that I examine the attributes, and if
        //I find out, that it's CJK/CTL, then I set the paragraph space
        //to the value set in HTML_CJK_PARSPACE/HTML_CTL_PARSPACE.

        bool bIsCJK = false;
        bool bIsCTL = false;

        const size_t nCntAttr = pTextNode->GetpSwpHints()
                        ? pTextNode->GetSwpHints().Count() : 0;

        for(size_t i = 0; i < nCntAttr; ++i)
        {
            SwTextAttr *const pHt = pTextNode->GetSwpHints().Get(i);
            sal_uInt16 const nWhich = pHt->Which();
            if (RES_CHRATR_CJK_FONT == nWhich ||
                RES_CHRATR_CJK_FONTSIZE == nWhich ||
                RES_CHRATR_CJK_LANGUAGE == nWhich ||
                RES_CHRATR_CJK_POSTURE == nWhich ||
                RES_CHRATR_CJK_WEIGHT == nWhich)
            {
                bIsCJK = true;
                break;
            }
            if (RES_CHRATR_CTL_FONT == nWhich ||
                RES_CHRATR_CTL_FONTSIZE == nWhich ||
                RES_CHRATR_CTL_LANGUAGE == nWhich ||
                RES_CHRATR_CTL_POSTURE == nWhich ||
                RES_CHRATR_CTL_WEIGHT == nWhich)
            {
                bIsCTL = true;
                break;
            }
        }

        if( bIsCTL )
        {
            pTextNode->SetAttr(
                SvxULSpaceItem( rULSpace.GetUpper(), HTML_CTL_PARSPACE, RES_UL_SPACE )  );
        }
        else if( bIsCJK )
        {
            pTextNode->SetAttr(
                SvxULSpaceItem( rULSpace.GetUpper(), HTML_CJK_PARSPACE, RES_UL_SPACE )  );
        } else {
            pTextNode->SetAttr(
                SvxULSpaceItem( rULSpace.GetUpper(), HTML_PARSPACE, RES_UL_SPACE )  );
        }
    }
}

void SwHTMLParser::Show()
{
    // Here
    // - a EndAction is called, so the document is formatted
    // - a Reschedule is called,
    // - the own View-Shell is set again
    // - and a StartAction is called

    OSL_ENSURE( SvParserState::Working==eState, "Show not in working state - That can go wrong" );
    SwViewShell *pOldVSh = CallEndAction();

    Application::Reschedule();

    SwDocShell* pShell = m_xDoc->GetDocShell();
    if( ( pShell && pShell->IsAbortingImport() )
        || 1 == m_xDoc->getReferenceCount() )
    {
        // was the import aborted by SFX?
        eState = SvParserState::Error;
    }

    // Fetch the SwViewShell again, as it could be destroyed in Reschedule.
    SwViewShell *pVSh = CallStartAction( pOldVSh );

    // is the current node not visible anymore, then we use a bigger increment
    if( pVSh )
    {
        m_nParaCnt = (m_pPam->GetPoint()->GetNode().IsInVisibleArea(pVSh))
            ? 5 : 50;
    }
}

void SwHTMLParser::ShowStatline()
{
    // Here
    // - a Reschedule is called, so it can be scrolled
    // - the own View-Shell is set again
    // - a StartAction/EndAction is called, when there was scrolling.

    OSL_ENSURE( SvParserState::Working==eState, "ShowStatLine not in working state - That can go wrong" );

    // scroll bar
    if (m_xProgress)
    {
        m_xProgress->Update(rInput.Tell());
        CheckActionViewShell();
    }
    else
    {
        Application::Reschedule();

        SwDocShell* pShell = m_xDoc->GetDocShell();
        if( ( pShell && pShell->IsAbortingImport() )
            || 1 == m_xDoc->getReferenceCount() )
            // was the import aborted by SFX?
            eState = SvParserState::Error;

        SwViewShell *pVSh = CheckActionViewShell();
        if( pVSh && pVSh->HasInvalidRect() )
        {
            CallEndAction( false, false );
            CallStartAction( pVSh, false );
        }
    }
}

SwViewShell *SwHTMLParser::CallStartAction( SwViewShell *pVSh, bool bChkPtr )
{
    OSL_ENSURE( !m_pActionViewShell, "CallStartAction: SwViewShell already set" );

    if( !pVSh || bChkPtr )
    {
#if OSL_DEBUG_LEVEL > 0
        SwViewShell *pOldVSh = pVSh;
#endif
        pVSh = m_xDoc->getIDocumentLayoutAccess().GetCurrentViewShell();
#if OSL_DEBUG_LEVEL > 0
        OSL_ENSURE( !pVSh || !pOldVSh || pOldVSh == pVSh, "CallStartAction: Who swapped the SwViewShell?" );
        if( pOldVSh && !pVSh )
            pVSh = nullptr;
#endif
    }
    m_pActionViewShell = pVSh;

    if( m_pActionViewShell )
    {
        if( auto pEditShell = dynamic_cast< SwEditShell *>( m_pActionViewShell ) )
            pEditShell->StartAction();
        else
            m_pActionViewShell->StartAction();
    }

    return m_pActionViewShell;
}

SwViewShell *SwHTMLParser::CallEndAction( bool bChkAction, bool bChkPtr )
{
    if( bChkPtr )
    {
        SwViewShell *pVSh = m_xDoc->getIDocumentLayoutAccess().GetCurrentViewShell();
        OSL_ENSURE( !pVSh || m_pActionViewShell == pVSh,
                "CallEndAction: Who swapped the SwViewShell?" );
#if OSL_DEBUG_LEVEL > 0
        if( m_pActionViewShell && !pVSh )
            pVSh = nullptr;
#endif
        if( pVSh != m_pActionViewShell )
            m_pActionViewShell = nullptr;
    }

    if( !m_pActionViewShell || (bChkAction && !m_pActionViewShell->ActionPend()) )
        return m_pActionViewShell;

    if (SwEditShell* pEditShell = dynamic_cast<SwEditShell*>(m_pActionViewShell))
    {
        // Already scrolled?, then make sure that the view doesn't move!
        const bool bOldLock = m_pActionViewShell->IsViewLocked();
        m_pActionViewShell->LockView( true );
        pEditShell->EndAction();
        m_pActionViewShell->LockView( bOldLock );

        // bChkJumpMark is only set when the object was also found
        if( m_bChkJumpMark )
        {
            const Point aVisSttPos( DOCUMENTBORDER, DOCUMENTBORDER );
            if( GetMedium() && aVisSttPos == m_pActionViewShell->VisArea().Pos() )
                ::JumpToSwMark( m_pActionViewShell,
                                ReferenceMarkerName(GetMedium()->GetURLObject().GetMark()) );
            m_bChkJumpMark = false;
        }
    }
    else
        m_pActionViewShell->EndAction();

    // if the parser holds the last reference to the document, then we can
    // abort here and set an error.
    if( 1 == m_xDoc->getReferenceCount() )
    {
        eState = SvParserState::Error;
    }

    SwViewShell *pVSh = m_pActionViewShell;
    m_pActionViewShell = nullptr;

    return pVSh;
}

SwViewShell *SwHTMLParser::CheckActionViewShell()
{
    SwViewShell *pVSh = m_xDoc->getIDocumentLayoutAccess().GetCurrentViewShell();
    OSL_ENSURE( !pVSh || m_pActionViewShell == pVSh,
            "CheckActionViewShell: Who has swapped SwViewShell?" );
#if OSL_DEBUG_LEVEL > 0
    if( m_pActionViewShell && !pVSh )
        pVSh = nullptr;
#endif
    if( pVSh != m_pActionViewShell )
        m_pActionViewShell = nullptr;

    return m_pActionViewShell;
}

SwHTMLFrameFormatListener::SwHTMLFrameFormatListener(SwFrameFormat* pFrameFormat)
    : m_pFrameFormat(pFrameFormat)
{
    StartListening(m_pFrameFormat->GetNotifier());
}

void SwHTMLFrameFormatListener::Notify(const SfxHint& rHint)
{
    if (rHint.GetId() == SfxHintId::Dying)
        m_pFrameFormat = nullptr;
}

void SwHTMLParser::SetAttr_( bool bChkEnd, bool bBeforeTable,
                             std::deque<std::unique_ptr<HTMLAttr>> *pPostIts )
{
    SwPaM aAttrPam( *m_pPam->GetPoint() );
    const SwPosition& rEndPos = *m_pPam->GetPoint();
    const sal_Int32 nEndCnt = m_pPam->GetPoint()->GetContentIndex();
    HTMLAttr* pAttr;
    SwContentNode* pCNd;

    std::vector<std::unique_ptr<HTMLAttr>> aFields;

    for( auto n = m_aSetAttrTab.size(); n; )
    {
        pAttr = m_aSetAttrTab[ --n ];
        sal_uInt16 nWhich = pAttr->m_pItem->Which();

        SwNodeOffset nEndParaIdx = pAttr->GetEndParagraphIdx();
        bool bSetAttr;
        if( bChkEnd )
        {
            // Set character attribute with end early on, so set them still in
            // the current paragraph (because of JavaScript and various "chats"(?)).
            // This shouldn't be done for attributes which are used for
            // the whole paragraph, because they could be from a paragraph style
            // which can't be set. Because the attributes are inserted with
            // SETATTR_DONTREPLACE, they should be able to be set later.
            bSetAttr = ( nEndParaIdx < rEndPos.GetNodeIndex() &&
                         ((RES_MARGIN_FIRSTLINE != nWhich && RES_MARGIN_TEXTLEFT != nWhich) || !GetNumInfo().GetNumRule()) ) ||
                       ( !pAttr->IsLikePara() &&
                         nEndParaIdx == rEndPos.GetNodeIndex() &&
                         pAttr->GetEndContent() < nEndCnt &&
                         (isCHRATR(nWhich) || isTXTATR_WITHEND(nWhich)) ) ||
                       ( bBeforeTable &&
                         nEndParaIdx == rEndPos.GetNodeIndex() &&
                         !pAttr->GetEndContent() );
        }
        else
        {
            // Attributes in body nodes array section shouldn't be set if we are in a
            // special nodes array section, but vice versa it's possible.
            SwNodeOffset nEndOfIcons = m_xDoc->GetNodes().GetEndOfExtras().GetIndex();
            bSetAttr = nEndParaIdx < rEndPos.GetNodeIndex() ||
                       rEndPos.GetNodeIndex() > nEndOfIcons ||
                       nEndParaIdx <= nEndOfIcons;
        }

        if( bSetAttr )
        {
            // The attribute shouldn't be in the list of temporary paragraph
            // attributes, because then it would be deleted.
            while( !m_aParaAttrs.empty() )
            {
                OSL_ENSURE( pAttr != m_aParaAttrs.back(),
                        "SetAttr: Attribute must not yet be set" );
                m_aParaAttrs.pop_back();
            }

            // then set it
            m_aSetAttrTab.erase( m_aSetAttrTab.begin() + n );

            while( pAttr )
            {
                HTMLAttr *pPrev = pAttr->GetPrev();
                if( !pAttr->m_bValid )
                {
                    // invalid attributes can be deleted
                    delete pAttr;
                    pAttr = pPrev;
                    continue;
                }

                pCNd = pAttr->m_nStartPara.GetNode().GetContentNode();
                if( !pCNd )
                {
                    // because of the awful deleting of nodes an index can also
                    // point to an end node :-(
                    if ( (pAttr->GetStartParagraph() == pAttr->GetEndParagraph()) &&
                         !isTXTATR_NOEND(nWhich) )
                    {
                        // when the end index also points to the node, we don't
                        // need to set attributes anymore, except if it's a text attribute.
                        delete pAttr;
                        pAttr = pPrev;
                        continue;
                    }
                    pCNd = SwNodes::GoNext(&(pAttr->m_nStartPara));
                    if( pCNd )
                        pAttr->m_nStartContent = 0;
                    else
                    {
                        OSL_ENSURE( false, "SetAttr: GoNext() failed!" );
                        delete pAttr;
                        pAttr = pPrev;
                        continue;
                    }
                }

                // because of the deleting of BRs the start index can also
                // point behind the end the text
                if( pAttr->m_nStartContent > pCNd->Len() )
                    pAttr->m_nStartContent = pCNd->Len();
                aAttrPam.GetPoint()->Assign( *pCNd, pAttr->m_nStartContent );

                aAttrPam.SetMark();
                if ( (pAttr->GetStartParagraph() != pAttr->GetEndParagraph()) &&
                         !isTXTATR_NOEND(nWhich) )
                {
                    pCNd = pAttr->m_nEndPara.GetNode().GetContentNode();
                    if( !pCNd )
                    {
                        pCNd = SwNodes::GoPrevious( &(pAttr->m_nEndPara) );
                        if( pCNd )
                            pAttr->m_nEndContent = pCNd->Len();
                        else
                        {
                            OSL_ENSURE( false, "SetAttr: GoPrevious() failed!" );
                            aAttrPam.DeleteMark();
                            delete pAttr;
                            pAttr = pPrev;
                            continue;
                        }
                    }
                }
                else if( pAttr->IsLikePara() )
                {
                    pAttr->m_nEndContent = pCNd->Len();
                }

                // because of the deleting of BRs the start index can also
                // point behind the end the text
                if( pAttr->m_nEndContent > pCNd->Len() )
                    pAttr->m_nEndContent = pCNd->Len();

                aAttrPam.GetPoint()->Assign( *pCNd, pAttr->m_nEndContent );
                if( bBeforeTable &&
                    aAttrPam.GetPoint()->GetNodeIndex() ==
                        rEndPos.GetNodeIndex() )
                {
                    // If we're before inserting a table and the attribute ends
                    // in the current node, then we must end it in the previous
                    // node or discard it, if it starts in that node.
                    if( nWhich != RES_BREAK && nWhich != RES_PAGEDESC &&
                         !isTXTATR_NOEND(nWhich) )
                    {
                        if( aAttrPam.GetMark()->GetNodeIndex() !=
                            rEndPos.GetNodeIndex() )
                        {
                            OSL_ENSURE( !aAttrPam.GetPoint()->GetContentIndex(),
                                    "Content-Position before table not 0???" );
                            aAttrPam.Move( fnMoveBackward );
                        }
                        else
                        {
                            aAttrPam.DeleteMark();
                            delete pAttr;
                            pAttr = pPrev;
                            continue;
                        }
                    }
                }

                switch( nWhich )
                {
                case RES_FLTR_BOOKMARK: // insert bookmark
                    {
                        const ReferenceMarkerName sName( static_cast<SfxStringItem*>(pAttr->m_pItem.get())->GetValue() );
                        IDocumentMarkAccess* const pMarkAccess = m_xDoc->getIDocumentMarkAccess();
                        auto ppBkmk = pMarkAccess->findMark( sName );
                        if( ppBkmk != pMarkAccess->getAllMarksEnd() &&
                            (*ppBkmk)->GetMarkStart() == *aAttrPam.GetPoint() )
                            break; // do not generate duplicates on this position
                        aAttrPam.DeleteMark();
                        const ::sw::mark::MarkBase* const pNewMark = pMarkAccess->makeMark(
                            aAttrPam,
                            sName,
                            IDocumentMarkAccess::MarkType::BOOKMARK,
                            ::sw::mark::InsertMode::New);

                        // jump to bookmark
                        if( JumpToMarks::Mark == m_eJumpTo && pNewMark->GetName() == m_sJmpMark )
                        {
                            m_bChkJumpMark = true;
                            m_eJumpTo = JumpToMarks::NONE;
                        }
                    }
                    break;
                case RES_TXTATR_FIELD:
                case RES_TXTATR_ANNOTATION:
                case RES_TXTATR_INPUTFIELD:
                    {
                        SwFieldIds nFieldWhich =
                            pPostIts
                            ? static_cast<const SwFormatField *>(pAttr->m_pItem.get())->GetField()->GetTyp()->Which()
                            : SwFieldIds::Database;
                        if( pPostIts && (SwFieldIds::Postit == nFieldWhich ||
                                         SwFieldIds::Script == nFieldWhich) )
                        {
                            pPostIts->emplace_front( pAttr );
                        }
                        else
                        {
                            aFields.emplace_back( pAttr);
                        }
                    }
                    aAttrPam.DeleteMark();
                    pAttr = pPrev;
                    continue;

                // tdf#94088 expand RES_BACKGROUND to the new fill attribute
                // definitions in the range [XATTR_FILL_FIRST .. XATTR_FILL_LAST].
                // This is the right place in the future if the adapted fill attributes
                // may be handled more directly in HTML import to handle them.
                case RES_BACKGROUND:
                {
                    const SvxBrushItem& rBrush = static_cast< SvxBrushItem& >(*pAttr->m_pItem);
                    SfxItemSetFixed<XATTR_FILL_FIRST, XATTR_FILL_LAST> aNewSet(m_xDoc->GetAttrPool());

                    setSvxBrushItemAsFillAttributesToTargetSet(rBrush, aNewSet);
                    m_xDoc->getIDocumentContentOperations().InsertItemSet(aAttrPam, aNewSet, SetAttrMode::DONTREPLACE);
                    break;
                }

                case RES_LR_SPACE:
                    assert(false);
                    break;

                case RES_MARGIN_FIRSTLINE:
                case RES_MARGIN_TEXTLEFT:
                case RES_MARGIN_RIGHT:
                    if( aAttrPam.GetPoint()->GetNodeIndex() ==
                        aAttrPam.GetMark()->GetNodeIndex())
                    {
                        // because of numbering set this attribute directly at node
                        pCNd->SetAttr( *pAttr->m_pItem );
                        break;
                    }
                    OSL_ENSURE( false,
                            "LRSpace set over multiple paragraphs!" );
                    [[fallthrough]]; // (shouldn't reach this point anyway)
                default:

                    // maybe jump to a bookmark
                    if( RES_TXTATR_INETFMT == nWhich &&
                        JumpToMarks::Mark == m_eJumpTo &&
                        m_sJmpMark == static_cast<SwFormatINetFormat*>(pAttr->m_pItem.get())->GetName() )
                    {
                        m_bChkJumpMark = true;
                        m_eJumpTo = JumpToMarks::NONE;
                    }

                    m_xDoc->getIDocumentContentOperations().InsertPoolItem( aAttrPam, *pAttr->m_pItem, SetAttrMode::DONTREPLACE );
                }
                aAttrPam.DeleteMark();

                delete pAttr;
                pAttr = pPrev;
            }
        }
    }

    for( auto n = m_aMoveFlyFrames.size(); n; )
    {
        SwFrameFormat *pFrameFormat = m_aMoveFlyFrames[--n]->GetFrameFormat();
        if (!pFrameFormat)
        {
            SAL_WARN("sw.html", "SwFrameFormat deleted during import");
            m_aMoveFlyFrames.erase( m_aMoveFlyFrames.begin() + n );
            m_aMoveFlyCnts.erase( m_aMoveFlyCnts.begin() + n );
            continue;
        }

        const SwFormatAnchor& rAnchor = pFrameFormat->GetAnchor();
        OSL_ENSURE( RndStdIds::FLY_AT_PARA == rAnchor.GetAnchorId(),
                "Only At-Para flys need special handling" );
        SwNodeOffset nFlyParaIdx = rAnchor.GetAnchorNode()->GetIndex();
        bool bMoveFly;
        if( bChkEnd )
        {
            bMoveFly = nFlyParaIdx < rEndPos.GetNodeIndex() ||
                       ( nFlyParaIdx == rEndPos.GetNodeIndex() &&
                         m_aMoveFlyCnts[n] < nEndCnt );
        }
        else
        {
            SwNodeOffset nEndOfIcons = m_xDoc->GetNodes().GetEndOfExtras().GetIndex();
            bMoveFly = nFlyParaIdx < rEndPos.GetNodeIndex() ||
                       rEndPos.GetNodeIndex() > nEndOfIcons ||
                       nFlyParaIdx <= nEndOfIcons;
        }
        if( bMoveFly )
        {
            pFrameFormat->DelFrames();
            *aAttrPam.GetPoint() = *rAnchor.GetContentAnchor();
            aAttrPam.GetPoint()->SetContent( m_aMoveFlyCnts[n] );
            SwFormatAnchor aAnchor( rAnchor );
            aAnchor.SetType( RndStdIds::FLY_AT_CHAR );
            aAnchor.SetAnchor( aAttrPam.GetPoint() );
            pFrameFormat->SetFormatAttr( aAnchor );

            const SwFormatHoriOrient& rHoriOri = pFrameFormat->GetHoriOrient();
            if( text::HoriOrientation::LEFT == rHoriOri.GetHoriOrient() )
            {
                SwFormatHoriOrient aHoriOri( rHoriOri );
                aHoriOri.SetRelationOrient( text::RelOrientation::CHAR );
                pFrameFormat->SetFormatAttr( aHoriOri );
            }
            const SwFormatVertOrient& rVertOri = pFrameFormat->GetVertOrient();
            if( text::VertOrientation::TOP == rVertOri.GetVertOrient() )
            {
                SwFormatVertOrient aVertOri( rVertOri );
                aVertOri.SetRelationOrient( text::RelOrientation::CHAR );
                pFrameFormat->SetFormatAttr( aVertOri );
            }

            pFrameFormat->MakeFrames();
            m_aMoveFlyFrames.erase( m_aMoveFlyFrames.begin() + n );
            m_aMoveFlyCnts.erase( m_aMoveFlyCnts.begin() + n );
        }
    }
    for (auto & field : aFields)
    {
        pCNd = field->m_nStartPara.GetNode().GetContentNode();
        aAttrPam.GetPoint()->Assign( *pCNd, field->m_nStartContent );

        if( bBeforeTable &&
            aAttrPam.GetPoint()->GetNodeIndex() == rEndPos.GetNodeIndex() )
        {
            OSL_ENSURE( !bBeforeTable, "Aha, the case does occur" );
            OSL_ENSURE( !aAttrPam.GetPoint()->GetContentIndex(),
                    "Content-Position before table not 0???" );
            // !!!
            aAttrPam.Move( fnMoveBackward );
        }

        m_xDoc->getIDocumentContentOperations().InsertPoolItem( aAttrPam, *field->m_pItem );

        field.reset();
    }
    aFields.clear();
}

void SwHTMLParser::NewAttr(const std::shared_ptr<HTMLAttrTable>& rAttrTable, HTMLAttr **ppAttr, const SfxPoolItem& rItem )
{
    // Font height and font colour as well as escape attributes may not be
    // combined. Therefore they're saved in a list and in it the last opened
    // attribute is at the beginning and count is always one. For all other
    // attributes count is just incremented.
    if( *ppAttr )
    {
        HTMLAttr *pAttr = new HTMLAttr(*m_pPam->GetPoint(), rItem, ppAttr, rAttrTable);
        pAttr->InsertNext( *ppAttr );
        (*ppAttr) = pAttr;
    }
    else
        (*ppAttr) = new HTMLAttr(*m_pPam->GetPoint(), rItem, ppAttr, rAttrTable);
}

bool SwHTMLParser::EndAttr( HTMLAttr* pAttr, bool bChkEmpty )
{
    bool bRet = true;

    // The list header is saved in the attribute.
    HTMLAttr **ppHead = pAttr->m_ppHead;

    OSL_ENSURE( ppHead, "No list header attribute found!" );

    // save the current position as end position
    const SwPosition* pEndPos = m_pPam->GetPoint();
    sal_Int32 nEndCnt = m_pPam->GetPoint()->GetContentIndex();

    // Is the last started or an earlier started attribute being ended?
    HTMLAttr *pLast = nullptr;
    if( ppHead && pAttr != *ppHead )
    {
        // The last started attribute isn't being ended

        // Then we look for attribute which was started immediately afterwards,
        // which has also not yet been ended (otherwise it would no longer be
        // in the list).
        pLast = *ppHead;
        while( pLast && pLast->GetNext() != pAttr )
            pLast = pLast->GetNext();

        OSL_ENSURE( pLast, "Attribute not found in own list!" );
    }

    bool bMoveBack = false;
    sal_uInt16 nWhich = pAttr->m_pItem->Which();
    if( !nEndCnt && RES_PARATR_BEGIN <= nWhich &&
        pEndPos->GetNodeIndex() != pAttr->GetStartParagraph().GetIndex() )
    {
        // Then move back one position in the content!
        bMoveBack = m_pPam->Move( fnMoveBackward );
        nEndCnt = m_pPam->GetPoint()->GetContentIndex();
    }

    // now end the attribute
    HTMLAttr *pNext = pAttr->GetNext();

    bool bInsert;
    sal_uInt16 nScriptItem = 0;
    bool bScript = false;
    // does it have a non-empty range?
    if( !bChkEmpty || (RES_PARATR_BEGIN <= nWhich && bMoveBack) ||
        RES_PAGEDESC == nWhich || RES_BREAK == nWhich ||
        pEndPos->GetNodeIndex() != pAttr->GetStartParagraph().GetIndex() ||
        nEndCnt != pAttr->GetStartContent() )
    {
        bInsert = true;
        // We do some optimization for script dependent attributes here.
        if( pEndPos->GetNodeIndex() == pAttr->GetStartParagraph().GetIndex() )
        {
            lcl_swhtml_getItemInfo( *pAttr, bScript, nScriptItem );
        }
    }
    else
    {
        bInsert = false;
    }

    const SwTextNode *pTextNd = (bInsert && bScript) ?
        pAttr->GetStartParagraph().GetNode().GetTextNode() :
        nullptr;

    if (pTextNd)
    {
        const OUString& rText = pTextNd->GetText();
        sal_uInt16 nScriptText = g_pBreakIt->GetBreakIter()->getScriptType(
                        rText, pAttr->GetStartContent() );
        sal_Int32 nScriptEnd = g_pBreakIt->GetBreakIter()
                    ->endOfScript( rText, pAttr->GetStartContent(), nScriptText );
        while (nScriptEnd < nEndCnt && nScriptEnd != -1)
        {
            if( nScriptItem == nScriptText )
            {
                HTMLAttr *pSetAttr = pAttr->Clone( pEndPos->GetNode(), nScriptEnd );
                pSetAttr->ClearPrev();
                if( pNext )
                    pNext->InsertPrev( pSetAttr );
                else
                {
                    if (pSetAttr->m_bInsAtStart)
                        m_aSetAttrTab.push_front( pSetAttr );
                    else
                        m_aSetAttrTab.push_back( pSetAttr );
                }
            }
            pAttr->m_nStartContent = nScriptEnd;
            nScriptText = g_pBreakIt->GetBreakIter()->getScriptType(
                            rText, nScriptEnd );
            nScriptEnd = g_pBreakIt->GetBreakIter()
                    ->endOfScript( rText, nScriptEnd, nScriptText );
        }
        bInsert = nScriptItem == nScriptText;
    }
    if( bInsert )
    {
        pAttr->m_nEndPara = pEndPos->GetNode();
        pAttr->m_nEndContent = nEndCnt;
        pAttr->m_bInsAtStart = RES_TXTATR_INETFMT != nWhich &&
                             RES_TXTATR_CHARFMT != nWhich;

        if( !pNext )
        {
            // No open attributes of that type exists any longer, so all
            // can be set. Except they depend on another attribute, then
            // they're appended there.
            if (pAttr->m_bInsAtStart)
                m_aSetAttrTab.push_front( pAttr );
            else
                m_aSetAttrTab.push_back( pAttr );
        }
        else
        {
            // There are other open attributes of that type,
            // therefore the setting must be postponed.
            // Hence the current attribute is added at the end
            // of the Prev-List of the successor.
            pNext->InsertPrev( pAttr );
        }
    }
    else
    {
        // Then don't insert, but delete. Because of the "faking" of styles
        // by hard attributing there can be also other empty attributes in the
        // Prev-List, which must be set anyway.
        HTMLAttr *pPrev = pAttr->GetPrev();
        bRet = false;
        delete pAttr;

        if( pPrev )
        {
            // The previous attributes must be set anyway.
            if( pNext )
                pNext->InsertPrev( pPrev );
            else
            {
                if (pPrev->m_bInsAtStart)
                    m_aSetAttrTab.push_front( pPrev );
                else
                    m_aSetAttrTab.push_back( pPrev );
            }
        }

    }

    // If the first attribute of the list was set, then the list header
    // must be corrected as well.
    if( pLast )
        pLast->m_pNext = pNext;
    else if( ppHead )
        *ppHead = pNext;

    if( bMoveBack )
        m_pPam->Move( fnMoveForward );

    return bRet;
}

void SwHTMLParser::DeleteAttr( HTMLAttr* pAttr )
{
    // preliminary paragraph attributes are not allowed here, they could
    // be set here and then the pointers become invalid!
    OSL_ENSURE(m_aParaAttrs.empty(),
        "Danger: there are non-final paragraph attributes");
    m_aParaAttrs.clear();

    // The list header is saved in the attribute
    HTMLAttr **ppHead = pAttr->m_ppHead;

    OSL_ENSURE( ppHead, "no list header attribute found!" );

    // Is the last started or an earlier started attribute being removed?
    HTMLAttr *pLast = nullptr;
    if( ppHead && pAttr != *ppHead )
    {
        // The last started attribute isn't being ended

        // Then we look for attribute which was started immediately afterwards,
        // which has also not yet been ended (otherwise it would no longer be
        // in the list).
        pLast = *ppHead;
        while( pLast && pLast->GetNext() != pAttr )
            pLast = pLast->GetNext();

        OSL_ENSURE( pLast, "Attribute not found in own list!" );
    }

    // now delete the attribute
    HTMLAttr *pNext = pAttr->GetNext();
    HTMLAttr *pPrev = pAttr->GetPrev();
    //hold ref to xAttrTab until end of scope to ensure *ppHead validity
    std::shared_ptr<HTMLAttrTable> xKeepAlive(pAttr->m_xAttrTab);
    delete pAttr;

    if( pPrev )
    {
        // The previous attributes must be set anyway.
        if( pNext )
            pNext->InsertPrev( pPrev );
        else
        {
            if (pPrev->m_bInsAtStart)
                m_aSetAttrTab.push_front( pPrev );
            else
                m_aSetAttrTab.push_back( pPrev );
        }
    }

    // If the first attribute of the list was deleted, then the list header
    // must be corrected as well.
    if( pLast )
        pLast->m_pNext = pNext;
    else if( ppHead )
        *ppHead = pNext;
}

void SwHTMLParser::SaveAttrTab(std::shared_ptr<HTMLAttrTable> const & rNewAttrTab)
{
    // preliminary paragraph attributes are not allowed here, they could
    // be set here and then the pointers become invalid!
    OSL_ENSURE(m_aParaAttrs.empty(),
            "Danger: there are non-final paragraph attributes");
    m_aParaAttrs.clear();

    HTMLAttr** pHTMLAttributes = reinterpret_cast<HTMLAttr**>(m_xAttrTab.get());
    HTMLAttr** pSaveAttributes = reinterpret_cast<HTMLAttr**>(rNewAttrTab.get());

    for (auto nCnt = sizeof(HTMLAttrTable) / sizeof(HTMLAttr*); nCnt--; ++pHTMLAttributes, ++pSaveAttributes)
    {
        *pSaveAttributes = *pHTMLAttributes;

        HTMLAttr *pAttr = *pSaveAttributes;
        while (pAttr)
        {
            pAttr->SetHead(pSaveAttributes, rNewAttrTab);
            pAttr = pAttr->GetNext();
        }

        *pHTMLAttributes = nullptr;
    }
}

void SwHTMLParser::SplitAttrTab( std::shared_ptr<HTMLAttrTable> const & rNewAttrTab,
                                 bool bMoveEndBack )
{
    // preliminary paragraph attributes are not allowed here, they could
    // be set here and then the pointers become invalid!
    OSL_ENSURE(m_aParaAttrs.empty(),
            "Danger: there are non-final paragraph attributes");
    m_aParaAttrs.clear();

    SwNodeIndex nEndIdx( m_pPam->GetPoint()->GetNode() );

    // close all still open attributes and re-open them after the table
    HTMLAttr** pHTMLAttributes = reinterpret_cast<HTMLAttr**>(m_xAttrTab.get());
    HTMLAttr** pSaveAttributes = reinterpret_cast<HTMLAttr**>(rNewAttrTab.get());
    bool bSetAttr = true;
    const sal_Int32 nSttCnt = m_pPam->GetPoint()->GetContentIndex();
    sal_Int32 nEndCnt = nSttCnt;

    if( bMoveEndBack )
    {
        SwNodeOffset nOldEnd = nEndIdx.GetIndex();
        SwNodeOffset nTmpIdx;
        if( ( nTmpIdx = m_xDoc->GetNodes().GetEndOfExtras().GetIndex()) >= nOldEnd ||
            ( nTmpIdx = m_xDoc->GetNodes().GetEndOfAutotext().GetIndex()) >= nOldEnd )
        {
            nTmpIdx = m_xDoc->GetNodes().GetEndOfInserts().GetIndex();
        }
        SwContentNode* pCNd = SwNodes::GoPrevious(&nEndIdx);

        // Don't set attributes, when the PaM was moved outside of the content area.
        bSetAttr = pCNd && nTmpIdx < nEndIdx.GetIndex();

        nEndCnt = (bSetAttr ? pCNd->Len() : 0);
    }
    for (auto nCnt = sizeof(HTMLAttrTable) / sizeof(HTMLAttr*); nCnt--; (++pHTMLAttributes, ++pSaveAttributes))
    {
        HTMLAttr *pAttr = *pHTMLAttributes;
        *pSaveAttributes = nullptr;
        while( pAttr )
        {
            HTMLAttr *pNext = pAttr->GetNext();
            HTMLAttr *pPrev = pAttr->GetPrev();

            if( bSetAttr &&
                ( pAttr->GetStartParagraphIdx() < nEndIdx.GetIndex() ||
                  (pAttr->GetStartParagraph() == nEndIdx &&
                   pAttr->GetStartContent() != nEndCnt) ) )
            {
                // The attribute must be set before the list. We need the
                // original and therefore we clone it, because pointer to the
                // attribute exist in the other contexts. The Next-List is lost
                // in doing so, but the Previous-List is preserved.
                HTMLAttr *pSetAttr = pAttr->Clone( nEndIdx.GetNode(), nEndCnt );

                if( pNext )
                    pNext->InsertPrev( pSetAttr );
                else
                {
                    if (pSetAttr->m_bInsAtStart)
                        m_aSetAttrTab.push_front( pSetAttr );
                    else
                        m_aSetAttrTab.push_back( pSetAttr );
                }
            }
            else if( pPrev )
            {
                // If the attribute doesn't need to be set before the table, then
                // the previous attributes must still be set.
                if( pNext )
                    pNext->InsertPrev( pPrev );
                else
                {
                    if (pPrev->m_bInsAtStart)
                        m_aSetAttrTab.push_front( pPrev );
                    else
                        m_aSetAttrTab.push_back( pPrev );
                }
            }

            // set the start of the attribute anew and break link
            pAttr->Reset(m_pPam->GetPoint()->GetNode(), nSttCnt, pSaveAttributes, rNewAttrTab);

            if (*pSaveAttributes)
            {
                HTMLAttr *pSAttr = *pSaveAttributes;
                while( pSAttr->GetNext() )
                    pSAttr = pSAttr->GetNext();
                pSAttr->InsertNext( pAttr );
            }
            else
                *pSaveAttributes = pAttr;

            pAttr = pNext;
        }

        *pHTMLAttributes = nullptr;
    }
}

void SwHTMLParser::RestoreAttrTab(std::shared_ptr<HTMLAttrTable> const & rNewAttrTab)
{
    // preliminary paragraph attributes are not allowed here, they could
    // be set here and then the pointers become invalid!
    OSL_ENSURE(m_aParaAttrs.empty(),
            "Danger: there are non-final paragraph attributes");
    m_aParaAttrs.clear();

    HTMLAttr** pHTMLAttributes = reinterpret_cast<HTMLAttr**>(m_xAttrTab.get());
    HTMLAttr** pSaveAttributes = reinterpret_cast<HTMLAttr**>(rNewAttrTab.get());

    for (auto nCnt = sizeof(HTMLAttrTable) / sizeof(HTMLAttr*); nCnt--; ++pHTMLAttributes, ++pSaveAttributes)
    {
        OSL_ENSURE(!*pHTMLAttributes, "The attribute table is not empty!");

        *pHTMLAttributes = *pSaveAttributes;

        HTMLAttr *pAttr = *pHTMLAttributes;
        while (pAttr)
        {
            OSL_ENSURE( !pAttr->GetPrev() || !pAttr->GetPrev()->m_ppHead,
                    "Previous attribute has still a header" );
            pAttr->SetHead(pHTMLAttributes, m_xAttrTab);
            pAttr = pAttr->GetNext();
        }

        *pSaveAttributes = nullptr;
    }
}

void SwHTMLParser::InsertAttr( const SfxPoolItem& rItem, bool bInsAtStart )
{
    HTMLAttr* pTmp = new HTMLAttr(*m_pPam->GetPoint(), rItem, nullptr, std::shared_ptr<HTMLAttrTable>());
    if (bInsAtStart)
        m_aSetAttrTab.push_front( pTmp );
    else
        m_aSetAttrTab.push_back( pTmp );
}

void SwHTMLParser::InsertAttrs( std::deque<std::unique_ptr<HTMLAttr>> rAttrs )
{
    while( !rAttrs.empty() )
    {
        std::unique_ptr<HTMLAttr> pAttr = std::move(rAttrs.front());
        InsertAttr( pAttr->GetItem(), false );
        rAttrs.pop_front();
    }
}

void SwHTMLParser::NewStdAttr( HtmlTokenId nToken )
{
    OUString aId, aStyle, aLang, aDir;
    OUString aClass;

    const HTMLOptions& rHTMLOptions = GetOptions();
    for (size_t i = rHTMLOptions.size(); i; )
    {
        const HTMLOption& rOption = rHTMLOptions[--i];
        switch( rOption.GetToken() )
        {
        case HtmlOptionId::ID:
            aId = rOption.GetString();
            break;
        case HtmlOptionId::STYLE:
            aStyle = rOption.GetString();
            break;
        case HtmlOptionId::CLASS:
            aClass = rOption.GetString();
            break;
        case HtmlOptionId::LANG:
            aLang = rOption.GetString();
            break;
        case HtmlOptionId::DIR:
            aDir = rOption.GetString();
            break;
        default: break;
        }
    }

    // create a new context
    std::unique_ptr<HTMLAttrContext> xCntxt(new HTMLAttrContext(nToken));

    // parse styles
    if( HasStyleOptions( aStyle, aId, aClass, &aLang, &aDir ) )
    {
        SfxItemSet aItemSet( m_xDoc->GetAttrPool(), m_pCSS1Parser->GetWhichMap() );
        SvxCSS1PropertyInfo aPropInfo;

        if( ParseStyleOptions( aStyle, aId, aClass, aItemSet, aPropInfo, &aLang, &aDir ) )
        {
            if( HtmlTokenId::SPAN_ON != nToken || aClass.isEmpty() ||
                !CreateContainer( aClass, aItemSet, aPropInfo, xCntxt.get() ) )
                DoPositioning( aItemSet, aPropInfo, xCntxt.get() );
            InsertAttrs( aItemSet, aPropInfo, xCntxt.get(), true );
        }
    }

    // save the context
    PushContext(xCntxt);
}

void SwHTMLParser::NewStdAttr( HtmlTokenId nToken,
                               HTMLAttr **ppAttr, const SfxPoolItem & rItem,
                               HTMLAttr **ppAttr2, const SfxPoolItem *pItem2,
                               HTMLAttr **ppAttr3, const SfxPoolItem *pItem3 )
{
    OUString aId, aStyle, aClass, aLang, aDir;

    const HTMLOptions& rHTMLOptions = GetOptions();
    for (size_t i = rHTMLOptions.size(); i; )
    {
        const HTMLOption& rOption = rHTMLOptions[--i];
        switch( rOption.GetToken() )
        {
        case HtmlOptionId::ID:
            aId = rOption.GetString();
            break;
        case HtmlOptionId::STYLE:
            aStyle = rOption.GetString();
            break;
        case HtmlOptionId::CLASS:
            aClass = rOption.GetString();
            break;
        case HtmlOptionId::LANG:
            aLang = rOption.GetString();
            break;
        case HtmlOptionId::DIR:
            aDir = rOption.GetString();
            break;
        default: break;
        }
    }

    // create a new context
    std::unique_ptr<HTMLAttrContext> xCntxt(new HTMLAttrContext(nToken));

    // parse styles
    if( HasStyleOptions( aStyle, aId, aClass, &aLang, &aDir ) )
    {
        SfxItemSet aItemSet( m_xDoc->GetAttrPool(), m_pCSS1Parser->GetWhichMap() );
        SvxCSS1PropertyInfo aPropInfo;

        aItemSet.Put( rItem );
        if( pItem2 )
            aItemSet.Put( *pItem2 );
        if( pItem3 )
            aItemSet.Put( *pItem3 );

        if( ParseStyleOptions( aStyle, aId, aClass, aItemSet, aPropInfo, &aLang, &aDir ) )
            DoPositioning( aItemSet, aPropInfo, xCntxt.get() );

        InsertAttrs( aItemSet, aPropInfo, xCntxt.get(), true );
    }
    else
    {
        InsertAttr( ppAttr ,rItem, xCntxt.get() );
        if( pItem2 )
        {
            OSL_ENSURE( ppAttr2, "missing table entry for item2" );
            InsertAttr( ppAttr2, *pItem2, xCntxt.get() );
        }
        if( pItem3 )
        {
            OSL_ENSURE( ppAttr3, "missing table entry for item3" );
            InsertAttr( ppAttr3, *pItem3, xCntxt.get() );
        }
    }

    // save the context
    PushContext(xCntxt);
}

void SwHTMLParser::EndTag( HtmlTokenId nToken )
{
    // fetch context
    std::unique_ptr<HTMLAttrContext> xCntxt(PopContext(getOnToken(nToken)));
    if (xCntxt)
    {
        // and maybe end the attributes
        EndContext(xCntxt.get());
    }
}

void SwHTMLParser::NewBasefontAttr()
{
    OUString aId, aStyle, aClass, aLang, aDir;
    sal_uInt16 nSize = 3;

    const HTMLOptions& rHTMLOptions = GetOptions();
    for (size_t i = rHTMLOptions.size(); i; )
    {
        const HTMLOption& rOption = rHTMLOptions[--i];
        switch( rOption.GetToken() )
        {
        case HtmlOptionId::SIZE:
            nSize = o3tl::narrowing<sal_uInt16>(rOption.GetNumber());
            break;
        case HtmlOptionId::ID:
            aId = rOption.GetString();
            break;
        case HtmlOptionId::STYLE:
            aStyle = rOption.GetString();
            break;
        case HtmlOptionId::CLASS:
            aClass = rOption.GetString();
            break;
        case HtmlOptionId::LANG:
            aLang = rOption.GetString();
            break;
        case HtmlOptionId::DIR:
            aDir = rOption.GetString();
            break;
        default: break;
        }
    }

    if( nSize < 1 )
        nSize = 1;

    if( nSize > 7 )
        nSize = 7;

    // create a new context
    std::unique_ptr<HTMLAttrContext> xCntxt(new HTMLAttrContext(HtmlTokenId::BASEFONT_ON));

    // parse styles
    if( HasStyleOptions( aStyle, aId, aClass, &aLang, &aDir ) )
    {
        SfxItemSet aItemSet( m_xDoc->GetAttrPool(), m_pCSS1Parser->GetWhichMap() );
        SvxCSS1PropertyInfo aPropInfo;

        //CJK has different defaults
        SvxFontHeightItem aFontHeight( m_aFontHeights[nSize-1], 100, RES_CHRATR_FONTSIZE );
        aItemSet.Put( aFontHeight );
        SvxFontHeightItem aFontHeightCJK( m_aFontHeights[nSize-1], 100, RES_CHRATR_CJK_FONTSIZE );
        aItemSet.Put( aFontHeightCJK );
        //Complex type can contain so many types of letters,
        //that it's not really worthy to bother, IMO.
        //Still, I have set a default.
        SvxFontHeightItem aFontHeightCTL( m_aFontHeights[nSize-1], 100, RES_CHRATR_CTL_FONTSIZE );
        aItemSet.Put( aFontHeightCTL );

        if( ParseStyleOptions( aStyle, aId, aClass, aItemSet, aPropInfo, &aLang, &aDir ) )
            DoPositioning( aItemSet, aPropInfo, xCntxt.get() );

        InsertAttrs( aItemSet, aPropInfo, xCntxt.get(), true );
    }
    else
    {
        SvxFontHeightItem aFontHeight( m_aFontHeights[nSize-1], 100, RES_CHRATR_FONTSIZE );
        InsertAttr( &m_xAttrTab->pFontHeight, aFontHeight, xCntxt.get() );
        SvxFontHeightItem aFontHeightCJK( m_aFontHeights[nSize-1], 100, RES_CHRATR_CJK_FONTSIZE );
        InsertAttr( &m_xAttrTab->pFontHeightCJK, aFontHeightCJK, xCntxt.get() );
        SvxFontHeightItem aFontHeightCTL( m_aFontHeights[nSize-1], 100, RES_CHRATR_CTL_FONTSIZE );
        InsertAttr( &m_xAttrTab->pFontHeightCTL, aFontHeightCTL, xCntxt.get() );
    }

    // save the context
    PushContext(xCntxt);

    // save the font size
    m_aBaseFontStack.push_back( nSize );
}

void SwHTMLParser::EndBasefontAttr()
{
    EndTag( HtmlTokenId::BASEFONT_ON );

    // avoid stack underflow in tables
    if( m_aBaseFontStack.size() > m_nBaseFontStMin )
        m_aBaseFontStack.erase( m_aBaseFontStack.begin() + m_aBaseFontStack.size() - 1 );
}

void SwHTMLParser::NewFontAttr( HtmlTokenId nToken )
{
    sal_uInt16 nBaseSize =
        ( m_aBaseFontStack.size() > m_nBaseFontStMin
            ? (m_aBaseFontStack[m_aBaseFontStack.size()-1] & FONTSIZE_MASK)
            : 3 );
    sal_uInt16 nFontSize =
        ( m_aFontStack.size() > m_nFontStMin
            ? (m_aFontStack[m_aFontStack.size()-1] & FONTSIZE_MASK)
            : nBaseSize );

    OUString aFace, aId, aStyle, aClass, aLang, aDir;
    Color aColor;
    sal_uLong nFontHeight = 0;  // actual font height to set
    sal_uInt32 nSize = 0;       // font height in Netscape notation (1-7)
    bool bColor = false;

    const HTMLOptions& rHTMLOptions = GetOptions();
    for (size_t i = rHTMLOptions.size(); i; )
    {
        const HTMLOption& rOption = rHTMLOptions[--i];
        switch( rOption.GetToken() )
        {
        case HtmlOptionId::SIZE:
            if( HtmlTokenId::FONT_ON==nToken && !rOption.GetString().isEmpty() )
            {
                sal_Int32 nSSize;
                if( '+' == rOption.GetString()[0] ||
                    '-' == rOption.GetString()[0] )
                    nSSize = o3tl::saturating_add<sal_Int32>(nBaseSize, rOption.GetSNumber());
                else
                    nSSize = static_cast<sal_Int32>(rOption.GetNumber());

                if( nSSize < 1 )
                    nSSize = 1;
                else if( nSSize > 7 )
                    nSSize = 7;

                nSize = nSSize;
                nFontHeight = m_aFontHeights[nSize-1];
            }
            break;
        case HtmlOptionId::COLOR:
            if( HtmlTokenId::FONT_ON==nToken )
            {
                rOption.GetColor( aColor );
                bColor = true;
            }
            break;
        case HtmlOptionId::FACE:
            if( HtmlTokenId::FONT_ON==nToken )
                aFace = rOption.GetString();
            break;
        case HtmlOptionId::ID:
            aId = rOption.GetString();
            break;
        case HtmlOptionId::STYLE:
            aStyle = rOption.GetString();
            break;
        case HtmlOptionId::CLASS:
            aClass = rOption.GetString();
            break;
        case HtmlOptionId::LANG:
            aLang = rOption.GetString();
            break;
        case HtmlOptionId::DIR:
            aDir = rOption.GetString();
            break;
        default: break;
        }
    }

    if( HtmlTokenId::FONT_ON != nToken )
    {
        // HTML_BIGPRINT_ON or HTML_SMALLPRINT_ON

        // In headings the current heading sets the font height
        // and not BASEFONT.
        const SwFormatColl *pColl = GetCurrFormatColl();
        sal_uInt16 nPoolId = pColl ? pColl->GetPoolFormatId() : 0;
        if( nPoolId>=RES_POOLCOLL_HEADLINE1 &&
            nPoolId<=RES_POOLCOLL_HEADLINE6 )
        {
            // If the font height in the heading wasn't changed yet,
            // then take the one from the style.
            if( m_nFontStHeadStart==m_aFontStack.size() )
                nFontSize = static_cast< sal_uInt16 >(6 - (nPoolId - RES_POOLCOLL_HEADLINE1));
        }
        else
            nPoolId = 0;

        if( HtmlTokenId::BIGPRINT_ON == nToken )
            nSize = ( nFontSize<7 ? nFontSize+1 : 7 );
        else
            nSize = ( nFontSize>1 ? nFontSize-1 : 1 );

        // If possible in headlines we fetch the new font height
        // from the style.
        if( nPoolId && nSize>=1 && nSize <=6 )
            nFontHeight =
                m_pCSS1Parser->GetTextCollFromPool(
                    RES_POOLCOLL_HEADLINE1+6-nSize )->GetSize().GetHeight();
        else
            nFontHeight = m_aFontHeights[nSize-1];
    }

    OSL_ENSURE( !nSize == !nFontHeight, "HTML-Font-Size != Font-Height" );

    OUString aFontName;
    const OUString aStyleName;
    FontFamily eFamily = FAMILY_DONTKNOW;   // family and pitch,
    FontPitch ePitch = PITCH_DONTKNOW;      // if not found
    rtl_TextEncoding eEnc = osl_getThreadTextEncoding();

    if( !aFace.isEmpty() && !m_pCSS1Parser->IsIgnoreFontFamily() )
    {
        const FontList *pFList = nullptr;
        SwDocShell *pDocSh = m_xDoc->GetDocShell();
        if( pDocSh )
        {
            const SvxFontListItem *pFListItem =
               static_cast<const SvxFontListItem *>(pDocSh->GetItem(SID_ATTR_CHAR_FONTLIST));
            if( pFListItem )
                pFList = pFListItem->GetFontList();
        }

        bool bFound = false;
        sal_Int32 nStrPos = 0;
        while( nStrPos!= -1 )
        {
            OUString aFName = aFace.getToken( 0, ',', nStrPos );
            aFName = comphelper::string::strip(aFName, ' ');
            if( !aFName.isEmpty() )
            {
                if( !bFound && pFList )
                {
                    sal_Handle hFont = pFList->GetFirstFontMetric( aFName );
                    if( nullptr != hFont )
                    {
                        const FontMetric& rFMetric = FontList::GetFontMetric( hFont );
                        if( RTL_TEXTENCODING_DONTKNOW != rFMetric.GetCharSet() )
                        {
                            bFound = true;
                            if( RTL_TEXTENCODING_SYMBOL == rFMetric.GetCharSet() )
                                eEnc = RTL_TEXTENCODING_SYMBOL;
                        }
                    }
                }
                if( !aFontName.isEmpty() )
                    aFontName += ";";
                aFontName += aFName;
            }
        }
    }

    // create a new context
    std::unique_ptr<HTMLAttrContext> xCntxt(new HTMLAttrContext(nToken));

    // parse styles
    if( HasStyleOptions( aStyle, aId, aClass, &aLang, &aDir ) )
    {
        SfxItemSet aItemSet( m_xDoc->GetAttrPool(), m_pCSS1Parser->GetWhichMap() );
        SvxCSS1PropertyInfo aPropInfo;

        if( nFontHeight )
        {
            SvxFontHeightItem aFontHeight( nFontHeight, 100, RES_CHRATR_FONTSIZE );
            aItemSet.Put( aFontHeight );
            SvxFontHeightItem aFontHeightCJK( nFontHeight, 100, RES_CHRATR_CJK_FONTSIZE );
            aItemSet.Put( aFontHeightCJK );
            SvxFontHeightItem aFontHeightCTL( nFontHeight, 100, RES_CHRATR_CTL_FONTSIZE );
            aItemSet.Put( aFontHeightCTL );
        }
        if( bColor )
            aItemSet.Put( SvxColorItem(aColor, RES_CHRATR_COLOR) );
        if( !aFontName.isEmpty() )
        {
            SvxFontItem aFont( eFamily, aFontName, aStyleName, ePitch, eEnc, RES_CHRATR_FONT );
            aItemSet.Put( aFont );
            SvxFontItem aFontCJK( eFamily, aFontName, aStyleName, ePitch, eEnc, RES_CHRATR_CJK_FONT );
            aItemSet.Put( aFontCJK );
            SvxFontItem aFontCTL( eFamily, aFontName, aStyleName, ePitch, eEnc, RES_CHRATR_CTL_FONT );
            aItemSet.Put( aFontCTL );
        }

        if( ParseStyleOptions( aStyle, aId, aClass, aItemSet, aPropInfo, &aLang, &aDir ) )
            DoPositioning( aItemSet, aPropInfo, xCntxt.get() );

        InsertAttrs( aItemSet, aPropInfo, xCntxt.get(), true );
    }
    else
    {
        if( nFontHeight )
        {
            SvxFontHeightItem aFontHeight( nFontHeight, 100, RES_CHRATR_FONTSIZE );
            InsertAttr( &m_xAttrTab->pFontHeight, aFontHeight, xCntxt.get() );
            SvxFontHeightItem aFontHeightCJK( nFontHeight, 100, RES_CHRATR_CJK_FONTSIZE );
            InsertAttr( &m_xAttrTab->pFontHeight, aFontHeightCJK, xCntxt.get() );
            SvxFontHeightItem aFontHeightCTL( nFontHeight, 100, RES_CHRATR_CTL_FONTSIZE );
            InsertAttr( &m_xAttrTab->pFontHeight, aFontHeightCTL, xCntxt.get() );
        }
        if( bColor )
            InsertAttr( &m_xAttrTab->pFontColor, SvxColorItem(aColor, RES_CHRATR_COLOR), xCntxt.get() );
        if( !aFontName.isEmpty() )
        {
            SvxFontItem aFont( eFamily, aFontName, aStyleName, ePitch, eEnc, RES_CHRATR_FONT );
            InsertAttr( &m_xAttrTab->pFont, aFont, xCntxt.get() );
            SvxFontItem aFontCJK( eFamily, aFontName, aStyleName, ePitch, eEnc, RES_CHRATR_CJK_FONT );
            InsertAttr( &m_xAttrTab->pFont, aFontCJK, xCntxt.get() );
            SvxFontItem aFontCTL( eFamily, aFontName, aStyleName, ePitch, eEnc, RES_CHRATR_CTL_FONT );
            InsertAttr( &m_xAttrTab->pFont, aFontCTL, xCntxt.get() );
        }
    }

    // save the context
    PushContext(xCntxt);

    m_aFontStack.push_back( nSize );
}

void SwHTMLParser::EndFontAttr( HtmlTokenId nToken )
{
    EndTag( nToken );

    // avoid stack underflow in tables
    if( m_aFontStack.size() > m_nFontStMin )
        m_aFontStack.erase( m_aFontStack.begin() + m_aFontStack.size() - 1 );
}

void SwHTMLParser::NewPara()
{
    if( m_pPam->GetPoint()->GetContentIndex() )
        AppendTextNode( AM_SPACE );
    else
        AddParSpace();

    m_eParaAdjust = SvxAdjust::End;
    OUString aId, aStyle, aClass, aLang, aDir;

    const HTMLOptions& rHTMLOptions = GetOptions();
    for (size_t i = rHTMLOptions.size(); i; )
    {
        const HTMLOption& rOption = rHTMLOptions[--i];
        switch( rOption.GetToken() )
        {
            case HtmlOptionId::ID:
                aId = rOption.GetString();
                break;
            case HtmlOptionId::ALIGN:
                m_eParaAdjust = rOption.GetEnum( aHTMLPAlignTable, m_eParaAdjust );
                break;
            case HtmlOptionId::STYLE:
                aStyle = rOption.GetString();
                break;
            case HtmlOptionId::CLASS:
                aClass = rOption.GetString();
                break;
            case HtmlOptionId::LANG:
                aLang = rOption.GetString();
                break;
            case HtmlOptionId::DIR:
                aDir = rOption.GetString();
                break;
            case HtmlOptionId::XML_SPACE:
                if (rOption.GetString() == "preserve")
                    SetPreserveSpaces(true);
                break;

            default: break;
        }
    }

    // create a new context
    std::unique_ptr<HTMLAttrContext> xCntxt(
        !aClass.isEmpty() ? new HTMLAttrContext( HtmlTokenId::PARABREAK_ON,
                                             RES_POOLCOLL_TEXT, aClass )
                     : new HTMLAttrContext( HtmlTokenId::PARABREAK_ON ));

    // parse styles (Don't consider class. This is only possible as long as none of
    // the CSS1 properties of the class must be formatted hard!!!)
    if (HasStyleOptions(aStyle, aId, {}, &aLang, &aDir))
    {
        SfxItemSet aItemSet( m_xDoc->GetAttrPool(), m_pCSS1Parser->GetWhichMap() );
        SvxCSS1PropertyInfo aPropInfo;

        if (ParseStyleOptions(aStyle, aId, OUString(), aItemSet, aPropInfo, &aLang, &aDir))
        {
            OSL_ENSURE( aClass.isEmpty() || !m_pCSS1Parser->GetClass( aClass ),
                    "Class is not considered" );
            DoPositioning( aItemSet, aPropInfo, xCntxt.get() );
            InsertAttrs( aItemSet, aPropInfo, xCntxt.get() );

            if (aPropInfo.m_bPreserveSpace)
                SetPreserveSpaces(true);
        }
    }

    if( SvxAdjust::End != m_eParaAdjust )
        InsertAttr( &m_xAttrTab->pAdjust, SvxAdjustItem(m_eParaAdjust, RES_PARATR_ADJUST), xCntxt.get() );

    // and push on stack
    PushContext( xCntxt );

    // set the current style or its attributes
    SetTextCollAttrs( !aClass.isEmpty() ? m_aContexts.back().get() : nullptr );

    // progress bar
    ShowStatline();

    OSL_ENSURE( m_nOpenParaToken == HtmlTokenId::NONE, "Now an open paragraph element will be lost." );
    m_nOpenParaToken = HtmlTokenId::PARABREAK_ON;
}

void SwHTMLParser::EndPara( bool bReal )
{
    if (HtmlTokenId::LI_ON==m_nOpenParaToken && m_xTable)
    {
#if OSL_DEBUG_LEVEL > 0
        const SwNumRule *pNumRule = m_pPam->GetPointNode().GetTextNode()->GetNumRule();
        OSL_ENSURE( pNumRule, "Where is the NumRule" );
#endif
    }

    // Netscape skips empty paragraphs, we do the same; unless in XHTML mode, which prefers mapping
    // the source document to the doc model 1:1 if possible.
    if( bReal )
    {
        if (m_pPam->GetPoint()->GetContentIndex() || m_bXHTML)
            AppendTextNode( AM_SPACE );
        else
            AddParSpace();
    }

    // If a DD or DT was open, it's an implied definition list,
    // which must be closed now.
    if( (m_nOpenParaToken == HtmlTokenId::DT_ON || m_nOpenParaToken == HtmlTokenId::DD_ON) &&
        m_nDefListDeep)
    {
        m_nDefListDeep--;
    }

    // Pop the context of the stack. It can also be from an
    // implied opened definition list.
    std::unique_ptr<HTMLAttrContext> xCntxt(
        PopContext( m_nOpenParaToken != HtmlTokenId::NONE ? getOnToken(m_nOpenParaToken) : HtmlTokenId::PARABREAK_ON ));

    // close attribute
    if (xCntxt)
    {
        EndContext(xCntxt.get());
        SetAttr();  // because of JavaScript set paragraph attributes as fast as possible
        xCntxt.reset();
    }

    // reset the existing style
    if( bReal )
        SetTextCollAttrs();

    m_nOpenParaToken = HtmlTokenId::NONE;
    SetPreserveSpaces(false);
}

void SwHTMLParser::NewHeading( HtmlTokenId nToken )
{
    m_eParaAdjust = SvxAdjust::End;

    OUString aId, aStyle, aClass, aLang, aDir;

    const HTMLOptions& rHTMLOptions = GetOptions();
    for (size_t i = rHTMLOptions.size(); i; )
    {
        const HTMLOption& rOption = rHTMLOptions[--i];
        switch( rOption.GetToken() )
        {
            case HtmlOptionId::ID:
                aId = rOption.GetString();
                break;
            case HtmlOptionId::ALIGN:
                m_eParaAdjust = rOption.GetEnum( aHTMLPAlignTable, m_eParaAdjust );
                break;
            case HtmlOptionId::STYLE:
                aStyle = rOption.GetString();
                break;
            case HtmlOptionId::CLASS:
                aClass = rOption.GetString();
                break;
            case HtmlOptionId::LANG:
                aLang = rOption.GetString();
                break;
            case HtmlOptionId::DIR:
                aDir = rOption.GetString();
                break;
            default: break;
        }
    }

    // open a new paragraph
    if( m_pPam->GetPoint()->GetContentIndex() )
        AppendTextNode( AM_SPACE );
    else
        AddParSpace();

    // search for the matching style
    sal_uInt16 nTextColl;
    switch( nToken )
    {
    case HtmlTokenId::HEAD1_ON:         nTextColl = RES_POOLCOLL_HEADLINE1;  break;
    case HtmlTokenId::HEAD2_ON:         nTextColl = RES_POOLCOLL_HEADLINE2;  break;
    case HtmlTokenId::HEAD3_ON:         nTextColl = RES_POOLCOLL_HEADLINE3;  break;
    case HtmlTokenId::HEAD4_ON:         nTextColl = RES_POOLCOLL_HEADLINE4;  break;
    case HtmlTokenId::HEAD5_ON:         nTextColl = RES_POOLCOLL_HEADLINE5;  break;
    case HtmlTokenId::HEAD6_ON:         nTextColl = RES_POOLCOLL_HEADLINE6;  break;
    default:                    nTextColl = RES_POOLCOLL_STANDARD;   break;
    }

    // create the context
    std::unique_ptr<HTMLAttrContext> xCntxt(new HTMLAttrContext(nToken, nTextColl, aClass));

    // parse styles (regarding class see also NewPara)
    if (HasStyleOptions(aStyle, aId, {}, &aLang, &aDir))
    {
        SfxItemSet aItemSet( m_xDoc->GetAttrPool(), m_pCSS1Parser->GetWhichMap() );
        SvxCSS1PropertyInfo aPropInfo;

        if (ParseStyleOptions(aStyle, aId, OUString(), aItemSet, aPropInfo, &aLang, &aDir))
        {
            OSL_ENSURE( aClass.isEmpty() || !m_pCSS1Parser->GetClass( aClass ),
                    "Class is not considered" );
            DoPositioning( aItemSet, aPropInfo, xCntxt.get() );
            InsertAttrs( aItemSet, aPropInfo, xCntxt.get() );
        }
    }

    if( SvxAdjust::End != m_eParaAdjust )
        InsertAttr( &m_xAttrTab->pAdjust, SvxAdjustItem(m_eParaAdjust, RES_PARATR_ADJUST), xCntxt.get() );

    // and push on stack
    PushContext(xCntxt);

    // set the current style or its attributes
    SetTextCollAttrs(m_aContexts.back().get());

    m_nFontStHeadStart = m_aFontStack.size();

    // progress bar
    ShowStatline();
}

void SwHTMLParser::EndHeading()
{
    // open a new paragraph
    if( m_pPam->GetPoint()->GetContentIndex() )
        AppendTextNode( AM_SPACE );
    else
        AddParSpace();

    // search context matching the token and fetch it from stack
    std::unique_ptr<HTMLAttrContext> xCntxt;
    auto nPos = m_aContexts.size();
    while( !xCntxt && nPos>m_nContextStMin )
    {
        switch( m_aContexts[--nPos]->GetToken() )
        {
        case HtmlTokenId::HEAD1_ON:
        case HtmlTokenId::HEAD2_ON:
        case HtmlTokenId::HEAD3_ON:
        case HtmlTokenId::HEAD4_ON:
        case HtmlTokenId::HEAD5_ON:
        case HtmlTokenId::HEAD6_ON:
            xCntxt = std::move(m_aContexts[nPos]);
            m_aContexts.erase( m_aContexts.begin() + nPos );
            break;
        default: break;
        }
    }

    // and now end attributes
    if (xCntxt)
    {
        EndContext(xCntxt.get());
        SetAttr();  // because of JavaScript set paragraph attributes as fast as possible
        xCntxt.reset();
    }

    // reset existing style
    SetTextCollAttrs();

    m_nFontStHeadStart = m_nFontStMin;
}

void SwHTMLParser::NewTextFormatColl( HtmlTokenId nToken, sal_uInt16 nColl )
{
    OUString aId, aStyle, aClass, aLang, aDir;

    const HTMLOptions& rHTMLOptions = GetOptions();
    for (size_t i = rHTMLOptions.size(); i; )
    {
        const HTMLOption& rOption = rHTMLOptions[--i];
        switch( rOption.GetToken() )
        {
            case HtmlOptionId::ID:
                aId = rOption.GetString();
                break;
            case HtmlOptionId::STYLE:
                aStyle = rOption.GetString();
                break;
            case HtmlOptionId::CLASS:
                aClass = rOption.GetString();
                break;
            case HtmlOptionId::LANG:
                aLang = rOption.GetString();
                break;
            case HtmlOptionId::DIR:
                aDir = rOption.GetString();
                break;
            default: break;
        }
    }

    // open a new paragraph
    SwHTMLAppendMode eMode = AM_NORMAL;
    switch( nToken )
    {
    case HtmlTokenId::LISTING_ON:
    case HtmlTokenId::XMP_ON:
        // These both tags will be mapped to the PRE style. For the case that a
        // a CLASS exists we will delete it so that we don't get the CLASS of
        // the PRE style.
        aClass.clear();
        [[fallthrough]];
    case HtmlTokenId::BLOCKQUOTE_ON:
    case HtmlTokenId::BLOCKQUOTE30_ON:
    case HtmlTokenId::PREFORMTXT_ON:
        eMode = AM_SPACE;
        break;
    case HtmlTokenId::ADDRESS_ON:
        eMode = AM_NOSPACE; // ADDRESS can follow on a <P> without </P>
        break;
    case HtmlTokenId::DT_ON:
    case HtmlTokenId::DD_ON:
        eMode = AM_SOFTNOSPACE;
        break;
    default:
        OSL_ENSURE( false, "unknown style" );
        break;
    }
    if( m_pPam->GetPoint()->GetContentIndex() )
        AppendTextNode( eMode );
    else if( AM_SPACE==eMode )
        AddParSpace();

    // ... and save in a context
    std::unique_ptr<HTMLAttrContext> xCntxt(new HTMLAttrContext(nToken, nColl, aClass));

    // parse styles (regarding class see also NewPara)
    if (HasStyleOptions(aStyle, aId, {}, &aLang, &aDir))
    {
        SfxItemSet aItemSet( m_xDoc->GetAttrPool(), m_pCSS1Parser->GetWhichMap() );
        SvxCSS1PropertyInfo aPropInfo;

        if (ParseStyleOptions(aStyle, aId, OUString(), aItemSet, aPropInfo, &aLang, &aDir))
        {
            OSL_ENSURE( aClass.isEmpty() || !m_pCSS1Parser->GetClass( aClass ),
                    "Class is not considered" );
            DoPositioning( aItemSet, aPropInfo, xCntxt.get() );
            InsertAttrs( aItemSet, aPropInfo, xCntxt.get() );
        }
    }

    PushContext(xCntxt);

    // set the new style
    SetTextCollAttrs(m_aContexts.back().get());

    // update progress bar
    ShowStatline();
}

void SwHTMLParser::EndTextFormatColl( HtmlTokenId nToken )
{
    SwHTMLAppendMode eMode = AM_NORMAL;
    switch( getOnToken(nToken) )
    {
    case HtmlTokenId::BLOCKQUOTE_ON:
    case HtmlTokenId::BLOCKQUOTE30_ON:
    case HtmlTokenId::PREFORMTXT_ON:
    case HtmlTokenId::LISTING_ON:
    case HtmlTokenId::XMP_ON:
        eMode = AM_SPACE;
        break;
    case HtmlTokenId::ADDRESS_ON:
    case HtmlTokenId::DT_ON:
    case HtmlTokenId::DD_ON:
        eMode = AM_SOFTNOSPACE;
        break;
    default:
        OSL_ENSURE( false, "unknown style" );
        break;
    }
    if( m_pPam->GetPoint()->GetContentIndex() )
        AppendTextNode( eMode );
    else if( AM_SPACE==eMode )
        AddParSpace();

    // pop current context of stack
    std::unique_ptr<HTMLAttrContext> xCntxt(PopContext(getOnToken(nToken)));

    // and now end attributes
    if (xCntxt)
    {
        EndContext(xCntxt.get());
        SetAttr();  // because of JavaScript set paragraph attributes as fast as possible
        xCntxt.reset();
    }

    // reset existing style
    SetTextCollAttrs();
}

void SwHTMLParser::NewDefList()
{
    OUString aId, aStyle, aClass, aLang, aDir;

    const HTMLOptions& rHTMLOptions = GetOptions();
    for (size_t i = rHTMLOptions.size(); i; )
    {
        const HTMLOption& rOption = rHTMLOptions[--i];
        switch( rOption.GetToken() )
        {
            case HtmlOptionId::ID:
                aId = rOption.GetString();
                break;
            case HtmlOptionId::STYLE:
                aStyle = rOption.GetString();
                break;
            case HtmlOptionId::CLASS:
                aClass = rOption.GetString();
                break;
            case HtmlOptionId::LANG:
                aLang = rOption.GetString();
                break;
            case HtmlOptionId::DIR:
                aDir = rOption.GetString();
                break;
            default: break;
        }
    }

    // open a new paragraph
    bool bSpace = (GetNumInfo().GetDepth() + m_nDefListDeep) == 0;
    if( m_pPam->GetPoint()->GetContentIndex() )
        AppendTextNode( bSpace ? AM_SPACE : AM_SOFTNOSPACE );
    else if( bSpace )
        AddParSpace();

    // one level more
    m_nDefListDeep++;

    bool bInDD = false, bNotInDD = false;
    auto nPos = m_aContexts.size();
    while( !bInDD && !bNotInDD && nPos>m_nContextStMin )
    {
        HtmlTokenId nCntxtToken = m_aContexts[--nPos]->GetToken();
        switch( nCntxtToken )
        {
        case HtmlTokenId::DEFLIST_ON:
        case HtmlTokenId::DIRLIST_ON:
        case HtmlTokenId::MENULIST_ON:
        case HtmlTokenId::ORDERLIST_ON:
        case HtmlTokenId::UNORDERLIST_ON:
            bNotInDD = true;
            break;
        case HtmlTokenId::DD_ON:
            bInDD = true;
            break;
        default: break;
        }
    }

    // ... and save in a context
    std::unique_ptr<HTMLAttrContext> xCntxt(new HTMLAttrContext(HtmlTokenId::DEFLIST_ON));

    // in it save also the margins
    sal_uInt16 nLeft=0, nRight=0;
    short nIndent=0;
    GetMarginsFromContext( nLeft, nRight, nIndent );

    // The indentation, which already results from a DL, correlates with a DT
    // on the current level and this correlates to a DD from the previous level.
    // For a level >=2 we must add DD distance.
    if( !bInDD && m_nDefListDeep > 1 )
    {

        // and the one of the DT-style of the current level
        SvxTextLeftMarginItem const& rTextLeftMargin =
            m_pCSS1Parser->GetTextFormatColl(RES_POOLCOLL_HTML_DD, OUString())
                       ->GetTextLeftMargin();
        nLeft = nLeft + static_cast<sal_uInt16>(rTextLeftMargin.ResolveTextLeft({}));
    }

    xCntxt->SetMargins( nLeft, nRight, nIndent );

    // parse styles
    if( HasStyleOptions( aStyle, aId, aClass, &aLang, &aDir ) )
    {
        SfxItemSet aItemSet( m_xDoc->GetAttrPool(), m_pCSS1Parser->GetWhichMap() );
        SvxCSS1PropertyInfo aPropInfo;

        if( ParseStyleOptions( aStyle, aId, aClass, aItemSet, aPropInfo, &aLang, &aDir ) )
        {
            DoPositioning( aItemSet, aPropInfo, xCntxt.get() );
            InsertAttrs( aItemSet, aPropInfo, xCntxt.get() );
        }
    }

    PushContext(xCntxt);

    // set the attributes of the new style
    if( m_nDefListDeep > 1 )
        SetTextCollAttrs(m_aContexts.back().get());
}

void SwHTMLParser::EndDefList()
{
    bool bSpace = (GetNumInfo().GetDepth() + m_nDefListDeep) == 1;
    if( m_pPam->GetPoint()->GetContentIndex() )
        AppendTextNode( bSpace ? AM_SPACE : AM_SOFTNOSPACE );
    else if( bSpace )
        AddParSpace();

    // one level less
    if( m_nDefListDeep > 0 )
        m_nDefListDeep--;

    // pop current context of stack
    std::unique_ptr<HTMLAttrContext> xCntxt(PopContext(HtmlTokenId::DEFLIST_ON));

    // and now end attributes
    if (xCntxt)
    {
        EndContext(xCntxt.get());
        SetAttr();  // because of JavaScript set paragraph attributes as fast as possible
        xCntxt.reset();
    }

    // and set style
    SetTextCollAttrs();
}

void SwHTMLParser::NewDefListItem( HtmlTokenId nToken )
{
    // determine if the DD/DT exist in a DL
    bool bInDefList = false, bNotInDefList = false;
    auto nPos = m_aContexts.size();
    while( !bInDefList && !bNotInDefList && nPos>m_nContextStMin )
    {
        HtmlTokenId nCntxtToken = m_aContexts[--nPos]->GetToken();
        switch( nCntxtToken )
        {
        case HtmlTokenId::DEFLIST_ON:
            bInDefList = true;
            break;
        case HtmlTokenId::DIRLIST_ON:
        case HtmlTokenId::MENULIST_ON:
        case HtmlTokenId::ORDERLIST_ON:
        case HtmlTokenId::UNORDERLIST_ON:
            bNotInDefList = true;
            break;
        default: break;
        }
    }

    // if not, then implicitly open a new DL
    if( !bInDefList )
    {
        m_nDefListDeep++;
        OSL_ENSURE( m_nOpenParaToken == HtmlTokenId::NONE,
                "Now an open paragraph element will be lost." );
        m_nOpenParaToken = nToken;
    }

    NewTextFormatColl( nToken, static_cast< sal_uInt16 >(nToken==HtmlTokenId::DD_ON ? RES_POOLCOLL_HTML_DD
                                              : RES_POOLCOLL_HTML_DT) );
}

void SwHTMLParser::EndDefListItem( HtmlTokenId nToken )
{
    // open a new paragraph
    if( nToken == HtmlTokenId::NONE && m_pPam->GetPoint()->GetContentIndex() )
        AppendTextNode( AM_SOFTNOSPACE );

    // search context matching the token and fetch it from stack
    nToken = getOnToken(nToken);
    std::unique_ptr<HTMLAttrContext> xCntxt;
    auto nPos = m_aContexts.size();
    while( !xCntxt && nPos>m_nContextStMin )
    {
        HtmlTokenId nCntxtToken = m_aContexts[--nPos]->GetToken();
        switch( nCntxtToken )
        {
        case HtmlTokenId::DD_ON:
        case HtmlTokenId::DT_ON:
            if( nToken == HtmlTokenId::NONE || nToken == nCntxtToken  )
            {
                xCntxt = std::move(m_aContexts[nPos]);
                m_aContexts.erase( m_aContexts.begin() + nPos );
            }
            break;
        case HtmlTokenId::DEFLIST_ON:
            // don't look at DD/DT outside the current DefList
        case HtmlTokenId::DIRLIST_ON:
        case HtmlTokenId::MENULIST_ON:
        case HtmlTokenId::ORDERLIST_ON:
        case HtmlTokenId::UNORDERLIST_ON:
            // and also not outside another list
            nPos = m_nContextStMin;
            break;
        default: break;
        }
    }

    // and now end attributes
    if (xCntxt)
    {
        EndContext(xCntxt.get());
        SetAttr();  // because of JavaScript set paragraph attributes as fast as possible
    }
}

/**
 *
 * @param bNoSurroundOnly   The paragraph contains at least one frame
 *                          without wrapping.
 * @param bSurroundOnly     The paragraph contains at least one frame
 *                          with wrapping, but none without wrapping.
 *
 *                          Otherwise the paragraph contains any frame.
 */
bool SwHTMLParser::HasCurrentParaFlys( bool bNoSurroundOnly,
                                       bool bSurroundOnly ) const
{
    SwNode& rNode = m_pPam->GetPoint()->GetNode();


    bool bFound = false;
    for(sw::SpzFrameFormat* pFormat: *m_xDoc->GetSpzFrameFormats())
    {
        SwFormatAnchor const*const pAnchor = &pFormat->GetAnchor();
        // A frame was found, when
        // - it is paragraph-bound, and
        // - is anchored in current paragraph, and
        //   - every paragraph-bound frame counts, or
        //   - (only frames without wrapping count and) the frame doesn't have
        //     a wrapping
        SwNode const*const pAnchorNode = pAnchor->GetAnchorNode();
        if (pAnchorNode &&
            ((RndStdIds::FLY_AT_PARA == pAnchor->GetAnchorId()) ||
             (RndStdIds::FLY_AT_CHAR == pAnchor->GetAnchorId())) &&
            *pAnchorNode == rNode )
        {
            if( !(bNoSurroundOnly || bSurroundOnly) )
            {
                bFound = true;
                break;
            }
            else
            {
                // When looking for frames with wrapping, also disregard
                // ones with wrap-through. In this case it's (still) HIDDEN-Controls,
                // and you don't want to evade those when positioning.
                css::text::WrapTextMode eSurround = pFormat->GetSurround().GetSurround();
                if( bNoSurroundOnly )
                {
                    if( css::text::WrapTextMode_NONE==eSurround )
                    {
                        bFound = true;
                        break;
                    }
                }
                if( bSurroundOnly )
                {
                    if( css::text::WrapTextMode_NONE==eSurround )
                    {
                        bFound = false;
                        break;
                    }
                    else if( css::text::WrapTextMode_THROUGH!=eSurround )
                    {
                        bFound = true;
                        // Continue searching: It's possible that some without
                        // wrapping will follow...
                    }
                }
            }
        }
    }

    return bFound;
}

// the special methods for inserting of objects

const SwFormatColl *SwHTMLParser::GetCurrFormatColl() const
{
    const SwContentNode* pCNd = m_pPam->GetPointContentNode();
    return pCNd ? &pCNd->GetAnyFormatColl() : nullptr;
}

void SwHTMLParser::SetTextCollAttrs( HTMLAttrContext *pContext )
{
    SwTextFormatColl *pCollToSet = nullptr; // the style to set
    SfxItemSet *pItemSet = nullptr;         // set of hard attributes
    sal_uInt16 nTopColl = pContext ? pContext->GetTextFormatColl() : 0;
    const OUString rTopClass = pContext ? pContext->GetClass() : OUString();
    sal_uInt16 nDfltColl = RES_POOLCOLL_TEXT;

    bool bInPRE=false;                          // some context info

    sal_uInt16 nLeftMargin = 0, nRightMargin = 0;   // the margins and
    short nFirstLineIndent = 0;                     // indentations

    auto nDepth = m_aContexts.size();
    if (bFuzzing && nDepth > 128)
    {
        SAL_WARN("sw.html", "Not applying any more text collection attributes to a deeply nested node for fuzzing performance");
        nDepth = 0;
    }

    for (auto i = m_nContextStAttrMin; i < nDepth; ++i)
    {
        const HTMLAttrContext *pCntxt = m_aContexts[i].get();

        sal_uInt16 nColl = pCntxt->GetTextFormatColl();
        if( nColl )
        {
            // There is a style to set. Then at first we must decide,
            // if the style can be set.
            bool bSetThis = true;
            switch( nColl )
            {
            case RES_POOLCOLL_HTML_PRE:
                bInPRE = true;
                break;
            case RES_POOLCOLL_TEXT:
                // <TD><P CLASS=xxx> must become TD.xxx
                if( nDfltColl==RES_POOLCOLL_TABLE ||
                    nDfltColl==RES_POOLCOLL_TABLE_HDLN )
                    nColl = nDfltColl;
                break;
            case RES_POOLCOLL_HTML_HR:
                // also <HR> in <PRE> set as style, otherwise it can't
                // be exported anymore
                break;
            default:
                if( bInPRE )
                    bSetThis = false;
                break;
            }

            SwTextFormatColl *pNewColl =
                m_pCSS1Parser->GetTextFormatColl( nColl, pCntxt->GetClass() );

            if( bSetThis )
            {
                // If now a different style should be set as previously, the
                // previous style must be replaced by hard attribution.

                if( pCollToSet )
                {
                    // insert the attributes hard, which previous style sets
                    if( !pItemSet )
                        pItemSet = new SfxItemSet( pCollToSet->GetAttrSet() );
                    else
                    {
                        const SfxItemSet& rCollSet = pCollToSet->GetAttrSet();
                        SfxItemSet aItemSet( *rCollSet.GetPool(),
                                             rCollSet.GetRanges() );
                        aItemSet.Set( rCollSet );
                        pItemSet->Put( aItemSet );
                    }
                    // but remove the attributes, which the current style sets,
                    // because otherwise they will be overwritten later
                    pItemSet->Differentiate( pNewColl->GetAttrSet() );
                }

                pCollToSet = pNewColl;
            }
            else
            {
                // hard attribution
                if( !pItemSet )
                    pItemSet = new SfxItemSet( pNewColl->GetAttrSet() );
                else
                {
                    const SfxItemSet& rCollSet = pNewColl->GetAttrSet();
                    SfxItemSet aItemSet( *rCollSet.GetPool(),
                                         rCollSet.GetRanges() );
                    aItemSet.Set( rCollSet );
                    pItemSet->Put( aItemSet );
                }
            }
        }
        else
        {
            // Maybe a default style exists?
            nColl = pCntxt->GetDefaultTextFormatColl();
            if( nColl )
                nDfltColl = nColl;
        }

        // if applicable fetch new paragraph indents
        if( pCntxt->IsLRSpaceChanged() )
        {
            sal_uInt16 nLeft=0, nRight=0;

            pCntxt->GetMargins( nLeft, nRight, nFirstLineIndent );
            nLeftMargin = nLeft;
            nRightMargin = nRight;
        }
    }

    // If in current context a new style should be set,
    // its paragraph margins must be inserted in the context.
    if( pContext && nTopColl )
    {
        // <TD><P CLASS=xxx> must become TD.xxx
        if( nTopColl==RES_POOLCOLL_TEXT &&
            (nDfltColl==RES_POOLCOLL_TABLE ||
             nDfltColl==RES_POOLCOLL_TABLE_HDLN) )
            nTopColl = nDfltColl;

        const SwTextFormatColl *pTopColl =
            m_pCSS1Parser->GetTextFormatColl( nTopColl, rTopClass );
        const SfxItemSet& rItemSet = pTopColl->GetAttrSet();
        if (rItemSet.GetItemIfSet(RES_MARGIN_FIRSTLINE)
            || rItemSet.GetItemIfSet(RES_MARGIN_TEXTLEFT)
            || rItemSet.GetItemIfSet(RES_MARGIN_RIGHT))
        {
            sal_Int32 nLeft = rItemSet.Get(RES_MARGIN_TEXTLEFT).ResolveTextLeft({});
            sal_Int32 nRight = rItemSet.Get(RES_MARGIN_RIGHT).ResolveRight({});
            nFirstLineIndent = rItemSet.Get(RES_MARGIN_FIRSTLINE).ResolveTextFirstLineOffset({});

            // In Definition lists the margins also contain the margins from the previous levels
            if( RES_POOLCOLL_HTML_DD == nTopColl )
            {
                auto const*const pColl(m_pCSS1Parser->GetTextFormatColl(RES_POOLCOLL_HTML_DT, OUString()));
                nLeft -= pColl->GetTextLeftMargin().ResolveTextLeft({});
                nRight -= pColl->GetRightMargin().ResolveRight({});
            }
            else if( RES_POOLCOLL_HTML_DT == nTopColl )
            {
                nLeft = 0;
                nRight = 0;
            }

            // the paragraph margins add up
            nLeftMargin = nLeftMargin + static_cast< sal_uInt16 >(nLeft);
            nRightMargin = nRightMargin + static_cast< sal_uInt16 >(nRight);

            pContext->SetMargins( nLeftMargin, nRightMargin,
                                  nFirstLineIndent );
        }
        if( const SvxULSpaceItem* pULItem = rItemSet.GetItemIfSet(RES_UL_SPACE) )
        {
            pContext->SetULSpace( pULItem->GetUpper(), pULItem->GetLower() );
        }
    }

    // If no style is set in the context use the text body.
    if( !pCollToSet )
    {
        pCollToSet = m_pCSS1Parser->GetTextCollFromPool( nDfltColl );
        if( !nLeftMargin )
        {
            nLeftMargin
                = static_cast<sal_uInt16>(pCollToSet->GetTextLeftMargin().ResolveTextLeft({}));
        }
        if( !nRightMargin )
        {
            nRightMargin = static_cast<sal_uInt16>(pCollToSet->GetRightMargin().ResolveRight({}));
        }
        if( !nFirstLineIndent )
        {
            nFirstLineIndent = pCollToSet->GetFirstLineIndent().ResolveTextFirstLineOffset({});
        }
    }

    // remove previous hard attribution of paragraph
    for( auto pParaAttr : m_aParaAttrs )
        pParaAttr->Invalidate();
    m_aParaAttrs.clear();

    // set the style
    m_xDoc->SetTextFormatColl( *m_pPam, pCollToSet );

    if (m_pTargetCharAttrs)
    {
        std::unique_ptr<SfxItemSet> const pCharSet(new SfxItemSet(*m_pTargetCharAttrs));
        // do not use SfxItemSet::Differentiate(), it doesn't compare values!
        std::vector<sal_uInt16> clear;
        for (SfxItemIter it{*pCharSet}; !it.IsAtEnd(); it.NextItem())
        {
            SfxPoolItem const& rItem{*it.GetCurItem()};
            if (SfxPoolItem const*const pItem{pCollToSet->GetAttrSet().GetItem(rItem.Which(), true)})
            {
                if (rItem == *pItem)
                {
                    clear.emplace_back(rItem.Which());
                }
            }
        }
        for (auto const it : clear)
        {
            pCharSet->ClearItem(it);
        }
        m_xDoc->getIDocumentContentOperations().InsertItemSet(*m_pPam, *pCharSet);
    }

    // if applicable correct the paragraph indent
    const SvxFirstLineIndentItem & rFirstLine = pCollToSet->GetFirstLineIndent();
    const SvxTextLeftMarginItem & rTextLeftMargin = pCollToSet->GetTextLeftMargin();
    const SvxRightMarginItem & rRightMargin = pCollToSet->GetRightMargin();
    bool bSetLRSpace = nLeftMargin != rTextLeftMargin.ResolveTextLeft({})
                       || nFirstLineIndent != rFirstLine.ResolveTextFirstLineOffset({})
                       || nRightMargin != rRightMargin.ResolveRight({});

    if( bSetLRSpace )
    {
        SvxFirstLineIndentItem firstLine(rFirstLine);
        SvxTextLeftMarginItem leftMargin(rTextLeftMargin);
        SvxRightMarginItem rightMargin(rRightMargin);
        firstLine.SetTextFirstLineOffset(SvxIndentValue::twips(nFirstLineIndent));
        leftMargin.SetTextLeft(SvxIndentValue::twips(nLeftMargin));
        rightMargin.SetRight(SvxIndentValue::twips(nRightMargin));
        if( pItemSet )
        {
            pItemSet->Put(firstLine);
            pItemSet->Put(leftMargin);
            pItemSet->Put(rightMargin);
        }
        else
        {
            NewAttr(m_xAttrTab, &m_xAttrTab->pFirstLineIndent, firstLine);
            m_xAttrTab->pFirstLineIndent->SetLikePara();
            m_aParaAttrs.push_back(m_xAttrTab->pFirstLineIndent);
            EndAttr(m_xAttrTab->pFirstLineIndent, false);
            NewAttr(m_xAttrTab, &m_xAttrTab->pTextLeftMargin, leftMargin);
            m_xAttrTab->pTextLeftMargin->SetLikePara();
            m_aParaAttrs.push_back(m_xAttrTab->pTextLeftMargin);
            EndAttr(m_xAttrTab->pTextLeftMargin, false);
            NewAttr(m_xAttrTab, &m_xAttrTab->pRightMargin, rightMargin);
            m_xAttrTab->pRightMargin->SetLikePara();
            m_aParaAttrs.push_back(m_xAttrTab->pRightMargin);
            EndAttr(m_xAttrTab->pRightMargin, false);
        }
    }

    // and now set the attributes
    if( pItemSet )
    {
        InsertParaAttrs( *pItemSet );
        delete pItemSet;
    }
}

void SwHTMLParser::NewCharFormat( HtmlTokenId nToken )
{
    OUString aId, aStyle, aLang, aDir;
    OUString aClass;

    const HTMLOptions& rHTMLOptions = GetOptions();
    for (size_t i = rHTMLOptions.size(); i; )
    {
        const HTMLOption& rOption = rHTMLOptions[--i];
        switch( rOption.GetToken() )
        {
        case HtmlOptionId::ID:
            aId = rOption.GetString();
            break;
        case HtmlOptionId::STYLE:
            aStyle = rOption.GetString();
            break;
        case HtmlOptionId::CLASS:
            aClass = rOption.GetString();
            break;
        case HtmlOptionId::LANG:
            aLang = rOption.GetString();
            break;
        case HtmlOptionId::DIR:
            aDir = rOption.GetString();
            break;
        default: break;
        }
    }

    // create a new context
    std::unique_ptr<HTMLAttrContext> xCntxt(new HTMLAttrContext(nToken));

    // set the style and save it in the context
    SwCharFormat* pCFormat = m_pCSS1Parser->GetChrFormat( nToken, aClass );
    OSL_ENSURE( pCFormat, "No character format found for token" );

    // parse styles (regarding class see also NewPara)
    if (HasStyleOptions(aStyle, aId, {}, &aLang, &aDir))
    {
        SfxItemSet aItemSet( m_xDoc->GetAttrPool(), m_pCSS1Parser->GetWhichMap() );
        SvxCSS1PropertyInfo aPropInfo;

        if (ParseStyleOptions(aStyle, aId, OUString(), aItemSet, aPropInfo, &aLang, &aDir))
        {
            OSL_ENSURE( aClass.isEmpty() || !m_pCSS1Parser->GetClass( aClass ),
                    "Class is not considered" );
            DoPositioning( aItemSet, aPropInfo, xCntxt.get() );
            InsertAttrs( aItemSet, aPropInfo, xCntxt.get(), true );
        }
    }

    // Character formats are stored in their own stack and can never be inserted
    // by styles. Therefore the attribute doesn't exist in CSS1-Which-Range.
    if( pCFormat )
        InsertAttr( &m_xAttrTab->pCharFormats, SwFormatCharFormat( pCFormat ), xCntxt.get() );

    // save the context
    PushContext(xCntxt);
}

void SwHTMLParser::InsertSpacer()
{
    // and if applicable change it via the options
    sal_Int16 eVertOri = text::VertOrientation::TOP;
    sal_Int16 eHoriOri = text::HoriOrientation::NONE;
    Size aSize( 0, 0);
    tools::Long nSize = 0;
    bool bPercentWidth = false;
    bool bPercentHeight = false;
    sal_uInt16 nType = HTML_SPTYPE_HORI;

    const HTMLOptions& rHTMLOptions = GetOptions();
    for (size_t i = rHTMLOptions.size(); i; )
    {
        const HTMLOption& rOption = rHTMLOptions[--i];
        switch( rOption.GetToken() )
        {
        case HtmlOptionId::TYPE:
            rOption.GetEnum( nType, aHTMLSpacerTypeTable );
            break;
        case HtmlOptionId::ALIGN:
            eVertOri =
                rOption.GetEnum( aHTMLImgVAlignTable,
                                  eVertOri );
            eHoriOri =
                rOption.GetEnum( aHTMLImgHAlignTable,
                                  eHoriOri );
            break;
        case HtmlOptionId::WIDTH:
            // First only save as pixel value!
            bPercentWidth = (rOption.GetString().indexOf('%') != -1);
            aSize.setWidth( static_cast<tools::Long>(rOption.GetNumber()) );
            break;
        case HtmlOptionId::HEIGHT:
            // First only save as pixel value!
            bPercentHeight = (rOption.GetString().indexOf('%') != -1);
            aSize.setHeight( static_cast<tools::Long>(rOption.GetNumber()) );
            break;
        case HtmlOptionId::SIZE:
            // First only save as pixel value!
            nSize = rOption.GetNumber();
            break;
        default: break;
        }
    }

    switch( nType )
    {
    case HTML_SPTYPE_BLOCK:
        {
            // create an empty text frame

            // fetch the ItemSet
            SfxItemSetFixed<RES_FRMATR_BEGIN, RES_FRMATR_END-1> aFrameSet( m_xDoc->GetAttrPool() );
            if( !IsNewDoc() )
                Reader::ResetFrameFormatAttrs( aFrameSet );

            // set the anchor and the adjustment
            SetAnchorAndAdjustment( eVertOri, eHoriOri, aFrameSet );

            // and the size of the frame
            Size aDfltSz( MINFLY, MINFLY );
            Size aSpace( 0, 0 );
            SfxItemSet aDummyItemSet( m_xDoc->GetAttrPool(),
                                 m_pCSS1Parser->GetWhichMap() );
            SvxCSS1PropertyInfo aDummyPropInfo;

            SetFixSize( aSize, aDfltSz, bPercentWidth, bPercentHeight,
                        aDummyPropInfo, aFrameSet );
            SetSpace( aSpace, aDummyItemSet, aDummyPropInfo, aFrameSet );

            // protect the content
            SvxProtectItem aProtectItem( RES_PROTECT) ;
            aProtectItem.SetContentProtect( true );
            aFrameSet.Put( aProtectItem );

            // create the frame
            RndStdIds eAnchorId =
                aFrameSet.Get(RES_ANCHOR).GetAnchorId();
            SwFrameFormat *pFlyFormat = m_xDoc->MakeFlySection( eAnchorId,
                                            m_pPam->GetPoint(), &aFrameSet );
            // Possibly create frames and register auto-bound frames.
            RegisterFlyFrame( pFlyFormat );
        }
        break;
    case HTML_SPTYPE_VERT:
        if( nSize > 0 )
        {
            nSize = o3tl::convert(nSize, o3tl::Length::px, o3tl::Length::twip);

            // set a paragraph margin
            SwTextNode *pTextNode = nullptr;
            if( !m_pPam->GetPoint()->GetContentIndex() )
            {
                // if possible change the bottom paragraph margin
                // of previous node

                SetAttr();  // set still open paragraph attributes

                pTextNode = m_xDoc->GetNodes()[m_pPam->GetPoint()->GetNodeIndex()-1]
                               ->GetTextNode();

                // If the previous paragraph isn't a text node, then now an
                // empty paragraph is created, which already generates a single
                // line of spacing.
                if( !pTextNode )
                    nSize = nSize>HTML_PARSPACE ? nSize-HTML_PARSPACE : 0;
            }

            if( pTextNode )
            {
                SvxULSpaceItem aULSpace( pTextNode->SwContentNode::GetAttr( RES_UL_SPACE ) );
                aULSpace.SetLower( aULSpace.GetLower() + o3tl::narrowing<sal_uInt16>(nSize) );
                pTextNode->SetAttr( aULSpace );
            }
            else
            {
                NewAttr(m_xAttrTab, &m_xAttrTab->pULSpace, SvxULSpaceItem(0, o3tl::narrowing<sal_uInt16>(nSize), RES_UL_SPACE));
                EndAttr( m_xAttrTab->pULSpace, false );

                AppendTextNode();    // Don't change spacing!
            }
        }
        break;
    case HTML_SPTYPE_HORI:
        if( nSize > 0 )
        {
            // If the paragraph is still empty, set first line
            // indentation, otherwise apply letter spacing over a space.

            nSize = o3tl::convert(nSize, o3tl::Length::px, o3tl::Length::twip);

            if( !m_pPam->GetPoint()->GetContentIndex() )
            {
                sal_uInt16 nLeft=0, nRight=0;
                short nIndent = 0;

                GetMarginsFromContextWithNumberBullet( nLeft, nRight, nIndent );
                nIndent = nIndent + static_cast<short>(nSize);

                SvxFirstLineIndentItem const firstLine(SvxIndentValue::twips(nIndent),
                                                       RES_MARGIN_FIRSTLINE);
                SvxTextLeftMarginItem const leftMargin(SvxIndentValue::twips(nLeft),
                                                       RES_MARGIN_TEXTLEFT);
                SvxRightMarginItem const rightMargin(SvxIndentValue::twips(nRight),
                                                     RES_MARGIN_RIGHT);

                NewAttr(m_xAttrTab, &m_xAttrTab->pFirstLineIndent, firstLine);
                EndAttr(m_xAttrTab->pFirstLineIndent, false);
                NewAttr(m_xAttrTab, &m_xAttrTab->pTextLeftMargin, leftMargin);
                EndAttr(m_xAttrTab->pTextLeftMargin, false);
                NewAttr(m_xAttrTab, &m_xAttrTab->pRightMargin, rightMargin);
                EndAttr(m_xAttrTab->pRightMargin, false);
            }
            else
            {
                NewAttr(m_xAttrTab, &m_xAttrTab->pKerning, SvxKerningItem( static_cast<short>(nSize), RES_CHRATR_KERNING ));
                m_xDoc->getIDocumentContentOperations().InsertString( *m_pPam, u" "_ustr );
                EndAttr( m_xAttrTab->pKerning );
            }
        }
    }
}

sal_uInt16 SwHTMLParser::ToTwips( sal_uInt16 nPixel )
{
    return std::min(o3tl::convert(nPixel, o3tl::Length::px, o3tl::Length::twip),
                    sal_Int64(SAL_MAX_UINT16));
}

SwTwips SwHTMLParser::GetCurrentBrowseWidth()
{
    const SwTwips nWidth = SwHTMLTableLayout::GetBrowseWidth( *m_xDoc );
    if( nWidth )
        return nWidth;

    if( !m_aHTMLPageSize.Width() )
    {
        const SwFrameFormat& rPgFormat = m_pCSS1Parser->GetMasterPageDesc()->GetMaster();

        const SwFormatFrameSize& rSz   = rPgFormat.GetFrameSize();
        const SvxLRSpaceItem& rLR = rPgFormat.GetLRSpace();
        const SvxULSpaceItem& rUL = rPgFormat.GetULSpace();
        const SwFormatCol& rCol = rPgFormat.GetCol();

        m_aHTMLPageSize.setWidth(rSz.GetWidth() - rLR.ResolveLeft({}) - rLR.ResolveRight({}));
        m_aHTMLPageSize.setHeight( rSz.GetHeight() - rUL.GetUpper() - rUL.GetLower() );

        if( 1 < rCol.GetNumCols() )
            m_aHTMLPageSize.setWidth( m_aHTMLPageSize.Width() / ( rCol.GetNumCols()) );
    }

    return m_aHTMLPageSize.Width();
}

void SwHTMLParser::InsertIDOption()
{
    OUString aId;
    const HTMLOptions& rHTMLOptions = GetOptions();
    for (size_t i = rHTMLOptions.size(); i; )
    {
        const HTMLOption& rOption = rHTMLOptions[--i];
        if( HtmlOptionId::ID==rOption.GetToken() )
        {
            aId = rOption.GetString();
            break;
        }
    }

    if( !aId.isEmpty() )
        InsertBookmark( aId );
}

void SwHTMLParser::InsertLineBreak()
{
    OUString aId, aStyle, aClass;             // the id of bookmark
    SwLineBreakClear eClear = SwLineBreakClear::NONE;

    // then we fetch the options
    const HTMLOptions& rHTMLOptions = GetOptions();
    for (size_t i = rHTMLOptions.size(); i; )
    {
        const HTMLOption& rOption = rHTMLOptions[--i];
        switch( rOption.GetToken() )
        {
            case HtmlOptionId::CLEAR:
                {
                    const OUString &rClear = rOption.GetString();
                    if( rClear.equalsIgnoreAsciiCase( OOO_STRING_SVTOOLS_HTML_AL_all ) )
                    {
                        eClear = SwLineBreakClear::ALL;
                    }
                    else if( rClear.equalsIgnoreAsciiCase( OOO_STRING_SVTOOLS_HTML_AL_left ) )
                    {
                        eClear = SwLineBreakClear::LEFT;
                    }
                    else if( rClear.equalsIgnoreAsciiCase( OOO_STRING_SVTOOLS_HTML_AL_right ) )
                    {
                        eClear = SwLineBreakClear::LEFT;
                    }
                }
                break;
            case HtmlOptionId::ID:
                aId = rOption.GetString();
                break;
            case HtmlOptionId::STYLE:
                aStyle = rOption.GetString();
                break;
            case HtmlOptionId::CLASS:
                aClass = rOption.GetString();
                break;
            default: break;
        }
    }

    // parse styles
    std::shared_ptr<SvxFormatBreakItem> aBreakItem(std::make_shared<SvxFormatBreakItem>(SvxBreak::NONE, RES_BREAK));
    bool bBreakItem = false;
    if( HasStyleOptions( aStyle, aId, aClass ) )
    {
        SfxItemSet aItemSet( m_xDoc->GetAttrPool(), m_pCSS1Parser->GetWhichMap() );
        SvxCSS1PropertyInfo aPropInfo;

        if( ParseStyleOptions( aStyle, aId, aClass, aItemSet, aPropInfo ) )
        {
            if( m_pCSS1Parser->SetFormatBreak( aItemSet, aPropInfo ) )
            {
                aBreakItem.reset(aItemSet.Get(RES_BREAK).Clone());
                bBreakItem = true;
            }
            if( !aPropInfo.m_aId.isEmpty() )
                InsertBookmark( aPropInfo.m_aId );
        }
    }

    if( bBreakItem && SvxBreak::PageAfter == aBreakItem->GetBreak() )
    {
        NewAttr(m_xAttrTab, &m_xAttrTab->pBreak, *aBreakItem);
        EndAttr( m_xAttrTab->pBreak, false );
    }

    if (!bBreakItem)
    {
        if (eClear == SwLineBreakClear::NONE)
        {
            // If no CLEAR could or should be executed, a line break will be inserted
            m_xDoc->getIDocumentContentOperations().InsertString(*m_pPam, u"\x0A"_ustr);
        }
        else
        {
            // <BR CLEAR=xxx> is mapped an SwFormatLineBreak.
            SwTextNode* pTextNode = m_pPam->GetPointNode().GetTextNode();
            if (pTextNode)
            {
                SwFormatLineBreak aLineBreak(eClear);
                sal_Int32 nPos = m_pPam->GetPoint()->GetContentIndex();
                pTextNode->InsertItem(aLineBreak, nPos, nPos);
            }
        }
    }
    else if( m_pPam->GetPoint()->GetContentIndex() )
    {
        // If a CLEAR is executed in a non-empty paragraph, then after it
        // a new paragraph has to be opened.
        // MIB 21.02.97: Here actually we should change the bottom paragraph
        // margin to zero. This will fail for something like this <BR ..><P>
        // (>Netscape). That's why we don't do it.
        AppendTextNode( AM_NOSPACE );
    }
    if( bBreakItem && SvxBreak::PageBefore == aBreakItem->GetBreak() )
    {
        NewAttr(m_xAttrTab, &m_xAttrTab->pBreak, *aBreakItem);
        EndAttr( m_xAttrTab->pBreak, false );
    }
}

void SwHTMLParser::InsertHorzRule()
{
    sal_uInt16 nSize = 0;
    sal_uInt16 nWidth = 0;

    SvxAdjust eAdjust = SvxAdjust::End;

    bool bPercentWidth = false;
    bool bNoShade = false;
    bool bColor = false;

    Color aColor;
    OUString aId;

    // let's fetch the options
    const HTMLOptions& rHTMLOptions = GetOptions();
    for (size_t i = rHTMLOptions.size(); i; )
    {
        const HTMLOption& rOption = rHTMLOptions[--i];
        switch( rOption.GetToken() )
        {
        case HtmlOptionId::ID:
            aId = rOption.GetString();
            break;
        case HtmlOptionId::SIZE:
            nSize = o3tl::narrowing<sal_uInt16>(rOption.GetNumber());
            break;
        case HtmlOptionId::WIDTH:
            bPercentWidth = (rOption.GetString().indexOf('%') != -1);
            nWidth = o3tl::narrowing<sal_uInt16>(rOption.GetNumber());
            if( bPercentWidth && nWidth>=100 )
            {
                // the default case are 100% lines (no attributes necessary)
                nWidth = 0;
                bPercentWidth = false;
            }
            break;
        case HtmlOptionId::ALIGN:
            eAdjust = rOption.GetEnum( aHTMLPAlignTable, eAdjust );
            break;
        case HtmlOptionId::NOSHADE:
            bNoShade = true;
            break;
        case HtmlOptionId::COLOR:
            rOption.GetColor( aColor );
            bColor = true;
            break;
        default: break;
        }
    }

    if( m_pPam->GetPoint()->GetContentIndex() )
        AppendTextNode( AM_NOSPACE );
    if( m_nOpenParaToken != HtmlTokenId::NONE )
        EndPara();
    AppendTextNode();
    m_pPam->Move( fnMoveBackward );

    // ...and save in a context
    std::unique_ptr<HTMLAttrContext> xCntxt(
        new HTMLAttrContext(HtmlTokenId::HORZRULE, RES_POOLCOLL_HTML_HR, OUString()));

    PushContext(xCntxt);

    // set the new style
    SetTextCollAttrs(m_aContexts.back().get());

    // the hard attributes of the current paragraph will never become invalid
    m_aParaAttrs.clear();

    if( nSize>0 || bColor || bNoShade )
    {
        // set line colour and/or width
        if( !bColor )
            aColor = COL_GRAY;

        SvxBorderLine aBorderLine( &aColor );
        if( nSize )
        {
            tools::Long nPWidth = 0;
            tools::Long nPHeight = static_cast<tools::Long>(nSize);
            SvxCSS1Parser::PixelToTwip( nPWidth, nPHeight );
            if ( !bNoShade )
            {
                aBorderLine.SetBorderLineStyle(SvxBorderLineStyle::DOUBLE);
            }
            aBorderLine.SetWidth( nPHeight );
        }
        else if( bNoShade )
        {
            aBorderLine.SetWidth( SvxBorderLineWidth::Medium );
        }
        else
        {
            aBorderLine.SetBorderLineStyle(SvxBorderLineStyle::DOUBLE);
            aBorderLine.SetWidth(SvxBorderLineWidth::Hairline);
        }

        SvxBoxItem aBoxItem(RES_BOX);
        aBoxItem.SetLine( &aBorderLine, SvxBoxItemLine::BOTTOM );
        HTMLAttr* pTmp = new HTMLAttr(*m_pPam->GetPoint(), aBoxItem, nullptr, std::shared_ptr<HTMLAttrTable>());
        m_aSetAttrTab.push_back( pTmp );
    }
    if( nWidth )
    {
        // If we aren't in a table, then the width value will be "faked" with
        // paragraph indents. That makes little sense in a table. In order to
        // avoid that the line is considered during the width calculation, it
        // still gets an appropriate LRSpace-Item.
        if (!m_xTable)
        {
            // fake length and alignment of line above paragraph indents
            tools::Long nBrowseWidth = GetCurrentBrowseWidth();
            nWidth = bPercentWidth ? o3tl::narrowing<sal_uInt16>((nWidth*nBrowseWidth) / 100)
                               : ToTwips( o3tl::narrowing<sal_uInt16>(nBrowseWidth) );
            if( nWidth < MINLAY )
                nWidth = MINLAY;

            const SwFormatColl *pColl = (static_cast<tools::Long>(nWidth) < nBrowseWidth) ? GetCurrFormatColl() : nullptr;
            if (pColl)
            {
                tools::Long nDist = nBrowseWidth - nWidth;
                ::std::optional<SvxTextLeftMarginItem> oLeft;
                ::std::optional<SvxRightMarginItem> oRight;

                switch( eAdjust )
                {
                case SvxAdjust::Right:
                    oLeft.emplace(SvxIndentValue::twips(o3tl::narrowing<sal_uInt16>(nDist)),
                                  RES_MARGIN_TEXTLEFT);
                    break;
                case SvxAdjust::Left:
                    oRight.emplace(SvxIndentValue::twips(o3tl::narrowing<sal_uInt16>(nDist)),
                                   RES_MARGIN_RIGHT);
                    break;
                case SvxAdjust::Center:
                default:
                    nDist /= 2;
                    oLeft.emplace(SvxIndentValue::twips(o3tl::narrowing<sal_uInt16>(nDist)),
                                  RES_MARGIN_TEXTLEFT);
                    oRight.emplace(SvxIndentValue::twips(o3tl::narrowing<sal_uInt16>(nDist)),
                                   RES_MARGIN_RIGHT);
                    break;
                }

                if (oLeft)
                {
                    HTMLAttr* pTmp = new HTMLAttr(*m_pPam->GetPoint(), *oLeft, nullptr, std::shared_ptr<HTMLAttrTable>());
                    m_aSetAttrTab.push_back( pTmp );
                }
                if (oRight)
                {
                    HTMLAttr* pTmp = new HTMLAttr(*m_pPam->GetPoint(), *oRight, nullptr, std::shared_ptr<HTMLAttrTable>());
                    m_aSetAttrTab.push_back( pTmp );
                }
            }
        }
    }

    // it's not possible to insert bookmarks in links
    if( !aId.isEmpty() )
        InsertBookmark( aId );

    // pop current context of stack
    std::unique_ptr<HTMLAttrContext> xPoppedContext(PopContext(HtmlTokenId::HORZRULE));
    xPoppedContext.reset();

    m_pPam->Move( fnMoveForward );

    // and set the current style in the next paragraph
    SetTextCollAttrs();
}

void SwHTMLParser::ParseMoreMetaOptions()
{
    OUString aName, aContent;
    bool bHTTPEquiv = false;

    const HTMLOptions& rHTMLOptions = GetOptions();
    for (size_t i = rHTMLOptions.size(); i; )
    {
        const HTMLOption& rOption = rHTMLOptions[--i];
        switch( rOption.GetToken() )
        {
        case HtmlOptionId::NAME:
            aName = rOption.GetString();
            bHTTPEquiv = false;
            break;
        case HtmlOptionId::HTTPEQUIV:
            aName = rOption.GetString();
            bHTTPEquiv = true;
            break;
        case HtmlOptionId::CONTENT:
            aContent = rOption.GetString();
            break;
        default: break;
        }
    }

    // Here things get a little tricky: We know for sure, that the Doc-Info
    // wasn't changed. Therefore it's enough to query for Generator and Refresh
    // to find a not processed Token. These are the only ones which won't change
    // the Doc-Info.
    if( aName.equalsIgnoreAsciiCase( OOO_STRING_SVTOOLS_HTML_META_generator ) ||
        aName.equalsIgnoreAsciiCase( OOO_STRING_SVTOOLS_HTML_META_refresh ) ||
        aName.equalsIgnoreAsciiCase( OOO_STRING_SVTOOLS_HTML_META_content_type ) ||
        aName.equalsIgnoreAsciiCase( OOO_STRING_SVTOOLS_HTML_META_content_script_type ) )
        return;

    aContent = aContent.replaceAll("\r", "").replaceAll("\n", "");

    if( aName.equalsIgnoreAsciiCase( OOO_STRING_SVTOOLS_HTML_META_sdendnote ) )
    {
        FillEndNoteInfo( aContent );
        return;
    }

    if( aName.equalsIgnoreAsciiCase( OOO_STRING_SVTOOLS_HTML_META_sdfootnote ) )
    {
        FillFootNoteInfo( aContent );
        return;
    }

    OUStringBuffer sText(
        "HTML: <"
        OOO_STRING_SVTOOLS_HTML_meta
        " ");
    if( bHTTPEquiv  )
        sText.append(OOO_STRING_SVTOOLS_HTML_O_httpequiv);
    else
        sText.append(OOO_STRING_SVTOOLS_HTML_O_name);
    sText.append(
        "=\"" + aName
        + "\" "
        OOO_STRING_SVTOOLS_HTML_O_content
        "=\""
        + aContent
        + "\">");

    SwPostItField aPostItField(
        static_cast<SwPostItFieldType*>(m_xDoc->getIDocumentFieldsAccess().GetSysFieldType( SwFieldIds::Postit )),
        OUString(), sText.makeStringAndClear(), OUString(), ReferenceMarkerName(), DateTime(DateTime::SYSTEM));
    SwFormatField aFormatField( aPostItField );
    InsertAttr( aFormatField,  false );
}

HTMLAttr::HTMLAttr( const SwPosition& rPos, const SfxPoolItem& rItem,
                      HTMLAttr **ppHd, std::shared_ptr<HTMLAttrTable> xAttrTab ) :
    m_nStartPara( rPos.GetNode() ),
    m_nEndPara( rPos.GetNode() ),
    m_nStartContent( rPos.GetContentIndex() ),
    m_nEndContent(rPos.GetContentIndex() ),
    m_bInsAtStart( true ),
    m_bLikePara( false ),
    m_bValid( true ),
    m_pItem( rItem.Clone() ),
    m_xAttrTab(std::move( xAttrTab )),
    m_pNext( nullptr ),
    m_pPrev( nullptr ),
    m_ppHead( ppHd )
{
}

HTMLAttr::HTMLAttr( const HTMLAttr &rAttr, const SwNode &rEndPara,
                      sal_Int32 nEndCnt, HTMLAttr **ppHd, std::shared_ptr<HTMLAttrTable> xAttrTab ) :
    m_nStartPara( rAttr.m_nStartPara ),
    m_nEndPara( rEndPara ),
    m_nStartContent( rAttr.m_nStartContent ),
    m_nEndContent( nEndCnt ),
    m_bInsAtStart( rAttr.m_bInsAtStart ),
    m_bLikePara( rAttr.m_bLikePara ),
    m_bValid( rAttr.m_bValid ),
    m_pItem( rAttr.m_pItem->Clone() ),
    m_xAttrTab(std::move( xAttrTab )),
    m_pNext( nullptr ),
    m_pPrev( nullptr ),
    m_ppHead( ppHd )
{
}

HTMLAttr::~HTMLAttr()
{
}

HTMLAttr *HTMLAttr::Clone(const SwNode& rEndPara, sal_Int32 nEndCnt) const
{
    // create the attribute anew with old start position
    HTMLAttr *pNew = new HTMLAttr( *this, rEndPara, nEndCnt, m_ppHead, m_xAttrTab );

    // The Previous-List must be taken over, the Next-List not!
    pNew->m_pPrev = m_pPrev;

    return pNew;
}

void HTMLAttr::Reset(const SwNode& rSttPara, sal_Int32 nSttCnt,
                     HTMLAttr **ppHd, const std::shared_ptr<HTMLAttrTable>& rAttrTab)
{
    // reset the start (and the end)
    m_nStartPara = rSttPara;
    m_nStartContent = nSttCnt;
    m_nEndPara = rSttPara;
    m_nEndContent = nSttCnt;

    // correct the head and nullify link
    m_pNext = nullptr;
    m_pPrev = nullptr;
    m_ppHead = ppHd;
    m_xAttrTab = rAttrTab;
}

void HTMLAttr::InsertPrev( HTMLAttr *pPrv )
{
    OSL_ENSURE( !pPrv->m_pNext || pPrv->m_pNext == this,
            "HTMLAttr::InsertPrev: pNext wrong" );
    pPrv->m_pNext = nullptr;

    OSL_ENSURE( nullptr == pPrv->m_ppHead || m_ppHead == pPrv->m_ppHead,
            "HTMLAttr::InsertPrev: ppHead wrong" );
    pPrv->m_ppHead = nullptr;

    HTMLAttr *pAttr = this;
    while( pAttr->GetPrev() )
        pAttr = pAttr->GetPrev();

    pAttr->m_pPrev = pPrv;
}

bool SwHTMLParser::ParseMetaOptions(
        const uno::Reference<document::XDocumentProperties> & i_xDocProps,
        SvKeyValueIterator *i_pHeader )
{
    // always call base ParseMetaOptions, it sets the encoding (#i96700#)
    bool ret( HTMLParser::ParseMetaOptions(i_xDocProps, i_pHeader) );
    if (!ret && IsNewDoc())
    {
        ParseMoreMetaOptions();
    }
    return ret;
}

// override so we can parse DOCINFO field subtypes INFO[1-4]
void SwHTMLParser::AddMetaUserDefined( OUString const & i_rMetaName )
{
    // unless we already have 4 names, append the argument to m_InfoNames
    OUString* pName // the first empty string in m_InfoNames
         (m_InfoNames[0].isEmpty() ? &m_InfoNames[0] :
         (m_InfoNames[1].isEmpty() ? &m_InfoNames[1] :
         (m_InfoNames[2].isEmpty() ? &m_InfoNames[2] :
         (m_InfoNames[3].isEmpty() ? &m_InfoNames[3] : nullptr ))));
    if (pName)
    {
        (*pName) = i_rMetaName;
    }
}

void HTMLReader::SetupFilterOptions()
{
    // Reset state from previous Read() invocation.
    m_aNamespace.clear();

    if (!m_pMedium)
        return;

    auto pItem = m_pMedium->GetItemSet().GetItem(SID_FILE_FILTEROPTIONS);
    if (!pItem)
        return;

    OUString aFilterOptions = pItem->GetValue();
    static constexpr OUString aXhtmlNsKey(u"xhtmlns="_ustr);
    if (aFilterOptions.startsWith(aXhtmlNsKey))
    {
        OUString aNamespace = aFilterOptions.copy(aXhtmlNsKey.getLength());
        m_aNamespace = aNamespace;
    }
}

namespace
{
    class FontCacheGuard
    {
    public:
        ~FontCacheGuard()
        {
            FlushFontCache();
        }
    };
}

bool TestImportHTML(SvStream &rStream)
{
    FontCacheGuard aFontCacheGuard;
    HTMLReader aReader;
    aReader.m_pStream = &rStream;

    SwGlobals::ensure();

    SfxObjectShellLock xDocSh(new SwDocShell(SfxObjectCreateMode::INTERNAL));
    xDocSh->DoInitNew();
    SwDoc *pD =  static_cast<SwDocShell*>((&xDocSh))->GetDoc();

    SwPaM aPaM(pD->GetNodes().GetEndOfContent(), SwNodeOffset(-1));
    pD->SetInReading(true);
    bool bRet = false;
    try
    {
        bRet = aReader.Read(*pD, OUString(), aPaM, OUString()) == ERRCODE_NONE;
    }
    catch (const std::runtime_error&)
    {
    }
    catch (const std::out_of_range&)
    {
    }
    pD->SetInReading(false);

    return bRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
