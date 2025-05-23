/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
* This file is Part of the SnipeOffice project.
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <tools/link.hxx>
#include <vcl/weld.hxx>

class SdrObjGroup;

/** Edit Diagram dialog */
class DiagramDialog : public weld::GenericDialogController
{
public:
    DiagramDialog(weld::Window* pWindow, SdrObjGroup& rDiagram);
    virtual ~DiagramDialog() override;

private:
    SdrObjGroup& m_rDiagram;
    sal_uInt32 m_nUndos;

    std::unique_ptr<weld::Button> mpBtnCancel;
    std::unique_ptr<weld::Button> mpBtnAdd;
    std::unique_ptr<weld::Button> mpBtnRemove;
    std::unique_ptr<weld::TreeView> mpTreeDiagram;
    std::unique_ptr<weld::TextView> mpTextAdd;

    DECL_LINK(OnAddCancel, weld::Button&, void);
    DECL_LINK(OnAddClick, weld::Button&, void);
    DECL_LINK(OnRemoveClick, weld::Button&, void);

    void populateTree(const weld::TreeIter* pParent, const OUString& rParentId);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
