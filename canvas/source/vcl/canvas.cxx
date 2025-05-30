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

#include <sal/config.h>

#include "canvas.hxx"

#include <com/sun/star/lang/NoSupportException.hpp>
#include <sal/log.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <vcl/outdev.hxx>

#include "outdevholder.hxx"

using namespace ::com::sun::star;

namespace vclcanvas
{
    Canvas::Canvas( const uno::Sequence< uno::Any >&                aArguments,
                    const uno::Reference< uno::XComponentContext >& /*rxContext*/ ) :
        maArguments(aArguments)
    {
    }

    void Canvas::initialize()
    {
        // #i64742# Only perform initialization when not in probe mode
        if( !maArguments.hasElements() )
            return;

        /* maArguments:
           0: ptr to creating instance (Window or VirtualDevice)
           1: current bounds of creating instance
           2: bool, denoting always on top state for Window (always false for VirtualDevice)
           3: XWindow for creating Window (or empty for VirtualDevice)
           4: SystemGraphicsData as a streamed Any
         */
        SolarMutexGuard aGuard;

        SAL_INFO("canvas.vcl", "VCLCanvas::initialize called" );

        ENSURE_ARG_OR_THROW( maArguments.getLength() >= 5 &&
                             maArguments[0].getValueTypeClass() == uno::TypeClass_HYPER,
                             "Canvas::initialize: wrong number of arguments, or wrong types" );

        sal_Int64 nPtr = 0;
        maArguments[0] >>= nPtr;

        OutputDevice* pOutDev = reinterpret_cast<OutputDevice*>(nPtr);
        if( !pOutDev )
            throw lang::NoSupportException(u"Passed OutDev invalid!"_ustr, nullptr);

        OutDevProviderSharedPtr pOutdevProvider = std::make_shared<OutDevHolder>(*pOutDev);

        // setup helper
        maDeviceHelper.init( pOutdevProvider );
        maCanvasHelper.init( *this,
                             pOutdevProvider,
                             true,   // OutDev state preservation
                             false ); // no alpha on surface

        maArguments.realloc(0);
    }

    Canvas::~Canvas()
    {
        SAL_INFO("canvas.vcl", "VCLCanvas destroyed" );
    }

    void Canvas::disposeThis()
    {
        SolarMutexGuard aGuard;

        // forward to parent
        CanvasBaseT::disposeThis();
    }

    OUString SAL_CALL Canvas::getServiceName(  )
    {
        return u"com.sun.star.rendering.Canvas.VCL"_ustr;
    }

    OUString Canvas::getImplementationName() {
        return u"com.sun.star.comp.rendering.Canvas.VCL"_ustr;
    }

    sal_Bool Canvas::supportsService(OUString const & ServiceName) {
        return cppu::supportsService(this, ServiceName);
    }

    css::uno::Sequence<OUString> Canvas::getSupportedServiceNames() {
        return {getServiceName()};
    }

    bool Canvas::repaint( const GraphicObjectSharedPtr& rGrf,
                          const rendering::ViewState&   viewState,
                          const rendering::RenderState& renderState,
                          const ::Point&                rPt,
                          const ::Size&                 rSz,
                          const GraphicAttr&            rAttr ) const
    {
        SolarMutexGuard aGuard;

        return maCanvasHelper.repaint( rGrf, viewState, renderState, rPt, rSz, rAttr );
    }
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_comp_rendering_Canvas_VCL_get_implementation(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const& args)
{
    rtl::Reference<vclcanvas::Canvas> p = new vclcanvas::Canvas(args, context);
    p->initialize();
    return cppu::acquire(p.get());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
