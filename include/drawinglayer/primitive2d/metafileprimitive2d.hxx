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
#include <vcl/gdimtf.hxx>
#include <vcl/gdimetafiletools.hxx>


// MetafilePrimitive2D class

namespace drawinglayer::primitive2d
{
        /** MetafilePrimitive2D class

            This is the MetaFile representing primitive. It's geometry is defined
            by MetaFileTransform. The content (defined by MetaFile) will be scaled
            to the geometric definition by using PrefMapMode and PrefSize of the
            Metafile.

            It has shown that this not always guarantees that all Metafile content
            is inside the geometric definition, but this primitive defines that this
            is the case to allow a getB2DRange implementation. If it cannot be
            guaranteed that the Metafile is inside the geometric definition, it should
            be embedded to a MaskPrimitive2D.

            This primitive has no decomposition yet, so when not supported by a renderer,
            it will not be visualized.

            In the future, a decomposition implementation would be appreciated and would
            have many advantages; Metafile would no longer have to be rendered by
            sub-systems and a standard way for converting Metafiles would exist.
         */
        class DRAWINGLAYER_DLLPUBLIC MetafilePrimitive2D final : public BufferedDecompositionPrimitive2D, public MetafileAccessor
        {
        private:
            /// the geometry definition
            basegfx::B2DHomMatrix                       maMetaFileTransform;

            /// the content definition
            GDIMetaFile                                 maMetaFile;

            /// local decomposition.
            virtual Primitive2DReference create2DDecomposition(const geometry::ViewInformation2D& rViewInformation) const override;
        public:
            /// constructor
            MetafilePrimitive2D(
                basegfx::B2DHomMatrix aMetaFileTransform,
                const GDIMetaFile& rMetaFile);

            /// data read access
            const basegfx::B2DHomMatrix& getTransform() const { return maMetaFileTransform; }
            const GDIMetaFile& getMetaFile() const { return maMetaFile; }

            /// compare operator
            virtual bool operator==(const BasePrimitive2D& rPrimitive) const override;

            /// get range
            virtual basegfx::B2DRange getB2DRange(const geometry::ViewInformation2D& rViewInformation) const override;

            /// from MetafileAccessor
            virtual void accessMetafile(GDIMetaFile& rTargetMetafile) const override;

            /// provide unique ID
            virtual sal_uInt32 getPrimitive2DID() const override;
        };
} // end of namespace drawinglayer::primitive2d


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
