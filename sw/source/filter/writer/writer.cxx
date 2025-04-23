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
#include <hintids.hxx>

#include <sot/storage.hxx>
#include <sfx2/docfile.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <editeng/fontitem.hxx>
#include <editeng/eeitem.hxx>
#include <osl/diagnose.h>
#include <shellio.hxx>
#include <doc.hxx>
#include <docary.hxx>
#include <IMark.hxx>
#include <IDocumentSettingAccess.hxx>
#include <IDocumentMarkAccess.hxx>
#include <numrule.hxx>
#include <swerror.h>
#include <com/sun/star/ucb/ContentCreationException.hpp>

using namespace css;

typedef std::multimap<SwNodeOffset, const ::sw::mark::MarkBase*> SwBookmarkNodeTable;

struct Writer_Impl
{
    SvStream * m_pStream;

    std::map<OUString, OUString> maFileNameMap;
    std::vector<SfxPoolItemHolder> aFontRemoveLst;
    SwBookmarkNodeTable aBkmkNodePos;

    Writer_Impl();

    void RemoveFontList();
    void InsertBkmk( const ::sw::mark::MarkBase& rBkmk );
};

Writer_Impl::Writer_Impl()
    : m_pStream(nullptr)
{
}

void Writer_Impl::RemoveFontList()
{
    aFontRemoveLst.clear();
}

void Writer_Impl::InsertBkmk(const ::sw::mark::MarkBase& rBkmk)
{
    SwNodeOffset nNd = rBkmk.GetMarkPos().GetNodeIndex();

    aBkmkNodePos.emplace( nNd, &rBkmk );

    if(rBkmk.IsExpanded() && rBkmk.GetOtherMarkPos().GetNodeIndex() != nNd)
    {
        nNd = rBkmk.GetOtherMarkPos().GetNodeIndex();
        aBkmkNodePos.emplace( nNd, &rBkmk );
    }
}

/*
 * This module is the central collection point for all writer-filters
 * and is a DLL !
 *
 * So that the Writer can work with different writers, the output-functions
 * of the content carrying objects have to be mapped to the various
 * output-functions.
 *
 * For that, to inquire its output function, every object can be gripped
 * via the which-value in a table.
 * These functions are available in the corresponding Writer-DLL's.
 */

Writer::Writer()
    : m_pImpl(std::make_unique<Writer_Impl>())
    , m_pOrigFileName(nullptr), m_pDoc(nullptr), m_pOrigPam(nullptr)
    , m_bHideDeleteRedlines(false)
{
    m_bWriteAll = m_bShowProgress = m_bUCS2_WithStartChar = true;
    m_bASCII_NoLastLineEnd = m_bASCII_ParaAsBlank = m_bASCII_ParaAsCR =
        m_bWriteClipboardDoc = m_bWriteOnlyFirstTable = m_bBlock =
        m_bOrganizerMode = false;
    m_bExportParagraphNumbering = true;
}

Writer::~Writer()
{
}

/*
 * Document Interface Access
 */
IDocumentSettingAccess& Writer::getIDocumentSettingAccess() { return m_pDoc->getIDocumentSettingAccess(); }
const IDocumentSettingAccess& Writer::getIDocumentSettingAccess() const { return m_pDoc->getIDocumentSettingAccess(); }
IDocumentStylePoolAccess& Writer::getIDocumentStylePoolAccess() { return m_pDoc->getIDocumentStylePoolAccess(); }
const IDocumentStylePoolAccess& Writer::getIDocumentStylePoolAccess() const { return m_pDoc->getIDocumentStylePoolAccess(); }

void Writer::ResetWriter()
{
    m_pImpl->RemoveFontList();
    m_pImpl.reset(new Writer_Impl);

    if( m_pCurrentPam )
    {
        while (m_pCurrentPam->GetNext() != m_pCurrentPam.get())
            delete m_pCurrentPam->GetNext();
        m_pCurrentPam.reset();
    }
    m_pCurrentPam = nullptr;
    m_pOrigFileName = nullptr;
    m_pDoc = nullptr;

    m_bShowProgress = m_bUCS2_WithStartChar = true;
    m_bASCII_NoLastLineEnd = m_bASCII_ParaAsBlank = m_bASCII_ParaAsCR =
        m_bWriteClipboardDoc = m_bWriteOnlyFirstTable = m_bBlock =
        m_bOrganizerMode = false;
}

bool Writer::CopyNextPam( SwPaM ** ppPam )
{
    if( (*ppPam)->GetNext() == m_pOrigPam )
    {
        *ppPam = m_pOrigPam;          // set back to the beginning pam
        return false;           // end of the ring
    }

    // otherwise copy the next value from the next Pam
    *ppPam = (*ppPam)->GetNext();

    *m_pCurrentPam->GetPoint() = *(*ppPam)->Start();
    *m_pCurrentPam->GetMark() = *(*ppPam)->End();

    return true;
}

// search the next Bookmark-Position from the Bookmark-Table

sal_Int32 Writer::FindPos_Bkmk(const SwPosition& rPos) const
{
    const IDocumentMarkAccess* const pMarkAccess = m_pDoc->getIDocumentMarkAccess();
    const auto ppBkmk = pMarkAccess->findFirstBookmarkNotStartsBefore(rPos);
    if(ppBkmk != pMarkAccess->getBookmarksEnd())
        return ppBkmk - pMarkAccess->getBookmarksBegin();
    return -1;
}

std::shared_ptr<SwUnoCursor>
Writer::NewUnoCursor(SwDoc & rDoc, SwNodeOffset const nStartIdx, SwNodeOffset const nEndIdx)
{
    SwNodes *const pNds = &rDoc.GetNodes();

    SwNodeIndex aStt( *pNds, nStartIdx );
    SwContentNode* pCNode = aStt.GetNode().GetContentNode();
    if (!pCNode && nullptr == SwNodes::GoNext(&aStt))
    {
        OSL_FAIL( "No more ContentNode at StartPos" );
    }

    auto const pNew = rDoc.CreateUnoCursor(SwPosition(aStt), false);
    pNew->SetMark();
    aStt = nEndIdx;
    pCNode = aStt.GetNode().GetContentNode();
    if (!pCNode)
        pCNode = SwNodes::GoPrevious(&aStt);
    assert(pCNode && "No more ContentNode at StartPos");
    pNew->GetPoint()->AssignEndIndex( *pCNode );
    return pNew;
}

// Stream-specific
SvStream& Writer::Strm()
{
    assert(m_pImpl->m_pStream && "Oh-oh. Writer with no Stream!");
    return *m_pImpl->m_pStream;
}

void Writer::SetStream(SvStream *const pStream)
{
    m_pImpl->m_pStream = pStream;
}

ErrCodeMsg Writer::Write( SwPaM& rPaM, SvStream& rStrm, const OUString* pFName )
{
    if ( IsStgWriter() )
    {
        ErrCodeMsg nResult = ERRCODE_ABORT;
        try
        {
            rtl::Reference<SotStorage> aRef = new SotStorage(rStrm);
            nResult = Write( rPaM, *aRef, pFName );
            if ( nResult == ERRCODE_NONE )
                aRef->Commit();
        }
        catch (const css::ucb::ContentCreationException &)
        {
            TOOLS_WARN_EXCEPTION("sw", "Writer::Write caught");
        }
        return nResult;
    }

    m_pDoc = &rPaM.GetDoc();
    m_pOrigFileName = pFName;
    m_pImpl->m_pStream = &rStrm;

    // Copy PaM, so that it can be modified
    m_pCurrentPam = m_pDoc->CreateUnoCursor(*rPaM.End(), false);
    m_pCurrentPam->SetMark();
    *m_pCurrentPam->GetPoint() = *rPaM.Start();
    // for comparison secure to the current Pam
    m_pOrigPam = &rPaM;

    ErrCode nRet = WriteStream();

    ResetWriter();

    return nRet;
}

void Writer::SetupFilterOptions(SfxMedium& /*rMedium*/)
{}

ErrCodeMsg Writer::Write( SwPaM& rPam, SfxMedium& rMedium, const OUString* pFileName )
{
    SetupFilterOptions(rMedium);
    // This method must be overridden in SwXMLWriter a storage from medium will be used there.
    // The microsoft format can write to storage but the storage will be based on the stream.
    return Write( rPam, *rMedium.GetOutStream(), pFileName );
}

ErrCodeMsg Writer::Write( SwPaM& /*rPam*/, SotStorage&, const OUString* )
{
    OSL_ENSURE( false, "Write in Storages on a stream?" );
    return ERR_SWG_WRITE_ERROR;
}

ErrCodeMsg Writer::Write( SwPaM&, const uno::Reference < embed::XStorage >&, const OUString*, SfxMedium* )
{
    OSL_ENSURE( false, "Write in Storages on a stream?" );
    return ERR_SWG_WRITE_ERROR;
}

bool Writer::CopyLocalFileToINet( OUString& rFileNm )
{
    if( !m_pOrigFileName )                // can be happen, by example if we
        return false;                   // write into the clipboard

    bool bRet = false;
    INetURLObject aFileUrl( rFileNm ), aTargetUrl( *m_pOrigFileName );

    if (!(INetProtocol::File == aFileUrl.GetProtocol()
            && (INetProtocol::Http == aTargetUrl.GetProtocol()
                || INetProtocol::Https == aTargetUrl.GetProtocol()
                || INetProtocol::VndSunStarWebdav == aTargetUrl.GetProtocol()
                || INetProtocol::Smb == aTargetUrl.GetProtocol()
                || INetProtocol::Sftp == aTargetUrl.GetProtocol()
                || INetProtocol::Cmis == aTargetUrl.GetProtocol())))
    {
        return bRet;
    }

    // has the file been moved?
    std::map<OUString, OUString>::iterator it = m_pImpl->maFileNameMap.find( rFileNm );
    if ( it != m_pImpl->maFileNameMap.end() )
    {
        rFileNm = it->second;
        return true;
    }

    OUString aSrc  = rFileNm;
    OUString aDest = aTargetUrl.GetPartBeforeLastName() + aFileUrl.GetLastName();

    SfxMedium aSrcFile( aSrc, StreamMode::READ );
    SfxMedium aDstFile( aDest, StreamMode::WRITE | StreamMode::SHARE_DENYNONE );

    aDstFile.GetOutStream()->WriteStream( *aSrcFile.GetInStream() );

    aSrcFile.Close();
    aDstFile.Commit();

    bRet = ERRCODE_NONE == aDstFile.GetErrorIgnoreWarning();

    if( bRet )
    {
        m_pImpl->maFileNameMap.insert( std::make_pair( aSrc, aDest ) );
        rFileNm = aDest;
    }

    return bRet;
}

void Writer::PutNumFormatFontsInAttrPool()
{
    // then there are a few fonts in the NumRules
    // These put into the Pool. After this does they have a RefCount > 1
    // it can be removed - it is already in the Pool
    SfxItemPool& rPool = m_pDoc->GetAttrPool();
    const SwNumRuleTable& rListTable = m_pDoc->GetNumRuleTable();
    const SwNumFormat* pFormat;
    const vcl::Font* pDefFont = &numfunc::GetDefBulletFont();
    bool bCheck = false;

    for( size_t nGet = rListTable.size(); nGet; )
    {
        SwNumRule const*const pRule = rListTable[ --nGet ];
        if (m_pDoc->IsUsed(*pRule))
        {
            for( sal_uInt8 nLvl = 0; nLvl < MAXLEVEL; ++nLvl )
            {
                if( SVX_NUM_CHAR_SPECIAL == (pFormat = &pRule->Get( nLvl ))->GetNumberingType() ||
                    SVX_NUM_BITMAP == pFormat->GetNumberingType() )
                {
                    std::optional<vcl::Font> pFont = pFormat->GetBulletFont();
                    if( !pFont )
                        pFont = *pDefFont;

                    if( bCheck )
                    {
                        if( *pFont == *pDefFont )
                            continue;
                    }
                    else if( *pFont == *pDefFont )
                        bCheck = true;

                    AddFontItem( rPool, SvxFontItem( pFont->GetFamilyTypeMaybeAskConfig(),
                                pFont->GetFamilyName(), pFont->GetStyleName(),
                                pFont->GetPitchMaybeAskConfig(), pFont->GetCharSet(), RES_CHRATR_FONT ));
                }
            }
        }
    }
}

void Writer::PutEditEngFontsInAttrPool()
{
    SfxItemPool& rPool = m_pDoc->GetAttrPool();
    if( rPool.GetSecondaryPool() )
    {
        AddFontItems_( rPool, EE_CHAR_FONTINFO );
        AddFontItems_( rPool, EE_CHAR_FONTINFO_CJK );
        AddFontItems_( rPool, EE_CHAR_FONTINFO_CTL );
    }
}

void Writer::AddFontItems_( SfxItemPool& rPool, TypedWhichId<SvxFontItem> nWhich )
{
    const SvxFontItem* pFont = &rPool.GetUserOrPoolDefaultItem( nWhich );
    AddFontItem( rPool, *pFont );

    pFont = rPool.GetUserDefaultItem( nWhich );
    if( nullptr != pFont )
        AddFontItem( rPool, *pFont );

    if (nWhich == RES_CHRATR_FONT || nWhich == RES_CHRATR_CJK_FONT || nWhich == RES_CHRATR_CTL_FONT)
    {
        m_pDoc->ForEachCharacterFontItem(nWhich, /*bIgnoreAutoStyles*/false,
            [this, &rPool] (const SvxFontItem& rFontItem) -> bool
            {
                AddFontItem( rPool, rFontItem );
                return true;
            });
    }
    else
    {
        // nWhich is one of EE_CHAR_FONTINFO /  EE_CHAR_FONTINFO_CJK / rPool, EE_CHAR_FONTINFO_CTL
        ItemSurrogates aSurrogates;
        rPool.GetItemSurrogates(aSurrogates, nWhich);
        for (const SfxPoolItem* pItem : aSurrogates)
            AddFontItem( rPool, *static_cast<const SvxFontItem*>(pItem) );
    }
}

void Writer::AddFontItem( SfxItemPool& rPool, const SvxFontItem& rFont )
{
    SfxPoolItemHolder aItem;
    if( RES_CHRATR_FONT != rFont.Which() )
    {
        SvxFontItem aFont( rFont );
        aFont.SetWhich( RES_CHRATR_FONT );
        aItem = SfxPoolItemHolder(rPool, &aFont);
        assert(aItem.getItem() != &aFont && "Pointer to local outside scope (pushed to aFontRemoveLst)");
    }
    else
        aItem = SfxPoolItemHolder(rPool, &rFont);

    if(1 == aItem.getItem()->GetRefCount())
    {
        m_pImpl->aFontRemoveLst.push_back(aItem);
    }
}

// build a bookmark table, which is sort by the node position. The
// OtherPos of the bookmarks also inserted.
void Writer::CreateBookmarkTable()
{
    const IDocumentMarkAccess* const pMarkAccess = m_pDoc->getIDocumentMarkAccess();
    for(auto ppBkmk = pMarkAccess->getBookmarksBegin();
        ppBkmk != pMarkAccess->getBookmarksEnd();
        ++ppBkmk)
    {
        m_pImpl->InsertBkmk(**ppBkmk);
    }
}

// search all Bookmarks in the range and return it in the Array
bool Writer::GetBookmarks(const SwContentNode& rNd, sal_Int32 nStt,
    sal_Int32 nEnd, std::vector< const ::sw::mark::MarkBase* >& rArr)
{
    OSL_ENSURE( rArr.empty(), "there are still entries available" );

    SwNodeOffset nNd = rNd.GetIndex();
    std::pair<SwBookmarkNodeTable::const_iterator, SwBookmarkNodeTable::const_iterator> aIterPair
        = m_pImpl->aBkmkNodePos.equal_range( nNd );
    if( aIterPair.first != aIterPair.second )
    {
        // there exist some bookmarks, search now all which is in the range
        if( !nStt && nEnd == rNd.Len() )
            // all
            for( SwBookmarkNodeTable::const_iterator it = aIterPair.first; it != aIterPair.second; ++it )
                rArr.push_back( it->second );
        else
        {
            for( SwBookmarkNodeTable::const_iterator it = aIterPair.first; it != aIterPair.second; ++it )
            {
                const ::sw::mark::MarkBase& rBkmk = *(it->second);
                sal_Int32 nContent;
                if( rBkmk.GetMarkPos().GetNode() == rNd &&
                    (nContent = rBkmk.GetMarkPos().GetContentIndex() ) >= nStt &&
                    nContent < nEnd )
                {
                    rArr.push_back( &rBkmk );
                }
                else if( rBkmk.IsExpanded() &&
                        (rNd == rBkmk.GetOtherMarkPos().GetNode()) &&
                        (nContent = rBkmk.GetOtherMarkPos().GetContentIndex()) >= nStt &&
                        nContent < nEnd )
                {
                    rArr.push_back( &rBkmk );
                }
            }
        }
    }
    return !rArr.empty();
}

// Storage-specific
ErrCode StgWriter::WriteStream()
{
    OSL_ENSURE( false, "Write in Storages on a stream?" );
    return ERR_SWG_WRITE_ERROR;
}

ErrCodeMsg StgWriter::Write( SwPaM& rPaM, SotStorage& rStg, const OUString* pFName )
{
    SetStream(nullptr);
    m_pStg = &rStg;
    m_pDoc = &rPaM.GetDoc();
    m_pOrigFileName = pFName;

    // Copy PaM, so that it can be modified
    m_pCurrentPam = m_pDoc->CreateUnoCursor(*rPaM.End(), false);
    m_pCurrentPam->SetMark();
    *m_pCurrentPam->GetPoint() = *rPaM.Start();
    // for comparison secure to the current Pam
    m_pOrigPam = &rPaM;

    ErrCodeMsg nRet = WriteStorage();

    m_pStg = nullptr;
    ResetWriter();

    return nRet;
}

ErrCodeMsg StgWriter::Write( SwPaM& rPaM, const uno::Reference < embed::XStorage >& rStg, const OUString* pFName, SfxMedium* pMedium )
{
    SetStream(nullptr);
    m_pStg = nullptr;
    m_xStg = rStg;
    m_pDoc = &rPaM.GetDoc();
    m_pOrigFileName = pFName;

    // Copy PaM, so that it can be modified
    m_pCurrentPam = m_pDoc->CreateUnoCursor(*rPaM.End(), false);
    m_pCurrentPam->SetMark();
    *m_pCurrentPam->GetPoint() = *rPaM.Start();
    // for comparison secure to the current Pam
    m_pOrigPam = &rPaM;

    ErrCodeMsg nRet = pMedium ? WriteMedium( *pMedium ) : WriteStorage();

    m_pStg = nullptr;
    ResetWriter();

    return nRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
