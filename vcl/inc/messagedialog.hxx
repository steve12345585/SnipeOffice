/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vcl/toolkit/dialog.hxx>
#include <vcl/toolkit/vclmedit.hxx>
#include <vcl/layout.hxx>
#include <vcl/toolkit/fixed.hxx>

class MessageDialog final : public Dialog
{
private:
    VclButtonsType m_eButtonsType;
    VclMessageType m_eMessageType;
    VclPtr<VclBox> m_pOwnedContentArea;
    VclPtr<VclButtonBox> m_pOwnedActionArea;
    VclPtr<VclGrid> m_pGrid;
    VclPtr<VclVBox> m_pMessageBox;
    VclPtr<FixedImage> m_pImage;
    VclPtr<VclMultiLineEdit> m_pPrimaryMessage;
    VclPtr<VclMultiLineEdit> m_pSecondaryMessage;
    OUString m_sPrimaryString;
    OUString m_sSecondaryString;
    void create_owned_areas();

    static void SetMessagesWidths(vcl::Window const* pParent, VclMultiLineEdit* pPrimaryMessage,
                                  VclMultiLineEdit* pSecondaryMessage);

    friend class VclPtr<MessageDialog>;
    MessageDialog(vcl::Window* pParent, WinBits nStyle);

    virtual void StateChanged(StateChangedType nType) override;

public:
    MessageDialog(vcl::Window* pParent, OUString aMessage, VclMessageType eMessageType,
                  VclButtonsType eButtonsType);
    virtual bool set_property(const OUString& rKey, const OUString& rValue) override;
    OUString const& get_primary_text() const;
    OUString const& get_secondary_text() const;
    void set_primary_text(const OUString& rPrimaryString);
    void set_secondary_text(const OUString& rSecondaryString);
    virtual ~MessageDialog() override;
    virtual void dispose() override;

    void create_message_area();
    VclContainer* get_message_area() const { return m_pMessageBox.get(); }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
