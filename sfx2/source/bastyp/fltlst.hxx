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

#ifndef INCLUDED_SFX2_SOURCE_BASTYP_FLTLST_HXX
#define INCLUDED_SFX2_SOURCE_BASTYP_FLTLST_HXX

#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/util/XRefreshable.hpp>
#include <com/sun/star/util/XRefreshListener.hpp>
#include <com/sun/star/lang/EventObject.hpp>

class SfxFilterListener final
{
    private:
        css::uno::Reference< css::util::XRefreshable >  m_xFilterCache;
        css::uno::Reference< css::util::XRefreshListener >  m_xFilterCacheListener;

    public:
        SfxFilterListener();
        ~SfxFilterListener();

    public:
        // XRefreshListener
        /// @throws css::uno::RuntimeException
        void refreshed( const css::lang::EventObject& aSource );
        // XEventListener
        /// @throws css::uno::RuntimeException
        void disposing( const css::lang::EventObject& aSource );

};

#endif // INCLUDED_SFX2_SOURCE_BASTYP_FLTLST_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
