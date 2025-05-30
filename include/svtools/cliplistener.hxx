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

#include <svtools/svtdllapi.h>
#include <tools/link.hxx>
#include <cppuhelper/implbase.hxx>
#include <com/sun/star/datatransfer/clipboard/XClipboardListener.hpp>

namespace vcl { class Window; }

class TransferableDataHelper;

class SVT_DLLPUBLIC TransferableClipboardListener final : public cppu::WeakImplHelper<
                            css::datatransfer::clipboard::XClipboardListener >
{
    Link<TransferableDataHelper*,void>  aLink;

    void    AddRemoveListener( vcl::Window* pWin, bool bAdd );
public:
            // Link is called with a TransferableDataHelper pointer
            TransferableClipboardListener( const Link<TransferableDataHelper*,void>& rCallback );
            virtual ~TransferableClipboardListener() override;

    void    AddListener( vcl::Window* pWin ) { AddRemoveListener(pWin, true); }
    void    RemoveListener( vcl::Window* pWin ) { AddRemoveListener(pWin, false); }
    void    ClearCallbackLink();

            // XEventListener
    virtual void SAL_CALL disposing( const css::lang::EventObject& Source ) override;
            // XClipboardListener
    virtual void SAL_CALL changedContents( const css::datatransfer::clipboard::ClipboardEvent& event ) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
