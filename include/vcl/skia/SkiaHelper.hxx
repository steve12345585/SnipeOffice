/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_VCL_SKIA_SKIAHELPER_HXX
#define INCLUDED_VCL_SKIA_SKIAHELPER_HXX

#include <vcl/dllapi.h>
#include <rtl/ustring.hxx>

#include <config_features.h>

namespace SkiaHelper
{
VCL_DLLPUBLIC bool isVCLSkiaEnabled();
VCL_DLLPUBLIC OUString readLog();
VCL_DLLPUBLIC bool isAlphaMaskBlendingEnabled();

#if HAVE_FEATURE_SKIA

// Which Skia backend to use.
enum RenderMethod
{
    RenderRaster,
    RenderVulkan,
    RenderMetal
};

VCL_DLLPUBLIC RenderMethod renderMethodToUse();

// Clean up before exit.
VCL_DLLPUBLIC void cleanup();

#endif // HAVE_FEATURE_SKIA

} // namespace

#endif // INCLUDED_VCL_SKIA_SKIAHELPER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
