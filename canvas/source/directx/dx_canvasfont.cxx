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

#include <memory>

#include <o3tl/char16_t2wchar_t.hxx>

#include <com/sun/star/rendering/PanoseWeight.hpp>
#include <com/sun/star/rendering/XSpriteCanvas.hpp>
#include <cppuhelper/supportsservice.hxx>

#include "dx_canvasfont.hxx"
#include "dx_spritecanvas.hxx"
#include "dx_textlayout.hxx"
#include "dx_winstuff.hxx"

using namespace ::com::sun::star;

namespace dxcanvas
{
    namespace
    {
        INT calcFontStyle( const rendering::FontRequest& rFontRequest )
        {
            INT nFontStyle( Gdiplus::FontStyleRegular );

            if( rFontRequest.FontDescription.FontDescription.Weight > rendering::PanoseWeight::BOOK )
                nFontStyle = Gdiplus::FontStyleBold;

            return nFontStyle;
        }
    }

    CanvasFont::CanvasFont( const rendering::FontRequest&                   rFontRequest,
                            const uno::Sequence< beans::PropertyValue >&    extraFontProperties,
                            const geometry::Matrix2D&                       fontMatrix ) :
        CanvasFont_Base( m_aMutex ),
        mpGdiPlusUser( GDIPlusUser::createInstance() ),
        mpFontFamily(),
        mpFont(),
        maFontRequest( rFontRequest ),
        mnEmphasisMark(0),
        maFontMatrix( fontMatrix )
    {
        mpFontFamily = std::make_shared<Gdiplus::FontFamily>(o3tl::toW(rFontRequest.FontDescription.FamilyName.getStr()),nullptr);
        if( !mpFontFamily->IsAvailable() )
            mpFontFamily = std::make_shared<Gdiplus::FontFamily>(L"Arial",nullptr);

        mpFont = std::make_shared<Gdiplus::Font>( mpFontFamily.get(),
                                         static_cast<Gdiplus::REAL>(rFontRequest.CellSize),
                                         calcFontStyle( rFontRequest ),
                                         Gdiplus::UnitWorld );

        ::canvas::tools::extractExtraFontProperties(extraFontProperties, mnEmphasisMark);
    }

    void SAL_CALL CanvasFont::disposing()
    {
        ::osl::MutexGuard aGuard( m_aMutex );

        mpFont.reset();
        mpFontFamily.reset();
        mpGdiPlusUser.reset();
    }

    uno::Reference< rendering::XTextLayout > SAL_CALL CanvasFont::createTextLayout( const rendering::StringContext& aText,
                                                                                    sal_Int8                        nDirection,
                                                                                    sal_Int64                       nRandomSeed )
    {
        ::osl::MutexGuard aGuard( m_aMutex );

        return new TextLayout( aText, nDirection, nRandomSeed, ImplRef( this ) );
    }

    uno::Sequence< double > SAL_CALL CanvasFont::getAvailableSizes(  )
    {
        // TODO
        return uno::Sequence< double >();
    }

    uno::Sequence< beans::PropertyValue > SAL_CALL CanvasFont::getExtraFontProperties(  )
    {
        // TODO
        return uno::Sequence< beans::PropertyValue >();
    }

    rendering::FontRequest SAL_CALL CanvasFont::getFontRequest(  )
    {
        ::osl::MutexGuard aGuard( m_aMutex );

        return maFontRequest;
    }

    rendering::FontMetrics SAL_CALL CanvasFont::getFontMetrics(  )
    {
        // TODO
        return rendering::FontMetrics();
    }

    OUString SAL_CALL CanvasFont::getImplementationName()
    {
        return "DXCanvas::CanvasFont";
    }

    sal_Bool SAL_CALL CanvasFont::supportsService( const OUString& ServiceName )
    {
        return cppu::supportsService( this, ServiceName );
    }

    uno::Sequence< OUString > SAL_CALL CanvasFont::getSupportedServiceNames()
    {
        return { "com.sun.star.rendering.CanvasFont" };
    }

    double CanvasFont::getCellAscent() const
    {
        ::osl::MutexGuard aGuard( m_aMutex );

        return mpFontFamily->GetCellAscent(0); // TODO(F1): rFontRequest.styleName
    }

    double CanvasFont::getEmHeight() const
    {
        ::osl::MutexGuard aGuard( m_aMutex );

        return mpFontFamily->GetEmHeight(0); // TODO(F1): rFontRequest.styleName
    }

    FontSharedPtr CanvasFont::getFont() const
    {
        ::osl::MutexGuard aGuard( m_aMutex );

        return mpFont;
    }

    const css::geometry::Matrix2D& CanvasFont::getFontMatrix() const
    {
        ::osl::MutexGuard aGuard( m_aMutex );

        return maFontMatrix;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
