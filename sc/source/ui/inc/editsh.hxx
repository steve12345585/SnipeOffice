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

#include <sfx2/shell.hxx>
#include <tools/link.hxx>
#include <rtl/ref.hxx>

#include <shellids.hxx>

class SfxModule;
class EditView;
class ScViewData;
class ScInputHandler;
class SvxFieldData;
class TransferableDataHelper;
class TransferableClipboardListener;

class ScEditShell final : public SfxShell
{
private:
    EditView*   pEditView;
    ScViewData& rViewData;
    rtl::Reference<TransferableClipboardListener> mxClipEvtLstnr;
    bool        bPastePossible;
    bool        bIsInsertMode;

    // These methods did return 'const SvxURLField*' before, but
    // at least for GetFirstURLFieldFromCell this is not safe: The
    // SvxFieldItem accessed there and held in the local temporary
    // SfxItemSet may be deleted with it, so return value can be
    // corrupted/deleted. To avoid that, return a Clone
    std::unique_ptr<const SvxFieldData> GetURLField();
    std::unique_ptr<const SvxFieldData> GetFirstURLFieldFromCell();

    ScInputHandler* GetMyInputHdl();

    DECL_LINK( ClipboardChanged, TransferableDataHelper*, void );

public:
    SFX_DECL_INTERFACE(SCID_EDIT_SHELL)

private:
    /// SfxInterface initializer.
    static void InitInterface_Impl();

public:
    ScEditShell(EditView* pView, ScViewData& rData);
    virtual ~ScEditShell() override;

    void    SetEditView(EditView* pView);
    EditView* GetEditView() {return pEditView;}

    void    Execute(SfxRequest& rReq);
    void    ExecuteTrans(const SfxRequest& rReq);
    void    GetState(SfxItemSet &rSet);
    void    GetClipState(SfxItemSet& rSet);

    void    ExecuteAttr(SfxRequest& rReq);
    void    GetAttrState(SfxItemSet &rSet);

    void    ExecuteUndo(const SfxRequest& rReq);
    void    GetUndoState(SfxItemSet &rSet);

    OUString GetSelectionText( bool bWholeWord );
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
