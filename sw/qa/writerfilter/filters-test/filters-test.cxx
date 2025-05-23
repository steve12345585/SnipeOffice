/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <unotest/filters-test.hxx>
#include <test/bootstrapfixture.hxx>
#include <com/sun/star/document/XFilter.hpp>
#include <com/sun/star/io/WrongFormatException.hpp>
#include <com/sun/star/lang/WrappedTargetRuntimeException.hpp>

using namespace ::com::sun::star;

/**
 * Unit test invoking sw/source/writerfilter/ only.
 *
 * This does only minimal testing, checking if the filter crashes and returns
 * the expected bool value for given inputs. More fine-grained tests can be
 * found under sw/qa/extras/rtfimport/.
 */
class RtfTest : public test::FiltersTest, public test::BootstrapFixture
{
public:
    virtual void setUp() override;

    virtual bool load(const OUString&, const OUString& rURL, const OUString&, SfxFilterFlags,
                      SotClipboardFormatId, unsigned int) override;

private:
    uno::Reference<document::XFilter> m_xFilter;
};

void RtfTest::setUp()
{
    test::BootstrapFixture::setUp();

    m_xFilter.set(m_xSFactory->createInstance(u"com.sun.star.comp.Writer.RtfFilter"_ustr),
                  uno::UNO_QUERY_THROW);
}

bool RtfTest::load(const OUString&, const OUString& rURL, const OUString&, SfxFilterFlags,
                   SotClipboardFormatId, unsigned int)
{
    uno::Sequence<beans::PropertyValue> aDescriptor = { beans::PropertyValue(
        u"URL"_ustr, sal_Int32(0), uno::Any(rURL), beans::PropertyState_DIRECT_VALUE) };
    try
    {
        return m_xFilter->filter(aDescriptor);
    }
    catch (const lang::WrappedTargetRuntimeException& rWrapped)
    {
        io::WrongFormatException e;
        if (rWrapped.TargetException >>= e)
        {
            return false;
        }
        throw;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

CPPUNIT_TEST_FIXTURE(RtfTest, testFilter)
{
#ifndef DISABLE_CVE_TESTS
#if defined _WIN32 && defined _ARM64_
// skip for windows arm64 build
#else
    testDir(OUString(), m_directories.getURLFromSrc(u"/sw/qa/writerfilter/filters-test/data/"));
#endif
#endif
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
