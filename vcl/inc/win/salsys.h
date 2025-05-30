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

#include <salsys.hxx>

#include <vector>
#include <map>

class WinSalSystem : public SalSystem
{
public:
    struct DisplayMonitor
    {
        OUString        m_aName;
        AbsoluteScreenPixelRectangle       m_aArea;

        DisplayMonitor() {}
        DisplayMonitor( const OUString& rName,
                        const AbsoluteScreenPixelRectangle& rArea )
                        : m_aName( rName ),
                          m_aArea( rArea )
        {
        }
    };
private:
    std::vector<DisplayMonitor>             m_aMonitors;
    std::map<OUString, unsigned int>   m_aDeviceNameToMonitor;
    unsigned int                            m_nPrimary;
public:
    WinSalSystem() : m_nPrimary( 0 ) {}
    virtual ~WinSalSystem() override;

    virtual unsigned int GetDisplayScreenCount() override;
    virtual unsigned int GetDisplayBuiltInScreen() override;
    virtual AbsoluteScreenPixelRectangle GetDisplayScreenPosSizePixel( unsigned int nScreen )  override;
    virtual int ShowNativeMessageBox( const OUString& rTitle,
                                      const OUString& rMessage)  override;
    bool initMonitors();
    // discards monitorinfo; used by WM_DISPLAYCHANGED handler
    void clearMonitors();
    const std::vector<DisplayMonitor>& getMonitors()
    { initMonitors(); return m_aMonitors;}

    bool handleMonitorCallback( sal_IntPtr /*HMONITOR*/,
                                sal_IntPtr /*HDC*/,
                                sal_IntPtr /*LPRECT*/ );
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
