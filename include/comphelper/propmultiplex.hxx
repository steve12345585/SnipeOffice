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

#ifndef INCLUDED_COMPHELPER_PROPMULTIPLEX_HXX
#define INCLUDED_COMPHELPER_PROPMULTIPLEX_HXX

#include <com/sun/star/beans/XPropertyChangeListener.hpp>
#include <cppuhelper/implbase.hxx>
#include <comphelper/comphelperdllapi.h>
#include <rtl/ref.hxx>
#include <mutex>
#include <vector>

namespace com::sun::star::beans { class XPropertySet; }

//= property helper classes


namespace comphelper
{


    class OPropertyChangeMultiplexer;


    //= OPropertyChangeListener

    /// simple listener adapter for property sets
    class COMPHELPER_DLLPUBLIC OPropertyChangeListener
    {
        friend class OPropertyChangeMultiplexer;

        rtl::Reference<OPropertyChangeMultiplexer> m_xAdapter;
        // We have our own mutex here for the m_xAdapter field, because we sit between two different objects
        // which often have their own mutexes, and if we use a mutex from one of them,
        // we end up with ABBA deadlock risks.
        std::mutex                  m_aAdapterMutex;

    public:
        virtual ~OPropertyChangeListener();

        /// @throws css::uno::RuntimeException
        virtual void _propertyChanged(const css::beans::PropertyChangeEvent& _rEvent) = 0;
        /// @throws css::uno::RuntimeException
        virtual void _disposing(const css::lang::EventObject& _rSource);

    protected:
        /** If the derivee also owns the mutex which we know as reference, then call this within your
            derivee's dtor.
        */
        void    disposeAdapter();

    private:
        void    setAdapter( OPropertyChangeMultiplexer* _pAdapter );
    };


    //= OPropertyChangeMultiplexer

    /// multiplexer for property changes
    // workaround for incremental linking bugs in MSVC2019
    class SAL_DLLPUBLIC_TEMPLATE OPropertyChangeMultiplexer_Base : public cppu::WeakImplHelper< css::beans::XPropertyChangeListener > {};
    class COMPHELPER_DLLPUBLIC OPropertyChangeMultiplexer final : public OPropertyChangeMultiplexer_Base
    {
        friend class OPropertyChangeListener;
        std::vector< OUString >                         m_aProperties;
        css::uno::Reference< css::beans::XPropertySet>  m_xSet;
        OPropertyChangeListener*                        m_pListener;
        sal_Int32                                       m_nLockCount;
        bool                                            m_bListening        : 1;
        bool const                                      m_bAutoSetRelease   : 1;


        virtual ~OPropertyChangeMultiplexer() override;
    public:
        OPropertyChangeMultiplexer(OPropertyChangeListener* _pListener, const  css::uno::Reference< css::beans::XPropertySet>& _rxSet, bool _bAutoReleaseSet = true);

    // XEventListener
        virtual void SAL_CALL disposing( const  css::lang::EventObject& Source ) override;

    // XPropertyChangeListener
        virtual void SAL_CALL propertyChange( const  css::beans::PropertyChangeEvent& evt ) override;

        /// incremental lock
        void        lock();
        /// incremental unlock
        void        unlock();
        /// get the lock count
        sal_Int32   locked() const { return m_nLockCount; }

        void addProperty(const OUString& aPropertyName);
        void dispose();
    };


}   // namespace comphelper


#endif // INCLUDED_COMPHELPER_PROPMULTIPLEX_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
