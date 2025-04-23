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

#include <sal/config.h>

#include <accelerators/keymapping.hxx>
#include <utility>
#include <xml/acceleratorconfigurationwriter.hxx>

#include <acceleratorconst.h>

#include <com/sun/star/xml/sax/XExtendedDocumentHandler.hpp>
#include <com/sun/star/awt/KeyModifier.hpp>

#include <comphelper/attributelist.hxx>
#include <rtl/ref.hxx>

namespace framework{

AcceleratorConfigurationWriter::AcceleratorConfigurationWriter(const AcceleratorCache&                                       rContainer,
                                                               css::uno::Reference< css::xml::sax::XDocumentHandler >  xConfig   )
    : m_xConfig     (std::move(xConfig                      ))
    , m_rContainer  (rContainer                   )
{
}

AcceleratorConfigurationWriter::~AcceleratorConfigurationWriter()
{
}

void AcceleratorConfigurationWriter::flush()
{
    css::uno::Reference< css::xml::sax::XExtendedDocumentHandler > xExtendedCFG(m_xConfig, css::uno::UNO_QUERY_THROW);

    // prepare attribute list
    rtl::Reference<::comphelper::AttributeList> pAttribs = new ::comphelper::AttributeList;

    pAttribs->AddAttribute(
        u"xmlns:accel"_ustr,
        u"http://openoffice.org/2001/accel"_ustr);
    pAttribs->AddAttribute(
        u"xmlns:xlink"_ustr, u"http://www.w3.org/1999/xlink"_ustr);

    // generate xml
    xExtendedCFG->startDocument();

    xExtendedCFG->unknown(
        u"<!DOCTYPE accel:acceleratorlist PUBLIC \"-//OpenOffice.org//DTD"
        " OfficeDocument 1.0//EN\" \"accelerator.dtd\">"_ustr);
    xExtendedCFG->ignorableWhitespace(OUString());

    xExtendedCFG->startElement(AL_ELEMENT_ACCELERATORLIST, pAttribs);
    xExtendedCFG->ignorableWhitespace(OUString());

    // TODO think about threadsafe using of cache
    AcceleratorCache::TKeyList                 lKeys = m_rContainer.getAllKeys();
    for (auto const& lKey : lKeys)
    {
        const OUString aCommand = m_rContainer.getCommandByKey(lKey);
        impl_ts_writeKeyCommandPair(lKey, aCommand, xExtendedCFG);
    }

    /* TODO write key-command list
    for (auto const& writeAccelerator : m_aWriteAcceleratorList)
        WriteAcceleratorItem(writeAccelerator);
    */

    xExtendedCFG->ignorableWhitespace(OUString());
    xExtendedCFG->endElement(AL_ELEMENT_ACCELERATORLIST);
    xExtendedCFG->ignorableWhitespace(OUString());
    xExtendedCFG->endDocument();
}

// static
void AcceleratorConfigurationWriter::impl_ts_writeKeyCommandPair(const css::awt::KeyEvent&                                     aKey    ,
                                                                 const OUString&                                        sCommand,
                                                                 const css::uno::Reference< css::xml::sax::XDocumentHandler >& xConfig )
{
    rtl::Reference<::comphelper::AttributeList> pAttribs = new ::comphelper::AttributeList;

    OUString sKey = KeyMapping::get().mapCodeToIdentifier(aKey.KeyCode);
    // TODO check if key is empty!

    pAttribs->AddAttribute(u"accel:code"_ustr, sKey    );
    pAttribs->AddAttribute(u"xlink:href"_ustr, sCommand);

    if ((aKey.Modifiers & css::awt::KeyModifier::SHIFT) == css::awt::KeyModifier::SHIFT)
        pAttribs->AddAttribute(u"accel:shift"_ustr, u"true"_ustr);

    if ((aKey.Modifiers & css::awt::KeyModifier::MOD1) == css::awt::KeyModifier::MOD1)
        pAttribs->AddAttribute(u"accel:mod1"_ustr, u"true"_ustr);

    if ((aKey.Modifiers & css::awt::KeyModifier::MOD2) == css::awt::KeyModifier::MOD2)
        pAttribs->AddAttribute(u"accel:mod2"_ustr, u"true"_ustr);

    if ((aKey.Modifiers & css::awt::KeyModifier::MOD3) == css::awt::KeyModifier::MOD3)
        pAttribs->AddAttribute(u"accel:mod3"_ustr, u"true"_ustr);

    xConfig->ignorableWhitespace(OUString());
    xConfig->startElement(AL_ELEMENT_ITEM, pAttribs);
    xConfig->ignorableWhitespace(OUString());
    xConfig->endElement(AL_ELEMENT_ITEM);
    xConfig->ignorableWhitespace(OUString());
}

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
