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

#include <memory>
#include <stdlib.h>
#include <tools/urlobj.hxx>
#include <svx/drawitem.hxx>
#include <svx/xbtmpit.hxx>
#include <svx/svxids.hrc>
#include <strings.hrc>
#include <svx/xfillit0.hxx>
#include <svx/xtable.hxx>
#include <svx/xflbmsxy.hxx>
#include <svx/xflbmtit.hxx>
#include <svx/xflbstit.hxx>
#include <svx/xflbmsli.hxx>
#include <svx/xflbmpit.hxx>
#include <svx/xflboxy.hxx>
#include <svx/xflbtoxy.hxx>
#include <cuitabarea.hxx>
#include <dialmgr.hxx>
#include <svx/dlgutil.hxx>
#include <svl/intitem.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/opengrf.hxx>
#include <sfx2/viewfrm.hxx>
#include <vcl/image.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <svx/svxdlg.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/dialoghelper.hxx>
#include <sal/log.hxx>
#include <comphelper/lok.hxx>
#include <svtools/unitconv.hxx>

using namespace com::sun::star;

namespace {

enum BitmapStyle
{
    CUSTOM,
    TILED,
    STRETCHED
};

enum TileOffset
{
    ROW,
    COLUMN
};

}

SvxBitmapTabPage::SvxBitmapTabPage(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rInAttrs)
    : SfxTabPage(pPage, pController, u"cui/ui/imagetabpage.ui"_ustr, u"ImageTabPage"_ustr, &rInAttrs)
    , m_rOutAttrs(rInAttrs)
    , m_nBitmapListState(ChangeType::NONE)
    , m_fObjectWidth(0.0)
    , m_fObjectHeight(0.0)
    , m_bLogicalSize(false)
    , m_aXFillAttr(rInAttrs.GetPool())
    , m_rXFSet(m_aXFillAttr.GetItemSet())
    , mpView(nullptr)
    , m_xBitmapLB(new SvxPresetListBoxValueSet(m_xBuilder->weld_scrolled_window(u"imagewin"_ustr, true)))
    , m_xBitmapStyleLB(m_xBuilder->weld_combo_box(u"imagestyle"_ustr))
    , m_xSizeBox(m_xBuilder->weld_container(u"sizebox"_ustr))
    , m_xTsbScale(m_xBuilder->weld_check_button(u"scaletsb"_ustr))
    , m_xBitmapWidth(m_xBuilder->weld_metric_spin_button(u"width"_ustr, FieldUnit::PERCENT))
    , m_xBitmapHeight(m_xBuilder->weld_metric_spin_button(u"height"_ustr, FieldUnit::PERCENT))
    , m_xPositionBox(m_xBuilder->weld_container(u"posbox"_ustr))
    , m_xPositionLB(m_xBuilder->weld_combo_box(u"positionlb"_ustr))
    , m_xPositionOffBox(m_xBuilder->weld_container(u"posoffbox"_ustr))
    , m_xPositionOffX(m_xBuilder->weld_metric_spin_button(u"posoffx"_ustr, FieldUnit::PERCENT))
    , m_xPositionOffY(m_xBuilder->weld_metric_spin_button(u"posoffy"_ustr, FieldUnit::PERCENT))
    , m_xTileOffBox(m_xBuilder->weld_container(u"tileoffbox"_ustr))
    , m_xTileOffLB(m_xBuilder->weld_combo_box(u"tileofflb"_ustr))
    , m_xTileOffset(m_xBuilder->weld_metric_spin_button(u"tileoffmtr"_ustr, FieldUnit::PERCENT))
    , m_xBtnImport(m_xBuilder->weld_button(u"BTN_IMPORT"_ustr))
    , m_xCtlBitmapPreview(new weld::CustomWeld(*m_xBuilder, u"CTL_IMAGE_PREVIEW"_ustr, m_aCtlBitmapPreview))
    , m_xBitmapLBWin(new weld::CustomWeld(*m_xBuilder, u"IMAGE"_ustr, *m_xBitmapLB))
{
    // setting the output device
    m_rXFSet.Put( XFillStyleItem(drawing::FillStyle_BITMAP) );
    m_rXFSet.Put( XFillBitmapItem(OUString(), Graphic()) );
    m_aCtlBitmapPreview.SetAttributes( m_aXFillAttr.GetItemSet() );

    m_xBitmapLB->SetSelectHdl( LINK(this, SvxBitmapTabPage, ModifyBitmapHdl) );
    m_xBitmapLB->SetRenameHdl( LINK(this, SvxBitmapTabPage, ClickRenameHdl) );
    m_xBitmapLB->SetDeleteHdl( LINK(this, SvxBitmapTabPage, ClickDeleteHdl) );
    m_xBitmapLB->SetDialog(this);
    m_xBitmapStyleLB->connect_changed( LINK(this, SvxBitmapTabPage, ModifyBitmapStyleHdl) );
    Link<weld::MetricSpinButton&, void> aLink1( LINK(this, SvxBitmapTabPage, ModifyBitmapSizeHdl) );
    m_xBitmapWidth->connect_value_changed( aLink1 );
    m_xBitmapHeight->connect_value_changed( aLink1 );
    m_xTsbScale->connect_toggled(LINK(this, SvxBitmapTabPage, ClickScaleHdl));
    m_xPositionLB->connect_changed( LINK( this, SvxBitmapTabPage, ModifyBitmapPositionHdl ) );
    Link<weld::MetricSpinButton&, void> aLink( LINK( this, SvxBitmapTabPage, ModifyPositionOffsetHdl ) );
    m_xPositionOffX->connect_value_changed(aLink);
    m_xPositionOffY->connect_value_changed(aLink);
    m_xTileOffset->connect_value_changed( LINK( this, SvxBitmapTabPage, ModifyTileOffsetHdl ) );
    m_xBtnImport->connect_clicked( LINK(this, SvxBitmapTabPage, ClickImportHdl) );
    if (comphelper::LibreOfficeKit::isActive())
        m_xBtnImport->hide();

    // Calculate size of display boxes
    Size aSize = getDrawPreviewOptimalSize(m_aCtlBitmapPreview.GetDrawingArea()->get_ref_device());
    m_xBitmapLB->set_size_request(aSize.Width(), aSize.Height());
    m_xCtlBitmapPreview->set_size_request(aSize.Width(), aSize.Height());

    SfxItemPool* pPool = m_rXFSet.GetPool();
    mePoolUnit = pPool->GetMetric( XATTR_FILLBMP_SIZEX );
    meFieldUnit = GetModuleFieldUnit( rInAttrs );
    SetFieldUnit( *m_xBitmapWidth, meFieldUnit, true );
    SetFieldUnit( *m_xBitmapHeight, meFieldUnit, true );

    m_xBitmapLB->SetStyle(WB_FLATVALUESET | WB_NO_DIRECTSELECT | WB_TABSTOP);

    SfxViewShell* pViewShell = SfxViewShell::Current();
    if( pViewShell )
        mpView = pViewShell->GetDrawView();
    DBG_ASSERT( mpView, "no valid view (!)" );
}

SvxBitmapTabPage::~SvxBitmapTabPage()
{
    m_xBitmapLBWin.reset();
    m_xBitmapLB.reset();
    m_xCtlBitmapPreview.reset();

    if (m_nBitmapListState & ChangeType::MODIFIED)
    {
        m_pBitmapList->SetPath(AreaTabHelper::GetPalettePath());
        m_pBitmapList->Save();

        // ToolBoxControls are informed:
        SfxObjectShell* pShell = SfxObjectShell::Current();
        if (pShell)
            pShell->PutItem(SvxBitmapListItem(m_pBitmapList, SID_BITMAP_LIST));
    }
}

void SvxBitmapTabPage::Construct()
{
    m_xBitmapLB->FillPresetListBox( *m_pBitmapList );
}

void SvxBitmapTabPage::ActivatePage( const SfxItemSet& rSet )
{
    const XFillBitmapItem& aItem( rSet.Get(XATTR_FILLBITMAP) );

    sal_Int32 nPos( 0 );
    if ( !aItem.isPattern() )
    {
        const GraphicObject& aGraphicObj = aItem.GetGraphicObject();
        if (aGraphicObj.GetType() != GraphicType::Bitmap)
            return;

        nPos = SearchBitmapList(aGraphicObj);
        if (nPos == -1)
            nPos = AddBitmap(aGraphicObj, aItem.GetName(), /*OnlyForThisDocument=*/true);
    }
    else
    {
        m_xBitmapWidth->set_value( 100, FieldUnit::NONE );
        m_xBitmapHeight->set_value( 100, FieldUnit::NONE );
        const_cast<SfxItemSet&>(rSet).Put( XFillBmpSizeXItem( GetCoreValue( *m_xBitmapWidth, mePoolUnit ) ) );
        const_cast<SfxItemSet&>(rSet).Put( XFillBmpSizeYItem( GetCoreValue( *m_xBitmapHeight, mePoolUnit ) ) );
    }

    sal_uInt16 nId = m_xBitmapLB->GetItemId( static_cast<size_t>( nPos ) );
    m_xBitmapLB->SelectItem( nId );
}

DeactivateRC SvxBitmapTabPage::DeactivatePage( SfxItemSet* _pSet )
{
    if( _pSet )
        FillItemSet( _pSet );

    return DeactivateRC::LeavePage;
}


bool SvxBitmapTabPage::FillItemSet( SfxItemSet* rAttrs )
{
    rAttrs->Put(XFillStyleItem(drawing::FillStyle_BITMAP));
    size_t nPos = m_xBitmapLB->GetSelectItemPos();
    if(VALUESET_ITEM_NOTFOUND != nPos)
    {
        const XBitmapEntry* pXBitmapEntry = m_pBitmapList->GetBitmap(nPos);
        const OUString aString(m_xBitmapLB->GetItemText( m_xBitmapLB->GetSelectedItemId() ));
        rAttrs->Put(XFillBitmapItem(aString, pXBitmapEntry->GetGraphicObject()));
    }

    BitmapStyle eStylePos = static_cast<BitmapStyle>(m_xBitmapStyleLB->get_active());
    bool bIsStretched( eStylePos == STRETCHED );
    bool bIsTiled( eStylePos == TILED );

    rAttrs->Put( XFillBmpTileItem(bIsTiled) );
    rAttrs->Put( XFillBmpStretchItem(bIsStretched) );

    if(!bIsStretched)
    {
        Size aSetBitmapSize;
        switch(eStylePos)
        {
            case CUSTOM:
            case TILED:
            {
                sal_Int64 nWidthPercent = m_xBitmapWidth->get_value(FieldUnit::NONE);
                sal_Int64 nHeightPercent = m_xBitmapHeight->get_value(FieldUnit::NONE);
                if (m_xTsbScale->get_sensitive() && m_xTsbScale->get_state() == TRISTATE_TRUE)
                {
                    aSetBitmapSize.setWidth( -nWidthPercent );
                    aSetBitmapSize.setHeight( -nHeightPercent );
                }
                else if (!m_bLogicalSize)
                {
                    aSetBitmapSize.setWidth( GetCoreValue(*m_xBitmapWidth, mePoolUnit) );
                    aSetBitmapSize.setHeight( GetCoreValue(*m_xBitmapHeight, mePoolUnit) );
                }
                else
                {
                    rAttrs->Put( XFillBmpSizeLogItem(true) );
                    aSetBitmapSize.setWidth( 0 );
                    aSetBitmapSize.setHeight( 0 );
                }

                break;
            }
            default:
                break;
        }

        rAttrs->Put( XFillBmpSizeXItem( aSetBitmapSize.Width() ) );
        rAttrs->Put( XFillBmpSizeYItem( aSetBitmapSize.Height() ) );
    }

    if (m_xPositionLB->get_sensitive())
        rAttrs->Put( XFillBmpPosItem( static_cast<RectPoint>( m_xPositionLB->get_active() ) ) );
    if (m_xPositionOffX->get_sensitive())
        rAttrs->Put( XFillBmpPosOffsetXItem(m_xPositionOffX->get_value(FieldUnit::PERCENT)));
    if (m_xPositionOffY->get_sensitive())
        rAttrs->Put( XFillBmpPosOffsetYItem(m_xPositionOffY->get_value(FieldUnit::PERCENT)));
    if (m_xTileOffBox->get_sensitive())
    {
        TileOffset eValue = static_cast<TileOffset>(m_xTileOffLB->get_active());
        sal_uInt16 nOffsetValue = static_cast<sal_uInt16>(m_xTileOffset->get_value(FieldUnit::PERCENT));
        sal_uInt16 nRowOff = (eValue == ROW) ? nOffsetValue : 0;
        sal_uInt16 nColOff = (eValue == COLUMN) ? nOffsetValue : 0;
        rAttrs->Put( XFillBmpTileOffsetXItem(nRowOff) );
        rAttrs->Put( XFillBmpTileOffsetYItem(nColOff) );
    }
    return true;
}


void SvxBitmapTabPage::Reset( const SfxItemSet* rAttrs )
{
    double transfWidth = 0.0;
    double transfHeight = 0.0;
    double fUIScale  = 1.0;
    if (mpView)
    {
        fUIScale  = double(mpView->GetModel().GetUIScale());


        if (mpView->GetMarkedObjectList().GetMarkCount() != 0)
        {
            SfxItemSet rGeoAttr(mpView->GetGeoAttrFromMarked());
            transfWidth = static_cast<double>(GetItem( rGeoAttr, SID_ATTR_TRANSFORM_WIDTH )->GetValue());
            transfHeight= static_cast<double>(GetItem( rGeoAttr, SID_ATTR_TRANSFORM_HEIGHT )->GetValue());
        }
    }
    m_fObjectWidth = std::max( transfWidth, 1.0 );
    m_fObjectHeight = std::max( transfHeight, 1.0 );
    double fTmpWidth((OutputDevice::LogicToLogic(static_cast<sal_Int32>(m_fObjectWidth), mePoolUnit, MapUnit::Map100thMM )) / fUIScale);
    m_fObjectWidth = fTmpWidth;

    double fTmpHeight((OutputDevice::LogicToLogic(static_cast<sal_Int32>(m_fObjectHeight), mePoolUnit, MapUnit::Map100thMM )) / fUIScale);
    m_fObjectHeight = fTmpHeight;

    const XFillBitmapItem& aItem( rAttrs->Get(XATTR_FILLBITMAP) );

    if(!aItem.isPattern())
    {
        m_rXFSet.Put( aItem );
        m_aCtlBitmapPreview.SetAttributes( m_aXFillAttr.GetItemSet() );
        m_aCtlBitmapPreview.Invalidate();
    }
    else
        m_xCtlBitmapPreview->set_sensitive(false);

    std::unique_ptr<GraphicObject> pGraphicObject;
    pGraphicObject.reset( new GraphicObject(aItem.GetGraphicObject()) );

    BitmapEx aBmpEx(pGraphicObject->GetGraphic().GetBitmapEx());
    Size aTempBitmapSize = aBmpEx.GetSizePixel();
    rBitmapSize = Application::GetDefaultDevice()->PixelToLogic(aTempBitmapSize, MapMode(MapUnit::Map100thMM));
    CalculateBitmapPresetSize();

    bool bTiled = false; bool bStretched = false;
    if(rAttrs->GetItemState( XATTR_FILLBMP_TILE ) != SfxItemState::INVALID)
        bTiled = rAttrs->Get( XATTR_FILLBMP_TILE ).GetValue();
    if(rAttrs->GetItemState( XATTR_FILLBMP_STRETCH ) != SfxItemState::INVALID)
        bStretched = rAttrs->Get( XATTR_FILLBMP_STRETCH ).GetValue();

    if (bTiled)
        m_xBitmapStyleLB->set_active(static_cast<sal_Int32>(TILED));
    else if (bStretched)
        m_xBitmapStyleLB->set_active(static_cast<sal_Int32>(STRETCHED));
    else
        m_xBitmapStyleLB->set_active(static_cast<sal_Int32>(CUSTOM));

    tools::Long nWidth = 0;
    tools::Long nHeight = 0;

    if(rAttrs->GetItemState(XATTR_FILLBMP_SIZELOG) != SfxItemState::INVALID)
    {
        if (rAttrs->Get( XATTR_FILLBMP_SIZELOG ).GetValue())
            m_xTsbScale->set_state(TRISTATE_FALSE);
        else
            m_xTsbScale->set_state(TRISTATE_TRUE);
    }
    else
        m_xTsbScale->set_state(TRISTATE_INDET);

    TriState eRelative = TRISTATE_FALSE;
    if(rAttrs->GetItemState(XATTR_FILLBMP_SIZEX) != SfxItemState::INVALID)
    {
        nWidth = static_cast<const XFillBmpSizeXItem&>( rAttrs->Get( XATTR_FILLBMP_SIZEX ) ).GetValue();
        if(nWidth == 0)
            nWidth = rBitmapSize.Width();
        else if(nWidth < 0)
        {
            m_bLogicalSize = true;
            eRelative = TRISTATE_TRUE;
            nWidth = std::abs(nWidth);
        }
    }

    if(rAttrs->GetItemState( XATTR_FILLBMP_SIZEY ) != SfxItemState::INVALID)
    {
        nHeight = rAttrs->Get( XATTR_FILLBMP_SIZEY ).GetValue();
        if(nHeight == 0)
            nHeight = rBitmapSize.Height();
        else if(nHeight < 0)
        {
            m_bLogicalSize = true;
            eRelative = TRISTATE_TRUE;
            nHeight = std::abs(nHeight);
        }
    }
    m_xTsbScale->set_state(eRelative);
    ClickScaleHdl(*m_xTsbScale);


    if(!rBitmapSize.IsEmpty())
    {
        if (eRelative == TRISTATE_TRUE)
        {
            m_xBitmapWidth->set_value(nWidth, FieldUnit::NONE);
            m_xBitmapHeight->set_value(nHeight, FieldUnit::NONE);
        }
        else
        {
            SetMetricValue(*m_xBitmapWidth, nWidth, mePoolUnit);
            SetMetricValue(*m_xBitmapHeight, nHeight, mePoolUnit);
        }
    }

    if( rAttrs->GetItemState( XATTR_FILLBMP_POS ) != SfxItemState::INVALID )
    {
        RectPoint eValue = rAttrs->Get( XATTR_FILLBMP_POS ).GetValue();
        m_xPositionLB->set_active( static_cast< sal_Int32 >(eValue) );
    }

    if( rAttrs->GetItemState( XATTR_FILLBMP_POSOFFSETX ) != SfxItemState::INVALID )
    {
        sal_Int32 nValue = rAttrs->Get( XATTR_FILLBMP_POSOFFSETX ).GetValue();
        m_xPositionOffX->set_value(nValue, FieldUnit::PERCENT);
    }
    else
        m_xPositionOffX->set_text(u""_ustr);

    if( rAttrs->GetItemState( XATTR_FILLBMP_POSOFFSETY ) != SfxItemState::INVALID )
    {
        sal_Int32 nValue = rAttrs->Get( XATTR_FILLBMP_POSOFFSETY ).GetValue();
        m_xPositionOffY->set_value(nValue, FieldUnit::PERCENT);
    }
    else
        m_xPositionOffY->set_text(u""_ustr);

    if( rAttrs->GetItemState( XATTR_FILLBMP_TILEOFFSETX ) != SfxItemState::INVALID)
    {
        sal_Int32 nValue = rAttrs->Get( XATTR_FILLBMP_TILEOFFSETX ).GetValue();
        if(nValue > 0)
        {
            m_xTileOffLB->set_active(static_cast<sal_Int32>(ROW));
            m_xTileOffset->set_value(nValue, FieldUnit::PERCENT);
        }
    }

    if( rAttrs->GetItemState( XATTR_FILLBMP_TILEOFFSETY ) != SfxItemState::INVALID )
    {
        sal_Int32 nValue = rAttrs->Get( XATTR_FILLBMP_TILEOFFSETY ).GetValue();
        if(nValue > 0)
        {
            m_xTileOffLB->set_active(static_cast<sal_Int32>(COLUMN));
            m_xTileOffset->set_value(nValue, FieldUnit::PERCENT);
        }
    }

    ClickBitmapHdl_Impl();
}

std::unique_ptr<SfxTabPage> SvxBitmapTabPage::Create(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet* rAttrs)
{
    return std::make_unique<SvxBitmapTabPage>(pPage, pController, *rAttrs);
}

void SvxBitmapTabPage::ClickBitmapHdl_Impl()
{
    m_xBitmapLBWin->set_sensitive(true);
    m_xCtlBitmapPreview->set_sensitive(true);

    ModifyBitmapHdl(m_xBitmapLB.get());
}

void SvxBitmapTabPage::DeleteBitmapHdl_Impl(const sal_uInt16 nId)
{
    const size_t nPos = m_xBitmapLB->GetItemPos(nId);
    if( nPos == VALUESET_ITEM_NOTFOUND )
        return;

    std::unique_ptr<weld::Builder> xBuilder(Application::CreateBuilder(GetFrameWeld(), u"cui/ui/querydeletebitmapdialog.ui"_ustr));
    std::unique_ptr<weld::MessageDialog> xQueryBox(xBuilder->weld_message_dialog(u"AskDelBitmapDialog"_ustr));

    if (xQueryBox->run() != RET_YES)
        return;

    sal_uInt16 nNextId = m_xBitmapLB->GetSelectedItemId();
    const bool bDeletingSelectedItem(nId == nNextId);
    if (bDeletingSelectedItem)
    {
        nNextId = m_xBitmapLB->GetItemId(nPos + 1);
        if (!nNextId)
            nNextId = m_xBitmapLB->GetItemId(nPos - 1);
    }

    m_pBitmapList->Remove( static_cast<sal_uInt16>(nPos) );
    m_xBitmapLB->RemoveItem( nId );

    if (bDeletingSelectedItem)
    {
        m_xBitmapLB->SelectItem(nNextId);
        m_aCtlBitmapPreview.Invalidate();
    }
    ModifyBitmapHdl(m_xBitmapLB.get());
    m_nBitmapListState |= ChangeType::MODIFIED;
}

void SvxBitmapTabPage::CalculateBitmapPresetSize()
{
    if(rBitmapSize.IsEmpty())
        return;

    tools::Long nObjectWidth = static_cast<tools::Long>(m_fObjectWidth);
    tools::Long nObjectHeight = static_cast<tools::Long>(m_fObjectHeight);

    if(std::abs(rBitmapSize.Width() - nObjectWidth) < std::abs(rBitmapSize.Height() - nObjectHeight))
    {
        rFilledSize.setWidth( nObjectWidth );
        rFilledSize.setHeight( rBitmapSize.Height()*nObjectWidth/rBitmapSize.Width() );
        rZoomedSize.setWidth( rBitmapSize.Width()*nObjectHeight/rBitmapSize.Height() );
        rZoomedSize.setHeight( nObjectHeight );
    }
    else
    {
        rFilledSize.setWidth( rBitmapSize.Width()*nObjectHeight/rBitmapSize.Height() );
        rFilledSize.setHeight( nObjectHeight );
        rZoomedSize.setWidth( nObjectWidth );
        rZoomedSize.setHeight( rBitmapSize.Height()*nObjectWidth/rBitmapSize.Width() );
    }
}

IMPL_LINK_NOARG(SvxBitmapTabPage, ModifyBitmapHdl, ValueSet*, void)
{
    std::unique_ptr<GraphicObject> pGraphicObject;
    size_t nPos = m_xBitmapLB->GetSelectItemPos();

    if( nPos != VALUESET_ITEM_NOTFOUND )
    {
        pGraphicObject.reset(new GraphicObject(m_pBitmapList->GetBitmap( static_cast<sal_uInt16>(nPos) )->GetGraphicObject()));
    }
    else
    {
        if(const XFillStyleItem* pFillStyleItem = m_rOutAttrs.GetItemIfSet(GetWhich(XATTR_FILLSTYLE)))
        {
            const drawing::FillStyle eXFS(pFillStyleItem->GetValue());

            const XFillBitmapItem* pBitmapItem;
            if((drawing::FillStyle_BITMAP == eXFS) && (pBitmapItem = m_rOutAttrs.GetItemIfSet(GetWhich(XATTR_FILLBITMAP))))
            {
                pGraphicObject.reset(new GraphicObject(pBitmapItem->GetGraphicObject()));
            }
        }

        if(!pGraphicObject)
        {
            sal_uInt16 nId = m_xBitmapLB->GetItemId(0);
            m_xBitmapLB->SelectItem(nId);

            if(0 != nId)
            {
                pGraphicObject.reset(new GraphicObject(m_pBitmapList->GetBitmap(0)->GetGraphicObject()));
            }
        }
    }

    if(pGraphicObject)
    {
        BitmapEx aBmpEx(pGraphicObject->GetGraphic().GetBitmapEx());
        Size aTempBitmapSize = aBmpEx.GetSizePixel();
        const double fUIScale = mpView ? double(mpView->GetModel().GetUIScale()) : 1.0;
        Size aBitmapSize100mm = o3tl::convert(aTempBitmapSize, o3tl::Length::pt, o3tl::Length::mm100);

        rBitmapSize.setWidth(aBitmapSize100mm.Width() / fUIScale);
        rBitmapSize.setHeight(aBitmapSize100mm.Height() / fUIScale);
        CalculateBitmapPresetSize();
        ModifyBitmapStyleHdl( *m_xBitmapStyleLB );
        ModifyBitmapPositionHdl( *m_xPositionLB );

        m_rXFSet.ClearItem(XATTR_FILLBITMAP);
        m_rXFSet.Put(XFillStyleItem(drawing::FillStyle_BITMAP));
        m_rXFSet.Put(XFillBitmapItem(OUString(), *pGraphicObject));

        m_aCtlBitmapPreview.SetAttributes( m_aXFillAttr.GetItemSet() );
        m_aCtlBitmapPreview.Invalidate();
    }
    else
    {
        SAL_WARN("cui.tabpages", "SvxBitmapTabPage::ModifyBitmapHdl(): null pGraphicObject");
    }

}

IMPL_LINK_NOARG(SvxBitmapTabPage, ClickRenameHdl, SvxPresetListBox*, void)
{
    const sal_uInt16 nId = m_xBitmapLB->GetContextMenuItemId();
    const size_t nPos = m_xBitmapLB->GetItemPos(nId);

    if( nPos == VALUESET_ITEM_NOTFOUND )
        return;

    OUString aDesc( CuiResId( RID_CUISTR_DESC_NEW_BITMAP ) );
    OUString aName( m_pBitmapList->GetBitmap( nPos )->GetName() );

    SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
    ScopedVclPtr<AbstractSvxNameDialog> pDlg(pFact->CreateSvxNameDialog(GetFrameWeld(), aName, aDesc));

    bool bLoop = true;
    while( bLoop && pDlg->Execute() == RET_OK )
    {
        aName = pDlg->GetName();
        sal_Int32 nBitmapPos = SearchBitmapList( aName );
        bool bValidBitmapName = (nBitmapPos == static_cast<sal_Int32>(nPos) ) || (nBitmapPos == -1);

        if(bValidBitmapName)
        {
            bLoop = false;
            m_pBitmapList->GetBitmap(nPos)->SetName(aName);

            m_xBitmapLB->SetItemText(nId, aName);

            m_nBitmapListState |= ChangeType::MODIFIED;
        }
        else
        {
            std::unique_ptr<weld::Builder> xBuilder(Application::CreateBuilder(GetFrameWeld(), u"cui/ui/queryduplicatedialog.ui"_ustr));
            std::unique_ptr<weld::MessageDialog> xBox(xBuilder->weld_message_dialog(u"DuplicateNameDialog"_ustr));
            xBox->run();
        }
    }
}

IMPL_LINK_NOARG(SvxBitmapTabPage, ClickDeleteHdl, SvxPresetListBox*, void)
{
    DeleteBitmapHdl_Impl(m_xBitmapLB->GetContextMenuItemId());
}


SvxBitmapTabPage::SvxPresetListBoxValueSet::SvxPresetListBoxValueSet(std::unique_ptr<weld::ScrolledWindow> pWindow)
    : SvxPresetListBox(std::move(pWindow))
    , m_pSvxBitmapTabPage(nullptr)
{
}

bool SvxBitmapTabPage::SvxPresetListBoxValueSet::KeyInput(const KeyEvent& rKEvt)
{
    switch (rKEvt.GetKeyCode().GetCode())
    {
        case KEY_DELETE:
        {
            m_pSvxBitmapTabPage->DeleteBitmapHdl_Impl(GetSelectedItemId());
            return true;
        }
        break;
        default:
            return SvxPresetListBox::KeyInput(rKEvt);
    }
}

IMPL_LINK_NOARG( SvxBitmapTabPage, ModifyBitmapSizeHdl, weld::MetricSpinButton&, void )
{
    m_bLogicalSize = false;
    if (m_xTsbScale->get_state() != TRISTATE_TRUE && static_cast<BitmapStyle>(m_xBitmapStyleLB->get_active()) != TILED)
    {
        sal_Int64 nWidthPercent = m_xBitmapWidth->denormalize(m_xBitmapWidth->get_value(FieldUnit::NONE));
        sal_Int64 nHeightPercent = m_xBitmapHeight->denormalize(m_xBitmapHeight->get_value(FieldUnit::NONE));
        if (nWidthPercent == 100 && nHeightPercent == 100)
            m_xBitmapStyleLB->set_active(static_cast<sal_Int32>(CUSTOM));
    }
    ModifyBitmapStyleHdl(*m_xBitmapStyleLB);

    m_aCtlBitmapPreview.SetAttributes( m_aXFillAttr.GetItemSet() );
    m_aCtlBitmapPreview.Invalidate();
}

IMPL_LINK_NOARG( SvxBitmapTabPage, ClickScaleHdl, weld::Toggleable&, void )
{
    if (m_xTsbScale->get_state() == TRISTATE_TRUE)
    {
        m_xBitmapWidth->set_digits( 0 );
        m_xBitmapWidth->set_unit(FieldUnit::PERCENT);
        m_xBitmapWidth->set_value(100, FieldUnit::NONE);
        m_xBitmapWidth->set_range(0, 100, FieldUnit::NONE);

        m_xBitmapHeight->set_digits( 0 );
        m_xBitmapHeight->set_unit(FieldUnit::PERCENT);
        m_xBitmapHeight->set_value(100, FieldUnit::NONE);
        m_xBitmapHeight->set_range(0, 100, FieldUnit::NONE);
    }
    else
    {
        m_xBitmapWidth->set_digits( 2 );
        m_xBitmapWidth->set_unit(meFieldUnit);
        m_xBitmapWidth->set_value(100, FieldUnit::NONE);
        m_xBitmapWidth->set_range(0, 999900, FieldUnit::NONE);

        m_xBitmapHeight->set_digits( 2 );
        m_xBitmapHeight->set_unit(meFieldUnit);
        m_xBitmapHeight->set_value(100, FieldUnit::NONE);
        m_xBitmapHeight->set_range(0, 999900, FieldUnit::NONE);
    }

    ModifyBitmapStyleHdl( *m_xBitmapStyleLB );
}

IMPL_LINK_NOARG( SvxBitmapTabPage, ModifyBitmapStyleHdl, weld::ComboBox&, void )
{
    BitmapStyle eStylePos = static_cast<BitmapStyle>(m_xBitmapStyleLB->get_active());
    bool bIsStretched( eStylePos == STRETCHED );
    bool bIsTiled( eStylePos == TILED );

    m_xSizeBox->set_sensitive( !bIsStretched );
    m_xPositionBox->set_sensitive( !bIsStretched );
    m_xPositionOffBox->set_sensitive( bIsTiled );
    m_xTileOffBox->set_sensitive( bIsTiled );

    m_rXFSet.Put( XFillBmpTileItem( bIsTiled ) );
    m_rXFSet.Put( XFillBmpStretchItem( bIsStretched ) );

    if(!bIsStretched)
    {
        Size aSetBitmapSize;
        switch(eStylePos)
        {
            case CUSTOM:
            case TILED:
            {
                if (m_xTsbScale->get_sensitive() && m_xTsbScale->get_state() == TRISTATE_TRUE)
                {
                    aSetBitmapSize.setWidth(-m_xBitmapWidth->get_value(FieldUnit::NONE));
                    aSetBitmapSize.setHeight(-m_xBitmapHeight->get_value(FieldUnit::NONE));
                }
                else
                {
                    aSetBitmapSize.setWidth( GetCoreValue( *m_xBitmapWidth, mePoolUnit ) );
                    aSetBitmapSize.setHeight( GetCoreValue( *m_xBitmapHeight, mePoolUnit ) );
                }
            }
                break;
            default:
                break;
        }

        m_rXFSet.Put( XFillBmpSizeXItem( aSetBitmapSize.Width() ) );
        m_rXFSet.Put( XFillBmpSizeYItem( aSetBitmapSize.Height() ) );
    }

    m_aCtlBitmapPreview.SetAttributes( m_aXFillAttr.GetItemSet() );
    m_aCtlBitmapPreview.Invalidate();
}

IMPL_LINK_NOARG(SvxBitmapTabPage, ModifyBitmapPositionHdl, weld::ComboBox&, void)
{
    if (m_xPositionLB->get_sensitive())
        m_rXFSet.Put( XFillBmpPosItem( static_cast< RectPoint >( m_xPositionLB->get_active() ) ) );

    m_aCtlBitmapPreview.SetAttributes( m_aXFillAttr.GetItemSet() );
    m_aCtlBitmapPreview.Invalidate();
}

IMPL_LINK_NOARG(SvxBitmapTabPage, ModifyPositionOffsetHdl, weld::MetricSpinButton&, void)
{
    if (m_xPositionOffX->get_sensitive())
        m_rXFSet.Put( XFillBmpPosOffsetXItem( m_xPositionOffX->get_value(FieldUnit::PERCENT) ) );

    if (m_xPositionOffY->get_sensitive())
        m_rXFSet.Put( XFillBmpPosOffsetYItem( m_xPositionOffY->get_value(FieldUnit::PERCENT) ) );

    m_aCtlBitmapPreview.SetAttributes( m_aXFillAttr.GetItemSet() );
    m_aCtlBitmapPreview.Invalidate();
}

IMPL_LINK_NOARG(SvxBitmapTabPage, ModifyTileOffsetHdl, weld::MetricSpinButton&, void)
{
    sal_uInt16 nTileXOff = 0;
    sal_uInt16 nTileYOff = 0;

    if(m_xTileOffLB->get_active() == static_cast<sal_Int32>(ROW))
        nTileXOff = m_xTileOffset->get_value(FieldUnit::PERCENT);

    if(m_xTileOffLB->get_active() == static_cast<sal_Int32>(COLUMN))
        nTileYOff = m_xTileOffset->get_value(FieldUnit::PERCENT);

    m_rXFSet.Put( XFillBmpTileOffsetXItem(nTileXOff) );
    m_rXFSet.Put( XFillBmpTileOffsetYItem(nTileYOff) );

    m_aCtlBitmapPreview.SetAttributes( m_aXFillAttr.GetItemSet() );
    m_aCtlBitmapPreview.Invalidate();
}

IMPL_LINK_NOARG(SvxBitmapTabPage, ClickImportHdl, weld::Button&, void)
{
    weld::Window* pDialogFrameWeld = GetFrameWeld();

    SvxOpenGraphicDialog aDlg(CuiResId(RID_CUISTR_ADD_IMAGE), pDialogFrameWeld);
    aDlg.EnableLink(false);
    tools::Long nCount = m_pBitmapList->Count();

    if( aDlg.Execute() )
        return;

    Graphic         aGraphic;

    std::unique_ptr<weld::WaitObject> xWait(new weld::WaitObject(pDialogFrameWeld));
    ErrCode nError = aDlg.GetGraphic( aGraphic );
    xWait.reset();

    if( !nError )
    {
        OUString aDesc(CuiResId(RID_CUISTR_DESC_EXT_BITMAP));

        // convert file URL to UI name
        OUString        aName;
        INetURLObject   aURL( aDlg.GetPath() );
        SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
        ScopedVclPtr<AbstractSvxNameDialog> pDlg(pFact->CreateSvxNameDialog(
            pDialogFrameWeld, aURL.GetLastName().getToken(0, '.'), aDesc));
        nError = ErrCode(1);

        while( pDlg->Execute() == RET_OK )
        {
            aName = pDlg->GetName();

            bool bDifferent = true;

            for( tools::Long i = 0; i < nCount && bDifferent; i++ )
                if( aName == m_pBitmapList->GetBitmap( i )->GetName() )
                    bDifferent = false;

            if( bDifferent ) {
                nError = ERRCODE_NONE;
                break;
            }

            std::unique_ptr<weld::Builder> xBuilder(Application::CreateBuilder(pDialogFrameWeld, u"cui/ui/queryduplicatedialog.ui"_ustr));
            std::unique_ptr<weld::MessageDialog> xBox(xBuilder->weld_message_dialog(u"DuplicateNameDialog"_ustr));
            if (xBox->run() != RET_OK)
                break;
        }

        pDlg.disposeAndClear();

        if( !nError )
            AddBitmap(std::move(aGraphic), aName);
    }
    else
    {
        // graphic couldn't be loaded
        std::unique_ptr<weld::Builder> xBuilder(Application::CreateBuilder(pDialogFrameWeld, u"cui/ui/querynoloadedfiledialog.ui"_ustr));
        std::unique_ptr<weld::MessageDialog> xBox(xBuilder->weld_message_dialog(u"NoLoadedFileDialog"_ustr));
        xBox->run();
    }
}

sal_Int32 SvxBitmapTabPage::SearchBitmapList(const GraphicObject& rGraphicObject)
{
    tools::Long nCount = m_pBitmapList->Count();
    sal_Int32 nPos = -1;

    for(tools::Long i = 0;i < nCount;i++)
    {
        if(rGraphicObject.GetUniqueID() == m_pBitmapList->GetBitmap( i )->GetGraphicObject().GetUniqueID())
        {
            nPos = i;
            break;
        }
    }
    return nPos;
}

sal_Int32 SvxBitmapTabPage::SearchBitmapList(std::u16string_view rBitmapName)
{
    tools::Long nCount = m_pBitmapList->Count();
    bool bValidBitmapName = true;
    sal_Int32 nPos = -1;

    for(tools::Long i = 0;i < nCount && bValidBitmapName;i++)
    {
        if(rBitmapName == m_pBitmapList->GetBitmap( i )->GetName())
        {
            nPos = i;
            bValidBitmapName = false;
        }
    }
    return nPos;
}

tools::Long SvxBitmapTabPage::AddBitmap(const GraphicObject& rGraphicObject, const OUString& rName,
                                        bool bOnlyForThisDocument)
{
    const tools::Long nLastPos = m_pBitmapList->Count();

    auto xBitmapEntry = std::make_unique<XBitmapEntry>(rGraphicObject, rName);
    if (bOnlyForThisDocument)
        xBitmapEntry->SetSavingAllowed(false);
    m_pBitmapList->Insert(std::move(xBitmapEntry), nLastPos);

    BitmapEx aBitmap = m_pBitmapList->GetBitmapForPreview(nLastPos, m_xBitmapLB->GetIconSize());

    const sal_uInt16 nHighestId = m_xBitmapLB->GetItemId(nLastPos - 1);
    m_xBitmapLB->InsertItem(nHighestId + 1, Image(aBitmap), rName);
    m_xBitmapLB->SelectItem(nHighestId + 1);
    m_nBitmapListState |= ChangeType::MODIFIED;

    ModifyBitmapHdl(m_xBitmapLB.get());

    // inform sidebar, etc. that the list of images has changed.
    SfxViewFrame* pViewFrame = SfxViewFrame::Current();
    if (pViewFrame)
        pViewFrame->GetBindings().Invalidate(SID_ATTR_PAGE_BITMAP, /*ClearCacheStatus=*/true);

    return nLastPos;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
