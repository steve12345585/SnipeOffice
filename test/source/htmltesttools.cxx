/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/htmltesttools.hxx>
#include <sal/log.hxx>

#include <memory>

htmlDocUniquePtr HtmlTestTools::parseHtml(utl::TempFileNamed const & aTempFile)
{
    SvFileStream aFileStream(aTempFile.GetURL(), StreamMode::READ);
    htmlDocUniquePtr doc = parseHtmlStream(&aFileStream);
    xmlFree(doc->name);
    doc->name = reinterpret_cast<char *>(
        xmlStrdup(
            reinterpret_cast<xmlChar const *>(
                OUStringToOString(
                    aTempFile.GetURL(), RTL_TEXTENCODING_UTF8).getStr())));
    return doc;
}

htmlDocUniquePtr HtmlTestTools::parseHtmlStream(SvStream* pStream)
{
    std::size_t nSize = pStream->remainingSize();
    std::unique_ptr<sal_uInt8[]> pBuffer(new sal_uInt8[nSize + 1]);
    pStream->ReadBytes(pBuffer.get(), nSize);
    pBuffer[nSize] = 0;
    auto pCharBuffer = reinterpret_cast<xmlChar*>(pBuffer.get());
    SAL_INFO("test", "HtmlTestTools::parseXmlStream: pBuffer is '" << pCharBuffer << "'");
    return htmlDocUniquePtr(htmlParseDoc(pCharBuffer, nullptr));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
