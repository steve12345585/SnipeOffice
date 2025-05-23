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

#include <sal/config.h>

#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/sdb/DatabaseContext.hpp>
#include <com/sun/star/sdb/CommandType.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/sequence.hxx>
#include <sfx2/viewfrm.hxx>
#include <o3tl/string_view.hxx>

#include <view.hxx>
#include <wrtsh.hxx>
#include <dbmgr.hxx>
#include <changedb.hxx>

#include <strings.hrc>
#include <bitmaps.hlst>

using namespace ::com::sun::star::container;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::uno;

// edit insert-field
SwChangeDBDlg::SwChangeDBDlg(SwView const & rVw)
    : SfxDialogController(rVw.GetViewFrame().GetFrameWeld(), u"modules/swriter/ui/exchangedatabases.ui"_ustr,
                          u"ExchangeDatabasesDialog"_ustr)
    , m_pSh(rVw.GetWrtShellPtr())
    , m_xUsedDBTLB(m_xBuilder->weld_tree_view(u"inuselb"_ustr))
    , m_xAvailDBTLB(new SwDBTreeList(m_xBuilder->weld_tree_view(u"availablelb"_ustr)))
    , m_xAddDBPB(m_xBuilder->weld_button(u"browse"_ustr))
    , m_xDocDBNameFT(m_xBuilder->weld_label(u"dbnameft"_ustr))
    , m_xDefineBT(m_xBuilder->weld_button(u"ok"_ustr))
{
    int nWidth = m_xUsedDBTLB->get_approximate_digit_width() * 25;
    int nHeight = m_xUsedDBTLB->get_height_rows(8);
    m_xUsedDBTLB->set_size_request(nWidth, nHeight);
    m_xAvailDBTLB->set_size_request(nWidth, nHeight);

    m_xAvailDBTLB->SetWrtShell(*m_pSh);
    FillDBPopup();

    ShowDBName(m_pSh->GetDBData());
    m_xDefineBT->connect_clicked(LINK(this, SwChangeDBDlg, ButtonHdl));
    m_xAddDBPB->connect_clicked(LINK(this, SwChangeDBDlg, AddDBHdl));

    m_xUsedDBTLB->set_selection_mode(SelectionMode::Multiple);
    m_xUsedDBTLB->make_sorted();

    Link<weld::TreeView&,void> aLink = LINK(this, SwChangeDBDlg, TreeSelectHdl);

    m_xUsedDBTLB->connect_selection_changed(aLink);
    m_xAvailDBTLB->connect_changed(aLink);
    TreeSelect();
}

// initialise database listboxes
void SwChangeDBDlg::FillDBPopup()
{
    const Reference< XComponentContext >& xContext( ::comphelper::getProcessComponentContext() );
    Reference<XDatabaseContext> xDBContext = DatabaseContext::create(xContext);
    const SwDBData& rDBData = m_pSh->GetDBData();
    m_xAvailDBTLB->Select(rDBData.sDataSource, rDBData.sCommand, u"");
    TreeSelect();

    Sequence< OUString > aDBNames = xDBContext->getElementNames();
    auto aAllDBNames = comphelper::sequenceToContainer<std::vector<OUString>>(aDBNames);

    std::vector<OUString> aDBNameList;
    m_pSh->GetAllUsedDB( aDBNameList, &aAllDBNames );

    size_t nCount = aDBNameList.size();
    m_xUsedDBTLB->clear();
    std::unique_ptr<weld::TreeIter> xFirst;

    for(size_t k = 0; k < nCount; k++)
    {
        std::unique_ptr<weld::TreeIter> xLast = Insert(o3tl::getToken(aDBNameList[k], 0, ';'));
        if (!xFirst)
            xFirst = std::move(xLast);
    }

    if (xFirst)
    {
        m_xUsedDBTLB->expand_row(*xFirst);
        m_xUsedDBTLB->scroll_to_row(*xFirst);
        m_xUsedDBTLB->select(*xFirst);
    }
}

std::unique_ptr<weld::TreeIter> SwChangeDBDlg::Insert(std::u16string_view rDBName)
{
    sal_Int32 nIdx{ 0 };
    const OUString sDBName(o3tl::getToken(rDBName, 0, DB_DELIM, nIdx));
    const OUString sTableName(o3tl::getToken(rDBName, 0, DB_DELIM, nIdx));
    OUString sUserData( o3tl::getToken(rDBName, 0, DB_DELIM, nIdx) );
    sal_Int32 nCommandType = sUserData.toInt32();

    const OUString & rToInsert ( nCommandType ? RID_BMP_DBQUERY : RID_BMP_DBTABLE );

    std::unique_ptr<weld::TreeIter> xIter(m_xUsedDBTLB->make_iterator());
    if (m_xUsedDBTLB->get_iter_first(*xIter))
    {
        do
        {
            if (sDBName == m_xUsedDBTLB->get_text(*xIter))
            {
                if (m_xUsedDBTLB->iter_has_child(*xIter))
                {
                    std::unique_ptr<weld::TreeIter> xChild(m_xUsedDBTLB->make_iterator(xIter.get()));
                    if (m_xUsedDBTLB->iter_children(*xChild))
                    {
                        do
                        {
                            if (sTableName == m_xUsedDBTLB->get_text(*xChild))
                                return xChild;
                        } while (m_xUsedDBTLB->iter_next_sibling(*xChild));
                    }
                }
                m_xUsedDBTLB->insert(xIter.get(), -1, &sTableName, &sUserData, nullptr, nullptr,
                                     false, xIter.get());
                m_xUsedDBTLB->set_image(*xIter, rToInsert);
                return xIter;
            }
        } while (m_xUsedDBTLB->iter_next_sibling(*xIter));
    }

    m_xUsedDBTLB->insert(nullptr, -1, &sDBName, nullptr, nullptr, nullptr,
                         false, xIter.get());
    m_xUsedDBTLB->set_image(*xIter, RID_BMP_DB);
    m_xUsedDBTLB->insert(xIter.get(), -1, &sTableName, &sUserData, nullptr, nullptr,
                         false, xIter.get());
    m_xUsedDBTLB->set_image(*xIter, rToInsert);
    return xIter;
}

// destroy dialog
SwChangeDBDlg::~SwChangeDBDlg()
{
}

void SwChangeDBDlg::UpdateFields()
{
    std::vector<OUString> aDBNames;

    m_xUsedDBTLB->selected_foreach([this, &aDBNames](weld::TreeIter& rEntry){
        if (m_xUsedDBTLB->get_iter_depth(rEntry))
        {
            std::unique_ptr<weld::TreeIter> xIter(m_xUsedDBTLB->make_iterator(&rEntry));
            m_xUsedDBTLB->iter_parent(*xIter);
            OUString sTmp(m_xUsedDBTLB->get_text(*xIter) +
                          OUStringChar(DB_DELIM) + m_xUsedDBTLB->get_text(rEntry) + OUStringChar(DB_DELIM) +
                          m_xUsedDBTLB->get_id(rEntry));
            aDBNames.push_back(sTmp);
        }
        return false;
    });

    m_pSh->StartAllAction();
    OUString sTableName;
    OUString sColumnName;
    sal_Bool bIsTable = false;
    const OUString DBName(m_xAvailDBTLB->GetDBName(sTableName, sColumnName, &bIsTable));
    const OUString sTemp = DBName
        + OUStringChar(DB_DELIM)
        + sTableName
        + OUStringChar(DB_DELIM)
        + OUString::number(bIsTable
                            ? CommandType::TABLE
                            : CommandType::QUERY);
    m_pSh->ChangeDBFields( aDBNames, sTemp);
    m_pSh->EndAllAction();
}

IMPL_LINK_NOARG(SwChangeDBDlg, ButtonHdl, weld::Button&, void)
{
    OUString sTableName;
    OUString sColumnName;
    SwDBData aData;
    sal_Bool bIsTable = false;
    aData.sDataSource = m_xAvailDBTLB->GetDBName(sTableName, sColumnName, &bIsTable);
    aData.sCommand = sTableName;
    aData.nCommandType = bIsTable ? 0 : 1;
    m_pSh->ChgDBData(aData);
    ShowDBName(m_pSh->GetDBData());
    m_xDialog->response(RET_OK);
}

IMPL_LINK_NOARG(SwChangeDBDlg, TreeSelectHdl, weld::TreeView&, void)
{
    TreeSelect();
}

void SwChangeDBDlg::TreeSelect()
{
    bool bEnable = false;
    std::unique_ptr<weld::TreeIter> xIter(m_xAvailDBTLB->make_iterator());
    if (m_xAvailDBTLB->get_selected(xIter.get()))
    {
        if (m_xAvailDBTLB->get_iter_depth(*xIter))
            bEnable = true;
    }
    m_xDefineBT->set_sensitive(bEnable);
}


// convert database name for display
void SwChangeDBDlg::ShowDBName(const SwDBData& rDBData)
{
    if (rDBData.sDataSource.isEmpty() && rDBData.sCommand.isEmpty())
    {
        m_xDocDBNameFT->set_label(SwResId(SW_STR_NONE));
    }
    else
    {
        const OUString sName(rDBData.sDataSource + "." + rDBData.sCommand);
        m_xDocDBNameFT->set_label(sName.replaceAll("~", "~~"));
    }
}

IMPL_LINK_NOARG(SwChangeDBDlg, AddDBHdl, weld::Button&, void)
{
    const OUString sNewDB = SwDBManager::LoadAndRegisterDataSource(m_xDialog.get());
    if (!sNewDB.isEmpty())
    {
        m_xAvailDBTLB->AddDataSource(sNewDB);
        TreeSelect();
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
