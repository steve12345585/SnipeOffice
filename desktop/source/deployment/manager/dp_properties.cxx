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


#include <com/sun/star/ucb/XCommandEnvironment.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <xmlscript/xml_helper.hxx>
#include <ucbhelper/content.hxx>

#include <dp_ucb.h>
#include "dp_properties.hxx"

namespace lang  = css::lang;
namespace ucb = css::ucb;
namespace uno = css::uno;


using css::uno::Reference;

constexpr OUString PROP_SUPPRESS_LICENSE = u"SUPPRESS_LICENSE"_ustr;
constexpr OUStringLiteral PROP_EXTENSION_UPDATE = u"EXTENSION_UPDATE";

namespace dp_manager {

//Reading the file
ExtensionProperties::ExtensionProperties(
    std::u16string_view urlExtension,
    Reference<ucb::XCommandEnvironment> const & xCmdEnv,
    Reference<uno::XComponentContext> const & xContext) :
    m_xCmdEnv(xCmdEnv), m_xContext(xContext)
{
    m_propFileUrl = OUString::Concat(urlExtension) + "properties";

    std::vector< std::pair< OUString, OUString> > props;
    if (! dp_misc::create_ucb_content(nullptr, m_propFileUrl, nullptr, false))
        return;

    ::ucbhelper::Content contentProps(m_propFileUrl, m_xCmdEnv, m_xContext);
    dp_misc::readProperties(props, contentProps);

    for (auto const& prop : props)
    {
        if (prop.first == PROP_SUPPRESS_LICENSE)
            m_prop_suppress_license = prop.second;
    }
}

//Writing the file
ExtensionProperties::ExtensionProperties(
    std::u16string_view urlExtension,
    uno::Sequence<css::beans::NamedValue> const & properties,
    Reference<ucb::XCommandEnvironment> const & xCmdEnv,
    Reference<uno::XComponentContext> const & xContext) :
    m_xCmdEnv(xCmdEnv), m_xContext(xContext)
{
    m_propFileUrl = OUString::Concat(urlExtension) + "properties";

    for (css::beans::NamedValue const & v : properties)
    {
        if (v.Name == PROP_SUPPRESS_LICENSE)
        {
            m_prop_suppress_license = getPropertyValue(v);
        }
        else if (v.Name == PROP_EXTENSION_UPDATE)
        {
            m_prop_extension_update = getPropertyValue(v);
        }
        else
        {
            throw lang::IllegalArgumentException(
                u"Extension Manager: unknown property"_ustr, nullptr, -1);
        }
    }
}

OUString ExtensionProperties::getPropertyValue(css::beans::NamedValue const & v)
{
    OUString value(u"0"_ustr);
    if (! (v.Value >>= value) )
    {
        throw lang::IllegalArgumentException(
            u"Extension Manager: wrong property value"_ustr, nullptr, -1);
    }
    return value;
}
void ExtensionProperties::write()
{
    ::ucbhelper::Content contentProps(m_propFileUrl, m_xCmdEnv, m_xContext);
    OUString buf;

    if (m_prop_suppress_license)
    {
        buf = OUString::Concat(PROP_SUPPRESS_LICENSE) + "=" + *m_prop_suppress_license;
    }

    OString stamp = OUStringToOString(buf, RTL_TEXTENCODING_UTF8);
    Reference<css::io::XInputStream> xData(
        ::xmlscript::createInputStream(
                reinterpret_cast<sal_Int8 const *>(stamp.getStr()),
                stamp.getLength() ) );
    contentProps.writeStream( xData, true /* replace existing */ );
}

bool ExtensionProperties::isSuppressedLicense() const
{
    bool ret = false;
    if (m_prop_suppress_license)
    {
        if (*m_prop_suppress_license == "1")
            ret = true;
    }
    return ret;
}

bool ExtensionProperties::isExtensionUpdate() const
{
    bool ret = false;
    if (m_prop_extension_update)
    {
        if (*m_prop_extension_update == "1")
            ret = true;
    }
    return ret;
}

} // namespace dp_manager


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
