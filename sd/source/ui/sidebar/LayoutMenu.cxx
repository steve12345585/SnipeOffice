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

#include "LayoutMenu.hxx"

#include <app.hrc>
#include <drawdoc.hxx>
#include <framework/FrameworkHelper.hxx>
#include <strings.hrc>
#include <helpids.h>
#include <pres.hxx>
#include <sdmod.hxx>

#include <sdpage.hxx>
#include <sdresid.hxx>
#include <unokywds.hxx>
#include <bitmaps.hlst>
#include <tools/gen.hxx>
#include <tools/SlotStateListener.hxx>
#include <DrawController.hxx>
#include <DrawDocShell.hxx>
#include <DrawViewShell.hxx>
#include <EventMultiplexer.hxx>
#include <SlideSorterViewShell.hxx>
#include <ViewShellBase.hxx>
#include <sfx2/sidebar/Theme.hxx>
#include <sal/log.hxx>

#include <comphelper/processfactory.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/request.hxx>
#include <sfx2/viewfrm.hxx>
#include <svl/cjkoptions.hxx>
#include <svl/stritem.hxx>
#include <svl/intitem.hxx>
#include <utility>
#include <vcl/commandevent.hxx>
#include <vcl/image.hxx>
#include <xmloff/autolayout.hxx>

#include <com/sun/star/drawing/framework/XControllerManager.hpp>
#include <com/sun/star/drawing/framework/XView.hpp>
#include <com/sun/star/drawing/framework/ResourceId.hpp>

#include <string_view>
#include <vector>

using namespace ::com::sun::star;
using namespace ::com::sun::star::text;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::drawing::framework;
using namespace ::sd::slidesorter;
using ::sd::framework::FrameworkHelper;

namespace sd::sidebar {

namespace {

struct snew_slide_value_info
{
    OUString msBmpResId;
    TranslateId mpStrResId;
    WritingMode meWritingMode;
    AutoLayout maAutoLayout;
};

}

constexpr snew_slide_value_info notes[] =
{
    {BMP_SLIDEN_01, STR_AUTOLAYOUT_NOTES, WritingMode_LR_TB,
     AUTOLAYOUT_NOTES},
};

constexpr snew_slide_value_info handout[] =
{
    {BMP_SLIDEH_01, STR_AUTOLAYOUT_HANDOUT1, WritingMode_LR_TB,
     AUTOLAYOUT_HANDOUT1},
    {BMP_SLIDEH_02, STR_AUTOLAYOUT_HANDOUT2, WritingMode_LR_TB,
     AUTOLAYOUT_HANDOUT2},
    {BMP_SLIDEH_03, STR_AUTOLAYOUT_HANDOUT3, WritingMode_LR_TB,
     AUTOLAYOUT_HANDOUT3},
    {BMP_SLIDEH_04, STR_AUTOLAYOUT_HANDOUT4, WritingMode_LR_TB,
     AUTOLAYOUT_HANDOUT4},
    {BMP_SLIDEH_06, STR_AUTOLAYOUT_HANDOUT6, WritingMode_LR_TB,
     AUTOLAYOUT_HANDOUT6},
    {BMP_SLIDEH_09, STR_AUTOLAYOUT_HANDOUT9, WritingMode_LR_TB,
     AUTOLAYOUT_HANDOUT9},
};

constexpr snew_slide_value_info standard[] =
{
    {BMP_LAYOUT_EMPTY, STR_AUTOLAYOUT_NONE, WritingMode_LR_TB,        AUTOLAYOUT_NONE},
    {BMP_LAYOUT_HEAD03, STR_AUTOLAYOUT_TITLE, WritingMode_LR_TB,       AUTOLAYOUT_TITLE},
    {BMP_LAYOUT_HEAD02, STR_AUTOLAYOUT_CONTENT, WritingMode_LR_TB,        AUTOLAYOUT_TITLE_CONTENT},
    {BMP_LAYOUT_HEAD02A, STR_AUTOLAYOUT_2CONTENT, WritingMode_LR_TB,       AUTOLAYOUT_TITLE_2CONTENT},
    {BMP_LAYOUT_HEAD01, STR_AUTOLAYOUT_ONLY_TITLE, WritingMode_LR_TB,  AUTOLAYOUT_TITLE_ONLY},
    {BMP_LAYOUT_TEXTONLY, STR_AUTOLAYOUT_ONLY_TEXT, WritingMode_LR_TB,   AUTOLAYOUT_ONLY_TEXT},
    {BMP_LAYOUT_HEAD03B, STR_AUTOLAYOUT_2CONTENT_CONTENT, WritingMode_LR_TB,    AUTOLAYOUT_TITLE_2CONTENT_CONTENT},
    {BMP_LAYOUT_HEAD03C, STR_AUTOLAYOUT_CONTENT_2CONTENT, WritingMode_LR_TB,    AUTOLAYOUT_TITLE_CONTENT_2CONTENT},
    {BMP_LAYOUT_HEAD03A, STR_AUTOLAYOUT_2CONTENT_OVER_CONTENT,WritingMode_LR_TB, AUTOLAYOUT_TITLE_2CONTENT_OVER_CONTENT},
    {BMP_LAYOUT_HEAD02B, STR_AUTOLAYOUT_CONTENT_OVER_CONTENT, WritingMode_LR_TB, AUTOLAYOUT_TITLE_CONTENT_OVER_CONTENT},
    {BMP_LAYOUT_HEAD04, STR_AUTOLAYOUT_4CONTENT, WritingMode_LR_TB,        AUTOLAYOUT_TITLE_4CONTENT},
    {BMP_LAYOUT_HEAD06, STR_AUTOLAYOUT_6CONTENT, WritingMode_LR_TB,    AUTOLAYOUT_TITLE_6CONTENT},

    // vertical
    {BMP_LAYOUT_VERTICAL02, STR_AL_VERT_TITLE_TEXT_CHART, WritingMode_TB_RL, AUTOLAYOUT_VTITLE_VCONTENT_OVER_VCONTENT},
    {BMP_LAYOUT_VERTICAL01, STR_AL_VERT_TITLE_VERT_OUTLINE, WritingMode_TB_RL, AUTOLAYOUT_VTITLE_VCONTENT},
    {BMP_LAYOUT_HEAD02, STR_AL_TITLE_VERT_OUTLINE, WritingMode_TB_RL, AUTOLAYOUT_TITLE_VCONTENT},
    {BMP_LAYOUT_HEAD02A, STR_AL_TITLE_VERT_OUTLINE_CLIPART,   WritingMode_TB_RL, AUTOLAYOUT_TITLE_2VTEXT},
};

class LayoutValueSet : public ValueSet
{
private:
    LayoutMenu& mrMenu;

    /** Calculate the number of displayed rows.  This depends on the given
        item size, the given number of columns, and the size of the
        control.  Note that this is not the number of rows managed by the
        valueset.  This number may be larger.  In that case a vertical
        scroll bar is displayed.
    */
    int CalculateRowCount(const Size& rItemSize, int nColumnCount);

public:
    LayoutValueSet(LayoutMenu& rMenu)
        : ValueSet(nullptr)
        , mrMenu(rMenu)
    {
    }

    virtual void Resize() override;

    virtual bool Command(const CommandEvent& rEvent) override;
};

LayoutMenu::LayoutMenu (
    weld::Widget* pParent,
    ViewShellBase& rViewShellBase,
    css::uno::Reference<css::ui::XSidebar> xSidebar)
    : PanelLayout( pParent, u"LayoutPanel"_ustr, u"modules/simpress/ui/layoutpanel.ui"_ustr ),
      mrBase(rViewShellBase),
      mxLayoutValueSet(new LayoutValueSet(*this)),
      mxLayoutValueSetWin(new weld::CustomWeld(*m_xBuilder, u"layoutvalueset"_ustr, *mxLayoutValueSet)),
      mbIsMainViewChangePending(false),
      mxSidebar(std::move(xSidebar)),
      mbIsDisposed(false)
{
    implConstruct( *mrBase.GetDocument()->GetDocSh() );
    SAL_INFO("sd.ui", "created LayoutMenu at " << this);

    mxLayoutValueSet->SetStyle(mxLayoutValueSet->GetStyle() | WB_ITEMBORDER | WB_FLATVALUESET | WB_TABSTOP);

    mxLayoutValueSet->SetColor(sfx2::sidebar::Theme::GetColor(sfx2::sidebar::Theme::Color_PanelBackground));
}

void LayoutMenu::implConstruct( DrawDocShell& rDocumentShell )
{
    OSL_ENSURE( mrBase.GetDocument()->GetDocSh() == &rDocumentShell,
        "LayoutMenu::implConstruct: hmm?" );
    // if this fires, then my assumption that the rDocumentShell parameter to our first ctor is superfluous ...
    (void) rDocumentShell;

    mxLayoutValueSet->SetStyle (
        ( mxLayoutValueSet->GetStyle()  & ~(WB_ITEMBORDER) )
        | WB_TABSTOP
        | WB_MENUSTYLEVALUESET
        | WB_NO_DIRECTSELECT
        );
    mxLayoutValueSet->SetExtraSpacing(2);
    mxLayoutValueSet->SetSelectHdl (LINK(this, LayoutMenu, ClickHandler));
    InvalidateContent();

    Link<::sd::tools::EventMultiplexerEvent&,void> aEventListenerLink (LINK(this,LayoutMenu,EventMultiplexerListener));
    mrBase.GetEventMultiplexer()->AddEventListener(aEventListenerLink);

    mxLayoutValueSet->SetHelpId(HID_SD_TASK_PANE_PREVIEW_LAYOUTS);
    mxLayoutValueSet->SetAccessibleName(SdResId(STR_TASKPANEL_LAYOUT_MENU_TITLE));

    Link<const OUString&,void> aStateChangeLink (LINK(this,LayoutMenu,StateChangeHandler));
    mxListener = new ::sd::tools::SlotStateListener(
        aStateChangeLink,
        Reference<frame::XDispatchProvider>(mrBase.GetController()->getFrame(), UNO_QUERY),
        u".uno:VerticalTextState"_ustr);
}

LayoutMenu::~LayoutMenu()
{
    SAL_INFO("sd.ui", "destroying LayoutMenu at " << this);
    Dispose();
    mxLayoutValueSetWin.reset();
    mxLayoutValueSet.reset();
}

void LayoutMenu::Dispose()
{
    if (mbIsDisposed)
        return;

    SAL_INFO("sd.ui", "disposing LayoutMenu at " << this);

    mbIsDisposed = true;

    if (mxListener.is())
        mxListener->dispose();

    Clear();
    Link<tools::EventMultiplexerEvent&,void> aLink (LINK(this,LayoutMenu,EventMultiplexerListener));
    mrBase.GetEventMultiplexer()->RemoveEventListener (aLink);
}

AutoLayout LayoutMenu::GetSelectedAutoLayout() const
{
    AutoLayout aResult = AUTOLAYOUT_NONE;

    if (!mxLayoutValueSet->IsNoSelection() && mxLayoutValueSet->GetSelectedItemId()!=0)
    {
        AutoLayout* pLayout = static_cast<AutoLayout*>(mxLayoutValueSet->GetItemData(mxLayoutValueSet->GetSelectedItemId()));
        if (pLayout != nullptr)
            aResult = *pLayout;
    }

    return aResult;
}

ui::LayoutSize LayoutMenu::GetHeightForWidth (const sal_Int32 nWidth)
{
    sal_Int32 nPreferredHeight = 200;
    if (mxLayoutValueSet->GetItemCount()>0)
    {
        Image aImage = mxLayoutValueSet->GetItemImage(mxLayoutValueSet->GetItemId(0));
        Size aItemSize = mxLayoutValueSet->CalcItemSizePixel(aImage.GetSizePixel());
        if (nWidth>0 && aItemSize.Width()>0)
        {
            aItemSize.AdjustWidth(8 );
            aItemSize.AdjustHeight(8 );
            int nColumnCount = nWidth / aItemSize.Width();
            if (nColumnCount <= 0)
                nColumnCount = 1;
            else if (nColumnCount > 4)
                nColumnCount = 4;
            int nRowCount = (mxLayoutValueSet->GetItemCount() + nColumnCount-1) / nColumnCount;
            nPreferredHeight = nRowCount * aItemSize.Height();
        }
    }
    return ui::LayoutSize(nPreferredHeight,nPreferredHeight,nPreferredHeight);
}

void LayoutValueSet::Resize()
{
    Size aWindowSize = GetOutputSizePixel();
    if (IsVisible() && aWindowSize.Width() > 0)
    {
        // Calculate the number of rows and columns.
        if (GetItemCount() > 0)
        {
            Image aImage = GetItemImage(GetItemId(0));
            Size aItemSize = CalcItemSizePixel (
                aImage.GetSizePixel());
            aItemSize.AdjustWidth(8 );
            aItemSize.AdjustHeight(8 );
            int nColumnCount = aWindowSize.Width() / aItemSize.Width();
            if (nColumnCount < 1)
                nColumnCount = 1;
            else if (nColumnCount > 4)
                nColumnCount = 4;

            int nRowCount = CalculateRowCount (aItemSize, nColumnCount);

            SetColCount(nColumnCount);
            SetLineCount(nRowCount);
        }
    }

    ValueSet::Resize();
}

bool LayoutValueSet::Command(const CommandEvent& rEvent)
{
    if (rEvent.GetCommand() != CommandEventId::ContextMenu)
        return false;

    // As a preparation for the context menu the item under the mouse is
    // selected.
    if (rEvent.IsMouseEvent())
    {
        sal_uInt16 nIndex = GetItemId(rEvent.GetMousePosPixel());
        if (nIndex > 0)
            SelectItem(nIndex);
    }

    mrMenu.ShowContextMenu(rEvent.IsMouseEvent() ? &rEvent.GetMousePosPixel() : nullptr);
    return true;
}

void LayoutMenu::InsertPageWithLayout (AutoLayout aLayout)
{
    ViewShell* pViewShell = mrBase.GetMainViewShell().get();
    if (pViewShell == nullptr)
        return;

    SfxViewFrame& rViewFrame = mrBase.GetViewFrame();
    SfxDispatcher* pDispatcher = rViewFrame.GetDispatcher();
    if (pDispatcher == nullptr)
        return;

    // Call SID_INSERTPAGE with the right arguments.  This is because
    // the popup menu can not call this slot with arguments directly.
    SfxRequest aRequest (CreateRequest(SID_INSERTPAGE, aLayout));
    if (aRequest.GetArgs() != nullptr)
    {
        pDispatcher->Execute(
            SID_INSERTPAGE,
            SfxCallMode::ASYNCHRON | SfxCallMode::RECORD,
            *aRequest.GetArgs());
    }
    UpdateSelection();
}

void LayoutMenu::InvalidateContent()
{
    // Throw away the current set and fill the menu anew according to the
    // current settings (this includes the support for vertical writing.)
    Fill();

    if (mxSidebar.is())
        mxSidebar->requestLayout();

    // set selection inside the control during Impress start up
    UpdateSelection();
}

int LayoutValueSet::CalculateRowCount (const Size&, int nColumnCount)
{
    int nRowCount = 0;

    if (GetItemCount() > 0 && nColumnCount > 0)
    {
        nRowCount = (GetItemCount() + nColumnCount - 1) / nColumnCount;
        if (nRowCount < 1)
            nRowCount = 1;
    }

    return nRowCount;
}

IMPL_LINK_NOARG(LayoutMenu, ClickHandler, ValueSet*, void)
{
    AssignLayoutToSelectedSlides( GetSelectedAutoLayout() );
}

/** The specified layout is assigned to the current page of the view shell
    in the center pane.
*/
void LayoutMenu::AssignLayoutToSelectedSlides (AutoLayout aLayout)
{
    using namespace ::sd::slidesorter;
    using namespace ::sd::slidesorter::controller;

    do
    {
        // The view shell in the center pane has to be present.
        ViewShell* pMainViewShell = mrBase.GetMainViewShell().get();
        if (pMainViewShell == nullptr)
            break;

        // Determine if the current view is in an invalid master page mode.
        // The handout view is always in master page mode and therefore not
        // invalid.
        bool bMasterPageMode (false);
        switch (pMainViewShell->GetShellType())
        {
            case ViewShell::ST_NOTES:
            case ViewShell::ST_IMPRESS:
            {
                DrawViewShell* pDrawViewShell = static_cast<DrawViewShell*>(pMainViewShell);
                if (pDrawViewShell->GetEditMode() == EditMode::MasterPage)
                    bMasterPageMode = true;
                break;
            }
            default:
                break;
        }
        if (bMasterPageMode)
            break;

        // Get a list of all selected slides and call the SID_MODIFYPAGE
        // slot for all of them.
        ::sd::slidesorter::SharedPageSelection pPageSelection;

        // Get a list of selected pages.
        // First we try to obtain this list from a slide sorter.  This is
        // possible only some of the view shells in the center pane.  When
        // no valid slide sorter is available then ask the main view shell
        // for its current page.
        SlideSorterViewShell* pSlideSorter = nullptr;
        switch (pMainViewShell->GetShellType())
        {
            case ViewShell::ST_IMPRESS:
            case ViewShell::ST_NOTES:
            case ViewShell::ST_SLIDE_SORTER:
                pSlideSorter = SlideSorterViewShell::GetSlideSorter(mrBase);
                break;
            default:
                break;
        }
        if (pSlideSorter != nullptr)
        {
            // There is a slide sorter visible so get the list of selected pages from it.
            pPageSelection = pSlideSorter->GetPageSelection();
        }

        if( (pSlideSorter == nullptr) || !pPageSelection || pPageSelection->empty() )
        {
            // No valid slide sorter available.  Ask the main view shell for
            // its current page.
            pPageSelection = std::make_shared<::sd::slidesorter::SlideSorterViewShell::PageSelection>();
            pPageSelection->push_back(pMainViewShell->GetActualPage());
        }

        if (pPageSelection->empty())
            break;

        for (const auto& rpPage : *pPageSelection)
        {
            if (rpPage == nullptr)
                continue;

            // Call the SID_ASSIGN_LAYOUT slot with all the necessary parameters.
            SfxRequest aRequest(mrBase.GetViewFrame(), SID_ASSIGN_LAYOUT);
            aRequest.AppendItem(SfxUInt32Item (ID_VAL_WHATPAGE, (rpPage->GetPageNum()-1)/2));
            aRequest.AppendItem(SfxUInt32Item (ID_VAL_WHATLAYOUT, aLayout));
            pMainViewShell->ExecuteSlot (aRequest, false);
        }
    }
    while(false);
}

SfxRequest LayoutMenu::CreateRequest (
    sal_uInt16 nSlotId,
    AutoLayout aLayout)
{
    SfxRequest aRequest(mrBase.GetViewFrame(), nSlotId);

    do
    {
        SdrLayerAdmin& rLayerAdmin (mrBase.GetDocument()->GetLayerAdmin());
        SdrLayerID aBackground (rLayerAdmin.GetLayerID(sUNO_LayerName_background));
        SdrLayerID aBackgroundObject (rLayerAdmin.GetLayerID(sUNO_LayerName_background_objects));
        ViewShell* pViewShell = mrBase.GetMainViewShell().get();
        if (pViewShell == nullptr)
            break;
        SdPage* pPage = pViewShell->GetActualPage();
        if (pPage == nullptr)
            break;

        SdrLayerIDSet aVisibleLayers (pPage->TRG_GetMasterPageVisibleLayers());

        aRequest.AppendItem(
            SfxStringItem (ID_VAL_PAGENAME, OUString()));//pPage->GetName()));
        aRequest.AppendItem(SfxUInt32Item (ID_VAL_WHATLAYOUT, aLayout));
        aRequest.AppendItem(
            SfxBoolItem(ID_VAL_ISPAGEBACK, aVisibleLayers.IsSet(aBackground)));
        aRequest.AppendItem(
            SfxBoolItem(
                ID_VAL_ISPAGEOBJ,
                aVisibleLayers.IsSet(aBackgroundObject)));
    }
    while (false);

    return aRequest;
}

void LayoutMenu::Fill()
{
    bool bVertical = SvtCJKOptions::IsVerticalTextEnabled();
    SdDrawDocument* pDocument = mrBase.GetDocument();
    bool bRightToLeft = (pDocument!=nullptr
        && pDocument->GetDefaultWritingMode() == WritingMode_RL_TB);

    // Get URL of the view in the center pane.
    OUString sCenterPaneViewName;
    try
    {
        if (mrBase.GetDrawController())
        {
            Reference<XResourceId> xPaneId (ResourceId::create(
                ::comphelper::getProcessComponentContext(),
                FrameworkHelper::msCenterPaneURL));
            Reference<XView> xView (FrameworkHelper::Instance(mrBase)->GetView(xPaneId));
            if (xView.is())
                sCenterPaneViewName = xView->getResourceId()->getResourceURL();
        }
    }
    catch (RuntimeException&)
    {}

    std::span<const snew_slide_value_info> pInfo;
    if (sCenterPaneViewName == framework::FrameworkHelper::msNotesViewURL)
    {
        pInfo = notes;
    }
    else if (sCenterPaneViewName == framework::FrameworkHelper::msHandoutViewURL)
    {
        pInfo = handout;
    }
    else if (sCenterPaneViewName == framework::FrameworkHelper::msImpressViewURL
        || sCenterPaneViewName == framework::FrameworkHelper::msSlideSorterURL)
    {
        pInfo = standard;
    }

    Clear();
    sal_uInt16 id = 1;
    for (const auto& elem : pInfo)
    {
        if ((WritingMode_TB_RL != elem.meWritingMode) || bVertical)
        {
            Image aImg(OUString::Concat("private:graphicrepository/") + elem.msBmpResId);

            if (bRightToLeft && (WritingMode_TB_RL != elem.meWritingMode))
            { // FIXME: avoid interpolating RTL layouts.
                BitmapEx aRTL = aImg.GetBitmapEx();
                aRTL.Mirror(BmpMirrorFlags::Horizontal);
                aImg = Image(aRTL);
            }

            mxLayoutValueSet->InsertItem(id, aImg, SdResId(elem.mpStrResId));
            mxLayoutValueSet->SetItemData(id, new AutoLayout(elem.maAutoLayout));
            ++id;
        }
    }
}

void LayoutMenu::Clear()
{
    for (size_t nId=1; nId<=mxLayoutValueSet->GetItemCount(); nId++)
        delete static_cast<AutoLayout*>(mxLayoutValueSet->GetItemData(nId));
    mxLayoutValueSet->Clear();
}

void LayoutMenu::ShowContextMenu(const Point* pPos)
{
    if (SdModule::get()->GetWaterCan())
        return;

    // Determine the position where to show the menu.
    Point aMenuPosition;
    if (pPos)
    {
        auto nItemId = mxLayoutValueSet->GetItemId(*pPos);
        if (nItemId <= 0)
            return;
        mxLayoutValueSet->SelectItem(nItemId);
        aMenuPosition = *pPos;
    }
    else
    {
        if (mxLayoutValueSet->GetSelectedItemId() == sal_uInt16(-1))
            return;
        ::tools::Rectangle aBBox(mxLayoutValueSet->GetItemRect(mxLayoutValueSet->GetSelectedItemId()));
        aMenuPosition = aBBox.Center();
    }

    // Setup the menu.
    ::tools::Rectangle aRect(aMenuPosition, Size(1, 1));
    weld::Widget* pPopupParent = mxLayoutValueSet->GetDrawingArea();
    std::unique_ptr<weld::Builder> xBuilder(Application::CreateBuilder(pPopupParent, u"modules/simpress/ui/layoutmenu.ui"_ustr));
    std::unique_ptr<weld::Menu> xMenu(xBuilder->weld_menu(u"menu"_ustr));

    // Disable the SID_INSERTPAGE_LAYOUT_MENU item when
    // the document is read-only.
    SfxPoolItemHolder aResult;
    const SfxItemState aState (
        mrBase.GetViewFrame().GetDispatcher()->QueryState(SID_INSERTPAGE, aResult));
    if (aState == SfxItemState::DISABLED)
        xMenu->set_sensitive(u"insert"_ustr, false);

    // Show the menu.
    OnMenuItemSelected(xMenu->popup_at_rect(pPopupParent, aRect));
}

IMPL_LINK_NOARG(LayoutMenu, StateChangeHandler, const OUString&, void)
{
    InvalidateContent();
}

void LayoutMenu::OnMenuItemSelected(std::u16string_view ident)
{
    if (ident.empty())
        return;

    if (ident == u"apply")
    {
        AssignLayoutToSelectedSlides(GetSelectedAutoLayout());
    }
    else if (ident == u"insert")
    {
        // Add arguments to this slot and forward it to the main view
        // shell.
        InsertPageWithLayout(GetSelectedAutoLayout());
    }
}

// Selects an appropriate layout of the slide inside control.
//
// Method may be called several times with the same item-id to be selected -
// only once the actually state of the control will be changed.
//
void LayoutMenu::UpdateSelection()
{
    bool bItemSelected = false;

    do
    {
        // Get current page of main view.
        ViewShell* pViewShell = mrBase.GetMainViewShell().get();
        if (pViewShell == nullptr)
            break;

        SdPage* pCurrentPage = pViewShell->getCurrentPage();
        if (pCurrentPage == nullptr)
            break;

        // Get layout of current page.
        AutoLayout aLayout (pCurrentPage->GetAutoLayout());
        if (aLayout<AUTOLAYOUT_START || aLayout>AUTOLAYOUT_END)
            break;

        // Find the entry of the menu for to the layout.
        const sal_uInt16 nItemCount = mxLayoutValueSet->GetItemCount();
        for (sal_uInt16 nId=1; nId<=nItemCount; nId++)
        {
            if (*static_cast<AutoLayout*>(mxLayoutValueSet->GetItemData(nId)) == aLayout)
            {
                // do not set selection twice to the same item
                if (mxLayoutValueSet->GetSelectedItemId() != nId)
                {
                    mxLayoutValueSet->SetNoSelection();
                    mxLayoutValueSet->SelectItem(nId);
                }

                bItemSelected = true; // no need to call SetNoSelection()
                break;
            }
        }
    }
    while (false);

    if (!bItemSelected)
        mxLayoutValueSet->SetNoSelection();
}

IMPL_LINK(LayoutMenu, EventMultiplexerListener, ::sd::tools::EventMultiplexerEvent&, rEvent, void)
{
    switch (rEvent.meEventId)
    {
        // tdf#89890 During changes of the Layout of the slide when focus is not set inside main area
        // we do not receive notification EventMultiplexerEventId::CurrentPageChanged, but we receive the following 3 notification types.
        // => let's make UpdateSelection() also when some shape is changed (during Layout changes)
        case EventMultiplexerEventId::ShapeChanged:
        case EventMultiplexerEventId::ShapeInserted:
        case EventMultiplexerEventId::ShapeRemoved:
        case EventMultiplexerEventId::CurrentPageChanged:
        case EventMultiplexerEventId::SlideSortedSelection:
            UpdateSelection();
            break;

        case EventMultiplexerEventId::MainViewAdded:
            mbIsMainViewChangePending = true;
            break;

        case EventMultiplexerEventId::MainViewRemoved:
            mxLayoutValueSet->Invalidate(); // redraw without focus
            break;

        case EventMultiplexerEventId::ConfigurationUpdated:
            if (mbIsMainViewChangePending)
            {
                mbIsMainViewChangePending = false;
                InvalidateContent();
            }
            break;

        default:
            break;
    }
}

void LayoutMenu::DataChanged(const DataChangedEvent& rEvent)
{
    PanelLayout::DataChanged(rEvent);
    Fill();
    mxLayoutValueSet->StyleUpdated();
    mxLayoutValueSet->SetColor(sfx2::sidebar::Theme::GetColor(sfx2::sidebar::Theme::Color_PanelBackground));
}

} // end of namespace ::sd::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
