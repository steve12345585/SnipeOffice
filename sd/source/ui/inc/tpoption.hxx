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
#include <svx/optgrid.hxx>

/**
 * Option-Tab-Page: Snap
 */
class SdTpOptionsSnap final : public SvxGridTabPage
{
public:
    SdTpOptionsSnap(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rInAttrs);
    static std::unique_ptr<SfxTabPage> Create( weld::Container* pPage, weld::DialogController* pController, const SfxItemSet* );
    virtual ~SdTpOptionsSnap() override;

    virtual bool FillItemSet( SfxItemSet* ) override;
    virtual void Reset( const SfxItemSet * ) override;
};

/**
 * Option-Tab-Page: Contents
 */
class SdTpOptionsContents final : public SfxTabPage
{
private:
    bool m_bDrawMode;
    std::unique_ptr<weld::CheckButton> m_xCbxRuler;
    std::unique_ptr<weld::Widget> m_xCbxRulerImg;
    std::unique_ptr<weld::CheckButton> m_xCbxDragStripes;
    std::unique_ptr<weld::Widget> m_xCbxDragStripesImg;
    std::unique_ptr<weld::CheckButton> m_xCbxHandlesBezier;
    std::unique_ptr<weld::Widget> m_xCbxHandlesBezierImg;
    std::unique_ptr<weld::CheckButton> m_xCbxMoveOutline;
    std::unique_ptr<weld::Widget> m_xCbxMoveOutlineImg;

public:
    SdTpOptionsContents(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rInAttrs);
    static  std::unique_ptr<SfxTabPage> Create( weld::Container* pPage, weld::DialogController* pController, const SfxItemSet* );
    virtual ~SdTpOptionsContents() override;

    virtual OUString GetAllStrings() override;

    virtual bool FillItemSet( SfxItemSet* ) override;
    virtual void Reset( const SfxItemSet * ) override;

    virtual void PageCreated(const SfxAllItemSet& aSet) override;

    void SetDrawMode() { m_bDrawMode = true; }
};

/**
 * Option-Tab-Page: View
 */

class SdTpOptionsMisc final : public SfxTabPage
{
 friend class SdModule;

private:
    sal_uInt32          nWidth;
    sal_uInt32          nHeight;
    OUString aInfo1;
    OUString aInfo2;

    bool                m_bDrawMode;
    MapUnit             ePoolUnit;

    std::unique_ptr<weld::CheckButton> m_xCbxQuickEdit;
    std::unique_ptr<weld::Widget> m_xCbxQuickEditImg;
    std::unique_ptr<weld::CheckButton> m_xCbxPickThrough;
    std::unique_ptr<weld::Widget> m_xCbxPickThroughImg;

    std::unique_ptr<weld::Frame> m_xNewDocumentFrame;
    std::unique_ptr<weld::CheckButton> m_xCbxStartWithTemplate;
    std::unique_ptr<weld::Widget> m_xCbxStartWithTemplateImg;

    std::unique_ptr<weld::CheckButton> m_xCbxMasterPageCache;
    std::unique_ptr<weld::Widget> m_xCbxMasterPageCacheImg;
    std::unique_ptr<weld::CheckButton> m_xCbxCopy;
    std::unique_ptr<weld::Widget> m_xCbxCopyImg;
    std::unique_ptr<weld::CheckButton> m_xCbxMarkedHitMovesAlways;
    std::unique_ptr<weld::Widget> m_xCbxMarkedHitMovesAlwaysImg;

    std::unique_ptr<weld::ComboBox> m_xLbMetric;
    std::unique_ptr<weld::Widget> m_xLbMetricImg;
    std::unique_ptr<weld::MetricSpinButton> m_xMtrFldTabstop;
    std::unique_ptr<weld::Widget> m_xMtrFldTabstopImg;

    std::unique_ptr<weld::CheckButton> m_xCbxCompatibility;
    std::unique_ptr<weld::Widget> m_xCbxCompatibilityImg;

    //Scale
    std::unique_ptr<weld::Frame> m_xScaleFrame;
    std::unique_ptr<weld::ComboBox> m_xCbScale;
    std::unique_ptr<weld::Widget> m_xCbScaleImg;
    std::unique_ptr<weld::Label> m_xNewDocLb;
    std::unique_ptr<weld::Label> m_xFiInfo1;
    std::unique_ptr<weld::MetricSpinButton> m_xMtrFldOriginalWidth;
    std::unique_ptr<weld::Label> m_xWidthLb;
    std::unique_ptr<weld::Label> m_xHeightLb;
    std::unique_ptr<weld::Label> m_xFiInfo2;
    std::unique_ptr<weld::MetricSpinButton> m_xMtrFldOriginalHeight;
    std::unique_ptr<weld::CheckButton> m_xCbxDistort;
    std::unique_ptr<weld::Widget> m_xCbxDistortImg;
    std::unique_ptr<weld::MetricSpinButton> m_xMtrFldInfo1;
    std::unique_ptr<weld::MetricSpinButton> m_xMtrFldInfo2;

    static OUString        GetScale( sal_Int32 nX, sal_Int32 nY );
    static bool            SetScale( std::u16string_view aScale, sal_Int32& rX, sal_Int32& rY );

    DECL_LINK( SelectMetricHdl_Impl, weld::ComboBox&, void );

    /** Enable or disable the controls in the compatibility section of the
        'general' tab page depending on whether there is at least one
        document.
    */
    void UpdateCompatibilityControls();

    virtual void ActivatePage( const SfxItemSet& rSet ) override;
    virtual DeactivateRC DeactivatePage( SfxItemSet* pSet ) override;

public:
    SdTpOptionsMisc(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rInAttrs);
    static std::unique_ptr<SfxTabPage> Create( weld::Container* pPage, weld::DialogController* pController, const SfxItemSet* );
    virtual ~SdTpOptionsMisc() override;

    virtual OUString GetAllStrings() override;

    virtual bool FillItemSet( SfxItemSet* ) override;
    virtual void Reset( const SfxItemSet * ) override;

    /** Hide Impress specific controls, make Draw specific controls visible
        and arrange the visible controls.  Do not call this method more than once.
    */
    void SetDrawMode();

    virtual void        PageCreated(const SfxAllItemSet& aSet) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
