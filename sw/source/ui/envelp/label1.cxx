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
#include <vcl/svapp.hxx>
#include <rtl/ustring.hxx>
#include <tools/lineend.hxx>
#include <svtools/unitconv.hxx>
#include <com/sun/star/uno/Sequence.h>
#include <swtypes.hxx>
#include <labimp.hxx>
#include "swuilabimp.hxx"
#include "labfmt.hxx"
#include "labprt.hxx"
#include <dbmgr.hxx>
#include <uitool.hxx>
#include <cmdid.h>
#include <helpids.h>
#include <strings.hrc>
#include <envimg.hxx>

void SwLabRec::SetFromItem( const SwLabItem& rItem )
{
    m_nHDist  = rItem.m_lHDist;
    m_nVDist  = rItem.m_lVDist;
    m_nWidth  = rItem.m_lWidth;
    m_nHeight = rItem.m_lHeight;
    m_nLeft   = rItem.m_lLeft;
    m_nUpper  = rItem.m_lUpper;
    m_nCols   = rItem.m_nCols;
    m_nRows   = rItem.m_nRows;
    m_nPWidth  = rItem.m_lPWidth;
    m_nPHeight = rItem.m_lPHeight;
    m_bCont   = rItem.m_bCont;
}

void SwLabRec::FillItem( SwLabItem& rItem ) const
{
    rItem.m_lHDist  = m_nHDist;
    rItem.m_lVDist  = m_nVDist;
    rItem.m_lWidth  = m_nWidth;
    rItem.m_lHeight = m_nHeight;
    rItem.m_lLeft   = m_nLeft;
    rItem.m_lUpper  = m_nUpper;
    rItem.m_nCols   = m_nCols;
    rItem.m_lPWidth  = m_nPWidth;
    rItem.m_lPHeight = m_nPHeight;
    rItem.m_nRows   = m_nRows;
}

void SwLabDlg::ReplaceGroup_( const OUString &rMake )
{
    // Remove old entries
    m_pRecs->erase(m_pRecs->begin() + 1, m_pRecs->end());
    m_aLabelsCfg.FillLabels(rMake, *m_pRecs);
    m_aLstGroup = rMake;
}

void SwLabDlg::PageCreated(const OUString &rId, SfxTabPage &rPage)
{
    if (rId == "labels")
    {
        static_cast<SwLabPage*>(&rPage)->SetDBManager(m_pDBManager);
        static_cast<SwLabPage*>(&rPage)->InitDatabaseBox();
        if (!m_bLabel)
            static_cast<SwLabPage*>(&rPage)->SetToBusinessCard();
    }
    else if (rId == "options")
        m_pPrtPage = static_cast<SwLabPrtPage*>(&rPage);
}

SwLabDlg::SwLabDlg(weld::Window* pParent, const SfxItemSet& rSet,
                                SwDBManager* pDBManager_, bool bLabel)
    : SfxTabDialogController(pParent, u"modules/swriter/ui/labeldialog.ui"_ustr, u"LabelDialog"_ustr, &rSet)
    , m_pDBManager(pDBManager_)
    , m_pPrtPage(nullptr)
    , m_aTypeIds(50, 10)
    , m_pRecs(new SwLabRecs)
    , m_bLabel(bLabel)
{
    weld::WaitObject aWait(pParent);

    // Read user label from writer.cfg
    SwLabItem aItem(static_cast<const SwLabItem&>(rSet.Get( FN_LABEL )));
    std::unique_ptr<SwLabRec> pRec(new SwLabRec);
    pRec->m_aMake = pRec->m_aType = SwResId(STR_CUSTOM_LABEL);
    pRec->SetFromItem( aItem );

    bool bDouble = false;

    for (const std::unique_ptr<SwLabRec> & i : *m_pRecs)
    {
        if (pRec->m_aMake == i->m_aMake &&
            pRec->m_aType == i->m_aType)
        {
            bDouble = true;
            break;
        }
    }

    if (!bDouble)
        m_pRecs->insert( m_pRecs->begin(), std::move(pRec));

    size_t nLstGroup = 0;
    const std::vector<OUString>& rMan = m_aLabelsCfg.GetManufacturers();
    for(size_t nMan = 0; nMan < rMan.size(); ++nMan)
    {
        m_aMakes.push_back(rMan[nMan]);
        if ( rMan[nMan] == aItem.m_aLstMake )
            nLstGroup = nMan;
    }

    if ( !m_aMakes.empty() )
        ReplaceGroup_( m_aMakes[nLstGroup] );

    if (m_xExampleSet)
        m_xExampleSet->Put(aItem);

    AddTabPage(u"format"_ustr, SwLabFormatPage::Create, nullptr);
    AddTabPage(u"options"_ustr, SwLabPrtPage::Create, nullptr);
    AddTabPage(u"labels"_ustr, SwLabPage::Create, nullptr);
    m_sBusinessCardDlg = SwResId(STR_BUSINESS_CARDS);

    if (m_bLabel)
    {
        RemoveTabPage(u"business"_ustr);
        RemoveTabPage(u"private"_ustr);
    }
    else
    {
        AddTabPage(u"business"_ustr, SwBusinessDataPage::Create, nullptr );
        AddTabPage(u"private"_ustr, SwPrivateDataPage::Create, nullptr);
        m_xDialog->set_title(m_sBusinessCardDlg);
    }
}

SwLabDlg::~SwLabDlg()
{
    m_pRecs.reset();
}

void SwLabDlg::GetLabItem(SwLabItem &rItem)
{
    const SwLabItem& rActItem = static_cast<const SwLabItem&>(GetExampleSet()->Get(FN_LABEL));
    const SwLabItem& rOldItem = static_cast<const SwLabItem&>(GetInputSetImpl()->Get(FN_LABEL));

    if (rActItem != rOldItem)
    {
        // Was already "put" with (hopefully) correct content
        rItem = rActItem;
    }
    else
    {
        rItem = rOldItem;

        // In rItem there are only settings defined by users.
        // Therefore get the real settings directly from Record
        SwLabRec* pRec = GetRecord(rItem.m_aType, rItem.m_bCont);
        pRec->FillItem( rItem );
    }
}

SwLabRec* SwLabDlg::GetRecord(std::u16string_view rRecName, bool bCont)
{
    SwLabRec* pRec = nullptr;
    bool bFound = false;
    const OUString sCustom(SwResId(STR_CUSTOM_LABEL));

    const size_t nCount = Recs().size();
    for (size_t i = 0; i < nCount; ++i)
    {
        pRec = Recs()[i].get();
        if (pRec->m_aType != sCustom &&
            rRecName == pRec->m_aType && bCont == pRec->m_bCont)
        {
            bFound = true;
            break;
        }
    }
    if (!bFound)    // User defined
        pRec = Recs()[0].get();

    return pRec;
}

Printer *SwLabDlg::GetPrt()
{
    if (m_pPrtPage)
        return m_pPrtPage->GetPrt();
    else
        return nullptr;
}

SwLabPage::SwLabPage(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rSet)
    : SfxTabPage(pPage, pController, u"modules/swriter/ui/cardmediumpage.ui"_ustr, u"CardMediumPage"_ustr, &rSet)
    , m_pDBManager(nullptr)
    , m_aItem(static_cast<const SwLabItem&>(rSet.Get(FN_LABEL)))
    , m_xAddressFrame(m_xBuilder->weld_widget(u"addressframe"_ustr))
    , m_xAddrBox(m_xBuilder->weld_check_button(u"address"_ustr))
    , m_xWritingEdit(m_xBuilder->weld_text_view(u"textview"_ustr))
    , m_xDatabaseLB(m_xBuilder->weld_combo_box(u"database"_ustr))
    , m_xTableLB(m_xBuilder->weld_combo_box(u"table"_ustr))
    , m_xInsertBT(m_xBuilder->weld_button(u"insert"_ustr))
    , m_xDBFieldLB(m_xBuilder->weld_combo_box(u"field"_ustr))
    , m_xContButton(m_xBuilder->weld_radio_button(u"continuous"_ustr))
    , m_xSheetButton(m_xBuilder->weld_radio_button(u"sheet"_ustr))
    , m_xMakeBox(m_xBuilder->weld_combo_box(u"brand"_ustr))
    , m_xTypeBox(m_xBuilder->weld_combo_box(u"type"_ustr))
    , m_xHiddenSortTypeBox(m_xBuilder->weld_combo_box(u"hiddentype"_ustr))
    , m_xFormatInfo(m_xBuilder->weld_label(u"formatinfo"_ustr))
{
    weld::WaitObject aWait(GetFrameWeld());

    m_xWritingEdit->set_size_request(m_xWritingEdit->get_approximate_digit_width() * 30,
                                     m_xWritingEdit->get_height_rows(10));
    m_xHiddenSortTypeBox->make_sorted();

    tools::Long nListBoxWidth = m_xWritingEdit->get_approximate_digit_width() * 25;
    m_xTableLB->set_size_request(nListBoxWidth, -1);
    m_xDatabaseLB->set_size_request(nListBoxWidth, -1);
    m_xDBFieldLB->set_size_request(nListBoxWidth, -1);

    SetExchangeSupport();

    // Install handlers
    m_xAddrBox->connect_toggled(LINK(this, SwLabPage, AddrHdl));
    m_xDatabaseLB->connect_changed(LINK(this, SwLabPage, DatabaseHdl));
    m_xTableLB->connect_changed(LINK(this, SwLabPage, DatabaseHdl));
    m_xDBFieldLB->connect_changed(LINK(this, SwLabPage, DatabaseHdl));
    m_xInsertBT->connect_clicked(LINK(this, SwLabPage, FieldHdl));
    // Disable insert button first,
    // it'll be enabled if m_xDatabaseLB, m_pTableLB and m_pInsertBT are filled
    m_xInsertBT->set_sensitive(false);
    m_xContButton->connect_toggled(LINK(this, SwLabPage, PageHdl));
    m_xSheetButton->connect_toggled(LINK(this, SwLabPage, PageHdl));
    auto nMaxWidth = m_xMakeBox->get_approximate_digit_width() * 32;
    m_xMakeBox->set_size_request(nMaxWidth, -1);
    m_xTypeBox->set_size_request(nMaxWidth, -1);
    m_xMakeBox->connect_changed(LINK(this, SwLabPage, MakeHdl));
    m_xTypeBox->connect_changed(LINK(this, SwLabPage, TypeHdl));

    InitDatabaseBox();
}

SwLabPage::~SwLabPage()
{
}

void SwLabPage::SetToBusinessCard()
{
    m_xContainer->set_help_id(HID_BUSINESS_FMT_PAGE);
    m_xContButton->set_help_id(HID_BUSINESS_FMT_PAGE_CONT);
    m_xSheetButton->set_help_id(HID_BUSINESS_FMT_PAGE_SHEET);
    m_xMakeBox->set_help_id(HID_BUSINESS_FMT_PAGE_BRAND);
    m_xTypeBox->set_help_id(HID_BUSINESS_FMT_PAGE_TYPE);
};

IMPL_LINK_NOARG(SwLabPage, AddrHdl, weld::Toggleable&, void)
{
    OUString aWriting;

    if (m_xAddrBox->get_active())
        aWriting = convertLineEnd(MakeSender(), GetSystemLineEnd());

    m_xWritingEdit->set_text(aWriting);
    m_xWritingEdit->grab_focus();
}

IMPL_LINK( SwLabPage, DatabaseHdl, weld::ComboBox&, rListBox, void )
{
    m_sActDBName = m_xDatabaseLB->get_active_text();

    weld::WaitObject aObj(GetParentSwLabDlg()->getDialog());

    if (&rListBox == m_xDatabaseLB.get())
        GetDBManager()->GetTableNames(*m_xTableLB, m_sActDBName);

    if (&rListBox == m_xDatabaseLB.get() || &rListBox == m_xTableLB.get())
        GetDBManager()->GetColumnNames(*m_xDBFieldLB, m_sActDBName, m_xTableLB->get_active_text());

    if (!m_xDatabaseLB->get_active_text().isEmpty() && !m_xTableLB->get_active_text().isEmpty()
            && !m_xDBFieldLB->get_active_text().isEmpty())
        m_xInsertBT->set_sensitive(true);
    else
        m_xInsertBT->set_sensitive(false);
}

IMPL_LINK_NOARG(SwLabPage, FieldHdl, weld::Button&, void)
{
    OUString aStr("<" + m_xDatabaseLB->get_active_text() + "." +
                  m_xTableLB->get_active_text() + "." +
                  m_xTableLB->get_active_id() + "." +
                  m_xDBFieldLB->get_active_text() + ">");
    m_xWritingEdit->replace_selection(aStr);
    int nStartPos, nEndPos;
    m_xWritingEdit->get_selection_bounds(nStartPos, nEndPos);
    m_xWritingEdit->grab_focus();
    m_xWritingEdit->select_region(nStartPos, nEndPos);
}

IMPL_LINK_NOARG(SwLabPage, PageHdl, weld::Toggleable&, void)
{
    MakeHdl(*m_xMakeBox);
}

IMPL_LINK_NOARG(SwLabPage, MakeHdl, weld::ComboBox&, void)
{
    weld::WaitObject aWait(GetParentSwLabDlg()->getDialog());

    m_xTypeBox->clear();
    m_xHiddenSortTypeBox->clear();
    GetParentSwLabDlg()->TypeIds().clear();

    const OUString aMake = m_xMakeBox->get_active_text();
    GetParentSwLabDlg()->ReplaceGroup( aMake );
    m_aItem.m_aLstMake = aMake;

    const bool   bCont    = m_xContButton->get_active();
    const size_t nCount   = GetParentSwLabDlg()->Recs().size();
    size_t nLstType = 0;

    const OUString sCustom(SwResId(STR_CUSTOM_LABEL));
    //insert the entries into the sorted list box
    for ( size_t i = 0; i < nCount; ++i )
    {
        const OUString aType(GetParentSwLabDlg()->Recs()[i]->m_aType);
        bool bInsert = false;
        if (GetParentSwLabDlg()->Recs()[i]->m_aType == sCustom)
        {
            bInsert = true;
            m_xTypeBox->append_text(aType );
        }
        else if (GetParentSwLabDlg()->Recs()[i]->m_bCont == bCont)
        {
            if (m_xHiddenSortTypeBox->find_text(aType) == -1)
            {
                bInsert = true;
                m_xHiddenSortTypeBox->append_text( aType );
            }
        }
        if(bInsert)
        {
            GetParentSwLabDlg()->TypeIds().push_back(i);
            if ( !nLstType && aType == m_aItem.m_aLstType )
                nLstType = GetParentSwLabDlg()->TypeIds().size();
        }
    }
    for (int nEntry = 0; nEntry < m_xHiddenSortTypeBox->get_count(); ++nEntry)
    {
        m_xTypeBox->append_text(m_xHiddenSortTypeBox->get_text(nEntry));
    }
    if (nLstType)
        m_xTypeBox->set_active_text(m_aItem.m_aLstType);
    else
        m_xTypeBox->set_active(0);
    TypeHdl(*m_xTypeBox);
}

IMPL_LINK_NOARG(SwLabPage, TypeHdl, weld::ComboBox&, void)
{
    DisplayFormat();
    m_aItem.m_aType = m_xTypeBox->get_active_text();
}

void SwLabPage::DisplayFormat()
{
    std::unique_ptr<weld::Builder> xBuilder(Application::CreateBuilder(GetFrameWeld(), u"cui/ui/spinbox.ui"_ustr));
    std::unique_ptr<weld::Dialog> xTopLevel(xBuilder->weld_dialog(u"SpinDialog"_ustr));
    std::unique_ptr<weld::MetricSpinButton> xField(xBuilder->weld_metric_spin_button(u"spin"_ustr, FieldUnit::CM));
    SetFieldUnit(*xField, ::GetDfltMetric(false));
    xField->set_digits(2);
    xField->set_range(0, INT_MAX - 1, FieldUnit::NONE);

    SwLabRec* pRec = GetSelectedEntryPos();
    m_aItem.m_aLstType = pRec->m_aType;
    setfldval(*xField, pRec->m_nWidth);
    xField->reformat();
    const OUString aWString = xField->get_text();

    setfldval(*xField, pRec->m_nHeight);
    xField->reformat();

    OUString aText = pRec->m_aType + ": " + aWString +
           " x " + xField->get_text() +
           " (" + OUString::number( pRec->m_nCols ) +
           " x " + OUString::number( pRec->m_nRows ) + ")";
    m_xFormatInfo->set_label(aText);
}

SwLabRec* SwLabPage::GetSelectedEntryPos()
{
    OUString sSelEntry(m_xTypeBox->get_active_text());

    return GetParentSwLabDlg()->GetRecord(sSelEntry, m_xContButton->get_active());
}

void SwLabPage::InitDatabaseBox()
{
    if( !GetDBManager() )
        return;

    m_xDatabaseLB->clear();
    const css::uno::Sequence<OUString> aDataNames = SwDBManager::GetExistingDatabaseNames();
    for (const OUString& rDataName : aDataNames)
        m_xDatabaseLB->append_text(rDataName);
    sal_Int32 nIdx{ 0 };
    OUString sDBName = m_sActDBName.getToken( 0, DB_DELIM, nIdx );
    OUString sTableName = m_sActDBName.getToken( 0, DB_DELIM, nIdx );
    m_xDatabaseLB->set_active_text(sDBName);
    if( !sDBName.isEmpty() && GetDBManager()->GetTableNames(*m_xTableLB, sDBName))
    {
        m_xTableLB->set_active_text(sTableName);
        GetDBManager()->GetColumnNames(*m_xDBFieldLB, m_sActDBName, sTableName);
    }
    else
        m_xDBFieldLB->clear();
}

std::unique_ptr<SfxTabPage> SwLabPage::Create(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet* rSet)
{
    return std::make_unique<SwLabPage>(pPage, pController, *rSet);
}

void SwLabPage::ActivatePage(const SfxItemSet& rSet)
{
    Reset( &rSet );
}

DeactivateRC SwLabPage::DeactivatePage(SfxItemSet* _pSet)
{
    if (_pSet)
        FillItemSet(_pSet);

    return DeactivateRC::LeavePage;
}

void SwLabPage::FillItem(SwLabItem& rItem)
{
    rItem.m_bAddr    = m_xAddrBox->get_active();
    rItem.m_aWriting = m_xWritingEdit->get_text();
    rItem.m_bCont    = m_xContButton->get_active();
    rItem.m_aMake    = m_xMakeBox->get_active_text();
    rItem.m_aType    = m_xTypeBox->get_active_text();
    rItem.m_sDBName  = m_sActDBName;

    SwLabRec* pRec = GetSelectedEntryPos();
    pRec->FillItem( rItem );

    rItem.m_aLstMake = m_xMakeBox->get_active_text();
    rItem.m_aLstType = m_xTypeBox->get_active_text();
}

bool SwLabPage::FillItemSet(SfxItemSet* rSet)
{
    FillItem( m_aItem );
    rSet->Put( m_aItem );

    return true;
}

void SwLabPage::Reset(const SfxItemSet* rSet)
{
    m_xMakeBox->clear();

    size_t nLstGroup = 0;

    const size_t nCount = GetParentSwLabDlg()->Makes().size();
    for(size_t i = 0; i < nCount; ++i)
    {
        OUString& rStr = GetParentSwLabDlg()->Makes()[i];
        m_xMakeBox->append_text(rStr);

        if ( rStr == m_aItem.m_aLstMake)
            nLstGroup = i;
    }

    m_xMakeBox->set_active( nLstGroup );
    MakeHdl(*m_xMakeBox);

    m_aItem = static_cast<const SwLabItem&>( rSet->Get(FN_LABEL));
    OUString sDBName  = m_aItem.m_sDBName;

    OUString aWriting(convertLineEnd(m_aItem.m_aWriting, GetSystemLineEnd()));

    m_xAddrBox->set_active( m_aItem.m_bAddr );
    m_xWritingEdit->set_text( aWriting );

    for(const auto& rMake : GetParentSwLabDlg()->Makes())
    {
        if (m_xMakeBox->find_text(rMake) == -1)
            m_xMakeBox->append_text(rMake);
    }

    m_xMakeBox->set_active_text(m_aItem.m_aMake);
    //save the current type
    OUString sType(m_aItem.m_aType);
    MakeHdl(*m_xMakeBox);
    m_aItem.m_aType = sType;
    //#102806# a newly added make may not be in the type ListBox already
    if (m_xTypeBox->find_text(m_aItem.m_aType) == -1 && !m_aItem.m_aMake.isEmpty())
        GetParentSwLabDlg()->UpdateGroup( m_aItem.m_aMake );
    if (m_xTypeBox->find_text(m_aItem.m_aType) != -1)
    {
        m_xTypeBox->set_active_text(m_aItem.m_aType);
        TypeHdl(*m_xTypeBox);
    }
    if (m_xDatabaseLB->find_text(sDBName) != -1)
    {
        m_xDatabaseLB->set_active_text(sDBName);
        DatabaseHdl(*m_xDatabaseLB);
    }

    if (m_aItem.m_bCont)
        m_xContButton->set_active(true);
    else
        m_xSheetButton->set_active(true);
}

SwPrivateDataPage::SwPrivateDataPage(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rSet)
    : SfxTabPage(pPage, pController, u"modules/swriter/ui/privateuserpage.ui"_ustr, u"PrivateUserPage"_ustr, &rSet)
    , m_xFirstNameED(m_xBuilder->weld_entry(u"firstname"_ustr))
    , m_xNameED(m_xBuilder->weld_entry(u"lastname"_ustr))
    , m_xShortCutED(m_xBuilder->weld_entry(u"shortname"_ustr))
    , m_xFirstName2ED(m_xBuilder->weld_entry(u"firstname2"_ustr))
    , m_xName2ED(m_xBuilder->weld_entry(u"lastname2"_ustr))
    , m_xShortCut2ED(m_xBuilder->weld_entry(u"shortname2"_ustr))
    , m_xStreetED(m_xBuilder->weld_entry(u"street"_ustr))
    , m_xZipED(m_xBuilder->weld_entry(u"izip"_ustr))
    , m_xCityED(m_xBuilder->weld_entry(u"icity"_ustr))
    , m_xCountryED(m_xBuilder->weld_entry(u"country"_ustr))
    , m_xStateED(m_xBuilder->weld_entry(u"state"_ustr))
    , m_xTitleED(m_xBuilder->weld_entry(u"title"_ustr))
    , m_xProfessionED(m_xBuilder->weld_entry(u"job"_ustr))
    , m_xPhoneED(m_xBuilder->weld_entry(u"phone"_ustr))
    , m_xMobilePhoneED(m_xBuilder->weld_entry(u"mobile"_ustr))
    , m_xFaxED(m_xBuilder->weld_entry(u"fax"_ustr))
    , m_xHomePageED(m_xBuilder->weld_entry(u"url"_ustr))
    , m_xMailED(m_xBuilder->weld_entry(u"email"_ustr))
{
    SetExchangeSupport();
}

SwPrivateDataPage::~SwPrivateDataPage()
{
}

std::unique_ptr<SfxTabPage> SwPrivateDataPage::Create(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet* rSet)
{
    return std::make_unique<SwPrivateDataPage>(pPage, pController, *rSet);
}

void SwPrivateDataPage::ActivatePage(const SfxItemSet& rSet)
{
    Reset(&rSet);
}

DeactivateRC SwPrivateDataPage::DeactivatePage(SfxItemSet* _pSet)
{
    if (_pSet)
        FillItemSet(_pSet);
    return DeactivateRC::LeavePage;
}

bool SwPrivateDataPage::FillItemSet(SfxItemSet* rSet)
{
    const SfxItemSet* pExampleSet = GetDialogExampleSet();
    assert(pExampleSet);
    SwLabItem aItem = static_cast<const SwLabItem&>(pExampleSet->Get(FN_LABEL));

    aItem.m_aPrivFirstName = m_xFirstNameED->get_text();
    aItem.m_aPrivName      = m_xNameED->get_text();
    aItem.m_aPrivShortCut  = m_xShortCutED->get_text();
    aItem.m_aPrivFirstName2 = m_xFirstName2ED->get_text();
    aItem.m_aPrivName2     = m_xName2ED->get_text();
    aItem.m_aPrivShortCut2 = m_xShortCut2ED->get_text();
    aItem.m_aPrivStreet    = m_xStreetED->get_text();
    aItem.m_aPrivZip       = m_xZipED->get_text();
    aItem.m_aPrivCity      = m_xCityED->get_text();
    aItem.m_aPrivCountry   = m_xCountryED->get_text();
    aItem.m_aPrivState     = m_xStateED->get_text();
    aItem.m_aPrivTitle     = m_xTitleED->get_text();
    aItem.m_aPrivProfession= m_xProfessionED->get_text();
    aItem.m_aPrivPhone     = m_xPhoneED->get_text();
    aItem.m_aPrivMobile    = m_xMobilePhoneED->get_text();
    aItem.m_aPrivFax       = m_xFaxED->get_text();
    aItem.m_aPrivWWW       = m_xHomePageED->get_text();
    aItem.m_aPrivMail      = m_xMailED->get_text();

    rSet->Put(aItem);
    return true;
}

void SwPrivateDataPage::Reset(const SfxItemSet* rSet)
{
    const SwLabItem& aItem = static_cast<const SwLabItem&>( rSet->Get(FN_LABEL) );
    m_xFirstNameED->set_text(aItem.m_aPrivFirstName);
    m_xNameED->set_text(aItem.m_aPrivName);
    m_xShortCutED->set_text(aItem.m_aPrivShortCut);
    m_xFirstName2ED->set_text(aItem.m_aPrivFirstName2);
    m_xName2ED->set_text(aItem.m_aPrivName2);
    m_xShortCut2ED->set_text(aItem.m_aPrivShortCut2);
    m_xStreetED->set_text(aItem.m_aPrivStreet);
    m_xZipED->set_text(aItem.m_aPrivZip);
    m_xCityED->set_text(aItem.m_aPrivCity);
    m_xCountryED->set_text(aItem.m_aPrivCountry);
    m_xStateED->set_text(aItem.m_aPrivState);
    m_xTitleED->set_text(aItem.m_aPrivTitle);
    m_xProfessionED->set_text(aItem.m_aPrivProfession);
    m_xPhoneED->set_text(aItem.m_aPrivPhone);
    m_xMobilePhoneED->set_text(aItem.m_aPrivMobile);
    m_xFaxED->set_text(aItem.m_aPrivFax);
    m_xHomePageED->set_text(aItem.m_aPrivWWW);
    m_xMailED->set_text(aItem.m_aPrivMail);
}

SwBusinessDataPage::SwBusinessDataPage(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rSet)
    : SfxTabPage(pPage, pController, u"modules/swriter/ui/businessdatapage.ui"_ustr, u"BusinessDataPage"_ustr, &rSet)
    , m_xCompanyED(m_xBuilder->weld_entry(u"company"_ustr))
    , m_xCompanyExtED(m_xBuilder->weld_entry(u"company2"_ustr))
    , m_xSloganED(m_xBuilder->weld_entry(u"slogan"_ustr))
    , m_xStreetED(m_xBuilder->weld_entry(u"street"_ustr))
    , m_xZipED(m_xBuilder->weld_entry(u"izip"_ustr))
    , m_xCityED(m_xBuilder->weld_entry(u"icity"_ustr))
    , m_xCountryED(m_xBuilder->weld_entry(u"country"_ustr))
    , m_xStateED(m_xBuilder->weld_entry(u"state"_ustr))
    , m_xPositionED(m_xBuilder->weld_entry(u"position"_ustr))
    , m_xPhoneED(m_xBuilder->weld_entry(u"phone"_ustr))
    , m_xMobilePhoneED(m_xBuilder->weld_entry(u"mobile"_ustr))
    , m_xFaxED(m_xBuilder->weld_entry(u"fax"_ustr))
    , m_xHomePageED(m_xBuilder->weld_entry(u"url"_ustr))
    , m_xMailED(m_xBuilder->weld_entry(u"email"_ustr))
{
    SetExchangeSupport();
}

SwBusinessDataPage::~SwBusinessDataPage()
{
}

std::unique_ptr<SfxTabPage> SwBusinessDataPage::Create(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet* rSet)
{
    return std::make_unique<SwBusinessDataPage>(pPage, pController, *rSet);
}

void SwBusinessDataPage::ActivatePage(const SfxItemSet& rSet)
{
    Reset(&rSet);
}

DeactivateRC SwBusinessDataPage::DeactivatePage(SfxItemSet* _pSet)
{
    if (_pSet)
        FillItemSet(_pSet);
    return DeactivateRC::LeavePage;
}

bool SwBusinessDataPage::FillItemSet(SfxItemSet* rSet)
{
    const SfxItemSet* pExampleSet = GetDialogExampleSet();
    assert(pExampleSet);
    SwLabItem aItem = static_cast<const SwLabItem&>(pExampleSet->Get(FN_LABEL));

    aItem.m_aCompCompany   = m_xCompanyED->get_text();
    aItem.m_aCompCompanyExt= m_xCompanyExtED->get_text();
    aItem.m_aCompSlogan    = m_xSloganED->get_text();
    aItem.m_aCompStreet    = m_xStreetED->get_text();
    aItem.m_aCompZip       = m_xZipED->get_text();
    aItem.m_aCompCity      = m_xCityED->get_text();
    aItem.m_aCompCountry   = m_xCountryED->get_text();
    aItem.m_aCompState     = m_xStateED->get_text();
    aItem.m_aCompPosition  = m_xPositionED->get_text();
    aItem.m_aCompPhone     = m_xPhoneED->get_text();
    aItem.m_aCompMobile    = m_xMobilePhoneED->get_text();
    aItem.m_aCompFax       = m_xFaxED->get_text();
    aItem.m_aCompWWW       = m_xHomePageED->get_text();
    aItem.m_aCompMail      = m_xMailED->get_text();

    rSet->Put(aItem);
    return true;
}

void SwBusinessDataPage::Reset(const SfxItemSet* rSet)
{
    const SwLabItem& aItem = static_cast<const SwLabItem&>( rSet->Get(FN_LABEL) );
    m_xCompanyED->set_text(aItem.m_aCompCompany);
    m_xCompanyExtED->set_text(aItem.m_aCompCompanyExt);
    m_xSloganED->set_text(aItem.m_aCompSlogan);
    m_xStreetED->set_text(aItem.m_aCompStreet);
    m_xZipED->set_text(aItem.m_aCompZip);
    m_xCityED->set_text(aItem.m_aCompCity);
    m_xCountryED->set_text(aItem.m_aCompCountry);
    m_xStateED->set_text(aItem.m_aCompState);
    m_xPositionED->set_text(aItem.m_aCompPosition);
    m_xPhoneED->set_text(aItem.m_aCompPhone);
    m_xMobilePhoneED->set_text(aItem.m_aCompMobile);
    m_xFaxED->set_text(aItem.m_aCompFax);
    m_xHomePageED->set_text(aItem.m_aCompWWW);
    m_xMailED->set_text(aItem.m_aCompMail);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
