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

#ifndef INCLUDED_COMPHELPER_COMPONENTBASE_HXX
#define INCLUDED_COMPHELPER_COMPONENTBASE_HXX

#include <comphelper/comphelperdllapi.h>
#include <cppuhelper/interfacecontainer.h>


namespace comphelper
{


    //= ComponentBase

    class COMPHELPER_DLLPUBLIC ComponentBase
    {
    protected:
        /** creates a ComponentBase instance

            The instance is not initialized. As a consequence, every ComponentMethodGuard instantiated for
            this component will throw a css::lang::NotInitializedException,
            until ->setInitialized() is called.
        */
        ComponentBase( ::cppu::OBroadcastHelper& _rBHelper )
            :m_rBHelper( _rBHelper )
            ,m_bInitialized( false )
        {
        }

        struct NoInitializationNeeded { };

        /** creates a ComponentBase instance

            The instance is already initialized, so there's no need to call setInitialized later on. Use this
            constructor for component implementations which do not require explicit initialization.
        */
        ComponentBase( ::cppu::OBroadcastHelper& _rBHelper, NoInitializationNeeded )
            :m_rBHelper( _rBHelper )
            ,m_bInitialized( true )
        {
        }

        ~ComponentBase() COVERITY_NOEXCEPT_FALSE {}

        /** marks the instance as initialized

            Subsequent instantiations of a ComponentMethodGuard won't throw the NotInitializedException now.
        */
        void setInitialized()    { m_bInitialized = true; }

    public:
        /// helper struct to grant access to selected public methods to the ComponentMethodGuard class
        struct GuardAccess { friend class ComponentMethodGuard; private: GuardAccess() { } };

        /// retrieves the component's mutex
        ::osl::Mutex&   getMutex( GuardAccess )                 { return getMutex(); }
        /// checks whether the component is already disposed, throws a DisposedException if so.
        void                    checkDisposed( GuardAccess ) const;
        /// checks whether the component is already initialized, throws a NotInitializedException if not.
        void                    checkInitialized( GuardAccess ) const;

    protected:
        /// retrieves the component's broadcast helper
        ::cppu::OBroadcastHelper&   getBroadcastHelper()    { return m_rBHelper; }
        /// retrieves the component's mutex
        ::osl::Mutex&               getMutex()              { return m_rBHelper.rMutex; }
        /// determines whether the instance is already disposed
        bool                        impl_isDisposed() const { return m_rBHelper.bDisposed; }

        /// determines whether the component is already initialized
        bool
                impl_isInitialized_nothrow() const { return m_bInitialized; }

        /** returns the context to be used when throwing exceptions

            The default implementation returns <NULL/>.
        */
        static css::uno::Reference< css::uno::XInterface >
                getComponent();

    private:
        ::cppu::OBroadcastHelper&   m_rBHelper;
        bool                        m_bInitialized;
    };

    class ComponentMethodGuard
    {
    public:
        enum class MethodType
        {
            /// allow the method to be called only when being initialized and not being disposed
            Default,
            /// allow the method to be called without being initialized
            WithoutInit

        };

        ComponentMethodGuard( ComponentBase& _rComponent, const MethodType _eType = MethodType::Default )
            :m_aMutexGuard( _rComponent.getMutex( ComponentBase::GuardAccess() ) )
        {
            if ( _eType != MethodType::WithoutInit )
                _rComponent.checkInitialized( ComponentBase::GuardAccess() );
            _rComponent.checkDisposed( ComponentBase::GuardAccess() );
        }

        void clear()
        {
            m_aMutexGuard.clear();
        }

    private:
        osl::ClearableMutexGuard m_aMutexGuard;
    };


} // namespace ComponentBase


#endif // INCLUDED_COMPHELPER_COMPONENTBASE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
