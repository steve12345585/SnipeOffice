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

#ifndef INCLUDED_SW_SOURCE_CORE_INC_ACORRECT_HXX
#define INCLUDED_SW_SOURCE_CORE_INC_ACORRECT_HXX

#include <memory>

#include <svl/itemset.hxx>
#include <editeng/svxacorr.hxx>
#include <nodeoffset.hxx>
#include <ndindex.hxx>
#include <utility>

class SwEditShell;
class SwPaM;
struct SwPosition;
class SfxItemSet;

class SwDontExpandItem
{
    std::unique_ptr<SfxItemSet> m_pDontExpandItems;

public:
    SwDontExpandItem() {}
    ~SwDontExpandItem();

    void SaveDontExpandItems( const SwPosition& rPos );
    void RestoreDontExpandItems( const SwPosition& rPos );

};

class SwAutoCorrDoc final : public SvxAutoCorrDoc
{
    SwEditShell& m_rEditSh;
    SwPaM& m_rCursor;
    std::optional<SwNodeIndex> m_oIndex;
    int m_nEndUndoCounter;
    bool    m_bUndoIdInitialized;

    void DeleteSel( SwPaM& rDelPam );
    void DeleteSelImpl(SwPaM & rDelPam);

public:
    SwAutoCorrDoc( SwEditShell& rEditShell, SwPaM& rPam, sal_Unicode cIns = 0 );
    virtual ~SwAutoCorrDoc() override;

    virtual bool Delete( sal_Int32 nStt, sal_Int32 nEnd ) override;
    virtual bool Insert( sal_Int32 nPos, const OUString& rText ) override;
    virtual bool Replace( sal_Int32 nPos, const OUString& rText ) override;
    virtual bool ReplaceRange( sal_Int32 nPos, sal_Int32 nLen, const OUString& rText ) override;

    virtual void SetAttr( sal_Int32 nStt, sal_Int32 nEnd, sal_uInt16 nSlotId,
                            SfxPoolItem& ) override;

    virtual bool SetINetAttr( sal_Int32 nStt, sal_Int32 nEnd, const OUString& rURL ) override;

    // return text of a previous paragraph
    // If it does not exist or if there is nothing before, return blank.
    //  - true:  paragraph before "normal" insertion position
    //  - false: paragraph in that the corrected word was inserted
    //               (does not need to be the same paragraph)
    virtual OUString const* GetPrevPara(bool bAtNormalPos) override;

    virtual bool ChgAutoCorrWord( sal_Int32& rSttPos, sal_Int32 nEndPos,
                                  SvxAutoCorrect& rACorrect,
                                  OUString* pPara ) override;
    virtual bool TransliterateRTLWord( sal_Int32& rSttPos, sal_Int32 nEndPos,
                                  bool bApply = false ) override;

    // Will be called after swapping characters by the functions
    //  - FnCapitalStartWord and
    //  - FnCapitalStartSentence.
    // Afterwards the words can be added into exception list if needed.
    virtual void SaveCpltSttWord( ACFlags nFlag, sal_Int32 nPos,
                                    const OUString& rExceptWord, sal_Unicode cChar ) override;
    virtual LanguageType GetLanguage( sal_Int32 nPos ) const override;
};

class SwAutoCorrExceptWord
{
    OUString m_sWord;
    SwNodeOffset m_nNode;
    ACFlags m_nFlags;
    sal_Int32 m_nContent;
    sal_Unicode m_cChar;
    LanguageType m_eLanguage;
    bool m_bDeleted;

public:
    SwAutoCorrExceptWord(ACFlags nAFlags, SwNodeOffset nNd, sal_Int32 nContent,
                         OUString aWord, sal_Unicode cChr,
                         LanguageType eLang)
        : m_sWord(std::move(aWord)), m_nNode(nNd), m_nFlags(nAFlags), m_nContent(nContent),
          m_cChar(cChr), m_eLanguage(eLang), m_bDeleted(false)
    {}

    bool IsDeleted() const { return m_bDeleted; }
    void CheckChar(const SwPosition& rPos, sal_Unicode cChar);
    bool CheckDelChar(const SwPosition& rPos);
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
