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

#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <vector>

namespace framework
{

struct ImageItemDescriptor
{
    // index of the bitmap inside the bitmaplist; not required currently, but was mandatory
    // previously, so needs to be written for backward compatibility
    long nIndex = -1;
    OUString  aCommandURL;                // URL command to dispatch
};

struct ImageItemDescriptorList
{
    // a URL to a bitmap with several images inside; not required currently, but was mandatory
    // previously, so needs to be written for backward compatibility
    OUString aURL;
    std::vector<ImageItemDescriptor> aImageItemDescriptors;
};

class ImagesConfiguration
{
    public:
        static bool LoadImages(
            const css::uno::Reference< css::uno::XComponentContext >& rxContext,
            const css::uno::Reference< css::io::XInputStream >& rInputStream,
            ImageItemDescriptorList& rItems );

        static bool StoreImages(
            const css::uno::Reference< css::uno::XComponentContext >& rxContext,
            const css::uno::Reference< css::io::XOutputStream >& rOutputStream,
            const ImageItemDescriptorList& rItems );
};

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
