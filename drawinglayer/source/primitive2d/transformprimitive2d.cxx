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

#include <drawinglayer/primitive2d/transformprimitive2d.hxx>
#include <drawinglayer/primitive2d/drawinglayer_primitivetypes2d.hxx>
#include <drawinglayer/primitive2d/Tools.hxx>
#include <utility>


using namespace com::sun::star;


namespace drawinglayer::primitive2d
{
        TransformPrimitive2D::TransformPrimitive2D(
            basegfx::B2DHomMatrix aTransformation,
            Primitive2DContainer&& aChildren)
        :   maTransformation(std::move(aTransformation)),
            mxChildren(new GroupPrimitive2D(std::move(aChildren)))
        {
        }

        TransformPrimitive2D::TransformPrimitive2D(
            basegfx::B2DHomMatrix aTransformation,
            GroupPrimitive2D& rChildren)
        :   maTransformation(std::move(aTransformation)),
            mxChildren(&rChildren)
        {
        }

        bool TransformPrimitive2D::operator==(const BasePrimitive2D& rPrimitive) const
        {
            if(BasePrimitive2D::operator==(rPrimitive))
            {
                const TransformPrimitive2D& rCompare = static_cast< const TransformPrimitive2D& >(rPrimitive);

                return maTransformation == rCompare.maTransformation
                    && arePrimitive2DReferencesEqual(mxChildren, rCompare.mxChildren);
            }

            return false;
        }

        basegfx::B2DRange TransformPrimitive2D::getB2DRange(const geometry::ViewInformation2D& rViewInformation) const
        {
            basegfx::B2DRange aRetval(getChildren().getB2DRange(rViewInformation));
            aRetval.transform(getTransformation());
            return aRetval;
        }

        // provide unique ID
        sal_uInt32 TransformPrimitive2D::getPrimitive2DID() const
        {
            return PRIMITIVE2D_ID_TRANSFORMPRIMITIVE2D;
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
