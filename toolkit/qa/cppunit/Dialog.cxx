/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/bootstrapfixture.hxx>
#include <unotest/macros_test.hxx>

#include <com/sun/star/awt/UnoControlDialog.hpp>
#include <com/sun/star/awt/XUnoControlDialog.hpp>
#include <com/sun/star/awt/XControlModel.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/lang/XMultiComponentFactory.hpp>

#include <comphelper/processfactory.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <vcl/vclptr.hxx>
#include <vcl/window.hxx>

using namespace css;

namespace
{
/// Test dialogs constructed via UNO
class DialogTest : public test::BootstrapFixture, public unotest::MacrosTest
{
};

CPPUNIT_TEST_FIXTURE(DialogTest, testDialogSizeable)
{
    uno::Reference<awt::XDialog> xDialog;
    uno::Reference<lang::XMultiComponentFactory> xFactory(m_xContext->getServiceManager(),
                                                          uno::UNO_SET_THROW);
    uno::Reference<awt::XControlModel> xControlModel(
        xFactory->createInstanceWithContext(u"com.sun.star.awt.UnoControlDialogModel"_ustr,
                                            m_xContext),
        uno::UNO_QUERY_THROW);

    uno::Reference<beans::XPropertySet> xPropSet(xControlModel, uno::UNO_QUERY_THROW);
    xPropSet->setPropertyValue(u"Sizeable"_ustr, uno::Any(true));

    uno::Reference<awt::XUnoControlDialog> xControl = awt::UnoControlDialog::create(m_xContext);
    xControl->setModel(xControlModel);
    xControl->setVisible(true);
    xDialog.set(xControl, uno::UNO_QUERY_THROW);
    xDialog->execute();

    VclPtr<vcl::Window> pWindow = VCLUnoHelper::GetWindow(xControl->getPeer());
    CPPUNIT_ASSERT(pWindow);
    CPPUNIT_ASSERT(pWindow->GetStyle() & WB_SIZEABLE);

    xDialog->endExecute();
    css::uno::Reference<css::lang::XComponent>(xDialog, css::uno::UNO_QUERY_THROW)->dispose();
    css::uno::Reference<css::lang::XComponent>(xControlModel, css::uno::UNO_QUERY_THROW)->dispose();
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
