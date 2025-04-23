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

#undef SC_DLLIMPLEMENTATION

#include <officecfg/Office/Calc.hxx>
#include <tpview.hxx>
#include <global.hxx>
#include <viewopti.hxx>
#include <scresid.hxx>
#include <docsh.hxx>
#include <sc.hrc>
#include <strings.hrc>
#include <units.hrc>
#include <appoptio.hxx>
#include <scmod.hxx>
#include <svl/eitem.hxx>
#include <svtools/unitconv.hxx>
#include <unotools/localedatawrapper.hxx>

ScTpContentOptions::ScTpContentOptions(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet&  rArgSet)
    : SfxTabPage(pPage, pController, u"modules/scalc/ui/tpviewpage.ui"_ustr, u"TpViewPage"_ustr, &rArgSet)
    , m_xGridLB(m_xBuilder->weld_combo_box(u"grid"_ustr))
    , m_xGridImg(m_xBuilder->weld_widget(u"lockgrid"_ustr))
    , m_xBreakCB(m_xBuilder->weld_check_button(u"break"_ustr))
    , m_xBreakImg(m_xBuilder->weld_widget(u"lockbreak"_ustr))
    , m_xGuideLineCB(m_xBuilder->weld_check_button(u"guideline"_ustr))
    , m_xGuideLineImg(m_xBuilder->weld_widget(u"lockguideline"_ustr))
    , m_xFormulaCB(m_xBuilder->weld_check_button(u"formula"_ustr))
    , m_xFormulaImg(m_xBuilder->weld_widget(u"lockformula"_ustr))
    , m_xNilCB(m_xBuilder->weld_check_button(u"nil"_ustr))
    , m_xNilImg(m_xBuilder->weld_widget(u"locknil"_ustr))
    , m_xAnnotCB(m_xBuilder->weld_check_button(u"annot"_ustr))
    , m_xAnnotImg(m_xBuilder->weld_widget(u"lockannot"_ustr))
    , m_xNoteAuthorCB(m_xBuilder->weld_check_button(u"cbNoteAuthor"_ustr))
    , m_xNoteAuthorImg(m_xBuilder->weld_widget(u"imNoteAuthor"_ustr))
    , m_xFormulaMarkCB(m_xBuilder->weld_check_button(u"formulamark"_ustr))
    , m_xFormulaMarkImg(m_xBuilder->weld_widget(u"lockformulamark"_ustr))
    , m_xValueCB(m_xBuilder->weld_check_button(u"value"_ustr))
    , m_xValueImg(m_xBuilder->weld_widget(u"lockvalue"_ustr))
    , m_xColRowHighCB(m_xBuilder->weld_check_button(u"colrowhigh"_ustr))
    , m_xColRowHighImg(m_xBuilder->weld_widget(u"lockcolrowhigh"_ustr))
    , m_xEditCellBgHighCB(m_xBuilder->weld_check_button(u"editcellbg"_ustr))
    , m_xEditCellBgHighImg(m_xBuilder->weld_widget(u"lockeditcellbghigh"_ustr))
    , m_xAnchorCB(m_xBuilder->weld_check_button(u"anchor"_ustr))
    , m_xAnchorImg(m_xBuilder->weld_widget(u"lockanchor"_ustr))
    , m_xRangeFindCB(m_xBuilder->weld_check_button(u"rangefind"_ustr))
    , m_xRangeFindImg(m_xBuilder->weld_widget(u"lockrangefind"_ustr))
    , m_xObjGrfLB(m_xBuilder->weld_combo_box(u"objgrf"_ustr))
    , m_xObjGrfImg(m_xBuilder->weld_widget(u"lockobjgrf"_ustr))
    , m_xDiagramLB(m_xBuilder->weld_combo_box(u"diagram"_ustr))
    , m_xDiagramImg(m_xBuilder->weld_widget(u"lockdiagram"_ustr))
    , m_xDrawLB(m_xBuilder->weld_combo_box(u"draw"_ustr))
    , m_xDrawImg(m_xBuilder->weld_widget(u"lockdraw"_ustr))
    , m_xSyncZoomCB(m_xBuilder->weld_check_button(u"synczoom"_ustr))
    , m_xSyncZoomImg(m_xBuilder->weld_widget(u"locksynczoom"_ustr))
    , m_xRowColHeaderCB(m_xBuilder->weld_check_button(u"rowcolheader"_ustr))
    , m_xRowColHeaderImg(m_xBuilder->weld_widget(u"lockrowcolheader"_ustr))
    , m_xHScrollCB(m_xBuilder->weld_check_button(u"hscroll"_ustr))
    , m_xHScrollImg(m_xBuilder->weld_widget(u"lockhscroll"_ustr))
    , m_xVScrollCB(m_xBuilder->weld_check_button(u"vscroll"_ustr))
    , m_xVScrollImg(m_xBuilder->weld_widget(u"lockvscroll"_ustr))
    , m_xTblRegCB(m_xBuilder->weld_check_button(u"tblreg"_ustr))
    , m_xTblRegImg(m_xBuilder->weld_widget(u"locktblreg"_ustr))
    , m_xOutlineCB(m_xBuilder->weld_check_button(u"outline"_ustr))
    , m_xOutlineImg(m_xBuilder->weld_widget(u"lockoutline"_ustr))
    , m_xSummaryCB(m_xBuilder->weld_check_button(u"cbSummary"_ustr))
    , m_xSummaryImg(m_xBuilder->weld_widget(u"lockcbSummary"_ustr))
    , m_xThemedCursorRB(m_xBuilder->weld_radio_button(u"rbThemedCursor"_ustr))
    , m_xSystemCursorRB(m_xBuilder->weld_radio_button(u"rbSystemCursor"_ustr))
    , m_xCursorImg(m_xBuilder->weld_widget(u"lockCursor"_ustr))
{
    SetExchangeSupport();
    Link<weld::ComboBox&,void> aSelObjHdl(LINK( this, ScTpContentOptions, SelLbObjHdl ) );
    m_xObjGrfLB->connect_changed(aSelObjHdl);
    m_xDiagramLB->connect_changed(aSelObjHdl);
    m_xDrawLB->connect_changed(aSelObjHdl);
    m_xGridLB->connect_changed( LINK( this, ScTpContentOptions, GridHdl ) );

    Link<weld::Toggleable&, void> aCBHdl(LINK( this, ScTpContentOptions, CBHdl ) );
    m_xFormulaCB->connect_toggled(aCBHdl);
    m_xNilCB->connect_toggled(aCBHdl);
    m_xAnnotCB->connect_toggled(aCBHdl);
    m_xAnnotCB->set_accessible_description(ScResId(STR_A11Y_DESC_ANNOT));
    m_xNoteAuthorCB->connect_toggled(aCBHdl);
    m_xFormulaMarkCB->connect_toggled(aCBHdl);
    m_xValueCB->connect_toggled(aCBHdl);
    m_xColRowHighCB->connect_toggled(aCBHdl);
    m_xEditCellBgHighCB->connect_toggled(aCBHdl);
    m_xAnchorCB->connect_toggled(aCBHdl);

    m_xVScrollCB->connect_toggled(aCBHdl);
    m_xHScrollCB->connect_toggled(aCBHdl);
    m_xTblRegCB->connect_toggled(aCBHdl);
    m_xOutlineCB->connect_toggled(aCBHdl);
    m_xBreakCB->connect_toggled(aCBHdl);
    m_xGuideLineCB->connect_toggled(aCBHdl);
    m_xRowColHeaderCB->connect_toggled(aCBHdl);
    m_xSummaryCB->connect_toggled(aCBHdl);
    m_xThemedCursorRB->connect_toggled(aCBHdl);
}

ScTpContentOptions::~ScTpContentOptions()
{
}

std::unique_ptr<SfxTabPage> ScTpContentOptions::Create( weld::Container* pPage, weld::DialogController* pController,
                                               const SfxItemSet*     rCoreSet )
{
    return std::make_unique<ScTpContentOptions>(pPage, pController, *rCoreSet);
}

OUString ScTpContentOptions::GetAllStrings()
{
    OUString sAllStrings;
    OUString labels[] = { u"label4"_ustr,   u"label5"_ustr, u"label3"_ustr,       u"label1"_ustr,        u"grid_label"_ustr,
                          u"lbCursor"_ustr, u"label2"_ustr, u"objgrf_label"_ustr, u"diagram_label"_ustr, u"draw_label"_ustr };

    for (const auto& label : labels)
    {
        if (const auto pString = m_xBuilder->weld_label(label))
            sAllStrings += pString->get_label() + " ";
    }

    OUString checkButton[]
        = { u"formula"_ustr,   u"nil"_ustr,          u"annot"_ustr,   u"formulamark"_ustr, u"value"_ustr,  u"anchor"_ustr,
            u"rangefind"_ustr, u"rowcolheader"_ustr, u"hscroll"_ustr, u"vscroll"_ustr,     u"tblreg"_ustr, u"outline"_ustr,
            u"cbSummary"_ustr, u"synczoom"_ustr,     u"break"_ustr,   u"guideline"_ustr };

    for (const auto& check : checkButton)
    {
        if (const auto pString = m_xBuilder->weld_check_button(check))
            sAllStrings += pString->get_label() + " ";
    }

    return sAllStrings.replaceAll("_", "");
}

bool    ScTpContentOptions::FillItemSet( SfxItemSet* rCoreSet )
{
    bool bRet = false;
    if( m_xFormulaCB->get_state_changed_from_saved() ||
        m_xNilCB->get_state_changed_from_saved() ||
        m_xAnnotCB->get_state_changed_from_saved() ||
        m_xNoteAuthorCB->get_state_changed_from_saved() ||
        m_xFormulaMarkCB->get_state_changed_from_saved() ||
        m_xValueCB->get_state_changed_from_saved() ||
        m_xAnchorCB->get_state_changed_from_saved() ||
        m_xObjGrfLB->get_value_changed_from_saved() ||
        m_xDiagramLB->get_value_changed_from_saved() ||
        m_xDrawLB->get_value_changed_from_saved() ||
        m_xGridLB->get_value_changed_from_saved() ||
        m_xRowColHeaderCB->get_state_changed_from_saved() ||
        m_xHScrollCB->get_state_changed_from_saved() ||
        m_xVScrollCB->get_state_changed_from_saved() ||
        m_xTblRegCB->get_state_changed_from_saved() ||
        m_xOutlineCB->get_state_changed_from_saved() ||
        m_xBreakCB->get_state_changed_from_saved() ||
        m_xSummaryCB->get_state_changed_from_saved() ||
        m_xThemedCursorRB->get_state_changed_from_saved() ||
        m_xGuideLineCB->get_state_changed_from_saved())
    {
        rCoreSet->Put(ScTpViewItem(*m_xLocalOptions));
        bRet = true;
    }
    if(m_xRangeFindCB->get_state_changed_from_saved())
    {
        rCoreSet->Put(SfxBoolItem(SID_SC_INPUT_RANGEFINDER, m_xRangeFindCB->get_active()));
        bRet = true;
    }
    if(m_xSyncZoomCB->get_state_changed_from_saved())
    {
        rCoreSet->Put(SfxBoolItem(SID_SC_OPT_SYNCZOOM, m_xSyncZoomCB->get_active()));
        bRet = true;
    }
    if (m_xColRowHighCB->get_state_changed_from_saved())
    {
        auto pChange(comphelper::ConfigurationChanges::create());
        officecfg::Office::Calc::Content::Display::ColumnRowHighlighting::set(m_xColRowHighCB->get_active(), pChange);
        pChange->commit();
        bRet = true;
    }
    if (m_xEditCellBgHighCB->get_state_changed_from_saved())
    {
        auto pChange(comphelper::ConfigurationChanges::create());
        officecfg::Office::Calc::Content::Display::EditCellBackgroundHighlighting::set(m_xEditCellBgHighCB->get_active(), pChange);
        pChange->commit();
        bRet = true;
    }

    return bRet;
}

void    ScTpContentOptions::Reset( const SfxItemSet* rCoreSet )
{
    if(const ScTpViewItem* pViewItem = rCoreSet->GetItemIfSet(SID_SCVIEWOPTIONS, false))
        m_xLocalOptions.reset( new ScViewOptions( pViewItem->GetViewOptions() ) );
    else
        m_xLocalOptions.reset( new ScViewOptions );
    m_xFormulaCB ->set_active(m_xLocalOptions->GetOption(VOPT_FORMULAS));
    m_xNilCB     ->set_active(m_xLocalOptions->GetOption(VOPT_NULLVALS));
    m_xAnnotCB   ->set_active(m_xLocalOptions->GetOption(VOPT_NOTES));
    m_xNoteAuthorCB->set_active(m_xLocalOptions->GetOption(VOPT_NOTEAUTHOR));
    m_xFormulaMarkCB->set_active(m_xLocalOptions->GetOption(VOPT_FORMULAS_MARKS));
    m_xValueCB   ->set_active(m_xLocalOptions->GetOption(VOPT_SYNTAX));
    m_xColRowHighCB->set_active(officecfg::Office::Calc::Content::Display::ColumnRowHighlighting::get());
    m_xEditCellBgHighCB->set_active(officecfg::Office::Calc::Content::Display::EditCellBackgroundHighlighting::get());
    m_xAnchorCB  ->set_active(m_xLocalOptions->GetOption(VOPT_ANCHOR));

    m_xObjGrfLB  ->set_active( static_cast<sal_uInt16>(m_xLocalOptions->GetObjMode(VOBJ_TYPE_OLE)) );
    m_xDiagramLB ->set_active( static_cast<sal_uInt16>(m_xLocalOptions->GetObjMode(VOBJ_TYPE_CHART)) );
    m_xDrawLB    ->set_active( static_cast<sal_uInt16>(m_xLocalOptions->GetObjMode(VOBJ_TYPE_DRAW)) );

    m_xRowColHeaderCB->set_active( m_xLocalOptions->GetOption(VOPT_HEADER) );
    m_xHScrollCB->set_active( m_xLocalOptions->GetOption(VOPT_HSCROLL) );
    m_xVScrollCB->set_active( m_xLocalOptions->GetOption(VOPT_VSCROLL) );
    m_xTblRegCB ->set_active( m_xLocalOptions->GetOption(VOPT_TABCONTROLS) );
    m_xOutlineCB->set_active( m_xLocalOptions->GetOption(VOPT_OUTLINER) );
    m_xSummaryCB->set_active( m_xLocalOptions->GetOption(VOPT_SUMMARY) );
    if ( m_xLocalOptions->GetOption(VOPT_THEMEDCURSOR) )
        m_xThemedCursorRB->set_active( true );
    else
        m_xSystemCursorRB->set_active( true );

    InitGridOpt();

    m_xBreakCB->set_active( m_xLocalOptions->GetOption(VOPT_PAGEBREAKS) );
    m_xGuideLineCB->set_active( m_xLocalOptions->GetOption(VOPT_HELPLINES) );

    if(const SfxBoolItem* pFinderItem = rCoreSet->GetItemIfSet(SID_SC_INPUT_RANGEFINDER, false))
        m_xRangeFindCB->set_active(pFinderItem->GetValue());
    if(const SfxBoolItem* pZoomItem = rCoreSet->GetItemIfSet(SID_SC_OPT_SYNCZOOM, false))
        m_xSyncZoomCB->set_active(pZoomItem->GetValue());

    bool bReadOnly = officecfg::Office::Calc::Layout::Line::GridLine::isReadOnly() ||
        officecfg::Office::Calc::Layout::Line::GridOnColoredCells::isReadOnly();
    m_xGridLB->set_sensitive(!bReadOnly);
    m_xGridImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Layout::Line::PageBreak::isReadOnly();
    m_xBreakCB->set_sensitive(!bReadOnly);
    m_xBreakImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Layout::Line::Guide::isReadOnly();
    m_xGuideLineCB->set_sensitive(!bReadOnly);
    m_xGuideLineImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Content::Display::Formula::isReadOnly();
    m_xFormulaCB->set_sensitive(!bReadOnly);
    m_xFormulaImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Content::Display::ZeroValue::isReadOnly();
    m_xNilCB->set_sensitive(!bReadOnly);
    m_xNilImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Content::Display::NoteTag::isReadOnly();
    m_xAnnotCB->set_sensitive(!bReadOnly);
    m_xAnnotImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Content::Display::NoteAuthor::isReadOnly();
    m_xNoteAuthorCB->set_sensitive(!bReadOnly);
    m_xNoteAuthorImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Content::Display::FormulaMark::isReadOnly();
    m_xFormulaMarkCB->set_sensitive(!bReadOnly);
    m_xFormulaMarkImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Content::Display::ValueHighlighting::isReadOnly();
    m_xValueCB->set_sensitive(!bReadOnly);
    m_xValueImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Content::Display::ColumnRowHighlighting::isReadOnly();
    m_xColRowHighCB->set_sensitive(!bReadOnly);
    m_xColRowHighImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Content::Display::EditCellBackgroundHighlighting::isReadOnly();
    m_xEditCellBgHighCB->set_sensitive(!bReadOnly);
    m_xEditCellBgHighImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Content::Display::Anchor::isReadOnly();
    m_xAnchorCB->set_sensitive(!bReadOnly);
    m_xAnchorImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Input::ShowReference::isReadOnly();
    m_xRangeFindCB->set_sensitive(!bReadOnly);
    m_xRangeFindImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Content::Display::ObjectGraphic::isReadOnly();
    m_xObjGrfLB->set_sensitive(!bReadOnly);
    m_xObjGrfImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Content::Display::Chart::isReadOnly();
    m_xDiagramLB->set_sensitive(!bReadOnly);
    m_xDiagramImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Content::Display::DrawingObject::isReadOnly();
    m_xDrawLB->set_sensitive(!bReadOnly);
    m_xDrawImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Layout::Zoom::Synchronize::isReadOnly();
    m_xSyncZoomCB->set_sensitive(!bReadOnly);
    m_xSyncZoomImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Layout::Window::ColumnRowHeader::isReadOnly();
    m_xRowColHeaderCB->set_sensitive(!bReadOnly);
    m_xRowColHeaderImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Layout::Window::HorizontalScroll::isReadOnly();
    m_xHScrollCB->set_sensitive(!bReadOnly);
    m_xHScrollImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Layout::Window::VerticalScroll::isReadOnly();
    m_xVScrollCB->set_sensitive(!bReadOnly);
    m_xVScrollImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Layout::Window::SheetTab::isReadOnly();
    m_xTblRegCB->set_sensitive(!bReadOnly);
    m_xTblRegImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Layout::Window::OutlineSymbol::isReadOnly();
    m_xOutlineCB->set_sensitive(!bReadOnly);
    m_xOutlineImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Layout::Window::SearchSummary::isReadOnly();
    m_xSummaryCB->set_sensitive(!bReadOnly);
    m_xSummaryImg->set_visible(bReadOnly);

    bReadOnly = officecfg::Office::Calc::Layout::Window::ThemedCursor::isReadOnly();
    m_xThemedCursorRB->set_sensitive(!bReadOnly);
    m_xSystemCursorRB->set_sensitive(!bReadOnly);
    m_xCursorImg->set_visible(bReadOnly);

    m_xRangeFindCB->save_state();
    m_xSyncZoomCB->save_state();

    m_xFormulaCB->save_state();
    m_xNilCB->save_state();
    m_xAnnotCB->save_state();
    m_xNoteAuthorCB->save_state();
    m_xFormulaMarkCB->save_state();
    m_xValueCB->save_state();
    m_xColRowHighCB->save_state();
    m_xEditCellBgHighCB->save_state();
    m_xAnchorCB->save_state();
    m_xObjGrfLB->save_value();
    m_xDiagramLB->save_value();
    m_xDrawLB->save_value();
    m_xRowColHeaderCB->save_state();
    m_xHScrollCB->save_state();
    m_xVScrollCB->save_state();
    m_xTblRegCB->save_state();
    m_xOutlineCB->save_state();
    m_xGridLB->save_value();
    m_xBreakCB->save_state();
    m_xGuideLineCB->save_state();
    m_xSummaryCB->save_state();
    m_xThemedCursorRB->save_state();
}

void ScTpContentOptions::ActivatePage( const SfxItemSet& rSet)
{
    if(const ScTpViewItem* pViewItem = rSet.GetItemIfSet(SID_SCVIEWOPTIONS, false))
        *m_xLocalOptions = pViewItem->GetViewOptions();
}

DeactivateRC ScTpContentOptions::DeactivatePage( SfxItemSet* pSetP )
{
    if(pSetP)
        FillItemSet(pSetP);
    return DeactivateRC::LeavePage;
}

IMPL_LINK( ScTpContentOptions, SelLbObjHdl, weld::ComboBox&, rLb, void )
{
    const sal_Int32 nSelPos = rLb.get_active();
    ScVObjMode  eMode   = ScVObjMode(nSelPos);
    ScVObjType  eType   = VOBJ_TYPE_OLE;

    if ( &rLb == m_xDiagramLB.get() )
        eType = VOBJ_TYPE_CHART;
    else if ( &rLb == m_xDrawLB.get() )
        eType = VOBJ_TYPE_DRAW;

    m_xLocalOptions->SetObjMode( eType, eMode );
}

IMPL_LINK( ScTpContentOptions, CBHdl, weld::Toggleable&, rBtn, void )
{
    ScViewOption eOption = VOPT_FORMULAS;
    bool         bChecked = rBtn.get_active();

    if (m_xFormulaCB.get() == &rBtn )   eOption = VOPT_FORMULAS;
    else if ( m_xNilCB.get() == &rBtn )   eOption = VOPT_NULLVALS;
    else if ( m_xAnnotCB.get() == &rBtn )   eOption = VOPT_NOTES;
    else if ( m_xNoteAuthorCB.get() == &rBtn )   eOption = VOPT_NOTEAUTHOR;
    else if ( m_xFormulaMarkCB.get() == &rBtn )   eOption = VOPT_FORMULAS_MARKS;
    else if ( m_xValueCB.get() == &rBtn )   eOption = VOPT_SYNTAX;
    else if ( m_xAnchorCB.get() == &rBtn )   eOption = VOPT_ANCHOR;
    else if ( m_xVScrollCB.get()  == &rBtn )   eOption = VOPT_VSCROLL;
    else if ( m_xHScrollCB.get() == &rBtn )   eOption = VOPT_HSCROLL;
    else if ( m_xTblRegCB.get() == &rBtn )   eOption = VOPT_TABCONTROLS;
    else if ( m_xOutlineCB.get() == &rBtn )   eOption = VOPT_OUTLINER;
    else if ( m_xBreakCB.get() == &rBtn )   eOption = VOPT_PAGEBREAKS;
    else if ( m_xGuideLineCB.get() == &rBtn )   eOption = VOPT_HELPLINES;
    else if ( m_xRowColHeaderCB.get() == &rBtn )   eOption = VOPT_HEADER;
    else if ( m_xSummaryCB.get()  == &rBtn )   eOption = VOPT_SUMMARY;
    else if ( m_xThemedCursorRB.get() == &rBtn )   eOption = VOPT_THEMEDCURSOR;

    m_xLocalOptions->SetOption( eOption, bChecked );
}

void ScTpContentOptions::InitGridOpt()
{
    bool    bGrid = m_xLocalOptions->GetOption( VOPT_GRID );
    bool    bGridOnTop = m_xLocalOptions->GetOption( VOPT_GRID_ONTOP );
    sal_Int32   nSelPos = 0;

    if ( bGrid || bGridOnTop )
    {
        if ( !bGridOnTop )
            nSelPos = 0;
        else
            nSelPos = 1;
    }
    else
        nSelPos = 2;

    m_xGridLB->set_active (nSelPos);
}

IMPL_LINK( ScTpContentOptions, GridHdl, weld::ComboBox&, rLb, void )
{
    sal_Int32   nSelPos = rLb.get_active();
    bool    bGrid = ( nSelPos <= 1 );
    bool    bGridOnTop = ( nSelPos == 1 );

    m_xLocalOptions->SetOption( VOPT_GRID, bGrid );
    m_xLocalOptions->SetOption( VOPT_GRID_ONTOP, bGridOnTop );
}

ScTpLayoutOptions::ScTpLayoutOptions(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rArgSet)
    : SfxTabPage(pPage, pController, u"modules/scalc/ui/scgeneralpage.ui"_ustr, u"ScGeneralPage"_ustr, &rArgSet)
    , pDoc(nullptr)
    , m_xUnitLB(m_xBuilder->weld_combo_box(u"unitlb"_ustr))
    , m_xUnitImg(m_xBuilder->weld_widget(u"lockunitlb"_ustr))
    , m_xTabMF(m_xBuilder->weld_metric_spin_button(u"tabmf"_ustr, FieldUnit::CM))
    , m_xTabImg(m_xBuilder->weld_widget(u"locktabmf"_ustr))
    , m_xAlwaysRB(m_xBuilder->weld_radio_button(u"alwaysrb"_ustr))
    , m_xRequestRB(m_xBuilder->weld_radio_button(u"requestrb"_ustr))
    , m_xNeverRB(m_xBuilder->weld_radio_button(u"neverrb"_ustr))
    , m_xUpdateLinksImg(m_xBuilder->weld_widget(u"lockupdatelinks"_ustr))
    , m_xAlignCB(m_xBuilder->weld_check_button(u"aligncb"_ustr))
    , m_xAlignImg(m_xBuilder->weld_widget(u"lockaligncb"_ustr))
    , m_xAlignLB(m_xBuilder->weld_combo_box(u"alignlb"_ustr))
    , m_xEditModeCB(m_xBuilder->weld_check_button(u"editmodecb"_ustr))
    , m_xEditModeImg(m_xBuilder->weld_widget(u"lockeditmodecb"_ustr))
    , m_xFormatCB(m_xBuilder->weld_check_button(u"formatcb"_ustr))
    , m_xFormatImg(m_xBuilder->weld_widget(u"lockformatcb"_ustr))
    , m_xExpRefCB(m_xBuilder->weld_check_button(u"exprefcb"_ustr))
    , m_xExpRefImg(m_xBuilder->weld_widget(u"lockexprefcb"_ustr))
    , m_xSortRefUpdateCB(m_xBuilder->weld_check_button(u"sortrefupdatecb"_ustr))
    , m_xSortRefUpdateImg(m_xBuilder->weld_widget(u"locksortrefupdatecb"_ustr))
    , m_xMarkHdrCB(m_xBuilder->weld_check_button(u"markhdrcb"_ustr))
    , m_xMarkHdrImg(m_xBuilder->weld_widget(u"lockmarkhdrcb"_ustr))
    , m_xReplWarnCB(m_xBuilder->weld_check_button(u"replwarncb"_ustr))
    , m_xReplWarnImg(m_xBuilder->weld_widget(u"lockreplwarncb"_ustr))
    , m_xLegacyCellSelectionCB(m_xBuilder->weld_check_button(u"legacy_cell_selection_cb"_ustr))
    , m_xLegacyCellSelectionImg(m_xBuilder->weld_widget(u"locklegacy_cell"_ustr))
    , m_xEnterPasteModeCB(m_xBuilder->weld_check_button(u"enter_paste_mode_cb"_ustr))
    , m_xEnterPasteModeImg(m_xBuilder->weld_widget(u"lockenter_paste"_ustr))
    , m_xWarnActiveSheetCB(m_xBuilder->weld_check_button(u"warnactivesheet_cb"_ustr))
    , m_xWarnActiveSheetImg(m_xBuilder->weld_widget(u"lockwarnactivesheet"_ustr))
{
    SetExchangeSupport();

    m_xUnitLB->connect_changed( LINK( this, ScTpLayoutOptions, MetricHdl ) );
    m_xAlignCB->connect_toggled(LINK(this, ScTpLayoutOptions, AlignHdl));

    for (size_t i = 0; i < SAL_N_ELEMENTS(SCSTR_UNIT); ++i)
    {
        OUString sMetric = ScResId(SCSTR_UNIT[i].first);
        FieldUnit eFUnit = SCSTR_UNIT[i].second;

        switch ( eFUnit )
        {
            case FieldUnit::MM:
            case FieldUnit::CM:
            case FieldUnit::POINT:
            case FieldUnit::PICA:
            case FieldUnit::INCH:
            {
                // only use these metrics
                m_xUnitLB->append(OUString::number(static_cast<sal_uInt32>(eFUnit)), sMetric);
            }
            break;
            default:
            {
                // added to avoid warnings
            }
        }
    }
}

ScTpLayoutOptions::~ScTpLayoutOptions()
{
}

std::unique_ptr<SfxTabPage> ScTpLayoutOptions::Create( weld::Container* pPage, weld::DialogController* pController,
                                              const SfxItemSet*   rCoreSet )
{
    auto xNew = std::make_unique<ScTpLayoutOptions>(pPage, pController, *rCoreSet);

    ScDocShell* pDocSh = dynamic_cast< ScDocShell *>( SfxObjectShell::Current() );
    if (pDocSh!=nullptr)
        xNew->pDoc = &pDocSh->GetDocument();
    return xNew;
}

OUString ScTpLayoutOptions::GetAllStrings()
{
    OUString sAllStrings;
    OUString labels[] = { u"label1"_ustr, u"label4"_ustr, u"label5"_ustr, u"label6"_ustr, u"label3"_ustr };

    for (const auto& label : labels)
    {
        if (const auto pString = m_xBuilder->weld_label(label))
            sAllStrings += pString->get_label() + " ";
    }

    OUString checkButton[] = { u"aligncb"_ustr,   u"editmodecb"_ustr, u"enter_paste_mode_cb"_ustr,
                               u"formatcb"_ustr,  u"exprefcb"_ustr,   u"sortrefupdatecb"_ustr,
                               u"markhdrcb"_ustr, u"replwarncb"_ustr, u"legacy_cell_selection_cb"_ustr,
                               u"warnactivesheet_cb"_ustr };

    for (const auto& check : checkButton)
    {
        if (const auto pString = m_xBuilder->weld_check_button(check))
            sAllStrings += pString->get_label() + " ";
    }

    OUString radioButton[] = { u"alwaysrb"_ustr, u"requestrb"_ustr, u"neverrb"_ustr };

    for (const auto& radio : radioButton)
    {
        if (const auto pString = m_xBuilder->weld_radio_button(radio))
            sAllStrings += pString->get_label() + " ";
    }

    return sAllStrings.replaceAll("_", "");
}

bool    ScTpLayoutOptions::FillItemSet( SfxItemSet* rCoreSet )
{
    bool bRet = true;
    if (m_xUnitLB->get_value_changed_from_saved())
    {
        const sal_Int32 nMPos = m_xUnitLB->get_active();
        sal_uInt16 nFieldUnit = m_xUnitLB->get_id(nMPos).toUInt32();
        rCoreSet->Put( SfxUInt16Item( SID_ATTR_METRIC, nFieldUnit ) );
        bRet = true;
    }

    if (m_xTabMF->get_value_changed_from_saved())
    {
        rCoreSet->Put(SfxUInt16Item(SID_ATTR_DEFTABSTOP,
                    sal::static_int_cast<sal_uInt16>( m_xTabMF->denormalize(m_xTabMF->get_value(FieldUnit::TWIP)) )));
        bRet = true;
    }

    ScLkUpdMode nSet=LM_ALWAYS;

    if (m_xRequestRB->get_active())
    {
        nSet=LM_ON_DEMAND;
    }
    else if (m_xNeverRB->get_active())
    {
        nSet=LM_NEVER;
    }

    if (m_xRequestRB->get_state_changed_from_saved() ||
        m_xNeverRB->get_state_changed_from_saved() )
    {
        if(pDoc)
            pDoc->SetLinkMode(nSet);
        ScModule* mod = ScModule::get();
        ScAppOptions aAppOptions = mod->GetAppOptions();
        aAppOptions.SetLinkMode(nSet );
        mod->SetAppOptions(aAppOptions);
        bRet = true;
    }
    if (m_xAlignCB->get_state_changed_from_saved())
    {
        rCoreSet->Put(SfxBoolItem(SID_SC_INPUT_SELECTION, m_xAlignCB->get_active()));
        bRet = true;
    }

    if (m_xAlignLB->get_value_changed_from_saved())
    {
        rCoreSet->Put(SfxUInt16Item(SID_SC_INPUT_SELECTIONPOS, m_xAlignLB->get_active()));
        bRet = true;
    }

    if (m_xEditModeCB->get_state_changed_from_saved())
    {
        rCoreSet->Put(SfxBoolItem(SID_SC_INPUT_EDITMODE, m_xEditModeCB->get_active()));
        bRet = true;
    }

    if (m_xFormatCB->get_state_changed_from_saved())
    {
        rCoreSet->Put(SfxBoolItem(SID_SC_INPUT_FMT_EXPAND, m_xFormatCB->get_active()));
        bRet = true;
    }

    if (m_xExpRefCB->get_state_changed_from_saved())
    {
        rCoreSet->Put(SfxBoolItem(SID_SC_INPUT_REF_EXPAND, m_xExpRefCB->get_active()));
        bRet = true;
    }

    if (m_xSortRefUpdateCB->get_state_changed_from_saved())
    {
        rCoreSet->Put(SfxBoolItem(SID_SC_OPT_SORT_REF_UPDATE, m_xSortRefUpdateCB->get_active()));
        bRet = true;
    }

    if (m_xMarkHdrCB->get_state_changed_from_saved())
    {
        rCoreSet->Put(SfxBoolItem(SID_SC_INPUT_MARK_HEADER, m_xMarkHdrCB->get_active()));
        bRet = true;
    }

    if (m_xReplWarnCB->get_state_changed_from_saved())
    {
        rCoreSet->Put( SfxBoolItem( SID_SC_INPUT_REPLCELLSWARN, m_xReplWarnCB->get_active() ) );
        bRet = true;
    }

    if (m_xLegacyCellSelectionCB->get_state_changed_from_saved())
    {
        rCoreSet->Put( SfxBoolItem( SID_SC_INPUT_LEGACY_CELL_SELECTION, m_xLegacyCellSelectionCB->get_active() ) );
        bRet = true;
    }

    if (m_xEnterPasteModeCB->get_state_changed_from_saved())
    {
        rCoreSet->Put( SfxBoolItem( SID_SC_INPUT_ENTER_PASTE_MODE, m_xEnterPasteModeCB->get_active() ) );
        bRet = true;
    }

    if (m_xWarnActiveSheetCB->get_state_changed_from_saved())
    {
        rCoreSet->Put( SfxBoolItem( SID_SC_INPUT_WARNACTIVESHEET, m_xWarnActiveSheetCB->get_active() ) );
        bRet = true;
    }

    return bRet;
}

void    ScTpLayoutOptions::Reset( const SfxItemSet* rCoreSet )
{
    m_xUnitLB->set_active(-1);
    if ( rCoreSet->GetItemState( SID_ATTR_METRIC ) >= SfxItemState::DEFAULT )
    {
        const SfxUInt16Item& rItem = rCoreSet->Get( SID_ATTR_METRIC );
        FieldUnit eFieldUnit = static_cast<FieldUnit>(rItem.GetValue());

        for (sal_Int32 i = 0, nEntryCount = m_xUnitLB->get_count(); i < nEntryCount; ++i)
        {
            if (m_xUnitLB->get_id(i).toUInt32() == static_cast<sal_uInt32>(eFieldUnit))
            {
                m_xUnitLB->set_active(i);
                break;
            }
        }
        ::SetFieldUnit(*m_xTabMF, eFieldUnit);
    }

    bool bReadOnly = false;
    MeasurementSystem eSys = ScGlobal::getLocaleData().getMeasurementSystemEnum();
    if (eSys == MeasurementSystem::Metric)
    {
        bReadOnly = officecfg::Office::Calc::Layout::Other::MeasureUnit::Metric::isReadOnly();
    }
    else
    {
        bReadOnly = officecfg::Office::Calc::Layout::Other::MeasureUnit::NonMetric::isReadOnly();
    }
    m_xUnitLB->set_sensitive(!bReadOnly);
    m_xUnitImg->set_visible(bReadOnly);

    if(const SfxUInt16Item* pTabStopItem = rCoreSet->GetItemIfSet(SID_ATTR_DEFTABSTOP, false))
        m_xTabMF->set_value(m_xTabMF->normalize(pTabStopItem->GetValue()), FieldUnit::TWIP);

    if (eSys == MeasurementSystem::Metric)
    {
        bReadOnly = officecfg::Office::Calc::Layout::Other::TabStop::Metric::isReadOnly();
    }
    else
    {
        bReadOnly = officecfg::Office::Calc::Layout::Other::TabStop::NonMetric::isReadOnly();
    }
    m_xTabMF->set_sensitive(!bReadOnly);
    m_xTabImg->set_visible(bReadOnly);

    m_xUnitLB->save_value();
    m_xTabMF->save_value();

    ScLkUpdMode nSet=LM_UNKNOWN;

    if(pDoc!=nullptr)
    {
        nSet=pDoc->GetLinkMode();
    }

    if(nSet==LM_UNKNOWN)
    {
        ScAppOptions aAppOptions = ScModule::get()->GetAppOptions();
        nSet=aAppOptions.GetLinkMode();
    }

    switch(nSet)
    {
        case LM_ALWAYS:     m_xAlwaysRB->set_active(true);    break;
        case LM_NEVER:      m_xNeverRB->set_active(true);    break;
        case LM_ON_DEMAND:  m_xRequestRB->set_active(true);    break;
        default:
        {
            // added to avoid warnings
        }
    }

    if (officecfg::Office::Calc::Content::Update::Link::isReadOnly())
    {
        m_xAlwaysRB->set_sensitive(false);
        m_xNeverRB->set_sensitive(false);
        m_xRequestRB->set_sensitive(false);
        m_xUpdateLinksImg->set_visible(true);
    }
    if(const SfxBoolItem* pSelectionItem = rCoreSet->GetItemIfSet(SID_SC_INPUT_SELECTION, false))
        m_xAlignCB->set_active(pSelectionItem->GetValue());

    bReadOnly = officecfg::Office::Calc::Input::MoveSelection::isReadOnly();
    m_xAlignCB->set_sensitive(!bReadOnly);
    m_xAlignImg->set_visible(bReadOnly);

    if(const SfxUInt16Item* pPosItem = rCoreSet->GetItemIfSet(SID_SC_INPUT_SELECTIONPOS, false))
        m_xAlignLB->set_active(pPosItem->GetValue());

    bReadOnly = officecfg::Office::Calc::Input::MoveSelectionDirection::isReadOnly();
    m_xAlignCB->set_sensitive(!bReadOnly);

    if(const SfxBoolItem* pEditModeItem = rCoreSet->GetItemIfSet(SID_SC_INPUT_EDITMODE, false))
        m_xEditModeCB->set_active(pEditModeItem->GetValue());

    bReadOnly = officecfg::Office::Calc::Input::SwitchToEditMode::isReadOnly();
    m_xEditModeCB->set_sensitive(!bReadOnly);
    m_xEditModeImg->set_visible(bReadOnly);

    if(const SfxBoolItem* pExpandItem = rCoreSet->GetItemIfSet(SID_SC_INPUT_FMT_EXPAND, false))
        m_xFormatCB->set_active(pExpandItem->GetValue());

    bReadOnly = officecfg::Office::Calc::Input::ExpandFormatting::isReadOnly();
    m_xFormatCB->set_sensitive(!bReadOnly);
    m_xFormatImg->set_visible(bReadOnly);

    if(const SfxBoolItem* pExpandItem = rCoreSet->GetItemIfSet(SID_SC_INPUT_REF_EXPAND, false))
        m_xExpRefCB->set_active(pExpandItem->GetValue());

    bReadOnly = officecfg::Office::Calc::Input::ExpandReference::isReadOnly();
    m_xExpRefCB->set_sensitive(!bReadOnly);
    m_xExpRefImg->set_visible(bReadOnly);

    if (const SfxBoolItem* pUpdateItem = rCoreSet->GetItemIfSet(SID_SC_OPT_SORT_REF_UPDATE))
        m_xSortRefUpdateCB->set_active(pUpdateItem->GetValue());

    bReadOnly = officecfg::Office::Calc::Input::UpdateReferenceOnSort::isReadOnly();
    m_xSortRefUpdateCB->set_sensitive(!bReadOnly);
    m_xSortRefUpdateImg->set_visible(bReadOnly);

    if(const SfxBoolItem* pHeaderItem = rCoreSet->GetItemIfSet(SID_SC_INPUT_MARK_HEADER, false))
        m_xMarkHdrCB->set_active(pHeaderItem->GetValue());

    bReadOnly = officecfg::Office::Calc::Input::HighlightSelection::isReadOnly();
    m_xMarkHdrCB->set_sensitive(!bReadOnly);
    m_xMarkHdrImg->set_visible(bReadOnly);

    if( const SfxBoolItem* pWarnItem = rCoreSet->GetItemIfSet( SID_SC_INPUT_REPLCELLSWARN, false ) )
        m_xReplWarnCB->set_active( pWarnItem->GetValue() );

    bReadOnly = officecfg::Office::Calc::Input::ReplaceCellsWarning::isReadOnly();
    m_xReplWarnCB->set_sensitive(!bReadOnly);
    m_xReplWarnImg->set_visible(bReadOnly);

    if( const SfxBoolItem* pSelectionItem = rCoreSet->GetItemIfSet( SID_SC_INPUT_LEGACY_CELL_SELECTION, false ) )
        m_xLegacyCellSelectionCB->set_active( pSelectionItem->GetValue() );

    bReadOnly = officecfg::Office::Calc::Input::LegacyCellSelection::isReadOnly();
    m_xLegacyCellSelectionCB->set_sensitive(!bReadOnly);
    m_xLegacyCellSelectionImg->set_visible(bReadOnly);

    if( const SfxBoolItem* pPasteModeItem = rCoreSet->GetItemIfSet( SID_SC_INPUT_ENTER_PASTE_MODE, false ) )
        m_xEnterPasteModeCB->set_active( pPasteModeItem->GetValue() );

    bReadOnly = officecfg::Office::Calc::Input::EnterPasteMode::isReadOnly();
    m_xEnterPasteModeCB->set_sensitive(!bReadOnly);
    m_xEnterPasteModeImg->set_visible(bReadOnly);

    if( const SfxBoolItem* pWarnActiveSheetItem = rCoreSet->GetItemIfSet( SID_SC_INPUT_WARNACTIVESHEET, false ) )
        m_xWarnActiveSheetCB->set_active( pWarnActiveSheetItem->GetValue() );

    bReadOnly = officecfg::Office::Calc::Input::WarnActiveSheet::isReadOnly();
    m_xWarnActiveSheetCB->set_sensitive(!bReadOnly);
    m_xWarnActiveSheetImg->set_visible(bReadOnly);

    m_xAlignCB->save_state();
    m_xAlignLB->save_value();
    m_xEditModeCB->save_state();
    m_xFormatCB->save_state();

    m_xExpRefCB->save_state();
    m_xSortRefUpdateCB->save_state();
    m_xMarkHdrCB->save_state();
    m_xReplWarnCB->save_state();

    m_xLegacyCellSelectionCB->save_state();
    m_xEnterPasteModeCB->save_state();
    m_xWarnActiveSheetCB->save_state();

    AlignHdl(*m_xAlignCB);

    m_xAlwaysRB->save_state();
    m_xNeverRB->save_state();
    m_xRequestRB->save_state();
}

void ScTpLayoutOptions::ActivatePage( const SfxItemSet& /* rCoreSet */ )
{
}

DeactivateRC ScTpLayoutOptions::DeactivatePage( SfxItemSet* pSetP )
{
    if(pSetP)
        FillItemSet(pSetP);
    return DeactivateRC::LeavePage;
}

IMPL_LINK_NOARG(ScTpLayoutOptions, MetricHdl, weld::ComboBox&, void)
{
    const sal_Int32 nMPos = m_xUnitLB->get_active();
    if (nMPos != -1)
    {
        FieldUnit eFieldUnit = static_cast<FieldUnit>(m_xUnitLB->get_id(nMPos).toUInt32());
        sal_Int64 nVal =
            m_xTabMF->denormalize( m_xTabMF->get_value( FieldUnit::TWIP ) );
        ::SetFieldUnit( *m_xTabMF, eFieldUnit );
        m_xTabMF->set_value( m_xTabMF->normalize( nVal ), FieldUnit::TWIP );
    }
}

IMPL_LINK(ScTpLayoutOptions, AlignHdl, weld::Toggleable&, rBox, void)
{
    m_xAlignLB->set_sensitive(rBox.get_active() &&
        !officecfg::Office::Calc::Input::MoveSelectionDirection::isReadOnly());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
