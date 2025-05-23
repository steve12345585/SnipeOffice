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

#ifndef INCLUDED_OOX_CORE_RELATIONSHANDLER_HXX
#define INCLUDED_OOX_CORE_RELATIONSHANDLER_HXX

#include <com/sun/star/uno/Reference.hxx>
#include <oox/core/fragmenthandler.hxx>
#include <oox/core/relations.hxx>
#include <sal/types.h>

namespace oox::core {

class XmlFilterBase;

class RelationsFragment final : public FragmentHandler
{
public:
    explicit            RelationsFragment(
                            XmlFilterBase& rFilter,
                            const RelationsRef& xRelations );

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL
                        createFastChildContext(
                            sal_Int32 nElement,
                            const css::uno::Reference< css::xml::sax::XFastAttributeList >& rxAttribs ) override;

private:
    RelationsRef        mxRelations;
};


} // namespace oox::core

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
