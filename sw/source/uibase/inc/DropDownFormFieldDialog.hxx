/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SW_SOURCE_UIBASE_INC_DROPDOWNFORMFIELDDIALOG_HXX
#define INCLUDED_SW_SOURCE_UIBASE_INC_DROPDOWNFORMFIELDDIALOG_HXX

#include <vcl/weld.hxx>

namespace sw::mark
{
class Fieldmark;
}

/// Dialog to specify the properties of drop-down form field
namespace sw
{
class DropDownFormFieldDialog final : public weld::GenericDialogController
{
private:
    mark::Fieldmark* m_pDropDownField;
    bool m_bListHasChanged;

    std::unique_ptr<weld::Entry> m_xListItemEntry;
    std::unique_ptr<weld::Button> m_xListAddButton;

    std::unique_ptr<weld::TreeView> m_xListItemsTreeView;

    std::unique_ptr<weld::Button> m_xListRemoveButton;
    std::unique_ptr<weld::Button> m_xListUpButton;
    std::unique_ptr<weld::Button> m_xListDownButton;

    DECL_LINK(ListChangedHdl, weld::TreeView&, void);
    DECL_LINK(KeyPressedHdl, const KeyEvent&, bool);
    DECL_LINK(EntryChangedHdl, weld::Entry&, void);
    DECL_LINK(ButtonPushedHdl, weld::Button&, void);

    void InitControls();
    void AppendItemToList();
    void UpdateButtons();

public:
    DropDownFormFieldDialog(weld::Widget* pParent, mark::Fieldmark* pDropDownField);
    virtual ~DropDownFormFieldDialog() override;

    void Apply();
};

} // namespace sw

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
