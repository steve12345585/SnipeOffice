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


#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <comphelper/accessiblekeybindinghelper.hxx>

#include "AccessibleHyperlink.hxx"
#include <editeng/unoedprx.hxx>
#include <editeng/flditem.hxx>
#include <vcl/keycodes.hxx>

using namespace ::com::sun::star;


// AccessibleHyperlink implementation


namespace accessibility
{

AccessibleHyperlink::AccessibleHyperlink( SvxAccessibleTextAdapter& r, SvxFieldItem* p, sal_Int32 nStt, sal_Int32 nEnd, const OUString& rD )
: rTA( r )
{
    pFld.reset( p );
    nStartIdx = nStt;
    nEndIdx = nEnd;
    aDescription = rD;
}

AccessibleHyperlink::~AccessibleHyperlink()
{
}

// XAccessibleAction
sal_Int32 SAL_CALL AccessibleHyperlink::getAccessibleActionCount()
{
     return isValid() ? 1 : 0;
}

sal_Bool SAL_CALL AccessibleHyperlink::doAccessibleAction( sal_Int32 nIndex  )
{
    bool bRet = false;
    if ( isValid() && ( nIndex == 0 ) )
    {
        rTA.FieldClicked( *pFld );
        bRet = true;
    }
    return bRet;
}

OUString  SAL_CALL AccessibleHyperlink::getAccessibleActionDescription( sal_Int32 nIndex )
{
    OUString aDesc;

    if ( isValid() && ( nIndex == 0 ) )
        aDesc = aDescription;

    return aDesc;
}

uno::Reference< css::accessibility::XAccessibleKeyBinding > SAL_CALL AccessibleHyperlink::getAccessibleActionKeyBinding( sal_Int32 nIndex )
{
    if( !isValid() || ( nIndex != 0 ) )
        return nullptr;

    rtl::Reference<::comphelper::OAccessibleKeyBindingHelper> pKeyBindingHelper = new ::comphelper::OAccessibleKeyBindingHelper();

    awt::KeyStroke aKeyStroke;
    aKeyStroke.Modifiers = 0;
    aKeyStroke.KeyCode = KEY_RETURN;
    aKeyStroke.KeyChar = 0;
    aKeyStroke.KeyFunc = 0;
    pKeyBindingHelper->AddKeyBinding( aKeyStroke );

    return pKeyBindingHelper;
}

// XAccessibleHyperlink
uno::Any SAL_CALL AccessibleHyperlink::getAccessibleActionAnchor( sal_Int32 /*nIndex*/ )
{
    return uno::Any();
}

uno::Any SAL_CALL AccessibleHyperlink::getAccessibleActionObject( sal_Int32 /*nIndex*/ )
{
    return uno::Any();
}

sal_Int32 SAL_CALL AccessibleHyperlink::getStartIndex()
{
    return nStartIdx;
}

sal_Int32 SAL_CALL AccessibleHyperlink::getEndIndex()
{
    return nEndIdx;
}

sal_Bool SAL_CALL AccessibleHyperlink::isValid(  )
{
    return rTA.IsValid();
}

}  // end of namespace accessibility


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
