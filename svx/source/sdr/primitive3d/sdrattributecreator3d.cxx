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

#include <sdr/primitive3d/sdrattributecreator3d.hxx>
#include <svx/svx3ditems.hxx>
#include <svl/itemset.hxx>
#include <com/sun/star/drawing/NormalsKind.hpp>
#include <com/sun/star/drawing/TextureProjectionMode.hpp>
#include <com/sun/star/drawing/TextureKind2.hpp>
#include <com/sun/star/drawing/TextureMode.hpp>
#include <svx/xflclit.hxx>
#include <drawinglayer/attribute/materialattribute3d.hxx>
#include <drawinglayer/attribute/sdrobjectattribute3d.hxx>


namespace drawinglayer::primitive2d
{
        attribute::Sdr3DObjectAttribute createNewSdr3DObjectAttribute(const SfxItemSet& rSet)
        {
            // get NormalsKind
            css::drawing::NormalsKind aNormalsKind(css::drawing::NormalsKind_SPECIFIC);
            const sal_uInt16 nNormalsValue(rSet.Get(SDRATTR_3DOBJ_NORMALS_KIND).GetValue());

            if(1 == nNormalsValue)
            {
                aNormalsKind = css::drawing::NormalsKind_FLAT;
            }
            else if(2 == nNormalsValue)
            {
                aNormalsKind = css::drawing::NormalsKind_SPHERE;
            }

            // get NormalsInvert flag
            const bool bInvertNormals(rSet.Get(SDRATTR_3DOBJ_NORMALS_INVERT).GetValue());

            // get TextureProjectionX
            css::drawing::TextureProjectionMode aTextureProjectionX(css::drawing::TextureProjectionMode_OBJECTSPECIFIC);
            const sal_uInt16 nTextureValueX(rSet.Get(SDRATTR_3DOBJ_TEXTURE_PROJ_X).GetValue());

            if(1 == nTextureValueX)
            {
                aTextureProjectionX = css::drawing::TextureProjectionMode_PARALLEL;
            }
            else if(2 == nTextureValueX)
            {
                aTextureProjectionX = css::drawing::TextureProjectionMode_SPHERE;
            }

            // get TextureProjectionY
            css::drawing::TextureProjectionMode aTextureProjectionY(css::drawing::TextureProjectionMode_OBJECTSPECIFIC);
            const sal_uInt16 nTextureValueY(rSet.Get(SDRATTR_3DOBJ_TEXTURE_PROJ_Y).GetValue());

            if(1 == nTextureValueY)
            {
                aTextureProjectionY = css::drawing::TextureProjectionMode_PARALLEL;
            }
            else if(2 == nTextureValueY)
            {
                aTextureProjectionY = css::drawing::TextureProjectionMode_SPHERE;
            }

            // get DoubleSided flag
            const bool bDoubleSided(rSet.Get(SDRATTR_3DOBJ_DOUBLE_SIDED).GetValue());

            // get Shadow3D flag
            const bool bShadow3D(rSet.Get(SDRATTR_3DOBJ_SHADOW_3D).GetValue());

            // get TextureFilter flag
            const bool bTextureFilter(rSet.Get(SDRATTR_3DOBJ_TEXTURE_FILTER).GetValue());

            // get texture kind
            // TextureKind: 0 == Base3DTextureLuminance, 1 == Base3DTextureIntensity, 2 == Base3DTextureColor
            // see offapi/com/sun/star/drawing/TextureKind2.idl
            css::drawing::TextureKind2 aTextureKind = static_cast<css::drawing::TextureKind2>(rSet.Get(SDRATTR_3DOBJ_TEXTURE_KIND).GetValue());

            // get texture mode
            // TextureMode: 1 == Base3DTextureReplace, 2 == Base3DTextureModulate, 3 == Base3DTextureBlend
            css::drawing::TextureMode aTextureMode(css::drawing::TextureMode_REPLACE);
            const sal_uInt16 nTextureMode(rSet.Get(SDRATTR_3DOBJ_TEXTURE_MODE).GetValue());

            if(2 == nTextureMode)
            {
                aTextureMode = css::drawing::TextureMode_MODULATE;
            }
            else if(3 == nTextureMode)
            {
                aTextureMode = css::drawing::TextureMode_BLEND;
            }

            // get object color
            const ::basegfx::BColor aObjectColor(rSet.Get(XATTR_FILLCOLOR).GetColorValue().getBColor());

            // get specular color
            const ::basegfx::BColor aSpecular(rSet.Get(SDRATTR_3DOBJ_MAT_SPECULAR).GetValue().getBColor());

            // get emissive color
            const ::basegfx::BColor aEmission(rSet.Get(SDRATTR_3DOBJ_MAT_EMISSION).GetValue().getBColor());

            // get specular intensity
            sal_uInt16 nSpecularIntensity(rSet.Get(SDRATTR_3DOBJ_MAT_SPECULAR_INTENSITY).GetValue());

            if(nSpecularIntensity > 128)
            {
                nSpecularIntensity = 128;
            }

            // get reduced line geometry
            const bool bReducedLineGeometry(rSet.Get(SDRATTR_3DOBJ_REDUCED_LINE_GEOMETRY).GetValue());

            // prepare material
            attribute::MaterialAttribute3D aMaterial(aObjectColor, aSpecular, aEmission, nSpecularIntensity);

            return attribute::Sdr3DObjectAttribute(
                aNormalsKind, aTextureProjectionX, aTextureProjectionY,
                aTextureKind, aTextureMode, aMaterial,
                bInvertNormals, bDoubleSided, bShadow3D, bTextureFilter, bReducedLineGeometry);
        }
} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
