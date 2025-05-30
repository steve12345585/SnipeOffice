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

#include <com/sun/star/awt/MessageBoxButtons.hpp>
#include <com/sun/star/awt/Rectangle.hpp>
#include <com/sun/star/awt/Toolkit.hpp>
#include <com/sun/star/awt/XMessageBox.hpp>
#include <com/sun/star/awt/XWindowPeer.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/frame/DispatchDescriptor.hpp>
#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/frame/XDispatch.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/XStatusListener.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/DeploymentException.hpp>
#include <com/sun/star/uno/Exception.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/RuntimeException.hpp>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/uno/XInterface.hpp>
#include <com/sun/star/util/URL.hpp>
#include <cppuhelper/factory.hxx>
#include <cppuhelper/implbase2.hxx>
#include <cppuhelper/implementationentry.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/weak.hxx>
#include <rtl/ustring.hxx>
#include <sal/types.h>
#include <uno/lbnames.h>

namespace {

class Provider:
    public cppu::WeakImplHelper2<
        css::lang::XServiceInfo, css::frame::XDispatchProvider >
{
public:
    Provider(const Provider&) = delete;
    const Provider& operator=(const Provider&) = delete;

    static css::uno::Reference< css::uno::XInterface > SAL_CALL static_create(
        css::uno::Reference< css::uno::XComponentContext > const & xContext)
    { return static_cast< cppu::OWeakObject * >(new Provider(xContext)); }

    static rtl::OUString SAL_CALL static_getImplementationName();

    static css::uno::Sequence< rtl::OUString > SAL_CALL
    static_getSupportedServiceNames();

private:
    explicit Provider(
        css::uno::Reference< css::uno::XComponentContext > const & context):
        context_(context) { assert(context.is()); }

    virtual ~Provider() {}

    virtual rtl::OUString SAL_CALL getImplementationName() override
    { return static_getImplementationName(); }

    virtual sal_Bool SAL_CALL supportsService(rtl::OUString const & ServiceName) override
    { return cppu::supportsService(this, ServiceName); }

    virtual css::uno::Sequence< rtl::OUString > SAL_CALL
    getSupportedServiceNames() override
    { return static_getSupportedServiceNames(); }

    virtual css::uno::Reference< css::frame::XDispatch > SAL_CALL queryDispatch(
        css::util::URL const &, rtl::OUString const &, sal_Int32) override;

    virtual css::uno::Sequence< css::uno::Reference< css::frame::XDispatch > >
    SAL_CALL queryDispatches(
        css::uno::Sequence< css::frame::DispatchDescriptor > const & Requests) override;

    css::uno::Reference< css::uno::XComponentContext > context_;
};

rtl::OUString Provider::static_getImplementationName() {
    return rtl::OUString("com.sun.star.comp.test.deployment.passive_native");
}

css::uno::Sequence< rtl::OUString > Provider::static_getSupportedServiceNames()
{
    rtl::OUString name("com.sun.star.test.deployment.passive_native");
    return css::uno::Sequence< rtl::OUString >(&name, 1);
}

css::uno::Reference< css::frame::XDispatch > Provider::queryDispatch(
    css::util::URL const &, rtl::OUString const &, sal_Int32)
{
    css::uno::Reference< css::frame::XDispatch > dispatch;
    if (!(context_->getValueByName(
              "/singletons/com.sun.star.test.deployment."
              "passive_native_singleton") >>=
          dispatch) ||
        !dispatch.is())
    {
        throw css::uno::DeploymentException(
            "component context fails to supply singleton"
            " com.sun.star.test.deployment.passive_native_singleton of type"
            " com.sun.star.frame.XDispatch",
            context_);
    }
    return dispatch;
}

css::uno::Sequence< css::uno::Reference< css::frame::XDispatch > >
Provider::queryDispatches(
    css::uno::Sequence< css::frame::DispatchDescriptor > const & Requests)
{
    css::uno::Sequence< css::uno::Reference< css::frame::XDispatch > > s(
        Requests.getLength());
    for (sal_Int32 i = 0; i < s.getLength(); ++i) {
        s[i] = queryDispatch(
            Requests[i].FeatureURL, Requests[i].FrameName,
            Requests[i].SearchFlags);
    }
    return s;
}

class Dispatch:
    public cppu::WeakImplHelper2<
        css::lang::XServiceInfo, css::frame::XDispatch >
{
public:
    Dispatch(const Dispatch&) = delete;
    const Dispatch& operator=(const Dispatch&) = delete;

    static css::uno::Reference< css::uno::XInterface > SAL_CALL static_create(
        css::uno::Reference< css::uno::XComponentContext > const & xContext)
    { return static_cast< cppu::OWeakObject * >(new Dispatch(xContext)); }

    static rtl::OUString SAL_CALL static_getImplementationName();

    static css::uno::Sequence< rtl::OUString > SAL_CALL
    static_getSupportedServiceNames()
    { return css::uno::Sequence< rtl::OUString >(); }

private:
    explicit Dispatch(
        css::uno::Reference< css::uno::XComponentContext > const & context):
        context_(context) { assert(context.is()); }

    virtual ~Dispatch() {}

    virtual rtl::OUString SAL_CALL getImplementationName() override
    { return static_getImplementationName(); }

    virtual sal_Bool SAL_CALL supportsService(rtl::OUString const & ServiceName) override
    { return cppu::supportsService(this, ServiceName); }

    virtual css::uno::Sequence< rtl::OUString > SAL_CALL
    getSupportedServiceNames() override
    { return static_getSupportedServiceNames(); }

    virtual void SAL_CALL dispatch(
        css::util::URL const &,
        css::uno::Sequence< css::beans::PropertyValue > const &) override;

    virtual void SAL_CALL addStatusListener(
        css::uno::Reference< css::frame::XStatusListener > const &,
        css::util::URL const &) override
    {}

    virtual void SAL_CALL removeStatusListener(
        css::uno::Reference< css::frame::XStatusListener > const &,
        css::util::URL const &) override
    {}

    css::uno::Reference< css::uno::XComponentContext > context_;
};

rtl::OUString Dispatch::static_getImplementationName() {
    return rtl::OUString(
        "com.sun.star.comp.test.deployment.passive_native_singleton");
}

void Dispatch::dispatch(
    css::util::URL const &,
    css::uno::Sequence< css::beans::PropertyValue > const &)
{
    css::uno::Reference< css::frame::XDesktop2 > xDesktop = css::frame::Desktop::create(context_);
    css::uno::Reference< css::frame::XFrame >    xFrame = xDesktop->getCurrentFrame();
    css::uno::Reference< css::awt::XWindowPeer > xWindowPeer( xFrame->getComponentWindow(), css::uno::UNO_QUERY_THROW );
    css::uno::Reference< css::awt::XToolkit2 >    xToolkit = css::awt::Toolkit::create(context_);
    css::uno::Reference< css::awt::XMessageBox > box(
        xToolkit->createMessageBox(
            xWindowPeer,
            css::awt::MessageBoxType_INFOBOX,
            css::awt::MessageBoxButtons::BUTTONS_OK, "passive", "native"),
        css::uno::UNO_SET_THROW);

    box->execute();

    css::uno::Reference< css::lang::XComponent > xComponent(box, css::uno::UNO_QUERY_THROW);
    xComponent->dispose();
}

cppu::ImplementationEntry const services[] = {
    { &Provider::static_create, &Provider::static_getImplementationName,
      &Provider::static_getSupportedServiceNames,
      &cppu::createSingleComponentFactory, nullptr, 0 },
    { &Dispatch::static_create, &Dispatch::static_getImplementationName,
      &Dispatch::static_getSupportedServiceNames,
      &cppu::createSingleComponentFactory, nullptr, 0 },
    { nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

}

extern "C" SAL_DLLPUBLIC_EXPORT void * component_getFactory(
    char const * pImplName, void * pServiceManager, void * pRegistryKey)
{
    return cppu::component_getFactoryHelper(
        pImplName, pServiceManager, pRegistryKey, services);
}

extern "C" SAL_DLLPUBLIC_EXPORT void
component_getImplementationEnvironment(
    char const ** ppEnvTypeName, uno_Environment **)
{
    *ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
