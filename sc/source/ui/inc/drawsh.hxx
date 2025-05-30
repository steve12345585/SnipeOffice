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
#include <shellids.hxx>
#include <svx/svdtypes.hxx>
#include <tools/link.hxx>
#include <rtl/ref.hxx>

class AbstractSvxObjectNameDialog;
class ScViewData;
class ScDrawView;
class SdrMarkList;
class SfxModule;
class SdrObject;

namespace weld { class Window; }

namespace svx::sidebar { class SelectionChangeHandler; }

class ScDrawShell : public SfxShell
{
    ScViewData& rViewData;
    ::rtl::Reference<svx::sidebar::SelectionChangeHandler> mpSelectionChangeHandler;

    DECL_LINK( NameObjectHdl, AbstractSvxObjectNameDialog&, bool );

protected:
    virtual void    Activate(bool bMDI) override;
    ScViewData& GetViewData() { return rViewData; }

public:
    SFX_DECL_INTERFACE(SCID_DRAW_SHELL)

private:
    /// SfxInterface initializer.
    static void InitInterface_Impl();

public:
                    ScDrawShell(ScViewData& rData);
                    virtual ~ScDrawShell() override;

    static void StateDisableItems( SfxItemSet &rSet );

    void    ExecDrawAttr(SfxRequest& rReq);
    void    GetDrawAttrState(SfxItemSet &rSet);
    void    GetAttrFuncState(SfxItemSet &rSet);

    void    ExecDrawFunc(SfxRequest& rReq);
    void    GetDrawFuncState(SfxItemSet &rSet);
    void    GetState(SfxItemSet &rSet);

    void    ExecFormText(const SfxRequest& rReq);     // StarFontWork
    void    GetFormTextState(SfxItemSet& rSet);

    void    ExecuteHLink(const SfxRequest& rReq);     // Hyperlink
    void    GetHLinkState(SfxItemSet& rSet);

    void    ExecFormatPaintbrush(const SfxRequest& rReq);
    void    StateFormatPaintbrush(SfxItemSet& rSet);

    void    ExecuteMacroAssign(SdrObject* pObj, weld::Window* pWin);
    void    ExecuteLineDlg( const SfxRequest& rReq );
    void    ExecuteAreaDlg( const SfxRequest& rReq );
    void    ExecuteTextAttrDlg( SfxRequest& rReq );
    void    ExecuteMeasureDlg( SfxRequest& rReq );

    ScDrawView* GetDrawView();

    static bool AreAllObjectsOnLayer(SdrLayerID nLayerNo,const SdrMarkList& rMark);

    void GetDrawAttrStateForIFBX( SfxItemSet& rSet );
    OUString const & GetSidebarContextName();

    void setModified();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
