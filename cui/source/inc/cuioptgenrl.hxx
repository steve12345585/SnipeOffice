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
#pragma once

// include ---------------------------------------------------------------

#include <sfx2/tabdlg.hxx>
#include <vcl/weld.hxx>

#include <vector>

// class SvxGeneralTabPage -----------------------------------------------

class SvxGeneralTabPage : public SfxTabPage
{
private:
    // the "Use data for document properties" checkbox
    std::unique_ptr<weld::CheckButton> m_xUseDataCB;
    std::unique_ptr<weld::Widget> m_xUseDataImg;
    std::unique_ptr<weld::Widget> m_xCryptoFrame;
    std::unique_ptr<weld::Entry> m_xSigningKeyLB;
    std::unique_ptr<weld::Label> m_xSigningKeyFT;
    std::unique_ptr<weld::Widget> m_xSigningKeyImg;
    std::unique_ptr<weld::Button> m_xSigningKeyButton;
    std::unique_ptr<weld::Button> m_xRemoveSigningKeyButton;
    std::unique_ptr<weld::Entry> m_xEncryptionKeyLB;
    std::unique_ptr<weld::Label> m_xEncryptionKeyFT;
    std::unique_ptr<weld::Widget> m_xEncryptionKeyImg;
    std::unique_ptr<weld::Button> m_xEncryptionKeyButton;
    std::unique_ptr<weld::Button> m_xRemoveEncryptionKeyButton;
    std::unique_ptr<weld::CheckButton> m_xEncryptToSelfCB;
    std::unique_ptr<weld::Widget> m_xEncryptToSelfImg;
    // rows
    struct Row;
    std::vector<std::shared_ptr<Row> > vRows;
    // fields
    struct Field;
    std::vector<std::shared_ptr<Field> > vFields;
    // "name" fields
    unsigned nNameRow;
    unsigned nShortNameField;

    OUString msCurrentSigningKey;
    OUString msCurrentEncryptionKey;

    DECL_LINK( ModifyHdl_Impl, weld::Entry&, void );
    DECL_LINK( ChooseKeyButtonHdl, weld::Button&, void );
    DECL_LINK( RemoveKeyButtonHdl, weld::Button&, void );

    bool                GetData_Impl();
    void                SetData_Impl();

    void InitControls ();
    void InitCryptography();
    void SetLinks ();

protected:
    virtual DeactivateRC DeactivatePage( SfxItemSet* pSet ) override;

public:
    SvxGeneralTabPage(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rSet);
    virtual ~SvxGeneralTabPage() override;

    static std::unique_ptr<SfxTabPage> Create( weld::Container* pPage, weld::DialogController* pController, const SfxItemSet* rAttrSet );

    virtual OUString GetAllStrings() override;

    virtual bool        FillItemSet( SfxItemSet* rSet ) override;
    virtual void        Reset( const SfxItemSet* rSet ) override;
};


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
