/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#include <cppunit/TestFixture.h>
#include <unotest/bootstrapfixturebase.hxx>
#include <svtools/parhtml.hxx>
#include <tools/ref.hxx>
#include <tools/stream.hxx>

namespace
{
/// Subclass of HTMLParser that can sense the import result.
class TestHTMLParser : public HTMLParser
{
public:
    TestHTMLParser(SvStream& rStream);
    virtual void NextToken(HtmlTokenId nToken) override;
    /// Make this public for test purposes.
    using HTMLParser::SetNamespace;

    OUString m_aDocument;
    int m_nLineBreakCount = 0;
    OUString m_aCdata;
};

TestHTMLParser::TestHTMLParser(SvStream& rStream)
    : HTMLParser(rStream)
{
}

void TestHTMLParser::NextToken(HtmlTokenId nToken)
{
    if (nToken == HtmlTokenId::TEXTTOKEN)
        m_aDocument += aToken;
    else if (nToken == HtmlTokenId::LINEBREAK)
        ++m_nLineBreakCount;
    else if (nToken == HtmlTokenId::CDATA)
        m_aCdata = aToken;
}

/// Tests HTMLParser.
class Test : public CppUnit::TestFixture
{
};

CPPUNIT_TEST_FIXTURE(Test, testTdf114428)
{
    SvMemoryStream aStream;
    OString aDocument("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<html>hello</html>"_ostr);
    aStream.WriteBytes(aDocument.getStr(), aDocument.getLength());
    aStream.Seek(0);

    tools::SvRef<TestHTMLParser> xParser = new TestHTMLParser(aStream);
    xParser->CallParser();

    // This was '<?xml version="1.0" encoding="utf-8"?> hello', XML declaration
    // was not ignored.
    CPPUNIT_ASSERT_EQUAL(u"hello"_ustr, xParser->m_aDocument.trim());
}

CPPUNIT_TEST_FIXTURE(Test, testLineBreak)
{
    SvMemoryStream aStream;
    OString aDocument("aaa<br></br>bbb"_ostr);
    aStream.WriteBytes(aDocument.getStr(), aDocument.getLength());
    aStream.Seek(0);

    tools::SvRef<TestHTMLParser> xParser = new TestHTMLParser(aStream);
    xParser->SetNamespace(u"reqif-xhtml");
    xParser->CallParser();

    // This was 2, <br></br> was interpreted as 2 line breaks in XHTML mode.
    CPPUNIT_ASSERT_EQUAL(1, xParser->m_nLineBreakCount);
}

CPPUNIT_TEST_FIXTURE(Test, testCdata)
{
    // Given a document with CDATA:
    SvMemoryStream aStream;
    OString aDocument("A<![CDATA[B &uuml; &lt;]]>C"_ostr);
    aStream.WriteBytes(aDocument.getStr(), aDocument.getLength());
    aStream.Seek(0);

    // When parsing that HTML:
    tools::SvRef<TestHTMLParser> xParser = new TestHTMLParser(aStream);
    xParser->CallParser();

    // Then make sure that we get a cdata token with the correct content:
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: B &uuml; &lt;
    // - Actual  :
    // i.e. the content inside CDATA was lost.
    CPPUNIT_ASSERT_EQUAL(u"B &uuml; &lt;"_ustr, xParser->m_aCdata);
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
