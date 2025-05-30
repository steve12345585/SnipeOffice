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

#include <sal/config.h>

#include <vector>

#include <com/sun/star/lang/XComponent.hpp>
#include <unotools/eventlisteneradapter.hxx>
#include <osl/diagnose.h>
#include <cppuhelper/implbase.hxx>
#include <rtl/ref.hxx>

namespace utl
{

    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::lang;

    //= OEventListenerImpl

    class OEventListenerImpl : public ::cppu::WeakImplHelper< XEventListener >
    {
    protected:
        OEventListenerAdapter*          m_pAdapter;
        Reference< XEventListener >     m_xKeepMeAlive;
            // imagine an implementation of XComponent which holds it's listeners with a weak reference ...
            // would be very bad if we don't hold ourself
        Reference< XComponent >         m_xComponent;

    public:
        OEventListenerImpl( OEventListenerAdapter* _pAdapter, const Reference< XComponent >& _rxComp );

        void                            dispose();
        const Reference< XComponent >&  getComponent() const { return m_xComponent; }

    protected:
        virtual void SAL_CALL disposing( const EventObject& _rSource ) override;
    };

    OEventListenerImpl::OEventListenerImpl( OEventListenerAdapter* _pAdapter, const Reference< XComponent >& _rxComp )
        :m_pAdapter(_pAdapter)
    {
        OSL_ENSURE(m_pAdapter, "OEventListenerImpl::OEventListenerImpl: invalid adapter!");
        // no checks of _rxComp !!
        // (OEventListenerAdapter is responsible for this)

        // just in case addEventListener throws an exception ... don't initialize m_xKeepMeAlive before this
        // is done
        Reference< XEventListener > xMeMyselfAndI = this;
        _rxComp->addEventListener(xMeMyselfAndI);

        m_xComponent = _rxComp;
        m_xKeepMeAlive = std::move(xMeMyselfAndI);
    }

    void OEventListenerImpl::dispose()
    {
        if (m_xComponent.is())
        {
            if (m_xKeepMeAlive.is())
                m_xComponent->removeEventListener(m_xKeepMeAlive);
            m_xComponent.clear();
            m_xKeepMeAlive.clear();
        }
    }

    void SAL_CALL OEventListenerImpl::disposing( const EventObject& _rSource )
    {
        Reference< XEventListener > xDeleteUponLeaving = m_xKeepMeAlive;
        m_xKeepMeAlive.clear();

        m_pAdapter->_disposing(_rSource);
    }

    //= OEventListenerAdapterImpl

    struct OEventListenerAdapterImpl
    {
    public:
        std::vector< rtl::Reference<OEventListenerImpl> >  aListeners;
    };

    //= OEventListenerAdapter

    OEventListenerAdapter::OEventListenerAdapter()
        :m_pImpl(new OEventListenerAdapterImpl)
    {
    }

    OEventListenerAdapter::~OEventListenerAdapter()
    {
        stopAllComponentListening( );
    }

    void OEventListenerAdapter::stopComponentListening( const css::uno::Reference< css::lang::XComponent >& _rxComp )
    {
        if ( m_pImpl->aListeners.empty() )
            return;

        auto it = m_pImpl->aListeners.begin();
        do
        {
            rtl::Reference<OEventListenerImpl>& pListenerImpl = *it;
            if ((pListenerImpl->getComponent().get() == _rxComp.get()) || (pListenerImpl->getComponent() == _rxComp))
            {
                pListenerImpl->dispose();
                it = m_pImpl->aListeners.erase( it );
            }
            else
                ++it;
        }
        while ( it != m_pImpl->aListeners.end() );
    }

    void OEventListenerAdapter::stopAllComponentListening(  )
    {
        for ( const auto & i : m_pImpl->aListeners )
        {
            i->dispose();
        }
        m_pImpl->aListeners.clear();
    }

    void OEventListenerAdapter::startComponentListening( const Reference< XComponent >& _rxComp )
    {
        if (!_rxComp.is())
        {
            OSL_FAIL("OEventListenerAdapter::startComponentListening: invalid component!");
            return;
        }

        rtl::Reference<OEventListenerImpl> pListenerImpl = new OEventListenerImpl(this, _rxComp);
        m_pImpl->aListeners.emplace_back(pListenerImpl);
    }

}   // namespace utl

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
