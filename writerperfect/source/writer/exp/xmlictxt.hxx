/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/xml/sax/XDocumentHandler.hpp>

#include <cppuhelper/implbase.hxx>
#include <rtl/ref.hxx>

namespace writerperfect::exp
{
class XMLImport;

/// Base class for a handler of a single XML element during ODF -> librevenge conversion.
class XMLImportContext : public cppu::WeakImplHelper<css::xml::sax::XDocumentHandler>
{
public:
    XMLImportContext(XMLImport& rImport);
    XMLImport& GetImport() { return mrImport; }

    virtual rtl::Reference<XMLImportContext>
    CreateChildContext(const OUString& rName,
                       const css::uno::Reference<css::xml::sax::XAttributeList>& xAttribs);

    // XDocumentHandler
    void SAL_CALL startDocument() override;
    void SAL_CALL endDocument() override;
    void SAL_CALL
    startElement(const OUString& rName,
                 const css::uno::Reference<css::xml::sax::XAttributeList>& xAttribs) override;
    void SAL_CALL endElement(const OUString& rName) override;
    void SAL_CALL characters(const OUString& rChars) override;
    void SAL_CALL ignorableWhitespace(const OUString& rWhitespaces) override;
    void SAL_CALL processingInstruction(const OUString& rTarget, const OUString& rData) override;
    void SAL_CALL
    setDocumentLocator(const css::uno::Reference<css::xml::sax::XLocator>& xLocator) override;

private:
    XMLImport& mrImport;
};

} // namespace writerperfect::exp

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
