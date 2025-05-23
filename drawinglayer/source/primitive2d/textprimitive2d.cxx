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

#include <drawinglayer/primitive2d/textprimitive2d.hxx>
#include <drawinglayer/primitive2d/textlayoutdevice.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>
#include <drawinglayer/primitive2d/PolyPolygonColorPrimitive2D.hxx>
#include <drawinglayer/primitive2d/drawinglayer_primitivetypes2d.hxx>
#include <drawinglayer/primitive2d/groupprimitive2d.hxx>
#include <primitive2d/texteffectprimitive2d.hxx>
#include <basegfx/matrix/b2dhommatrixtools.hxx>
#include <vcl/vcllayout.hxx>
#include <vcl/rendercontext/State.hxx>
#include <vcl/kernarray.hxx>
#include <utility>
#include <osl/diagnose.h>

using namespace com::sun::star;

namespace drawinglayer::primitive2d
{
namespace
{
// adapts fontScale for usage with TextLayouter. Input is rScale which is the extracted
// scale from a text transformation. A copy is modified so that it contains only positive
// scalings and XY-equal scalings to allow to get a non-X-scaled Vcl-Font for TextLayouter.
// rScale is adapted accordingly to contain the corrected scale which would need to be
// applied to e.g. outlines received from TextLayouter under usage of fontScale. This
// includes Y-Scale, X-Scale-correction and mirrorings.
basegfx::B2DVector getCorrectedScaleAndFontScale(basegfx::B2DVector& rScale)
{
    // copy input value
    basegfx::B2DVector aFontScale(rScale);

    // correct FontHeight settings
    if (basegfx::fTools::equalZero(aFontScale.getY()))
    {
        // no font height; choose one and adapt scale to get back to original scaling
        static const double fDefaultFontScale(100.0);
        rScale.setY(1.0 / fDefaultFontScale);
        aFontScale.setY(fDefaultFontScale);
    }
    else if (aFontScale.getY() < 0.0)
    {
        // negative font height; invert and adapt scale to get back to original scaling
        aFontScale.setY(-aFontScale.getY());
        rScale.setY(-1.0);
    }
    else
    {
        // positive font height; adapt scale; scaling will be part of the polygons
        rScale.setY(1.0);
    }

    // correct FontWidth settings
    if (basegfx::fTools::equal(aFontScale.getX(), aFontScale.getY()))
    {
        // no FontScale, adapt scale
        rScale.setX(1.0);
    }
    else
    {
        // If FontScale is used, force to no FontScale to get a non-scaled VCL font.
        // Adapt scaling in X accordingly.
        rScale.setX(aFontScale.getX() / aFontScale.getY());
        aFontScale.setX(aFontScale.getY());
    }

    return aFontScale;
}
} // end of anonymous namespace

void TextSimplePortionPrimitive2D::getTextOutlinesAndTransformation(
    basegfx::B2DPolyPolygonVector& rTarget, basegfx::B2DHomMatrix& rTransformation) const
{
    if (!getTextLength())
        return;

    // decompose object transformation to single values
    basegfx::B2DVector aScale, aTranslate;
    double fRotate, fShearX;

    // if decomposition returns false, create no geometry since e.g. scaling may
    // be zero
    if (!(getTextTransform().decompose(aScale, aTranslate, fRotate, fShearX)
          && aScale.getX() != 0.0))
        return;

    // handle special case: If scale is negative in (x,y) (3rd quadrant), it can
    // be expressed as rotation by PI
    if (aScale.getX() < 0.0 && aScale.getY() < 0.0)
    {
        aScale = basegfx::absolute(aScale);
        fRotate += M_PI;
    }

    // for the TextLayouterDevice, it is necessary to have a scaling representing
    // the font size. Since we want to extract polygons here, it is okay to
    // work just with scaling and to ignore shear, rotation and translation,
    // all that can be applied to the polygons later
    const basegfx::B2DVector aFontScale(getCorrectedScaleAndFontScale(aScale));

    // prepare textlayoutdevice
    TextLayouterDevice aTextLayouter;
    aTextLayouter.setFontAttribute(getFontAttribute(), aFontScale.getX(), aFontScale.getY(),
                                   getLocale());

    // When getting outlines from stretched text (aScale.getX() != 1.0) it
    // is necessary to inverse-scale the DXArray (if used) to not get the
    // outlines already aligned to given, but wrong DXArray
    if (!getDXArray().empty() && !basegfx::fTools::equal(aScale.getX(), 1.0))
    {
        std::vector<double> aScaledDXArray = getDXArray();
        const double fDXArrayScale(1.0 / aScale.getX());

        for (double& a : aScaledDXArray)
        {
            a *= fDXArrayScale;
        }

        // get the text outlines
        aTextLayouter.getTextOutlines(rTarget, getText(), getTextPosition(), getTextLength(),
                                      aScaledDXArray, getKashidaArray());
    }
    else
    {
        // get the text outlines
        aTextLayouter.getTextOutlines(rTarget, getText(), getTextPosition(), getTextLength(),
                                      getDXArray(), getKashidaArray());
    }

    // create primitives for the outlines
    const sal_uInt32 nCount(rTarget.size());

    if (nCount)
    {
        // prepare object transformation for polygons
        rTransformation = basegfx::utils::createScaleShearXRotateTranslateB2DHomMatrix(
            aScale, fShearX, fRotate, aTranslate);
    }
}

Primitive2DReference TextSimplePortionPrimitive2D::create2DDecomposition(
    const geometry::ViewInformation2D& /*rViewInformation*/) const
{
    if (!getTextLength())
        return nullptr;

    basegfx::B2DPolyPolygonVector aB2DPolyPolyVector;
    basegfx::B2DHomMatrix aPolygonTransform;

    // get text outlines and their object transformation
    getTextOutlinesAndTransformation(aB2DPolyPolyVector, aPolygonTransform);

    // create primitives for the outlines
    const sal_uInt32 nCount(aB2DPolyPolyVector.size());

    if (!nCount)
        return nullptr;

    // alloc space for the primitives
    Primitive2DContainer aRetval;
    aRetval.resize(nCount);

    // color-filled polypolygons
    for (sal_uInt32 a(0); a < nCount; a++)
    {
        // prepare polypolygon
        basegfx::B2DPolyPolygon& rPolyPolygon = aB2DPolyPolyVector[a];
        rPolyPolygon.transform(aPolygonTransform);
        aRetval[a] = new PolyPolygonColorPrimitive2D(rPolyPolygon, getFontColor());
    }

    if (getFontAttribute().getOutline())
    {
        // decompose polygon transformation to single values
        basegfx::B2DVector aScale, aTranslate;
        double fRotate, fShearX;
        aPolygonTransform.decompose(aScale, aTranslate, fRotate, fShearX);

        // create outline text effect with current content and replace
        return new TextEffectPrimitive2D(std::move(aRetval), aTranslate, fRotate,
                                         TextEffectStyle2D::Outline);
    }

    return new GroupPrimitive2D(std::move(aRetval));
}

TextSimplePortionPrimitive2D::TextSimplePortionPrimitive2D(
    basegfx::B2DHomMatrix rNewTransform, OUString rText, sal_Int32 nTextPosition,
    sal_Int32 nTextLength, std::vector<double>&& rDXArray, std::vector<sal_Bool>&& rKashidaArray,
    attribute::FontAttribute aFontAttribute, css::lang::Locale aLocale,
    const basegfx::BColor& rFontColor, const Color& rTextFillColor)
    : maTextTransform(std::move(rNewTransform))
    , maText(std::move(rText))
    , mnTextPosition(nTextPosition)
    , mnTextLength(nTextLength)
    , maDXArray(std::move(rDXArray))
    , maKashidaArray(std::move(rKashidaArray))
    , maFontAttribute(std::move(aFontAttribute))
    , maLocale(std::move(aLocale))
    , maFontColor(rFontColor)
    , maTextFillColor(rTextFillColor)
{
#if OSL_DEBUG_LEVEL > 0
    const sal_Int32 aStringLength(getText().getLength());
    OSL_ENSURE(aStringLength >= getTextPosition()
                   && aStringLength >= getTextPosition() + getTextLength(),
               "TextSimplePortionPrimitive2D with text out of range (!)");
#endif
}

bool LocalesAreEqual(const css::lang::Locale& rA, const css::lang::Locale& rB)
{
    return (rA.Language == rB.Language && rA.Country == rB.Country && rA.Variant == rB.Variant);
}

bool TextSimplePortionPrimitive2D::hasTextRelief() const
{
    // not possible for TextSimplePortionPrimitive2D
    return false;
}

bool TextSimplePortionPrimitive2D::hasShadow() const
{
    // not possible for TextSimplePortionPrimitive2D
    return false;
}

bool TextSimplePortionPrimitive2D::hasTextDecoration() const
{
    // not possible for TextSimplePortionPrimitive2D
    return false;
}

bool TextSimplePortionPrimitive2D::hasOutline() const
{
    // not allowed with TextRelief, else defined in FontAttributes
    return !hasTextRelief() && getFontAttribute().getOutline();
}

bool TextSimplePortionPrimitive2D::operator==(const BasePrimitive2D& rPrimitive) const
{
    if (BufferedDecompositionPrimitive2D::operator==(rPrimitive))
    {
        const TextSimplePortionPrimitive2D& rCompare
            = static_cast<const TextSimplePortionPrimitive2D&>(rPrimitive);

        return (getTextTransform() == rCompare.getTextTransform() && getText() == rCompare.getText()
                && getTextPosition() == rCompare.getTextPosition()
                && getTextLength() == rCompare.getTextLength()
                && getDXArray() == rCompare.getDXArray()
                && getKashidaArray() == rCompare.getKashidaArray()
                && getFontAttribute() == rCompare.getFontAttribute()
                && LocalesAreEqual(getLocale(), rCompare.getLocale())
                && getFontColor() == rCompare.getFontColor()
                && maTextFillColor == rCompare.maTextFillColor);
    }

    return false;
}

basegfx::B2DRange TextSimplePortionPrimitive2D::getB2DRange(
    const geometry::ViewInformation2D& /*rViewInformation*/) const
{
    if (maB2DRange.isEmpty() && getTextLength())
    {
        // get TextBoundRect as base size
        // decompose object transformation to single values
        basegfx::B2DVector aScale, aTranslate;
        double fRotate, fShearX;

        if (getTextTransform().decompose(aScale, aTranslate, fRotate, fShearX))
        {
            // for the TextLayouterDevice, it is necessary to have a scaling representing
            // the font size. Since we want to extract polygons here, it is okay to
            // work just with scaling and to ignore shear, rotation and translation,
            // all that can be applied to the polygons later
            const basegfx::B2DVector aFontScale(getCorrectedScaleAndFontScale(aScale));

            // prepare textlayoutdevice
            TextLayouterDevice aTextLayouter;
            aTextLayouter.setFontAttribute(getFontAttribute(), aFontScale.getX(), aFontScale.getY(),
                                           getLocale());

            // get basic text range
            basegfx::B2DRange aNewRange(
                aTextLayouter.getTextBoundRect(getText(), getTextPosition(), getTextLength()));

            // #i104432#, #i102556# take empty results into account
            if (!aNewRange.isEmpty())
            {
                // prepare object transformation for range
                const basegfx::B2DHomMatrix aRangeTransformation(
                    basegfx::utils::createScaleShearXRotateTranslateB2DHomMatrix(
                        aScale, fShearX, fRotate, aTranslate));

                // apply range transformation to it
                aNewRange.transform(aRangeTransformation);

                // assign to buffered value
                const_cast<TextSimplePortionPrimitive2D*>(this)->maB2DRange = aNewRange;
            }
        }
    }

    return maB2DRange;
}

void TextSimplePortionPrimitive2D::createTextLayouter(TextLayouterDevice& rTextLayouter) const
{
    // decompose primitive-local matrix to get local font scaling
    const basegfx::utils::B2DHomMatrixBufferedOnDemandDecompose aDecTrans(getTextTransform());

    // create a TextLayouter to access encapsulated VCL Text/Font related tooling
    rTextLayouter.setFontAttribute(getFontAttribute(), aDecTrans.getScale().getX(),
                                   aDecTrans.getScale().getY(), getLocale());

    if (getFontAttribute().getRTL())
    {
        vcl::text::ComplexTextLayoutFlags nRTLLayoutMode(
            rTextLayouter.getLayoutMode() & ~vcl::text::ComplexTextLayoutFlags::BiDiStrong);
        nRTLLayoutMode |= vcl::text::ComplexTextLayoutFlags::BiDiRtl
                          | vcl::text::ComplexTextLayoutFlags::TextOriginLeft;
        rTextLayouter.setLayoutMode(nRTLLayoutMode);
    }
    else
    {
        // tdf#101686: This is LTR text, but the output device may have RTL state.
        vcl::text::ComplexTextLayoutFlags nLTRLayoutMode(rTextLayouter.getLayoutMode());
        nLTRLayoutMode = nLTRLayoutMode & ~vcl::text::ComplexTextLayoutFlags::BiDiRtl;
        nLTRLayoutMode = nLTRLayoutMode & ~vcl::text::ComplexTextLayoutFlags::BiDiStrong;
        rTextLayouter.setLayoutMode(nLTRLayoutMode);
    }
}

std::unique_ptr<SalLayout>
TextSimplePortionPrimitive2D::createSalLayout(const TextLayouterDevice& rTextLayouter) const
{
    // As mentioned above we can act in the
    // Text's local coordinate system without transformation at all
    const ::std::vector<double>& rDXArray(getDXArray());

    // create SalLayout. No need for a position, as mentioned text can work
    // without transformations, so start point is always 0,0
    return rTextLayouter.getSalLayout(getText(), getTextPosition(), getTextLength(),
                                      basegfx::B2DPoint(0.0, 0.0), rDXArray, getKashidaArray());
}

// provide unique ID
sal_uInt32 TextSimplePortionPrimitive2D::getPrimitive2DID() const
{
    return PRIMITIVE2D_ID_TEXTSIMPLEPORTIONPRIMITIVE2D;
}

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
