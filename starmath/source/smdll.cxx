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

#include <sal/config.h>

#include <svx/svxids.hrc>
#include <svx/modctrl.hxx>
#include <svx/zoomctrl.hxx>
#include <svx/zoomsliderctrl.hxx>
#include <sfx2/docfac.hxx>
#include <sfx2/app.hxx>
#include <sfx2/sidebar/SidebarChildWindow.hxx>

#include <smdll.hxx>
#include <smmod.hxx>
#include <document.hxx>
#include <view.hxx>

#include <starmath.hrc>

#include <svx/xmlsecctrl.hxx>

namespace
{
    class SmDLL
    {
    public:
        SmDLL();
    };

    SmDLL::SmDLL()
    {
        if ( SfxApplication::GetModule(SfxToolsModule::Math) )    // Module already active
            return;

        SfxObjectFactory& rFactory = SmDocShell::Factory();

        auto pUniqueModule = std::make_unique<SmModule>(&rFactory);
        SmModule* pModule = pUniqueModule.get();
        SfxApplication::SetModule(SfxToolsModule::Math, std::move(pUniqueModule));

        rFactory.SetDocumentServiceName( u"com.sun.star.formula.FormulaProperties"_ustr );

        SmModule::RegisterInterface(pModule);
        SmDocShell::RegisterInterface(pModule);
        SmViewShell::RegisterInterface(pModule);

        SmViewShell::RegisterFactory(SFX_INTERFACE_SFXAPP);

        SvxZoomStatusBarControl::RegisterControl(SID_ATTR_ZOOM, pModule);
        SvxZoomSliderControl::RegisterControl(SID_ATTR_ZOOMSLIDER, pModule);
        SvxModifyControl::RegisterControl(SID_TEXTSTATUS, pModule);
        XmlSecStatusBarControl::RegisterControl(SID_SIGNATURE, pModule);

        sfx2::sidebar::SidebarChildWindow::RegisterChildWindow(true, pModule);

        SmCmdBoxWrapper::RegisterChildWindow(true);
    }
}

namespace SmGlobals
{
    void ensure()
    {
        static SmDLL theDll;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
