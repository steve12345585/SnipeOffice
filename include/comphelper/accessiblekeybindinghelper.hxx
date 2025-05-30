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

#ifndef INCLUDED_COMPHELPER_ACCESSIBLEKEYBINDINGHELPER_HXX
#define INCLUDED_COMPHELPER_ACCESSIBLEKEYBINDINGHELPER_HXX

#include <com/sun/star/accessibility/XAccessibleKeyBinding.hpp>
#include <comphelper/comphelperdllapi.h>
#include <cppuhelper/implbase.hxx>
#include <mutex>
#include <vector>

namespace comphelper
{


    // OAccessibleKeyBindingHelper


    typedef ::cppu::WeakImplHelper <   css::accessibility::XAccessibleKeyBinding
                                   >   OAccessibleKeyBindingHelper_Base;

    /** a helper class for implementing an accessible keybinding
     */
    class COMPHELPER_DLLPUBLIC OAccessibleKeyBindingHelper final : public OAccessibleKeyBindingHelper_Base
    {
    private:
        typedef ::std::vector< css::uno::Sequence< css::awt::KeyStroke > > KeyBindings;
        KeyBindings     m_aKeyBindings;
        std::mutex      m_aMutex;

        virtual ~OAccessibleKeyBindingHelper() override;

    public:
        OAccessibleKeyBindingHelper();

        /// @throws css::uno::RuntimeException
        void AddKeyBinding( const css::uno::Sequence< css::awt::KeyStroke >& rKeyBinding );
        /// @throws css::uno::RuntimeException
        void AddKeyBinding( const css::awt::KeyStroke& rKeyStroke );

        // XAccessibleKeyBinding
        virtual sal_Int32 SAL_CALL getAccessibleKeyBindingCount() override;
        virtual css::uno::Sequence< css::awt::KeyStroke > SAL_CALL getAccessibleKeyBinding( sal_Int32 nIndex ) override;
    };


}   // namespace comphelper


#endif // INCLUDED_COMPHELPER_ACCESSIBLEKEYBINDINGHELPER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
