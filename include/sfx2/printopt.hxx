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

#include <sal/config.h>
#include <sfx2/dllapi.h>
#include <sfx2/tabdlg.hxx>
#include <sal/types.h>
#include <vcl/printer/Options.hxx>

class SFX2_DLLPUBLIC SfxCommonPrintOptionsTabPage final : public SfxTabPage
{
private:

    std::unique_ptr<weld::RadioButton> m_xPrinterOutputRB;
    std::unique_ptr<weld::RadioButton> m_xPrintFileOutputRB;
    std::unique_ptr<weld::CheckButton> m_xReduceTransparencyCB;
    std::unique_ptr<weld::Widget> m_xReduceTransparencyImg;
    std::unique_ptr<weld::RadioButton> m_xReduceTransparencyAutoRB;
    std::unique_ptr<weld::RadioButton> m_xReduceTransparencyNoneRB;
    std::unique_ptr<weld::Widget> m_xReduceTransparencyModeImg;
    std::unique_ptr<weld::CheckButton> m_xReduceGradientsCB;
    std::unique_ptr<weld::Widget> m_xReduceGradientsImg;
    std::unique_ptr<weld::RadioButton> m_xReduceGradientsStripesRB;
    std::unique_ptr<weld::RadioButton> m_xReduceGradientsColorRB;
    std::unique_ptr<weld::Widget> m_xReduceGradientsModeImg;
    std::unique_ptr<weld::SpinButton> m_xReduceGradientsStepCountNF;
    std::unique_ptr<weld::CheckButton> m_xReduceBitmapsCB;
    std::unique_ptr<weld::Widget> m_xReduceBitmapsImg;
    std::unique_ptr<weld::RadioButton> m_xReduceBitmapsOptimalRB;
    std::unique_ptr<weld::RadioButton> m_xReduceBitmapsNormalRB;
    std::unique_ptr<weld::RadioButton> m_xReduceBitmapsResolutionRB;
    std::unique_ptr<weld::Widget> m_xReduceBitmapsModeImg;
    std::unique_ptr<weld::ComboBox> m_xReduceBitmapsResolutionLB;
    std::unique_ptr<weld::CheckButton> m_xReduceBitmapsTransparencyCB;
    std::unique_ptr<weld::Widget> m_xReduceBitmapsTransparencyImg;
    std::unique_ptr<weld::CheckButton> m_xConvertToGreyscalesCB;
    std::unique_ptr<weld::Widget> m_xConvertToGreyscalesImg;
    std::unique_ptr<weld::CheckButton> m_xPaperSizeCB;
    std::unique_ptr<weld::Widget> m_xPaperSizeImg;
    std::unique_ptr<weld::CheckButton> m_xPaperOrientationCB;
    std::unique_ptr<weld::Widget> m_xPaperOrientationImg;
    std::unique_ptr<weld::CheckButton> m_xTransparencyCB;
    std::unique_ptr<weld::Widget> m_xTransparencyImg;

private:

    vcl::printer::Options      maPrinterOptions;
    vcl::printer::Options      maPrintFileOptions;

                        DECL_DLLPRIVATE_LINK( ToggleOutputPrinterRBHdl, weld::Toggleable&, void );
                        DECL_DLLPRIVATE_LINK( ToggleOutputPrintFileRBHdl, weld::Toggleable&, void);

                        DECL_DLLPRIVATE_LINK( ClickReduceTransparencyCBHdl, weld::Toggleable&, void );
                        DECL_DLLPRIVATE_LINK( ClickReduceGradientsCBHdl, weld::Toggleable&, void );
                        DECL_DLLPRIVATE_LINK( ClickReduceBitmapsCBHdl, weld::Toggleable&, void );

                        DECL_DLLPRIVATE_LINK( ToggleReduceGradientsStripesRBHdl, weld::Toggleable&, void );
                        DECL_DLLPRIVATE_LINK( ToggleReduceBitmapsResolutionRBHdl, weld::Toggleable&, void );

    SAL_DLLPRIVATE void ImplUpdateControls( const vcl::printer::Options* pCurrentOptions );
    SAL_DLLPRIVATE void ImplSaveControls( vcl::printer::Options* pCurrentOptions );

    virtual DeactivateRC DeactivatePage( SfxItemSet* pSet ) override;

public:

    SfxCommonPrintOptionsTabPage(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rSet);
    virtual             ~SfxCommonPrintOptionsTabPage() override;

    virtual OUString GetAllStrings() override;

    virtual bool        FillItemSet( SfxItemSet* rSet ) override;
    virtual void        Reset( const SfxItemSet* rSet ) override;

    static std::unique_ptr<SfxTabPage> Create(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet*);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
