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

#include <com/sun/star/uno/XComponentContext.hpp>
#include <awt/vclxpointer.hxx>
#include <cppuhelper/supportsservice.hxx>

VCLXPointer::VCLXPointer() : maPointer(PointerStyle::Arrow)
{
}

VCLXPointer::~VCLXPointer()
{
}

void VCLXPointer::setType( sal_Int32 nType )
{
    std::scoped_lock aGuard( maMutex );

    maPointer = static_cast<PointerStyle>(nType);
}

sal_Int32 VCLXPointer::getType()
{
    std::scoped_lock aGuard( maMutex );

    return static_cast<sal_Int32>(maPointer);
}

OUString VCLXPointer::getImplementationName()
{
    return u"stardiv.Toolkit.VCLXPointer"_ustr;
}

sal_Bool VCLXPointer::supportsService(OUString const & ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

css::uno::Sequence<OUString> VCLXPointer::getSupportedServiceNames()
{
    return css::uno::Sequence<OUString>{
        u"com.sun.star.awt.Pointer"_ustr, u"stardiv.vcl.Pointer"_ustr};
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_VCLXPointer_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new VCLXPointer());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
