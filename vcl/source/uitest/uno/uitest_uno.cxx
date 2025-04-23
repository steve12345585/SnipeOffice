/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <comphelper/compbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/ui/test/XUITest.hpp>

#include <memory>

#include <vcl/uitest/uitest.hxx>
#include <vcl/svapp.hxx>
#include <vcl/window.hxx>

#include "uiobject_uno.hxx"

namespace
{
    typedef ::comphelper::WeakComponentImplHelper <
        css::ui::test::XUITest, css::lang::XServiceInfo
        > UITestBase;

class UITestUnoObj : public UITestBase
{
public:

    UITestUnoObj();

    sal_Bool SAL_CALL executeCommand(const OUString& rCommand) override;

    sal_Bool SAL_CALL executeCommandWithParameters(const OUString& rCommand,
        const css::uno::Sequence< css::beans::PropertyValue >& rArgs) override;

    sal_Bool SAL_CALL executeDialog(const OUString& rCommand) override;

    css::uno::Reference<css::ui::test::XUIObject> SAL_CALL getTopFocusWindow() override;

    css::uno::Reference<css::ui::test::XUIObject> SAL_CALL getFloatWindow() override;

    OUString SAL_CALL getImplementationName() override;

    sal_Bool SAL_CALL supportsService(OUString const & ServiceName) override;

    css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;
};

}

UITestUnoObj::UITestUnoObj()
{
}

sal_Bool SAL_CALL UITestUnoObj::executeCommand(const OUString& rCommand)
{
    SolarMutexGuard aGuard;
    return UITest::executeCommand(rCommand);
}

sal_Bool SAL_CALL UITestUnoObj::executeCommandWithParameters(const OUString& rCommand,
    const css::uno::Sequence< css::beans::PropertyValue >& rArgs)
{
    SolarMutexGuard aGuard;
    return UITest::executeCommandWithParameters(rCommand,rArgs);
}

sal_Bool SAL_CALL UITestUnoObj::executeDialog(const OUString& rCommand)
{
    SolarMutexGuard aGuard;
    return UITest::executeDialog(rCommand);
}

css::uno::Reference<css::ui::test::XUIObject> SAL_CALL UITestUnoObj::getTopFocusWindow()
{
    SolarMutexGuard aGuard;
    std::unique_ptr<UIObject> pObj = UITest::getFocusTopWindow();
    if (!pObj)
        throw css::uno::RuntimeException(u"UITest::getFocusTopWindow did not find a window"_ustr);
    return new UIObjectUnoObj(std::move(pObj));
}

css::uno::Reference<css::ui::test::XUIObject> SAL_CALL UITestUnoObj::getFloatWindow()
{
    SolarMutexGuard aGuard;
    std::unique_ptr<UIObject> pObj = UITest::getFloatWindow();
    if (!pObj)
        throw css::uno::RuntimeException(u"UITest::getFloatWindow did not find a window"_ustr);
    return new UIObjectUnoObj(std::move(pObj));
}

OUString SAL_CALL UITestUnoObj::getImplementationName()
{
    return u"org.libreoffice.uitest.UITest"_ustr;
}

sal_Bool UITestUnoObj::supportsService(OUString const & ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

css::uno::Sequence<OUString> UITestUnoObj::getSupportedServiceNames()
{
    return { u"com.sun.star.ui.test.UITest"_ustr };
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
UITest_get_implementation(css::uno::XComponentContext*, css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UITestUnoObj());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
