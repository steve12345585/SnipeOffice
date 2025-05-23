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

#include <algorithm>
#include <cassert>

#include <com/sun/star/bridge/BridgeExistsException.hpp>
#include <com/sun/star/connection/XConnection.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/uno/Exception.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/RuntimeException.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/uno/XInterface.hpp>
#include <cppuhelper/supportsservice.hxx>
#include <o3tl/safeint.hxx>
#include <rtl/ref.hxx>
#include <sal/log.hxx>
#include <sal/types.h>

#include "bridge.hxx"
#include "bridgefactory.hxx"

namespace binaryurp {

void BridgeFactory::removeBridge(
    css::uno::Reference< css::bridge::XBridge > const & bridge)
{
    assert(bridge.is());
    OUString n(bridge->getName());
    osl::MutexGuard g(m_aMutex);
    if (n.isEmpty())
    {
        std::erase(unnamed_, bridge);
    }
    else
    {
        BridgeMap::iterator i(named_.find(n));
        if (i != named_.end() && i->second == bridge)
            named_.erase(i);
    }
}

BridgeFactory::BridgeFactory():
    BridgeFactoryBase(m_aMutex)
{
}

BridgeFactory::~BridgeFactory() {}

OUString BridgeFactory::getImplementationName()
{
    return u"com.sun.star.comp.bridge.BridgeFactory"_ustr;
}

sal_Bool BridgeFactory::supportsService(OUString const & ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

css::uno::Sequence< OUString > BridgeFactory::getSupportedServiceNames()
{
    return { u"com.sun.star.bridge.BridgeFactory"_ustr };
}

css::uno::Reference< css::bridge::XBridge > BridgeFactory::createBridge(
    OUString const & sName, OUString const & sProtocol,
    css::uno::Reference< css::connection::XConnection > const & aConnection,
    css::uno::Reference< css::bridge::XInstanceProvider > const &
        anInstanceProvider)
{
    rtl::Reference< Bridge > b;
    {
        osl::MutexGuard g(m_aMutex);
        if (rBHelper.bDisposed) {
            throw css::lang::DisposedException(
                u"BridgeFactory disposed"_ustr,
                getXWeak());
        }
        if (named_.find(sName) != named_.end()) {
            throw css::bridge::BridgeExistsException(
                sName, getXWeak());
        }
        if (sProtocol != "urp" || !aConnection.is()) {
            throw css::lang::IllegalArgumentException(
                (u"BridgeFactory::createBridge: sProtocol != urp ||"
                 " aConnection == null"_ustr),
                getXWeak(), -1);
        }
        b.set(new Bridge(this, sName, aConnection, anInstanceProvider));
        if (sName.isEmpty()) {
            unnamed_.emplace_back(b.get());
        } else {
            named_[sName] = b.get();
        }
    }
    b->start();
    return b;
}

css::uno::Reference< css::bridge::XBridge > BridgeFactory::getBridge(
    OUString const & sName)
{
    osl::MutexGuard g(m_aMutex);
    BridgeMap::iterator i(named_.find(sName));
    return i == named_.end()
        ? css::uno::Reference< css::bridge::XBridge >() : i->second;
}

css::uno::Sequence< css::uno::Reference< css::bridge::XBridge > >
BridgeFactory::getExistingBridges() {
    osl::MutexGuard g(m_aMutex);
    if (unnamed_.size() > SAL_MAX_INT32) {
        throw css::uno::RuntimeException(
            u"BridgeFactory::getExistingBridges: too many"_ustr,
            getXWeak());
    }
    sal_Int32 n = static_cast< sal_Int32 >(unnamed_.size());
    if (named_.size() > o3tl::make_unsigned(SAL_MAX_INT32 - n)) {
        throw css::uno::RuntimeException(
            u"BridgeFactory::getExistingBridges: too many"_ustr,
            getXWeak());
    }
    n = static_cast< sal_Int32 >(n + named_.size());
    css::uno::Sequence< css::uno::Reference< css::bridge::XBridge > > s(n);
    auto r = asNonConstRange(s);
    sal_Int32 i = 0;
    for (auto const& item : unnamed_)
        r[i++] = item;

    for (auto const& item : named_)
        r[i++] = item.second;

    return s;
}

void BridgeFactory::disposing() {
    BridgeVector l1;
    BridgeMap l2;
    {
        osl::MutexGuard g(m_aMutex);
        l1.swap(unnamed_);
        l2.swap(named_);
    }
    for (auto const& item : l1)
    {
        try {
            css::uno::Reference<css::lang::XComponent>(
                item, css::uno::UNO_QUERY_THROW)->dispose();
        } catch (css::uno::Exception & e) {
            SAL_WARN("binaryurp", "ignoring " << e);
        }
    }
    for (auto const& item : l2)
    {
        try {
            css::uno::Reference<css::lang::XComponent>(
                item.second, css::uno::UNO_QUERY_THROW)->dispose();
        } catch (css::uno::Exception & e) {
            SAL_WARN("binaryurp", "ignoring " << e);
        }
    }
}

}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_comp_bridge_BridgeFactory_get_implementation(
    css::uno::XComponentContext* , css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new binaryurp::BridgeFactory);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
