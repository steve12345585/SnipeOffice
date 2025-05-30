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

#include <config_options.h>
#include <basegfx/basegfxdllapi.h>
#include <basegfx/matrix/b3dhommatrix.hxx>
#include <basegfx/color/bcolor.hxx>
#include <rtl/ustring.hxx>

#include <osl/diagnose.h>

#include <memory>
#include <vector>

namespace basegfx
{
    enum class BColorModifierType : sal_uInt16 {
        BCMType_gray,
        BCMType_invert,
        BCMType_luminance_to_alpha,
        BCMType_replace,
        BCMType_interpolate,
        BCMType_saturate,
        BCMType_matrix,
        BCMType_hueRotate,
        BCMType_black_and_white,
        BCMType_gamma,
        BCMType_RGBLuminanceContrast,
        BCMType_randomize
    };

    /** base class to define color modifications

        The basic idea is to have instances of color modifiers where each
        of these can be asked to get a modified version of a color. This
        can be as easy as to return a fixed color, but may also do any
        other computation based on the given source color and the local
        algorithm to apply.

        This base implementation defines the abstract base class. Every
        derivation offers another color blending effect, when needed with
        parameters for that blending defined as members.

        As long as aw080 is not applied, an operator== is needed to implement
        the operator== of the primitive based on this instances.

        For the exact definitions of the color blending applied refer to the
        implementation of the method getModifiedColor

        BColorModifier is not copyable (no copy constructor, no assignment
        operator); local values cannot be changed after construction. The
        instances are cheap and the idea is to create them on demand. To
        be able to reuse these as much as possible, a define for a
        std::shared_ptr named BColorModifierSharedPtr exists below.
        All usages should handle instances of BColorModifier encapsulated
        into these shared pointers.
    */
    class SAL_WARN_UNUSED BASEGFX_DLLPUBLIC BColorModifier
    {
    private:
        BColorModifierType  maType;
        BColorModifier(const BColorModifier&) = delete;
        BColorModifier& operator=(const BColorModifier&) = delete;
    protected:
        // no one is allowed to incarnate the abstract base class
        // except derivations
        BColorModifier(BColorModifierType aType)
        : maType(aType)
        {
        }

    public:
        // no one should directly destroy it; all incarnations should be
        // handled in a std::shared_ptr of type BColorModifierSharedPtr
        virtual ~BColorModifier();

        // compare operator
        virtual bool operator==(const BColorModifier& rCompare) const
        {
            if (maType != rCompare.maType)
                return false;

            return true;
        }

        bool operator!=(const BColorModifier& rCompare) const
        {
            if (maType != rCompare.maType)
                return true;

            return !(operator==(rCompare));
        }

        // compute modified color
        virtual ::basegfx::BColor getModifiedColor(const ::basegfx::BColor& aSourceColor) const = 0;

        virtual OUString getModifierName() const = 0;

        // return type
        BColorModifierType getBColorModifierType() const { return maType; }
    };

    /** convert color to gray
    */
    class SAL_WARN_UNUSED BASEGFX_DLLPUBLIC BColorModifier_gray final : public BColorModifier
    {
    public:
        BColorModifier_gray()
        : BColorModifier(basegfx::BColorModifierType::BCMType_gray)
        {
        }

        virtual ~BColorModifier_gray() override;

        // compute modified color
        SAL_DLLPRIVATE virtual ::basegfx::BColor getModifiedColor(const ::basegfx::BColor& aSourceColor) const override;
        SAL_DLLPRIVATE virtual OUString getModifierName() const override;
    };

    /** invert color

        returns a color where red green and blue are inverted using 1.0 - n
    */
    class SAL_WARN_UNUSED BASEGFX_DLLPUBLIC BColorModifier_invert final : public BColorModifier
    {
    public:
        BColorModifier_invert()
        : BColorModifier(basegfx::BColorModifierType::BCMType_invert)
        {
        }

        virtual ~BColorModifier_invert() override;

        // compute modified color
        SAL_DLLPRIVATE virtual ::basegfx::BColor getModifiedColor(const ::basegfx::BColor& aSourceColor) const override;
        SAL_DLLPRIVATE virtual OUString getModifierName() const override;
    };

    /** convert to alpha based on luminance

        returns a color where red green and blue are first weighted and added
        to build a luminance value which is then inverted and used for red,
        green and blue. The weights are  r * 0.2125 + g * 0.7154 + b * 0.0721.
        This derivation is used for the svg importer and does exactly what SVG
        defines for this needed case.
    */
    class SAL_WARN_UNUSED BASEGFX_DLLPUBLIC BColorModifier_luminance_to_alpha final : public BColorModifier
    {
    public:
        BColorModifier_luminance_to_alpha()
        : BColorModifier(basegfx::BColorModifierType::BCMType_luminance_to_alpha)
        {
        }

        virtual ~BColorModifier_luminance_to_alpha() override;

        // compute modified color
        SAL_DLLPRIVATE virtual ::basegfx::BColor getModifiedColor(const ::basegfx::BColor& aSourceColor) const override;
        SAL_DLLPRIVATE virtual OUString getModifierName() const override;
    };

    /** replace color

        does not use the source color at all, but always returns the
        given color, replacing everything. Useful e.g. for unified shadow
        creation
    */
    class SAL_WARN_UNUSED BASEGFX_DLLPUBLIC BColorModifier_replace final : public BColorModifier
    {
    private:
        ::basegfx::BColor           maBColor;

    public:
        BColorModifier_replace(const ::basegfx::BColor& rBColor)
        : BColorModifier(basegfx::BColorModifierType::BCMType_replace)
        , maBColor(rBColor)
        {
        }

        virtual ~BColorModifier_replace() override;

        // data access
        const ::basegfx::BColor& getBColor() const { return maBColor; }

        // compare operator
        SAL_DLLPRIVATE virtual bool operator==(const BColorModifier& rCompare) const override;

        // compute modified color
        SAL_DLLPRIVATE virtual ::basegfx::BColor getModifiedColor(const ::basegfx::BColor& aSourceColor) const override;
        SAL_DLLPRIVATE virtual OUString getModifierName() const override;
    };

    /** interpolate color

        returns an interpolated color mixed by the given value (f) in the range
        [0.0 .. 1.0] and the given color (col) as follows:

        col * (1 - f) + aSourceColor * f
    */
    class SAL_WARN_UNUSED BASEGFX_DLLPUBLIC BColorModifier_interpolate final : public BColorModifier
    {
    private:
        ::basegfx::BColor           maBColor;
        double                      mfValue;

    public:
        BColorModifier_interpolate(const ::basegfx::BColor& rBColor, double fValue)
        : BColorModifier(basegfx::BColorModifierType::BCMType_interpolate)
        , maBColor(rBColor)
        , mfValue(fValue)
        {
        }

        virtual ~BColorModifier_interpolate() override;

        // compare operator
        SAL_DLLPRIVATE virtual bool operator==(const BColorModifier& rCompare) const override;

        // compute modified color
        SAL_DLLPRIVATE virtual ::basegfx::BColor getModifiedColor(const ::basegfx::BColor& aSourceColor) const override;
        SAL_DLLPRIVATE virtual OUString getModifierName() const override;
    };

    /** Apply saturation
        This derivation is used for the svg importer and does exactly what SVG
        defines for this needed case.

        See:
        https://www.w3.org/TR/filter-effects/#elementdef-fecolormatrix
    */
    class SAL_WARN_UNUSED BASEGFX_DLLPUBLIC BColorModifier_saturate final : public BColorModifier
    {
    private:
        basegfx::B3DHomMatrix       maSatMatrix;

    public:
        BColorModifier_saturate(double fValue);

        virtual ~BColorModifier_saturate() override;

        // compare operator
        SAL_DLLPRIVATE virtual bool operator==(const BColorModifier& rCompare) const override;

        // compute modified color
        SAL_DLLPRIVATE virtual ::basegfx::BColor getModifiedColor(const ::basegfx::BColor& aSourceColor) const override;
        SAL_DLLPRIVATE virtual OUString getModifierName() const override;
    };

    /** Apply matrix
        This derivation is used for the svg importer and does exactly what SVG
        defines for this needed case.

        See:
        https://www.w3.org/TR/filter-effects/#elementdef-fecolormatrix
    */
    class SAL_WARN_UNUSED BASEGFX_DLLPUBLIC BColorModifier_matrix final : public BColorModifier
    {
    private:
        std::vector<double>       maVector;

    public:
        BColorModifier_matrix(std::vector<double> aVector)
        : BColorModifier(basegfx::BColorModifierType::BCMType_matrix)
        , maVector(std::move(aVector))
        {
        }

        virtual ~BColorModifier_matrix() override;

        // compare operator
        SAL_DLLPRIVATE virtual bool operator==(const BColorModifier& rCompare) const override;
        // compute modified color
        SAL_DLLPRIVATE virtual ::basegfx::BColor getModifiedColor(const ::basegfx::BColor& aSourceColor) const override;
        SAL_DLLPRIVATE virtual OUString getModifierName() const override;
    };

    /** Apply hueRotate
        This derivation is used for the svg importer and does exactly what SVG
        defines for this needed case.

        See:
        https://www.w3.org/TR/filter-effects/#elementdef-fecolormatrix
    */
    class SAL_WARN_UNUSED BASEGFX_DLLPUBLIC BColorModifier_hueRotate final : public BColorModifier
    {
    private:
        basegfx::B3DHomMatrix       maHueMatrix;

    public:
        BColorModifier_hueRotate(double fRad);

        virtual ~BColorModifier_hueRotate() override;

        // compare operator
        SAL_DLLPRIVATE virtual bool operator==(const BColorModifier& rCompare) const override;

        // compute modified color
        SAL_DLLPRIVATE virtual ::basegfx::BColor getModifiedColor(const ::basegfx::BColor& aSourceColor) const override;
        SAL_DLLPRIVATE virtual OUString getModifierName() const override;
    };

    /** convert color to black and white

        returns black when the luminance of the given color is less than
        the given threshold value in the range [0.0 .. 1.0], else white
    */
    class SAL_WARN_UNUSED BASEGFX_DLLPUBLIC BColorModifier_black_and_white final : public BColorModifier
    {
    private:
        double                      mfValue;

    public:
        BColorModifier_black_and_white(double fValue)
        : BColorModifier(basegfx::BColorModifierType::BCMType_black_and_white)
        , mfValue(fValue)
        {
        }

        virtual ~BColorModifier_black_and_white() override;

        // compare operator
        SAL_DLLPRIVATE virtual bool operator==(const BColorModifier& rCompare) const override;

        // compute modified color
        SAL_DLLPRIVATE virtual ::basegfx::BColor getModifiedColor(const ::basegfx::BColor& aSourceColor) const override;
        SAL_DLLPRIVATE virtual OUString getModifierName() const override;
    };

    /** gamma correction

        Input is a gamma correction value in the range ]0.0 .. 10.0]; the
        color values get corrected using

        col(r,g,b) = clamp(pow(col(r,g,b), 1.0 / gamma), 0.0, 1.0)
    */
    class SAL_WARN_UNUSED UNLESS_MERGELIBS(BASEGFX_DLLPUBLIC) BColorModifier_gamma final : public BColorModifier
    {
    private:
        double                      mfValue;
        double                      mfInvValue;

        bool                        mbUseIt : 1;

    public:
        BColorModifier_gamma(double fValue);

        virtual ~BColorModifier_gamma() override;

        // compare operator
        SAL_DLLPRIVATE virtual bool operator==(const BColorModifier& rCompare) const override;

        // compute modified color
        virtual ::basegfx::BColor getModifiedColor(const ::basegfx::BColor& aSourceColor) const override;
        SAL_DLLPRIVATE virtual OUString getModifierName() const override;
    };

    /** Red, Green, Blue, Luminance and Contrast correction

        Input are percent values from [-1.0 .. 1-0] which correspond to -100% to 100%
        correction of Red, Green, Blue, Luminance or Contrast. 0.0 means no change of
        the corresponding channel. All these are combined (but can be used single) to
        - be able to cover a bigger change range utilizing the combination
        - allow execution by a small, common, precalculated table
    */
    class SAL_WARN_UNUSED UNLESS_MERGELIBS(BASEGFX_DLLPUBLIC) BColorModifier_RGBLuminanceContrast final : public BColorModifier
    {
    private:
        double                      mfRed;
        double                      mfGreen;
        double                      mfBlue;
        double                      mfLuminance;
        double                      mfContrast;

        double                      mfContrastOff;
        double                      mfRedOff;
        double                      mfGreenOff;
        double                      mfBlueOff;

        bool                        mbUseIt : 1;

    public:
        BColorModifier_RGBLuminanceContrast(double fRed, double fGreen, double fBlue, double fLuminance, double fContrast);

        virtual ~BColorModifier_RGBLuminanceContrast() override;

        // compare operator
        SAL_DLLPRIVATE virtual bool operator==(const BColorModifier& rCompare) const override;

        // compute modified color
        SAL_DLLPRIVATE virtual ::basegfx::BColor getModifiedColor(const ::basegfx::BColor& aSourceColor) const override;
        SAL_DLLPRIVATE virtual OUString getModifierName() const override;
    };

    /** mix a part of the original color with randomized color (mainly for debug visualizations)
    */
    class SAL_WARN_UNUSED UNLESS_MERGELIBS(BASEGFX_DLLPUBLIC) BColorModifier_randomize final : public BColorModifier
    {
    private:
        // [0.0 .. 1.0] where 0.0 is no randomize, 1.0 is all random and in-between
        // describes the mixed part. Default is 0.1 which means to mix with 10% random color
        double                      mfRandomPart;

    public:
        BColorModifier_randomize(double fRandomPart = 0.1);

        virtual ~BColorModifier_randomize() override;

        // compare operator
        SAL_DLLPRIVATE virtual bool operator==(const BColorModifier& rCompare) const override;

        // compute modified color
        SAL_DLLPRIVATE virtual ::basegfx::BColor getModifiedColor(const ::basegfx::BColor& aSourceColor) const override;
        SAL_DLLPRIVATE virtual OUString getModifierName() const override;
    };

    /// typedef to allow working with shared instances of BColorModifier
    /// for the whole mechanism
    typedef std::shared_ptr< BColorModifier > BColorModifierSharedPtr;

    /** Class to hold a stack of BColorModifierSharedPtrs and to get the modified color with
        applying all existing entry changes as defined in the stack. Instances of BColorModifier
        can be pushed and popped to change the stack.

        All references to BColorModifier members use shared pointers, thus instances of
        BColorModifierStack can be copied by the default mechanisms if needed.
    */
    class BASEGFX_DLLPUBLIC BColorModifierStack final
    {
        ::std::vector< BColorModifierSharedPtr >        maBColorModifiers;

    public:
        sal_uInt32 count() const
        {
            return maBColorModifiers.size();
        }

        const BColorModifierSharedPtr& getBColorModifier(sal_uInt32 nIndex) const
        {
            OSL_ENSURE(nIndex < count(), "BColorModifierStack: Access out of range (!)");
            return maBColorModifiers[nIndex];
        }

        // get the color in its modified form by applying all existing BColorModifiers,
        // from back to front (the newest first)
        ::basegfx::BColor getModifiedColor(const ::basegfx::BColor& rSource) const;

        void push(const BColorModifierSharedPtr& rNew)
        {
            maBColorModifiers.push_back(rNew);
        }

        void pop()
        {
            maBColorModifiers.pop_back();
        }

        bool operator==(const BColorModifierStack& rComp) const;
    };
} // end of namespace basegfx

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
