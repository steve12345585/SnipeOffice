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
#ifndef INCLUDED_SW_SOURCE_UIBASE_INC_CPTION_HXX
#define INCLUDED_SW_SOURCE_UIBASE_INC_CPTION_HXX

#include <sfx2/basedlgs.hxx>

#include <com/sun/star/container/XNameAccess.hpp>

#include "wrtsh.hxx"
#include "optload.hxx"

class SwFieldMgr;
class SwView;

class SwCaptionDialog final : public SfxDialogController
{
    OUString m_sNone;
    TextFilterAutoConvert m_aTextFilter;
    SwView& m_rView; // search per active, avoid View
    std::unique_ptr<SwFieldMgr> m_pMgr; // pointer to save the include

    OUString m_sCharacterStyle;
    bool m_bCopyAttributes;
    bool m_bOrderNumberingFirst; //#i61007# order of captions

    css::uno::Reference<css::container::XNameAccess> m_xNameAccess;

    SwCaptionPreview m_aPreview;
    std::unique_ptr<weld::Entry> m_xTextEdit;
    std::unique_ptr<weld::ComboBox> m_xCategoryBox;
    std::unique_ptr<weld::Label> m_xFormatText;
    std::unique_ptr<weld::ComboBox> m_xFormatBox;
    //#i61007# order of captions
    std::unique_ptr<weld::Label> m_xNumberingSeparatorFT;
    std::unique_ptr<weld::Entry> m_xNumberingSeparatorED;
    std::unique_ptr<weld::Label> m_xSepText;
    std::unique_ptr<weld::Entry> m_xSepEdit;
    std::unique_ptr<weld::ComboBox> m_xPosBox;
    std::unique_ptr<weld::Button> m_xOKButton;
    std::unique_ptr<weld::Button> m_xAutoCaptionButton;
    std::unique_ptr<weld::Button> m_xOptionButton;
    std::unique_ptr<weld::CustomWeld> m_xPreview;

    DECL_LINK(SelectListBoxHdl, weld::ComboBox&, void);
    DECL_LINK(ModifyEntryHdl, weld::Entry&, void);
    DECL_LINK(ModifyComboHdl, weld::ComboBox&, void);
    DECL_LINK(OptionHdl, weld::Button&, void);
    DECL_LINK(CaptionHdl, weld::Button&, void);
    DECL_LINK(OKHdl, weld::Button&, void);

    void Apply();

    void ModifyHdl();
    void DrawSample();
    void ApplyCaptionOrder(); //#i61007# order of captions

    static OUString s_aSepTextSave; // Save caption separator text
public:
    SwCaptionDialog(weld::Window* pParent, SwView& rV);
    virtual short run() override;
    virtual ~SwCaptionDialog() override;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
