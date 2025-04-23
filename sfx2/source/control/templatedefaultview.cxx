/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4     -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <templatedefaultview.hxx>
#include <sfx2/thumbnailview.hxx>
#include <templateviewitem.hxx>
#include <sfx2/sfxresid.hxx>
#include <vcl/event.hxx>
#include <vcl/svapp.hxx>

#include <sfx2/strings.hrc>

#include <officecfg/Office/Common.hxx>

constexpr int gnItemPadding(5); //TODO:: Change padding to 10. It looks really crowded and occupied.
constexpr tools::Long gnTextHeight = 30;

TemplateDefaultView::TemplateDefaultView(std::unique_ptr<weld::ScrolledWindow> xWindow,
                                         std::unique_ptr<weld::Menu> xMenu)
    : TemplateLocalView(std::move(xWindow), std::move(xMenu))
{
    mbAllowMultiSelection = false;
    AbsoluteScreenPixelRectangle aScreen = Application::GetScreenPosSizePixel(Application::GetDisplayBuiltInScreen());
    tools::Long nItemMaxSize = std::min(aScreen.GetWidth(),aScreen.GetHeight()) > 800 ? 256 : 192;
    ThumbnailView::setItemDimensions( nItemMaxSize, nItemMaxSize, gnTextHeight, gnItemPadding );
    updateThumbnailDimensions(nItemMaxSize);

    // startcenter specific settings
    maFillColor = Color(ColorTransparency, officecfg::Office::Common::Help::StartCenter::StartCenterThumbnailsBackgroundColor::get());
    maTextColor = Color(ColorTransparency, officecfg::Office::Common::Help::StartCenter::StartCenterThumbnailsTextColor::get());

    const StyleSettings& rSettings = Application::GetSettings().GetStyleSettings();
    maHighlightColor = rSettings.GetHighlightColor();
    maHighlightTextColor = rSettings.GetHighlightTextColor();

    mfHighlightTransparence = 0.75;

    UpdateColors();
}

void TemplateDefaultView::showAllTemplates()
{
    mnCurRegionId = 0;

    insertItems(maAllTemplates, false);
}

bool TemplateDefaultView::KeyInput( const KeyEvent& rKEvt )
{
    return ThumbnailView::KeyInput(rKEvt);
}

bool TemplateDefaultView::MouseButtonDown( const MouseEvent& rMEvt )
{
    if( rMEvt.IsLeft() && rMEvt.GetClicks() == 1 )
    {
        size_t nPos = ImplGetItem(rMEvt.GetPosPixel());
        ThumbnailViewItem* pItem = ImplGetItem(nPos);
        TemplateViewItem* pViewItem = dynamic_cast<TemplateViewItem*>(pItem);
        if(pViewItem)
            maOpenTemplateHdl.Call(pViewItem);
        return true;
    }

    return TemplateLocalView::MouseButtonDown(rMEvt);
}

void TemplateDefaultView::createContextMenu()
{
    mxContextMenu->clear();
    mxContextMenu->append(u"open"_ustr,SfxResId(STR_OPEN));
    mxContextMenu->append(u"edit"_ustr,SfxResId(STR_EDIT_TEMPLATE));
    deselectItems();
    maSelectedItem->setSelection(true);
    maItemStateHdl.Call(maSelectedItem);
    ContextMenuSelectHdl(mxContextMenu->popup_at_rect(GetDrawingArea(), tools::Rectangle(maPosition, Size(1,1))));
    Invalidate();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
