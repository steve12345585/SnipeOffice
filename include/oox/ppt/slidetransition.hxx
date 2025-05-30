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


#ifndef INCLUDED_OOX_PPT_SLIDETRANSITION_HXX
#define INCLUDED_OOX_PPT_SLIDETRANSITION_HXX

#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/presentation/AnimationSpeed.hpp>
#include <sal/types.h>

namespace com::sun::star {
    namespace animations { class XTransitionFilter; }
}

namespace oox { class PropertyMap; }

namespace oox::ppt {

    class SlideTransition
    {
    public:
        SlideTransition();
        explicit SlideTransition(std::u16string_view );

        void setSlideProperties( PropertyMap& props );
        void setTransitionFilterProperties( const css::uno::Reference< css::animations::XTransitionFilter > & xFilter );

        /// Set one of standard values for slide transition duration
        void setOoxTransitionSpeed( sal_Int32 nToken );
        /// Set slide transition time directly
        void setOoxTransitionSpeed( double fDuration );
        void setMode( bool bMode )
            { mbMode = bMode; }
        void setOoxAdvanceTime( sal_Int32 nAdvanceTime )
            { mnAdvanceTime = nAdvanceTime; }

    static sal_Int16 ooxToOdpDirection( ::sal_Int32 nOoxType );
    static sal_Int16 ooxToOdpEightDirections( ::sal_Int32 nOoxType );
    static sal_Int16 ooxToOdpCornerDirections( ::sal_Int32 nOoxType );
    static sal_Int16 ooxToOdpBorderDirections( ::sal_Int32 nOoxType );
    static sal_Int16 ooxToOdpSideDirections( ::sal_Int32 nOoxType );
    static bool      ooxToOdpSideDirectionsDirectionNormal( ::sal_Int32 nOoxType );

        void setOoxTransitionType( ::sal_Int32 OoxType,
                                                             ::sal_Int32 param1, ::sal_Int32 param2 );

        void setPresetTransition(std::u16string_view sPresetTransition);

    private:
        ::sal_Int16 mnTransitionType;
        ::sal_Int16 mnTransitionSubType;
        bool  mbTransitionDirectionNormal;
        css::presentation::AnimationSpeed mnAnimationSpeed;
        double mfTransitionDurationInSeconds;
        bool  mbMode; /**< https://api.libreoffice.org/docs/common/ref/com/sun/star/animations/XTransitionFilter.html Mode property */
        ::sal_Int32 mnAdvanceTime;
        ::sal_Int32 mnTransitionFadeColor;
    };

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
