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

#include <oox/ppt/slidetimingcontext.hxx>

#include <oox/helper/attributelist.hxx>
#include <oox/ppt/timenodelistcontext.hxx>
#include "buildlistcontext.hxx"
#include <oox/token/namespaces.hxx>


using namespace ::com::sun::star;
using namespace ::oox::core;

namespace oox::ppt {

SlideTimingContext::SlideTimingContext( FragmentHandler2 const & rParent, TimeNodePtrList & aTimeNodeList ) noexcept
    : FragmentHandler2( rParent )
    , maTimeNodeList( aTimeNodeList )
{
}

SlideTimingContext::~SlideTimingContext() noexcept
{

}

::oox::core::ContextHandlerRef SlideTimingContext::onCreateContext( sal_Int32 aElementToken, const AttributeList& )
{
    switch( aElementToken )
    {
    case PPT_TOKEN( bldLst ):
        return new BuildListContext( *this );
    case PPT_TOKEN( extLst ):
        return this;
    case PPT_TOKEN( tnLst ):
        // timing nodes
    {
        return new TimeNodeListContext( *this, maTimeNodeList );
    }
    break;

    default:
        return this;
    }
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
