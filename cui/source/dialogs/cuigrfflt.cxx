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

#include <vcl/bitmap/BitmapMosaicFilter.hxx>
#include <vcl/bitmap/BitmapSharpenFilter.hxx>
#include <vcl/bitmap/BitmapEmbossGreyFilter.hxx>
#include <vcl/bitmap/BitmapSepiaFilter.hxx>
#include <vcl/bitmap/BitmapSmoothenFilter.hxx>
#include <vcl/bitmap/BitmapSolarizeFilter.hxx>
#include <vcl/bitmap/BitmapColorQuantizationFilter.hxx>
#include <vcl/settings.hxx>
#include <vcl/svapp.hxx>
#include <osl/diagnose.h>
#include <tools/helpers.hxx>
#include <cuigrfflt.hxx>

CuiGraphicPreviewWindow::CuiGraphicPreviewWindow()
    : mpOrigGraphic(nullptr)
    , mfScaleX(0.0)
    , mfScaleY(0.0)
{
}

void CuiGraphicPreviewWindow::SetDrawingArea(weld::DrawingArea* pDrawingArea)
{
    CustomWidgetController::SetDrawingArea(pDrawingArea);
    OutputDevice &rDevice = pDrawingArea->get_ref_device();
    maOutputSizePixel = rDevice.LogicToPixel(Size(81, 73), MapMode(MapUnit::MapAppFont));
    pDrawingArea->set_size_request(maOutputSizePixel.Width(), maOutputSizePixel.Height());
}

void CuiGraphicPreviewWindow::Paint(vcl::RenderContext& rRenderContext, const ::tools::Rectangle&)
{
    rRenderContext.SetBackground(Wallpaper(Application::GetSettings().GetStyleSettings().GetDialogColor()));
    rRenderContext.Erase();

    const Size aOutputSize(GetOutputSizePixel());

    if (maPreview.IsAnimated())
    {
        const Size aGraphicSize(rRenderContext.LogicToPixel(maPreview.GetPrefSize(), maPreview.GetPrefMapMode()));
        const Point aGraphicPosition((aOutputSize.Width()  - aGraphicSize.Width()  ) >> 1,
                                     (aOutputSize.Height() - aGraphicSize.Height() ) >> 1);
        maPreview.StartAnimation(rRenderContext, aGraphicPosition, aGraphicSize);
    }
    else
    {
        const Size  aGraphicSize(maPreview.GetSizePixel());
        const Point aGraphicPosition((aOutputSize.Width()  - aGraphicSize.Width())  >> 1,
                                     (aOutputSize.Height() - aGraphicSize.Height()) >> 1);
        maPreview.Draw(rRenderContext, aGraphicPosition, aGraphicSize);
    }
}

void CuiGraphicPreviewWindow::SetPreview(const Graphic& rGraphic)
{
    maPreview = rGraphic;
    Invalidate();
}

void CuiGraphicPreviewWindow::ScaleImageToFit()
{
    if (!mpOrigGraphic)
        return;

    maScaledOrig = *mpOrigGraphic;

    const Size aPreviewSize(GetOutputSizePixel());
    Size aGrfSize(maOrigGraphicSizePixel);

    if( mpOrigGraphic->GetType() == GraphicType::Bitmap &&
        aPreviewSize.Width() && aPreviewSize.Height() &&
        aGrfSize.Width() && aGrfSize.Height() )
    {
        const double fGrfWH = static_cast<double>(aGrfSize.Width()) / aGrfSize.Height();
        const double fPreWH = static_cast<double>(aPreviewSize.Width()) / aPreviewSize.Height();

        if( fGrfWH < fPreWH )
        {
            aGrfSize.setWidth( static_cast<tools::Long>( aPreviewSize.Height() * fGrfWH ) );
            aGrfSize.setHeight( aPreviewSize.Height() );
        }
        else
        {
            aGrfSize.setWidth( aPreviewSize.Width() );
            aGrfSize.setHeight( static_cast<tools::Long>( aPreviewSize.Width() / fGrfWH ) );
        }

        mfScaleX = static_cast<double>(aGrfSize.Width()) / maOrigGraphicSizePixel.Width();
        mfScaleY = static_cast<double>(aGrfSize.Height()) / maOrigGraphicSizePixel.Height();

        if( !mpOrigGraphic->IsAnimated() )
        {
            BitmapEx aBmpEx( mpOrigGraphic->GetBitmapEx() );

            if( aBmpEx.Scale( aGrfSize ) )
                maScaledOrig = aBmpEx;
        }
    }

    maModifyHdl.Call(nullptr);
}

void CuiGraphicPreviewWindow::Resize()
{
    maOutputSizePixel = GetOutputSizePixel();
    ScaleImageToFit();
}

GraphicFilterDialog::GraphicFilterDialog(weld::Window* pParent,
    const OUString& rUIXMLDescription, const OUString& rID,
    const Graphic& rGraphic)
    : GenericDialogController(pParent, rUIXMLDescription, rID)
    , maTimer("cui GraphicFilterDialog maTimer")
    , maModifyHdl(LINK(this, GraphicFilterDialog, ImplModifyHdl))
    , mxPreview(new weld::CustomWeld(*m_xBuilder, u"preview"_ustr, maPreview))
{
    bIsBitmap = rGraphic.GetType() == GraphicType::Bitmap;

    maTimer.SetInvokeHandler(LINK(this, GraphicFilterDialog, ImplPreviewTimeoutHdl));
    maTimer.SetTimeout(5);

    maPreview.init(&rGraphic, maModifyHdl);
}

IMPL_LINK_NOARG(GraphicFilterDialog, ImplPreviewTimeoutHdl, Timer *, void)
{
    maTimer.Stop();
    maPreview.SetPreview(GetFilteredGraphic(maPreview.GetScaledOriginal(),
        maPreview.GetScaleX(), maPreview.GetScaleY()));
}

IMPL_LINK_NOARG(GraphicFilterDialog, ImplModifyHdl, LinkParamNone*, void)
{
    if (bIsBitmap)
    {
        maTimer.Stop();
        maTimer.Start();
    }
}

GraphicFilterMosaic::GraphicFilterMosaic(weld::Window* pParent, const Graphic& rGraphic,
                                         sal_uInt16 nTileWidth, sal_uInt16 nTileHeight, bool bEnhanceEdges)
    : GraphicFilterDialog(pParent, u"cui/ui/mosaicdialog.ui"_ustr, u"MosaicDialog"_ustr, rGraphic)
    , mxMtrWidth(m_xBuilder->weld_metric_spin_button(u"width"_ustr, FieldUnit::PIXEL))
    , mxMtrHeight(m_xBuilder->weld_metric_spin_button(u"height"_ustr, FieldUnit::PIXEL))
    , mxCbxEdges(m_xBuilder->weld_check_button(u"edges"_ustr))
{
    mxMtrWidth->set_value(nTileWidth, FieldUnit::PIXEL);
    mxMtrWidth->set_max(GetGraphicSizePixel().Width(), FieldUnit::PIXEL);
    mxMtrWidth->connect_value_changed(LINK(this, GraphicFilterMosaic, EditModifyHdl));

    mxMtrHeight->set_value(nTileHeight, FieldUnit::PIXEL);
    mxMtrHeight->set_max(GetGraphicSizePixel().Height(), FieldUnit::PIXEL);
    mxMtrHeight->connect_value_changed(LINK(this, GraphicFilterMosaic, EditModifyHdl));

    mxCbxEdges->set_active(bEnhanceEdges);
    mxCbxEdges->connect_toggled(LINK(this, GraphicFilterMosaic, CheckBoxModifyHdl));

    mxMtrWidth->grab_focus();
}

IMPL_LINK_NOARG(GraphicFilterMosaic, CheckBoxModifyHdl, weld::Toggleable&, void)
{
    GetModifyHdl().Call(nullptr);
}

IMPL_LINK_NOARG(GraphicFilterMosaic, EditModifyHdl, weld::MetricSpinButton&, void)
{
    GetModifyHdl().Call(nullptr);
}

Graphic GraphicFilterMosaic::GetFilteredGraphic( const Graphic& rGraphic,
                                                 double fScaleX, double fScaleY )
{
    Graphic         aRet;
    tools::Long            nTileWidth = static_cast<tools::Long>(mxMtrWidth->get_value(FieldUnit::PIXEL));
    tools::Long            nTileHeight = static_cast<tools::Long>(mxMtrHeight->get_value(FieldUnit::PIXEL));
    const Size      aSize( std::max( basegfx::fround<tools::Long>( nTileWidth * fScaleX ), tools::Long(1) ),
                           std::max( basegfx::fround<tools::Long>( nTileHeight * fScaleY ), tools::Long(1) ) );

    if( rGraphic.IsAnimated() )
    {
        Animation aAnim( rGraphic.GetAnimation() );

        if (BitmapFilter::Filter(aAnim, BitmapMosaicFilter(aSize.getWidth(), aSize.getHeight())))
        {
            if( IsEnhanceEdges() )
                (void)BitmapFilter::Filter(aAnim, BitmapSharpenFilter());

            aRet = aAnim;
        }
    }
    else
    {
        BitmapEx aBmpEx( rGraphic.GetBitmapEx() );

        if (BitmapFilter::Filter(aBmpEx, BitmapMosaicFilter(aSize.getWidth(), aSize.getHeight())))
        {
            if( IsEnhanceEdges() )
                BitmapFilter::Filter(aBmpEx, BitmapSharpenFilter());

            aRet = aBmpEx;
        }
    }

    return aRet;
}

GraphicFilterSmooth::GraphicFilterSmooth(weld::Window* pParent, const Graphic& rGraphic, double nRadius)
    : GraphicFilterDialog(pParent, u"cui/ui/smoothdialog.ui"_ustr, u"SmoothDialog"_ustr, rGraphic)
    , mxMtrRadius(m_xBuilder->weld_spin_button(u"radius"_ustr))
{
    mxMtrRadius->set_value(nRadius * 10);
    mxMtrRadius->connect_value_changed(LINK(this, GraphicFilterSmooth, EditModifyHdl));
    mxMtrRadius->grab_focus();
}

IMPL_LINK_NOARG(GraphicFilterSmooth, EditModifyHdl, weld::SpinButton&, void)
{
    GetModifyHdl().Call(nullptr);
}

Graphic GraphicFilterSmooth::GetFilteredGraphic( const Graphic& rGraphic, double, double )
{
    Graphic         aRet;
    double          nRadius = mxMtrRadius->get_value() / 10.0;

    if( rGraphic.IsAnimated() )
    {
        Animation aAnim( rGraphic.GetAnimation() );

        if (BitmapFilter::Filter(aAnim, BitmapSmoothenFilter(nRadius)))
        {
            aRet = aAnim;
        }
    }
    else
    {
        BitmapEx aBmpEx( rGraphic.GetBitmapEx() );

        if (BitmapFilter::Filter(aBmpEx, BitmapSmoothenFilter(nRadius)))
        {
            aRet = aBmpEx;
        }
    }

    return aRet;
}

GraphicFilterSolarize::GraphicFilterSolarize(weld::Window* pParent, const Graphic& rGraphic,
                                             sal_uInt8 cGreyThreshold, bool bInvert)
    : GraphicFilterDialog(pParent, u"cui/ui/solarizedialog.ui"_ustr, u"SolarizeDialog"_ustr, rGraphic)
    , mxMtrThreshold(m_xBuilder->weld_metric_spin_button(u"value"_ustr, FieldUnit::PERCENT))
    , mxCbxInvert(m_xBuilder->weld_check_button(u"invert"_ustr))
{
    mxMtrThreshold->set_value(basegfx::fround(cGreyThreshold / 2.55), FieldUnit::PERCENT);
    mxMtrThreshold->connect_value_changed(LINK(this, GraphicFilterSolarize, EditModifyHdl));

    mxCbxInvert->set_active(bInvert);
    mxCbxInvert->connect_toggled(LINK(this, GraphicFilterSolarize, CheckBoxModifyHdl));
}

IMPL_LINK_NOARG(GraphicFilterSolarize, CheckBoxModifyHdl, weld::Toggleable&, void)
{
    GetModifyHdl().Call(nullptr);
}

IMPL_LINK_NOARG(GraphicFilterSolarize, EditModifyHdl, weld::MetricSpinButton&, void)
{
    GetModifyHdl().Call(nullptr);
}

Graphic GraphicFilterSolarize::GetFilteredGraphic( const Graphic& rGraphic, double, double )
{
    Graphic         aRet;
    sal_uInt8       nGreyThreshold = basegfx::fround<sal_uInt8>(mxMtrThreshold->get_value(FieldUnit::PERCENT) * 2.55);

    if( rGraphic.IsAnimated() )
    {
        Animation aAnim( rGraphic.GetAnimation() );

        if (BitmapFilter::Filter(aAnim, BitmapSolarizeFilter(nGreyThreshold)))
        {
            if( IsInvert() )
                aAnim.Invert();

            aRet = aAnim;
        }
    }
    else
    {
        BitmapEx aBmpEx( rGraphic.GetBitmapEx() );

        if (BitmapFilter::Filter(aBmpEx, BitmapSolarizeFilter(nGreyThreshold)))
        {
            if( IsInvert() )
                aBmpEx.Invert();

            aRet = aBmpEx;
        }
    }

    return aRet;
}

GraphicFilterSepia::GraphicFilterSepia(weld::Window* pParent, const Graphic& rGraphic,
                                       sal_uInt16 nSepiaPercent)
    : GraphicFilterDialog(pParent, u"cui/ui/agingdialog.ui"_ustr, u"AgingDialog"_ustr, rGraphic)
    , mxMtrSepia(m_xBuilder->weld_metric_spin_button(u"value"_ustr, FieldUnit::PERCENT))
{
    mxMtrSepia->set_value(nSepiaPercent, FieldUnit::PERCENT);
    mxMtrSepia->connect_value_changed(LINK(this, GraphicFilterSepia, EditModifyHdl));
}

IMPL_LINK_NOARG(GraphicFilterSepia, EditModifyHdl, weld::MetricSpinButton&, void)
{
    GetModifyHdl().Call(nullptr);
}

Graphic GraphicFilterSepia::GetFilteredGraphic( const Graphic& rGraphic, double, double )
{
    Graphic         aRet;
    sal_uInt16      nSepiaPct = sal::static_int_cast< sal_uInt16 >(mxMtrSepia->get_value(FieldUnit::PERCENT));

    if( rGraphic.IsAnimated() )
    {
        Animation aAnim( rGraphic.GetAnimation() );

        if (BitmapFilter::Filter(aAnim, BitmapSepiaFilter(nSepiaPct)))
            aRet = aAnim;
    }
    else
    {
        BitmapEx aBmpEx( rGraphic.GetBitmapEx() );

        if (BitmapFilter::Filter(aBmpEx, BitmapSepiaFilter(nSepiaPct)))
            aRet = aBmpEx;
    }

    return aRet;
}

GraphicFilterPoster::GraphicFilterPoster(weld::Window* pParent, const Graphic& rGraphic,
                                          sal_uInt16 nPosterCount)
    : GraphicFilterDialog(pParent, u"cui/ui/posterdialog.ui"_ustr, u"PosterDialog"_ustr, rGraphic)
    , mxNumPoster(m_xBuilder->weld_spin_button(u"value"_ustr))
{
    mxNumPoster->set_range(2, vcl::pixelFormatBitCount(rGraphic.GetBitmapEx().getPixelFormat()));
    mxNumPoster->set_value(nPosterCount);
    mxNumPoster->connect_value_changed(LINK(this, GraphicFilterPoster, EditModifyHdl));
}

IMPL_LINK_NOARG(GraphicFilterPoster, EditModifyHdl, weld::SpinButton&, void)
{
    GetModifyHdl().Call(nullptr);
}

Graphic GraphicFilterPoster::GetFilteredGraphic( const Graphic& rGraphic, double, double )
{
    Graphic          aRet;
    const sal_uInt16 nPosterCount = static_cast<sal_uInt16>(mxNumPoster->get_value());

    if( rGraphic.IsAnimated() )
    {
        Animation aAnim( rGraphic.GetAnimation() );

        if( aAnim.ReduceColors( nPosterCount ) )
            aRet = aAnim;
    }
    else
    {
        BitmapEx aBmpEx( rGraphic.GetBitmapEx() );

        if (BitmapFilter::Filter(aBmpEx, BitmapColorQuantizationFilter(nPosterCount)))
            aRet = aBmpEx;
    }

    return aRet;
}

bool EmbossControl::MouseButtonDown( const MouseEvent& rEvt )
{
    const RectPoint eOldRP = GetActualRP();

    SvxRectCtl::MouseButtonDown( rEvt );

    if( GetActualRP() != eOldRP )
        maModifyHdl.Call( nullptr );

    return true;
}

void EmbossControl::SetDrawingArea(weld::DrawingArea* pDrawingArea)
{
    SvxRectCtl::SetDrawingArea(pDrawingArea);
    Size aSize(pDrawingArea->get_ref_device().LogicToPixel(Size(77, 60), MapMode(MapUnit::MapAppFont)));
    pDrawingArea->set_size_request(aSize.Width(), aSize.Height());
}

GraphicFilterEmboss::GraphicFilterEmboss(weld::Window* pParent,
    const Graphic& rGraphic, RectPoint eLightSource)
    : GraphicFilterDialog(pParent, u"cui/ui/embossdialog.ui"_ustr, u"EmbossDialog"_ustr, rGraphic)
    , mxCtlLight(new weld::CustomWeld(*m_xBuilder, u"lightsource"_ustr, maCtlLight))
{
    maCtlLight.SetActualRP(eLightSource);
    maCtlLight.SetModifyHdl( GetModifyHdl() );
    maCtlLight.GrabFocus();
}

GraphicFilterEmboss::~GraphicFilterEmboss()
{
}

Graphic GraphicFilterEmboss::GetFilteredGraphic( const Graphic& rGraphic, double, double )
{
    Graphic aRet;
    Degree100  nAzim, nElev;

    switch (maCtlLight.GetActualRP())
    {
        default:       OSL_FAIL("svx::GraphicFilterEmboss::GetFilteredGraphic(), unknown Reference Point!" );
                       [[fallthrough]];
        case RectPoint::LT: nAzim = 4500_deg100;    nElev = 4500_deg100; break;
        case RectPoint::MT: nAzim = 9000_deg100;    nElev = 4500_deg100; break;
        case RectPoint::RT: nAzim = 13500_deg100;   nElev = 4500_deg100; break;
        case RectPoint::LM: nAzim = 0_deg100;       nElev = 4500_deg100; break;
        case RectPoint::MM: nAzim = 0_deg100;       nElev = 9000_deg100; break;
        case RectPoint::RM: nAzim = 18000_deg100;   nElev = 4500_deg100; break;
        case RectPoint::LB: nAzim = 31500_deg100;   nElev = 4500_deg100; break;
        case RectPoint::MB: nAzim = 27000_deg100;   nElev = 4500_deg100; break;
        case RectPoint::RB: nAzim = 22500_deg100;   nElev = 4500_deg100; break;
    }

    if( rGraphic.IsAnimated() )
    {
        Animation aAnim( rGraphic.GetAnimation() );

        if (BitmapFilter::Filter(aAnim, BitmapEmbossGreyFilter(nAzim, nElev)))
            aRet = aAnim;
    }
    else
    {
        BitmapEx aBmpEx( rGraphic.GetBitmapEx() );

        if (BitmapFilter::Filter(aBmpEx, BitmapEmbossGreyFilter(nAzim, nElev)))
            aRet = aBmpEx;
    }

    return aRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
