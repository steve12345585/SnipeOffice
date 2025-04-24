/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <vector>

#include "LoggedResources.hxx"
#include <rtl/ref.hxx>

class SwXTextDocument;
namespace com::sun::star
{
namespace rdf
{
class XDocumentMetadataAccess;
}
namespace text
{
class XTextDocument;
class XTextRange;
}
namespace uno
{
class XComponentContext;
}
}

namespace writerfilter::dmapper
{
/// Handler for smart tags, i.e. <w:smartTag> and below.
class SmartTagHandler : public LoggedProperties
{
    css::uno::Reference<css::uno::XComponentContext> m_xComponentContext;
    css::uno::Reference<css::rdf::XDocumentMetadataAccess> m_xDocumentMetadataAccess;
    OUString m_aURI;
    OUString m_aElement;
    std::vector<std::pair<OUString, OUString>> m_aAttributes;

public:
    SmartTagHandler(css::uno::Reference<css::uno::XComponentContext> xComponentContext,
                    const rtl::Reference<SwXTextDocument>& xTextDocument);
    ~SmartTagHandler() override;

    void lcl_attribute(Id nId, const Value& rValue) override;
    void lcl_sprm(Sprm& rSprm) override;

    void setURI(const OUString& rURI);
    void setElement(const OUString& rElement);

    /// Set m_aAttributes as RDF statements on xParagraph.
    void handle(const css::uno::Reference<css::text::XTextRange>& xParagraph);
};

} // namespace writerfilter::dmapper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
