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

#include "doclinkdialog.hxx"

#include <com/sun/star/ui/dialogs/TemplateDescription.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <comphelper/processfactory.hxx>
#include <officecfg/Office/DataAccess.hxx>
#include <strings.hrc>
#include <svl/filenotation.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <ucbhelper/content.hxx>
#include <dialmgr.hxx>
#include <tools/urlobj.hxx>
#include <sfx2/filedlghelper.hxx>
#include <sfx2/docfilt.hxx>

namespace svx
{
    using namespace ::com::sun::star;
    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::ucb;
    using namespace ::svt;

    ODocumentLinkDialog::ODocumentLinkDialog(weld::Window* pParent, bool _bCreateNew)
        : GenericDialogController(pParent, u"cui/ui/databaselinkdialog.ui"_ustr, u"DatabaseLinkDialog"_ustr)
        , m_xBrowseFile(m_xBuilder->weld_button(u"browse"_ustr))
        , m_xName(m_xBuilder->weld_entry(u"name"_ustr))
        , m_xOK(m_xBuilder->weld_button(u"ok"_ustr))
        , m_xAltTitle(m_xBuilder->weld_label(u"alttitle"_ustr))
        , m_xURL(new SvtURLBox(m_xBuilder->weld_combo_box(u"url"_ustr)))
    {
        if (!_bCreateNew)
            m_xDialog->set_title(m_xAltTitle->get_label());

        m_xURL->SetSmartProtocol(INetProtocol::File);
        m_xURL->DisableHistory();
        m_xURL->SetFilter(u"*.odb");

        const css::uno::Reference < css::uno::XComponentContext >& xContext(::comphelper::getProcessComponentContext());
        m_xReadWriteAccess = css::configuration::ReadWriteAccess::create(xContext, u"*"_ustr);

        m_xName->connect_changed( LINK(this, ODocumentLinkDialog, OnEntryModified) );
        m_xURL->connect_changed( LINK(this, ODocumentLinkDialog, OnComboBoxModified) );
        m_xBrowseFile->connect_clicked( LINK(this, ODocumentLinkDialog, OnBrowseFile) );
        m_xOK->connect_clicked( LINK(this, ODocumentLinkDialog, OnOk) );

        validate();
    }

    ODocumentLinkDialog::~ODocumentLinkDialog()
    {
    }

    void ODocumentLinkDialog::setLink(const OUString& rName, const OUString& rURL)
    {
        m_xName->set_text(rName);
        m_xURL->set_entry_text(rURL);
        validate();
    }

    void ODocumentLinkDialog::getLink(OUString& rName, OUString& rURL) const
    {
        rName = m_xName->get_text();
        rURL = m_xURL->get_active_text();
    }

    void ODocumentLinkDialog::validate( )
    {
        m_xOK->set_sensitive((!m_xName->get_text().isEmpty()) && (!m_xURL->get_active_text().isEmpty()));

        if (m_xOK->get_sensitive())
        {
            Reference<container::XNameAccess> xItemList = officecfg::Office::DataAccess::RegisteredNames::get();
            Sequence< OUString > lNodeNames = xItemList->getElementNames();

            for (const OUString& sNodeName : lNodeNames)
            {
                Reference<css::beans::XPropertySet> xSet;
                xItemList->getByName(sNodeName) >>= xSet;

                OUString aDatabaseName;
                if (xSet->getPropertySetInfo()->hasPropertyByName(u"Name"_ustr))
                    xSet->getPropertyValue(u"Name"_ustr) >>= aDatabaseName;

                if (!aDatabaseName.isEmpty() && m_xName->get_text() == aDatabaseName)
                {
                    const OUString aConfigPath = officecfg::Office::DataAccess::RegisteredNames::path() + "/" + sNodeName;
                    if (m_xReadWriteAccess->hasPropertyByHierarchicalName(aConfigPath + "/Name"))
                    {
                        css::beans::Property aProperty = m_xReadWriteAccess->getPropertyByHierarchicalName(aConfigPath + "/Name");
                        bool bReadOnly = (aProperty.Attributes & css::beans::PropertyAttribute::READONLY) != 0;

                        m_xURL->set_sensitive(!bReadOnly);
                        m_xBrowseFile->set_sensitive(!bReadOnly);
                    }

                    if (m_xReadWriteAccess->hasPropertyByHierarchicalName(aConfigPath + "/Location"))
                    {
                        css::beans::Property aProperty = m_xReadWriteAccess->getPropertyByHierarchicalName(aConfigPath + "/Location");
                        bool bReadOnly = (aProperty.Attributes & css::beans::PropertyAttribute::READONLY) != 0;

                        m_xName->set_sensitive(!bReadOnly);
                    }
                    break;
                }
            }
        }
    }

    IMPL_LINK_NOARG(ODocumentLinkDialog, OnOk, weld::Button&, void)
    {
        // get the current URL
        OUString sURL = m_xURL->get_active_text();
        OFileNotation aTransformer(sURL);
        sURL = aTransformer.get(OFileNotation::N_URL);

        // check for the existence of the selected file
        bool bFileExists = false;
        try
        {
            ::ucbhelper::Content aFile(sURL, Reference< XCommandEnvironment >(), comphelper::getProcessComponentContext());
            if (aFile.isDocument())
                bFileExists = true;
        }
        catch(Exception&)
        {
        }

        if (!bFileExists)
        {
            OUString sMsg = CuiResId(STR_LINKEDDOC_DOESNOTEXIST);
            sMsg = sMsg.replaceFirst("$file$", m_xURL->get_active_text());
            std::unique_ptr<weld::MessageDialog> xErrorBox(Application::CreateMessageDialog(m_xDialog.get(),
                                                           VclMessageType::Warning, VclButtonsType::Ok, sMsg));
            xErrorBox->run();
            return;
        } // if (!bFileExists)
        INetURLObject aURL( sURL );
        if ( aURL.GetProtocol() != INetProtocol::File )
        {
            OUString sMsg = CuiResId(STR_LINKEDDOC_NO_SYSTEM_FILE);
            sMsg = sMsg.replaceFirst("$file$", m_xURL->get_active_text());
            std::unique_ptr<weld::MessageDialog> xErrorBox(Application::CreateMessageDialog(m_xDialog.get(),
                                                           VclMessageType::Warning, VclButtonsType::Ok, sMsg));
            xErrorBox->run();
            return;
        }

        OUString sCurrentText = m_xName->get_text();
        if ( m_aNameValidator.IsSet() )
        {
            if ( !m_aNameValidator.Call( sCurrentText ) )
            {
                OUString sMsg = CuiResId(STR_NAME_CONFLICT);
                sMsg = sMsg.replaceFirst("$file$", sCurrentText);
                std::unique_ptr<weld::MessageDialog> xErrorBox(Application::CreateMessageDialog(m_xDialog.get(),
                                                               VclMessageType::Info, VclButtonsType::Ok, sMsg));
                xErrorBox->run();

                m_xName->select_region(0, -1);
                m_xName->grab_focus();
                return;
            }
        }

        m_xDialog->response(RET_OK);
    }

    IMPL_LINK_NOARG(ODocumentLinkDialog, OnBrowseFile, weld::Button&, void)
    {
        ::sfx2::FileDialogHelper aFileDlg(
                ui::dialogs::TemplateDescription::FILEOPEN_READONLY_VERSION, FileDialogFlags::NONE, m_xDialog.get());
        std::shared_ptr<const SfxFilter> pFilter = SfxFilter::GetFilterByName(u"StarOffice XML (Base)"_ustr);
        if ( pFilter )
        {
            aFileDlg.AddFilter(pFilter->GetUIName(),pFilter->GetDefaultExtension());
            aFileDlg.SetCurrentFilter(pFilter->GetUIName());
        }

        OUString sPath = m_xURL->get_active_text();
        if (!sPath.isEmpty())
        {
            OFileNotation aTransformer( sPath, OFileNotation::N_SYSTEM );
            aFileDlg.SetDisplayDirectory( aTransformer.get( OFileNotation::N_URL ) );
        }

        if (ERRCODE_NONE != aFileDlg.Execute())
            return;

        if (m_xName->get_text().isEmpty())
        {   // default the name to the base of the chosen URL
            INetURLObject aParser;

            aParser.SetSmartProtocol(INetProtocol::File);
            aParser.SetSmartURL(aFileDlg.GetPath());

            m_xName->set_text(aParser.getBase(INetURLObject::LAST_SEGMENT, true, INetURLObject::DecodeMechanism::WithCharset));

            m_xName->select_region(0, -1);
            m_xName->grab_focus();
        }
        else
            m_xURL->grab_focus();

        // get the path in system notation
        OFileNotation aTransformer(aFileDlg.GetPath(), OFileNotation::N_URL);
        m_xURL->set_entry_text(aTransformer.get(OFileNotation::N_SYSTEM));

        validate();
    }

    IMPL_LINK_NOARG(ODocumentLinkDialog, OnEntryModified, weld::Entry&, void)
    {
        validate();
    }

    IMPL_LINK_NOARG(ODocumentLinkDialog, OnComboBoxModified, weld::ComboBox&, void)
    {
        validate();
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
