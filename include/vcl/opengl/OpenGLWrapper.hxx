/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// Fully wrapped methods that have no exotic GL header deps.

#ifndef INCLUDED_VCL_OPENGL_OPENGLWRAPPER_HXX
#define INCLUDED_VCL_OPENGL_OPENGLWRAPPER_HXX

#include <config_options.h>
#include <config_features.h>
#include <vcl/dllapi.h>

// All member functions static and VCL_DLLPUBLIC. Basically a glorified namespace.
struct UNLESS_MERGELIBS(VCL_DLLPUBLIC) OpenGLWrapper
{
    OpenGLWrapper() = delete; // Should not be instantiated

#if HAVE_FEATURE_UI
    /**
     * Returns the number of times OpenGL buffers have been swapped.
     */
    static sal_Int64 getBufferSwapCounter();
#endif
};

#endif // INCLUDED_VCL_OPENGL_OPENGLWRAPPER_HXX
