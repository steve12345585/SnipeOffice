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

#include <comphelper/fileformat.h>

#include <sfx2/app.hxx>
#include <sfx2/docfilt.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/sfxsids.hrc>
#include <sfx2/fcontnr.hxx>

#include <svl/stritem.hxx>
#include <unotools/tempfile.hxx>

#include <iodetect.hxx>
#include <docsh.hxx>

typedef rtl::Reference<SwDocShell> SwDocShellRef;

using namespace ::com::sun::star;

/* Implementation of Filters test */

class SwFiltersTest
    : public test::FiltersTest
    , public test::BootstrapFixture
{
public:
    virtual bool load( const OUString &rFilter, const OUString &rURL,
        const OUString &rUserData, SfxFilterFlags nFilterFlags,
        SotClipboardFormatId nClipboardID, unsigned int nFilterVersion) override;
    virtual bool save( const OUString &rFilter, const OUString &rURL,
        const OUString &rUserData, SfxFilterFlags nFilterFlags,
        SotClipboardFormatId nClipboardID, unsigned int nFilterVersion) override;
    virtual void setUp() override;
    virtual void tearDown() override;

    // Ensure CVEs remain unbroken
    void testCVEs();

    CPPUNIT_TEST_SUITE(SwFiltersTest);
    CPPUNIT_TEST(testCVEs);
    CPPUNIT_TEST_SUITE_END();

private:
    bool filter( const OUString &rFilter, const OUString &rURL,
        const OUString &rUserData, SfxFilterFlags nFilterFlags,
        SotClipboardFormatId nClipboardID, unsigned int nFilterVersion, bool bExport);
    uno::Reference<uno::XInterface> m_xWriterComponent;
};

bool SwFiltersTest::load(const OUString &rFilter, const OUString &rURL,
    const OUString &rUserData, SfxFilterFlags nFilterFlags,
        SotClipboardFormatId nClipboardID, unsigned int nFilterVersion)
{
    return filter(rFilter, rURL, rUserData, nFilterFlags, nClipboardID, nFilterVersion, false);
}

bool SwFiltersTest::save(const OUString &rFilter, const OUString &rURL,
    const OUString &rUserData, SfxFilterFlags nFilterFlags,
        SotClipboardFormatId nClipboardID, unsigned int nFilterVersion)
{
    return filter(rFilter, rURL, rUserData, nFilterFlags, nClipboardID, nFilterVersion, true);
}

bool SwFiltersTest::filter(const OUString &rFilter, const OUString &rURL,
    const OUString &rUserData, SfxFilterFlags nFilterFlags,
        SotClipboardFormatId nClipboardID, unsigned int nFilterVersion, bool bExport)
{
    auto pFilter = std::make_shared<SfxFilter>(
        rFilter, OUString(), nFilterFlags,
        nClipboardID, OUString(), OUString(),
        rUserData, OUString());
    pFilter->SetVersion(nFilterVersion);

    SwDocShellRef xDocShRef = new SwDocShell;
    SfxMedium* pSrcMed = new SfxMedium(rURL, StreamMode::STD_READ);

    std::shared_ptr<const SfxFilter> pImportFilter;
    std::shared_ptr<const SfxFilter> pExportFilter;
    if (bExport)
    {
        SfxGetpApp()->GetFilterMatcher().GuessFilter(*pSrcMed, pImportFilter, SfxFilterFlags::IMPORT, SfxFilterFlags::NONE);
        pExportFilter = pFilter;
    }
    else
        pImportFilter = pFilter;

    pSrcMed->SetFilter(pImportFilter);

    if (rUserData == FILTER_TEXT_DLG)
    {
        pSrcMed->GetItemSet().Put(
            SfxStringItem(SID_FILE_FILTEROPTIONS, u"UTF8,LF,Liberation Mono,en-US"_ustr));
    }

    bool bLoaded = xDocShRef->DoLoad(pSrcMed);
    if (!bExport)
    {
        if (xDocShRef.is())
            xDocShRef->DoClose();
        return bLoaded;
    }

    // How come an error may be set, and still DoLoad() returns success? Strange...
    if (bLoaded)
        xDocShRef->ResetError();

    bool bSaved;
    {
        utl::TempFileNamed aTempFile;
        aTempFile.EnableKillingFile();
        SfxMedium aDstMed(aTempFile.GetURL(), StreamMode::STD_WRITE);
        aDstMed.SetFilter(pExportFilter);
        bSaved = xDocShRef->DoSaveAs(aDstMed);
    }
    if (xDocShRef.is())
        xDocShRef->DoClose();
    return bSaved;
}

#define isstorage SotClipboardFormatId::STRING

void SwFiltersTest::testCVEs()
{
    testDir(u"StarOffice XML (Writer)"_ustr,
            m_directories.getURLFromSrc(u"/sw/qa/core/data/xml/"),
            FILTER_XML,
            SfxFilterFlags::IMPORT | SfxFilterFlags::OWN | SfxFilterFlags::DEFAULT,
            isstorage, SOFFICE_FILEFORMAT_CURRENT);

    testDir(u"writer8"_ustr,
            m_directories.getURLFromSrc(u"/sw/qa/core/data/odt/"),
            FILTER_XML,
            SfxFilterFlags::IMPORT | SfxFilterFlags::OWN | SfxFilterFlags::DEFAULT,
            isstorage, SOFFICE_FILEFORMAT_CURRENT);
#if defined _WIN32 && defined _ARM64_
    // skip for windows arm64 build
#else
    testDir(u"MS Word 97"_ustr,
            m_directories.getURLFromSrc(u"/sw/qa/core/data/ww8/"),
            FILTER_WW8);
#endif
    testDir(u"MS WinWord 6.0"_ustr,
            m_directories.getURLFromSrc(u"/sw/qa/core/data/ww6/"),
            sWW6);

    testDir(u"MS WinWord 5"_ustr,
            m_directories.getURLFromSrc(u"/sw/qa/core/data/ww5/"),
            sWW5);

    testDir(u"Text (encoded)"_ustr,
            m_directories.getURLFromSrc(u"/sw/qa/core/data/txt/"),
            FILTER_TEXT_DLG);

    testDir(u"MS Word 2007 XML"_ustr,
            m_directories.getURLFromSrc(u"/sw/qa/core/data/ooxml/"),
            OUString(),
            SfxFilterFlags::STARONEFILTER);
#if defined _WIN32 && defined _ARM64_
    // skip for windows arm64 build
#else
    testDir(u"Rich Text Format"_ustr,
            m_directories.getURLFromSrc(u"/sw/qa/core/data/rtf/"),
            OUString(),
            SfxFilterFlags::STARONEFILTER);
#endif
    testDir(u"HTML"_ustr,
            m_directories.getURLFromSrc(u"/sw/qa/core/data/html/"),
            sHTML);

    testDir(u"T602Document"_ustr,
            m_directories.getURLFromSrc(u"/sw/qa/core/data/602/"),
            OUString(),
            SfxFilterFlags::STARONEFILTER);

    testDir(u"Rich Text Format"_ustr,
            m_directories.getURLFromSrc(u"/sw/qa/core/exportdata/rtf/"),
            OUString(),
            SfxFilterFlags::STARONEFILTER,
            SotClipboardFormatId::NONE,
            0,
            /*bExport=*/true);

    testDir(u"HTML"_ustr,
            m_directories.getURLFromSrc(u"/sw/qa/core/exportdata/html/"),
            sHTML,
            SfxFilterFlags::NONE,
            SotClipboardFormatId::NONE,
            0,
            /*bExport=*/true);

    testDir(u"MS Word 2007 XML"_ustr,
            m_directories.getURLFromSrc(u"/sw/qa/core/exportdata/ooxml/"),
            OUString(),
            SfxFilterFlags::STARONEFILTER,
            SotClipboardFormatId::NONE,
            0,
            /*bExport=*/true);

}

void SwFiltersTest::setUp()
{
    test::BootstrapFixture::setUp();

    //This is a bit of a fudge, we do this to ensure that SwGlobals::ensure,
    //which is a private symbol to us, gets called
    m_xWriterComponent =
        getMultiServiceFactory()->createInstance(u"com.sun.star.comp.Writer.TextDocument"_ustr);
    CPPUNIT_ASSERT_MESSAGE("no writer component!", m_xWriterComponent.is());
}

void SwFiltersTest::tearDown()
{
    uno::Reference<lang::XComponent>(m_xWriterComponent, uno::UNO_QUERY_THROW)->dispose();
    m_xWriterComponent.clear();
}

CPPUNIT_TEST_SUITE_REGISTRATION(SwFiltersTest);

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
