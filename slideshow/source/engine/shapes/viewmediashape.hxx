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

#ifndef INCLUDED_SLIDESHOW_SOURCE_ENGINE_SHAPES_VIEWMEDIASHAPE_HXX
#define INCLUDED_SLIDESHOW_SOURCE_ENGINE_SHAPES_VIEWMEDIASHAPE_HXX

#include <basegfx/range/b2drectangle.hxx>
#include <com/sun/star/awt/Point.hpp>
#include <com/sun/star/drawing/XShape.hpp>

#include <memory>
#include <vcl/vclptr.hxx>

#include <viewlayer.hxx>

class SystemChildWindow;

namespace com::sun::star {
    namespace drawing {
        class XShape;
    }
    namespace media {
        class XPlayer;
        class XPlayerWindow;
    }
    namespace uno {
        class XComponentContext;
    }
    namespace beans{
        class XPropertySet;
    }
}

namespace slideshow::internal
    {
        /** This class is the viewable representation of a draw
            document's media object, associated to a specific View

            The class is able to render the associated media shape on
            View implementations.
         */
        class ViewMediaShape final
        {
        public:
            /** Create a ViewMediaShape for the given View

                @param rView
                The associated View object.
             */
            ViewMediaShape( const ViewLayerSharedPtr&                                  rViewLayer,
                            css::uno::Reference< css::drawing::XShape >          xShape,
                            css::uno::Reference< css::uno::XComponentContext >   xContext,
                            const OUString&                                      aFallbackDir );

            /** destroy the object
             */
            ~ViewMediaShape();

            /// Forbid copy construction
            ViewMediaShape(const ViewMediaShape&) = delete;
            /// Forbid copy assignment
            ViewMediaShape& operator=(const ViewMediaShape&) = delete;

            /** Query the associated view layer of this shape
             */
            const ViewLayerSharedPtr& getViewLayer() const;

            // animation methods


            /** Notify the ViewShape that an animation starts now

                This method enters animation mode on the associate
                target view. The shape can be animated in parallel on
                different views.
             */
            void startMedia();

            /** Notify the ViewShape that it is no longer animated

                This methods ends animation mode on the associate
                target view
             */
            void endMedia();

            /** Notify the ViewShape that it should pause playback

                This methods pauses animation on the associate
                target view. The content stays visible (for video)
             */
            void pauseMedia();

            /** Set current time of media.

            @param fTime
            Local media time that should now be presented, in seconds.
             */
            void setMediaTime(double fTime);

            void setLooping(bool bLooping);

            // render methods


            /** Render the ViewShape

                This method renders the ViewMediaShape on the associated view.

                @param rBounds
                The current media shape bounds

                @return whether the rendering finished successfully.
            */
            bool render( const ::basegfx::B2DRectangle& rBounds ) const;

            /** Resize the ViewShape

                This method updates the ViewMediaShape size on the
                associated view. It does not render.

                @param rBounds
                The current media shape bounds

                @return whether the resize finished successfully.
            */
            bool resize( const ::basegfx::B2DRectangle& rNewBounds ) const;

        private:

            bool implInitialize( const ::basegfx::B2DRectangle& rBounds );
            void implSetMediaProperties( const css::uno::Reference< css::beans::XPropertySet >& rxProps );
            void implInitializeMediaPlayer( const OUString& rMediaURL, const OUString& rMimeType );
            void implInitializePlayerWindow( const ::basegfx::B2DRectangle& rBounds,
                                             const css::uno::Sequence< css::uno::Any >& rVCLDeviceParams );
            ViewLayerSharedPtr                    mpViewLayer;
            VclPtr< SystemChildWindow >           mpMediaWindow;
            mutable css::awt::Point               maWindowOffset;
            mutable ::basegfx::B2DRectangle       maBounds;

            css::uno::Reference< css::drawing::XShape >       mxShape;
            css::uno::Reference< css::media::XPlayer >        mxPlayer;
            css::uno::Reference< css::media::XPlayerWindow >  mxPlayerWindow;
            css::uno::Reference< css::uno::XComponentContext> mxComponentContext;
            bool                                              mbIsSoundEnabled;
            OUString                                          maFallbackDir;
        };

        typedef ::std::shared_ptr< ViewMediaShape > ViewMediaShapeSharedPtr;

}

#endif // INCLUDED_SLIDESHOW_SOURCE_ENGINE_SHAPES_VIEWMEDIASHAPE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
