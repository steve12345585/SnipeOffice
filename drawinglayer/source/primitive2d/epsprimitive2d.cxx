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

#include <drawinglayer/primitive2d/epsprimitive2d.hxx>
#include <drawinglayer/primitive2d/drawinglayer_primitivetypes2d.hxx>
#include <drawinglayer/primitive2d/metafileprimitive2d.hxx>
#include <utility>

namespace drawinglayer::primitive2d
{
        Primitive2DReference EpsPrimitive2D::create2DDecomposition(const geometry::ViewInformation2D& /*rViewInformation*/) const
        {
            const GDIMetaFile& rSubstituteContent = getMetaFile();

            if( rSubstituteContent.GetActionSize() )
            {
                // the default decomposition will use the Metafile replacement visualisation.
                // To really use the Eps data, a renderer has to know and interpret this primitive
                // directly.

                return
                    new MetafilePrimitive2D(
                        getEpsTransform(),
                        rSubstituteContent);
            }
            return nullptr;
        }

        EpsPrimitive2D::EpsPrimitive2D(
            basegfx::B2DHomMatrix aEpsTransform,
            GfxLink aGfxLink,
            const GDIMetaFile& rMetaFile)
        :   maEpsTransform(std::move(aEpsTransform)),
            maGfxLink(std::move(aGfxLink)),
            maMetaFile(rMetaFile)
        {
        }

        bool EpsPrimitive2D::operator==(const BasePrimitive2D& rPrimitive) const
        {
            if(BufferedDecompositionPrimitive2D::operator==(rPrimitive))
            {
                const EpsPrimitive2D& rCompare = static_cast<const EpsPrimitive2D&>(rPrimitive);

                return (getEpsTransform() == rCompare.getEpsTransform()
                    && getGfxLink() == rCompare.getGfxLink()
                    && getMetaFile() == rCompare.getMetaFile());
            }

            return false;
        }

        basegfx::B2DRange EpsPrimitive2D::getB2DRange(const geometry::ViewInformation2D& /*rViewInformation*/) const
        {
            // use own implementation to quickly answer the getB2DRange question.
            basegfx::B2DRange aRetval(0.0, 0.0, 1.0, 1.0);
            aRetval.transform(getEpsTransform());

            return aRetval;
        }

        // provide unique ID
        sal_uInt32 EpsPrimitive2D::getPrimitive2DID() const
        {
            return PRIMITIVE2D_ID_EPSPRIMITIVE2D;
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
