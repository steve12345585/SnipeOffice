/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <memory>

#include <test/bootstrapfixture.hxx>
#include <unotest/macros_test.hxx>

#include <vcl/filter/pdfdocument.hxx>

class PDFDocumentTest : public test::BootstrapFixture, public unotest::MacrosTest
{
public:
    PDFDocumentTest() = default;
};

constexpr OUString DATA_DIRECTORY = u"/vcl/qa/cppunit/data/"_ustr;

CPPUNIT_TEST_FIXTURE(PDFDocumentTest, testParseBasicPDF)
{
    OUString aURL = m_directories.getURLFromSrc(DATA_DIRECTORY) + "basic.pdf";
    vcl::filter::PDFDocument aDocument;
    SvFileStream aStream(aURL, StreamMode::READ);
    CPPUNIT_ASSERT(aDocument.Read(aStream));

    std::vector<vcl::filter::PDFObjectElement*> aPages = aDocument.GetPages();
    CPPUNIT_ASSERT_EQUAL(size_t(1), aPages.size());

    vcl::filter::PDFObjectElement* pResources = aPages[0]->LookupObject("Resources"_ostr);
    CPPUNIT_ASSERT(pResources);

    vcl::filter::PDFObjectElement* pTest = pResources->LookupObject("Test"_ostr);
    CPPUNIT_ASSERT(pTest);

    vcl::filter::PDFObjectElement* pTestArray1 = pTest->LookupObject("TestArray1"_ostr);
    CPPUNIT_ASSERT(pTestArray1);
    {
        CPPUNIT_ASSERT_EQUAL(size_t(5), pTestArray1->GetArray()->GetElements().size());
    }

    vcl::filter::PDFObjectElement* pTestArray2 = pTest->LookupObject("TestArray2"_ostr);
    CPPUNIT_ASSERT(pTestArray2);
    {
        CPPUNIT_ASSERT_EQUAL(size_t(2), pTestArray2->GetArray()->GetElements().size());
    }

    vcl::filter::PDFObjectElement* pTestDictionary = pTest->LookupObject("TestDictionary"_ostr);
    {
        sal_uInt64 nOffset = pTestDictionary->GetDictionaryOffset();
        sal_uInt64 nLength = pTestDictionary->GetDictionaryLength();

        aStream.Seek(nOffset);
        std::vector<char> aBuffer(nLength + 1, 0);
        aStream.ReadBytes(aBuffer.data(), nLength);
        OString aString(aBuffer.data());

        CPPUNIT_ASSERT_EQUAL(
            "/TestReference 7 0 R/TestNumber "
            "123/TestName/SomeName/TestDictionary<</Key/Value>>/TestArray[1 2 3]"_ostr,
            aString);
    }

    CPPUNIT_ASSERT(pTestDictionary);
    {
        auto const& rItems = pTestDictionary->GetDictionaryItems();
        CPPUNIT_ASSERT_EQUAL(size_t(5), rItems.size());
        auto* pReference = dynamic_cast<vcl::filter::PDFReferenceElement*>(
            pTestDictionary->Lookup("TestReference"_ostr));
        CPPUNIT_ASSERT(pReference);
        CPPUNIT_ASSERT_EQUAL(7, pReference->GetObjectValue());

        auto* pNumber = dynamic_cast<vcl::filter::PDFNumberElement*>(
            pTestDictionary->Lookup("TestNumber"_ostr));
        CPPUNIT_ASSERT(pNumber);
        CPPUNIT_ASSERT_EQUAL(123.0, pNumber->GetValue());

        auto* pName
            = dynamic_cast<vcl::filter::PDFNameElement*>(pTestDictionary->Lookup("TestName"_ostr));
        CPPUNIT_ASSERT(pName);
        CPPUNIT_ASSERT_EQUAL("SomeName"_ostr, pName->GetValue());

        auto* pDictionary = dynamic_cast<vcl::filter::PDFDictionaryElement*>(
            pTestDictionary->Lookup("TestDictionary"_ostr));
        CPPUNIT_ASSERT(pDictionary);

        auto* pArray = dynamic_cast<vcl::filter::PDFArrayElement*>(
            pTestDictionary->Lookup("TestArray"_ostr));
        CPPUNIT_ASSERT(pArray);

        // Check offsets and lengths
        {
            sal_uInt64 nOffset
                = pTestDictionary->GetDictionary()->GetKeyOffset("TestReference"_ostr);
            sal_uInt64 nLength
                = pTestDictionary->GetDictionary()->GetKeyValueLength("TestReference"_ostr);

            aStream.Seek(nOffset);
            std::vector<char> aBuffer(nLength + 1, 0);
            aStream.ReadBytes(aBuffer.data(), nLength);
            OString aString(aBuffer.data());

            CPPUNIT_ASSERT_EQUAL("TestReference 7 0 R"_ostr, aString);
        }
        {
            sal_uInt64 nOffset = pTestDictionary->GetDictionary()->GetKeyOffset("TestNumber"_ostr);
            sal_uInt64 nLength
                = pTestDictionary->GetDictionary()->GetKeyValueLength("TestNumber"_ostr);

            aStream.Seek(nOffset);
            std::vector<char> aBuffer(nLength + 1, 0);
            aStream.ReadBytes(aBuffer.data(), nLength);
            OString aString(aBuffer.data());

            CPPUNIT_ASSERT_EQUAL("TestNumber 123"_ostr, aString);
        }
        {
            sal_uInt64 nOffset = pTestDictionary->GetDictionary()->GetKeyOffset("TestName"_ostr);
            sal_uInt64 nLength
                = pTestDictionary->GetDictionary()->GetKeyValueLength("TestName"_ostr);

            aStream.Seek(nOffset);
            std::vector<char> aBuffer(nLength + 1, 0);
            aStream.ReadBytes(aBuffer.data(), nLength);
            OString aString(aBuffer.data());

            CPPUNIT_ASSERT_EQUAL("TestName/SomeName"_ostr, aString);
        }
        {
            sal_uInt64 nOffset
                = pTestDictionary->GetDictionary()->GetKeyOffset("TestDictionary"_ostr);
            sal_uInt64 nLength
                = pTestDictionary->GetDictionary()->GetKeyValueLength("TestDictionary"_ostr);

            aStream.Seek(nOffset);
            std::vector<char> aBuffer(nLength + 1, 0);
            aStream.ReadBytes(aBuffer.data(), nLength);
            OString aString(aBuffer.data());

            CPPUNIT_ASSERT_EQUAL("TestDictionary<</Key/Value>>"_ostr, aString);
        }
        {
            sal_uInt64 nOffset = pTestDictionary->GetDictionary()->GetKeyOffset("TestArray"_ostr);
            sal_uInt64 nLength
                = pTestDictionary->GetDictionary()->GetKeyValueLength("TestArray"_ostr);

            aStream.Seek(nOffset);
            std::vector<char> aBuffer(nLength + 1, 0);
            aStream.ReadBytes(aBuffer.data(), nLength);
            OString aString(aBuffer.data());

            CPPUNIT_ASSERT_EQUAL("TestArray[1 2 3]"_ostr, aString);
        }
    }
}

CPPUNIT_TEST_FIXTURE(PDFDocumentTest, testParseDocumentWithNullAsWhitespace)
{
    // tdf#140606
    // Bug document contained a null, which cause the parser to panic,
    // but other PDF readers can handle the file well.

    OUString aURL = m_directories.getURLFromSrc(DATA_DIRECTORY) + "DocumentWithNull.pdf";
    vcl::filter::PDFDocument aDocument;
    SvFileStream aStream(aURL, StreamMode::READ);
    CPPUNIT_ASSERT(aDocument.Read(aStream));
}

namespace
{
vcl::filter::PDFObjectElement*
addObjectElement(std::vector<std::unique_ptr<vcl::filter::PDFElement>>& rElements,
                 vcl::filter::PDFDocument& rDocument, int nObjectNumber, int nGenerationNumber)
{
    auto pObject = std::make_unique<vcl::filter::PDFObjectElement>(rDocument, nObjectNumber,
                                                                   nGenerationNumber);
    auto pObjectPtr = pObject.get();
    rElements.push_back(std::move(pObject));
    return pObjectPtr;
}

vcl::filter::PDFTrailerElement*
addTrailerObjectElement(std::vector<std::unique_ptr<vcl::filter::PDFElement>>& rElements,
                        vcl::filter::PDFDocument& rDocument)
{
    auto pTrailer = std::make_unique<vcl::filter::PDFTrailerElement>(rDocument);
    auto pTrailerPtr = pTrailer.get();
    rElements.push_back(std::move(pTrailer));
    return pTrailerPtr;
}
void addEndObjectElement(std::vector<std::unique_ptr<vcl::filter::PDFElement>>& rElements)
{
    rElements.push_back(std::make_unique<vcl::filter::PDFEndObjectElement>());
}

void addDictionaryElement(std::vector<std::unique_ptr<vcl::filter::PDFElement>>& rElements)
{
    rElements.push_back(std::make_unique<vcl::filter::PDFDictionaryElement>());
}

void addEndDictionaryElement(std::vector<std::unique_ptr<vcl::filter::PDFElement>>& rElements)
{
    rElements.push_back(std::make_unique<vcl::filter::PDFEndDictionaryElement>());
}

void addNameElement(std::vector<std::unique_ptr<vcl::filter::PDFElement>>& rElements,
                    OString const& rName)
{
    auto pNameElement = std::make_unique<vcl::filter::PDFNameElement>();
    pNameElement->SetValue(rName);
    rElements.push_back(std::move(pNameElement));
}

vcl::filter::PDFNumberElement*
addNumberElement(std::vector<std::unique_ptr<vcl::filter::PDFElement>>& rElements, double fNumber)
{
    auto pNumberElement = std::make_unique<vcl::filter::PDFNumberElement>();
    auto pNumberElementPtr = pNumberElement.get();
    pNumberElement->SetValue(fNumber);
    rElements.push_back(std::move(pNumberElement));
    return pNumberElementPtr;
}

void addReferenceElement(std::vector<std::unique_ptr<vcl::filter::PDFElement>>& rElements,
                         vcl::filter::PDFDocument& rDocument,
                         vcl::filter::PDFNumberElement* pNumber1,
                         vcl::filter::PDFNumberElement* pNumber2)
{
    auto pReferenceElement
        = std::make_unique<vcl::filter::PDFReferenceElement>(rDocument, *pNumber1, *pNumber2);
    rElements.push_back(std::move(pReferenceElement));
}

void addArrayElement(std::vector<std::unique_ptr<vcl::filter::PDFElement>>& rElements,
                     vcl::filter::PDFObjectElement* pObjectPointer)
{
    auto pArray = std::make_unique<vcl::filter::PDFArrayElement>(pObjectPointer);
    rElements.push_back(std::move(pArray));
}

void addEndArrayElement(std::vector<std::unique_ptr<vcl::filter::PDFElement>>& rElements)
{
    rElements.push_back(std::make_unique<vcl::filter::PDFEndArrayElement>());
}

} // end anonymous namespace

CPPUNIT_TEST_FIXTURE(PDFDocumentTest, testParseEmptyDictionary)
{
    std::vector<std::unique_ptr<vcl::filter::PDFElement>> aElements;
    vcl::filter::PDFDocument aDocument;
    addObjectElement(aElements, aDocument, 1, 0);
    addDictionaryElement(aElements);
    addEndDictionaryElement(aElements);
    addEndObjectElement(aElements);

    auto pObject = dynamic_cast<vcl::filter::PDFObjectElement*>(aElements[0].get());
    CPPUNIT_ASSERT(pObject);

    vcl::filter::PDFObjectParser aParser(aElements);
    aParser.parse(pObject);

    CPPUNIT_ASSERT(pObject->GetDictionary());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pObject->GetDictionary()->GetItems().size());
}

CPPUNIT_TEST_FIXTURE(PDFDocumentTest, testParseDictionaryWithName)
{
    std::vector<std::unique_ptr<vcl::filter::PDFElement>> aElements;
    vcl::filter::PDFDocument aDocument;
    {
        addObjectElement(aElements, aDocument, 1, 0);
        addDictionaryElement(aElements);
        addNameElement(aElements, "Test"_ostr);
        addNumberElement(aElements, 30.0);
        addEndDictionaryElement(aElements);
        addEndObjectElement(aElements);
    }

    auto pObject = dynamic_cast<vcl::filter::PDFObjectElement*>(aElements[0].get());
    CPPUNIT_ASSERT(pObject);

    vcl::filter::PDFObjectParser aParser(aElements);
    aParser.parse(pObject);

    CPPUNIT_ASSERT(pObject->GetDictionary());
    CPPUNIT_ASSERT_EQUAL(size_t(1), pObject->GetDictionary()->GetItems().size());
    auto& rItems = pObject->GetDictionary()->GetItems();
    auto pNumberElement = dynamic_cast<vcl::filter::PDFNumberElement*>(rItems.at("Test"_ostr));
    CPPUNIT_ASSERT(pNumberElement);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(30.0, pNumberElement->GetValue(), 1e-4);
}

CPPUNIT_TEST_FIXTURE(PDFDocumentTest, testParseDictionaryNested)
{
    std::vector<std::unique_ptr<vcl::filter::PDFElement>> aElements;
    vcl::filter::PDFDocument aDocument;
    {
        addObjectElement(aElements, aDocument, 1, 0);
        addDictionaryElement(aElements);

        addNameElement(aElements, "Nested1"_ostr);
        addDictionaryElement(aElements);
        {
            addNameElement(aElements, "Nested2"_ostr);
            addDictionaryElement(aElements);
            {
                addNameElement(aElements, "SomeOtherKey"_ostr);
                addNameElement(aElements, "SomeOtherValue"_ostr);
            }
            addEndDictionaryElement(aElements);
        }
        addEndDictionaryElement(aElements);

        addNameElement(aElements, "SomeOtherKey"_ostr);
        addNameElement(aElements, "SomeOtherValue"_ostr);

        addEndObjectElement(aElements);
    }

    auto pObject = dynamic_cast<vcl::filter::PDFObjectElement*>(aElements[0].get());
    CPPUNIT_ASSERT(pObject);

    vcl::filter::PDFObjectParser aParser(aElements);
    aParser.parse(pObject);

    CPPUNIT_ASSERT(pObject->GetDictionary());
    CPPUNIT_ASSERT_EQUAL(size_t(2), pObject->GetDictionary()->GetItems().size());
    CPPUNIT_ASSERT(pObject->Lookup("Nested1"_ostr));
    CPPUNIT_ASSERT(pObject->Lookup("SomeOtherKey"_ostr));
}

CPPUNIT_TEST_FIXTURE(PDFDocumentTest, testParseEmptyArray)
{
    std::vector<std::unique_ptr<vcl::filter::PDFElement>> aElements;
    vcl::filter::PDFDocument aDocument;
    {
        auto pObjectPtr = addObjectElement(aElements, aDocument, 1, 0);
        addArrayElement(aElements, pObjectPtr);
        addEndArrayElement(aElements);
        addEndObjectElement(aElements);
    }

    auto pObject = dynamic_cast<vcl::filter::PDFObjectElement*>(aElements[0].get());
    CPPUNIT_ASSERT(pObject);

    vcl::filter::PDFObjectParser aParser(aElements);
    aParser.parse(pObject);

    CPPUNIT_ASSERT(pObject->GetArray());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pObject->GetArray()->GetElements().size());
}

CPPUNIT_TEST_FIXTURE(PDFDocumentTest, testParseArrayWithSimpleElements)
{
    std::vector<std::unique_ptr<vcl::filter::PDFElement>> aElements;
    vcl::filter::PDFDocument aDocument;

    {
        auto pObjectPtr = addObjectElement(aElements, aDocument, 1, 0);
        addArrayElement(aElements, pObjectPtr);
        addNameElement(aElements, "Test"_ostr);
        addNumberElement(aElements, 30.0);
        addEndArrayElement(aElements);
        addEndObjectElement(aElements);
    }

    auto pObject = dynamic_cast<vcl::filter::PDFObjectElement*>(aElements[0].get());
    CPPUNIT_ASSERT(pObject);

    vcl::filter::PDFObjectParser aParser(aElements);
    aParser.parse(pObject);

    CPPUNIT_ASSERT(pObject->GetArray());
    CPPUNIT_ASSERT_EQUAL(size_t(2), pObject->GetArray()->GetElements().size());
}

CPPUNIT_TEST_FIXTURE(PDFDocumentTest, testParseArrayNestedWithNumbers)
{
    std::vector<std::unique_ptr<vcl::filter::PDFElement>> aElements;
    vcl::filter::PDFDocument aDocument;

    // [ 1 [ 10 ] 2 ]
    {
        auto pObjectPtr = addObjectElement(aElements, aDocument, 1, 0);
        addArrayElement(aElements, pObjectPtr);
        {
            addNumberElement(aElements, 1.0);
            addArrayElement(aElements, pObjectPtr);
            addNumberElement(aElements, 10.0);
            addEndArrayElement(aElements);
            addNumberElement(aElements, 2.0);
        }
        addEndArrayElement(aElements);
        addEndObjectElement(aElements);
    }

    // Assert
    {
        auto pObject = dynamic_cast<vcl::filter::PDFObjectElement*>(aElements[0].get());
        CPPUNIT_ASSERT(pObject);

        vcl::filter::PDFObjectParser aParser(aElements);
        aParser.parse(pObject);

        CPPUNIT_ASSERT(pObject->GetArray());
        CPPUNIT_ASSERT_EQUAL(size_t(3), pObject->GetArray()->GetElements().size());
        auto pRootArray = pObject->GetArray();

        auto pNumber1 = dynamic_cast<vcl::filter::PDFNumberElement*>(pRootArray->GetElement(0));
        CPPUNIT_ASSERT(pNumber1);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, pNumber1->GetValue(), 1e-4);

        auto pArray3 = dynamic_cast<vcl::filter::PDFArrayElement*>(pRootArray->GetElement(1));
        CPPUNIT_ASSERT(pArray3);
        CPPUNIT_ASSERT_EQUAL(size_t(1), pArray3->GetElements().size());

        auto pNumber2 = dynamic_cast<vcl::filter::PDFNumberElement*>(pRootArray->GetElement(2));
        CPPUNIT_ASSERT(pNumber1);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, pNumber2->GetValue(), 1e-4);
    }
}

CPPUNIT_TEST_FIXTURE(PDFDocumentTest, testParseArrayNestedWithNames)
{
    std::vector<std::unique_ptr<vcl::filter::PDFElement>> aElements;
    vcl::filter::PDFDocument aDocument;

    // [/Inner1/Inner2[/Inner31][/Inner41/Inner42[/Inner431/Inner432]][/Inner51[/Inner521]]]

    {
        auto pObjectPtr = addObjectElement(aElements, aDocument, 1, 0);
        addArrayElement(aElements, pObjectPtr);
        {
            addNameElement(aElements, "Inner1"_ostr);
            addNameElement(aElements, "Inner2"_ostr);

            addArrayElement(aElements, pObjectPtr);
            {
                addNameElement(aElements, "Inner31"_ostr);
            }
            addEndArrayElement(aElements);

            addArrayElement(aElements, pObjectPtr);
            {
                addNameElement(aElements, "Inner41"_ostr);
                addNameElement(aElements, "Inner42"_ostr);
                addArrayElement(aElements, pObjectPtr);
                {
                    addNameElement(aElements, "Inner431"_ostr);
                    addNameElement(aElements, "Inner432"_ostr);
                }
                addEndArrayElement(aElements);
            }
            addEndArrayElement(aElements);

            addArrayElement(aElements, pObjectPtr);
            {
                addNameElement(aElements, "Inner51"_ostr);
                addArrayElement(aElements, pObjectPtr);
                {
                    addNameElement(aElements, "Inner521"_ostr);
                }
                addEndArrayElement(aElements);
            }
            addEndArrayElement(aElements);
        }
        addEndArrayElement(aElements);
        addEndObjectElement(aElements);
    }

    // Assert
    {
        auto pObject = dynamic_cast<vcl::filter::PDFObjectElement*>(aElements[0].get());
        CPPUNIT_ASSERT(pObject);

        vcl::filter::PDFObjectParser aParser(aElements);
        aParser.parse(pObject);

        CPPUNIT_ASSERT(pObject->GetArray());
        CPPUNIT_ASSERT_EQUAL(size_t(5), pObject->GetArray()->GetElements().size());
        auto pRootArray = pObject->GetArray();

        auto pName1 = dynamic_cast<vcl::filter::PDFNameElement*>(pRootArray->GetElement(0));
        CPPUNIT_ASSERT(pName1);
        CPPUNIT_ASSERT_EQUAL("Inner1"_ostr, pName1->GetValue());

        auto pName2 = dynamic_cast<vcl::filter::PDFNameElement*>(pRootArray->GetElement(1));
        CPPUNIT_ASSERT(pName2);
        CPPUNIT_ASSERT_EQUAL("Inner2"_ostr, pName2->GetValue());

        auto pArray3 = dynamic_cast<vcl::filter::PDFArrayElement*>(pRootArray->GetElement(2));
        CPPUNIT_ASSERT(pArray3);
        CPPUNIT_ASSERT_EQUAL(size_t(1), pArray3->GetElements().size());

        auto pInner31 = dynamic_cast<vcl::filter::PDFNameElement*>(pArray3->GetElement(0));
        CPPUNIT_ASSERT(pInner31);
        CPPUNIT_ASSERT_EQUAL("Inner31"_ostr, pInner31->GetValue());

        auto pArray4 = dynamic_cast<vcl::filter::PDFArrayElement*>(pRootArray->GetElement(3));
        CPPUNIT_ASSERT(pArray4);
        CPPUNIT_ASSERT_EQUAL(size_t(3), pArray4->GetElements().size());

        auto pInner41 = dynamic_cast<vcl::filter::PDFNameElement*>(pArray4->GetElement(0));
        CPPUNIT_ASSERT(pInner41);
        CPPUNIT_ASSERT_EQUAL("Inner41"_ostr, pInner41->GetValue());

        auto pInner42 = dynamic_cast<vcl::filter::PDFNameElement*>(pArray4->GetElement(1));
        CPPUNIT_ASSERT(pInner42);
        CPPUNIT_ASSERT_EQUAL("Inner42"_ostr, pInner42->GetValue());

        auto pArray43 = dynamic_cast<vcl::filter::PDFArrayElement*>(pArray4->GetElement(2));
        CPPUNIT_ASSERT(pArray43);
        CPPUNIT_ASSERT_EQUAL(size_t(2), pArray43->GetElements().size());

        auto pInner431 = dynamic_cast<vcl::filter::PDFNameElement*>(pArray43->GetElement(0));
        CPPUNIT_ASSERT(pInner431);
        CPPUNIT_ASSERT_EQUAL("Inner431"_ostr, pInner431->GetValue());

        auto pInner432 = dynamic_cast<vcl::filter::PDFNameElement*>(pArray43->GetElement(1));
        CPPUNIT_ASSERT(pInner432);
        CPPUNIT_ASSERT_EQUAL("Inner432"_ostr, pInner432->GetValue());

        auto pArray5 = dynamic_cast<vcl::filter::PDFArrayElement*>(pRootArray->GetElement(4));
        CPPUNIT_ASSERT(pArray5);
        CPPUNIT_ASSERT_EQUAL(size_t(2), pArray5->GetElements().size());

        auto pInner51 = dynamic_cast<vcl::filter::PDFNameElement*>(pArray5->GetElement(0));
        CPPUNIT_ASSERT(pInner51);
        CPPUNIT_ASSERT_EQUAL("Inner51"_ostr, pInner51->GetValue());

        auto pArray52 = dynamic_cast<vcl::filter::PDFArrayElement*>(pArray5->GetElement(1));
        CPPUNIT_ASSERT(pArray52);
        CPPUNIT_ASSERT_EQUAL(size_t(1), pArray52->GetElements().size());

        auto pInner521 = dynamic_cast<vcl::filter::PDFNameElement*>(pArray52->GetElement(0));
        CPPUNIT_ASSERT(pInner521);
        CPPUNIT_ASSERT_EQUAL("Inner521"_ostr, pInner521->GetValue());
    }
}

CPPUNIT_TEST_FIXTURE(PDFDocumentTest, testParseTrailer)
{
    std::vector<std::unique_ptr<vcl::filter::PDFElement>> aElements;
    vcl::filter::PDFDocument aDocument;

    {
        addTrailerObjectElement(aElements, aDocument);
        addDictionaryElement(aElements);
        addNameElement(aElements, "Size"_ostr);
        addNumberElement(aElements, 11.0);
        addEndDictionaryElement(aElements);
    }
    {
        auto pTrailer = dynamic_cast<vcl::filter::PDFTrailerElement*>(aElements[0].get());
        CPPUNIT_ASSERT(pTrailer);

        vcl::filter::PDFObjectParser aParser(aElements);
        aParser.parse(pTrailer);

        CPPUNIT_ASSERT(pTrailer->GetDictionary());
        CPPUNIT_ASSERT_EQUAL(size_t(1), pTrailer->GetDictionary()->GetItems().size());
    }
}

CPPUNIT_TEST_FIXTURE(PDFDocumentTest, testParseTrailerWithReference)
{
    std::vector<std::unique_ptr<vcl::filter::PDFElement>> aElements;
    vcl::filter::PDFDocument aDocument;

    {
        addTrailerObjectElement(aElements, aDocument);
        addDictionaryElement(aElements);
        addNameElement(aElements, "Reference"_ostr);
        auto pNumberElement1 = addNumberElement(aElements, 11.0);
        auto pNumberElement2 = addNumberElement(aElements, 0.0);
        addReferenceElement(aElements, aDocument, pNumberElement1, pNumberElement2);
        addEndDictionaryElement(aElements);
    }
    {
        auto pTrailer = dynamic_cast<vcl::filter::PDFTrailerElement*>(aElements[0].get());
        CPPUNIT_ASSERT(pTrailer);

        vcl::filter::PDFObjectParser aParser(aElements);
        aParser.parse(pTrailer);

        CPPUNIT_ASSERT(pTrailer->GetDictionary());
        CPPUNIT_ASSERT_EQUAL(size_t(1), pTrailer->GetDictionary()->GetItems().size());
        auto pElement = pTrailer->Lookup("Reference"_ostr);
        CPPUNIT_ASSERT(pElement);
        auto pReference = dynamic_cast<vcl::filter::PDFReferenceElement*>(pElement);
        CPPUNIT_ASSERT(pReference);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(11.0, pReference->GetObjectValue(), 1e-4);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, pReference->GetGenerationValue(), 1e-4);
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
