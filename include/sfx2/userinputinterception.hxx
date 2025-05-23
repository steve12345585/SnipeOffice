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

#ifndef INCLUDED_SFX2_USERINPUTINTERCEPTION_HXX
#define INCLUDED_SFX2_USERINPUTINTERCEPTION_HXX

#include <config_options.h>
#include <sfx2/dllapi.h>

#include <memory>

namespace com::sun::star::awt { class XKeyHandler; }
namespace com::sun::star::awt { class XMouseClickHandler; }
namespace com::sun::star::uno { template <typename > class Reference; }
namespace osl { class Mutex; }

class NotifyEvent;

namespace cppu { class OWeakObject; }


namespace sfx2
{


    //= UserInputInterception

    struct UserInputInterception_Data;
    /** helper class for implementing the XUserInputInterception interface
        for a controller implementation
    */
    class UNLESS_MERGELIBS_MORE(SFX2_DLLPUBLIC) UserInputInterception
    {
    public:
        UserInputInterception( ::cppu::OWeakObject& _rControllerImpl, ::osl::Mutex& _rMutex );
        ~UserInputInterception();
        UserInputInterception(const UserInputInterception&) = delete;
        UserInputInterception& operator=( const UserInputInterception& ) = delete;

        // delegator functions for your XUserInputInterception implementation
        /// @throws css::uno::RuntimeException
        void    addKeyHandler( const css::uno::Reference< css::awt::XKeyHandler >& xHandler );
        /// @throws css::uno::RuntimeException
        void    removeKeyHandler( const css::uno::Reference< css::awt::XKeyHandler >& xHandler );
        /// @throws css::uno::RuntimeException
        void    addMouseClickHandler( const css::uno::Reference< css::awt::XMouseClickHandler >& xHandler );
        /// @throws css::uno::RuntimeException
        void    removeMouseClickHandler( const css::uno::Reference< css::awt::XMouseClickHandler >& xHandler );

        // state
        bool    hasKeyHandlers() const;
        bool    hasMouseClickListeners() const;

        // forwarding a NotifyEvent to the KeyListeners respectively MouseClickListeners
        bool    handleNotifyEvent( const NotifyEvent& _rEvent );

    private:
        ::std::unique_ptr< UserInputInterception_Data >   m_pData;
    };


} // namespace sfx2


#endif // INCLUDED_SFX2_USERINPUTINTERCEPTION_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
