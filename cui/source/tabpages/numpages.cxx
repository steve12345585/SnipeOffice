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

#include <com/sun/star/text/VertOrientation.hpp>

#include <numpages.hxx>
#include <dialmgr.hxx>
#include <tools/mapunit.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <i18nlangtag/mslangid.hxx>
#include <editeng/numitem.hxx>
#include <svl/eitem.hxx>
#include <vcl/svapp.hxx>
#include <svx/colorbox.hxx>
#include <svx/dlgutil.hxx>
#include <svx/strarray.hxx>
#include <svx/gallery.hxx>
#include <editeng/brushitem.hxx>
#include <svl/intitem.hxx>
#include <sfx2/objsh.hxx>
#include <vcl/graph.hxx>
#include <vcl/settings.hxx>
#include <svx/cuicharmap.hxx>
#include <editeng/flstitem.hxx>
#include <svx/numvset.hxx>
#include <sfx2/htmlmode.hxx>
#include <unotools/pathoptions.hxx>
#include <svtools/ctrltool.hxx>
#include <svtools/unitconv.hxx>
#include <svtools/colorcfg.hxx>
#include <com/sun/star/style/NumberingType.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/text/XDefaultNumberingProvider.hpp>
#include <com/sun/star/text/XNumberingFormatter.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/propertyvalue.hxx>
#include <comphelper/lok.hxx>
#include <svx/svxids.hrc>
#include <o3tl/string_view.hxx>
#include <officecfg/Office/Common.hxx>

#include <algorithm>
#include <memory>
#include <vector>
#include <sfx2/opengrf.hxx>

#include <strings.hrc>
#include <svl/stritem.hxx>
#include <svl/slstitm.hxx>
#include <sfx2/filedlghelper.hxx>
#include <unotools/ucbstreamhelper.hxx>
#include <com/sun/star/ucb/SimpleFileAccess.hpp>
#include <sal/log.hxx>
#include <vcl/cvtgrf.hxx>
#include <vcl/graphicfilter.hxx>
#include <svx/SvxNumOptionsTabPageHelper.hxx>
#include <tools/urlobj.hxx>
#include <o3tl/temporary.hxx>
#include <osl/diagnose.h>

#include <bitmaps.hlst>

using namespace css;
using namespace css::uno;
using namespace css::beans;
using namespace css::lang;
using namespace css::text;
using namespace css::container;

#define MAX_BMP_WIDTH               16
#define MAX_BMP_HEIGHT              16
#define SEARCHPATH_DELIMITER        u';'
#define SEARCHFILENAME_DELIMITER    u'/'

static bool bLastRelative =         false;

static SvxNumSettings_Impl* lcl_CreateNumSettingsPtr(const Sequence<PropertyValue>& rLevelProps)
{
    SvxNumSettings_Impl* pNew = new SvxNumSettings_Impl;
    for (auto& prop : rLevelProps)
    {
        if (prop.Name == "NumberingType")
        {
            sal_Int16 nTmp;
            if (prop.Value >>= nTmp)
                pNew->nNumberType = static_cast<SvxNumType>(nTmp);
        }
        else if (prop.Name == "Prefix")
            prop.Value >>= pNew->sPrefix;
        else if (prop.Name == "Suffix")
            prop.Value >>= pNew->sSuffix;
        else if (prop.Name == "ParentNumbering")
            prop.Value >>= pNew->nParentNumbering;
        else if (prop.Name == "BulletChar")
            prop.Value >>= pNew->sBulletChar;
        else if (prop.Name == "BulletFontName")
            prop.Value >>= pNew->sBulletFont;
    }
    return pNew;
}

// Is one of the masked formats set?
static bool lcl_IsNumFmtSet(SvxNumRule const * pNum, sal_uInt16 nLevelMask)
{
    bool bRet = false;
    sal_uInt16 nMask = 1;
    for( sal_uInt16 i = 0; i < SVX_MAX_NUM && !bRet; i++ )
    {
        if(nLevelMask & nMask)
            bRet |= nullptr != pNum->Get( i );
        nMask <<= 1 ;
    }
    return bRet;
}

static vcl::Font& lcl_GetDefaultBulletFont()
{
    static vcl::Font aDefBulletFont = []()
    {
        vcl::Font tmp(u"OpenSymbol"_ustr, u""_ustr, Size(0, 14));
        tmp.SetCharSet( RTL_TEXTENCODING_SYMBOL );
        tmp.SetFamily( FAMILY_DONTKNOW );
        tmp.SetPitch( PITCH_DONTKNOW );
        tmp.SetWeight( WEIGHT_DONTKNOW );
        tmp.SetTransparent( true );
        return tmp;
    }();
    return aDefBulletFont;
}

SvxSingleNumPickTabPage::SvxSingleNumPickTabPage(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rSet)
    : SfxTabPage(pPage, pController, u"cui/ui/picknumberingpage.ui"_ustr, u"PickNumberingPage"_ustr, &rSet)
    , nActNumLvl(SAL_MAX_UINT16)
    , bModified(false)
    , bPreset(false)
    , nNumItemId(SID_ATTR_NUMBERING_RULE)
    , m_xExamplesVS(new SvxNumValueSet(m_xBuilder->weld_scrolled_window(u"valuesetwin"_ustr, true)))
    , m_xExamplesVSWin(new weld::CustomWeld(*m_xBuilder, u"valueset"_ustr, *m_xExamplesVS))
{
    SetExchangeSupport();
    m_xExamplesVS->init(NumberingPageType::SINGLENUM);
    m_xExamplesVS->SetSelectHdl(LINK(this, SvxSingleNumPickTabPage, NumSelectHdl_Impl));
    m_xExamplesVS->SetDoubleClickHdl(LINK(this, SvxSingleNumPickTabPage, DoubleClickHdl_Impl));

    Reference<XDefaultNumberingProvider> xDefNum = SvxNumOptionsTabPageHelper::GetNumberingProvider();
    if(!xDefNum.is())
        return;

    Sequence< Sequence< PropertyValue > > aNumberings;
    const Locale& rLocale = Application::GetSettings().GetLanguageTag().getLocale();
    try
    {
        aNumberings =
            xDefNum->getDefaultContinuousNumberingLevels( rLocale );

        sal_Int32 nLength = std::min<sal_Int32>(aNumberings.getLength(), NUM_VALUSET_COUNT);
        for(sal_Int32 i = 0; i < nLength; i++)
        {
            SvxNumSettings_Impl* pNew = lcl_CreateNumSettingsPtr(aNumberings[i]);
            aNumSettingsArr.push_back(std::unique_ptr<SvxNumSettings_Impl>(pNew));
        }
    }
    catch(const Exception&)
    {
    }
    Reference<XNumberingFormatter> xFormat(xDefNum, UNO_QUERY);
    m_xExamplesVS->SetNumberingSettings(aNumberings, xFormat, rLocale);
}

SvxSingleNumPickTabPage::~SvxSingleNumPickTabPage()
{
    m_xExamplesVSWin.reset();
    m_xExamplesVS.reset();
}

std::unique_ptr<SfxTabPage> SvxSingleNumPickTabPage::Create(weld::Container* pPage, weld::DialogController* pController,
                                                   const SfxItemSet* rAttrSet)
{
    return std::make_unique<SvxSingleNumPickTabPage>(pPage, pController, *rAttrSet);
}

bool  SvxSingleNumPickTabPage::FillItemSet( SfxItemSet* rSet )
{
    if( (bPreset || bModified) && pSaveNum)
    {
        *pSaveNum = *pActNum;
        rSet->Put(SvxNumBulletItem( *pSaveNum, nNumItemId ));
        rSet->Put(SfxBoolItem(SID_PARAM_NUM_PRESET, bPreset));
    }

    return bModified;
}

void  SvxSingleNumPickTabPage::ActivatePage(const SfxItemSet& rSet)
{
    bPreset = false;
    bool bIsPreset = false;
    const SfxItemSet* pExampleSet = GetDialogExampleSet();
    if(pExampleSet)
    {
        if(const SfxBoolItem* pPresetItem = pExampleSet->GetItemIfSet(SID_PARAM_NUM_PRESET, false))
            bIsPreset = pPresetItem->GetValue();
        if(const SfxUInt16Item* pLevelItem = pExampleSet->GetItemIfSet(SID_PARAM_CUR_NUM_LEVEL, false))
            nActNumLvl = pLevelItem->GetValue();
    }
    if(const SvxNumBulletItem* pNumItem = rSet.GetItemIfSet(nNumItemId, false))
    {
        pSaveNum.reset( new SvxNumRule(pNumItem->GetNumRule()) );
    }
    if(pActNum && *pSaveNum != *pActNum)
    {
        *pActNum = *pSaveNum;
        m_xExamplesVS->SetNoSelection();
    }

    if(pActNum && (!lcl_IsNumFmtSet(pActNum.get(), nActNumLvl) || bIsPreset))
    {
        m_xExamplesVS->SelectItem(1);
        NumSelectHdl_Impl(m_xExamplesVS.get());
        bPreset = true;
    }
    bPreset |= bIsPreset;

    bModified = false;
}

DeactivateRC SvxSingleNumPickTabPage::DeactivatePage(SfxItemSet *_pSet)
{
    if(_pSet)
        FillItemSet(_pSet);
    return DeactivateRC::LeavePage;
}

void  SvxSingleNumPickTabPage::Reset( const SfxItemSet* rSet )
{
    const SfxPoolItem* pItem;

    // in Draw the item exists as WhichId, in Writer only as SlotId
    SfxItemState eState = rSet->GetItemState(SID_ATTR_NUMBERING_RULE, false, &pItem);
    if(eState != SfxItemState::SET)
    {
        nNumItemId = rSet->GetPool()->GetWhichIDFromSlotID(SID_ATTR_NUMBERING_RULE);
        eState = rSet->GetItemState(nNumItemId, false, &pItem);

        if( eState != SfxItemState::SET )
        {
            pItem = & rSet->Get( nNumItemId );
            eState = SfxItemState::SET;
        }
    }
    DBG_ASSERT(eState == SfxItemState::SET, "no item found!");
    pSaveNum.reset( new SvxNumRule(static_cast<const SvxNumBulletItem*>(pItem)->GetNumRule()) );

    if(!pActNum)
        pActNum.reset( new SvxNumRule(*pSaveNum) );
    else if(*pSaveNum != *pActNum)
        *pActNum = *pSaveNum;
}

IMPL_LINK_NOARG(SvxSingleNumPickTabPage, NumSelectHdl_Impl, ValueSet*, void)
{
    if(!pActNum)
        return;

    bPreset = false;
    bModified = true;
    sal_uInt16 nIdx = m_xExamplesVS->GetSelectedItemId() - 1;
    DBG_ASSERT(aNumSettingsArr.size() > nIdx, "wrong index");
    if(aNumSettingsArr.size() <= nIdx)
        return;
    SvxNumSettings_Impl* _pSet = aNumSettingsArr[nIdx].get();
    SvxNumType eNewType = _pSet->nNumberType;
    const sal_Unicode cLocalPrefix = !_pSet->sPrefix.isEmpty() ? _pSet->sPrefix[0] : 0;
    const sal_Unicode cLocalSuffix = !_pSet->sSuffix.isEmpty() ? _pSet->sSuffix[0] : 0;

    sal_uInt16 nMask = 1;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aFmt(pActNum->GetLevel(i));
            aFmt.SetNumberingType(eNewType);
            aFmt.SetListFormat(cLocalPrefix == ' ' ? u""_ustr : _pSet->sPrefix,
                               cLocalSuffix == ' ' ? u""_ustr : _pSet->sSuffix, i);
            aFmt.SetCharFormatName(u""_ustr);
            aFmt.SetBulletRelSize(100);
            pActNum->SetLevel(i, aFmt);
        }
        nMask <<= 1;
    }
}

IMPL_LINK_NOARG(SvxSingleNumPickTabPage, DoubleClickHdl_Impl, ValueSet*, void)
{
    NumSelectHdl_Impl(m_xExamplesVS.get());
    weld::Button& rOk = GetDialogController()->GetOKButton();
    rOk.clicked();
}

SvxBulletPickTabPage::SvxBulletPickTabPage(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rSet)
    : SfxTabPage(pPage, pController, u"cui/ui/pickbulletpage.ui"_ustr, u"PickBulletPage"_ustr, &rSet)
    , nActNumLvl(SAL_MAX_UINT16)
    , bModified(false)
    , bPreset(false)
    , nNumItemId(SID_ATTR_NUMBERING_RULE)
    , m_xBtChangeBullet(m_xBuilder->weld_button(u"changeBulletBtn"_ustr))
    , m_xExamplesVS(new SvxNumValueSet(m_xBuilder->weld_scrolled_window(u"valuesetwin"_ustr, true)))
    , m_xExamplesVSWin(new weld::CustomWeld(*m_xBuilder, u"valueset"_ustr, *m_xExamplesVS))
{
    SetExchangeSupport();
    m_xBtChangeBullet->set_sensitive(false);
    m_xExamplesVS->init(NumberingPageType::BULLET);
    m_xExamplesVS->SetSelectHdl(LINK(this, SvxBulletPickTabPage, NumSelectHdl_Impl));
    m_xExamplesVS->SetDoubleClickHdl(LINK(this, SvxBulletPickTabPage, DoubleClickHdl_Impl));
    m_xBtChangeBullet->connect_clicked(LINK(this, SvxBulletPickTabPage, ClickAddChangeHdl_Impl));
    m_aBulletSymbols = officecfg::Office::Common::BulletsNumbering::DefaultBullets::get();
    m_aBulletSymbolsFonts = officecfg::Office::Common::BulletsNumbering::DefaultBulletsFonts::get();
}

SvxBulletPickTabPage::~SvxBulletPickTabPage()
{
    m_xExamplesVSWin.reset();
    m_xExamplesVS.reset();
}

std::unique_ptr<SfxTabPage> SvxBulletPickTabPage::Create(weld::Container* pPage, weld::DialogController* pController,
                                                const SfxItemSet* rAttrSet)
{
    return std::make_unique<SvxBulletPickTabPage>(pPage, pController, *rAttrSet);
}

bool  SvxBulletPickTabPage::FillItemSet( SfxItemSet* rSet )
{
    if( (bPreset || bModified) && pActNum)
    {
        *pSaveNum = *pActNum;
        rSet->Put(SvxNumBulletItem( *pSaveNum, nNumItemId ));
        rSet->Put(SfxBoolItem(SID_PARAM_NUM_PRESET, bPreset));
    }
    return bModified;
}

void  SvxBulletPickTabPage::ActivatePage(const SfxItemSet& rSet)
{
    bPreset = false;
    bool bIsPreset = false;
    const SfxItemSet* pExampleSet = GetDialogExampleSet();
    if(pExampleSet)
    {
        if(const SfxBoolItem* pPresetItem = pExampleSet->GetItemIfSet(SID_PARAM_NUM_PRESET, false))
            bIsPreset = pPresetItem->GetValue();
        if(const SfxUInt16Item* pLevelItem = pExampleSet->GetItemIfSet(SID_PARAM_CUR_NUM_LEVEL, false))
            nActNumLvl = pLevelItem->GetValue();
    }
    if(const SvxNumBulletItem* pBulletItem = rSet.GetItemIfSet(nNumItemId, false))
    {
        pSaveNum.reset( new SvxNumRule(pBulletItem->GetNumRule()) );
    }
    if(pActNum && *pSaveNum != *pActNum)
    {
        *pActNum = *pSaveNum;
        m_xExamplesVS->SetNoSelection();
    }

    if(pActNum && (!lcl_IsNumFmtSet(pActNum.get(), nActNumLvl) || bIsPreset))
    {
        m_xExamplesVS->SelectItem(1);
        NumSelectHdl_Impl(m_xExamplesVS.get());
        bPreset = true;
    }
    bPreset |= bIsPreset;
    bModified = false;
}

DeactivateRC SvxBulletPickTabPage::DeactivatePage(SfxItemSet *_pSet)
{
    if (IsCancelMode())
    {
        // Dialog cancelled, restore previous bullets
        std::shared_ptr<comphelper::ConfigurationChanges> batch(comphelper::ConfigurationChanges::create());
        officecfg::Office::Common::BulletsNumbering::DefaultBullets::set(m_aBulletSymbols, batch);
        officecfg::Office::Common::BulletsNumbering::DefaultBulletsFonts::set(m_aBulletSymbolsFonts, batch);
        batch->commit();
    }
    if(_pSet)
        FillItemSet(_pSet);
    return DeactivateRC::LeavePage;
}

void  SvxBulletPickTabPage::Reset( const SfxItemSet* rSet )
{
    // in Draw the item exists as WhichId, in Writer only as SlotId
    const SvxNumBulletItem* pItem = rSet->GetItemIfSet(SID_ATTR_NUMBERING_RULE, false);
    if(!pItem)
    {
        nNumItemId = rSet->GetPool()->GetWhichIDFromSlotID(SID_ATTR_NUMBERING_RULE);
        pItem = rSet->GetItemIfSet(nNumItemId, false);

        if( !pItem )
        {
            pItem = & rSet->Get( nNumItemId );
        }

    }
    pSaveNum.reset( new SvxNumRule(pItem->GetNumRule()) );

    if(!pActNum)
        pActNum.reset( new SvxNumRule(*pSaveNum) );
    else if(*pSaveNum != *pActNum)
        *pActNum = *pSaveNum;
}

IMPL_LINK_NOARG(SvxBulletPickTabPage, NumSelectHdl_Impl, ValueSet*, void)
{
    if(!pActNum)
        return;

    m_xBtChangeBullet->set_sensitive(true);

    bPreset = false;
    bModified = true;
    sal_uInt16 nIndex = m_xExamplesVS->GetSelectedItemId() - 1;
    sal_Unicode cChar = m_aBulletSymbols[nIndex].toChar();
    vcl::Font& rActBulletFont = lcl_GetDefaultBulletFont();
    rActBulletFont.SetFamilyName(m_aBulletSymbolsFonts[nIndex]);

    sal_uInt16 nMask = 1;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aFmt(pActNum->GetLevel(i));
            aFmt.SetNumberingType( SVX_NUM_CHAR_SPECIAL );
            // #i93908# clear suffix for bullet lists
            aFmt.SetListFormat(u""_ustr, u""_ustr, i);
            aFmt.SetBulletFont(&rActBulletFont);
            aFmt.SetBulletChar(cChar );
            aFmt.SetCharFormatName(sBulletCharFormatName);
            aFmt.SetBulletRelSize(45);
            pActNum->SetLevel(i, aFmt);
        }
        nMask <<= 1;
    }
}

IMPL_LINK_NOARG(SvxBulletPickTabPage, DoubleClickHdl_Impl, ValueSet*, void)
{
    NumSelectHdl_Impl(m_xExamplesVS.get());
    weld::Button& rOk = GetDialogController()->GetOKButton();
    rOk.clicked();
}

IMPL_LINK_NOARG(SvxBulletPickTabPage, ClickAddChangeHdl_Impl, weld::Button&, void)
{
    SvxCharacterMap aMap(GetFrameWeld(), nullptr, nullptr);

    sal_uInt16 nMask = 1;
    std::optional<vcl::Font> pFmtFont;
    bool bSameBullet = true;
    sal_UCS4 cBullet = 0;
    bool bFirst = true;
    for (sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if (nActNumLvl & nMask)
        {
            const SvxNumberFormat& rCurFmt = pActNum->GetLevel(i);
            if (bFirst)
            {
                cBullet = rCurFmt.GetBulletChar();
            }
            else if (rCurFmt.GetBulletChar() != cBullet)
            {
                bSameBullet = false;
                break;
            }
            if (!pFmtFont)
                pFmtFont = rCurFmt.GetBulletFont();
            bFirst = false;
        }
        nMask <<= 1;
    }

    if (pFmtFont)
        aMap.SetCharFont(*pFmtFont);
    if (bSameBullet)
        aMap.SetChar(cBullet);
    if (aMap.run() != RET_OK)
        return;

    sal_Unicode cChar = aMap.GetChar();
    vcl::Font aActBulletFont = aMap.GetCharFont();

    sal_uInt16 _nMask = 1;
    for (sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if (nActNumLvl & _nMask)
        {
            SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
            aNumFmt.SetBulletFont(&aActBulletFont);
            aNumFmt.SetBulletChar(cChar);
            pActNum->SetLevel(i, aNumFmt);
        }
        _nMask <<= 1;
    }

    css::uno::Sequence<OUString> aBulletSymbolsList(m_aBulletSymbols.size());
    css::uno::Sequence<OUString> aBulletSymbolsFontsList(m_aBulletSymbolsFonts.size());
    auto aBulletSymbolsListRange = asNonConstRange(aBulletSymbolsList);
    auto aBulletSymbolsFontsListRange = asNonConstRange(aBulletSymbolsFontsList);

    sal_uInt16 nIndex = m_xExamplesVS->GetSelectedItemId() - 1;
    for (size_t i = 0; i < m_aBulletSymbols.size(); ++i)
    {
        if (i == nIndex)
        {
            aBulletSymbolsListRange[i] = OUStringChar(cChar);
            aBulletSymbolsFontsListRange[i] = aActBulletFont.GetFamilyName();
        }
        else
        {
            aBulletSymbolsListRange[i] = m_aBulletSymbols[i];
            aBulletSymbolsFontsListRange[i] = m_aBulletSymbolsFonts[i];
        }
    }

    std::shared_ptr<comphelper::ConfigurationChanges> batch(comphelper::ConfigurationChanges::create());
    officecfg::Office::Common::BulletsNumbering::DefaultBullets::set(aBulletSymbolsList, batch);
    officecfg::Office::Common::BulletsNumbering::DefaultBulletsFonts::set(aBulletSymbolsFontsList, batch);
    batch->commit();

    m_xExamplesVS->SetFormat();
    m_xExamplesVS->Invalidate();
}

void SvxBulletPickTabPage::PageCreated(const SfxAllItemSet& aSet)
{
    const SfxStringItem* pBulletCharFmt = aSet.GetItem<SfxStringItem>(SID_BULLET_CHAR_FMT, false);

    if (pBulletCharFmt)
        sBulletCharFormatName = pBulletCharFmt->GetValue();
}

SvxNumPickTabPage::SvxNumPickTabPage(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rSet)
    : SfxTabPage(pPage, pController, u"cui/ui/pickoutlinepage.ui"_ustr, u"PickOutlinePage"_ustr, &rSet)
    , nActNumLvl(SAL_MAX_UINT16)
    , nNumItemId(SID_ATTR_NUMBERING_RULE)
    , bModified(false)
    , bPreset(false)
    , m_xExamplesVS(new SvxNumValueSet(m_xBuilder->weld_scrolled_window(u"valuesetwin"_ustr, true)))
    , m_xExamplesVSWin(new weld::CustomWeld(*m_xBuilder, u"valueset"_ustr, *m_xExamplesVS))
{
    SetExchangeSupport();

    m_xExamplesVS->init(NumberingPageType::OUTLINE);
    m_xExamplesVS->SetSelectHdl(LINK(this, SvxNumPickTabPage, NumSelectHdl_Impl));
    m_xExamplesVS->SetDoubleClickHdl(LINK(this, SvxNumPickTabPage, DoubleClickHdl_Impl));

    Reference<XDefaultNumberingProvider> xDefNum = SvxNumOptionsTabPageHelper::GetNumberingProvider();
    if(!xDefNum.is())
        return;

    Sequence<Reference<XIndexAccess> > aOutlineAccess;
    const Locale& rLocale = Application::GetSettings().GetLanguageTag().getLocale();
    try
    {
        aOutlineAccess = xDefNum->getDefaultOutlineNumberings( rLocale );

        for(sal_Int32 nItem = 0;
            nItem < aOutlineAccess.getLength() && nItem < NUM_VALUSET_COUNT;
            nItem++ )
        {
            SvxNumSettingsArr_Impl& rItemArr = aNumSettingsArrays[ nItem ];

            const Reference<XIndexAccess>& xLevel = aOutlineAccess[nItem];
            for(sal_Int32 nLevel = 0; nLevel < SVX_MAX_NUM; nLevel++)
            {
                // use the last locale-defined level for all remaining levels.
                sal_Int32 nLocaleLevel = std::min(nLevel, xLevel->getCount() - 1);
                Sequence<PropertyValue> aLevelProps;
                if (nLocaleLevel >= 0)
                    xLevel->getByIndex(nLocaleLevel) >>= aLevelProps;

                SvxNumSettings_Impl* pNew = lcl_CreateNumSettingsPtr(aLevelProps);
                rItemArr.push_back( std::unique_ptr<SvxNumSettings_Impl>(pNew) );
            }
        }
    }
    catch(const Exception&)
    {
    }
    Reference<XNumberingFormatter> xFormat(xDefNum, UNO_QUERY);
    m_xExamplesVS->SetOutlineNumberingSettings(aOutlineAccess, xFormat, rLocale);
}

SvxNumPickTabPage::~SvxNumPickTabPage()
{
    m_xExamplesVSWin.reset();
    m_xExamplesVS.reset();
}

std::unique_ptr<SfxTabPage> SvxNumPickTabPage::Create(weld::Container* pPage, weld::DialogController* pController,
                                             const SfxItemSet* rAttrSet)
{
    return std::make_unique<SvxNumPickTabPage>(pPage, pController, *rAttrSet);
}

bool  SvxNumPickTabPage::FillItemSet( SfxItemSet* rSet )
{
    if( (bPreset || bModified) && pActNum)
    {
        *pSaveNum = *pActNum;
        rSet->Put(SvxNumBulletItem( *pSaveNum, nNumItemId ));
        rSet->Put(SfxBoolItem(SID_PARAM_NUM_PRESET, bPreset));
    }
    return bModified;
}

void  SvxNumPickTabPage::ActivatePage(const SfxItemSet& rSet)
{
    bPreset = false;
    bool bIsPreset = false;
    const SfxItemSet* pExampleSet = GetDialogExampleSet();
    if(pExampleSet)
    {
        if(const SfxBoolItem* pPresetItem = pExampleSet->GetItemIfSet(SID_PARAM_NUM_PRESET, false))
            bIsPreset = pPresetItem->GetValue();
        if(const SfxUInt16Item* pLevelItem = pExampleSet->GetItemIfSet(SID_PARAM_CUR_NUM_LEVEL, false))
            nActNumLvl = pLevelItem->GetValue();
    }
    if(const SvxNumBulletItem* pBulletItem = rSet.GetItemIfSet(nNumItemId, false))
    {
        pSaveNum.reset( new SvxNumRule(pBulletItem->GetNumRule()) );
    }
    if(pActNum && *pSaveNum != *pActNum)
    {
        *pActNum = *pSaveNum;
        m_xExamplesVS->SetNoSelection();
    }

    if(pActNum && (!lcl_IsNumFmtSet(pActNum.get(), nActNumLvl) || bIsPreset))
    {
        m_xExamplesVS->SelectItem(1);
        NumSelectHdl_Impl(m_xExamplesVS.get());
        bPreset = true;
    }
    bPreset |= bIsPreset;
    bModified = false;
}

DeactivateRC SvxNumPickTabPage::DeactivatePage(SfxItemSet *_pSet)
{
    if(_pSet)
        FillItemSet(_pSet);
    return DeactivateRC::LeavePage;
}

void  SvxNumPickTabPage::Reset( const SfxItemSet* rSet )
{
    // in Draw the item exists as WhichId, in Writer only as SlotId
    const SvxNumBulletItem* pItem = rSet->GetItemIfSet(SID_ATTR_NUMBERING_RULE, false);
    if(!pItem)
    {
        nNumItemId = rSet->GetPool()->GetWhichIDFromSlotID(SID_ATTR_NUMBERING_RULE);
        pItem = rSet->GetItemIfSet(nNumItemId, false);

        if( !pItem )
        {
            pItem = & rSet->Get( nNumItemId );
        }
    }
    pSaveNum.reset( new SvxNumRule(pItem->GetNumRule()) );

    if(!pActNum)
        pActNum.reset( new SvxNumRule(*pSaveNum) );
    else if(*pSaveNum != *pActNum)
        *pActNum = *pSaveNum;

}

// all levels are changed here
IMPL_LINK_NOARG(SvxNumPickTabPage, NumSelectHdl_Impl, ValueSet*, void)
{
    if(!pActNum)
        return;

    bPreset = false;
    bModified = true;

    const FontList*  pList = nullptr;

    SvxNumSettingsArr_Impl& rItemArr = aNumSettingsArrays[m_xExamplesVS->GetSelectedItemId() - 1];

    const vcl::Font& rActBulletFont = lcl_GetDefaultBulletFont();
    SvxNumSettings_Impl* pLevelSettings = nullptr;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(rItemArr.size() > i)
            pLevelSettings = rItemArr[i].get();
        if(!pLevelSettings)
            break;
        SvxNumberFormat aFmt(pActNum->GetLevel(i));
        aFmt.SetNumberingType( pLevelSettings->nNumberType );
        sal_uInt16 nUpperLevelOrChar = static_cast<sal_uInt16>(pLevelSettings->nParentNumbering);
        if(aFmt.GetNumberingType() == SVX_NUM_CHAR_SPECIAL)
        {
            // #i93908# clear suffix for bullet lists
            aFmt.SetListFormat(u""_ustr, u""_ustr, i);
            if( !pLevelSettings->sBulletFont.isEmpty() &&
                pLevelSettings->sBulletFont != rActBulletFont.GetFamilyName())
            {
                //search for the font
                if(!pList)
                {
                    if (SfxObjectShell* pCurDocShell = SfxObjectShell::Current())
                    {
                        const SvxFontListItem* pFontListItem =
                                static_cast<const SvxFontListItem*>( pCurDocShell
                                                    ->GetItem( SID_ATTR_CHAR_FONTLIST ));
                        pList = pFontListItem ? pFontListItem->GetFontList() : nullptr;
                    }
                }
                if(pList && pList->IsAvailable( pLevelSettings->sBulletFont ) )
                {
                    vcl::Font aFont(pList->Get(
                        pLevelSettings->sBulletFont,WEIGHT_NORMAL, ITALIC_NONE));
                    aFmt.SetBulletFont(&aFont);
                }
                else
                {
                    //if it cannot be found then create a new one
                    vcl::Font aCreateFont( pLevelSettings->sBulletFont,
                                            OUString(), Size( 0, 14 ) );
                    aCreateFont.SetCharSet( RTL_TEXTENCODING_DONTKNOW );
                    aCreateFont.SetFamily( FAMILY_DONTKNOW );
                    aCreateFont.SetPitch( PITCH_DONTKNOW );
                    aCreateFont.SetWeight( WEIGHT_DONTKNOW );
                    aCreateFont.SetTransparent( true );
                    aFmt.SetBulletFont( &aCreateFont );
                }
            }
            else
                aFmt.SetBulletFont( &rActBulletFont );

            aFmt.SetBulletChar( !pLevelSettings->sBulletChar.isEmpty()
                                    ? pLevelSettings->sBulletChar.iterateCodePoints(
                                        &o3tl::temporary(sal_Int32(0)))
                                    : 0 );
            aFmt.SetCharFormatName( sBulletCharFormatName );
            aFmt.SetBulletRelSize(45);
        }
        else
        {
            aFmt.SetIncludeUpperLevels(sal::static_int_cast< sal_uInt8 >(0 != nUpperLevelOrChar ? pActNum->GetLevelCount() : 1));
            aFmt.SetCharFormatName(sNumCharFmtName);
            aFmt.SetBulletRelSize(100);

            // Completely ignore the Left/Right value provided by the locale outline definition,
            // because this function doesn't actually modify the indents at all,
            // and right-adjusted numbering definitely needs a different FirstLineIndent.

            // #i93908#
            aFmt.SetListFormat(pLevelSettings->sPrefix, pLevelSettings->sSuffix, i);
        }
        pActNum->SetLevel(i, aFmt);
    }
}

IMPL_LINK_NOARG(SvxNumPickTabPage, DoubleClickHdl_Impl, ValueSet*, void)
{
    NumSelectHdl_Impl(m_xExamplesVS.get());
    weld::Button& rOk = GetDialogController()->GetOKButton();
    rOk.clicked();
}

void SvxNumPickTabPage::PageCreated(const SfxAllItemSet& aSet)
{
    const SfxStringItem* pNumCharFmt = aSet.GetItem<SfxStringItem>(SID_NUM_CHAR_FMT, false);
    const SfxStringItem* pBulletCharFmt = aSet.GetItem<SfxStringItem>(SID_BULLET_CHAR_FMT, false);


    if (pNumCharFmt &&pBulletCharFmt)
        SetCharFormatNames( pNumCharFmt->GetValue(),pBulletCharFmt->GetValue());
}

SvxBitmapPickTabPage::SvxBitmapPickTabPage(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rSet)
    : SfxTabPage(pPage, pController, u"cui/ui/pickgraphicpage.ui"_ustr, u"PickGraphicPage"_ustr, &rSet)
    , nActNumLvl(SAL_MAX_UINT16)
    , nNumItemId(SID_ATTR_NUMBERING_RULE)
    , bModified(false)
    , bPreset(false)
    , m_xErrorText(m_xBuilder->weld_label(u"errorft"_ustr))
    , m_xBtBrowseFile(m_xBuilder->weld_button(u"browseBtn"_ustr))
    , m_xExamplesVS(new SvxBmpNumValueSet(m_xBuilder->weld_scrolled_window(u"valuesetwin"_ustr, true)))
    , m_xExamplesVSWin(new weld::CustomWeld(*m_xBuilder, u"valueset"_ustr, *m_xExamplesVS))
{
    SetExchangeSupport();

    m_xExamplesVS->init();
    m_xExamplesVS->SetSelectHdl(LINK(this, SvxBitmapPickTabPage, NumSelectHdl_Impl));
    m_xExamplesVS->SetDoubleClickHdl(LINK(this, SvxBitmapPickTabPage, DoubleClickHdl_Impl));
    m_xBtBrowseFile->connect_clicked(LINK(this, SvxBitmapPickTabPage, ClickAddBrowseHdl_Impl));

    if(comphelper::LibreOfficeKit::isActive())
        m_xBtBrowseFile->hide();

    eCoreUnit = rSet.GetPool()->GetMetric(rSet.GetPool()->GetWhichIDFromSlotID(SID_ATTR_NUMBERING_RULE));

    // determine graphic name
    GalleryExplorer::FillObjList(GALLERY_THEME_BULLETS, aGrfNames);

    size_t i = 0;
    for (auto & grfName : aGrfNames)
    {
        m_xExamplesVS->InsertItem( i + 1, i);

        INetURLObject aObj(grfName);
        if (aObj.GetProtocol() == INetProtocol::File)
        {
            // tdf#114070 - only show the last name of the filename without its extension
            aObj.removeExtension();
            grfName = aObj.GetLastName(INetURLObject::DecodeMechanism::Unambiguous);
        }

        m_xExamplesVS->SetItemText( i + 1, grfName );
        ++i;
    }

    if(aGrfNames.empty())
    {
        m_xErrorText->show();
    }
    else
    {
        m_xExamplesVS->Show();
        m_xExamplesVS->SetFormat();
        m_xExamplesVS->Invalidate();
    }
}

SvxBitmapPickTabPage::~SvxBitmapPickTabPage()
{
    m_xExamplesVSWin.reset();
    m_xExamplesVS.reset();
}

std::unique_ptr<SfxTabPage> SvxBitmapPickTabPage::Create(weld::Container* pPage, weld::DialogController* pController,
                                                const SfxItemSet* rAttrSet)
{
    return std::make_unique<SvxBitmapPickTabPage>(pPage, pController, *rAttrSet);
}

void  SvxBitmapPickTabPage::ActivatePage(const SfxItemSet& rSet)
{
    bPreset = false;
    bool bIsPreset = false;
    const SfxItemSet* pExampleSet = GetDialogExampleSet();
    if(pExampleSet)
    {
        if(const SfxBoolItem* pPresetItem = pExampleSet->GetItemIfSet(SID_PARAM_NUM_PRESET, false))
            bIsPreset = pPresetItem->GetValue();
        if(const SfxUInt16Item* pLevelItem = pExampleSet->GetItemIfSet(SID_PARAM_CUR_NUM_LEVEL, false))
            nActNumLvl = pLevelItem->GetValue();
    }
    if(const SvxNumBulletItem* pBulletItem = rSet.GetItemIfSet(nNumItemId, false))
    {
        pSaveNum.reset( new SvxNumRule(pBulletItem->GetNumRule()) );
    }
    if(pActNum && *pSaveNum != *pActNum)
    {
        *pActNum = *pSaveNum;
        m_xExamplesVS->SetNoSelection();
    }

    if(!aGrfNames.empty() &&
        (pActNum && (!lcl_IsNumFmtSet(pActNum.get(), nActNumLvl) || bIsPreset)))
    {
        m_xExamplesVS->SelectItem(1);
        NumSelectHdl_Impl(m_xExamplesVS.get());
        bPreset = true;
    }
    bPreset |= bIsPreset;
    bModified = false;
}

DeactivateRC SvxBitmapPickTabPage::DeactivatePage(SfxItemSet *_pSet)
{
    if(_pSet)
        FillItemSet(_pSet);
    return DeactivateRC::LeavePage;
}

bool  SvxBitmapPickTabPage::FillItemSet( SfxItemSet* rSet )
{
    if ( aGrfNames.empty() )
    {
        return false;
    }
    if( (bPreset || bModified) && pActNum)
    {
        *pSaveNum = *pActNum;
        rSet->Put(SvxNumBulletItem( *pSaveNum, nNumItemId ) );
        rSet->Put(SfxBoolItem(SID_PARAM_NUM_PRESET, bPreset));
    }

    return bModified;
}

void  SvxBitmapPickTabPage::Reset( const SfxItemSet* rSet )
{
    // in Draw the item exists as WhichId, in Writer only as SlotId
    const SvxNumBulletItem* pItem = rSet->GetItemIfSet(SID_ATTR_NUMBERING_RULE, false);
    if(!pItem)
    {
        nNumItemId = rSet->GetPool()->GetWhichIDFromSlotID(SID_ATTR_NUMBERING_RULE);
        pItem = rSet->GetItemIfSet(nNumItemId, false);

        if( !pItem )
        {
            pItem = & rSet->Get( nNumItemId );
        }

    }
    DBG_ASSERT(pItem, "no item found!");
    pSaveNum.reset( new SvxNumRule(pItem->GetNumRule()) );

    if(!pActNum)
        pActNum.reset( new SvxNumRule(*pSaveNum) );
    else if(*pSaveNum != *pActNum)
        *pActNum = *pSaveNum;
}

IMPL_LINK_NOARG(SvxBitmapPickTabPage, NumSelectHdl_Impl, ValueSet*, void)
{
    if(!pActNum)
        return;

    bPreset = false;
    bModified = true;
    sal_uInt16 nIdx = m_xExamplesVS->GetSelectedItemId() - 1;

    sal_uInt16 nMask = 1;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aFmt(pActNum->GetLevel(i));
            aFmt.SetNumberingType(SVX_NUM_BITMAP);
            aFmt.SetListFormat(u""_ustr, u""_ustr, i);
            aFmt.SetCharFormatName( u""_ustr );

            Graphic aGraphic;
            if(GalleryExplorer::GetGraphicObj( GALLERY_THEME_BULLETS, nIdx, &aGraphic))
            {
                Size aSize = SvxNumberFormat::GetGraphicSizeMM100(&aGraphic);
                sal_Int16 eOrient = text::VertOrientation::LINE_CENTER;
                aSize = OutputDevice::LogicToLogic(aSize, MapMode(MapUnit::Map100thMM), MapMode(eCoreUnit));
                SvxBrushItem aBrush(aGraphic, GPOS_AREA, SID_ATTR_BRUSH );
                aFmt.SetGraphicBrush( &aBrush, &aSize, &eOrient );
            }
            else if(aGrfNames.size() > nIdx)
                aFmt.SetGraphic( aGrfNames[nIdx] );
            pActNum->SetLevel(i, aFmt);
        }
        nMask <<= 1;
    }
}

IMPL_LINK_NOARG(SvxBitmapPickTabPage, DoubleClickHdl_Impl, ValueSet*, void)
{
    NumSelectHdl_Impl(m_xExamplesVS.get());
    weld::Button& rOk = GetDialogController()->GetOKButton();
    rOk.clicked();
}

IMPL_LINK_NOARG(SvxBitmapPickTabPage, ClickAddBrowseHdl_Impl, weld::Button&, void)
{
    sfx2::FileDialogHelper aFileDialog(0, FileDialogFlags::NONE, GetFrameWeld());
    aFileDialog.SetContext(sfx2::FileDialogHelper::BulletsAddImage);
    aFileDialog.SetTitle(CuiResId(RID_CUISTR_ADD_IMAGE));
    if ( aFileDialog.Execute() != ERRCODE_NONE )
        return;

    OUString aPath = SvtPathOptions().GetGalleryPath();
    std::u16string_view aPathToken = o3tl::getToken(aPath, 1 , SEARCHPATH_DELIMITER );

    OUString aUserImageURL = aFileDialog.GetPath();

    OUString aFileName;
    const sal_Int32 nPos {aUserImageURL.lastIndexOf(SEARCHFILENAME_DELIMITER)+1};
    if (nPos<=0)
        aFileName = aUserImageURL;
    else if (nPos<aUserImageURL.getLength())
        aFileName = aUserImageURL.copy(nPos);

    OUString aUserGalleryURL = OUString::Concat(aPathToken) + "/" + aFileName;
    INetURLObject aURL( aUserImageURL );
    DBG_ASSERT( aURL.GetProtocol() != INetProtocol::NotValid, "invalid URL" );

    GraphicDescriptor aDescriptor(aURL);
    if (!aDescriptor.Detect())
        return;

    uno::Reference< lang::XMultiServiceFactory > xFactory = ::comphelper::getProcessServiceFactory();
    uno::Reference<ucb::XSimpleFileAccess3> xSimpleFileAccess(
                 ucb::SimpleFileAccess::create( ::comphelper::getComponentContext(xFactory) ) );
    if ( !xSimpleFileAccess->exists( aUserImageURL ))
        return;

    xSimpleFileAccess->copy( aUserImageURL, aUserGalleryURL );
    INetURLObject gURL( aUserGalleryURL );
    std::unique_ptr<SvStream> pIn(::utl::UcbStreamHelper::CreateStream(
                  gURL.GetMainURL( INetURLObject::DecodeMechanism::NONE ), StreamMode::READ ));
    if ( !pIn )
        return;

    Graphic aGraphic;
    GraphicConverter::Import( *pIn, aGraphic );

    BitmapEx aBitmap = aGraphic.GetBitmapEx();
    tools::Long nPixelX = aBitmap.GetSizePixel().Width();
    tools::Long nPixelY = aBitmap.GetSizePixel().Height();
    double ratio = nPixelY/static_cast<double>(nPixelX);
    if(nPixelX > 30)
    {
        nPixelX = 30;
        nPixelY = static_cast<tools::Long>(nPixelX*ratio);
    }
    if(nPixelY > 30)
    {
        nPixelY = 30;
        nPixelX = static_cast<tools::Long>(nPixelY/ratio);
    }

    aBitmap.Scale( Size( nPixelX, nPixelY ), BmpScaleFlag::Fast );
    Graphic aScaledGraphic( aBitmap );
    GraphicFilter& rFilter = GraphicFilter::GetGraphicFilter();

    Sequence< PropertyValue > aFilterData{
        comphelper::makePropertyValue(u"Compression"_ustr, sal_Int32(-1)),
        comphelper::makePropertyValue(u"Quality"_ustr, sal_Int32(1))
    };

    sal_uInt16 nFilterFormat = rFilter.GetExportFormatNumberForShortName( gURL.GetFileExtension() );
    rFilter.ExportGraphic( aScaledGraphic, gURL , nFilterFormat, &aFilterData );
    GalleryExplorer::InsertURL( GALLERY_THEME_BULLETS, aUserGalleryURL );

    aGrfNames.push_back(aUserGalleryURL);
    size_t i = 0;
    for (auto & grfName : aGrfNames)
    {
        m_xExamplesVS->InsertItem( i + 1, i);
        INetURLObject aObj(grfName);
        if (aObj.GetProtocol() == INetProtocol::File)
        {
            // tdf#114070 - only show the last name of the filename without its extension
            aObj.removeExtension();
            grfName = aObj.GetLastName(INetURLObject::DecodeMechanism::Unambiguous);
        }
        m_xExamplesVS->SetItemText( i + 1, grfName );
        ++i;
    }

    if(aGrfNames.empty())
    {
        m_xErrorText->show();
    }
    else
    {
        m_xExamplesVS->Show();
        m_xExamplesVS->SetFormat();
    }
}

// tabpage numbering options
SvxNumOptionsTabPage::SvxNumOptionsTabPage(weld::Container* pPage, weld::DialogController* pController,
                               const SfxItemSet& rSet)
    : SfxTabPage(pPage, pController, u"cui/ui/numberingoptionspage.ui"_ustr, u"NumberingOptionsPage"_ustr, &rSet)
    , aInvalidateTimer("cui SvxNumOptionsTabPage aInvalidateTimer")
    , m_pLevelHdlEvent(nullptr)
    , bLastWidthModified(false)
    , bModified(false)
    , bPreset(false)
    , bAutomaticCharStyles(true)
    , bHTMLMode(false)
    , nBullet(NumberType::NONE)
    , nActNumLvl(1)
    , nNumItemId(SID_ATTR_NUMBERING_RULE)
    , m_aRatioTop(ConnectorType::Top)
    , m_aRatioBottom(ConnectorType::Bottom)
    , m_xGrid(m_xBuilder->weld_widget(u"grid2"_ustr))
    , m_xLevelLB(m_xBuilder->weld_tree_view(u"levellb"_ustr))
    , m_xFmtLB(m_xBuilder->weld_combo_box(u"numfmtlb"_ustr))
    , m_xSeparatorFT(m_xBuilder->weld_label(u"separator"_ustr))
    , m_xPrefixFT(m_xBuilder->weld_label(u"prefixft"_ustr))
    , m_xPrefixED(m_xBuilder->weld_entry(u"prefix"_ustr))
    , m_xSuffixFT(m_xBuilder->weld_label(u"suffixft"_ustr))
    , m_xSuffixED(m_xBuilder->weld_entry(u"suffix"_ustr))
    , m_xCharFmtFT(m_xBuilder->weld_label(u"charstyleft"_ustr))
    , m_xCharFmtLB(m_xBuilder->weld_combo_box(u"charstyle"_ustr))
    , m_xBulColorFT(m_xBuilder->weld_label(u"colorft"_ustr))
    , m_xBulColLB(new ColorListBox(m_xBuilder->weld_menu_button(u"color"_ustr),
                [this]{ return GetDialogController()->getDialog(); }))
    , m_xBulRelSizeFT(m_xBuilder->weld_label(u"relsizeft"_ustr))
    , m_xBulRelSizeMF(m_xBuilder->weld_metric_spin_button(u"relsize"_ustr, FieldUnit::PERCENT))
    , m_xAllLevelFT(m_xBuilder->weld_label(u"sublevelsft"_ustr))
    , m_xAllLevelNF(m_xBuilder->weld_spin_button(u"sublevels"_ustr))
    , m_xIsLegalCB(m_xBuilder->weld_check_button(u"islegal"_ustr))
    , m_xStartFT(m_xBuilder->weld_label(u"startatft"_ustr))
    , m_xStartED(m_xBuilder->weld_spin_button(u"startat"_ustr))
    , m_xBulletFT(m_xBuilder->weld_label(u"bulletft"_ustr))
    , m_xBulletPB(m_xBuilder->weld_button(u"bullet"_ustr))
    , m_xBitmapFT(m_xBuilder->weld_label(u"bitmapft"_ustr))
    , m_xBitmapMB(m_xBuilder->weld_menu_button(u"bitmap"_ustr))
    , m_xWidthFT(m_xBuilder->weld_label(u"widthft"_ustr))
    , m_xWidthMF(m_xBuilder->weld_metric_spin_button(u"widthmf"_ustr, FieldUnit::CM))
    , m_xHeightFT(m_xBuilder->weld_label(u"heightft"_ustr))
    , m_xHeightMF(m_xBuilder->weld_metric_spin_button(u"heightmf"_ustr, FieldUnit::CM))
    , m_xRatioCB(m_xBuilder->weld_check_button(u"keepratio"_ustr))
    , m_xCbxScaleImg(m_xBuilder->weld_image(u"imRatio"_ustr))
    , m_xImgRatioTop(new weld::CustomWeld(*m_xBuilder, u"daRatioTop"_ustr, m_aRatioTop))
    , m_xImgRatioBottom(new weld::CustomWeld(*m_xBuilder, u"daRatioBottom"_ustr, m_aRatioBottom))
    , m_xOrientFT(m_xBuilder->weld_label(u"orientft"_ustr))
    , m_xOrientLB(m_xBuilder->weld_combo_box(u"orientlb"_ustr))
    , m_xAllLevelsFrame(m_xBuilder->weld_widget(u"levelsframe"_ustr))
    , m_xSameLevelCB(m_xBuilder->weld_check_button(u"allsame"_ustr))
    , m_xPreviewWIN(new weld::CustomWeld(*m_xBuilder, u"preview"_ustr, m_aPreviewWIN))
{
    m_xBulColLB->SetSlotId(SID_ATTR_CHAR_COLOR);
    m_xBulRelSizeMF->set_min(SVX_NUM_REL_SIZE_MIN, FieldUnit::PERCENT);
    m_xBulRelSizeMF->set_increments(5, 50, FieldUnit::PERCENT);
    SetExchangeSupport();
    aActBulletFont = lcl_GetDefaultBulletFont();
    // vertical alignment = fill makes the drawingarea expand the associated spinedits so we have to size it here
    const sal_Int16 aHeight
        = static_cast<sal_Int16>(std::max(int(m_xRatioCB->get_preferred_size().getHeight() / 2
                                              - m_xWidthMF->get_preferred_size().getHeight() / 2),
                                          12));
    const sal_Int16 aWidth
        = static_cast<sal_Int16>(m_xRatioCB->get_preferred_size().getWidth() / 2);
    m_xImgRatioTop->set_size_request(aWidth, aHeight);
    m_xImgRatioBottom->set_size_request(aWidth, aHeight);
    //init needed for gtk3
    m_xCbxScaleImg->set_from_icon_name(m_xRatioCB->get_active() ? RID_SVXBMP_LOCKED
                                                                : RID_SVXBMP_UNLOCKED);

    m_xBulletPB->connect_clicked(LINK(this, SvxNumOptionsTabPage, BulletHdl_Impl));
    m_xFmtLB->connect_changed(LINK(this, SvxNumOptionsTabPage, NumberTypeSelectHdl_Impl));
    m_xBitmapMB->connect_selected(LINK(this, SvxNumOptionsTabPage, GraphicHdl_Impl));
    m_xBitmapMB->connect_toggled(LINK(this, SvxNumOptionsTabPage, PopupActivateHdl_Impl));
    m_xLevelLB->set_selection_mode(SelectionMode::Multiple);
    m_xLevelLB->connect_selection_changed(LINK(this, SvxNumOptionsTabPage, LevelHdl_Impl));
    m_xCharFmtLB->connect_changed(LINK(this, SvxNumOptionsTabPage, CharFmtHdl_Impl));
    m_xWidthMF->connect_value_changed(LINK(this, SvxNumOptionsTabPage, SizeHdl_Impl));
    m_xHeightMF->connect_value_changed(LINK(this, SvxNumOptionsTabPage, SizeHdl_Impl));
    m_xRatioCB->connect_toggled(LINK(this, SvxNumOptionsTabPage, RatioHdl_Impl));
    m_xStartED->connect_value_changed(LINK(this, SvxNumOptionsTabPage, SpinModifyHdl_Impl));
    m_xPrefixED->connect_changed(LINK(this, SvxNumOptionsTabPage, EditModifyHdl_Impl));
    m_xSuffixED->connect_changed(LINK(this, SvxNumOptionsTabPage, EditModifyHdl_Impl));
    m_xAllLevelNF->connect_value_changed(LINK(this,SvxNumOptionsTabPage, AllLevelHdl_Impl));
    m_xIsLegalCB->connect_toggled(LINK(this, SvxNumOptionsTabPage, IsLegalHdl_Impl));
    m_xOrientLB->connect_changed(LINK(this, SvxNumOptionsTabPage, OrientHdl_Impl));
    m_xSameLevelCB->connect_toggled(LINK(this, SvxNumOptionsTabPage, SameLevelHdl_Impl));
    m_xBulRelSizeMF->connect_value_changed(LINK(this,SvxNumOptionsTabPage, BulRelSizeHdl_Impl));
    m_xBulColLB->SetSelectHdl(LINK(this, SvxNumOptionsTabPage, BulColorHdl_Impl));
    aInvalidateTimer.SetInvokeHandler(LINK(this, SvxNumOptionsTabPage, PreviewInvalidateHdl_Impl));
    aInvalidateTimer.SetTimeout(50);

    eCoreUnit = rSet.GetPool()->GetMetric(rSet.GetPool()->GetWhichIDFromSlotID(SID_ATTR_NUMBERING_RULE));

    // Fill ListBox with predefined / translated numbering types.
    sal_uInt32 nCount = SvxNumberingTypeTable::Count();
    for (sal_uInt32 i = 0; i < nCount; ++i)
    {
        int nValue = SvxNumberingTypeTable::GetValue(i);
        if (comphelper::LibreOfficeKit::isActive() && (nValue & SVX_NUM_BITMAP)) continue;
        m_xFmtLB->append(OUString::number(nValue), SvxNumberingTypeTable::GetString(i));
    }

    // Get advanced numbering types from the component.
    // Watch out for the ugly
    // 136 == 0x88 == SVX_NUM_BITMAP|0x80 == SVX_NUM_BITMAP|LINK_TOKEN
    // to not remove that.
    SvxNumOptionsTabPageHelper::GetI18nNumbering( *m_xFmtLB, (SVX_NUM_BITMAP | LINK_TOKEN));

    m_xFmtLB->set_active(0);

    m_xCharFmtLB->set_size_request(m_xCharFmtLB->get_approximate_digit_width() * 10, -1);
    Size aSize(m_xGrid->get_preferred_size());
    m_xGrid->set_size_request(aSize.Width(), -1);
}

SvxNumOptionsTabPage::~SvxNumOptionsTabPage()
{
    m_xPreviewWIN.reset();
    m_xBulColLB.reset();
    pActNum.reset();
    pSaveNum.reset();
    if (m_pLevelHdlEvent)
    {
        Application::RemoveUserEvent(m_pLevelHdlEvent);
        m_pLevelHdlEvent = nullptr;
    }
}

void SvxNumOptionsTabPage::SetMetric(FieldUnit eMetric)
{
    if(eMetric == FieldUnit::MM)
    {
        m_xWidthMF->set_digits(1);
        m_xHeightMF->set_digits(1);
    }
    m_xWidthMF->set_unit(eMetric);
    m_xHeightMF->set_unit(eMetric);
}

std::unique_ptr<SfxTabPage> SvxNumOptionsTabPage::Create(weld::Container* pPage, weld::DialogController* pController,
                                                const SfxItemSet* rAttrSet)
{
    return std::make_unique<SvxNumOptionsTabPage>(pPage, pController, *rAttrSet);
};

void    SvxNumOptionsTabPage::ActivatePage(const SfxItemSet& rSet)
{
    const SfxItemSet* pExampleSet = GetDialogExampleSet();
    sal_uInt16 nTmpNumLvl = 1;
    if(pExampleSet)
    {
        if(const SfxBoolItem* pPresetItem = pExampleSet->GetItemIfSet(SID_PARAM_NUM_PRESET, false))
            bPreset = pPresetItem->GetValue();
        if(const SfxUInt16Item* pLevelItem = pExampleSet->GetItemIfSet(SID_PARAM_CUR_NUM_LEVEL, false))
            nTmpNumLvl = pLevelItem->GetValue();
    }
    if(const SvxNumBulletItem* pBulletItem = rSet.GetItemIfSet(nNumItemId, false))
    {
        pSaveNum.reset( new SvxNumRule(pBulletItem->GetNumRule()) );
    }

    bModified = (!pActNum->Get( 0 ) || bPreset);
    if(*pActNum == *pSaveNum && nActNumLvl == nTmpNumLvl)
        return;

    nActNumLvl = nTmpNumLvl;
    sal_uInt16 nMask = 1;
    m_xLevelLB->unselect_all();
    if (nActNumLvl == SAL_MAX_UINT16)
        m_xLevelLB->select(pActNum->GetLevelCount());
    if(nActNumLvl != SAL_MAX_UINT16)
    {
        for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
        {
            if(nActNumLvl & nMask)
                m_xLevelLB->select(i);
            nMask <<= 1 ;
        }
    }
    *pActNum = *pSaveNum;

    InitControls();
}

DeactivateRC SvxNumOptionsTabPage::DeactivatePage(SfxItemSet * _pSet)
{
    if(_pSet)
        FillItemSet(_pSet);
    return DeactivateRC::LeavePage;
}

bool    SvxNumOptionsTabPage::FillItemSet( SfxItemSet* rSet )
{
    rSet->Put(SfxUInt16Item(SID_PARAM_CUR_NUM_LEVEL, nActNumLvl));
    if(bModified && pActNum)
    {
        *pSaveNum = *pActNum;
        rSet->Put(SvxNumBulletItem( *pSaveNum, nNumItemId ));
        rSet->Put(SfxBoolItem(SID_PARAM_NUM_PRESET, false));
    }
    return bModified;
};

void    SvxNumOptionsTabPage::Reset( const SfxItemSet* rSet )
{
    // in Draw the item exists as WhichId, in Writer only as SlotId
    const SvxNumBulletItem* pBulletItem =
        rSet->GetItemIfSet(SID_ATTR_NUMBERING_RULE, false);
    if(!pBulletItem)
    {
        nNumItemId = rSet->GetPool()->GetWhichIDFromSlotID(SID_ATTR_NUMBERING_RULE);
        pBulletItem = rSet->GetItemIfSet(nNumItemId, false);

        if( !pBulletItem )
        {
            pBulletItem = & rSet->Get( nNumItemId );
        }
    }
    DBG_ASSERT(pBulletItem, "no item found!");
    pSaveNum.reset( new SvxNumRule(pBulletItem->GetNumRule()) );

    // insert levels
    if (!m_xLevelLB->n_children())
    {
        for(sal_uInt16 i = 1; i <= pSaveNum->GetLevelCount(); i++)
            m_xLevelLB->append_text(OUString::number(i));
        if(pSaveNum->GetLevelCount() > 1)
        {
            OUString sEntry = "1 - " + OUString::number( pSaveNum->GetLevelCount() );
            m_xLevelLB->append_text(sEntry);
            m_xLevelLB->select_text(sEntry);
        }
        else
            m_xLevelLB->select(0);
    }
    else
        m_xLevelLB->select(m_xLevelLB->n_children() - 1);

    sal_uInt16 nMask = 1;
    m_xLevelLB->unselect_all();
    if (nActNumLvl == SAL_MAX_UINT16)
    {
        m_xLevelLB->select( pSaveNum->GetLevelCount() );
    }
    else
    {
        for(sal_uInt16 i = 0; i < pSaveNum->GetLevelCount(); i++)
        {
            if(nActNumLvl & nMask)
                m_xLevelLB->select( i );
            nMask <<= 1 ;
        }
    }

    if(!pActNum)
        pActNum.reset( new SvxNumRule(*pSaveNum) );
    else if(*pSaveNum != *pActNum)
        *pActNum = *pSaveNum;
    m_aPreviewWIN.SetNumRule(pActNum.get());
    m_xSameLevelCB->set_active(pActNum->IsContinuousNumbering());

    const SfxUInt16Item* pHtmlModeItem =
        rSet->GetItemIfSet( SID_HTML_MODE, false );
    if (!pHtmlModeItem)
    {
        if (SfxObjectShell* pShell = SfxObjectShell::Current())
            pHtmlModeItem = pShell->GetItem( SID_HTML_MODE );
    }
    if ( pHtmlModeItem )
    {
        sal_uInt16 nHtmlMode = pHtmlModeItem->GetValue();
        bHTMLMode = 0 != (nHtmlMode&HTMLMODE_ON);
    }

    bool bCharFmt = pActNum->IsFeatureSupported(SvxNumRuleFlags::CHAR_STYLE);
    m_xCharFmtFT->set_visible(bCharFmt);
    m_xCharFmtLB->set_visible(bCharFmt);

    bool bContinuous = pActNum->IsFeatureSupported(SvxNumRuleFlags::CONTINUOUS);

    bool bAllLevel = bContinuous && !bHTMLMode;
    m_xAllLevelFT->set_visible(bAllLevel);
    m_xAllLevelNF->set_visible(bAllLevel);
    m_xIsLegalCB->set_visible(bAllLevel);

    m_xAllLevelsFrame->set_visible(bContinuous);

    // again misusage: in Draw there is numeration only until the bitmap
    // without SVX_NUM_NUMBER_NONE
    //remove types that are unsupported by Draw/Impress
    if(!bContinuous)
    {
        sal_Int32 nFmtCount = m_xFmtLB->get_count();
        for(sal_Int32 i = nFmtCount; i; i--)
        {
            sal_uInt16 nEntryData = m_xFmtLB->get_id(i - 1).toUInt32();
            if(/*SVX_NUM_NUMBER_NONE == nEntryData ||*/
                (SVX_NUM_BITMAP|LINK_TOKEN) ==  nEntryData)
                m_xFmtLB->remove(i - 1);
        }
    }
    //one must be enabled
    if(!pActNum->IsFeatureSupported(SvxNumRuleFlags::ENABLE_LINKED_BMP))
    {
        auto nPos = m_xFmtLB->find_id(OUString::number(SVX_NUM_BITMAP|LINK_TOKEN));
        if (nPos != -1)
            m_xFmtLB->remove(nPos);
    }
    else if(!pActNum->IsFeatureSupported(SvxNumRuleFlags::ENABLE_EMBEDDED_BMP))
    {
        auto nPos = m_xFmtLB->find_id(OUString::number(SVX_NUM_BITMAP));
        if (nPos != -1)
            m_xFmtLB->remove(nPos);
    }

    // MegaHack: because of a not-fixable 'design mistake/error' in Impress
    // delete all kinds of numeric enumerations
    if(pActNum->IsFeatureSupported(SvxNumRuleFlags::NO_NUMBERS))
    {
        sal_Int32 nFmtCount = m_xFmtLB->get_count();
        for(sal_Int32 i = nFmtCount; i; i--)
        {
            sal_uInt16 nEntryData = m_xFmtLB->get_id(i - 1).toUInt32();
            if( /*nEntryData >= SVX_NUM_CHARS_UPPER_LETTER &&*/  nEntryData <= SVX_NUM_NUMBER_NONE)
                m_xFmtLB->remove(i - 1);
        }
    }

    InitControls();
    bModified = false;
}

void SvxNumOptionsTabPage::InitControls()
{
    bool bShowBullet    = true;
    bool bShowBitmap    = true;
    bool bSameType      = true;
    bool bSameStart     = true;
    bool bSamePrefix    = true;
    bool bSameSuffix    = true;
    bool bAllLevel      = true;
    bool bSameCharFmt   = true;
    bool bSameVOrient   = true;
    bool bSameSize      = true;
    bool bSameBulColor  = true;
    bool bSameBulRelSize= true;

    TriState isLegal = TRISTATE_INDET;

    const SvxNumberFormat* aNumFmtArr[SVX_MAX_NUM];
    OUString sFirstCharFmt;
    sal_Int16 eFirstOrient = text::VertOrientation::NONE;
    Size aFirstSize(0,0);
    sal_uInt16 nMask = 1;
    sal_uInt16 nLvl = SAL_MAX_UINT16;
    sal_uInt16 nHighestLevel = 0;

    bool bBullColor = pActNum->IsFeatureSupported(SvxNumRuleFlags::BULLET_COLOR);
    bool bBullRelSize = pActNum->IsFeatureSupported(SvxNumRuleFlags::BULLET_REL_SIZE);
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & nMask)
        {
            aNumFmtArr[i] = &pActNum->GetLevel(i);
            bShowBullet &= aNumFmtArr[i]->GetNumberingType() == SVX_NUM_CHAR_SPECIAL;
            bShowBitmap &= (aNumFmtArr[i]->GetNumberingType()&(~LINK_TOKEN)) == SVX_NUM_BITMAP;
            if(SAL_MAX_UINT16 == nLvl)
            {
                nLvl = i;
                sFirstCharFmt = aNumFmtArr[i]->GetCharFormatName();
                eFirstOrient = aNumFmtArr[i]->GetVertOrient();
                if(bShowBitmap)
                    aFirstSize = aNumFmtArr[i]->GetGraphicSize();
                isLegal = aNumFmtArr[i]->GetIsLegal() ? TRISTATE_TRUE : TRISTATE_FALSE;
            }
            if( i > nLvl)
            {
                bSameType &=   aNumFmtArr[i]->GetNumberingType() == aNumFmtArr[nLvl]->GetNumberingType();
                bSameStart = aNumFmtArr[i]->GetStart() == aNumFmtArr[nLvl]->GetStart();

                bSamePrefix = aNumFmtArr[i]->GetPrefix() == aNumFmtArr[nLvl]->GetPrefix();
                bSameSuffix = aNumFmtArr[i]->GetSuffix() == aNumFmtArr[nLvl]->GetSuffix();
                bAllLevel &= aNumFmtArr[i]->GetIncludeUpperLevels() == aNumFmtArr[nLvl]->GetIncludeUpperLevels();
                if (aNumFmtArr[i]->GetIsLegal() != aNumFmtArr[nLvl]->GetIsLegal())
                    isLegal = TRISTATE_INDET;
                bSameCharFmt    &= sFirstCharFmt == aNumFmtArr[i]->GetCharFormatName();
                bSameVOrient    &= eFirstOrient == aNumFmtArr[i]->GetVertOrient();
                if(bShowBitmap && bSameSize)
                    bSameSize &= aNumFmtArr[i]->GetGraphicSize() == aFirstSize;
                bSameBulColor &= aNumFmtArr[i]->GetBulletColor() == aNumFmtArr[nLvl]->GetBulletColor();
                bSameBulRelSize &= aNumFmtArr[i]->GetBulletRelSize() == aNumFmtArr[nLvl]->GetBulletRelSize();
            }
            nHighestLevel = i;
        }
        else
            aNumFmtArr[i] = nullptr;

        nMask <<= 1 ;
    }
    SwitchNumberType(bShowBullet ? NumberType::SHOW_BULLET : bShowBitmap ? NumberType::SHOW_BITMAP : NumberType::SHOW_NUMBERING);

    sal_uInt16 nNumberingType;
    if (nLvl != SAL_MAX_UINT16)
        nNumberingType = aNumFmtArr[nLvl]->GetNumberingType();
    else
    {
        nNumberingType = SVX_NUM_NUMBER_NONE;
        bAllLevel = false;
        bSameBulRelSize = false;
        bSameBulColor = false;
        bSameStart = false;
        bSamePrefix = false;
        bSameSuffix = false;
    }

    CheckForStartValue_Impl(nNumberingType);

    if(bShowBitmap)
    {
        if(!bSameVOrient || eFirstOrient == text::VertOrientation::NONE)
            m_xOrientLB->set_active(-1);
        else
            m_xOrientLB->set_active(
                sal::static_int_cast< sal_Int32 >(eFirstOrient - 1));
                // no text::VertOrientation::NONE

        if(bSameSize)
        {
            SetMetricValue(*m_xHeightMF, aFirstSize.Height(), eCoreUnit);
            SetMetricValue(*m_xWidthMF, aFirstSize.Width(), eCoreUnit);
        }
        else
        {
            m_xHeightMF->set_text(u""_ustr);
            m_xWidthMF->set_text(u""_ustr);
        }
    }

    if(bSameType)
    {
        sal_uInt16 nLBData = nNumberingType;
        m_xFmtLB->set_active_id(OUString::number(nLBData));
    }
    else
        m_xFmtLB->set_active(-1);

    m_xAllLevelNF->set_sensitive(nHighestLevel > 0 && !m_xSameLevelCB->get_active());
    m_xAllLevelNF->set_max(nHighestLevel + 1);
    if(bAllLevel)
    {
        m_xAllLevelNF->set_value(aNumFmtArr[nLvl]->GetIncludeUpperLevels());
    }
    else
    {
        m_xAllLevelNF->set_text(u""_ustr);
    }

    m_xIsLegalCB->set_state(isLegal);
    m_xIsLegalCB->set_sensitive(!m_xSameLevelCB->get_active());

    if(bBullRelSize)
    {
        if(bSameBulRelSize)
            m_xBulRelSizeMF->set_value(aNumFmtArr[nLvl]->GetBulletRelSize(), FieldUnit::PERCENT);
        else
            m_xBulRelSizeMF->set_text(u""_ustr);
    }
    if(bBullColor)
    {
        if(bSameBulColor)
            m_xBulColLB->SelectEntry(aNumFmtArr[nLvl]->GetBulletColor());
        else
            m_xBulColLB->SetNoSelection();
    }
    m_xStartED->set_value(1); // If this isn't set then changing the bullet type to a numbered type doesn't reset the start level
    switch(nBullet)
    {
        case NumberType::SHOW_NUMBERING:
            if(bSameStart)
            {
                m_xStartED->set_value(aNumFmtArr[nLvl]->GetStart());
            }
            else
                m_xStartED->set_text(u""_ustr);
        break;
        case NumberType::SHOW_BULLET:
        break;
        case NumberType::SHOW_BITMAP:
        break;
        case NumberType::NONE:
        break;
    }

    if(bSamePrefix)
        m_xPrefixED->set_text(aNumFmtArr[nLvl]->GetPrefix());
    else
        m_xPrefixED->set_text(u""_ustr);
    if(bSameSuffix)
        m_xSuffixED->set_text(aNumFmtArr[nLvl]->GetSuffix());
    else
        m_xSuffixED->set_text(u""_ustr);

    if(bSameCharFmt)
    {
        if (!sFirstCharFmt.isEmpty())
            m_xCharFmtLB->set_active_text(sFirstCharFmt);
        else if (m_xCharFmtLB->get_count())
            m_xCharFmtLB->set_active(0);
    }
    else
        m_xCharFmtLB->set_active(-1);

    m_aPreviewWIN.SetLevel(nActNumLvl);
    m_aPreviewWIN.Invalidate();
}

// 0 - Number; 1 - Bullet; 2 - Bitmap
void SvxNumOptionsTabPage::SwitchNumberType( NumberType nType )
{
    if(nBullet == nType)
        return;
    nBullet = nType;
    bool bBullet = (nType == NumberType::SHOW_BULLET);
    bool bBitmap = (nType == NumberType::SHOW_BITMAP);
    bool bEnableBitmap = (nType == NumberType::SHOW_BITMAP);
    bool bNumeric = !(bBitmap||bBullet);
    m_xSeparatorFT->set_visible(bNumeric);
    m_xPrefixFT->set_visible(bNumeric);
    m_xPrefixED->set_visible(bNumeric);
    m_xSuffixFT->set_visible(bNumeric);
    m_xSuffixED->set_visible(bNumeric);

    bool bCharFmt = pActNum->IsFeatureSupported(SvxNumRuleFlags::CHAR_STYLE);
    m_xCharFmtFT->set_visible(!bBitmap && bCharFmt);
    m_xCharFmtLB->set_visible(!bBitmap && bCharFmt);

    // this is rather misusage, as there is no own flag
    // for complete numeration
    bool bAllLevelFeature = pActNum->IsFeatureSupported(SvxNumRuleFlags::CONTINUOUS);
    bool bAllLevel = bNumeric && bAllLevelFeature && !bHTMLMode;
    m_xAllLevelFT->set_visible(bAllLevel);
    m_xAllLevelNF->set_visible(bAllLevel);
    m_xIsLegalCB->set_visible(bAllLevel);

    m_xStartFT->set_visible(!(bBullet||bBitmap));
    m_xStartED->set_visible(!(bBullet||bBitmap));

    m_xBulletFT->set_visible(bBullet);
    m_xBulletPB->set_visible(bBullet);
    bool bBullColor = pActNum->IsFeatureSupported(SvxNumRuleFlags::BULLET_COLOR);
    m_xBulColorFT->set_visible(!bBitmap && bBullColor);
    m_xBulColLB->set_visible(!bBitmap && bBullColor);
    bool bBullResSize = pActNum->IsFeatureSupported(SvxNumRuleFlags::BULLET_REL_SIZE);
    m_xBulRelSizeFT->set_visible(!bBitmap && bBullResSize);
    m_xBulRelSizeMF->set_visible(!bBitmap && bBullResSize);

    m_xBitmapFT->set_visible(bBitmap);
    m_xBitmapMB->set_visible(bBitmap);

    m_xWidthFT->set_visible(bBitmap);
    m_xWidthMF->set_visible(bBitmap);
    m_xHeightFT->set_visible(bBitmap);
    m_xHeightMF->set_visible(bBitmap);
    m_xRatioCB->set_visible(bBitmap);
    m_xCbxScaleImg->set_visible(bBitmap);
    m_xImgRatioTop->set_visible(bBitmap);
    m_xImgRatioBottom->set_visible(bBitmap);

    m_xOrientFT->set_visible(bBitmap && bAllLevelFeature);
    m_xOrientLB->set_visible(bBitmap && bAllLevelFeature);

    m_xWidthFT->set_sensitive(bEnableBitmap);
    m_xWidthMF->set_sensitive(bEnableBitmap);
    m_xHeightFT->set_sensitive(bEnableBitmap);
    m_xHeightMF->set_sensitive(bEnableBitmap);
    m_xRatioCB->set_sensitive(bEnableBitmap);
    m_xOrientFT->set_sensitive(bEnableBitmap);
    m_xOrientLB->set_sensitive(bEnableBitmap);
}

IMPL_LINK_NOARG(SvxNumOptionsTabPage, LevelHdl_Impl, weld::TreeView&, void)
{
    if (m_pLevelHdlEvent)
        return;
    // tdf#127112 (borrowing tdf#127120 solution) multiselection may be implemented by deselect follow by select so
    // fire off the handler to happen on next event loop and only process the
    // final state
    m_pLevelHdlEvent = Application::PostUserEvent(LINK(this, SvxNumOptionsTabPage, LevelHdl));
}

IMPL_LINK_NOARG(SvxNumOptionsTabPage, LevelHdl, void*, void)
{
    m_pLevelHdlEvent = nullptr;

    sal_uInt16 nSaveNumLvl = nActNumLvl;
    nActNumLvl = 0;
    std::vector<int> aSelectedRows = m_xLevelLB->get_selected_rows();
    if (std::find(aSelectedRows.begin(), aSelectedRows.end(), pActNum->GetLevelCount()) != aSelectedRows.end() &&
        (aSelectedRows.size() == 1 || nSaveNumLvl != 0xffff))
    {
        nActNumLvl = 0xFFFF;
        for( sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++ )
             m_xLevelLB->unselect(i);
    }
    else if (!aSelectedRows.empty())
    {
        sal_uInt16 nMask = 1;
        for( sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++ )
        {
            if (std::find(aSelectedRows.begin(), aSelectedRows.end(), i) != aSelectedRows.end())
                nActNumLvl |= nMask;
            nMask <<= 1;
        }
        m_xLevelLB->unselect(pActNum->GetLevelCount());
    }
    else
    {
        nActNumLvl = nSaveNumLvl;
        sal_uInt16 nMask = 1;
        for( sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++ )
        {
            if(nActNumLvl & nMask)
            {
                m_xLevelLB->select(i);
                break;
            }
            nMask <<=1;
        }
    }
    InitControls();
}

IMPL_LINK_NOARG(SvxNumOptionsTabPage, PreviewInvalidateHdl_Impl, Timer *, void)
{
    m_aPreviewWIN.Invalidate();
}

IMPL_LINK(SvxNumOptionsTabPage, AllLevelHdl_Impl, weld::SpinButton&, rBox, void)
{
    sal_uInt16 nMask = 1;
    for(sal_uInt16 e = 0; e < pActNum->GetLevelCount(); e++)
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aNumFmt(pActNum->GetLevel(e));
            aNumFmt.SetIncludeUpperLevels(static_cast<sal_uInt8>(std::min(rBox.get_value(), sal_Int64(e + 1))) );
            // Set the same prefix/suffix to generate list format with changed IncludedUpperLevels
            aNumFmt.SetListFormat(aNumFmt.GetPrefix(), aNumFmt.GetSuffix(), e);
            pActNum->SetLevel(e, aNumFmt);
        }
        nMask <<= 1;
    }
    SetModified();
}

IMPL_LINK(SvxNumOptionsTabPage, IsLegalHdl_Impl, weld::Toggleable&, rBox, void)
{
    bool bSet = rBox.get_active();
    for (sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if (nActNumLvl & (sal_uInt16(1) << i))
        {
            SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
            aNumFmt.SetIsLegal(bSet);
            pActNum->SetLevel(i, aNumFmt);
        }
    }
    SetModified();
}

IMPL_LINK(SvxNumOptionsTabPage, NumberTypeSelectHdl_Impl, weld::ComboBox&, rBox, void)
{
    OUString sSelectStyle;
    bool bShowOrient = false;
    bool bBmp = false;
    sal_uInt16 nMask = 1;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
            // PAGEDESC does not exist
            SvxNumType nNumType = static_cast<SvxNumType>(rBox.get_active_id().toUInt32());
            aNumFmt.SetNumberingType(nNumType);
            sal_uInt16 nNumberingType = aNumFmt.GetNumberingType();
            if(SVX_NUM_BITMAP == (nNumberingType&(~LINK_TOKEN)))
            {
                bBmp |= nullptr != aNumFmt.GetBrush();
                aNumFmt.SetIncludeUpperLevels( 1 );
                aNumFmt.SetListFormat(u""_ustr, u""_ustr, i);
                if(!bBmp)
                    aNumFmt.SetGraphic(u""_ustr);
                pActNum->SetLevel(i, aNumFmt);
                SwitchNumberType(NumberType::SHOW_BITMAP);
                bShowOrient = true;
            }
            else if( SVX_NUM_CHAR_SPECIAL == nNumberingType )
            {
                aNumFmt.SetIncludeUpperLevels( 1 );
                aNumFmt.SetListFormat(u""_ustr, u""_ustr, i);
                if( !aNumFmt.GetBulletFont() )
                    aNumFmt.SetBulletFont(&aActBulletFont);
                if( !aNumFmt.GetBulletChar() )
                    aNumFmt.SetBulletChar( SVX_DEF_BULLET );
                pActNum->SetLevel(i, aNumFmt);
                SwitchNumberType(NumberType::SHOW_BULLET);
                // allocation of the drawing pattern is automatic
                if(bAutomaticCharStyles)
                {
                    sSelectStyle = m_sBulletCharFormatName;
                }
            }
            else
            {
                aNumFmt.SetListFormat(m_xPrefixED->get_text(), m_xSuffixED->get_text(), i);

                SwitchNumberType(NumberType::SHOW_NUMBERING);
                pActNum->SetLevel(i, aNumFmt);
                CheckForStartValue_Impl(nNumberingType);

                // allocation of the drawing pattern is automatic
                if(bAutomaticCharStyles)
                {
                    sSelectStyle = m_sNumCharFmtName;
                }
            }
        }
        nMask <<= 1;
    }
    bool bAllLevelFeature = pActNum->IsFeatureSupported(SvxNumRuleFlags::CONTINUOUS);
    if(bShowOrient && bAllLevelFeature)
    {
        m_xOrientFT->show();
        m_xOrientLB->show();
    }
    else
    {
        m_xOrientFT->hide();
        m_xOrientLB->hide();
    }
    SetModified();
    if(!sSelectStyle.isEmpty())
    {
        m_xCharFmtLB->set_active_text(sSelectStyle);
        CharFmtHdl_Impl(*m_xCharFmtLB);
        bAutomaticCharStyles = true;
    }
}

void SvxNumOptionsTabPage::CheckForStartValue_Impl(sal_uInt16 nNumberingType)
{
    bool bIsNull = m_xStartED->get_value() == 0;
    bool bNoZeroAllowed = nNumberingType < SVX_NUM_ARABIC ||
                        SVX_NUM_CHARS_UPPER_LETTER_N == nNumberingType ||
                        SVX_NUM_CHARS_LOWER_LETTER_N == nNumberingType;
    m_xStartED->set_min(bNoZeroAllowed ? 1 : 0);
    if (bIsNull && bNoZeroAllowed)
        EditModifyHdl_Impl(*m_xStartED);
}

IMPL_LINK(SvxNumOptionsTabPage, OrientHdl_Impl, weld::ComboBox&, rBox, void)
{
    sal_Int32 nPos = rBox.get_active();
    nPos ++; // no VERT_NONE

    sal_uInt16 nMask = 1;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
            if(SVX_NUM_BITMAP == (aNumFmt.GetNumberingType()&(~LINK_TOKEN)))
            {
                const SvxBrushItem* pBrushItem =  aNumFmt.GetBrush();
                const Size& rSize = aNumFmt.GetGraphicSize();
                sal_Int16 eOrient = static_cast<sal_Int16>(nPos);
                aNumFmt.SetGraphicBrush( pBrushItem, &rSize, &eOrient );
                pActNum->SetLevel(i, aNumFmt);
            }
        }
        nMask <<= 1;
    }
    SetModified(false);
}

IMPL_LINK(SvxNumOptionsTabPage, SameLevelHdl_Impl, weld::Toggleable&, rBox, void)
{
    bool bSet = rBox.get_active();
    pActNum->SetContinuousNumbering(bSet);
    bool bRepaint = false;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
        if(aNumFmt.GetNumberingType() != SVX_NUM_NUMBER_NONE)
        {
            bRepaint = true;
            break;
        }
    }
    SetModified(bRepaint);
    InitControls();
}

IMPL_LINK(SvxNumOptionsTabPage, BulColorHdl_Impl, ColorListBox&, rColorBox, void)
{
    Color nSetColor = rColorBox.GetSelectEntryColor();

    sal_uInt16 nMask = 1;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
            aNumFmt.SetBulletColor(nSetColor);
            pActNum->SetLevel(i, aNumFmt);
        }
        nMask <<= 1;
    }
    SetModified();
}

IMPL_LINK(SvxNumOptionsTabPage, BulRelSizeHdl_Impl, weld::MetricSpinButton&, rField, void)
{
    sal_uInt16 nRelSize = rField.get_value(FieldUnit::PERCENT);

    sal_uInt16 nMask = 1;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
            aNumFmt.SetBulletRelSize(nRelSize);
            pActNum->SetLevel(i, aNumFmt);
        }
        nMask <<= 1;
    }
    SetModified();
}

IMPL_LINK(SvxNumOptionsTabPage, GraphicHdl_Impl, const OUString&, rIdent, void)
{
    OUString                aGrfName;
    Size                    aSize;
    bool                bSucc(false);
    SvxOpenGraphicDialog aGrfDlg(CuiResId(RID_CUISTR_EDIT_GRAPHIC), GetFrameWeld());

    std::u16string_view sNumber;
    if (rIdent.startsWith("gallery", &sNumber))
    {
        auto idx = o3tl::toUInt32(sNumber);
        if (idx < aGrfNames.size())
        {
            aGrfName = aGrfNames[idx];
            Graphic aGraphic;
            if(GalleryExplorer::GetGraphicObj( GALLERY_THEME_BULLETS, idx, &aGraphic))
            {
                aSize = SvxNumberFormat::GetGraphicSizeMM100(&aGraphic);
                bSucc = true;
            }
        }
    }
    else if (rIdent == "fromfile")
    {
        aGrfDlg.EnableLink( false );
        aGrfDlg.AsLink( false );
        if ( !aGrfDlg.Execute() )
        {
            // memorize selected filter
            aGrfName = aGrfDlg.GetPath();

            Graphic aGraphic;
            if( !aGrfDlg.GetGraphic(aGraphic) )
            {
                aSize = SvxNumberFormat::GetGraphicSizeMM100(&aGraphic);
                bSucc = true;
            }
        }
    }
    if(!bSucc)
        return;

    aSize = OutputDevice::LogicToLogic(aSize, MapMode(MapUnit::Map100thMM), MapMode(eCoreUnit));

    sal_uInt16 nMask = 1;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
            aNumFmt.SetCharFormatName(m_sNumCharFmtName);
            aNumFmt.SetGraphic(aGrfName);

            // set size for a later comparison
            const SvxBrushItem* pBrushItem = aNumFmt.GetBrush();
            // initiate asynchronous loading
            sal_Int16 eOrient = aNumFmt.GetVertOrient();
            aNumFmt.SetGraphicBrush( pBrushItem, &aSize, &eOrient );
            aInitSize[i] = aNumFmt.GetGraphicSize();

            pActNum->SetLevel(i, aNumFmt);
        }
        nMask <<= 1;
    }
    m_xRatioCB->set_sensitive(true);
    m_xWidthFT->set_sensitive(true);
    m_xHeightFT->set_sensitive(true);
    m_xWidthMF->set_sensitive(true);
    m_xHeightMF->set_sensitive(true);
    SetMetricValue(*m_xWidthMF, aSize.Width(), eCoreUnit);
    SetMetricValue(*m_xHeightMF, aSize.Height(), eCoreUnit);
    m_xOrientFT->set_sensitive(true);
    m_xOrientLB->set_sensitive(true);
    SetModified();
    //needed due to asynchronous loading of graphics in the SvxBrushItem
    aInvalidateTimer.Start();
}

IMPL_LINK_NOARG(SvxNumOptionsTabPage, PopupActivateHdl_Impl, weld::Toggleable&, void)
{
    if (m_xGalleryMenu)
        return;

    m_xGalleryMenu = m_xBuilder->weld_menu(u"gallerysubmenu"_ustr);
    weld::WaitObject aWait(GetFrameWeld());

    if (!GalleryExplorer::FillObjList(GALLERY_THEME_BULLETS, aGrfNames))
        return;

    GalleryExplorer::BeginLocking(GALLERY_THEME_BULLETS);

    Graphic aGraphic;
    OUString sGrfName;
    ScopedVclPtrInstance< VirtualDevice > pVD;
    size_t i = 0;
    for (const auto & grfName : aGrfNames)
    {
        sGrfName = grfName;
        OUString sItemId = "gallery" + OUString::number(i);
        INetURLObject aObj(sGrfName);
        if (aObj.GetProtocol() == INetProtocol::File)
        {
            // tdf#141334 - only show the last name of the filename without its extension
            aObj.removeExtension();
            sGrfName = aObj.GetLastName(INetURLObject::DecodeMechanism::Unambiguous);
        }
        if(GalleryExplorer::GetGraphicObj( GALLERY_THEME_BULLETS, i, &aGraphic))
        {
            BitmapEx aBitmap(aGraphic.GetBitmapEx());
            Size aSize(aBitmap.GetSizePixel());
            if(aSize.Width() > MAX_BMP_WIDTH ||
                aSize.Height() > MAX_BMP_HEIGHT)
            {
                bool bWidth = aSize.Width() > aSize.Height();
                double nScale = bWidth ?
                                    double(MAX_BMP_WIDTH) / static_cast<double>(aSize.Width()):
                                        double(MAX_BMP_HEIGHT) / static_cast<double>(aSize.Height());
                aBitmap.Scale(nScale, nScale);
            }
            pVD->SetOutputSizePixel(aBitmap.GetSizePixel(), false);
            pVD->DrawBitmapEx(Point(), aBitmap);
            m_xGalleryMenu->append(sItemId, sGrfName, *pVD);
        }
        else
        {
            m_xGalleryMenu->append(sItemId, sGrfName);
        }
        ++i;
    }
    GalleryExplorer::EndLocking(GALLERY_THEME_BULLETS);
}

IMPL_LINK_NOARG(SvxNumOptionsTabPage, BulletHdl_Impl, weld::Button&, void)
{
    SvxCharacterMap aMap(GetFrameWeld(), nullptr, nullptr);

    sal_uInt16 nMask = 1;
    std::optional<vcl::Font> pFmtFont;
    bool bSameBullet = true;
    sal_UCS4 cBullet = 0;
    bool bFirst = true;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & nMask)
        {
            const SvxNumberFormat&  rCurFmt = pActNum->GetLevel(i);
            if(bFirst)
            {
                 cBullet = rCurFmt.GetBulletChar();
            }
            else if(rCurFmt.GetBulletChar() != cBullet )
            {
                bSameBullet = false;
                break;
            }
            if(!pFmtFont)
                pFmtFont = rCurFmt.GetBulletFont();
            bFirst = false;
        }
        nMask <<= 1;

    }

    if (pFmtFont)
        aMap.SetCharFont(*pFmtFont);
    else
        aMap.SetCharFont(aActBulletFont);
    if (bSameBullet)
        aMap.SetChar(cBullet);
    if (aMap.run() != RET_OK)
        return;

    // change Font Numrules
    aActBulletFont = aMap.GetCharFont();

    sal_uInt16 _nMask = 1;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & _nMask)
        {
            SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
            aNumFmt.SetBulletFont(&aActBulletFont);
            aNumFmt.SetBulletChar(aMap.GetChar());
            pActNum->SetLevel(i, aNumFmt);
        }
        _nMask <<= 1;
    }

    SetModified();
}

IMPL_LINK( SvxNumOptionsTabPage, SizeHdl_Impl, weld::MetricSpinButton&, rField, void)
{
    bool bWidth = &rField == m_xWidthMF.get();
    bLastWidthModified = bWidth;
    bool bRatio = m_xRatioCB->get_active();
    tools::Long nWidthVal = static_cast<tools::Long>(m_xWidthMF->denormalize(m_xWidthMF->get_value(FieldUnit::MM_100TH)));
    tools::Long nHeightVal = static_cast<tools::Long>(m_xHeightMF->denormalize(m_xHeightMF->get_value(FieldUnit::MM_100TH)));
    nWidthVal = OutputDevice::LogicToLogic( nWidthVal ,
                                                MapUnit::Map100thMM, eCoreUnit );
    nHeightVal = OutputDevice::LogicToLogic( nHeightVal,
                                                MapUnit::Map100thMM, eCoreUnit);
    double  fSizeRatio;

    bool bRepaint = false;
    sal_uInt16 nMask = 1;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
            if(SVX_NUM_BITMAP == (aNumFmt.GetNumberingType()&(~LINK_TOKEN)))
            {
                Size aSize(aNumFmt.GetGraphicSize() );
                Size aSaveSize(aSize);

                if (aInitSize[i].Height())
                    fSizeRatio = static_cast<double>(aInitSize[i].Width()) / static_cast<double>(aInitSize[i].Height());
                else
                    fSizeRatio = double(1);

                if(bWidth)
                {
                    tools::Long nDelta = nWidthVal - aInitSize[i].Width();
                    aSize.setWidth( nWidthVal );
                    if (bRatio)
                    {
                        aSize.setHeight( aInitSize[i].Height() + static_cast<tools::Long>(static_cast<double>(nDelta) / fSizeRatio) );
                        m_xHeightMF->set_value(m_xHeightMF->normalize(
                            OutputDevice::LogicToLogic( aSize.Height(), eCoreUnit, MapUnit::Map100thMM )),
                                FieldUnit::MM_100TH);
                    }
                }
                else
                {
                    tools::Long nDelta = nHeightVal - aInitSize[i].Height();
                    aSize.setHeight( nHeightVal );
                    if (bRatio)
                    {
                        aSize.setWidth( aInitSize[i].Width() + static_cast<tools::Long>(static_cast<double>(nDelta) * fSizeRatio) );
                        m_xWidthMF->set_value(m_xWidthMF->normalize(
                            OutputDevice::LogicToLogic( aSize.Width(), eCoreUnit, MapUnit::Map100thMM )),
                                FieldUnit::MM_100TH);
                    }
                }
                const SvxBrushItem* pBrushItem =  aNumFmt.GetBrush();
                sal_Int16 eOrient = aNumFmt.GetVertOrient();
                if(aSize != aSaveSize)
                    bRepaint = true;
                aNumFmt.SetGraphicBrush( pBrushItem, &aSize, &eOrient );
                pActNum->SetLevel(i, aNumFmt);
            }
        }
        nMask <<= 1;
    }
    SetModified(bRepaint);
}

IMPL_LINK(SvxNumOptionsTabPage, RatioHdl_Impl, weld::Toggleable&, rBox, void)
{
    m_xCbxScaleImg->set_from_icon_name(m_xRatioCB->get_active() ? RID_SVXBMP_LOCKED : RID_SVXBMP_UNLOCKED);
    if (rBox.get_active())
    {
        if (bLastWidthModified)
            SizeHdl_Impl(*m_xWidthMF);
        else
            SizeHdl_Impl(*m_xHeightMF);
    }
}

IMPL_LINK_NOARG(SvxNumOptionsTabPage, CharFmtHdl_Impl, weld::ComboBox&, void)
{
    bAutomaticCharStyles = false;
    sal_Int32 nEntryPos = m_xCharFmtLB->get_active();
    OUString sEntry = m_xCharFmtLB->get_active_text();
    sal_uInt16 nMask = 1;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
            if( 0 == nEntryPos )
                aNumFmt.SetCharFormatName(u""_ustr);
            else
            {
                if(SVX_NUM_BITMAP != (aNumFmt.GetNumberingType()&(~LINK_TOKEN)))
                    aNumFmt.SetCharFormatName(sEntry);
            }
            pActNum->SetLevel(i, aNumFmt);
        }
        nMask <<= 1;
    }
    SetModified(false);
};

IMPL_LINK(SvxNumOptionsTabPage, EditModifyHdl_Impl, weld::Entry&, rEdit, void)
{
    EditModifyHdl_Impl(&rEdit);
}

IMPL_LINK(SvxNumOptionsTabPage, SpinModifyHdl_Impl, weld::SpinButton&, rSpinButton, void)
{
    EditModifyHdl_Impl(&rSpinButton);
}

void SvxNumOptionsTabPage::EditModifyHdl_Impl(const weld::Entry* pEdit)
{
    bool bPrefixSuffix = (pEdit == m_xPrefixED.get())|| (pEdit == m_xSuffixED.get());
    bool bStart = pEdit == m_xStartED.get();
    sal_uInt16 nMask = 1;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
            if (bPrefixSuffix)
                aNumFmt.SetListFormat(m_xPrefixED->get_text(), m_xSuffixED->get_text(), i);
            else if(bStart)
                aNumFmt.SetStart(m_xStartED->get_value());
            pActNum->SetLevel(i, aNumFmt);
        }
        nMask <<= 1;
    }
    SetModified();
}

//See uiconfig/swriter/ui/outlinepositionpage.ui for effectively a duplicate
//dialog to this one, except with a different preview window impl.
//TODO, determine if SwNumPositionTabPage and SvxNumPositionTabPage can be
//merged
SvxNumPositionTabPage::SvxNumPositionTabPage(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rSet)
    : SfxTabPage(pPage, pController, u"cui/ui/numberingpositionpage.ui"_ustr, u"NumberingPositionPage"_ustr, &rSet)
    , m_pLevelHdlEvent(nullptr)
    , nActNumLvl(1)
    , nNumItemId(SID_ATTR_NUMBERING_RULE)
    , bModified(false)
    , bPreset(false)
    , bInInintControl(false)
    , bLabelAlignmentPosAndSpaceModeActive(false)
    , m_xLevelLB(m_xBuilder->weld_tree_view(u"levellb"_ustr))
    , m_xDistBorderFT(m_xBuilder->weld_label(u"indent"_ustr))
    , m_xDistBorderMF(m_xBuilder->weld_metric_spin_button(u"indentmf"_ustr, FieldUnit::CM))
    , m_xRelativeCB(m_xBuilder->weld_check_button(u"relative"_ustr))
    , m_xIndentFT(m_xBuilder->weld_label(u"numberingwidth"_ustr))
    , m_xIndentMF(m_xBuilder->weld_metric_spin_button(u"numberingwidthmf"_ustr, FieldUnit::CM))
    , m_xDistNumFT(m_xBuilder->weld_label(u"numdist"_ustr))
    , m_xDistNumMF(m_xBuilder->weld_metric_spin_button(u"numdistmf"_ustr, FieldUnit::CM))
    , m_xAlignFT(m_xBuilder->weld_label(u"numalign"_ustr))
    , m_xAlignLB(m_xBuilder->weld_combo_box(u"numalignlb"_ustr))
    , m_xLabelFollowedByFT(m_xBuilder->weld_label(u"numfollowedby"_ustr))
    , m_xLabelFollowedByLB(m_xBuilder->weld_combo_box(u"numfollowedbylb"_ustr))
    , m_xListtabFT(m_xBuilder->weld_label(u"at"_ustr))
    , m_xListtabMF(m_xBuilder->weld_metric_spin_button(u"atmf"_ustr, FieldUnit::CM))
    , m_xAlign2FT(m_xBuilder->weld_label(u"num2align"_ustr))
    , m_xAlign2LB(m_xBuilder->weld_combo_box(u"num2alignlb"_ustr))
    , m_xAlignedAtFT(m_xBuilder->weld_label(u"alignedat"_ustr))
    , m_xAlignedAtMF(m_xBuilder->weld_metric_spin_button(u"alignedatmf"_ustr, FieldUnit::CM))
    , m_xIndentAtFT(m_xBuilder->weld_label(u"indentat"_ustr))
    , m_xIndentAtMF(m_xBuilder->weld_metric_spin_button(u"indentatmf"_ustr, FieldUnit::CM))
    , m_xStandardPB(m_xBuilder->weld_button(u"standard"_ustr))
    , m_xPreviewWIN(new weld::CustomWeld(*m_xBuilder, u"preview"_ustr, m_aPreviewWIN))
{
    SetExchangeSupport();

    // set metric
    FieldUnit eFUnit = GetModuleFieldUnit(rSet);

    SetFieldUnit( *m_xDistBorderMF, eFUnit );
    SetFieldUnit( *m_xIndentMF, eFUnit );
    SetFieldUnit( *m_xDistNumMF, eFUnit );

    m_xAlignedAtMF->set_range(0, SAL_MAX_INT32, FieldUnit::NONE);
    m_xListtabMF->set_range(0, SAL_MAX_INT32, FieldUnit::NONE);
    m_xIndentAtMF->set_range(0, SAL_MAX_INT32, FieldUnit::NONE);

    m_xRelativeCB->set_active(true);
    m_xAlignLB->connect_changed(LINK(this, SvxNumPositionTabPage, EditModifyHdl_Impl));
    m_xAlign2LB->connect_changed(LINK(this, SvxNumPositionTabPage, EditModifyHdl_Impl));
    for ( sal_Int32 i = 0; i < m_xAlignLB->get_count(); ++i )
    {
        m_xAlign2LB->append_text(m_xAlignLB->get_text(i));
    }

    Link<weld::MetricSpinButton&,void> aLk3 = LINK(this, SvxNumPositionTabPage, DistanceHdl_Impl);
    m_xDistBorderMF->connect_value_changed(aLk3);
    m_xDistNumMF->connect_value_changed(aLk3);
    m_xIndentMF->connect_value_changed(aLk3);

    m_xLabelFollowedByLB->connect_changed(LINK(this, SvxNumPositionTabPage, LabelFollowedByHdl_Impl));

    m_xListtabMF->connect_value_changed(LINK(this, SvxNumPositionTabPage, ListtabPosHdl_Impl));
    m_xAlignedAtMF->connect_value_changed(LINK(this, SvxNumPositionTabPage, AlignAtHdl_Impl));
    m_xIndentAtMF->connect_value_changed(LINK(this, SvxNumPositionTabPage, IndentAtHdl_Impl));

    m_xLevelLB->set_selection_mode(SelectionMode::Multiple);
    m_xLevelLB->connect_selection_changed(LINK(this, SvxNumPositionTabPage, LevelHdl_Impl));
    m_xRelativeCB->connect_toggled(LINK(this, SvxNumPositionTabPage, RelativeHdl_Impl));
    m_xStandardPB->connect_clicked(LINK(this, SvxNumPositionTabPage, StandardHdl_Impl));

    m_xRelativeCB->set_active(bLastRelative);
    m_aPreviewWIN.SetPositionMode();
    eCoreUnit = rSet.GetPool()->GetMetric(rSet.GetPool()->GetWhichIDFromSlotID(SID_ATTR_NUMBERING_RULE));
}

SvxNumPositionTabPage::~SvxNumPositionTabPage()
{
    if (m_pLevelHdlEvent)
    {
        Application::RemoveUserEvent(m_pLevelHdlEvent);
        m_pLevelHdlEvent = nullptr;
    }
    m_xPreviewWIN.reset();
}

/*-------------------------------------------------------*/

void SvxNumPositionTabPage::InitControls()
{
    bInInintControl = true;
    const bool bRelative = !bLabelAlignmentPosAndSpaceModeActive &&
                     m_xRelativeCB->get_sensitive() && m_xRelativeCB->get_active();
    const bool bSingleSelection = m_xLevelLB->count_selected_rows() == 1 &&
                            SAL_MAX_UINT16 != nActNumLvl;

    m_xDistBorderMF->set_sensitive( !bLabelAlignmentPosAndSpaceModeActive &&
                          ( bSingleSelection || bRelative ) );
    m_xDistBorderFT->set_sensitive( !bLabelAlignmentPosAndSpaceModeActive &&
                          ( bSingleSelection || bRelative ) );

    bool bSetDistEmpty = false;
    bool bSameDistBorderNum = !bLabelAlignmentPosAndSpaceModeActive;
    bool bSameDist      = !bLabelAlignmentPosAndSpaceModeActive;
    bool bSameIndent    = !bLabelAlignmentPosAndSpaceModeActive;
    bool bSameAdjust    = true;

    bool bSameLabelFollowedBy = bLabelAlignmentPosAndSpaceModeActive;
    bool bSameListtab = bLabelAlignmentPosAndSpaceModeActive;
    bool bSameAlignAt = bLabelAlignmentPosAndSpaceModeActive;
    bool bSameIndentAt = bLabelAlignmentPosAndSpaceModeActive;

    const SvxNumberFormat* aNumFmtArr[SVX_MAX_NUM];
    sal_uInt16 nMask = 1;
    sal_uInt16 nLvl = SAL_MAX_UINT16;
    tools::Long nFirstBorderTextRelative = -1;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        aNumFmtArr[i] = &pActNum->GetLevel(i);
        if(nActNumLvl & nMask)
        {
            if(SAL_MAX_UINT16 == nLvl)
                nLvl = i;

            if( i > nLvl)
            {
                bSameAdjust &= aNumFmtArr[i]->GetNumAdjust() == aNumFmtArr[nLvl]->GetNumAdjust();
                if ( !bLabelAlignmentPosAndSpaceModeActive )
                {
                    if(bRelative)
                    {
                        if(nFirstBorderTextRelative == -1)
                            nFirstBorderTextRelative =
                            (aNumFmtArr[i]->GetAbsLSpace() + aNumFmtArr[i]->GetFirstLineOffset() -
                            aNumFmtArr[i - 1]->GetAbsLSpace() + aNumFmtArr[i - 1]->GetFirstLineOffset());
                        else
                            bSameDistBorderNum &= nFirstBorderTextRelative ==
                            (aNumFmtArr[i]->GetAbsLSpace() + aNumFmtArr[i]->GetFirstLineOffset() -
                            aNumFmtArr[i - 1]->GetAbsLSpace() + aNumFmtArr[i - 1]->GetFirstLineOffset());
                    }
                    else
                        bSameDistBorderNum &=
                        aNumFmtArr[i]->GetAbsLSpace() - aNumFmtArr[i]->GetFirstLineOffset() ==
                        aNumFmtArr[i - 1]->GetAbsLSpace() - aNumFmtArr[i - 1]->GetFirstLineOffset();

                    bSameDist       &= aNumFmtArr[i]->GetCharTextDistance() == aNumFmtArr[nLvl]->GetCharTextDistance();
                    bSameIndent     &= aNumFmtArr[i]->GetFirstLineOffset() == aNumFmtArr[nLvl]->GetFirstLineOffset();
                }
                else
                {
                    bSameLabelFollowedBy &=
                        aNumFmtArr[i]->GetLabelFollowedBy() == aNumFmtArr[nLvl]->GetLabelFollowedBy();
                    bSameListtab &=
                        aNumFmtArr[i]->GetListtabPos() == aNumFmtArr[nLvl]->GetListtabPos();
                    bSameAlignAt &=
                        ( ( aNumFmtArr[i]->GetIndentAt() + aNumFmtArr[i]->GetFirstLineIndent() )
                            == ( aNumFmtArr[nLvl]->GetIndentAt() + aNumFmtArr[nLvl]->GetFirstLineIndent() ) );
                    bSameIndentAt &=
                        aNumFmtArr[i]->GetIndentAt() == aNumFmtArr[nLvl]->GetIndentAt();
                }
            }
        }
        nMask <<= 1;

    }
    if (SVX_MAX_NUM <= nLvl)
    {
        OSL_ENSURE(false, "cannot happen.");
        return;
    }

    if(bSameDistBorderNum)
    {
        tools::Long nDistBorderNum;
        if(bRelative)
        {
            nDistBorderNum = static_cast<tools::Long>(aNumFmtArr[nLvl]->GetAbsLSpace())+ aNumFmtArr[nLvl]->GetFirstLineOffset();
            if(nLvl)
                nDistBorderNum -= static_cast<tools::Long>(aNumFmtArr[nLvl - 1]->GetAbsLSpace())+ aNumFmtArr[nLvl - 1]->GetFirstLineOffset();
        }
        else
        {
            nDistBorderNum = static_cast<tools::Long>(aNumFmtArr[nLvl]->GetAbsLSpace())+ aNumFmtArr[nLvl]->GetFirstLineOffset();
        }
        SetMetricValue(*m_xDistBorderMF, nDistBorderNum, eCoreUnit);
    }
    else
        bSetDistEmpty = true;

    if(bSameDist)
        SetMetricValue(*m_xDistNumMF, aNumFmtArr[nLvl]->GetCharTextDistance(), eCoreUnit);
    else
        m_xDistNumMF->set_text(u""_ustr);
    if(bSameIndent)
        SetMetricValue(*m_xIndentMF, - aNumFmtArr[nLvl]->GetFirstLineOffset(), eCoreUnit);
    else
        m_xIndentMF->set_text(u""_ustr);

    if(bSameAdjust)
    {
        sal_Int32 nPos = 1; // centered
        if(aNumFmtArr[nLvl]->GetNumAdjust() == SvxAdjust::Left)
            nPos = 0;
        else if(aNumFmtArr[nLvl]->GetNumAdjust() == SvxAdjust::Right)
            nPos = 2;
        m_xAlignLB->set_active(nPos);
        m_xAlign2LB->set_active(nPos);
    }
    else
    {
        m_xAlignLB->set_active(-1);
        m_xAlign2LB->set_active(-1);
    }

    if ( bSameLabelFollowedBy )
    {
        sal_Int32 nPos = 0; // LISTTAB
        if ( aNumFmtArr[nLvl]->GetLabelFollowedBy() == SvxNumberFormat::SPACE )
        {
            nPos = 1;
        }
        else if ( aNumFmtArr[nLvl]->GetLabelFollowedBy() == SvxNumberFormat::NOTHING )
        {
            nPos = 2;
        }
        else if ( aNumFmtArr[nLvl]->GetLabelFollowedBy() == SvxNumberFormat::NEWLINE )
        {
            nPos = 3;
        }
        m_xLabelFollowedByLB->set_active(nPos);
    }
    else
    {
        m_xLabelFollowedByLB->set_active(-1);
    }

    if ( aNumFmtArr[nLvl]->GetLabelFollowedBy() == SvxNumberFormat::LISTTAB )
    {
        m_xListtabFT->set_sensitive(true);
        m_xListtabMF->set_sensitive(true);
        if ( bSameListtab )
        {
            SetMetricValue(*m_xListtabMF, aNumFmtArr[nLvl]->GetListtabPos(), eCoreUnit);
        }
        else
        {
            m_xListtabMF->set_text(u""_ustr);
        }
    }
    else
    {
        m_xListtabFT->set_sensitive(false);
        m_xListtabMF->set_sensitive(false);
        m_xListtabMF->set_text(u""_ustr);
    }

    if ( bSameAlignAt )
    {
        SetMetricValue(*m_xAlignedAtMF,
                        aNumFmtArr[nLvl]->GetIndentAt() + aNumFmtArr[nLvl]->GetFirstLineIndent(),
                        eCoreUnit);
    }
    else
    {
        m_xAlignedAtMF->set_text(u""_ustr);
    }

    if ( bSameIndentAt )
    {
        SetMetricValue(*m_xIndentAtMF, aNumFmtArr[nLvl]->GetIndentAt(), eCoreUnit);
    }
    else
    {
        m_xIndentAtMF->set_text(u""_ustr);
    }

    if ( bSetDistEmpty )
        m_xDistBorderMF->set_text(u""_ustr);

    bInInintControl = false;
}

void SvxNumPositionTabPage::ActivatePage(const SfxItemSet& rSet)
{
    sal_uInt16 nTmpNumLvl = 1;
    const SfxItemSet* pExampleSet = GetDialogExampleSet();
    if(pExampleSet)
    {
        if(const SfxBoolItem* pPresetItem = pExampleSet->GetItemIfSet(SID_PARAM_NUM_PRESET, false))
            bPreset = pPresetItem->GetValue();
        if(const SfxUInt16Item* pLevelItem = pExampleSet->GetItemIfSet(SID_PARAM_CUR_NUM_LEVEL, false))
            nTmpNumLvl = pLevelItem->GetValue();
    }
    if(const SvxNumBulletItem* pBulletItem = rSet.GetItemIfSet(nNumItemId, false))
    {
        pSaveNum.reset( new SvxNumRule(pBulletItem->GetNumRule()) );
    }
    bModified = (!pActNum->Get( 0 ) || bPreset);
    if(*pSaveNum != *pActNum ||
        nActNumLvl != nTmpNumLvl )
    {
        *pActNum = *pSaveNum;
        nActNumLvl = nTmpNumLvl;
        sal_uInt16 nMask = 1;
        m_xLevelLB->unselect_all();
        if (nActNumLvl == SAL_MAX_UINT16)
            m_xLevelLB->select(pActNum->GetLevelCount());
        if (nActNumLvl != SAL_MAX_UINT16)
            for (sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
            {
                if (nActNumLvl & nMask)
                    m_xLevelLB->select(i);
                nMask <<= 1 ;
            }
        m_xRelativeCB->set_sensitive(nActNumLvl != 1);

        InitPosAndSpaceMode();
        ShowControlsDependingOnPosAndSpaceMode();

        InitControls();
    }
    m_aPreviewWIN.SetLevel(nActNumLvl);
    m_aPreviewWIN.Invalidate();
}

DeactivateRC SvxNumPositionTabPage::DeactivatePage(SfxItemSet *_pSet)
{
    if(_pSet)
    {
        if (m_xDistBorderMF->get_sensitive())
            DistanceHdl_Impl(*m_xDistBorderMF);
        DistanceHdl_Impl(*m_xIndentMF);
        FillItemSet(_pSet);
    }
    return DeactivateRC::LeavePage;
}

bool SvxNumPositionTabPage::FillItemSet( SfxItemSet* rSet )
{
    rSet->Put(SfxUInt16Item(SID_PARAM_CUR_NUM_LEVEL, nActNumLvl));

    if(bModified && pActNum)
    {
        *pSaveNum = *pActNum;
        rSet->Put(SvxNumBulletItem( *pSaveNum, nNumItemId ));
        rSet->Put(SfxBoolItem(SID_PARAM_NUM_PRESET, false));
    }
    return bModified;
}

void SvxNumPositionTabPage::Reset( const SfxItemSet* rSet )
{
    // in Draw the item exists as WhichId, in Writer only as SlotId
    const SvxNumBulletItem* pItem =
        rSet->GetItemIfSet(SID_ATTR_NUMBERING_RULE, false);
    if(!pItem)
    {
        nNumItemId = rSet->GetPool()->GetWhichIDFromSlotID(SID_ATTR_NUMBERING_RULE);
        pItem = rSet->GetItemIfSet(nNumItemId, false);

        if( !pItem )
        {
            pItem = & rSet->Get( nNumItemId );
        }
    }
    DBG_ASSERT(pItem, "no item found!");
    pSaveNum.reset( new SvxNumRule(pItem->GetNumRule()) );

    // insert levels
    if (!m_xLevelLB->count_selected_rows())
    {
        for(sal_uInt16 i = 1; i <= pSaveNum->GetLevelCount(); i++)
            m_xLevelLB->append_text(OUString::number(i));
        if(pSaveNum->GetLevelCount() > 1)
        {
            OUString sEntry = "1 - " + OUString::number( pSaveNum->GetLevelCount() );
            m_xLevelLB->append_text(sEntry);
            m_xLevelLB->select_text(sEntry);
        }
        else
            m_xLevelLB->select(0);
    }
    else
        m_xLevelLB->select(m_xLevelLB->count_selected_rows() - 1);
    sal_uInt16 nMask = 1;
    m_xLevelLB->unselect_all();
    if (nActNumLvl == SAL_MAX_UINT16)
    {
        m_xLevelLB->select(pSaveNum->GetLevelCount());
    }
    else
    {
        for(sal_uInt16 i = 0; i < pSaveNum->GetLevelCount(); i++)
        {
            if(nActNumLvl & nMask)
                m_xLevelLB->select(i);
            nMask <<= 1;
        }
    }

    if(!pActNum)
        pActNum.reset( new SvxNumRule(*pSaveNum) );
    else if(*pSaveNum != *pActNum)
        *pActNum = *pSaveNum;
    m_aPreviewWIN.SetNumRule(pActNum.get());

    InitPosAndSpaceMode();
    ShowControlsDependingOnPosAndSpaceMode();

    InitControls();
    bModified = false;
}

void SvxNumPositionTabPage::InitPosAndSpaceMode()
{
    if ( pActNum == nullptr )
    {
        SAL_WARN( "cui.tabpages",
                "<SvxNumPositionTabPage::InitPosAndSpaceMode()> - misusage of method -> <pAktNum> has to be already set!" );
        return;
    }

    SvxNumberFormat::SvxNumPositionAndSpaceMode ePosAndSpaceMode =
                                            SvxNumberFormat::LABEL_ALIGNMENT;
    sal_uInt16 nMask = 1;
    for( sal_uInt16 i = 0; i < pActNum->GetLevelCount(); ++i )
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aNumFmt( pActNum->GetLevel(i) );
            ePosAndSpaceMode = aNumFmt.GetPositionAndSpaceMode();
            if ( ePosAndSpaceMode == SvxNumberFormat::LABEL_ALIGNMENT )
            {
                break;
            }
        }
        nMask <<= 1;
    }

    bLabelAlignmentPosAndSpaceModeActive =
                    ePosAndSpaceMode == SvxNumberFormat::LABEL_ALIGNMENT;
}

void SvxNumPositionTabPage::ShowControlsDependingOnPosAndSpaceMode()
{
    m_xDistBorderFT->set_visible( !bLabelAlignmentPosAndSpaceModeActive );
    m_xDistBorderMF->set_visible( !bLabelAlignmentPosAndSpaceModeActive );
    m_xRelativeCB->set_visible( !bLabelAlignmentPosAndSpaceModeActive );
    m_xIndentFT->set_visible( !bLabelAlignmentPosAndSpaceModeActive );
    m_xIndentMF->set_visible( !bLabelAlignmentPosAndSpaceModeActive );
    m_xDistNumFT->set_visible( !bLabelAlignmentPosAndSpaceModeActive &&
                    pActNum->IsFeatureSupported(SvxNumRuleFlags::CONTINUOUS) );
    m_xDistNumMF->set_visible( !bLabelAlignmentPosAndSpaceModeActive &&
                    pActNum->IsFeatureSupported(SvxNumRuleFlags::CONTINUOUS));
    m_xAlignFT->set_visible( !bLabelAlignmentPosAndSpaceModeActive );
    m_xAlignLB->set_visible( !bLabelAlignmentPosAndSpaceModeActive );

    m_xLabelFollowedByFT->set_visible( bLabelAlignmentPosAndSpaceModeActive );
    m_xLabelFollowedByLB->set_visible( bLabelAlignmentPosAndSpaceModeActive );
    m_xListtabFT->set_visible( bLabelAlignmentPosAndSpaceModeActive );
    m_xListtabMF->set_visible( bLabelAlignmentPosAndSpaceModeActive );
    m_xAlign2FT->set_visible( bLabelAlignmentPosAndSpaceModeActive );
    m_xAlign2LB->set_visible( bLabelAlignmentPosAndSpaceModeActive );
    m_xAlignedAtFT->set_visible( bLabelAlignmentPosAndSpaceModeActive );
    m_xAlignedAtMF->set_visible( bLabelAlignmentPosAndSpaceModeActive );
    m_xIndentAtFT->set_visible( bLabelAlignmentPosAndSpaceModeActive );
    m_xIndentAtMF->set_visible( bLabelAlignmentPosAndSpaceModeActive );
}

std::unique_ptr<SfxTabPage> SvxNumPositionTabPage::Create(weld::Container* pPage, weld::DialogController* pController,
                                                 const SfxItemSet* rAttrSet)
{
    return std::make_unique<SvxNumPositionTabPage>(pPage, pController, *rAttrSet);
}

void SvxNumPositionTabPage::SetMetric(FieldUnit eMetric)
{
    if (eMetric == FieldUnit::MM)
    {
        m_xDistBorderMF->set_digits(1);
        m_xDistNumMF->set_digits(1);
        m_xIndentMF->set_digits(1);
        m_xListtabMF->set_digits(1);
        m_xAlignedAtMF->set_digits(1);
        m_xIndentAtMF->set_digits(1);
    }
    m_xDistBorderMF->set_unit(eMetric);
    m_xDistNumMF->set_unit(eMetric);
    m_xIndentMF->set_unit(eMetric);
    m_xListtabMF->set_unit(eMetric);
    m_xAlignedAtMF->set_unit(eMetric);
    m_xIndentAtMF->set_unit(eMetric);
}

IMPL_LINK_NOARG(SvxNumPositionTabPage, EditModifyHdl_Impl, weld::ComboBox&, void)
{
    sal_uInt16 nMask = 1;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aNumFmt(pActNum->GetLevel(i));

            const sal_Int32 nPos = m_xAlignLB->get_visible()
                                ? m_xAlignLB->get_active()
                                : m_xAlign2LB->get_active();
            SvxAdjust eAdjust = SvxAdjust::Center;
            if(nPos == 0)
                eAdjust = SvxAdjust::Left;
            else if(nPos == 2)
                eAdjust = SvxAdjust::Right;
            aNumFmt.SetNumAdjust( eAdjust );
            pActNum->SetLevel(i, aNumFmt);
        }
        nMask <<= 1;
    }
    SetModified();
}

IMPL_LINK_NOARG(SvxNumPositionTabPage, LevelHdl_Impl, weld::TreeView&, void)
{
    if (m_pLevelHdlEvent)
        return;
    // tdf#127120 multiselection may be implemented by deselect follow by select so
    // fire off the handler to happen on next event loop and only process the
    // final state
    m_pLevelHdlEvent = Application::PostUserEvent(LINK(this, SvxNumPositionTabPage, LevelHdl));
}

IMPL_LINK_NOARG(SvxNumPositionTabPage, LevelHdl, void*, void)
{
    m_pLevelHdlEvent = nullptr;

    sal_uInt16 nSaveNumLvl = nActNumLvl;
    nActNumLvl = 0;
    std::vector<int> aSelectedRows = m_xLevelLB->get_selected_rows();
    if (std::find(aSelectedRows.begin(), aSelectedRows.end(), pActNum->GetLevelCount()) != aSelectedRows.end() &&
            (aSelectedRows.size() == 1 || nSaveNumLvl != 0xffff))
    {
        nActNumLvl = 0xFFFF;
        for( sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++ )
            m_xLevelLB->unselect(i);
    }
    else if (!aSelectedRows.empty())
    {
        sal_uInt16 nMask = 1;
        for( sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++ )
        {
            if (std::find(aSelectedRows.begin(), aSelectedRows.end(), i) != aSelectedRows.end())
                nActNumLvl |= nMask;
            nMask <<= 1;
        }
        m_xLevelLB->unselect(pActNum->GetLevelCount());
    }
    else
    {
        nActNumLvl = nSaveNumLvl;
        sal_uInt16 nMask = 1;
        for( sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++ )
        {
            if(nActNumLvl & nMask)
            {
                m_xLevelLB->select(i);
                break;
            }
            nMask <<=1;
        }
    }
    m_xRelativeCB->set_sensitive(nActNumLvl != 1);
    SetModified();
    InitPosAndSpaceMode();
    ShowControlsDependingOnPosAndSpaceMode();
    InitControls();
}

IMPL_LINK(SvxNumPositionTabPage, DistanceHdl_Impl, weld::MetricSpinButton&, rFld, void)
{
    if(bInInintControl)
        return;
    tools::Long nValue = GetCoreValue(rFld, eCoreUnit);
    sal_uInt16 nMask = 1;
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aNumFmt( pActNum->GetLevel( i ) );
            if (&rFld == m_xDistBorderMF.get())
            {

                if (m_xRelativeCB->get_active())
                {
                    if(0 == i)
                    {
                        auto const nTmp = aNumFmt.GetFirstLineOffset();
                        aNumFmt.SetAbsLSpace( nValue - nTmp);
                    }
                    else
                    {
                        tools::Long nTmp = pActNum->GetLevel( i - 1 ).GetAbsLSpace() +
                                    pActNum->GetLevel( i - 1 ).GetFirstLineOffset() -
                                    pActNum->GetLevel( i ).GetFirstLineOffset();

                        aNumFmt.SetAbsLSpace( nValue + nTmp);
                    }
                }
                else
                {
                    aNumFmt.SetAbsLSpace( nValue - aNumFmt.GetFirstLineOffset());
                }
            }
            else if (&rFld == m_xDistNumMF.get())
            {
                aNumFmt.SetCharTextDistance( static_cast<short>(nValue) );
            }
            else if (&rFld == m_xIndentMF.get())
            {
                // together with the FirstLineOffset the AbsLSpace must be changed, too
                tools::Long nDiff = nValue + aNumFmt.GetFirstLineOffset();
                auto const nAbsLSpace = aNumFmt.GetAbsLSpace();
                aNumFmt.SetAbsLSpace(nAbsLSpace + nDiff);
                aNumFmt.SetFirstLineOffset( -nValue );
            }

            pActNum->SetLevel( i, aNumFmt );
        }
        nMask <<= 1;
    }

    SetModified();
    if (!m_xDistBorderMF->get_sensitive())
    {
        m_xDistBorderMF->set_text(u""_ustr);
    }
}

IMPL_LINK(SvxNumPositionTabPage, RelativeHdl_Impl, weld::Toggleable&, rBox, void)
{
    bool bOn = rBox.get_active();
    bool bSingleSelection = m_xLevelLB->count_selected_rows() == 1 && SAL_MAX_UINT16 != nActNumLvl;
    bool bSetValue = false;
    tools::Long nValue = 0;
    if(bOn || bSingleSelection)
    {
        sal_uInt16 nMask = 1;
        bool bFirst = true;
        bSetValue = true;
        for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
        {
            if(nActNumLvl & nMask)
            {
                const SvxNumberFormat &rNumFmt = pActNum->GetLevel(i);
                if(bFirst)
                {
                    nValue = rNumFmt.GetAbsLSpace() + rNumFmt.GetFirstLineOffset();
                    if(bOn && i)
                        nValue -= (pActNum->GetLevel(i - 1).GetAbsLSpace() + pActNum->GetLevel(i - 1).GetFirstLineOffset());
                }
                else
                    bSetValue = nValue ==
                        (rNumFmt.GetAbsLSpace() + rNumFmt.GetFirstLineOffset()) -
                            (pActNum->GetLevel(i - 1).GetAbsLSpace() + pActNum->GetLevel(i - 1).GetFirstLineOffset());
                bFirst = false;
            }
            nMask <<= 1;
        }

    }
    if(bSetValue)
        SetMetricValue(*m_xDistBorderMF, nValue,   eCoreUnit);
    else
        m_xDistBorderMF->set_text(u""_ustr);
    m_xDistBorderMF->set_sensitive(bOn || bSingleSelection);
    m_xDistBorderFT->set_sensitive(bOn || bSingleSelection);
    bLastRelative = bOn;
}

IMPL_LINK_NOARG(SvxNumPositionTabPage, LabelFollowedByHdl_Impl, weld::ComboBox&, void)
{
    // determine value to be set at the chosen list levels
    SvxNumberFormat::LabelFollowedBy eLabelFollowedBy = SvxNumberFormat::LISTTAB;
    {
        const auto nPos = m_xLabelFollowedByLB->get_active();
        if ( nPos == 1 )
        {
            eLabelFollowedBy = SvxNumberFormat::SPACE;
        }
        else if ( nPos == 2 )
        {
            eLabelFollowedBy = SvxNumberFormat::NOTHING;
        }
        else if ( nPos == 3 )
        {
            eLabelFollowedBy = SvxNumberFormat::NEWLINE;
        }
    }

    // set value at the chosen list levels
    bool bSameListtabPos = true;
    sal_uInt16 nFirstLvl = SAL_MAX_UINT16;
    sal_uInt16 nMask = 1;
    for( sal_uInt16 i = 0; i < pActNum->GetLevelCount(); ++i )
    {
        if ( nActNumLvl & nMask )
        {
            SvxNumberFormat aNumFmt( pActNum->GetLevel(i) );
            aNumFmt.SetLabelFollowedBy( eLabelFollowedBy );
            pActNum->SetLevel( i, aNumFmt );

            if ( nFirstLvl == SAL_MAX_UINT16 )
            {
                nFirstLvl = i;
            }
            else
            {
                bSameListtabPos &= aNumFmt.GetListtabPos() ==
                        pActNum->GetLevel( nFirstLvl ).GetListtabPos();
            }
        }
        nMask <<= 1;
    }

    // enable/disable metric field for list tab stop position depending on
    // selected item following the list label.
    m_xListtabFT->set_sensitive( eLabelFollowedBy == SvxNumberFormat::LISTTAB );
    m_xListtabMF->set_sensitive( eLabelFollowedBy == SvxNumberFormat::LISTTAB );
    if ( bSameListtabPos && eLabelFollowedBy == SvxNumberFormat::LISTTAB )
    {
        SetMetricValue(*m_xListtabMF, pActNum->GetLevel( nFirstLvl ).GetListtabPos(), eCoreUnit);
    }
    else
    {
        m_xListtabMF->set_text(OUString());
    }

    SetModified();
}

IMPL_LINK(SvxNumPositionTabPage, ListtabPosHdl_Impl, weld::MetricSpinButton&, rFld, void)
{
    // determine value to be set at the chosen list levels
    const tools::Long nValue = GetCoreValue(rFld, eCoreUnit);

    // set value at the chosen list levels
    sal_uInt16 nMask = 1;
    for( sal_uInt16 i = 0; i < pActNum->GetLevelCount(); ++i )
    {
        if ( nActNumLvl & nMask )
        {
            SvxNumberFormat aNumFmt( pActNum->GetLevel(i) );
            aNumFmt.SetListtabPos( nValue );
            pActNum->SetLevel( i, aNumFmt );
        }
        nMask <<= 1;
    }

    SetModified();
}

IMPL_LINK(SvxNumPositionTabPage, AlignAtHdl_Impl, weld::MetricSpinButton&, rFld, void)
{
    // determine value to be set at the chosen list levels
    const tools::Long nValue = GetCoreValue(rFld, eCoreUnit);

    // set value at the chosen list levels
    sal_uInt16 nMask = 1;
    for( sal_uInt16 i = 0; i < pActNum->GetLevelCount(); ++i )
    {
        if ( nActNumLvl & nMask )
        {
            SvxNumberFormat aNumFmt( pActNum->GetLevel(i) );
            const tools::Long nFirstLineIndent = nValue - aNumFmt.GetIndentAt();
            aNumFmt.SetFirstLineIndent( nFirstLineIndent );
            pActNum->SetLevel( i, aNumFmt );
        }
        nMask <<= 1;
    }

    SetModified();
}

IMPL_LINK(SvxNumPositionTabPage, IndentAtHdl_Impl, weld::MetricSpinButton&, rFld, void)
{
    // determine value to be set at the chosen list levels
    const tools::Long nValue = GetCoreValue(rFld, eCoreUnit);

    // set value at the chosen list levels
    sal_uInt16 nMask = 1;
    for( sal_uInt16 i = 0; i < pActNum->GetLevelCount(); ++i )
    {
        if ( nActNumLvl & nMask )
        {
            SvxNumberFormat aNumFmt( pActNum->GetLevel(i) );
            const tools::Long nAlignedAt = aNumFmt.GetIndentAt() +
                                    aNumFmt.GetFirstLineIndent();
            aNumFmt.SetIndentAt( nValue );
            const tools::Long nNewFirstLineIndent = nAlignedAt - nValue;
            aNumFmt.SetFirstLineIndent( nNewFirstLineIndent );
            pActNum->SetLevel( i, aNumFmt );
        }
        nMask <<= 1;
    }

    SetModified();
}

IMPL_LINK_NOARG(SvxNumPositionTabPage, StandardHdl_Impl, weld::Button&, void)
{
    sal_uInt16 nMask = 1;
    SvxNumRule aTmpNumRule( pActNum->GetFeatureFlags(),
                            pActNum->GetLevelCount(),
                            pActNum->IsContinuousNumbering(),
                            SvxNumRuleType::NUMBERING,
                            pActNum->GetLevel( 0 ).GetPositionAndSpaceMode() );
    for(sal_uInt16 i = 0; i < pActNum->GetLevelCount(); i++)
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aNumFmt( pActNum->GetLevel( i ) );
            const SvxNumberFormat& aTempFmt(aTmpNumRule.GetLevel( i ));
            aNumFmt.SetPositionAndSpaceMode( aTempFmt.GetPositionAndSpaceMode() );
            if ( aTempFmt.GetPositionAndSpaceMode() == SvxNumberFormat::LABEL_WIDTH_AND_POSITION )
            {
                aNumFmt.SetAbsLSpace( aTempFmt.GetAbsLSpace() );
                aNumFmt.SetCharTextDistance( aTempFmt.GetCharTextDistance() );
                aNumFmt.SetFirstLineOffset( aTempFmt.GetFirstLineOffset() );
            }
            else if ( aTempFmt.GetPositionAndSpaceMode() == SvxNumberFormat::LABEL_ALIGNMENT )
            {
                aNumFmt.SetNumAdjust( aTempFmt.GetNumAdjust() );
                aNumFmt.SetLabelFollowedBy( aTempFmt.GetLabelFollowedBy() );
                aNumFmt.SetListtabPos( aTempFmt.GetListtabPos() );
                aNumFmt.SetFirstLineIndent( aTempFmt.GetFirstLineIndent() );
                aNumFmt.SetIndentAt( aTempFmt.GetIndentAt() );
            }

            pActNum->SetLevel( i, aNumFmt );
        }
        nMask <<= 1;
    }

    InitControls();
    SetModified();
}

void SvxNumPositionTabPage::SetModified()
{
    bModified = true;
    m_aPreviewWIN.SetLevel(nActNumLvl);
    m_aPreviewWIN.Invalidate();
}

void SvxNumOptionsTabPage::SetModified(bool bRepaint)
{
    bModified = true;
    if (bRepaint)
    {
        m_aPreviewWIN.SetLevel(nActNumLvl);
        m_aPreviewWIN.Invalidate();
    }
}

void SvxNumOptionsTabPage::PageCreated(const SfxAllItemSet& aSet)
{
    const SfxStringListItem* pListItem = aSet.GetItem<SfxStringListItem>(SID_CHAR_FMT_LIST_BOX, false);
    const SfxStringItem* pNumCharFmt = aSet.GetItem<SfxStringItem>(SID_NUM_CHAR_FMT, false);
    const SfxStringItem* pBulletCharFmt = aSet.GetItem<SfxStringItem>(SID_BULLET_CHAR_FMT, false);
    const SfxUInt16Item* pMetricItem = aSet.GetItem<SfxUInt16Item>(SID_METRIC_ITEM, false);

    if (pNumCharFmt &&pBulletCharFmt)
        SetCharFmts( pNumCharFmt->GetValue(),pBulletCharFmt->GetValue());

    if (pListItem)
    {
        const std::vector<OUString> &aList = pListItem->GetList();
        for (const auto& rItem : aList)
            m_xCharFmtLB->append_text(rItem);
    }
    if (pMetricItem)
        SetMetric(static_cast<FieldUnit>(pMetricItem->GetValue()));
}

void SvxNumPositionTabPage::PageCreated(const SfxAllItemSet& aSet)
{
    const SfxUInt16Item* pMetricItem = aSet.GetItem<SfxUInt16Item>(SID_METRIC_ITEM, false);

    if (pMetricItem)
        SetMetric(static_cast<FieldUnit>(pMetricItem->GetValue()));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
