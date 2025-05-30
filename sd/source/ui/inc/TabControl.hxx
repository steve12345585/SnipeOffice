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

#include <svtools/tabbar.hxx>
#include <vcl/transfer.hxx>

namespace sd {

/**
 * TabControl-Class for page switch
 */

class DrawViewShell;

class TabControl final
    : public TabBar,
      public DragSourceHelper,
      public DropTargetHelper
{
public:
    TabControl (DrawViewShell* pDrViewSh, vcl::Window* pParent);
    virtual void dispose() override;
    virtual ~TabControl() override;

    /** Inform all listeners of this control that the current page has been
        activated.  Call this method after switching the current page and is
        not done elsewhere (like when using page up/down keys).
    */
    void SendActivatePageEvent();

    /** Inform all listeners of this control that the current page has been
        deactivated.  Call this method before switching the current page and
        is not done elsewhere (like when using page up/down keys).
    */
    void SendDeactivatePageEvent();

private:
    DrawViewShell*      pDrViewSh;
    bool                bInternalMove;

    // TabBar
    virtual void        Select() override;
    virtual void        DoubleClick() override;
    virtual void        MouseButtonDown(const MouseEvent& rMEvt) override;

    virtual void        Command(const CommandEvent& rCEvt) override;

    virtual bool        StartRenaming() override;
    virtual TabBarAllowRenamingReturnCode  AllowRenaming() override;
    virtual void        EndRenaming() override;

    virtual void        ActivatePage() override;
    virtual bool        DeactivatePage() override;

    // DragSourceHelper
    virtual void        StartDrag( sal_Int8 nAction, const Point& rPosPixel ) override;

    // DropTargetHelper
    virtual sal_Int8    AcceptDrop( const AcceptDropEvent& rEvt ) override;
    virtual sal_Int8    ExecuteDrop( const ExecuteDropEvent& rEvt ) override;

    // nested class to implement the TransferableHelper
    class TabControlTransferable final : public TransferableHelper
    {
    public:
        explicit TabControlTransferable( TabControl& rParent ) :
            mrParent( rParent ) {}
    private:

        TabControl&     mrParent;

        virtual             ~TabControlTransferable() override;

        virtual void        AddSupportedFormats() override;
        virtual bool GetData( const css::datatransfer::DataFlavor& rFlavor, const OUString& rDestDoc ) override;
        virtual void        DragFinished( sal_Int8 nDropAction ) override;

    };

    friend class TabControl::TabControlTransferable;

    void                DragFinished();

    using TabBar::StartDrag;
};

} // end of namespace sd

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
