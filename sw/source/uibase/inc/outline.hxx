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

#include <memory>
#include <sfx2/tabdlg.hxx>
#include <swtypes.hxx>
#include "numprevw.hxx"
#include "numberingtypelistbox.hxx"
#include <rtl/ustring.hxx>

class SwWrtShell;
class SwNumRule;
class SwChapterNumRules;

class SwOutlineTabDialog final : public SfxTabDialogController
{
    static     sal_uInt16    s_nNumLevel;
    static_assert(sizeof(s_nNumLevel) * CHAR_BIT >= MAXLEVEL);

    UIName              m_aCollNames[MAXLEVEL];

    SwWrtShell&         m_rWrtSh;
    std::unique_ptr<SwNumRule>  m_xNumRule;
    SwChapterNumRules*  m_pChapterNumRules;

    bool                m_bModified : 1;

    std::unique_ptr<weld::MenuButton> m_xMenuButton;

    DECL_LINK(CancelHdl, weld::Button&, void);
    DECL_LINK(FormHdl, weld::Toggleable&, void);
    DECL_LINK(MenuSelectHdl, const OUString&, void);

    virtual void    PageCreated(const OUString& rPageId, SfxTabPage& rPage) override;
    virtual short   Ok() override;

public:
    SwOutlineTabDialog(weld::Window* pParent, const SfxItemSet* pSwItemSet, SwWrtShell &);
    virtual ~SwOutlineTabDialog() override;

    SwNumRule*          GetNumRule() { return m_xNumRule.get(); }
    sal_uInt16          GetLevel(std::u16string_view rFormatName) const;
    UIName*             GetCollNames() {return m_aCollNames;}

    static sal_uInt16   GetActNumLevel() {return s_nNumLevel;}
    static void         SetActNumLevel(sal_uInt16 nSet) {s_nNumLevel = nSet;}
};

class SwOutlineSettingsTabPage final : public SfxTabPage
{
    OUString            m_aNoFormatName;
    UIName              m_aSaveCollNames[MAXLEVEL];
    SwWrtShell*         m_pSh;
    SwNumRule*          m_pNumRule;
    UIName*             m_pCollNames;
    sal_uInt16          m_nActLevel;
    NumberingPreview  m_aPreviewWIN;

    std::unique_ptr<weld::TreeView> m_xLevelLB;
    std::unique_ptr<weld::ComboBox> m_xCollBox;
    std::unique_ptr<SwNumberingTypeListBox> m_xNumberBox;
    std::unique_ptr<weld::ComboBox> m_xCharFormatLB;
    std::unique_ptr<weld::Label> m_xAllLevelFT;
    std::unique_ptr<weld::SpinButton>  m_xAllLevelNF;
    std::unique_ptr<weld::Entry> m_xPrefixED;
    std::unique_ptr<weld::Entry> m_xSuffixED;
    std::unique_ptr<weld::SpinButton> m_xStartEdit;
    std::unique_ptr<weld::CustomWeld> m_xPreviewWIN;

    DECL_LINK( LevelHdl, weld::TreeView&, void );
    DECL_LINK( ToggleComplete, weld::SpinButton&, void );
    DECL_LINK( CollSelect, weld::ComboBox&, void );
    void CollSave();
    DECL_LINK( NumberSelect, weld::ComboBox&, void );
    DECL_LINK( DelimModify, weld::Entry&, void );
    DECL_LINK( StartModified, weld::SpinButton&, void );
    DECL_LINK( CharFormatHdl, weld::ComboBox&, void );

    void    Update();

    void    SetModified() { m_aPreviewWIN.Invalidate(); }
    void    CheckForStartValue_Impl(sal_uInt16 nNumberingType);

public:
    SwOutlineSettingsTabPage(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rSet);
    static std::unique_ptr<SfxTabPage>  Create( weld::Container* pPage, weld::DialogController* pController,
                                       const SfxItemSet* rAttrSet);
    virtual ~SwOutlineSettingsTabPage() override;

    void SetWrtShell(SwWrtShell* pShell);

    virtual void        ActivatePage(const SfxItemSet& rSet) override;
    virtual DeactivateRC   DeactivatePage(SfxItemSet *pSet) override;

    virtual bool        FillItemSet( SfxItemSet* rSet ) override;
    virtual void        Reset( const SfxItemSet* rSet ) override;
    void SetNumRule(SwNumRule *pRule)
    {
        m_pNumRule = pRule;
        m_aPreviewWIN.SetNumRule(m_pNumRule);
    }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
