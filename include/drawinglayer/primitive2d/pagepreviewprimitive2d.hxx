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

#include <drawinglayer/drawinglayerdllapi.h>

#include <drawinglayer/primitive2d/BufferedDecompositionPrimitive2D.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>

namespace com::sun::star::drawing { class XDrawPage; }


namespace drawinglayer::primitive2d
{
        /** PagePreviewPrimitive2D class

            This primitive is needed to have the correct XDrawPage as ViewInformation for decomposing
            the page contents (given as PageContent here) if these contain e.g.
            view-dependent (in this case XDrawPage-dependent) text fields like PageNumber. If You want
            those primitives to be visualized correctly, Your renderer needs to locally correct its
            ViewInformation2D to reference the new XDrawPage.
         */
        class DRAWINGLAYER_DLLPUBLIC PagePreviewPrimitive2D final : public BufferedDecompositionPrimitive2D
        {
        private:
            /** the XDrawPage visualized by this primitive. When we go forward with primitives
                this will not only be used by the renderers to provide the correct decompose
                graphic attribute context, but also to completely create the page's sub-content.
             */
            const css::uno::Reference< css::drawing::XDrawPage > mxDrawPage;

            /// the PageContent
            Primitive2DContainer                           maPageContent;

            /// the own geometry
            basegfx::B2DHomMatrix                       maTransform;

            /// content width and height
            double                                      mfContentWidth;
            double                                      mfContentHeight;

            /// local decomposition. Implementation will just return children
            virtual Primitive2DReference create2DDecomposition(const geometry::ViewInformation2D& rViewInformation) const override;

        public:
            /// constructor
            PagePreviewPrimitive2D(
                css::uno::Reference< css::drawing::XDrawPage > xDrawPage,
                basegfx::B2DHomMatrix aTransform,
                double fContentWidth,
                double fContentHeight,
                Primitive2DContainer&& rPageContent);

            /// data read access
            const css::uno::Reference< css::drawing::XDrawPage >& getXDrawPage() const { return mxDrawPage; }
            const Primitive2DContainer& getPageContent() const { return maPageContent; }
            const basegfx::B2DHomMatrix& getTransform() const { return maTransform; }
            double getContentWidth() const { return mfContentWidth; }
            double getContentHeight() const { return mfContentHeight; }

            /// compare operator
            virtual bool operator==(const BasePrimitive2D& rPrimitive) const override;

            /// own getB2DRange
            virtual basegfx::B2DRange getB2DRange(const geometry::ViewInformation2D& rViewInformation) const override;

            /// provide unique ID
            virtual sal_uInt32 getPrimitive2DID() const override;
        };
} // end of namespace drawinglayer::primitive2d


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
