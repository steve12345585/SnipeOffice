/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "DomainMapper.hxx"
#include "TagLogger.hxx"
#include <unotools/mediadescriptor.hxx>

namespace writerfilter::dmapper
{
Stream::Pointer_t
DomainMapperFactory::createMapper(css::uno::Reference<css::uno::XComponentContext> const& xContext,
                                  css::uno::Reference<css::io::XInputStream> const& xInputStream,
                                  rtl::Reference<SwXTextDocument> const& xModel,
                                  bool bRepairStorage, SourceDocumentType eDocumentType,
                                  utl::MediaDescriptor const& rMediaDesc)
{
#ifdef DBG_UTIL
    OUString sURL
        = rMediaDesc.getUnpackedValueOrDefault(utl::MediaDescriptor::PROP_URL, OUString());
    ::std::string sURLc(OUStringToOString(sURL, RTL_TEXTENCODING_ASCII_US));

    if (getenv("SW_DEBUG_WRITERFILTER"))
        TagLogger::getInstance().setFileName(sURLc);
    TagLogger::getInstance().startDocument();
#endif

    return { new DomainMapper(xContext, xInputStream, xModel, bRepairStorage, eDocumentType,
                              rMediaDesc) };
}

} // namespace writerfilter::dmapper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
