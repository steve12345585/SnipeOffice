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

#pragma once

#include <oox/core/contexthandler.hxx>
#include <oox/core/fragmenthandler2.hxx>
#include <oox/dllapi.h>
#include <oox/drawingml/drawingmltypes.hxx>
#include <sal/types.h>

namespace oox::drawingml {

class OOX_DLLPUBLIC ShapeGroupContext : public ::oox::core::FragmentHandler2
{
public:
    ShapeGroupContext( ::oox::core::FragmentHandler2 const & rParent, ShapePtr const & pMasterShapePtr, ShapePtr pGroupShapePtr );
    virtual ~ShapeGroupContext() override;
    virtual ::oox::core::ContextHandlerRef onCreateContext( ::sal_Int32 Element, const ::oox::AttributeList& rAttribs ) override;

protected:
    ShapePtr            mpGroupShapePtr;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
