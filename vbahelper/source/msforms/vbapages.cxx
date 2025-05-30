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
#include "vbapages.hxx"

using namespace ::ooo::vba;
using namespace ::com::sun::star;

ScVbaPages::ScVbaPages( const uno::Reference< XHelperInterface >& xParent, const uno::Reference< uno::XComponentContext >& xContext, const uno::Reference< container::XIndexAccess >& xPages ) : ScVbaPages_BASE( xParent, xContext, xPages )
{
}

uno::Type SAL_CALL
ScVbaPages::getElementType()
{
    // return cppu::UnoType<msforms::XPage>::get();
    return cppu::UnoType<uno::XInterface>::get();
}

uno::Any
ScVbaPages::createCollectionObject( const css::uno::Any& aSource )
{
    return aSource;
}

OUString
ScVbaPages::getServiceImplName()
{
    return u"ScVbaPages"_ustr;
}

uno::Reference< container::XEnumeration > SAL_CALL
ScVbaPages::createEnumeration()
{
    // #STUB
    return uno::Reference< container::XEnumeration >();
}

uno::Sequence< OUString >
ScVbaPages::getServiceNames()
{
    static uno::Sequence< OUString > const aServiceNames
    {
        u"ooo.vba.msform.Pages"_ustr
    };
    return aServiceNames;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
