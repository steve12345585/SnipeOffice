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


#include <strings.hrc>
#include "general.hxx"
#include "bibview.hxx"
#include "datman.hxx"
#include "bibresid.hxx"
#include "bibmod.hxx"
#include "bibconfig.hxx"


#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::form;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::uno;

namespace
{
    class MessageWithCheck : public weld::MessageDialogController
    {
    private:
        std::unique_ptr<weld::CheckButton> m_xWarningOnBox;
    public:
        MessageWithCheck(weld::Window *pParent)
            : MessageDialogController(pParent, u"modules/sbibliography/ui/querydialog.ui"_ustr, u"QueryDialog"_ustr, u"ask"_ustr)
            , m_xWarningOnBox(m_xBuilder->weld_check_button(u"ask"_ustr))
        {
        }
        bool get_active() const { return m_xWarningOnBox->get_active(); }
    };
}

namespace bib
{


    BibView::BibView( vcl::Window* _pParent, BibDataManager* _pManager, WinBits _nStyle )
        :BibWindow( _pParent, _nStyle )
        ,m_pDatMan( _pManager )
        ,m_xDatMan( _pManager )
        ,m_pGeneralPage( nullptr )
        ,m_aFormControlContainer(this)
    {
        if ( m_xDatMan.is() )
            m_aFormControlContainer.connectForm( m_xDatMan );
    }


    BibView::~BibView()
    {
        disposeOnce();
    }

    void BibView::dispose()
    {
        VclPtr<BibGeneralPage> pGeneralPage = m_pGeneralPage;
        m_pGeneralPage.clear();
        pGeneralPage.disposeAndClear(); // dispose will commit any uncommitted weld::Entry changes

        if ( m_aFormControlContainer.isFormConnected() )
            m_aFormControlContainer.disconnectForm();

        BibWindow::dispose();
    }

    void BibView::UpdatePages()
    {
        // TODO:
        // this is _strange_: Why not updating the existent general page?
        // I consider the current behaviour a HACK.
        if ( m_pGeneralPage )
        {
            m_pGeneralPage->Hide();
            m_pGeneralPage.disposeAndClear();
        }

        m_pGeneralPage = VclPtr<BibGeneralPage>::Create( this, m_pDatMan );
        m_pGeneralPage->Show();

        if( HasFocus() )
            // "delayed" GetFocus() because GetFocus() is initially called before GeneralPage is created
            m_pGeneralPage->GrabFocus();

        OUString sErrorString( m_pGeneralPage->GetErrorString() );
        if ( sErrorString.isEmpty() )
            return;

        bool bExecute = BibModul::GetConfig()->IsShowColumnAssignmentWarning();
        if(!m_pDatMan->HasActiveConnection())
        {
            //no connection is available -> the data base has to be assigned
            m_pDatMan->DispatchDBChangeDialog();
            bExecute = false;
        }
        else if(bExecute)
        {
            sErrorString += "\n" + BibResId(RID_MAP_QUESTION);

            MessageWithCheck aQueryBox(GetFrameWeld());
            aQueryBox.set_primary_text(sErrorString);

            short nResult = aQueryBox.run();
            BibModul::GetConfig()->SetShowColumnAssignmentWarning(!aQueryBox.get_active());

            if( RET_YES != nResult )
            {
                bExecute = false;
            }
        }
        if(bExecute)
        {
            Application::PostUserEvent( LINK( this, BibView, CallMappingHdl ), nullptr, true );
        }
    }

    BibViewFormControlContainer::BibViewFormControlContainer(BibView *pBibView) : mpBibView(pBibView) {}

    void BibViewFormControlContainer::_loaded( const EventObject& _rEvent )
    {
        mpBibView->UpdatePages();
        FormControlContainer::_loaded( _rEvent );
        mpBibView->Resize();
    }

    void BibViewFormControlContainer::_reloaded( const EventObject& _rEvent )
    {
        mpBibView->UpdatePages();
        FormControlContainer::_loaded( _rEvent );
        mpBibView->Resize();
    }

    IMPL_LINK_NOARG( BibView, CallMappingHdl, void*, void)
    {
        m_pDatMan->CreateMappingDialog(GetFrameWeld());
    }

    void BibView::Resize()
    {
        if ( m_pGeneralPage )
        {
            ::Size aSz( GetOutputSizePixel() );
            m_pGeneralPage->SetSizePixel( aSz );
        }
        Window::Resize();
    }

    Reference< awt::XControlContainer > BibViewFormControlContainer::getControlContainer()
    {
        return nullptr;
    }

    void BibView::GetFocus()
    {
        if( m_pGeneralPage )
            m_pGeneralPage->GrabFocus();
    }

    bool BibView::HandleShortCutKey( const KeyEvent& rKeyEvent )
    {
        return m_pGeneralPage && m_pGeneralPage->HandleShortCutKey( rKeyEvent );
    }


}   // namespace bib


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
