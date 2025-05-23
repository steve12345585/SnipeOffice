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

#include <osl/diagnose.h>
#include <unotools/tempfile.hxx>
#include <svl/stritem.hxx>
#include <svl/eitem.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/docfilt.hxx>
#include <sfx2/fcontnr.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/request.hxx>
#include <sfx2/sfxsids.hrc>
#include <sfx2/viewfrm.hxx>
#include <tools/datetime.hxx>
#include <fmtinfmt.hxx>
#include <fmtanchr.hxx>
#include <doc.hxx>
#include <IDocumentUndoRedo.hxx>
#include <IDocumentRedlineAccess.hxx>
#include <DocumentSettingManager.hxx>
#include <DocumentContentOperationsManager.hxx>
#include <IDocumentLayoutAccess.hxx>
#include <docary.hxx>
#include <pam.hxx>
#include <ndtxt.hxx>
#include <docsh.hxx>
#include <section.hxx>
#include <calbck.hxx>
#include <iodetect.hxx>
#include <frameformats.hxx>
#include <memory>
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>
#include <com/sun/star/document/XDocumentProperties.hpp>
#include <com/sun/star/frame/XModel.hpp>

using namespace ::com::sun::star;

namespace {

enum SwSplitDocType
{
    SPLITDOC_TO_GLOBALDOC,
    SPLITDOC_TO_HTML
};

}

bool SwDoc::GenerateGlobalDoc( const OUString& rPath,
                                   const SwTextFormatColl* pSplitColl )
{
    return SplitDoc( SPLITDOC_TO_GLOBALDOC, rPath, false, pSplitColl );
}

bool SwDoc::GenerateGlobalDoc( const OUString& rPath, int nOutlineLevel )
{
    return SplitDoc( SPLITDOC_TO_GLOBALDOC, rPath, true, nullptr, nOutlineLevel );
}

bool SwDoc::GenerateHTMLDoc( const OUString& rPath, int nOutlineLevel )
{
    return SplitDoc( SPLITDOC_TO_HTML, rPath, true, nullptr, nOutlineLevel );
}

bool SwDoc::GenerateHTMLDoc( const OUString& rPath,
                                 const SwTextFormatColl* pSplitColl )
{
    return SplitDoc( SPLITDOC_TO_HTML, rPath, false, pSplitColl );
}

// two helpers for outline mode
static SwNode* GetStartNode( SwOutlineNodes const * pOutlNds, int nOutlineLevel, SwOutlineNodes::size_type* nOutl )
{
    for( ; *nOutl < pOutlNds->size(); ++(*nOutl) )
    {
        SwNode* pNd = (*pOutlNds)[ *nOutl ];
        if( pNd->GetTextNode()->GetAttrOutlineLevel() == nOutlineLevel && !pNd->FindTableNode() )
        {
            return pNd;
        }
    }

    return nullptr;
}

static SwNode* GetEndNode( SwOutlineNodes const * pOutlNds, int nOutlineLevel, SwOutlineNodes::size_type* nOutl )
{
    SwNode* pNd;

    for( ++(*nOutl); (*nOutl) < pOutlNds->size(); ++(*nOutl) )
    {
        pNd = (*pOutlNds)[ *nOutl ];

        const int nLevel = pNd->GetTextNode()->GetAttrOutlineLevel();

        if( ( 0 < nLevel && nLevel <= nOutlineLevel ) &&
            !pNd->FindTableNode() )
        {
            return pNd;
        }
    }
    return nullptr;
}

// two helpers for collection mode
static SwNode* GetStartNode( const SwOutlineNodes* pOutlNds, const SwTextFormatColl* pSplitColl, SwOutlineNodes::size_type* nOutl )
{
    for( ; *nOutl < pOutlNds->size(); ++(*nOutl) )
    {
        SwNode* pNd = (*pOutlNds)[ *nOutl ];
        if( pNd->GetTextNode()->GetTextColl() == pSplitColl &&
            !pNd->FindTableNode() )
        {
            return pNd;
        }
    }
    return nullptr;
}

static SwNode* GetEndNode( const SwOutlineNodes* pOutlNds, const SwTextFormatColl* pSplitColl, SwOutlineNodes::size_type* nOutl )
{
    SwNode* pNd;

    for( ++(*nOutl); *nOutl < pOutlNds->size(); ++(*nOutl) )
    {
        pNd = (*pOutlNds)[ *nOutl ];
        SwTextFormatColl* pTColl = pNd->GetTextNode()->GetTextColl();

        if( ( pTColl == pSplitColl ||
              (   pSplitColl->GetAttrOutlineLevel() > 0 &&
                  pTColl->GetAttrOutlineLevel() > 0   &&
                  pTColl->GetAttrOutlineLevel() <
                  pSplitColl->GetAttrOutlineLevel() )) &&
            !pNd->FindTableNode() )
        {
            return pNd;
        }
    }
    return nullptr;
}

bool SwDoc::SplitDoc( sal_uInt16 eDocType, const OUString& rPath, bool bOutline, const SwTextFormatColl* pSplitColl, int nOutlineLevel )
{
    // Iterate over all the template's Nodes, creating an own
    // document for every single one and replace linked sections (GlobalDoc) for links (HTML).
    // Finally, we save this document as a GlobalDoc/HTMLDoc.
    if( !mpDocShell || !mpDocShell->GetMedium() ||
        ( SPLITDOC_TO_GLOBALDOC == eDocType && GetDocumentSettingManager().get(DocumentSettingId::GLOBAL_DOCUMENT) ) )
        return false;

    SwOutlineNodes::size_type nOutl = 0;
    SwOutlineNodes* pOutlNds = const_cast<SwOutlineNodes*>(&GetNodes().GetOutLineNds());
    std::unique_ptr<SwOutlineNodes> xTmpOutlNds;
    SwNode* pStartNd;

    if ( !bOutline) {
        if( pSplitColl )
        {
            // If it isn't an OutlineNumbering, then use an own array and collect the Nodes.
            if( pSplitColl->GetAttrOutlineLevel() == 0 )
            {
                xTmpOutlNds.reset(new SwOutlineNodes);
                pOutlNds = xTmpOutlNds.get();
                SwIterator<SwTextNode,SwFormatColl> aIter( *pSplitColl );
                for( SwTextNode* pTNd = aIter.First(); pTNd; pTNd = aIter.Next() )
                    if( pTNd->GetNodes().IsDocNodes() )
                        pOutlNds->insert( pTNd );

                if( pOutlNds->empty() )
                    return false;
            }
        }
        else
        {
            // Look for the 1st level OutlineTemplate
            const SwTextFormatColls& rFormatColls =*GetTextFormatColls();
            for( SwTextFormatColls::size_type n = rFormatColls.size(); n; )
                if ( rFormatColls[ --n ]->GetAttrOutlineLevel() == 1 )
                {
                    pSplitColl = rFormatColls[ n ];
                    break;
                }

            if( !pSplitColl )
                return false;
        }
    }

    std::shared_ptr<const SfxFilter> pFilter;
    switch( eDocType )
    {
    case SPLITDOC_TO_HTML:
        pFilter = SwIoSystem::GetFilterOfFormat(u"HTML");
        break;

    default:
        pFilter = SwIoSystem::GetFilterOfFormat(FILTER_XML);
        eDocType = SPLITDOC_TO_GLOBALDOC;
        break;
    }

    if( !pFilter )
        return false;

    // Deactivate Undo/Redline in any case
    GetIDocumentUndoRedo().DoUndo(false);
    getIDocumentRedlineAccess().SetRedlineFlags_intern( getIDocumentRedlineAccess().GetRedlineFlags() & ~RedlineFlags::On );

    OUString sExt = pFilter->GetSuffixes().getToken(0, ',');
    if( sExt.isEmpty() )
    {
        sExt = ".sxw";
    }
    else
    {
        if( '.' != sExt[ 0 ] )
        {
            sExt = "." + sExt;
        }
    }

    INetURLObject aEntry(rPath);
    OUString sLeading(aEntry.GetBase());
    aEntry.removeSegment();
    OUString sPath = aEntry.GetMainURL( INetURLObject::DecodeMechanism::NONE );
    utl::TempFileNamed aTemp(sLeading, true, sExt, &sPath);
    aTemp.EnableKillingFile();

    DateTime aTmplDate( DateTime::SYSTEM );
    {
        tools::Time a2Min(0, 2);
        aTmplDate += a2Min;
    }

    // Skip all invalid ones
    while( nOutl < pOutlNds->size() &&
        (*pOutlNds)[ nOutl ]->GetIndex() < GetNodes().GetEndOfExtras().GetIndex() )
        ++nOutl;

    do {
        if( bOutline )
            pStartNd = GetStartNode( pOutlNds, nOutlineLevel, &nOutl );
        else
            pStartNd = GetStartNode( pOutlNds, pSplitColl, &nOutl );

        if( pStartNd )
        {
            SwNode* pEndNd;
            if( bOutline )
                pEndNd = GetEndNode( pOutlNds, nOutlineLevel, &nOutl );
            else
                pEndNd = GetEndNode( pOutlNds, pSplitColl, &nOutl );
            SwNodeIndex aEndIdx( pEndNd ? *pEndNd
                                        : GetNodes().GetEndOfContent() );

            // Write out the Nodes completely
            OUString sFileName;
            if( pStartNd->GetIndex() + 1 < aEndIdx.GetIndex() )
            {
                SfxObjectShellLock xDocSh( new SwDocShell( SfxObjectCreateMode::INTERNAL ));
                if( xDocSh->DoInitNew() )
                {
                    SwDoc* pDoc = static_cast<SwDocShell*>(&xDocSh)->GetDoc();

                    uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
                        static_cast<SwDocShell*>(&xDocSh)->GetModel(),
                        uno::UNO_QUERY_THROW);
                    uno::Reference<document::XDocumentProperties> xDocProps(
                        xDPS->getDocumentProperties());
                    OSL_ENSURE(xDocProps.is(), "Doc has no DocumentProperties");
                    // the GlobalDoc is the template
                    xDocProps->setTemplateName(OUString());
                    ::util::DateTime uDT = aTmplDate.GetUNODateTime();
                    xDocProps->setTemplateDate(uDT);
                    xDocProps->setTemplateURL(rPath);
                    // Set the new doc's title to the text of the "split para".
                    // If the current doc has a title, insert it at the begin.
                    OUString sTitle( xDocProps->getTitle() );
                    if (!sTitle.isEmpty())
                        sTitle += ": ";
                    sTitle += pStartNd->GetTextNode()->GetExpandText(nullptr);
                    xDocProps->setTitle( sTitle );

                    // Replace template
                    pDoc->ReplaceStyles( *this );

                    // Take over chapter numbering
                    if( mpOutlineRule )
                        pDoc->SetOutlineNumRule( *mpOutlineRule );

                    SwNodeRange aRg( *pStartNd, SwNodeOffset(0), aEndIdx.GetNode() );
                    GetDocumentContentOperationsManager().CopyWithFlyInFly(
                            aRg, pDoc->GetNodes().GetEndOfContent(), nullptr, false, false);

                    // Delete the initial TextNode
                    SwNodeIndex aIdx( pDoc->GetNodes().GetEndOfExtras(), 2 );
                    if( aIdx.GetIndex() + 1 !=
                        pDoc->GetNodes().GetEndOfContent().GetIndex() )
                        pDoc->GetNodes().Delete( aIdx );

                    sFileName = utl::CreateTempURL(sLeading, true, sExt, &sPath);
                    SfxMedium* pTmpMed = new SfxMedium( sFileName,
                                                StreamMode::STD_READWRITE );
                    pTmpMed->SetFilter( pFilter );

                    // We need to have a Layout for the HTMLFilter, so that
                    // TextFrames/Controls/OLE objects can be exported correctly as graphics.
                    if( SPLITDOC_TO_HTML == eDocType &&
                        !pDoc->GetSpzFrameFormats()->empty() )
                    {
                            SfxViewFrame::LoadHiddenDocument( *xDocSh, SFX_INTERFACE_NONE );
                    }
                    xDocSh->DoSaveAs( *pTmpMed );
                    xDocSh->DoSaveCompleted( pTmpMed );

                    // do not insert a FileLinkSection in case of error
                    if( xDocSh->GetErrorIgnoreWarning() )
                        sFileName.clear();
                }
                xDocSh->DoClose();
            }

            // We can now insert the section
            if( !sFileName.isEmpty() )
            {
                switch( eDocType )
                {
                case SPLITDOC_TO_HTML:
                    {
                        // Delete all nodes in the section and, in the "start node",
                        // set the Link to the saved document.
                        SwNodeOffset nNodeDiff = aEndIdx.GetIndex() -
                                            pStartNd->GetIndex() - 1;
                        if( nNodeDiff )
                        {
                            SwPaM aTmp( *pStartNd, aEndIdx.GetNode(), SwNodeOffset(1), SwNodeOffset(-1) );
                            SwNodeIndex aSIdx( aTmp.GetMark()->GetNode() );
                            SwNodeIndex aEIdx( aTmp.GetPoint()->GetNode() );

                            // Try to move past the end
                            if( !aTmp.Move( fnMoveForward, GoInNode ) )
                            {
                                // well then, back to the beginning
                                aTmp.Exchange();
                                if( !aTmp.Move( fnMoveBackward, GoInNode ))
                                {
                                    OSL_FAIL( "no more Nodes!" );
                                }
                            }
                            // Move Bookmarks and so forth
                            CorrAbs( aSIdx, aEIdx, *aTmp.GetPoint(), true);

                            // If FlyFrames are still around, delete these too
                            auto& rSpzs = *GetSpzFrameFormats();
                            for(sw::FrameFormats<sw::SpzFrameFormat*>::size_type n = 0; n < GetSpzFrameFormats()->size(); )
                            {
                                auto pFly = rSpzs[n];
                                const SwFormatAnchor* pAnchor = &pFly->GetAnchor();
                                SwNode const*const pAnchorNode =
                                    pAnchor->GetAnchorNode();
                                if (pAnchorNode &&
                                    ((RndStdIds::FLY_AT_PARA == pAnchor->GetAnchorId()) ||
                                     (RndStdIds::FLY_AT_CHAR == pAnchor->GetAnchorId())) &&
                                    aSIdx <= *pAnchorNode &&
                                    *pAnchorNode < aEIdx.GetNode() )
                                {
                                    getIDocumentLayoutAccess().DelLayoutFormat( pFly );
                                }
                                else
                                    ++n;
                            }

                            GetNodes().Delete( aSIdx, nNodeDiff );
                        }

                        // set the link in the StartNode
                        SwFormatINetFormat aINet( sFileName , OUString() );
                        SwTextNode* pTNd = pStartNd->GetTextNode();
                        pTNd->InsertItem(aINet, 0, pTNd->GetText().getLength());

                        // If the link cannot be found anymore,
                        // it has to be a bug!
                        if( !pOutlNds->Seek_Entry( pStartNd, &nOutl ))
                            pStartNd = nullptr;
                        ++nOutl ;
                    }
                    break;

                default:
                    {
                        const OUString sNm(INetURLObject(sFileName).GetLastName());
                        SwSectionData aSectData( SectionType::FileLink,
                                        UIName(GetUniqueSectionName( &sNm )));
                        SwSectionFormat* pFormat = MakeSectionFormat();
                        aSectData.SetLinkFileName(sFileName);
                        aSectData.SetProtectFlag(true);

                        --aEndIdx;  // in the InsertSection the end is inclusive
                        while( aEndIdx.GetNode().IsStartNode() )
                            --aEndIdx;

                        // If any Section ends or starts in the new sectionrange,
                        // they must end or start before or after the range!
                        SwSectionNode* pSectNd = pStartNd->FindSectionNode();
                        while( pSectNd && pSectNd->EndOfSectionIndex()
                                <= aEndIdx.GetIndex() )
                        {
                            const SwNode* pSectEnd = pSectNd->EndOfSectionNode();
                            if( pSectNd->GetIndex() + 1 ==
                                    pStartNd->GetIndex() )
                            {
                                bool bMvIdx = aEndIdx == *pSectEnd;
                                DelSectionFormat( pSectNd->GetSection().GetFormat() );
                                if( bMvIdx )
                                    --aEndIdx;
                            }
                            else
                            {
                                SwNodeRange aRg( *pStartNd, *pSectEnd );
                                SwNodeIndex aIdx( *pSectEnd, 1 );
                                GetNodes().MoveNodes( aRg, GetNodes(), aIdx.GetNode() );
                            }
                            pSectNd = pStartNd->FindSectionNode();
                        }

                        pSectNd = aEndIdx.GetNode().FindSectionNode();
                        while( pSectNd && pSectNd->GetIndex() >
                                pStartNd->GetIndex() )
                        {
                            // #i15712# don't attempt to split sections if
                            // they are fully enclosed in [pSectNd,aEndIdx].
                            if( aEndIdx < pSectNd->EndOfSectionIndex() )
                            {
                                SwNodeRange aRg( *pSectNd, SwNodeOffset(1), aEndIdx.GetNode(), SwNodeOffset(1) );
                                GetNodes().MoveNodes( aRg, GetNodes(), *pSectNd );
                            }

                            pSectNd = pStartNd->FindSectionNode();
                        }

                        // -> #i26762#
                        // Ensure order of start and end of section is sane.
                        SwNodeIndex aStartIdx(*pStartNd);

                        if (aEndIdx >= aStartIdx)
                        {
                            pSectNd = GetNodes().InsertTextSection(aStartIdx.GetNode(),
                                *pFormat, aSectData, nullptr, &aEndIdx.GetNode(), false);
                        }
                        else
                        {
                            pSectNd = GetNodes().InsertTextSection(aEndIdx.GetNode(),
                                *pFormat, aSectData, nullptr, &aStartIdx.GetNode(), false);
                        }
                        // <- #i26762#

                        pSectNd->GetSection().CreateLink( LinkCreateType::Connect );
                    }
                    break;
                }
            }
        }
    } while( pStartNd );

    xTmpOutlNds.reset();

    switch( eDocType )
    {
    case SPLITDOC_TO_HTML:
        if( GetDocumentSettingManager().get(DocumentSettingId::GLOBAL_DOCUMENT) )
        {
            // save all remaining sections
            while( !GetSections().empty() )
                DelSectionFormat( GetSections().front() );

            SfxFilterContainer* pFCntnr = mpDocShell->GetFactory().GetFilterContainer();
            pFilter = pFCntnr->GetFilter4EA( pFilter->GetTypeName(), SfxFilterFlags::EXPORT );
        }
        break;

    default:
        // save the Globaldoc
        GetDocumentSettingManager().set(DocumentSettingId::GLOBAL_DOCUMENT, true);
        GetDocumentSettingManager().set(DocumentSettingId::GLOBAL_DOCUMENT_SAVE_LINKS, false);
    }

    // The medium isn't locked after reopening the document.
    SfxRequest aReq( SID_SAVEASDOC, SfxCallMode::SYNCHRON, GetAttrPool() );
    aReq.AppendItem( SfxStringItem( SID_FILE_NAME, rPath ) );
    aReq.AppendItem( SfxBoolItem( SID_SAVETO, true ) );
    if(pFilter)
        aReq.AppendItem( SfxStringItem( SID_FILTER_NAME, pFilter->GetName() ) );
    const SfxPoolItemHolder& rResult(mpDocShell->ExecuteSlot(aReq));
    const SfxBoolItem *pRet(static_cast<const SfxBoolItem*>(rResult.getItem()));

    return pRet && pRet->GetValue();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
