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

#include <rtl/ustrbuf.hxx>
#include <tools/long.hxx>
#include <vcl/stdtext.hxx>

#include <osx/salsys.h>
#include <osx/saldata.hxx>
#include <osx/salinst.h>
#include <quartz/utils.h>

#include <strings.hrc>

AquaSalSystem::~AquaSalSystem()
{
}

unsigned int AquaSalSystem::GetDisplayScreenCount()
{
    NSArray* pScreens = [NSScreen screens];
    return pScreens ? [pScreens count] : 1;
}

AbsoluteScreenPixelRectangle AquaSalSystem::GetDisplayScreenPosSizePixel( unsigned int nScreen )
{
    if (Application::IsBitmapRendering())
    {
        AbsoluteScreenPixelRectangle aRect;
        if (nScreen == 0)
            aRect = AbsoluteScreenPixelRectangle(AbsoluteScreenPixelPoint(0,0), AbsoluteScreenPixelSize(1024, 768));
        return aRect;
    }

    NSArray* pScreens = [NSScreen screens];
    AbsoluteScreenPixelRectangle aRet;
    NSScreen* pScreen = nil;
    if( pScreens && nScreen < [pScreens count] )
        pScreen = [pScreens objectAtIndex: nScreen];
    else
        pScreen = [NSScreen mainScreen];

    if( pScreen )
    {
        NSRect aFrame = [pScreen frame];
        aRet = AbsoluteScreenPixelRectangle(
                   AbsoluteScreenPixelPoint( static_cast<tools::Long>(aFrame.origin.x), static_cast<tools::Long>(aFrame.origin.y) ),
                   AbsoluteScreenPixelSize( static_cast<tools::Long>(aFrame.size.width), static_cast<tools::Long>(aFrame.size.height) ) );
    }
    return aRet;
}

static NSString* getStandardString( StandardButtonType nButtonId, bool bUseResources )
{
    OUString aText;
    if( bUseResources )
    {
        aText = GetStandardText( nButtonId );
    }
    if( aText.isEmpty() ) // this is for bad cases, we might be missing the vcl resource
    {
        switch( nButtonId )
        {
        case StandardButtonType::OK:         aText = "OK";break;
        case StandardButtonType::Abort:      aText = "Abort";break;
        case StandardButtonType::Cancel:     aText = "Cancel";break;
        case StandardButtonType::Retry:      aText = "Retry";break;
        case StandardButtonType::Yes:        aText = "Yes";break;
        case StandardButtonType::No:         aText = "No";break;
        default: break;
        }
    }
    return aText.isEmpty() ? nil : CreateNSString( aText);
}

int AquaSalSystem::ShowNativeMessageBox( const OUString& rTitle,
                                        const OUString& rMessage )
{
    NSString* pTitle = CreateNSString( rTitle );
    NSString* pMessage = CreateNSString( rMessage );

    NSString* pDefText = getStandardString( StandardButtonType::OK, false/*bUseResources*/ );

    SAL_WNODEPRECATED_DECLARATIONS_PUSH //TODO: 10.10 NSRunAlertPanel
    int nResult = NSRunAlertPanel( pTitle, @"%@", pDefText, nil, nil, pMessage );
    SAL_WNODEPRECATED_DECLARATIONS_POP

    if( pTitle )
        [pTitle release];
    if( pMessage )
        [pMessage release];
    if( pDefText )
        [pDefText release];

    int nRet = 0;
    if( nResult == 1 )
        nRet = SALSYSTEM_SHOWNATIVEMSGBOX_BTN_OK;

    return nRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
