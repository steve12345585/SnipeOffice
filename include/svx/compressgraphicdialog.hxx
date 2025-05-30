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
#ifndef INCLUDED_SVX_COMPRESSGRAPHICDIALOG_HXX
#define INCLUDED_SVX_COMPRESSGRAPHICDIALOG_HXX

#include <sal/types.h>
#include <svx/svxdllapi.h>
#include <tools/gen.hxx>
#include <tools/link.hxx>
#include <vcl/weld.hxx>
#include <vcl/graph.hxx>

class SdrGrafObj;
class SfxBindings;
class SvStream;

class SAL_WARN_UNUSED SVX_DLLPUBLIC CompressGraphicsDialog final : public weld::GenericDialogController
{
private:
    std::unique_ptr<weld::Label>        m_xLabelGraphicType;
    std::unique_ptr<weld::Label>        m_xFixedText2;
    std::unique_ptr<weld::Label>        m_xFixedText3;
    std::unique_ptr<weld::Label>        m_xFixedText5;
    std::unique_ptr<weld::Label>        m_xFixedText6;

    std::unique_ptr<weld::CheckButton>  m_xReduceResolutionCB;
    std::unique_ptr<weld::SpinButton>   m_xMFNewWidth;
    std::unique_ptr<weld::SpinButton>   m_xMFNewHeight;
    std::unique_ptr<weld::ComboBox> m_xResolutionLB;
    std::unique_ptr<weld::RadioButton>  m_xLosslessRB;
    std::unique_ptr<weld::RadioButton>  m_xJpegCompRB;
    std::unique_ptr<weld::SpinButton>   m_xCompressionMF;
    std::unique_ptr<weld::Scale>        m_xCompressionSlider;
    std::unique_ptr<weld::SpinButton>   m_xQualityMF;
    std::unique_ptr<weld::Scale>        m_xQualitySlider;
    std::unique_ptr<weld::Button>       m_xBtnCalculate;
    std::unique_ptr<weld::ComboBox> m_xInterpolationCombo;
    std::unique_ptr<weld::Button>       m_xBtnOkay;

    SdrGrafObj*     m_xGraphicObj;
    Graphic         m_aGraphic;
    Size            m_aViewSize100mm;
    tools::Rectangle m_aCropRectangle;
    SfxBindings&    m_rBindings;

    double          m_dResolution;
    sal_Int32       m_aNativeSize;

    void Initialize();
    void recallParameter();

    DECL_DLLPRIVATE_LINK( SlideHdl, weld::Scale&, void );
    DECL_DLLPRIVATE_LINK( NewInterpolationModifiedHdl, weld::ComboBox&, void );
    DECL_DLLPRIVATE_LINK( NewQualityModifiedHdl, weld::SpinButton&, void );
    DECL_DLLPRIVATE_LINK( NewCompressionModifiedHdl, weld::SpinButton&, void );
    DECL_DLLPRIVATE_LINK( NewWidthModifiedHdl, weld::SpinButton&, void );
    DECL_DLLPRIVATE_LINK( NewHeightModifiedHdl, weld::SpinButton&, void );
    DECL_DLLPRIVATE_LINK( ResolutionModifiedHdl, weld::ComboBox&, void );
    DECL_DLLPRIVATE_LINK( ToggleCompressionRB, weld::Toggleable&, void );
    DECL_DLLPRIVATE_LINK( ToggleReduceResolutionRB, weld::Toggleable&, void );

    DECL_DLLPRIVATE_LINK( CalculateClickHdl, weld::Button&, void );
    DECL_DLLPRIVATE_LINK( OkayClickHdl, weld::Button&, void );

    void Update();
    void UpdateSensitivity(const bool bSensitive);
    void UpdateNewWidthMF();
    void UpdateNewHeightMF();
    void UpdateResolutionLB();

    void Compress(SvStream& aStream);

    double GetViewWidthInch() const;
    double GetViewHeightInch() const;

    BmpScaleFlag GetSelectedInterpolationType() const;

public:
    CompressGraphicsDialog( weld::Window* pParent, SdrGrafObj* pGraphicObj, SfxBindings& rBindings );
    CompressGraphicsDialog( weld::Window* pParent, Graphic aGraphic, Size aViewSize100mm, tools::Rectangle const & rCropRectangle, SfxBindings& rBindings );
    virtual ~CompressGraphicsDialog() override;

    rtl::Reference<SdrGrafObj> GetCompressedSdrGrafObj();
    Graphic GetCompressedGraphic();

    tools::Rectangle GetScaledCropRectangle() const;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
