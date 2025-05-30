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

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <osl/mutex.hxx>
#include <comphelper/interfacecontainer3.hxx>
#include <cppuhelper/implbase.hxx>
#include <com/sun/star/beans/XVetoableChangeListener.hpp>

using namespace ::osl;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::uno;

namespace
{
class TestInterfaceContainer3 : public CppUnit::TestFixture
{
public:
    void test1();

    CPPUNIT_TEST_SUITE(TestInterfaceContainer3);
    CPPUNIT_TEST(test1);
    CPPUNIT_TEST_SUITE_END();
};

class TestListener : public cppu::WeakImplHelper<XVetoableChangeListener>
{
public:
    // Methods
    virtual void SAL_CALL disposing(const css::lang::EventObject& /*Source*/) override {}

    virtual void SAL_CALL vetoableChange(const css::beans::PropertyChangeEvent& /*aEvent*/) override
    {
    }
};

void TestInterfaceContainer3::test1()
{
    Mutex mutex;

    {
        comphelper::OInterfaceContainerHelper3<XVetoableChangeListener> helper(mutex);

        Reference<XVetoableChangeListener> r1 = new TestListener;
        Reference<XVetoableChangeListener> r2 = new TestListener;
        Reference<XVetoableChangeListener> r3 = new TestListener;

        helper.addInterface(r1);
        helper.addInterface(r2);
        helper.addInterface(r3);

        helper.disposeAndClear(EventObject());
    }

    {
        comphelper::OInterfaceContainerHelper3<XVetoableChangeListener> helper(mutex);

        Reference<XVetoableChangeListener> r1 = new TestListener;
        Reference<XVetoableChangeListener> r2 = new TestListener;
        Reference<XVetoableChangeListener> r3 = new TestListener;

        helper.addInterface(r1);
        helper.addInterface(r2);
        helper.addInterface(r3);

        comphelper::OInterfaceIteratorHelper3 iterator(helper);

        while (iterator.hasMoreElements())
            iterator.next()->vetoableChange(PropertyChangeEvent());

        helper.disposeAndClear(EventObject());
    }

    {
        comphelper::OInterfaceContainerHelper3<XVetoableChangeListener> helper(mutex);

        Reference<XVetoableChangeListener> r1 = new TestListener;
        Reference<XVetoableChangeListener> r2 = new TestListener;
        Reference<XVetoableChangeListener> r3 = new TestListener;

        helper.addInterface(r1);
        helper.addInterface(r2);
        helper.addInterface(r3);

        comphelper::OInterfaceIteratorHelper3 iterator(helper);

        iterator.next()->vetoableChange(PropertyChangeEvent());
        iterator.remove();
        iterator.next()->vetoableChange(PropertyChangeEvent());
        iterator.remove();
        iterator.next()->vetoableChange(PropertyChangeEvent());
        iterator.remove();

        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0), helper.getLength());
        helper.disposeAndClear(EventObject());
    }

    {
        comphelper::OInterfaceContainerHelper3<XVetoableChangeListener> helper(mutex);

        Reference<XVetoableChangeListener> r1 = new TestListener;
        Reference<XVetoableChangeListener> r2 = new TestListener;
        Reference<XVetoableChangeListener> r3 = new TestListener;

        helper.addInterface(r1);
        helper.addInterface(r2);
        helper.addInterface(r3);

        {
            comphelper::OInterfaceIteratorHelper3 iterator(helper);
            while (iterator.hasMoreElements())
            {
                Reference<XVetoableChangeListener> r = iterator.next();
                if (r == r1)
                    iterator.remove();
            }
        }
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(2), helper.getLength());
        {
            comphelper::OInterfaceIteratorHelper3 iterator(helper);
            while (iterator.hasMoreElements())
            {
                Reference<XVetoableChangeListener> r = iterator.next();
                CPPUNIT_ASSERT(r != r1);
                CPPUNIT_ASSERT(r == r2 || r == r3);
            }
        }

        helper.disposeAndClear(EventObject());
    }

    {
        comphelper::OInterfaceContainerHelper3<XVetoableChangeListener> helper(mutex);

        Reference<XVetoableChangeListener> r1 = new TestListener;

        helper.addInterface(r1);

        {
            comphelper::OInterfaceIteratorHelper3 iterator(helper);
            iterator.next();
            iterator.remove();
        }
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0), helper.getLength());
    }
}

CPPUNIT_TEST_SUITE_REGISTRATION(TestInterfaceContainer3);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
