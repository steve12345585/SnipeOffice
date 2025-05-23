/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#pragma once

#include <com/sun/star/frame/XFrame.hpp>
#include <sfx2/sidebar/PanelLayout.hxx>
#include <sfx2/sidebar/ControllerItem.hxx>
#include <sfx2/weldutils.hxx>
#include <svx/relfld.hxx>

namespace sw::sidebar
{
class TableEditPanel : public PanelLayout,
                       public sfx2::sidebar::ControllerItem::ItemUpdateReceiverInterface
{
public:
    static std::unique_ptr<PanelLayout>
    Create(weld::Widget* pParent, const css::uno::Reference<css::frame::XFrame>& rxFrame,
           SfxBindings* pBindings);

    TableEditPanel(weld::Widget* pParent, const css::uno::Reference<css::frame::XFrame>& rxFrame,
                   SfxBindings* pBindings);
    virtual ~TableEditPanel() override;

    virtual void NotifyItemUpdate(const sal_uInt16 nSId, const SfxItemState eState,
                                  const SfxPoolItem* pState) override;

    virtual void GetControlState(const sal_uInt16 /*nSId*/,
                                 boost::property_tree::ptree& /*rState*/) override{};

private:
    void InitRowHeightToolitem();
    void InitColumnWidthToolitem();
    void InitAlignmentControls();
    void EnableLeftRight(sal_uInt16 nAlignment);
    void AlignmentModify(bool alignmentChanged);

    SfxBindings* m_pBindings;

    SvxRelativeField m_aRowHeightEdit;
    SvxRelativeField m_aColumnWidthEdit;
    std::unique_ptr<weld::Toolbar> m_xInsert;
    std::unique_ptr<ToolbarUnoDispatcher> m_xInsertDispatch;
    std::unique_ptr<weld::Toolbar> m_xSelect;
    std::unique_ptr<ToolbarUnoDispatcher> m_xSelectDispatch;
    std::unique_ptr<weld::Toolbar> m_xRowSizing;
    std::unique_ptr<ToolbarUnoDispatcher> m_xRowSizingDispatch;
    std::unique_ptr<weld::Toolbar> m_xColumnSizing;
    std::unique_ptr<ToolbarUnoDispatcher> m_xColumnSizingDispatch;
    std::unique_ptr<weld::Toolbar> m_xDelete;
    std::unique_ptr<ToolbarUnoDispatcher> m_xDeleteDispatch;
    std::unique_ptr<weld::Toolbar> m_xSplitMerge;
    std::unique_ptr<ToolbarUnoDispatcher> m_xSplitMergeDispatch;
    std::unique_ptr<weld::ComboBox> m_xAlignment;
    SvxRelativeField m_aLeftSpacingEdit;
    SvxRelativeField m_aRightSpacingEdit;
    std::unique_ptr<weld::Toolbar> m_xMisc;
    std::unique_ptr<ToolbarUnoDispatcher> m_xMiscDispatch;

    ::sfx2::sidebar::ControllerItem m_aRowHeightController;
    ::sfx2::sidebar::ControllerItem m_aColumnWidthController;
    ::sfx2::sidebar::ControllerItem m_aAlignmentController;
    ::sfx2::sidebar::ControllerItem m_aLeftSpacingController;
    ::sfx2::sidebar::ControllerItem m_aRightSpacingController;
    ::sfx2::sidebar::ControllerItem m_aInsertRowsBeforeController;
    ::sfx2::sidebar::ControllerItem m_aInsertRowsAfterController;
    ::sfx2::sidebar::ControllerItem m_aInsertColumnsBeforeController;
    ::sfx2::sidebar::ControllerItem m_aInsertColumnsAfterController;
    ::sfx2::sidebar::ControllerItem m_aDeleteRowsController;
    ::sfx2::sidebar::ControllerItem m_aDeleteColumnsController;
    ::sfx2::sidebar::ControllerItem m_aDeleteTableController;
    ::sfx2::sidebar::ControllerItem m_aSetMinimalRowHeightController;
    ::sfx2::sidebar::ControllerItem m_aSetOptimalRowHeightController;
    ::sfx2::sidebar::ControllerItem m_aDistributeRowsController;
    ::sfx2::sidebar::ControllerItem m_aSetMinimalColumnWidthController;
    ::sfx2::sidebar::ControllerItem m_aSetOptimalColumnWidthController;
    ::sfx2::sidebar::ControllerItem m_aDistributeColumnsController;
    ::sfx2::sidebar::ControllerItem m_aMergeCellsController;

    DECL_LINK(RowHeightMofiyHdl, weld::MetricSpinButton&, void);
    DECL_LINK(ColumnWidthMofiyHdl, weld::MetricSpinButton&, void);
    DECL_LINK(AlignmentHdl, weld::ComboBox&, void);
    DECL_LINK(SpacingHdl, weld::MetricSpinButton&, void);
};

} // end of namespace sw::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
