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

#include <drawinglayer/primitive2d/modifiedcolorprimitive2d.hxx>
#include <drawinglayer/primitive2d/drawinglayer_primitivetypes2d.hxx>
#include <utility>


using namespace com::sun::star;


namespace drawinglayer::primitive2d
{
        ModifiedColorPrimitive2D::ModifiedColorPrimitive2D(
            Primitive2DContainer&& aChildren,
            basegfx::BColorModifierSharedPtr xColorModifier)
        :   GroupPrimitive2D(std::move(aChildren)),
            maColorModifier(std::move(xColorModifier))
        {
        }

        bool ModifiedColorPrimitive2D::operator==(const BasePrimitive2D& rPrimitive) const
        {
            if(GroupPrimitive2D::operator==(rPrimitive))
            {
                const ModifiedColorPrimitive2D& rCompare = static_cast<const ModifiedColorPrimitive2D&>(rPrimitive);

                if(getColorModifier().get() == rCompare.getColorModifier().get())
                {
                    return true;
                }

                if(!getColorModifier() || !rCompare.getColorModifier())
                {
                    return false;
                }

                return *getColorModifier()== *rCompare.getColorModifier();
            }

            return false;
        }

        // provide unique ID
        sal_uInt32 ModifiedColorPrimitive2D::getPrimitive2DID() const
        {
            return PRIMITIVE2D_ID_MODIFIEDCOLORPRIMITIVE2D;
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
