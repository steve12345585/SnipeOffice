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

#ifndef INCLUDED_SVX_SOURCE_SIDEBAR_TEXT_TEXTUNDERLINECONTROL_HXX
#define INCLUDED_SVX_SOURCE_SIDEBAR_TEXT_TEXTUNDERLINECONTROL_HXX

#include <svtools/toolbarmenu.hxx>

namespace svx
{
class TextUnderlinePopup;

class TextUnderlineControl final : public WeldToolbarPopup
{
public:
    explicit TextUnderlineControl(TextUnderlinePopup* pControl, weld::Widget* pParent);
    virtual void GrabFocus() override;
    virtual ~TextUnderlineControl() override;

private:
    std::unique_ptr<weld::Button> mxNone;
    std::unique_ptr<weld::Button> mxSingle;
    std::unique_ptr<weld::Button> mxDouble;
    std::unique_ptr<weld::Button> mxBold;
    std::unique_ptr<weld::Button> mxDot;
    std::unique_ptr<weld::Button> mxDotBold;
    std::unique_ptr<weld::Button> mxDash;
    std::unique_ptr<weld::Button> mxDashLong;
    std::unique_ptr<weld::Button> mxDashDot;
    std::unique_ptr<weld::Button> mxDashDotDot;
    std::unique_ptr<weld::Button> mxWave;
    std::unique_ptr<weld::Button> mxMoreOptions;

    rtl::Reference<TextUnderlinePopup> mxControl;

    FontLineStyle getLineStyle(const weld::Button& rButton) const;

    DECL_LINK(PBClickHdl, weld::Button&, void);
};
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
