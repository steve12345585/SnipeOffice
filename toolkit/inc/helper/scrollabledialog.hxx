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

#ifndef INCLUDED_TOOLKIT_AWT_SCROLLABLEDIALOG_HXX
#define INCLUDED_TOOLKIT_AWT_SCROLLABLEDIALOG_HXX

#include <vcl/toolkit/dialog.hxx>
#include <vcl/toolkit/scrbar.hxx>

namespace toolkit
{
    class ScrollableDialog final : public Dialog
    {
    public:
        enum ScrollBarVisibility { None, Vert, Hori, Both };

    private:
        VclPtr<ScrollBar>  maHScrollBar;
        VclPtr<ScrollBar>  maVScrollBar;
        Size               maScrollArea;
        bool               mbHasHoriBar;
        bool               mbHasVertBar;
        Point              mnScrollPos;
        tools::Long               mnScrWidth;
        ScrollBarVisibility maScrollVis;

        void    lcl_Scroll( tools::Long nX, tools::Long nY );
        DECL_LINK( ScrollBarHdl, ScrollBar*, void );

    public:
        ScrollableDialog( vcl::Window* pParent, WinBits nStyle, Dialog::InitFlag eFlag = Dialog::InitFlag::Default );
        virtual ~ScrollableDialog() override;
        virtual void dispose() override;
        // Window
        virtual void Resize() override;

        void    SetScrollWidth( tools::Long nWidth );
        void    SetScrollHeight( tools::Long nHeight );
        void    SetScrollLeft( tools::Long nLeft );
        void    SetScrollTop( tools::Long Top );
        void    setScrollVisibility( ScrollBarVisibility rState );
        void    ResetScrollBars();
    };

} // namespacetoolkit


#endif // INCLUDED_TOOLKIT_AWT_SCROLLABLEDIALOG_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
