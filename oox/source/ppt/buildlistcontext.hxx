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

#ifndef INCLUDED_OOX_SOURCE_PPT_BUILDLISTCONTEXT_HXX
#define INCLUDED_OOX_SOURCE_PPT_BUILDLISTCONTEXT_HXX

#include <oox/core/fragmenthandler2.hxx>

namespace oox::ppt {

    /** CT_BuildList */
    class BuildListContext
        : public ::oox::core::FragmentHandler2
    {
    public:
        explicit BuildListContext( ::oox::core::FragmentHandler2 const & rParent );

        virtual ~BuildListContext( ) override;

        virtual void onEndElement() override;

        virtual ::oox::core::ContextHandlerRef onCreateContext( sal_Int32 aElementToken, const AttributeList& rAttribs ) override;
    private:
        bool              mbInBldGraphic;
        bool              mbBuildAsOne;
    };

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
