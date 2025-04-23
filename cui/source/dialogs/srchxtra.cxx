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

#include <srchxtra.hxx>
#include <sal/log.hxx>
#include <svl/cjkoptions.hxx>
#include <svl/intitem.hxx>
#include <svl/whiter.hxx>
#include <sfx2/objsh.hxx>
#include <svx/flagsdef.hxx>
#include <svx/strarray.hxx>
#include <editeng/flstitem.hxx>
#include <chardlg.hxx>
#include <paragrph.hxx>
#include <backgrnd.hxx>
#include <editeng/editids.hrc>
#include <svx/svxids.hrc>
#include <tools/debug.hxx>
#include <tools/resary.hxx>
#include <vcl/svapp.hxx>

SvxSearchFormatDialog::SvxSearchFormatDialog(weld::Window* pParent, const SfxItemSet& rSet)
    : SfxTabDialogController(pParent, u"cui/ui/searchformatdialog.ui"_ustr, u"SearchFormatDialog"_ustr, &rSet)
{
    AddTabPage(u"font"_ustr, SvxCharNamePage::Create, nullptr);
    AddTabPage(u"fonteffects"_ustr, SvxCharEffectsPage::Create, nullptr);
    AddTabPage(u"position"_ustr, SvxCharPositionPage::Create, nullptr);
    AddTabPage(u"asianlayout"_ustr, SvxCharTwoLinesPage::Create, nullptr);
    AddTabPage(u"labelTP_PARA_STD"_ustr, SvxStdParagraphTabPage::Create, nullptr);
    AddTabPage(u"labelTP_PARA_ALIGN"_ustr, SvxParaAlignTabPage::Create, nullptr);
    AddTabPage(u"labelTP_PARA_EXT"_ustr, SvxExtParagraphTabPage::Create, nullptr);
    AddTabPage(u"labelTP_PARA_ASIAN"_ustr, SvxAsianTabPage::Create, nullptr );
    AddTabPage(u"background"_ustr, SvxBkgTabPage::Create, nullptr);

    // remove asian tabpages if necessary
    if ( !SvtCJKOptions::IsDoubleLinesEnabled() )
        RemoveTabPage(u"asianlayout"_ustr);
    if ( !SvtCJKOptions::IsAsianTypographyEnabled() )
        RemoveTabPage(u"labelTP_PARA_ASIAN"_ustr);
}

SvxSearchFormatDialog::~SvxSearchFormatDialog()
{
}

void SvxSearchFormatDialog::PageCreated(const OUString& rId, SfxTabPage& rPage)
{
    if (rId == "font")
    {
        const FontList* pApm_pFontList = nullptr;
        if (SfxObjectShell* pSh = SfxObjectShell::Current())
        {
            const SvxFontListItem* pFLItem = static_cast<const SvxFontListItem*>(
                pSh->GetItem( SID_ATTR_CHAR_FONTLIST ));
            if ( pFLItem )
                pApm_pFontList = pFLItem->GetFontList();
        }

        const FontList* pList = pApm_pFontList;

        if ( !pList )
        {
            if ( !m_pFontList )
                m_pFontList.reset(new FontList(Application::GetDefaultDevice()));
            pList = m_pFontList.get();
        }

        static_cast<SvxCharNamePage&>(rPage).
                SetFontList( SvxFontListItem( pList, SID_ATTR_CHAR_FONTLIST ) );
        static_cast<SvxCharNamePage&>(rPage).EnableSearchMode();
    }
    else if (rId == "labelTP_PARA_STD")
    {
        static_cast<SvxStdParagraphTabPage&>(rPage).EnableAutoFirstLine();
    }
    else if (rId == "labelTP_PARA_ALIGN")
    {
        static_cast<SvxParaAlignTabPage&>(rPage).EnableJustifyExt();
    }
    else if (rId == "background")
    {
        SfxAllItemSet aSet(*(GetInputSetImpl()->GetPool()));
        aSet.Put(SfxUInt32Item(SID_FLAG_TYPE,static_cast<sal_uInt32>(SvxBackgroundTabFlags::SHOW_HIGHLIGHTING)));
        rPage.PageCreated(aSet);
    }
}

SvxSearchAttributeDialog::SvxSearchAttributeDialog(weld::Window* pParent,
    SearchAttrItemList& rLst, const WhichRangesContainer& pWhRanges)
    : GenericDialogController(pParent, u"cui/ui/searchattrdialog.ui"_ustr, u"SearchAttrDialog"_ustr)
    , rList(rLst)
    , m_xAttrLB(m_xBuilder->weld_tree_view(u"treeview"_ustr))
    , m_xOKBtn(m_xBuilder->weld_button(u"ok"_ustr))
{
    m_xAttrLB->set_size_request(m_xAttrLB->get_approximate_digit_width() * 50,
                                m_xAttrLB->get_height_rows(12));

    m_xAttrLB->enable_toggle_buttons(weld::ColumnToggleType::Check);

    m_xOKBtn->connect_clicked(LINK( this, SvxSearchAttributeDialog, OKHdl));

    SfxObjectShell* pSh = SfxObjectShell::Current();
    DBG_ASSERT( pSh, "No DocShell" );
    if (pSh)
    {
        SfxItemPool& rPool = pSh->GetPool();
        SfxItemSet aSet( rPool, pWhRanges );
        SfxWhichIter aIter( aSet );
        sal_uInt16 nWhich = aIter.FirstWhich();

        while ( nWhich )
        {
            sal_uInt16 nSlot = rPool.GetSlotId( nWhich );
            if ( nSlot >= SID_SVX_START )
            {
                bool bChecked = false, bFound = false;
                for ( sal_uInt16 i = 0; !bFound && i < rList.Count(); ++i )
                {
                    if ( nSlot == rList[i].nSlot )
                    {
                        bFound = true;
                        if ( IsInvalidItem( rList[i].aItemPtr.getItem() ) )
                            bChecked = true;
                    }
                }

                // item resources are in svx
                sal_uInt32 nId  = SvxAttrNameTable::FindIndex(nSlot);
                if (RESARRAY_INDEX_NOTFOUND != nId)
                {
                    m_xAttrLB->append();
                    const int nRow = m_xAttrLB->n_children() - 1;
                    m_xAttrLB->set_toggle(nRow, bChecked ? TRISTATE_TRUE : TRISTATE_FALSE);
                    m_xAttrLB->set_text(nRow, SvxAttrNameTable::GetString(nId), 0);
                    m_xAttrLB->set_id(nRow, OUString::number(nSlot));
                }
                else
                    SAL_WARN( "cui.dialogs", "no resource for slot id " << static_cast<sal_Int32>(nSlot) );
            }
            nWhich = aIter.NextWhich();
        }
    }

    m_xAttrLB->make_sorted();
    m_xAttrLB->select(0);
}

SvxSearchAttributeDialog::~SvxSearchAttributeDialog()
{
}

IMPL_LINK_NOARG(SvxSearchAttributeDialog, OKHdl, weld::Button&, void)
{
    SfxObjectShell* pObjSh = SfxObjectShell::Current();
    DBG_ASSERT( pObjSh, "No DocShell" );
    if (!pObjSh)
        return;
    SfxItemPool& rPool(pObjSh->GetPool());

    for (int i = 0, nCount = m_xAttrLB->n_children(); i < nCount; ++i)
    {
        const sal_uInt16 nSlot(m_xAttrLB->get_id(i).toUInt32());
        const bool bChecked(TRISTATE_TRUE == m_xAttrLB->get_toggle(i));

        sal_uInt16 j;
        for ( j = rList.Count(); j; )
        {
            SearchAttrInfo& rItem = rList[ --j ];
            if( rItem.nSlot == nSlot )
            {
                if( bChecked )
                    rItem.aItemPtr = SfxPoolItemHolder(rPool, INVALID_POOL_ITEM);
                else if( IsInvalidItem( rItem.aItemPtr.getItem() ) )
                    rItem.aItemPtr = SfxPoolItemHolder();
                j = 1;
                break;
            }
        }

        if ( !j && bChecked )
        {
            rList.Insert( { nSlot, SfxPoolItemHolder(rPool, INVALID_POOL_ITEM) });
        }
    }

    // remove invalid items (pItem == NULL)
    for ( sal_uInt16 n = rList.Count(); n; )
        if ( !rList[ --n ].aItemPtr.getItem() )
            rList.Remove( n );

    m_xDialog->response(RET_OK);
}

// class SvxSearchSimilarityDialog ---------------------------------------

SvxSearchSimilarityDialog::SvxSearchSimilarityDialog(weld::Window* pParent, bool bRelax,
    sal_uInt16 nOther, sal_uInt16 nShorter, sal_uInt16 nLonger)
    : GenericDialogController(pParent, u"cui/ui/similaritysearchdialog.ui"_ustr, u"SimilaritySearchDialog"_ustr)
    , m_xOtherFld(m_xBuilder->weld_spin_button(u"otherfld"_ustr))
    , m_xLongerFld(m_xBuilder->weld_spin_button(u"longerfld"_ustr))
    , m_xShorterFld(m_xBuilder->weld_spin_button(u"shorterfld"_ustr))
    , m_xRelaxBox(m_xBuilder->weld_check_button(u"relaxbox"_ustr))
{
    m_xOtherFld->set_value(nOther);
    m_xShorterFld->set_value(nShorter);
    m_xLongerFld->set_value(nLonger);
    m_xRelaxBox->set_active(bRelax);
}

SvxSearchSimilarityDialog::~SvxSearchSimilarityDialog()
{
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
