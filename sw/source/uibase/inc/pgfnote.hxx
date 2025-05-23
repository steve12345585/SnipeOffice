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

#include <svtools/ctrlbox.hxx>
#include <svx/colorbox.hxx>

// footnote settings TabPage
class SwFootNotePage final : public SfxTabPage
{
    static const WhichRangesContainer s_aPageRg;
public:
    SwFootNotePage(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet &rSet);
    static std::unique_ptr<SfxTabPage> Create(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet *rSet);
    virtual ~SwFootNotePage() override;

    static const WhichRangesContainer & GetRanges() { return s_aPageRg; }

    virtual bool FillItemSet(SfxItemSet *rSet) override;
    virtual void Reset(const SfxItemSet *rSet) override;

private:

    tools::Long            m_lMaxHeight;

    std::unique_ptr<weld::RadioButton> m_xMaxHeightPageBtn;
    std::unique_ptr<weld::RadioButton> m_xMaxHeightBtn;
    std::unique_ptr<weld::MetricSpinButton> m_xMaxHeightEdit;
    std::unique_ptr<weld::Label> m_xDistLabel;
    std::unique_ptr<weld::MetricSpinButton> m_xDistEdit;
    std::unique_ptr<weld::Label> m_xLinePosLabel;
    std::unique_ptr<weld::ComboBox> m_xLinePosBox;
    std::unique_ptr<SvtLineListBox> m_xLineTypeBox;
    std::unique_ptr<weld::MetricSpinButton> m_xLineWidthEdit;
    std::unique_ptr<ColorListBox> m_xLineColorBox;
    std::unique_ptr<weld::Label> m_xLineLengthLabel;
    std::unique_ptr<weld::MetricSpinButton> m_xLineLengthEdit;
    std::unique_ptr<weld::Label> m_xLineDistLabel;
    std::unique_ptr<weld::MetricSpinButton> m_xLineDistEdit;

    DECL_LINK(HeightPage, weld::Toggleable&, void);
    DECL_LINK(HeightMetric, weld::Toggleable&, void);
    DECL_LINK(HeightModify, weld::MetricSpinButton&, void);
    DECL_LINK(LineWidthChanged_Impl, weld::MetricSpinButton&, void);
    DECL_LINK(LineColorSelected_Impl, ColorListBox&, void);

    virtual void    ActivatePage( const SfxItemSet& rSet ) override;
    virtual DeactivateRC   DeactivatePage( SfxItemSet* pSet ) override;

};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
