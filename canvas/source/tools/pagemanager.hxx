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

#include <basegfx/vector/b2isize.hxx>
#include <rendering/irendermodule.hxx>
#include <utility>

#include "page.hxx"

namespace canvas
{
    // PageManager
    class PageManager
    {
    public:
        explicit PageManager(std::shared_ptr<canvas::IRenderModule> xRenderModule)
            : mpRenderModule(std::move(xRenderModule))
        {
        }

        // returns the maximum size of a hardware
        // accelerated page, e.g. OpenGL texture.
        ::basegfx::B2ISize getPageSize() const;

        const std::shared_ptr<canvas::IRenderModule>& getRenderModule() const { return mpRenderModule; }

        FragmentSharedPtr allocateSpace( const ::basegfx::B2ISize& rSize );
        void              free( const FragmentSharedPtr& pFragment );

        void              nakedFragment( const FragmentSharedPtr& pFragment );

        void              validatePages();

    private:
        // the pagemanager needs access to the rendermodule
        // since we query for system resources from it.
        std::shared_ptr<canvas::IRenderModule> mpRenderModule;

        // here we collect all fragments that will be created
        // since we need them for relocation purposes.
        typedef std::vector<FragmentSharedPtr> FragmentContainer_t;
        FragmentContainer_t maFragments;

        // this is the container holding all created pages,
        // behind the scenes these are real hardware surfaces.
        std::vector<PageSharedPtr> maPages;

        bool relocate( const FragmentSharedPtr& pFragment );
    };


    // PageManagerSharedPtr


    typedef std::shared_ptr< PageManager > PageManagerSharedPtr;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
