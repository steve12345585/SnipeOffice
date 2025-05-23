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

#include <sfx2/docfile.hxx>
#include <sfx2/docfilt.hxx>
#include <sfx2/docinsert.hxx>
#include <sfx2/fcontnr.hxx>
#include <sfx2/filedlghelper.hxx>
#include <svtools/ehdl.hxx>
#include <svtools/inettbc.hxx>
#include <svtools/sfxecode.hxx>
#include <o3tl/string_view.hxx>

#include <dbdata.hxx>
#include <linkarea.hxx>
#include <docsh.hxx>
#include <tablink.hxx>
#include <scresid.hxx>
#include <strings.hrc>

ScLinkedAreaDlg::ScLinkedAreaDlg(weld::Widget* pParent)
    : GenericDialogController(pParent, u"modules/scalc/ui/externaldata.ui"_ustr, u"ExternalDataDialog"_ustr)
    , m_xCbUrl(new SvtURLBox(m_xBuilder->weld_combo_box(u"url"_ustr)))
    , m_xBtnBrowse(m_xBuilder->weld_button(u"browse"_ustr))
    , m_xLbRanges(m_xBuilder->weld_tree_view(u"ranges"_ustr))
    , m_xBtnReload(m_xBuilder->weld_check_button(u"reload"_ustr))
    , m_xNfDelay(m_xBuilder->weld_spin_button(u"delay"_ustr))
    , m_xFtSeconds(m_xBuilder->weld_label(u"secondsft"_ustr))
    , m_xBtnOk(m_xBuilder->weld_button(u"ok"_ustr))
{
    m_xLbRanges->set_selection_mode(SelectionMode::Multiple);

    m_xCbUrl->connect_entry_activate(LINK(this, ScLinkedAreaDlg, FileHdl));
    m_xBtnBrowse->connect_clicked(LINK( this, ScLinkedAreaDlg, BrowseHdl));
    m_xLbRanges->connect_selection_changed(LINK(this, ScLinkedAreaDlg, RangeHdl));
    m_xLbRanges->set_size_request(m_xLbRanges->get_approximate_digit_width() * 54,
                                  m_xLbRanges->get_height_rows(5));
    m_xBtnReload->connect_toggled(LINK( this, ScLinkedAreaDlg, ReloadHdl));
    UpdateEnable();
}

ScLinkedAreaDlg::~ScLinkedAreaDlg()
{
}

constexpr OUString FILTERNAME_HTML = u"HTML (StarCalc)"_ustr;
constexpr OUString FILTERNAME_QUERY = u"calc_HTML_WebQuery"_ustr;

IMPL_LINK_NOARG(ScLinkedAreaDlg, BrowseHdl, weld::Button&, void)
{
    m_xDocInserter.reset( new sfx2::DocumentInserter(m_xDialog.get(), ScDocShell::Factory().GetFactoryName()) );
    m_xDocInserter->StartExecuteModal( LINK( this, ScLinkedAreaDlg, DialogClosedHdl ) );
}

IMPL_LINK_NOARG(ScLinkedAreaDlg, FileHdl, weld::ComboBox&, bool)
{
    OUString aEntered = m_xCbUrl->GetURL();
    if (m_pSourceShell)
    {
        SfxMedium* pMed = m_pSourceShell->GetMedium();
        if ( aEntered == pMed->GetName() )
        {
            //  already loaded - nothing to do
            return true;
        }
    }

    OUString aFilter;
    OUString aOptions;
    //  get filter name by looking at the file content (bWithContent = true)
    // Break operation if any error occurred inside.
    if (!ScDocumentLoader::GetFilterName( aEntered, aFilter, aOptions, true, false ))
        return true;

    // #i53241# replace HTML filter with DataQuery filter
    if (aFilter == FILTERNAME_HTML)
        aFilter = FILTERNAME_QUERY;

    LoadDocument( aEntered, aFilter, aOptions );

    UpdateSourceRanges();
    UpdateEnable();

    return true;
}

void ScLinkedAreaDlg::LoadDocument( const OUString& rFile, const OUString& rFilter, const OUString& rOptions )
{
    if (m_pSourceShell)
    {
        //  unload old document
        m_pSourceShell->DoClose();
        m_pSourceShell.clear();
    }

    if ( rFile.isEmpty() )
        return;

    weld::WaitObject aWait(m_xDialog.get());

    OUString aNewFilter = rFilter;
    OUString aNewOptions = rOptions;

    SfxErrorContext aEc( ERRCTX_SFX_OPENDOC, rFile );

    ScDocumentLoader aLoader( rFile, aNewFilter, aNewOptions, 0, m_xDialog.get() );    // with interaction
    m_pSourceShell = aLoader.GetDocShell();
    if (m_pSourceShell)
    {
        ErrCodeMsg nErr = m_pSourceShell->GetErrorCode();
        if (nErr)
            ErrorHandler::HandleError( nErr );      // including warnings

        aLoader.ReleaseDocRef();    // don't call DoClose in DocLoader dtor
    }
}

void ScLinkedAreaDlg::InitFromOldLink( const OUString& rFile, const OUString& rFilter,
                                        const OUString& rOptions, std::u16string_view rSource,
                                        sal_Int32 nRefreshDelaySeconds )
{
    LoadDocument( rFile, rFilter, rOptions );
    if (m_pSourceShell)
    {
        SfxMedium* pMed = m_pSourceShell->GetMedium();
        m_xCbUrl->set_entry_text(pMed->GetName());
    }
    else
        m_xCbUrl->set_entry_text(OUString());

    UpdateSourceRanges();

    if (!rSource.empty())
    {
        sal_Int32 nIdx {0};
        do
        {
            m_xLbRanges->select_text(OUString(o3tl::getToken(rSource, 0, ';', nIdx)));
        }
        while (nIdx>0);
    }

    bool bDoRefresh = (nRefreshDelaySeconds != 0);
    m_xBtnReload->set_active(bDoRefresh);
    if (bDoRefresh)
        m_xNfDelay->set_value(nRefreshDelaySeconds);

    UpdateEnable();
}

IMPL_LINK_NOARG(ScLinkedAreaDlg, RangeHdl, weld::TreeView&, void)
{
    UpdateEnable();
}

IMPL_LINK_NOARG(ScLinkedAreaDlg, ReloadHdl, weld::Toggleable&, void)
{
    UpdateEnable();
}

IMPL_LINK( ScLinkedAreaDlg, DialogClosedHdl, sfx2::FileDialogHelper*, _pFileDlg, void )
{
    if ( _pFileDlg->GetError() != ERRCODE_NONE )
        return;

    std::unique_ptr<SfxMedium> pMed = m_xDocInserter->CreateMedium();
    if ( pMed )
    {
        weld::WaitObject aWait(m_xDialog.get());

        // replace HTML filter with DataQuery filter
        std::shared_ptr<const SfxFilter> pFilter = pMed->GetFilter();
        if (pFilter && FILTERNAME_HTML == pFilter->GetFilterName())
        {
            std::shared_ptr<const SfxFilter> pNewFilter =
                ScDocShell::Factory().GetFilterContainer()->GetFilter4FilterName( FILTERNAME_QUERY );
            if( pNewFilter )
                pMed->SetFilter( pNewFilter );
        }

        //  ERRCTX_SFX_OPENDOC -> "Error loading document"
        SfxErrorContext aEc( ERRCTX_SFX_OPENDOC, pMed->GetName() );

        if (m_pSourceShell)
            m_pSourceShell->DoClose();        // deleted when assigning aSourceRef

        pMed->UseInteractionHandler( true );    // to enable the filter options dialog

        m_pSourceShell = new ScDocShell;
        m_pSourceShell->DoLoad( pMed.get() );

        ErrCodeMsg nErr = m_pSourceShell->GetErrorCode();
        if (nErr)
            ErrorHandler::HandleError( nErr );              // including warnings

        if (!m_pSourceShell->GetErrorIgnoreWarning())                    // only errors
        {
            m_xCbUrl->set_entry_text(pMed->GetName());
        }
        else
        {
            m_pSourceShell->DoClose();
            m_pSourceShell.clear();

            m_xCbUrl->set_entry_text(OUString());
        }
        pMed.release(); // DoLoad takes ownership
    }

    UpdateSourceRanges();
    UpdateEnable();
}

#undef FILTERNAME_HTML
#undef FILTERNAME_QUERY

void ScLinkedAreaDlg::UpdateSourceRanges()
{
    m_xLbRanges->freeze();

    m_xLbRanges->clear();
    if ( m_pSourceShell )
    {
        std::shared_ptr<const SfxFilter> pFilter = m_pSourceShell->GetMedium()->GetFilter();
        if (pFilter && pFilter->GetFilterName() == SC_TEXT_CSV_FILTER_NAME)
        {
            // Insert dummy All range to have something selectable.
            m_xLbRanges->append_text(u"CSV_all"_ustr);
        }

        // tdf#142600 - list tables in order of their appearance in the document's source
        const ScRangeName* pRangeName = m_pSourceShell->GetDocument().GetRangeName();
        for (size_t i = 1; i <= pRangeName->index_size(); i++)
        {
            if (const ScRangeData* pRangeData = pRangeName->findByIndex(i))
            {
                m_xLbRanges->append_text(pRangeData->GetName());
            }
        }
        // tdf#142600 - list database ranges
        if (const auto pDBs = m_pSourceShell->GetDocument().GetDBCollection())
        {
            const auto& rNamedDBs = pDBs->getNamedDBs();
            for (const auto& rNamedDB : rNamedDBs)
                m_xLbRanges->append_text(rNamedDB->GetName());
        }
    }

    m_xLbRanges->thaw();

    if (m_xLbRanges->n_children() >= 1)
        m_xLbRanges->select(0);
    else
    {
        m_xLbRanges->append_text(ScResId(STR_NO_NAMED_RANGES_AVAILABLE));
        m_xLbRanges->set_sensitive(false);
    }
}

void ScLinkedAreaDlg::UpdateEnable()
{
    bool bEnable = ( m_pSourceShell && m_xLbRanges->count_selected_rows() );
    m_xBtnOk->set_sensitive(bEnable);

    bool bReload = m_xBtnReload->get_active();
    m_xNfDelay->set_sensitive(bReload);
    m_xFtSeconds->set_sensitive(bReload);
}

OUString ScLinkedAreaDlg::GetURL() const
{
    if (m_pSourceShell)
    {
        SfxMedium* pMed = m_pSourceShell->GetMedium();
        return pMed->GetName();
    }
    return OUString();
}

OUString ScLinkedAreaDlg::GetFilter() const
{
    if (m_pSourceShell)
    {
        SfxMedium* pMed = m_pSourceShell->GetMedium();
        return pMed->GetFilter()->GetFilterName();
    }
    return OUString();
}

OUString ScLinkedAreaDlg::GetOptions() const
{
    if (m_pSourceShell)
    {
        SfxMedium* pMed = m_pSourceShell->GetMedium();
        return ScDocumentLoader::GetOptions( *pMed );
    }
    return OUString();
}

OUString ScLinkedAreaDlg::GetSource() const
{
    OUStringBuffer aBuf;
    std::vector<OUString> aSelection = m_xLbRanges->get_selected_rows_text();
    for (size_t i = 0; i < aSelection.size(); ++i)
    {
        if (i > 0)
            aBuf.append(';');
        aBuf.append(aSelection[i]);
    }
    return aBuf.makeStringAndClear();
}

sal_Int32 ScLinkedAreaDlg::GetRefreshDelaySeconds() const
{
    if (m_xBtnReload->get_active())
        return m_xNfDelay->get_value();
    else
        return 0;   // disabled
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
