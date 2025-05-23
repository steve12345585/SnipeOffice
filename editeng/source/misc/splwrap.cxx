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

#include <config_wasm_strip.h>

#include <rtl/ustring.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <svtools/langtab.hxx>

#include <vcl/errinf.hxx>
#include <editeng/unolingu.hxx>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/linguistic2/XLinguProperties.hpp>
#include <com/sun/star/linguistic2/XSpellChecker1.hpp>
#include <com/sun/star/linguistic2/XHyphenator.hpp>
#include <com/sun/star/linguistic2/XSearchableDictionaryList.hpp>
#include <com/sun/star/linguistic2/XDictionary.hpp>

#include <editeng/svxenum.hxx>
#include <editeng/splwrap.hxx>
#include <editeng/edtdlg.hxx>
#include <editeng/eerdll.hxx>
#include <editeng/editrids.hrc>
#include <editeng/editerr.hxx>

#include <map>
#include <memory>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::linguistic2;


// misc functions ---------------------------------------------

void SvxPrepareAutoCorrect( OUString &rOldText, std::u16string_view rNewText )
{
    // This function should be used to strip (or add) trailing '.' from
    // the strings before passing them on to the autocorrect function in
    // order that the autocorrect function will hopefully
    // works properly with normal words and abbreviations (with trailing '.')
    // independent of if they are at the end of the sentence or not.
    //
    // rOldText: text to be replaced
    // rNewText: replacement text

    sal_Int32 nOldLen = rOldText.getLength();
    sal_Int32 nNewLen = rNewText.size();
    if (nOldLen && nNewLen)
    {
        bool bOldHasDot = '.' == rOldText[ nOldLen - 1 ],
             bNewHasDot = '.' == rNewText[ nNewLen - 1 ];
        if (bOldHasDot && !bNewHasDot
            /*this is: !(bOldHasDot && bNewHasDot) && bOldHasDot*/)
            rOldText = rOldText.copy( 0, nOldLen - 1 );
    }
}

#define SVX_LANG_NEED_CHECK         0
#define SVX_LANG_OK                 1
#define SVX_LANG_MISSING            2
#define SVX_LANG_MISSING_DO_WARN    3

typedef std::map< LanguageType, sal_uInt16 >   LangCheckState_map_t;

static LangCheckState_map_t & GetLangCheckState()
{
    static LangCheckState_map_t aLangCheckState;
    return aLangCheckState;
}

void SvxSpellWrapper::ShowLanguageErrors()
{
    // display message boxes for languages not available for
    // spellchecking or hyphenation
    LangCheckState_map_t &rLCS = GetLangCheckState();
    for (auto const& elem : rLCS)
    {
        LanguageType nLang = elem.first;
        sal_uInt16   nVal  = elem.second;
        sal_uInt16 nTmpSpell = nVal & 0x00FF;
        sal_uInt16 nTmpHyph  = (nVal >> 8) & 0x00FF;

        if (SVX_LANG_MISSING_DO_WARN == nTmpSpell)
        {
            OUString aErr( SvtLanguageTable::GetLanguageString( nLang ) );
            ErrorHandler::HandleError(
                ErrCodeMsg( ERRCODE_SVX_LINGU_LANGUAGENOTEXISTS, aErr ) );
            nTmpSpell = SVX_LANG_MISSING;
        }
        if (SVX_LANG_MISSING_DO_WARN == nTmpHyph)
        {
            OUString aErr( SvtLanguageTable::GetLanguageString( nLang ) );
            ErrorHandler::HandleError(
                ErrCodeMsg( ERRCODE_SVX_LINGU_LANGUAGENOTEXISTS, aErr ) );
            nTmpHyph = SVX_LANG_MISSING;
        }

        rLCS[ nLang ] = (nTmpHyph << 8) | nTmpSpell;
    }

}

SvxSpellWrapper::~SvxSpellWrapper()
{
}

/*--------------------------------------------------------------------
 *  Description: Constructor, the test sequence is determined
 *
 *  !bStart && !bOtherCntnt:    BODY_END,   BODY_START, OTHER
 *  !bStart && bOtherCntnt:     OTHER,      BODY
 *  bStart && !bOtherCntnt:     BODY_END,   OTHER
 *  bStart && bOtherCntnt:      OTHER
 *
 --------------------------------------------------------------------*/

SvxSpellWrapper::SvxSpellWrapper( weld::Widget* pWn,
    const bool bStart, const bool bIsAllRight ) :

    pWin        ( pWn ),
    bOtherCntnt ( false ),
    bStartChk   ( false ),
    bRevAllowed ( true ),
    bAllRight   ( bIsAllRight )
{
    Reference< linguistic2::XLinguProperties >  xProp( LinguMgr::GetLinguPropertySet() );
    bool bWrapReverse = xProp.is() && xProp->getIsWrapReverse();
    bReverse = bWrapReverse;
    bStartDone = !bReverse && bStart;
    bEndDone   = bReverse && bStart;
}


SvxSpellWrapper::SvxSpellWrapper( weld::Widget* pWn,
        Reference< XHyphenator > const &xHyphenator,
        const bool bStart, const bool bOther ) :
    pWin        ( pWn ),
    xHyph       ( xHyphenator ),
    bOtherCntnt ( bOther ),
    bReverse    ( false ),
    bStartDone  ( bOther || bStart ),
    bEndDone    ( false ),
    bStartChk   ( bOther ),
    bRevAllowed ( false ),
    bAllRight   ( true )
{
}


sal_Int16 SvxSpellWrapper::CheckSpellLang(
        Reference< XSpellChecker1 > const & xSpell, LanguageType nLang)
{
    LangCheckState_map_t &rLCS = GetLangCheckState();

    LangCheckState_map_t::iterator aIt( rLCS.find( nLang ) );
    sal_uInt16 nVal = aIt == rLCS.end() ? SVX_LANG_NEED_CHECK : aIt->second;

    if (aIt == rLCS.end())
        rLCS[ nLang ] = nVal;

    if (SVX_LANG_NEED_CHECK == (nVal & 0x00FF))
    {
        sal_uInt16 nTmpVal = SVX_LANG_MISSING_DO_WARN;
        if (xSpell.is()  &&  xSpell->hasLanguage( static_cast<sal_uInt16>(nLang) ))
            nTmpVal = SVX_LANG_OK;
        nVal &= 0xFF00;
        nVal |= nTmpVal;

        rLCS[ nLang ] = nVal;
    }

    return static_cast<sal_Int16>(nVal);
}

sal_Int16 SvxSpellWrapper::CheckHyphLang(
        Reference< XHyphenator > const & xHyph, LanguageType nLang)
{
    LangCheckState_map_t &rLCS = GetLangCheckState();

    LangCheckState_map_t::iterator aIt( rLCS.find( nLang ) );
    sal_uInt16 nVal = aIt == rLCS.end() ? 0 : aIt->second;

    if (aIt == rLCS.end())
        rLCS[ nLang ] = nVal;

    if (SVX_LANG_NEED_CHECK == ((nVal >> 8) & 0x00FF))
    {
        sal_uInt16 nTmpVal = SVX_LANG_MISSING_DO_WARN;
        if (xHyph.is()  &&  xHyph->hasLocale( LanguageTag::convertToLocale( nLang ) ))
            nTmpVal = SVX_LANG_OK;
        nVal &= 0x00FF;
        nVal |= nTmpVal << 8;

        rLCS[ nLang ] = nVal;
    }

    return static_cast<sal_Int16>(nVal);
}


void SvxSpellWrapper::SpellStart( SvxSpellArea /*eSpell*/ )
{ // Here, the necessary preparations be made for SpellContinue in the
} // given area.


bool SvxSpellWrapper::SpellMore()
{
    return false; // Should additional documents be examined?
}


void SvxSpellWrapper::SpellEnd()
{   // Area is complete, tidy up if necessary

    // display error for last language not found
    ShowLanguageErrors();
}

void SvxSpellWrapper::SpellContinue()
{
}

void SvxSpellWrapper::ReplaceAll( const OUString & )
{   // Replace Word from the Replace list
}

void SvxSpellWrapper::InsertHyphen( const sal_Int32 )
{   // inserting and deleting Hyphen
}

// Testing of the document areas in the order specified by the flags
void SvxSpellWrapper::SpellDocument( )
{
#if ENABLE_WASM_STRIP_HUNSPELL
    return;
#else
    if ( bOtherCntnt )
    {
        bReverse = false;
        SpellStart( SvxSpellArea::Other );
    }
    else
    {
        bStartChk = bReverse;
        SpellStart( bReverse ? SvxSpellArea::BodyStart : SvxSpellArea::BodyEnd );
    }

    if ( !FindSpellError() )
        return;

    Reference< XHyphenatedWord >        xHyphWord( GetLast(), UNO_QUERY );

    if (xHyphWord.is())
    {
        EditAbstractDialogFactory* pFact = EditAbstractDialogFactory::Create();
        ScopedVclPtr<AbstractHyphenWordDialog> pDlg(pFact->CreateHyphenWordDialog(
                        pWin,
                        xHyphWord->getWord(),
                        LanguageTag( xHyphWord->getLocale() ).getLanguageType(),
                        xHyph, this ));
        pDlg->Execute();
    }
#endif
}


// Select the next area


bool SvxSpellWrapper::SpellNext( )
{
    Reference< linguistic2::XLinguProperties >  xProp( LinguMgr::GetLinguPropertySet() );
    bool bWrapReverse = xProp.is() && xProp->getIsWrapReverse();
    bool bActRev = bRevAllowed && bWrapReverse;

    // bActRev is the direction after Spell checking, bReverse is the one
    // at the beginning.
    if( bActRev == bReverse )
    {                           // No change of direction, thus is the
        if( bStartChk )         // desired area ( bStartChk )
            bStartDone = true;  // completely processed.
        else
            bEndDone = true;
    }
    else if( bReverse == bStartChk ) //For a change of direction, an area can
    {                          // be processed during certain circumstances
        if( bStartChk )        // If the first part is spell checked in backwards
            bEndDone = true;   // and this is reversed in the process, then
        else                   // then the end part is processed (and vice-versa).
            bStartDone = true;
    }

    bReverse = bActRev;
    if( bOtherCntnt && bStartDone && bEndDone ) // Document has been fully checked?
    {
        if ( SpellMore() )  // spell check another document?
        {
            bOtherCntnt = false;
            bStartDone = !bReverse;
            bEndDone  = bReverse;
            SpellStart( SvxSpellArea::Body );
            return true;
        }
        return false;
    }

    bool bGoOn = false;

    if ( bOtherCntnt )
    {
        bStartChk = false;
        SpellStart( SvxSpellArea::Body );
        bGoOn = true;
    }
    else if ( bStartDone && bEndDone )
    {
        if ( SpellMore() )  // check another document?
        {
            bOtherCntnt = false;
            bStartDone = !bReverse;
            bEndDone  = bReverse;
            SpellStart( SvxSpellArea::Body );
            return true;
        }
    }
    else
    {
        // a BODY_area done, ask for the other BODY_area
        xWait.reset();

        TranslateId pResId = bReverse ? RID_SVXSTR_QUERY_BW_CONTINUE : RID_SVXSTR_QUERY_CONTINUE;
        std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(pWin,
                                                                 VclMessageType::Question, VclButtonsType::YesNo,
                                                                 EditResId(pResId)));
        if (xBox->run() != RET_YES)
        {
            // sacrifice the other area if necessary ask for special area
            xWait.reset(new weld::WaitObject(pWin));
            bStartDone = bEndDone = true;
            return SpellNext();
        }
        else
        {
            bStartChk = !bStartDone;
            SpellStart( bStartChk ? SvxSpellArea::BodyStart : SvxSpellArea::BodyEnd );
            bGoOn = true;
        }
        xWait.reset(new weld::WaitObject(pWin));
    }
    return bGoOn;
}


Reference< XDictionary >  SvxSpellWrapper::GetAllRightDic()
{
    Reference< XDictionary >  xDic;

    Reference< XSearchableDictionaryList >  xDicList( LinguMgr::GetDictionaryList() );
    if (xDicList.is())
    {
        Sequence< Reference< XDictionary >  > aDics( xDicList->getDictionaries() );
        const Reference< XDictionary >  *pDic = aDics.getConstArray();
        sal_Int32 nCount = aDics.getLength();

        sal_Int32 i = 0;
        while (!xDic.is()  &&  i < nCount)
        {
            Reference< XDictionary >  xTmp = pDic[i];
            if (xTmp.is())
            {
                if ( xTmp->isActive() &&
                     xTmp->getDictionaryType() != DictionaryType_NEGATIVE &&
                     LanguageTag( xTmp->getLocale() ).getLanguageType() == LANGUAGE_NONE )
                {
                    Reference< frame::XStorable >  xStor( xTmp, UNO_QUERY );
                    if (xStor.is() && xStor->hasLocation() && !xStor->isReadonly())
                    {
                        xDic = std::move(xTmp);
                    }
                }
            }
            ++i;
        }

        if (!xDic.is())
        {
            xDic = LinguMgr::GetStandardDic();
            if (xDic.is())
                xDic->setActive( true );
        }
    }

    return xDic;
}


bool SvxSpellWrapper::FindSpellError()
{
    ShowLanguageErrors();

    xWait.reset(new weld::WaitObject(pWin));
    bool bSpell = true;

    Reference< XDictionary >  xAllRightDic;
    if (IsAllRight())
        xAllRightDic = GetAllRightDic();

    while ( bSpell )
    {
        SpellContinue();

        Reference< XSpellAlternatives >     xAlt( GetLast(), UNO_QUERY );
        Reference< XHyphenatedWord >        xHyphWord( GetLast(), UNO_QUERY );

        if (xAlt.is())
        {
            if (IsAllRight() && xAllRightDic.is())
            {
                xAllRightDic->add( xAlt->getWord(), false, OUString() );
            }
            else
            {
                // look up in ChangeAllList for misspelled word
                Reference< XDictionary >    xChangeAllList =
                        LinguMgr::GetChangeAllList();
                Reference< XDictionaryEntry >   xEntry;
                if (xChangeAllList.is())
                    xEntry = xChangeAllList->getEntry( xAlt->getWord() );

                if (xEntry.is())
                {
                    // replace word without asking
                    ReplaceAll( xEntry->getReplacementText() );
                }
                else
                    bSpell = false;
            }
        }
        else if (xHyphWord.is())
            bSpell = false;
        else
        {
            SpellEnd();
            bSpell = SpellNext();
        }
    }
    xWait.reset();
    return GetLast().is();
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
