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


#include "helpdispatch.hxx"
#include "newhelp.hxx"
#include <tools/debug.hxx>
#include <utility>

using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::util;

// class HelpInterceptor_Impl --------------------------------------------

HelpDispatch_Impl::HelpDispatch_Impl( HelpInterceptor_Impl& _rInterceptor,
                                      css::uno::Reference< css::frame::XDispatch > _xDisp ) :

    m_rInterceptor  ( _rInterceptor ),
    m_xRealDispatch (std::move( _xDisp ))

{
}


HelpDispatch_Impl::~HelpDispatch_Impl()
{
}


// XDispatch

void SAL_CALL HelpDispatch_Impl::dispatch(

    const URL& aURL, const Sequence< PropertyValue >& aArgs )

{
    DBG_ASSERT( m_xRealDispatch.is(), "invalid dispatch" );

    // search for a keyword (dispatch from the basic ide)
    bool bHasKeyword = false;
    OUString sKeyword;
    for ( const PropertyValue& rArg : aArgs )
    {
        if ( rArg.Name == "HelpKeyword" )
        {
            OUString sHelpKeyword;
            if ( ( rArg.Value >>= sHelpKeyword ) && !sHelpKeyword.isEmpty() )
            {
                sKeyword = sHelpKeyword;
                bHasKeyword = !sKeyword.isEmpty();
                break;
            }
        }
    }

    // if a keyword was found, then open it
    SfxHelpWindow_Impl* pHelpWin = m_rInterceptor.GetHelpWindow();
    DBG_ASSERT( pHelpWin, "invalid HelpWindow" );
    if ( bHasKeyword )
    {
        pHelpWin->OpenKeyword( sKeyword );
        return;
    }

    pHelpWin->loadHelpContent(aURL.Complete);
}


void SAL_CALL HelpDispatch_Impl::addStatusListener(

    const Reference< XStatusListener >& xControl, const URL& aURL )

{
    DBG_ASSERT( m_xRealDispatch.is(), "invalid dispatch" );
    m_xRealDispatch->addStatusListener( xControl, aURL );
}


void SAL_CALL HelpDispatch_Impl::removeStatusListener(

    const Reference< XStatusListener >& xControl, const URL& aURL )

{
    DBG_ASSERT( m_xRealDispatch.is(), "invalid dispatch" );
    m_xRealDispatch->removeStatusListener( xControl, aURL );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
