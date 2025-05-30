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

#pragma once

#include <sal/config.h>

#include <atomic>
#include <cstddef>

#include <cppuhelper/implbase1.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XContainerListener.hpp>

#include "FilteredContainer.hxx"

namespace dbaccess
{
    // OViewContainer
    class OViewContainer :  public OFilteredContainer,
                            public ::cppu::ImplHelper1< css::container::XContainerListener>
    {
    public:
        /** ctor of the container. The parent has to support the <type scope="css::sdbc">XConnection</type>
            interface.<BR>
            @param          _rParent            the object which acts as parent for the container.
                                                all refcounting is rerouted to this object
            @param          _rMutex             the access safety object of the parent
            @param          _rTableFilter       restricts the visible tables by name
            @param          _rTableTypeFilter   restricts the visible tables by type
            @see            construct
        */
        OViewContainer( ::cppu::OWeakObject& _rParent,
                        ::osl::Mutex& _rMutex,
                        const css::uno::Reference< css::sdbc::XConnection >& _xCon,
                        bool _bCase,
                        IRefreshListener*   _pRefreshListener,
                        std::atomic<std::size_t>& _nInAppend
                        );

        virtual ~OViewContainer() override;

    protected:
        // OFilteredContainer overridables
        virtual OUString getTableTypeRestriction() const override;

    private:
        virtual void SAL_CALL acquire() noexcept override { OFilteredContainer::acquire();}
        virtual void SAL_CALL release() noexcept override { OFilteredContainer::release();}
    // css::lang::XServiceInfo
        DECLARE_SERVICE_INFO();

        // XEventListener
        virtual void SAL_CALL disposing( const css::lang::EventObject& Source ) override;
        // XContainerListener
        virtual void SAL_CALL elementInserted( const css::container::ContainerEvent& Event ) override;
        virtual void SAL_CALL elementRemoved( const css::container::ContainerEvent& Event ) override;
        virtual void SAL_CALL elementReplaced( const css::container::ContainerEvent& Event ) override;

        // ::connectivity::sdbcx::OCollection
        virtual ::css::uno::Reference< css::beans::XPropertySet >       createObject(const OUString& _rName) override;
        virtual css::uno::Reference< css::beans::XPropertySet >   createDescriptor() override;
        virtual css::uno::Reference< css::beans::XPropertySet > appendObject( const OUString& _rForName, const css::uno::Reference< css::beans::XPropertySet >& descriptor ) override;
        virtual void dropObject(sal_Int32 _nPos, const OUString& _sElementName) override;

        using OFilteredContainer::disposing;

        bool m_bInElementRemoved;
    };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
