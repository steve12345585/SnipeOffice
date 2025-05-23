/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <rtl/ref.hxx>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

namespace rtl_ref
{

namespace {

class MoveTestClass
{
private:
    bool m_bIncFlag;
    long m_nRef;
public:
    MoveTestClass(): m_bIncFlag(false), m_nRef(0) { }

    // There should never be more than two references to this class as it
    // is used as a test class for move functions. One reference being the
    // original reference and the second being the test reference
    void acquire()
    {
        if(m_bIncFlag)
        {
            ++m_nRef;
            m_bIncFlag = false;
        }
        else
            CPPUNIT_FAIL("RC was incremented when in should not have been");
    }

    void release() { --m_nRef; }

    long use_count() { return m_nRef; }

    void set_inc_flag() { m_bIncFlag = true; }
};

}

static rtl::Reference< MoveTestClass > get_reference( MoveTestClass* pcTestClass )
{
    // constructor will increment the reference count
    pcTestClass->set_inc_flag();
    rtl::Reference< MoveTestClass > tmp(pcTestClass);
    return tmp;
}

class TestReferenceRefCounting : public CppUnit::TestFixture
{
    void testMove()
    {
        MoveTestClass cTestClass;

        // constructor will increment the reference count
        cTestClass.set_inc_flag();
        rtl::Reference< MoveTestClass > test1( &cTestClass );

        // move should not increment the reference count
        rtl::Reference< MoveTestClass > test2( std::move(test1) );
        CPPUNIT_ASSERT_EQUAL_MESSAGE("test2.use_count() == 1",
                               static_cast<long>(1), test2->use_count());

        // test1 now contains a null pointer
        CPPUNIT_ASSERT_MESSAGE("!test1.is()",
                               !test1.is()); // NOLINT(bugprone-use-after-move)

        // function return should move the reference
        test2 = get_reference( &cTestClass );
        CPPUNIT_ASSERT_EQUAL_MESSAGE("test2.use_count() == 1",
                               static_cast<long>(1), test2->use_count());

        // normal copy
        test2->set_inc_flag();
        test1 = test2;
        CPPUNIT_ASSERT_EQUAL_MESSAGE("test2.use_count() == 2",
                               static_cast<long>(2), test2->use_count());

        // use count should decrement
        test2 = rtl::Reference< MoveTestClass >();
        CPPUNIT_ASSERT_EQUAL_MESSAGE("test1.use_count() == 1",
                               static_cast<long>(1), test1->use_count());

        // move of a null pointer should not cause an error
        test1 = std::move(test2);

        CPPUNIT_ASSERT_MESSAGE("!test1.is()",
                               !test1.is());
        CPPUNIT_ASSERT_MESSAGE("!test2.is()",
                               !test2.is()); // NOLINT(bugprone-use-after-move)

        CPPUNIT_ASSERT_EQUAL_MESSAGE("cTestClass.use_count() == 0",
                               static_cast<long>(0), cTestClass.use_count());
    }

    CPPUNIT_TEST_SUITE(TestReferenceRefCounting);
    CPPUNIT_TEST(testMove);
    CPPUNIT_TEST_SUITE_END();
};

} // namespace rtl_ref
CPPUNIT_TEST_SUITE_REGISTRATION(rtl_ref::TestReferenceRefCounting);

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
