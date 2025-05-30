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
#include <cppuhelper/implbase.hxx>
#include <com/sun/star/ui/dialogs/XDialogClosedListener.hpp>
#include <tools/link.hxx>


namespace svt
{


    //= ODialogClosedListener

    /**
        C++ class to implement a css::ui::dialogs::XDialogClosedListener
    */
    class SVT_DLLPUBLIC DialogClosedListener final :
        public cppu::WeakImplHelper< css::ui::dialogs::XDialogClosedListener >
    {
    private:
        /**
            This link will be called when the dialog was closed.
        */
        Link<css::ui::dialogs::DialogClosedEvent*, void>  m_aDialogClosedLink;

    public:
        DialogClosedListener();

        void SetDialogClosedLink( const Link<css::ui::dialogs::DialogClosedEvent*,void>& rLink ) { m_aDialogClosedLink = rLink; }

        // XDialogClosedListener methods
        virtual void SAL_CALL   dialogClosed( const css::ui::dialogs::DialogClosedEvent& aEvent ) override;

        // XEventListener methods
        virtual void SAL_CALL   disposing( const css::lang::EventObject& Source ) override;
    };


}   // namespace svt

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
