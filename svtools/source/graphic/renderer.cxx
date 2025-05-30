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


#include <com/sun/star/awt/Rectangle.hpp>
#include <com/sun/star/awt/XDevice.hpp>
#include <com/sun/star/graphic/XGraphic.hpp>
#include <com/sun/star/graphic/XGraphicRenderer.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <tools/gen.hxx>
#include <vcl/svapp.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <comphelper/propertysethelper.hxx>
#include <comphelper/propertysetinfo.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <rtl/ref.hxx>
#include <vcl/GraphicObject.hxx>
#include <vcl/outdev.hxx>

#define UNOGRAPHIC_DEVICE           1
#define UNOGRAPHIC_DESTINATIONRECT  2
#define UNOGRAPHIC_RENDERDATA       3

using namespace ::com::sun::star;

namespace {

class GraphicRendererVCL : public ::cppu::OWeakObject,
                           public css::lang::XServiceInfo,
                           public css::lang::XTypeProvider,
                           public ::comphelper::PropertySetHelper,
                           public css::graphic::XGraphicRenderer
{
    static rtl::Reference<::comphelper::PropertySetInfo> createPropertySetInfo();

public:

    GraphicRendererVCL();

    // XInterface
    virtual css::uno::Any SAL_CALL queryInterface( const css::uno::Type & rType ) override;
    virtual void SAL_CALL acquire() noexcept override;
    virtual void SAL_CALL release() noexcept override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XTypeProvider
    virtual css::uno::Sequence< css::uno::Type > SAL_CALL getTypes(  ) override;
    virtual css::uno::Sequence< sal_Int8 > SAL_CALL getImplementationId(  ) override;

    // PropertySetHelper
    virtual void _setPropertyValues( const comphelper::PropertyMapEntry** ppEntries, const css::uno::Any* pValues ) override;
    virtual void _getPropertyValues( const comphelper::PropertyMapEntry** ppEntries, css::uno::Any* pValue ) override;

    // XGraphicRenderer
    virtual void SAL_CALL render( const css::uno::Reference< css::graphic::XGraphic >& Graphic ) override;

private:

    css::uno::Reference< css::awt::XDevice > mxDevice;

    VclPtr<OutputDevice>        mpOutDev;
    tools::Rectangle                   maDestRect;
    css::uno::Any               maRenderData;
};

GraphicRendererVCL::GraphicRendererVCL() :
    ::comphelper::PropertySetHelper( createPropertySetInfo() ),
    mpOutDev( nullptr )
{
}

uno::Any SAL_CALL GraphicRendererVCL::queryInterface( const uno::Type & rType )
{
    uno::Any aAny;

    if( rType == cppu::UnoType<lang::XServiceInfo>::get())
        aAny <<= uno::Reference< lang::XServiceInfo >(this);
    else if( rType == cppu::UnoType<lang::XTypeProvider>::get())
        aAny <<= uno::Reference< lang::XTypeProvider >(this);
    else if( rType == cppu::UnoType<beans::XPropertySet>::get())
        aAny <<= uno::Reference< beans::XPropertySet >(this);
    else if( rType == cppu::UnoType<beans::XPropertyState>::get())
        aAny <<= uno::Reference< beans::XPropertyState >(this);
    else if( rType == cppu::UnoType<beans::XMultiPropertySet>::get())
        aAny <<= uno::Reference< beans::XMultiPropertySet >(this);
    else if( rType == cppu::UnoType<graphic::XGraphicRenderer>::get())
        aAny <<= uno::Reference< graphic::XGraphicRenderer >(this);
    else
        aAny = OWeakObject::queryInterface( rType );

    return aAny;
}


void SAL_CALL GraphicRendererVCL::acquire()
    noexcept
{
    OWeakObject::acquire();
}


void SAL_CALL GraphicRendererVCL::release()
    noexcept
{
    OWeakObject::release();
}


OUString SAL_CALL GraphicRendererVCL::getImplementationName()
{
    return u"com.sun.star.comp.graphic.GraphicRendererVCL"_ustr;
}

sal_Bool SAL_CALL GraphicRendererVCL::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}


uno::Sequence< OUString > SAL_CALL GraphicRendererVCL::getSupportedServiceNames()
{
    return { u"com.sun.star.graphic.GraphicRendererVCL"_ustr };
}


uno::Sequence< uno::Type > SAL_CALL GraphicRendererVCL::getTypes()
{
    static const uno::Sequence< uno::Type >  aTypes {
        cppu::UnoType<lang::XServiceInfo>::get(),
        cppu::UnoType<lang::XTypeProvider>::get(),
        cppu::UnoType<beans::XPropertySet>::get(),
        cppu::UnoType<beans::XPropertyState>::get(),
        cppu::UnoType<beans::XMultiPropertySet>::get(),
        cppu::UnoType<graphic::XGraphicRenderer>::get() };
    return aTypes;
}

uno::Sequence< sal_Int8 > SAL_CALL GraphicRendererVCL::getImplementationId()
{
    return css::uno::Sequence<sal_Int8>();
}


rtl::Reference<::comphelper::PropertySetInfo> GraphicRendererVCL::createPropertySetInfo()
{
    static ::comphelper::PropertyMapEntry const aEntries[] =
    {
        { u"Device"_ustr, UNOGRAPHIC_DEVICE, cppu::UnoType<uno::Any>::get(), 0, 0 },
        { u"DestinationRect"_ustr, UNOGRAPHIC_DESTINATIONRECT, cppu::UnoType<awt::Rectangle>::get(), 0, 0 },
        { u"RenderData"_ustr, UNOGRAPHIC_RENDERDATA, cppu::UnoType<uno::Any>::get(), 0, 0 },
    };

    return rtl::Reference<::comphelper::PropertySetInfo>( new ::comphelper::PropertySetInfo(aEntries) );
}


void GraphicRendererVCL::_setPropertyValues( const comphelper::PropertyMapEntry** ppEntries, const uno::Any* pValues )
{
    SolarMutexGuard aGuard;

    while( *ppEntries )
    {
        switch( (*ppEntries)->mnHandle )
        {
            case UNOGRAPHIC_DEVICE:
            {
                uno::Reference< awt::XDevice > xDevice;

                if( ( *pValues >>= xDevice ) && xDevice.is() )
                {
                    mxDevice = xDevice;
                    mpOutDev = VCLUnoHelper::GetOutputDevice( xDevice );
                }
                else
                {
                    mxDevice.clear();
                    mpOutDev = nullptr;
                }
            }
            break;

            case UNOGRAPHIC_DESTINATIONRECT:
            {
                awt::Rectangle aAWTRect;

                if( *pValues >>= aAWTRect )
                {
                    maDestRect = tools::Rectangle( Point( aAWTRect.X, aAWTRect.Y ),
                                            Size( aAWTRect.Width, aAWTRect.Height ) );
                }
            }
            break;

            case UNOGRAPHIC_RENDERDATA:
            {
                 maRenderData = *pValues;
            }
            break;
        }

        ++ppEntries;
        ++pValues;
    }
}


void GraphicRendererVCL::_getPropertyValues( const comphelper::PropertyMapEntry** ppEntries, uno::Any* pValues )
{
    SolarMutexGuard aGuard;

    while( *ppEntries )
    {
        switch( (*ppEntries)->mnHandle )
        {
            case UNOGRAPHIC_DEVICE:
            {
                if( mxDevice.is() )
                    *pValues <<= mxDevice;
            }
            break;

            case UNOGRAPHIC_DESTINATIONRECT:
            {
                const awt::Rectangle aAWTRect( maDestRect.Left(), maDestRect.Top(),
                                               maDestRect.GetWidth(), maDestRect.GetHeight() );

                *pValues <<= aAWTRect;
            }
            break;

            case UNOGRAPHIC_RENDERDATA:
            {
                *pValues = maRenderData;
            }
            break;
        }

        ++ppEntries;
        ++pValues;
    }
}

void SAL_CALL GraphicRendererVCL::render( const uno::Reference< graphic::XGraphic >& rxGraphic )
{
    if( mpOutDev && mxDevice.is() && rxGraphic.is() )
    {
        Graphic aGraphic(rxGraphic);
        if (!aGraphic.IsNone())
        {
            GraphicObject aGraphicObject(std::move(aGraphic));
            aGraphicObject.Draw(*mpOutDev, maDestRect.TopLeft(), maDestRect.GetSize());
        }
    }
}

}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
com_sun_star_comp_graphic_GraphicRendererVCL_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new GraphicRendererVCL);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
