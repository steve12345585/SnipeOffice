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

#include <cassert>
#include <set>

#include <com/sun/star/configuration/XUpdate.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/uno/XInterface.hpp>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/weak.hxx>
#include <osl/mutex.hxx>
#include <rtl/ref.hxx>
#include <rtl/ustring.hxx>
#include <sal/types.h>

#include "broadcaster.hxx"
#include "components.hxx"
#include "lock.hxx"
#include "modifications.hxx"
#include "rootaccess.hxx"

namespace configmgr::update {

namespace {

class Service:
    public cppu::WeakImplHelper< css::configuration::XUpdate, css::lang::XServiceInfo >
{
public:
    explicit Service(const css::uno::Reference< css::uno::XComponentContext >& context):
        context_(context)
    {
        assert(context.is());
        lock_ = lock();
    }

private:
    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;

    virtual ~Service() override {}

    virtual void SAL_CALL insertExtensionXcsFile(
        sal_Bool shared, OUString const & fileUri) override;

    virtual void SAL_CALL insertExtensionXcuFile(
        sal_Bool shared, OUString const & fileUri) override;

    virtual void SAL_CALL removeExtensionXcuFile(OUString const & fileUri) override;

    virtual void SAL_CALL insertModificationXcuFile(
        OUString const & fileUri,
        css::uno::Sequence< OUString > const & includedPaths,
        css::uno::Sequence< OUString > const & excludedPaths) override;

    OUString SAL_CALL getImplementationName() override {
        return u"com.sun.star.comp.configuration.Update"_ustr;
    }

    sal_Bool SAL_CALL supportsService(OUString const & ServiceName) override {
        return cppu::supportsService(this, ServiceName);
    }

    css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override {
        return {u"com.sun.star.configuration.Update_Service"_ustr};
    }

    std::shared_ptr<osl::Mutex> lock_;
    css::uno::Reference< css::uno::XComponentContext > context_;
};

void Service::insertExtensionXcsFile(
    sal_Bool shared, OUString const & fileUri)
{
    osl::MutexGuard g(*lock_);
    Components::getSingleton(context_).insertExtensionXcsFile(shared, fileUri);
}

void Service::insertExtensionXcuFile(
    sal_Bool shared, OUString const & fileUri)
{
    Broadcaster bc;
    {
        osl::MutexGuard g(*lock_);
        Components & components = Components::getSingleton(context_);
        Modifications mods;
        components.insertExtensionXcuFile(shared, fileUri, &mods);
        components.initGlobalBroadcaster(
            mods, rtl::Reference< RootAccess >(), &bc);
    }
    bc.send();
}

void Service::removeExtensionXcuFile(OUString const & fileUri)
{
    Broadcaster bc;
    {
        osl::MutexGuard g(*lock_);
        Components & components = Components::getSingleton(context_);
        Modifications mods;
        components.removeExtensionXcuFile(fileUri, &mods);
        components.initGlobalBroadcaster(
            mods, rtl::Reference< RootAccess >(), &bc);
    }
    bc.send();
}

void Service::insertModificationXcuFile(
    OUString const & fileUri,
    css::uno::Sequence< OUString > const & includedPaths,
    css::uno::Sequence< OUString > const & excludedPaths)
{
    Broadcaster bc;
    {
        osl::MutexGuard g(*lock_);
        Components & components = Components::getSingleton(context_);
        Modifications mods;
        components.insertModificationXcuFile(
            fileUri, includedPaths, excludedPaths, &mods);
        components.initGlobalBroadcaster(
            mods, rtl::Reference< RootAccess >(), &bc);
    }
    bc.send();
}

}
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_comp_configuration_Update_get_implementation(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const& )
{
    return cppu::acquire(new configmgr::update::Service(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
