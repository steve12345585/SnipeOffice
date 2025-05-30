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

#include <sal/config.h>

#include <algorithm>

#include "PosSizePropertyPanel.hxx"
#include <sal/log.hxx>
#include <svx/svxids.hrc>
#include <sfx2/dispatch.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/module.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/weldutils.hxx>
#include <svx/dialcontrol.hxx>
#include <svx/dialmgr.hxx>
#include <svx/rectenum.hxx>
#include <svx/sdangitm.hxx>
#include <unotools/viewoptions.hxx>
#include <unotools/localedatawrapper.hxx>
#include <utility>
#include <vcl/canvastools.hxx>
#include <vcl/fieldvalues.hxx>
#include <svl/intitem.hxx>
#include <svx/strings.hrc>
#include <svx/svdpagv.hxx>
#include <svx/svdview.hxx>
#include <svx/transfrmhelper.hxx>
#include <boost/property_tree/ptree.hpp>

#include <svtools/unitconv.hxx>
#include <bitmaps.hlst>

using namespace css;
using namespace css::uno;

constexpr OUString USERITEM_NAME = u"FitItem"_ustr;

namespace svx::sidebar {

PosSizePropertyPanel::PosSizePropertyPanel(
    weld::Widget* pParent,
    const css::uno::Reference<css::frame::XFrame>& rxFrame,
    SfxBindings* pBindings,
    css::uno::Reference<css::ui::XSidebar> xSidebar)
:   PanelLayout(pParent, u"PosSizePropertyPanel"_ustr, u"svx/ui/sidebarpossize.ui"_ustr),
    m_aRatioTop(ConnectorType::Top),
    m_aRatioBottom(ConnectorType::Bottom),
    mxFtPosX(m_xBuilder->weld_label(u"horizontallabel"_ustr)),
    mxMtrPosX(m_xBuilder->weld_metric_spin_button(u"horizontalpos"_ustr, FieldUnit::CM)),
    mxFtPosY(m_xBuilder->weld_label(u"verticallabel"_ustr)),
    mxMtrPosY(m_xBuilder->weld_metric_spin_button(u"verticalpos"_ustr, FieldUnit::CM)),
    mxFtWidth(m_xBuilder->weld_label(u"widthlabel"_ustr)),
    mxMtrWidth(m_xBuilder->weld_metric_spin_button(u"selectwidth"_ustr, FieldUnit::CM)),
    mxFtHeight(m_xBuilder->weld_label(u"heightlabel"_ustr)),
    mxMtrHeight(m_xBuilder->weld_metric_spin_button(u"selectheight"_ustr, FieldUnit::CM))
    , mxCbxScale(m_xBuilder->weld_check_button(u"ratio"_ustr))
    , m_xCbxScaleImg(m_xBuilder->weld_image(u"imRatio"_ustr))
    , m_xImgRatioTop(new weld::CustomWeld(*m_xBuilder, u"daRatioTop"_ustr, m_aRatioTop))
    , m_xImgRatioBottom(new weld::CustomWeld(*m_xBuilder, u"daRatioBottom"_ustr, m_aRatioBottom))
    , mxFtAngle(m_xBuilder->weld_label(u"rotationlabel"_ustr))
    , mxMtrAngle(m_xBuilder->weld_metric_spin_button(u"rotation"_ustr, FieldUnit::DEGREE)),
    mxCtrlDial(new DialControl),
    mxDial(new weld::CustomWeld(*m_xBuilder, u"orientationcontrol"_ustr, *mxCtrlDial)),
    mxFtFlip(m_xBuilder->weld_label(u"fliplabel"_ustr)),
    mxFlipTbx(m_xBuilder->weld_toolbar(u"selectrotationtype"_ustr)),
    mxFlipDispatch(new ToolbarUnoDispatcher(*mxFlipTbx, *m_xBuilder, rxFrame)),
    mxArrangeTbx(m_xBuilder->weld_toolbar(u"arrangetoolbar"_ustr)),
    mxArrangeDispatch(new ToolbarUnoDispatcher(*mxArrangeTbx, *m_xBuilder, rxFrame)),
    mxArrangeTbx2(m_xBuilder->weld_toolbar(u"arrangetoolbar2"_ustr)),
    mxArrangeDispatch2(new ToolbarUnoDispatcher(*mxArrangeTbx2, *m_xBuilder, rxFrame)),
    mxAlignTbx(m_xBuilder->weld_toolbar(u"aligntoolbar"_ustr)),
    mxAlignDispatch(new ToolbarUnoDispatcher(*mxAlignTbx, *m_xBuilder, rxFrame)),
    mxAlignTbx2(m_xBuilder->weld_toolbar(u"aligntoolbar2"_ustr)),
    mxAlignDispatch2(new ToolbarUnoDispatcher(*mxAlignTbx2, *m_xBuilder, rxFrame)),
    mxBtnEditOLEObject(m_xBuilder->weld_button(u"btnEditObject"_ustr)),
    mpView(nullptr),
    mlOldWidth(1),
    mlOldHeight(1),
    mlRotX(0),
    mlRotY(0),
    mePoolUnit(),
    meDlgUnit(FieldUnit::INCH), // #i124409# init with fallback default
    mbFieldMetricOutDated(true),
    maTransfPosXControl(SID_ATTR_TRANSFORM_POS_X, *pBindings, *this),
    maTransfPosYControl(SID_ATTR_TRANSFORM_POS_Y, *pBindings, *this),
    maTransfWidthControl(SID_ATTR_TRANSFORM_WIDTH, *pBindings, *this),
    maTransfHeightControl(SID_ATTR_TRANSFORM_HEIGHT, *pBindings, *this),
    maSvxAngleControl( SID_ATTR_TRANSFORM_ANGLE, *pBindings, *this),
    maRotXControl(SID_ATTR_TRANSFORM_ROT_X, *pBindings, *this),
    maRotYControl(SID_ATTR_TRANSFORM_ROT_Y, *pBindings, *this),
    maProPosControl(SID_ATTR_TRANSFORM_PROTECT_POS, *pBindings, *this),
    maProSizeControl(SID_ATTR_TRANSFORM_PROTECT_SIZE, *pBindings, *this),
    maAutoWidthControl(SID_ATTR_TRANSFORM_AUTOWIDTH, *pBindings, *this),
    maAutoHeightControl(SID_ATTR_TRANSFORM_AUTOHEIGHT, *pBindings, *this),
    m_aMetricCtl(SID_ATTR_METRIC, *pBindings, *this),
    mpBindings(pBindings),
    mbSizeProtected(false),
    mbPositionProtected(false),
    mbAutoWidth(false),
    mbAutoHeight(false),
    mbAdjustEnabled(false),
    mbMtrPosXBlanked(false),
    mbMtrPosYBlanked(false),
    mbMtrWidthBlanked(false),
    mbMtrHeightBlanked(false),
    mbMtrAngleBlanked(false),
    mxSidebar(std::move(xSidebar))
{
    Initialize();

    // A guesstimate of the longest label in the various sidebar panes to use
    // to get this pane's contents to align with them, for lack of a better
    // solution
    auto nWidth = mxFtWidth->get_preferred_size().Width();
    OUString sLabel = mxFtWidth->get_label();
    mxFtWidth->set_label(SvxResId(RID_SVXSTR_TRANSPARENCY));
    nWidth = std::max(nWidth, mxFtWidth->get_preferred_size().Width());;
    mxFtWidth->set_label(sLabel);
    mxFtWidth->set_size_request(nWidth, -1);

    // vertical alignment = fill makes the drawingarea expand the associated spinedits so we have to size it here
    const sal_Int16 aHeight
        = static_cast<sal_Int16>(std::max(int(mxCbxScale->get_preferred_size().getHeight() / 2
                                              - mxMtrWidth->get_preferred_size().getHeight() / 2),
                                          12));
    const sal_Int16 aWidth
        = static_cast<sal_Int16>(mxCbxScale->get_preferred_size().getWidth() / 2);
    m_xImgRatioTop->set_size_request(aWidth, aHeight);
    m_xImgRatioBottom->set_size_request(aWidth, aHeight);
    //init needed for gtk3
    m_xCbxScaleImg->set_from_icon_name(mxCbxScale->get_active() ? RID_SVXBMP_LOCKED
                                                                : RID_SVXBMP_UNLOCKED);

    mpBindings->Update( SID_ATTR_METRIC );
    mpBindings->Update( SID_ATTR_TRANSFORM_WIDTH );
    mpBindings->Update( SID_ATTR_TRANSFORM_HEIGHT );
    mpBindings->Update( SID_ATTR_TRANSFORM_PROTECT_SIZE );
}

PosSizePropertyPanel::~PosSizePropertyPanel()
{
    mxFtPosX.reset();
    mxMtrPosX.reset();
    mxFtPosY.reset();
    mxMtrPosY.reset();
    mxFtWidth.reset();
    mxMtrWidth.reset();
    mxFtHeight.reset();
    mxMtrHeight.reset();
    mxCbxScale.reset();
    mxFtAngle.reset();
    mxMtrAngle.reset();
    mxDial.reset();
    mxCtrlDial.reset();
    mxFtFlip.reset();
    mxFlipDispatch.reset();
    mxFlipTbx.reset();
    mxAlignDispatch.reset();
    mxAlignDispatch2.reset();
    mxAlignTbx.reset();
    mxAlignTbx2.reset();
    mxArrangeDispatch.reset();
    mxArrangeDispatch2.reset();
    mxArrangeTbx.reset();
    mxArrangeTbx2.reset();
    mxBtnEditOLEObject.reset();

    maTransfPosXControl.dispose();
    maTransfPosYControl.dispose();
    maTransfWidthControl.dispose();
    maTransfHeightControl.dispose();

    maSvxAngleControl.dispose();
    maRotXControl.dispose();
    maRotYControl.dispose();
    maProPosControl.dispose();
    maProSizeControl.dispose();
    maAutoWidthControl.dispose();
    maAutoHeightControl.dispose();
    m_aMetricCtl.dispose();
}

namespace
{
    bool hasText(const SdrView& rSdrView)
    {
        const SdrMarkList& rMarkList = rSdrView.GetMarkedObjectList();

        if(1 == rMarkList.GetMarkCount())
        {
            const SdrObject* pObj = rMarkList.GetMark(0)->GetMarkedSdrObj();
            const SdrObjKind eKind(pObj->GetObjIdentifier());

            if((pObj->GetObjInventor() == SdrInventor::Default) && (SdrObjKind::Text == eKind || SdrObjKind::TitleText == eKind || SdrObjKind::OutlineText == eKind))
            {
                const SdrTextObj* pSdrTextObj = DynCastSdrTextObj(pObj);

                if(pSdrTextObj && pSdrTextObj->HasText())
                {
                    return true;
                }
            }
        }

        return false;
    }
} // end of anonymous namespace


void PosSizePropertyPanel::Initialize()
{
    //Position : Horizontal / Vertical
    mxMtrPosX->connect_value_changed( LINK( this, PosSizePropertyPanel, ChangePosXHdl ) );
    mxMtrPosY->connect_value_changed( LINK( this, PosSizePropertyPanel, ChangePosYHdl ) );

    //Size : Width / Height
    mxMtrWidth->connect_value_changed( LINK( this, PosSizePropertyPanel, ChangeWidthHdl ) );
    mxMtrHeight->connect_value_changed( LINK( this, PosSizePropertyPanel, ChangeHeightHdl ) );

    //Size : Keep ratio
    mxCbxScale->connect_toggled( LINK( this, PosSizePropertyPanel, ClickAutoHdl ) );

    //rotation control
    mxCtrlDial->SetLinkedField(mxMtrAngle.get(), 2);
    mxCtrlDial->SetModifyHdl(LINK( this, PosSizePropertyPanel, RotationHdl));

    //use same logic as DialControl_Impl::SetSize
    weld::DrawingArea* pDrawingArea = mxCtrlDial->GetDrawingArea();
    int nDim = (std::min<int>(pDrawingArea->get_approximate_digit_width() * 6,
                              pDrawingArea->get_text_height() * 3) - 1) | 1;
    Size aSize(nDim, nDim);
    pDrawingArea->set_size_request(aSize.Width(), aSize.Height());
    mxCtrlDial->Init(aSize);

    mxBtnEditOLEObject->connect_clicked( LINK( this, PosSizePropertyPanel, ClickObjectEditHdl ) );

    SfxViewShell* pCurSh = SfxViewShell::Current();
    if ( pCurSh )
        mpView = pCurSh->GetDrawView();
    else
        mpView = nullptr;

    if ( mpView != nullptr )
    {
        maUIScale = mpView->GetModel().GetUIScale();
        mbAdjustEnabled = hasText(*mpView);
    }

    mePoolUnit = maTransfWidthControl.GetCoreMetric();
}

std::unique_ptr<PanelLayout> PosSizePropertyPanel::Create (
    weld::Widget* pParent,
    const css::uno::Reference<css::frame::XFrame>& rxFrame,
    SfxBindings* pBindings,
    const css::uno::Reference<css::ui::XSidebar>& rxSidebar)
{
    if (pParent == nullptr)
        throw lang::IllegalArgumentException(u"no parent Window given to PosSizePropertyPanel::Create"_ustr, nullptr, 0);
    if ( ! rxFrame.is())
        throw lang::IllegalArgumentException(u"no XFrame given to PosSizePropertyPanel::Create"_ustr, nullptr, 1);
    if (pBindings == nullptr)
        throw lang::IllegalArgumentException(u"no SfxBindings given to PosSizePropertyPanel::Create"_ustr, nullptr, 2);

    return std::make_unique<PosSizePropertyPanel>(pParent, rxFrame, pBindings, rxSidebar);
}

void PosSizePropertyPanel::HandleContextChange(
    const vcl::EnumContext& rContext)
{
    if (maContext == rContext)
    {
        // Nothing to do.
        return;
    }

    maContext = rContext;

    bool bShowPosition = false;
    bool bShowAngle = false;
    bool bShowFlip = false;
    bool bShowEditObject = false;
    bool bShowArrangeTbx2 = false;

    switch (maContext.GetCombinedContext_DI())
    {
        case CombinedEnumContext(Application::WriterVariants, Context::Draw):
            bShowAngle = true;
            bShowFlip = true;
            bShowArrangeTbx2 = true;
            break;

        case CombinedEnumContext(Application::WriterVariants, Context::Graphic):
            bShowFlip = true;
            bShowAngle = true; // RotGrfFlyFrame: Writer FlyFrames for Graphics now support angle
            break;

        case CombinedEnumContext(Application::Calc, Context::Draw):
        case CombinedEnumContext(Application::Calc, Context::DrawLine):
        case CombinedEnumContext(Application::Calc, Context::Graphic):
        case CombinedEnumContext(Application::DrawImpress, Context::Draw):
        case CombinedEnumContext(Application::DrawImpress, Context::DrawLine):
        case CombinedEnumContext(Application::DrawImpress, Context::TextObject):
        case CombinedEnumContext(Application::DrawImpress, Context::Graphic):
            bShowPosition = true;
            bShowAngle = true;
            bShowFlip = true;
            break;

        case CombinedEnumContext(Application::WriterVariants, Context::OLE):
            bShowEditObject = true;
            break;

        case CombinedEnumContext(Application::Calc, Context::OLE):
        case CombinedEnumContext(Application::DrawImpress, Context::OLE):
            bShowPosition = true;
            bShowEditObject = true;
            break;

        case CombinedEnumContext(Application::Calc, Context::Chart):
        case CombinedEnumContext(Application::Calc, Context::Form):
        case CombinedEnumContext(Application::Calc, Context::Media):
        case CombinedEnumContext(Application::Calc, Context::MultiObject):
        case CombinedEnumContext(Application::DrawImpress, Context::Media):
        case CombinedEnumContext(Application::DrawImpress, Context::Form):
        case CombinedEnumContext(Application::DrawImpress, Context::ThreeDObject):
        case CombinedEnumContext(Application::DrawImpress, Context::MultiObject):
            bShowPosition = true;
            break;
    }

    // Position
    mxFtPosX->set_visible(bShowPosition);
    mxMtrPosX->set_visible(bShowPosition);
    mxFtPosY->set_visible(bShowPosition);
    mxMtrPosY->set_visible(bShowPosition);

    // Rotation
    mxFtAngle->set_visible(bShowAngle);
    mxMtrAngle->set_visible(bShowAngle);
    mxCtrlDial->set_visible(bShowAngle);

    // Flip
    mxFtFlip->set_visible(bShowFlip);
    mxFlipTbx->set_visible(bShowFlip);

    // Edit Object
    mxBtnEditOLEObject->set_visible(bShowEditObject);

    // Arrange tool bar 2
    mxArrangeTbx2->set_visible(bShowArrangeTbx2);

    if (mxSidebar.is())
        mxSidebar->requestLayout();
}


IMPL_LINK_NOARG( PosSizePropertyPanel, ChangeWidthHdl, weld::MetricSpinButton&, void )
{
    if( mxCbxScale->get_active() &&
        mxCbxScale->get_sensitive() )
    {
        tools::Long nHeight = static_cast<tools::Long>( (static_cast<double>(mlOldHeight) * static_cast<double>(mxMtrWidth->get_value(FieldUnit::NONE))) / static_cast<double>(mlOldWidth) );
        if( nHeight <= mxMtrHeight->get_max( FieldUnit::NONE ) )
        {
            mxMtrHeight->set_value( nHeight, FieldUnit::NONE );
        }
        else
        {
            nHeight = static_cast<tools::Long>(mxMtrHeight->get_max( FieldUnit::NONE ));
            mxMtrHeight->set_value(nHeight, FieldUnit::NONE);
            const tools::Long nWidth = static_cast<tools::Long>( (static_cast<double>(mlOldWidth) * static_cast<double>(nHeight)) / static_cast<double>(mlOldHeight) );
            mxMtrWidth->set_value( nWidth, FieldUnit::NONE );
        }
    }
    executeSize();
}


IMPL_LINK_NOARG( PosSizePropertyPanel, ChangeHeightHdl, weld::MetricSpinButton&, void )
{
    if( mxCbxScale->get_active() &&
        mxCbxScale->get_sensitive() )
    {
        tools::Long nWidth = static_cast<tools::Long>( (static_cast<double>(mlOldWidth) * static_cast<double>(mxMtrHeight->get_value(FieldUnit::NONE))) / static_cast<double>(mlOldHeight) );
        if( nWidth <= mxMtrWidth->get_max( FieldUnit::NONE ) )
        {
            mxMtrWidth->set_value( nWidth, FieldUnit::NONE );
        }
        else
        {
            nWidth = static_cast<tools::Long>(mxMtrWidth->get_max( FieldUnit::NONE ));
            mxMtrWidth->set_value( nWidth, FieldUnit::NONE );
            const tools::Long nHeight = static_cast<tools::Long>( (static_cast<double>(mlOldHeight) * static_cast<double>(nWidth)) / static_cast<double>(mlOldWidth) );
            mxMtrHeight->set_value( nHeight, FieldUnit::NONE );
        }
    }
    executeSize();
}


IMPL_LINK_NOARG( PosSizePropertyPanel, ChangePosXHdl, weld::MetricSpinButton&, void )
{
    if ( mxMtrPosX->get_value_changed_from_saved())
    {
        tools::Long lX = GetCoreValue( *mxMtrPosX, mePoolUnit );

        Fraction aUIScale = mpView->GetModel().GetUIScale();
        lX = tools::Long( lX * aUIScale );

        SfxInt32Item aPosXItem( SID_ATTR_TRANSFORM_POS_X,static_cast<sal_uInt32>(lX));

        GetBindings()->GetDispatcher()->ExecuteList(
            SID_ATTR_TRANSFORM, SfxCallMode::RECORD, { &aPosXItem });
    }
}

IMPL_LINK_NOARG( PosSizePropertyPanel, ChangePosYHdl, weld::MetricSpinButton&, void )
{
    if ( mxMtrPosY->get_value_changed_from_saved() )
    {
        tools::Long lY = GetCoreValue( *mxMtrPosY, mePoolUnit );

        Fraction aUIScale = mpView->GetModel().GetUIScale();
        lY = tools::Long( lY * aUIScale );

        SfxInt32Item aPosYItem( SID_ATTR_TRANSFORM_POS_Y,static_cast<sal_uInt32>(lY));

        GetBindings()->GetDispatcher()->ExecuteList(
            SID_ATTR_TRANSFORM, SfxCallMode::RECORD, { &aPosYItem });
    }
}

IMPL_LINK_NOARG( PosSizePropertyPanel, ClickAutoHdl, weld::Toggleable&, void )
{
    m_xCbxScaleImg->set_from_icon_name(mxCbxScale->get_active() ? RID_SVXBMP_LOCKED : RID_SVXBMP_UNLOCKED);
    if ( mxCbxScale->get_active() )
    {
        mlOldWidth  = std::max(GetCoreValue(*mxMtrWidth,  mePoolUnit), SAL_CONST_INT64(1));
        mlOldHeight = std::max(GetCoreValue(*mxMtrHeight, mePoolUnit), SAL_CONST_INT64(1));
    }

    // mxCbxScale must synchronized with that on Position and Size tabpage on Shape Properties dialog
    SvtViewOptions aPageOpt(EViewType::TabPage, u"cui/ui/possizetabpage/PositionAndSize"_ustr);
    aPageOpt.SetUserItem( USERITEM_NAME, css::uno::Any( OUString::number( int(mxCbxScale->get_active()) ) ) );
}

IMPL_LINK_NOARG( PosSizePropertyPanel, RotationHdl, DialControl&, void )
{
    Degree100 nTmp = mxCtrlDial->GetRotation();

    // #i123993# Need to take UIScale into account when executing rotations
    const double fUIScale(mpView ? double(mpView->GetModel().GetUIScale()) : 1.0);
    SdrAngleItem aAngleItem( SID_ATTR_TRANSFORM_ANGLE, nTmp);
    SfxInt32Item aRotXItem( SID_ATTR_TRANSFORM_ROT_X, basegfx::fround(mlRotX * fUIScale));
    SfxInt32Item aRotYItem( SID_ATTR_TRANSFORM_ROT_Y, basegfx::fround(mlRotY * fUIScale));

    GetBindings()->GetDispatcher()->ExecuteList(SID_ATTR_TRANSFORM,
            SfxCallMode::RECORD, { &aAngleItem, &aRotXItem, &aRotYItem });
}

IMPL_STATIC_LINK_NOARG( PosSizePropertyPanel, ClickObjectEditHdl, weld::Button&, void )
{
    SfxViewShell* pCurSh = SfxViewShell::Current();
    if ( pCurSh)
    {
        pCurSh->DoVerb( -1 );
    }
}

namespace
{
    void limitWidth(weld::MetricSpinButton& rMetricSpinButton)
    {
        // space is limited in the sidebar, so limit MetricSpinButtons to a width of 7 digits
        const int nMaxDigits = 7;

        weld::SpinButton& rSpinButton = rMetricSpinButton.get_widget();
        rSpinButton.set_width_chars(std::min(rSpinButton.get_width_chars(), nMaxDigits));
    }
}

void PosSizePropertyPanel::NotifyItemUpdate(
    sal_uInt16 nSID,
    SfxItemState eState,
    const SfxPoolItem* pState)
{
    mxFtAngle->set_sensitive(true);
    mxMtrAngle->set_sensitive(true);
    mxDial->set_sensitive(true);
    mxFtFlip->set_sensitive(true);
    mxFlipTbx->set_sensitive(true);

    const SfxUInt32Item*    pWidthItem;
    const SfxUInt32Item*    pHeightItem;

    SfxViewShell* pCurSh = SfxViewShell::Current();
    if ( pCurSh )
        mpView = pCurSh->GetDrawView();
    else
        mpView = nullptr;

    if ( mpView == nullptr )
        return;

    mbAdjustEnabled = hasText(*mpView);

    // Pool unit and dialog unit may have changed, make sure that we
    // have the current values.
    mePoolUnit = maTransfWidthControl.GetCoreMetric();

    switch (nSID)
    {
        case SID_ATTR_TRANSFORM_WIDTH:
            if ( SfxItemState::DEFAULT == eState )
            {
                pWidthItem = dynamic_cast< const SfxUInt32Item* >(pState);

                if(pWidthItem)
                {
                    tools::Long lOldWidth1 = tools::Long( pWidthItem->GetValue() / maUIScale );
                    SetFieldUnit( *mxMtrWidth, meDlgUnit, true );
                    SetMetricValue( *mxMtrWidth, lOldWidth1, mePoolUnit );
                    limitWidth(*mxMtrWidth);
                    mlOldWidth = lOldWidth1;
                    mxMtrWidth->save_value();
                    if (mbMtrWidthBlanked)
                    {
                        mxMtrWidth->reformat();
                        mbMtrWidthBlanked = false;
                    }
                    break;
                }
            }
            mbMtrWidthBlanked = true;
            break;

        case SID_ATTR_TRANSFORM_HEIGHT:
            if ( SfxItemState::DEFAULT == eState )
            {
                pHeightItem = dynamic_cast< const SfxUInt32Item* >(pState);

                if(pHeightItem)
                {
                    tools::Long nTmp = tools::Long( pHeightItem->GetValue() / maUIScale);
                    SetFieldUnit( *mxMtrHeight, meDlgUnit, true );
                    SetMetricValue( *mxMtrHeight, nTmp, mePoolUnit );
                    limitWidth(*mxMtrHeight);
                    mlOldHeight = nTmp;
                    mxMtrHeight->save_value();
                    if (mbMtrHeightBlanked)
                    {
                        mxMtrHeight->reformat();
                        mbMtrHeightBlanked = false;
                    }
                    break;
                }
            }
            mbMtrHeightBlanked = true;
            break;

        case SID_ATTR_TRANSFORM_POS_X:
            if(SfxItemState::DEFAULT == eState)
            {
                const SfxInt32Item* pItem = dynamic_cast< const SfxInt32Item* >(pState);

                if(pItem)
                {
                    tools::Long nTmp = tools::Long(pItem->GetValue() / maUIScale);
                    SetFieldUnit( *mxMtrPosX, meDlgUnit, true );
                    SetMetricValue( *mxMtrPosX, nTmp, mePoolUnit );
                    limitWidth(*mxMtrPosX);
                    mxMtrPosX->save_value();
                    if (mbMtrPosXBlanked)
                    {
                        mxMtrPosX->reformat();
                        mbMtrPosXBlanked = false;
                    }
                    break;
                }
            }
            mbMtrPosXBlanked = true;
            break;

        case SID_ATTR_TRANSFORM_POS_Y:
            if(SfxItemState::DEFAULT == eState)
            {
                const SfxInt32Item* pItem = dynamic_cast< const SfxInt32Item* >(pState);

                if(pItem)
                {
                    tools::Long nTmp = tools::Long(pItem->GetValue() / maUIScale);
                    SetFieldUnit( *mxMtrPosY, meDlgUnit, true );
                    SetMetricValue( *mxMtrPosY, nTmp, mePoolUnit );
                    limitWidth(*mxMtrPosY);
                    mxMtrPosY->save_value();
                    if (mbMtrPosYBlanked)
                    {
                        mxMtrPosY->reformat();
                        mbMtrPosYBlanked = false;
                    }
                    break;
                }
            }
            mbMtrPosYBlanked = true;
            break;

        case SID_ATTR_TRANSFORM_ROT_X:
            if (SfxItemState::DEFAULT == eState)
            {
                const SfxInt32Item* pItem = dynamic_cast< const SfxInt32Item* >(pState);

                if(pItem)
                {
                    mlRotX = pItem->GetValue();
                    mlRotX = tools::Long( mlRotX / maUIScale );
                }
            }
            break;

        case SID_ATTR_TRANSFORM_ROT_Y:
            if (SfxItemState::DEFAULT == eState)
            {
                const SfxInt32Item* pItem = dynamic_cast< const SfxInt32Item* >(pState);

                if(pItem)
                {
                    mlRotY = pItem->GetValue();
                    mlRotY = tools::Long( mlRotY / maUIScale );
                }
            }
            break;

        case SID_ATTR_TRANSFORM_PROTECT_POS:
            if(SfxItemState::DEFAULT == eState)
            {
                const SfxBoolItem* pItem = dynamic_cast< const SfxBoolItem* >(pState);

                if(pItem)
                {
                    // record the state of position protect
                    mbPositionProtected = pItem->GetValue();
                    break;
                }
            }

            mbPositionProtected = false;
            break;

        case SID_ATTR_TRANSFORM_PROTECT_SIZE:
            if(SfxItemState::DEFAULT == eState)
            {
                const SfxBoolItem* pItem = dynamic_cast< const SfxBoolItem* >(pState);

                if(pItem)
                {
                    // record the state of size protect
                    mbSizeProtected = pItem->GetValue();
                    break;
                }
            }

            mbSizeProtected = false;
            break;

        case SID_ATTR_TRANSFORM_AUTOWIDTH:
            if(SfxItemState::DEFAULT == eState)
            {
                const SfxBoolItem* pItem = dynamic_cast< const SfxBoolItem* >(pState);

                if(pItem)
                {
                    mbAutoWidth = pItem->GetValue();
                }
            }
            break;

        case SID_ATTR_TRANSFORM_AUTOHEIGHT:
            if(SfxItemState::DEFAULT == eState)
            {
                const SfxBoolItem* pItem = dynamic_cast< const SfxBoolItem* >(pState);

                if(pItem)
                {
                    mbAutoHeight = pItem->GetValue();
                }
            }
            break;

        case SID_ATTR_TRANSFORM_ANGLE:
            if (eState >= SfxItemState::DEFAULT)
            {
                const SdrAngleItem* pItem = dynamic_cast< const SdrAngleItem* >(pState);

                if(pItem)
                {
                    Degree100 nTmp = NormAngle36000(pItem->GetValue());

                    mxMtrAngle->set_value(nTmp.get(), FieldUnit::DEGREE);
                    mxCtrlDial->SetRotation(nTmp);

                    if (mbMtrAngleBlanked)
                    {
                        mxMtrAngle->reformat();
                        mbMtrAngleBlanked = false;
                    }

                    break;
                }
            }
            mbMtrAngleBlanked = true;
            mxCtrlDial->SetRotation( 0_deg100 );
            break;

        case SID_ATTR_METRIC:
        {
            const Fraction aUIScale(mpView->GetModel().GetUIScale());
            MetricState(eState, pState, aUIScale);
            UpdateUIScale(aUIScale);
            mbFieldMetricOutDated = false;
            break;
        }
        default:
            break;
    }

    const sal_Int32 nCombinedContext(maContext.GetCombinedContext_DI());
    const SdrMarkList& rMarkList = mpView->GetMarkedObjectList();

    switch (rMarkList.GetMarkCount())
    {
        case 0:
            break;

        case 1:
        {
            const SdrObject* pObj = rMarkList.GetMark(0)->GetMarkedSdrObj();
            const SdrObjKind eKind(pObj->GetObjIdentifier());

            if(((nCombinedContext == CombinedEnumContext(Application::DrawImpress, Context::Draw)
               || nCombinedContext == CombinedEnumContext(Application::DrawImpress, Context::TextObject)
                 ) && SdrObjKind::Edge == eKind)
               || SdrObjKind::Caption == eKind)
            {
                mxFtAngle->set_sensitive(false);
                mxMtrAngle->set_sensitive(false);
                mxDial->set_sensitive(false);
                mxFlipTbx->set_sensitive(false);
                mxFtFlip->set_sensitive(false);
            }
            break;
        }

        default:
        {
            sal_uInt16 nMarkObj = 0;
            bool isNoEdge = true;

            while(isNoEdge && rMarkList.GetMark(nMarkObj))
            {
                const SdrObject* pObj = rMarkList.GetMark(nMarkObj)->GetMarkedSdrObj();
                const SdrObjKind eKind(pObj->GetObjIdentifier());

                if(((nCombinedContext == CombinedEnumContext(Application::DrawImpress, Context::Draw)
                  || nCombinedContext == CombinedEnumContext(Application::DrawImpress, Context::TextObject)
                     ) && SdrObjKind::Edge == eKind)
                  || SdrObjKind::Caption == eKind)
                {
                    isNoEdge = false;
                    break;
                }
                nMarkObj++;
            }

            if(!isNoEdge)
            {
                mxFtAngle->set_sensitive(false);
                mxMtrAngle->set_sensitive(false);
                mxDial->set_sensitive(false);
                mxFlipTbx->set_sensitive(false);
                mxFtFlip->set_sensitive(false);
            }
            break;
        }
    }

    if(nCombinedContext == CombinedEnumContext(Application::DrawImpress, Context::TextObject))
    {
        mxFlipTbx->set_sensitive(false);
        mxFtFlip->set_sensitive(false);
    }

    DisableControls();

    // mxCbxScale must synchronized with that on Position and Size tabpage on Shape Properties dialog
    SvtViewOptions aPageOpt(EViewType::TabPage, u"cui/ui/possizetabpage/PositionAndSize"_ustr);
    OUString  sUserData;
    css::uno::Any  aUserItem = aPageOpt.GetUserItem( USERITEM_NAME );
    OUString aTemp;
    if ( aUserItem >>= aTemp )
        sUserData = aTemp;
    mxCbxScale->set_active(static_cast<bool>(sUserData.toInt32()));
    m_xCbxScaleImg->set_from_icon_name(mxCbxScale->get_active() ? RID_SVXBMP_LOCKED : RID_SVXBMP_UNLOCKED);
}

void PosSizePropertyPanel::GetControlState(const sal_uInt16 nSID, boost::property_tree::ptree& rState)
{
    weld::MetricSpinButton* pControl = nullptr;
    switch (nSID)
    {
        case SID_ATTR_TRANSFORM_POS_X:
            pControl = mxMtrPosX.get();
            break;
        case SID_ATTR_TRANSFORM_POS_Y:
            pControl = mxMtrPosY.get();
            break;
        case SID_ATTR_TRANSFORM_WIDTH:
            pControl = mxMtrWidth.get();
            break;
        case SID_ATTR_TRANSFORM_HEIGHT:
            pControl = mxMtrHeight.get();
            break;
    }

    if (pControl && !pControl->get_text().isEmpty())
    {
        OUString sValue = Application::GetSettings().GetNeutralLocaleDataWrapper().
            getNum(pControl->get_value(pControl->get_unit()), pControl->get_digits(), false, false);
        rState.put(pControl->get_buildable_name().toUtf8().getStr(), sValue.toUtf8().getStr());
    }
}

void PosSizePropertyPanel::executeSize()
{
    if ( !mxMtrWidth->get_value_changed_from_saved() && !mxMtrHeight->get_value_changed_from_saved())
        return;

    Fraction aUIScale = mpView->GetModel().GetUIScale();

    // get Width
    double nWidth = static_cast<double>(mxMtrWidth->get_value(FieldUnit::MM_100TH));
    tools::Long lWidth = tools::Long(nWidth * static_cast<double>(aUIScale));
    lWidth = OutputDevice::LogicToLogic( lWidth, MapUnit::Map100thMM, mePoolUnit );
    lWidth = static_cast<tools::Long>(mxMtrWidth->denormalize( lWidth ));

    // get Height
    double nHeight = static_cast<double>(mxMtrHeight->get_value(FieldUnit::MM_100TH));
    tools::Long lHeight = tools::Long(nHeight * static_cast<double>(aUIScale));
    lHeight = OutputDevice::LogicToLogic( lHeight, MapUnit::Map100thMM, mePoolUnit );
    lHeight = static_cast<tools::Long>(mxMtrHeight->denormalize( lHeight ));

    // put Width & Height to itemset
    SfxUInt32Item aWidthItem( SID_ATTR_TRANSFORM_WIDTH, static_cast<sal_uInt32>(lWidth));
    SfxUInt32Item aHeightItem( SID_ATTR_TRANSFORM_HEIGHT, static_cast<sal_uInt32>(lHeight));
    SfxUInt16Item aPointItem (SID_ATTR_TRANSFORM_SIZE_POINT, sal_uInt16(RectPoint::LT));
    const sal_Int32 nCombinedContext(maContext.GetCombinedContext_DI());

    if( nCombinedContext == CombinedEnumContext(Application::WriterVariants, Context::Graphic)
        || nCombinedContext == CombinedEnumContext(Application::WriterVariants, Context::OLE)
        )
    {
        GetBindings()->GetDispatcher()->ExecuteList(SID_ATTR_TRANSFORM,
            SfxCallMode::RECORD, { &aWidthItem, &aHeightItem, &aPointItem });
    }
    else
    {
        if ( (mxMtrWidth->get_value_changed_from_saved()) && (mxMtrHeight->get_value_changed_from_saved()))
            GetBindings()->GetDispatcher()->ExecuteList(SID_ATTR_TRANSFORM,
                SfxCallMode::RECORD, { &aWidthItem, &aHeightItem, &aPointItem });
        else if( mxMtrWidth->get_value_changed_from_saved())
            GetBindings()->GetDispatcher()->ExecuteList(SID_ATTR_TRANSFORM,
                SfxCallMode::RECORD, { &aWidthItem, &aPointItem });
        else if ( mxMtrHeight->get_value_changed_from_saved())
            GetBindings()->GetDispatcher()->ExecuteList(SID_ATTR_TRANSFORM,
                SfxCallMode::RECORD, { &aHeightItem, &aPointItem });
    }
}

void PosSizePropertyPanel::DumpAsPropertyTree(tools::JsonWriter& rJsonWriter)
{
    if (meDlgUnit != GetCurrentUnit(SfxItemState::DEFAULT, nullptr))
    {
        mpBindings->Update( SID_ATTR_METRIC );
    }

    PanelLayout::DumpAsPropertyTree(rJsonWriter);
}

void PosSizePropertyPanel::MetricState(SfxItemState eState, const SfxPoolItem* pState, const Fraction& rUIScale)
{
    bool bPosXBlank = false;
    bool bPosYBlank = false;
    bool bWidthBlank = false;
    bool bHeightBlank = false;

    // #i124409# use the given Item to get the correct UI unit and initialize it
    // and the Fields using it
    FieldUnit eDlgUnit = GetCurrentUnit(eState, pState);
    mbFieldMetricOutDated |= (eDlgUnit != meDlgUnit || maUIScale != rUIScale);
    if (!mbFieldMetricOutDated)
        return;
    meDlgUnit = eDlgUnit;

    if (mxMtrPosX->get_text().isEmpty())
        bPosXBlank = true;
    SetFieldUnit( *mxMtrPosX, meDlgUnit, true );
    if (bPosXBlank)
    {
        mbMtrPosXBlanked = true;
    }

    if (mxMtrPosY->get_text().isEmpty())
        bPosYBlank = true;
    SetFieldUnit( *mxMtrPosY, meDlgUnit, true );
    if (bPosYBlank)
    {
        mbMtrPosYBlanked = true;
    }

    SetPosSizeMinMax(rUIScale);

    if (mxMtrWidth->get_text().isEmpty())
        bWidthBlank = true;
    SetFieldUnit( *mxMtrWidth, meDlgUnit, true );
    if (bWidthBlank)
    {
        mbMtrWidthBlanked = true;
    }

    if (mxMtrHeight->get_text().isEmpty())
        bHeightBlank = true;
    SetFieldUnit( *mxMtrHeight, meDlgUnit, true );
    if (bHeightBlank)
    {
        mbMtrHeightBlanked = true;
    }
}


FieldUnit PosSizePropertyPanel::GetCurrentUnit( SfxItemState eState, const SfxPoolItem* pState )
{
    FieldUnit eUnit = FieldUnit::NONE;

    if ( pState && eState >= SfxItemState::DEFAULT )
    {
        eUnit = static_cast<FieldUnit>(static_cast<const SfxUInt16Item*>(pState)->GetValue());
    }
    else
    {
        SfxViewFrame* pFrame = SfxViewFrame::Current();
        SfxObjectShell* pSh = nullptr;
        if ( pFrame )
            pSh = pFrame->GetObjectShell();
        if ( pSh )
        {
            SfxModule* pModule = pSh->GetModule();
            if ( pModule )
            {
                eUnit = pModule->GetFieldUnit();
            }
            else
            {
                SAL_WARN("svx.sidebar", "GetModuleFieldUnit(): no module found");
            }
        }
    }

    return eUnit;
}


void PosSizePropertyPanel::DisableControls()
{
    if( mbPositionProtected )
    {
        // the position is protected("Position protect" option in modal dialog is checked),
        // disable all the Position controls in sidebar
        mxFtPosX->set_sensitive(false);
        mxMtrPosX->set_sensitive(false);
        mxFtPosY->set_sensitive(false);
        mxMtrPosY->set_sensitive(false);
        mxFtAngle->set_sensitive(false);
        mxMtrAngle->set_sensitive(false);
        mxDial->set_sensitive(false);
        mxFtFlip->set_sensitive(false);
        mxFlipTbx->set_sensitive(false);

        mxFtWidth->set_sensitive(false);
        mxMtrWidth->set_sensitive(false);
        mxFtHeight->set_sensitive(false);
        mxMtrHeight->set_sensitive(false);
        mxCbxScale->set_sensitive(false);
    }
    else
    {
        mxFtPosX->set_sensitive(true);
        mxMtrPosX->set_sensitive(true);
        mxFtPosY->set_sensitive(true);
        mxMtrPosY->set_sensitive(true);

        if( mbSizeProtected )
        {
            mxFtWidth->set_sensitive(false);
            mxMtrWidth->set_sensitive(false);
            mxFtHeight->set_sensitive(false);
            mxMtrHeight->set_sensitive(false);
            mxCbxScale->set_sensitive(false);
        }
        else
        {
            if( mbAdjustEnabled )
            {
                if( mbAutoWidth )
                {
                    mxFtWidth->set_sensitive(false);
                    mxMtrWidth->set_sensitive(false);
                    mxCbxScale->set_sensitive(false);
                }
                else
                {
                    mxFtWidth->set_sensitive(true);
                    mxMtrWidth->set_sensitive(true);
                }
                if( mbAutoHeight )
                {
                    mxFtHeight->set_sensitive(false);
                    mxMtrHeight->set_sensitive(false);
                    mxCbxScale->set_sensitive(false);
                }
                else
                {
                    mxFtHeight->set_sensitive(true);
                    mxMtrHeight->set_sensitive(true);
                }
                if( !mbAutoWidth && !mbAutoHeight )
                    mxCbxScale->set_sensitive(true);
            }
            else
            {
                mxFtWidth->set_sensitive(true);
                mxMtrWidth->set_sensitive(true);
                mxFtHeight->set_sensitive(true);
                mxMtrHeight->set_sensitive(true);
                mxCbxScale->set_sensitive(true);
            }
        }
    }
}

void PosSizePropertyPanel::SetPosSizeMinMax(const Fraction& rUIScale)
{
    SdrPageView* pPV = mpView->GetSdrPageView();
    if (!pPV)
        return;
    tools::Rectangle aTmpRect(mpView->GetAllMarkedRect());
    pPV->LogicToPagePos(aTmpRect);
    maRect = vcl::unotools::b2DRectangleFromRectangle(aTmpRect);

    tools::Rectangle aTmpRect2(mpView->GetWorkArea());
    pPV->LogicToPagePos(aTmpRect2);
    maWorkArea = vcl::unotools::b2DRectangleFromRectangle(aTmpRect2);

    TransfrmHelper::ScaleRect(maWorkArea, rUIScale);
    TransfrmHelper::ScaleRect(maRect, rUIScale);

    const sal_uInt16 nDigits(mxMtrPosX->get_digits());
    TransfrmHelper::ConvertRect( maWorkArea, nDigits, mePoolUnit, meDlgUnit );
    TransfrmHelper::ConvertRect( maRect, nDigits, mePoolUnit, meDlgUnit );

    double fLeft(maWorkArea.getMinX());
    double fTop(maWorkArea.getMinY());
    double fRight(maWorkArea.getMaxX());
    double fBottom(maWorkArea.getMaxY());

    // seems that sidebar defaults to top left reference point
    // and there's no way to set it to something else
    fRight  -= maRect.getWidth();
    fBottom -= maRect.getHeight();

    const double fMaxLong(static_cast<double>(vcl::ConvertValue( LONG_MAX, 0, MapUnit::Map100thMM, meDlgUnit ) - 1));
    fLeft = std::clamp(fLeft, -fMaxLong, fMaxLong);
    fRight = std::clamp(fRight, -fMaxLong, fMaxLong);
    fTop = std::clamp(fTop, - fMaxLong, fMaxLong);
    fBottom = std::clamp(fBottom, -fMaxLong, fMaxLong);

    mxMtrPosX->set_range(basegfx::fround64(fLeft), basegfx::fround64(fRight), FieldUnit::NONE);
    limitWidth(*mxMtrPosX);
    mxMtrPosY->set_range(basegfx::fround64(fTop), basegfx::fround64(fBottom), FieldUnit::NONE);
    limitWidth(*mxMtrPosY);

    double fMaxWidth = maWorkArea.getWidth() - (maRect.getWidth() - fLeft);
    double fMaxHeight = maWorkArea.getHeight() - (maRect.getHeight() - fTop);
    mxMtrWidth->set_max(std::min<sal_Int64>(INT_MAX, basegfx::fround64(fMaxWidth*100)), FieldUnit::NONE);
    limitWidth(*mxMtrWidth);
    mxMtrHeight->set_max(std::min<sal_Int64>(INT_MAX, basegfx::fround64(fMaxHeight*100)), FieldUnit::NONE);
    limitWidth(*mxMtrHeight);
}

void PosSizePropertyPanel::UpdateUIScale(const Fraction& rUIScale)
{
    if (maUIScale == rUIScale)
        return;

    // UI scale has changed.

    // Remember the new UI scale.
    maUIScale = rUIScale;

    // The content of the position and size boxes is only updated when item changes are notified.
    // Request such notifications without changing the actual item values.
    GetBindings()->Invalidate(SID_ATTR_TRANSFORM_POS_X, true);
    GetBindings()->Invalidate(SID_ATTR_TRANSFORM_POS_Y, true);
    GetBindings()->Invalidate(SID_ATTR_TRANSFORM_WIDTH, true);
    GetBindings()->Invalidate(SID_ATTR_TRANSFORM_HEIGHT, true);
}


} // end of namespace svx::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
