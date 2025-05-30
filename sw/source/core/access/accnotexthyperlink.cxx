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

#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>

#include <comphelper/accessiblekeybindinghelper.hxx>
#include <swurl.hxx>
#include <vcl/svapp.hxx>
#include <frmfmt.hxx>

#include "accnotexthyperlink.hxx"

#include <fmturl.hxx>

#include <vcl/imap.hxx>
#include <vcl/imapobj.hxx>
#include <vcl/keycodes.hxx>

#include <accmap.hxx>

using namespace css;
using namespace css::lang;
using namespace css::uno;
using namespace css::accessibility;

SwAccessibleNoTextHyperlink::SwAccessibleNoTextHyperlink( SwAccessibleNoTextFrame *p, const SwFrame *aFrame ) :
    mxFrame( p ),
    mpFrame( aFrame )
{
}

// XAccessibleAction
sal_Int32 SAL_CALL SwAccessibleNoTextHyperlink::getAccessibleActionCount()
{
    SolarMutexGuard g;

    SwFormatURL aURL( GetFormat()->GetURL() );
    ImageMap* pMap = aURL.GetMap();
    if( pMap != nullptr )
    {
        return pMap->GetIMapObjectCount();
    }
    else if( !aURL.GetURL().isEmpty() )
    {
        return 1;
    }

    return 0;
}

sal_Bool SAL_CALL SwAccessibleNoTextHyperlink::doAccessibleAction( sal_Int32 nIndex )
{
    SolarMutexGuard aGuard;

    if(nIndex < 0 || nIndex >= getAccessibleActionCount())
        throw lang::IndexOutOfBoundsException();

    bool bRet = false;
    SwFormatURL aURL( GetFormat()->GetURL() );
    ImageMap* pMap = aURL.GetMap();
    if( pMap != nullptr )
    {
        IMapObject* pMapObj = pMap->GetIMapObject(nIndex);
        bRet = LoadURL(mxFrame->GetShell(), pMapObj->GetURL(), LoadUrlFlags::NONE,
                       pMapObj->GetTarget());
    }
    else
    {
        bRet = LoadURL(mxFrame->GetShell(), aURL.GetURL(), LoadUrlFlags::NONE,
                       aURL.GetTargetFrameName());
    }

    return bRet;
}

OUString SAL_CALL SwAccessibleNoTextHyperlink::getAccessibleActionDescription(
        sal_Int32 nIndex )
{
    SolarMutexGuard g;

    OUString sDesc;

    if(nIndex < 0 || nIndex >= getAccessibleActionCount())
        throw lang::IndexOutOfBoundsException();

    SwFormatURL aURL( GetFormat()->GetURL() );
    ImageMap* pMap = aURL.GetMap();
    if( pMap != nullptr )
    {
        IMapObject* pMapObj = pMap->GetIMapObject(nIndex);
        if (!pMapObj->GetDesc().isEmpty())
            sDesc = pMapObj->GetDesc();
        else if (!pMapObj->GetURL().isEmpty())
            sDesc = pMapObj->GetURL();
    }
    else if( !aURL.GetURL().isEmpty() )
        sDesc = aURL.GetName();

    return sDesc;
}

Reference< XAccessibleKeyBinding > SAL_CALL
    SwAccessibleNoTextHyperlink::getAccessibleActionKeyBinding( sal_Int32 nIndex )
{
    SolarMutexGuard g;


    if(nIndex < 0 || nIndex >= getAccessibleActionCount())
        throw lang::IndexOutOfBoundsException();

    bool bIsValid = false;
    SwFormatURL aURL( GetFormat()->GetURL() );
    ImageMap* pMap = aURL.GetMap();
    if( pMap != nullptr )
    {
        IMapObject* pMapObj = pMap->GetIMapObject(nIndex);
        if (!pMapObj->GetURL().isEmpty())
            bIsValid = true;
    }
    else if (!aURL.GetURL().isEmpty())
        bIsValid = true;

    if(!bIsValid)
        return nullptr;

    rtl::Reference< ::comphelper::OAccessibleKeyBindingHelper > xKeyBinding =
            new ::comphelper::OAccessibleKeyBindingHelper();

    css::awt::KeyStroke aKeyStroke;
    aKeyStroke.Modifiers = 0;
    aKeyStroke.KeyCode = KEY_RETURN;
    aKeyStroke.KeyChar = 0;
    aKeyStroke.KeyFunc = 0;
    xKeyBinding->AddKeyBinding( aKeyStroke );

    return xKeyBinding;
}

// XAccessibleHyperlink
Any SAL_CALL SwAccessibleNoTextHyperlink::getAccessibleActionAnchor(
        sal_Int32 nIndex )
{
    SolarMutexGuard g;

    if(nIndex < 0 || nIndex >= getAccessibleActionCount())
        throw lang::IndexOutOfBoundsException();

    Any aRet;
    //SwFrame* pAnchor = static_cast<SwFlyFrame*>(mpFrame)->GetAnchor();
    Reference< XAccessible > xAnchor = mxFrame->GetAccessibleMap()->GetContext(mpFrame);
    //SwAccessibleNoTextFrame* pFrame = xFrame.get();
    //Reference< XAccessible > xAnchor = (XAccessible*)pFrame;
    aRet <<= xAnchor;
    return aRet;
}

Any SAL_CALL SwAccessibleNoTextHyperlink::getAccessibleActionObject(
            sal_Int32 nIndex )
{
    SolarMutexGuard g;

    if(nIndex < 0 || nIndex >= getAccessibleActionCount())
        throw lang::IndexOutOfBoundsException();

    SwFormatURL aURL( GetFormat()->GetURL() );
    OUString retText;
    ImageMap* pMap = aURL.GetMap();
    if( pMap != nullptr )
    {
        IMapObject* pMapObj = pMap->GetIMapObject(nIndex);
        if (!pMapObj->GetURL().isEmpty())
            retText = pMapObj->GetURL();
    }
    else if ( !aURL.GetURL().isEmpty() )
        retText = aURL.GetURL();

    Any aRet;
    aRet <<= retText;
    return aRet;
}

sal_Int32 SAL_CALL SwAccessibleNoTextHyperlink::getStartIndex()
{
    return 0;
}

sal_Int32 SAL_CALL SwAccessibleNoTextHyperlink::getEndIndex()
{
    return 0;
}

sal_Bool SAL_CALL SwAccessibleNoTextHyperlink::isValid(  )
{
    SolarMutexGuard g;

    SwFormatURL aURL( GetFormat()->GetURL() );

    if( aURL.GetMap() || !aURL.GetURL().isEmpty() )
        return true;
    return false;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
