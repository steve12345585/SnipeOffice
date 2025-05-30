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

#include <sfx2/tabdlg.hxx>
#include <svtools/ctrltool.hxx>
#include <svx/srchdlg.hxx>
#include <vcl/weld.hxx>

class SvxSearchFormatDialog : public SfxTabDialogController
{
public:
    SvxSearchFormatDialog(weld::Window* pParent, const SfxItemSet& rSet);
    virtual ~SvxSearchFormatDialog() override;

protected:
    virtual void PageCreated(const OUString& rId, SfxTabPage &rPage) override;

private:
    std::unique_ptr<FontList> m_pFontList;
};

// class SvxSearchFormatDialog -------------------------------------------

class SvxSearchAttributeDialog : public weld::GenericDialogController
{
public:
    SvxSearchAttributeDialog(weld::Window* pParent, SearchAttrItemList& rLst,
                             const WhichRangesContainer& pWhRanges);
    virtual ~SvxSearchAttributeDialog() override;

private:
    SearchAttrItemList& rList;

    std::unique_ptr<weld::TreeView> m_xAttrLB;
    std::unique_ptr<weld::Button> m_xOKBtn;

    DECL_LINK(OKHdl, weld::Button&, void);
};

// class SvxSearchSimilarityDialog ---------------------------------------

class SvxSearchSimilarityDialog : public weld::GenericDialogController
{
private:
    std::unique_ptr<weld::SpinButton> m_xOtherFld;
    std::unique_ptr<weld::SpinButton> m_xLongerFld;
    std::unique_ptr<weld::SpinButton> m_xShorterFld;
    std::unique_ptr<weld::CheckButton> m_xRelaxBox;

public:
    SvxSearchSimilarityDialog(weld::Window* pParent,
                              bool bRelax,
                              sal_uInt16 nOther,
                              sal_uInt16 nShorter,
                              sal_uInt16 nLonger);
    virtual ~SvxSearchSimilarityDialog() override;

    sal_uInt16  GetOther() const      { return static_cast<sal_uInt16>(m_xOtherFld->get_value()); }
    sal_uInt16  GetShorter() const    { return static_cast<sal_uInt16>(m_xShorterFld->get_value()); }
    sal_uInt16  GetLonger() const     { return static_cast<sal_uInt16>(m_xLongerFld->get_value()); }
    bool        IsRelaxed() const     { return m_xRelaxBox->get_active(); }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
