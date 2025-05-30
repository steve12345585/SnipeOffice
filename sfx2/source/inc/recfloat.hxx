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

#ifndef INCLUDED_SFX2_SOURCE_INC_RECFLOAT_HXX
#define INCLUDED_SFX2_SOURCE_INC_RECFLOAT_HXX

#include <sfx2/basedlgs.hxx>
#include <sfx2/childwin.hxx>
#include <sfx2/weldutils.hxx>

class SfxRecordingFloatWrapper_Impl final : public SfxChildWindow
{
    SfxBindings*        pBindings;
public:
                        SfxRecordingFloatWrapper_Impl( vcl::Window* pParent ,
                                                sal_uInt16 nId ,
                                                SfxBindings* pBindings ,
                                                SfxChildWinInfo const * pInfo );
                        virtual ~SfxRecordingFloatWrapper_Impl() override;

                        SFX_DECL_CHILDWINDOW(SfxRecordingFloatWrapper_Impl);
    virtual bool        QueryClose() override;
};

class SfxRecordingFloat_Impl final : public SfxModelessDialogController
{
    std::unique_ptr<weld::Toolbar> m_xToolbar;
    std::unique_ptr<ToolbarUnoDispatcher> m_xDispatcher;
    ImplSVEvent *mnPostUserEventId;
    bool m_bFirstActivate;

    DECL_LINK(PresentParentFrame, void*, void);

public:
    SfxRecordingFloat_Impl(SfxBindings* pBindings,
                           SfxChildWindow* pChildWin,
                           weld::Window* pParent);
    virtual ~SfxRecordingFloat_Impl() override;
    virtual void FillInfo(SfxChildWinInfo& rInfo) const override;
    virtual void Activate() override;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
