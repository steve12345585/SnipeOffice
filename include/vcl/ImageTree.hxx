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

#ifndef INCLUDED_INCLUDE_VCL_IMAGETREE_HXX
#define INCLUDED_INCLUDE_VCL_IMAGETREE_HXX

#include <memory>
#include <sal/config.h>
#include <vcl/dllapi.h>
#include <rtl/ustring.hxx>
#include <o3tl/typed_flags_set.hxx>

namespace com::sun::star::uno { template <class interface_type> class Reference; }

enum class ImageLoadFlags : sal_uInt16
{
    NONE                = 0,
    IgnoreScalingFactor = 1,
    IgnoreDarkTheme     = 2,
};

namespace o3tl {
template<> struct typed_flags<ImageLoadFlags>: is_typed_flags<ImageLoadFlags, 0x3> {};
}

namespace com::sun::star::container {
    class XNameAccess;
}

namespace com::sun::star::io {
    class XInputStream;
}

class ImplImageTree;
class BitmapEx;
class SvMemoryStream;

class ImageTree
{
private:
    std::unique_ptr<ImplImageTree> mpImplImageTree;

public:
    ImageTree();

    VCL_DLLPUBLIC static ImageTree & get();

    VCL_DLLPUBLIC OUString getImageUrl(
        OUString const & name, OUString const & style, OUString const & lang);

    VCL_DLLPUBLIC std::shared_ptr<SvMemoryStream> getImageStream(
        OUString const & rName, OUString const & rStyle, OUString const & rLang);

    VCL_DLLPUBLIC css::uno::Reference<css::io::XInputStream> getImageXInputStream(OUString const & rName,
        OUString const & rStyle, OUString const & rLang);

    VCL_DLLPUBLIC bool loadImage(
        OUString const & name, OUString const & style,
        BitmapEx & bitmap, bool localized,
        const ImageLoadFlags eFlags = ImageLoadFlags::NONE);

    VCL_DLLPUBLIC bool loadImage(
        OUString const & name, OUString const & style,
        BitmapEx & bitmap, bool localized,
        sal_Int32 nScalePercentage,
        const ImageLoadFlags eFlags = ImageLoadFlags::NONE);

    VCL_DLLPUBLIC css::uno::Reference<css::container::XNameAccess> const & getNameAccess();


    /** a crude form of life cycle control (called from DeInitVCL; otherwise,
     *  if the ImplImageTree singleton were destroyed during exit that would
     *  be too late for the destructors of the bitmaps in maIconCache)*/
    VCL_DLLPUBLIC void shutdown();
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
