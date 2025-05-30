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

#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/range/b2drange.hxx>
#include <basegfx/color/bcolor.hxx>
#include <basegfx/utils/gradienttools.hxx>
#include <vector>
#include <functional>

namespace drawinglayer::texture
{
        class GeoTexSvx
        {
        public:
            GeoTexSvx();
            virtual ~GeoTexSvx();

            // compare operator
            virtual bool operator==(const GeoTexSvx& rGeoTexSvx) const;
            bool operator!=(const GeoTexSvx& rGeoTexSvx) const { return !operator==(rGeoTexSvx); }

            // virtual base methods
            virtual void modifyBColor(const basegfx::B2DPoint& rUV, basegfx::BColor& rBColor, double& rfOpacity) const;
            virtual void modifyOpacity(const basegfx::B2DPoint& rUV, double& rfOpacity) const;
        };

        class GeoTexSvxGradient : public GeoTexSvx
        {
        protected:
            basegfx::ODFGradientInfo            maGradientInfo;
            basegfx::B2DRange                   maDefinitionRange;
            sal_uInt32                          mnRequestedSteps;
            basegfx::BColorStops                mnColorStops;
            double                              mfBorder;

            // provide a single buffer entry used for gradient texture
            // mapping, see ::modifyBColor implementations
            mutable basegfx::BColorStops::BColorStopRange     maLastColorStopRange;

        public:
            GeoTexSvxGradient(
                const basegfx::B2DRange& rDefinitionRange,
                sal_uInt32 nRequestedSteps,
                const basegfx::BColorStops& rColorStops,
                double fBorder);
            virtual ~GeoTexSvxGradient() override;

            // compare operator
            virtual bool operator==(const GeoTexSvx& rGeoTexSvx) const override;

            // virtual base methods
            virtual void appendTransformationsAndColors(
                const std::function<void(const basegfx::B2DHomMatrix& rMatrix, const basegfx::BColor& rColor)>& rCallback) = 0;
        };

        class GeoTexSvxGradientLinear final : public GeoTexSvxGradient
        {
            double                  mfUnitMinX;
            double                  mfUnitWidth;
            double                  mfUnitMaxY;

        public:
            GeoTexSvxGradientLinear(
                const basegfx::B2DRange& rDefinitionRange,
                const basegfx::B2DRange& rOutputRange,
                sal_uInt32 nRequestedSteps,
                const basegfx::BColorStops& rColorStops,
                double fBorder,
                double fAngle);
            virtual ~GeoTexSvxGradientLinear() override;

            virtual void appendTransformationsAndColors(
                const std::function<void(const basegfx::B2DHomMatrix& rMatrix, const basegfx::BColor& rColor)>& rCallback) override;
            virtual void modifyBColor(const basegfx::B2DPoint& rUV, basegfx::BColor& rBColor, double& rfOpacity) const override;
        };

        class GeoTexSvxGradientAxial final : public GeoTexSvxGradient
        {
            double                  mfUnitMinX;
            double                  mfUnitWidth;

        public:
            GeoTexSvxGradientAxial(
                const basegfx::B2DRange& rDefinitionRange,
                const basegfx::B2DRange& rOutputRange,
                sal_uInt32 nRequestedSteps,
                const basegfx::BColorStops& rColorStops,
                double fBorder,
                double fAngle);
            virtual ~GeoTexSvxGradientAxial() override;

            virtual void appendTransformationsAndColors(
                const std::function<void(const basegfx::B2DHomMatrix& rMatrix, const basegfx::BColor& rColor)>& rCallback) override;
            virtual void modifyBColor(const basegfx::B2DPoint& rUV, basegfx::BColor& rBColor, double& rfOpacity) const override;
        };

        class GeoTexSvxGradientRadial final : public GeoTexSvxGradient
        {
        public:
            GeoTexSvxGradientRadial(
                const basegfx::B2DRange& rDefinitionRange,
                sal_uInt32 nRequestedSteps,
                const basegfx::BColorStops& rColorStops,
                double fBorder,
                double fOffsetX,
                double fOffsetY);
            virtual ~GeoTexSvxGradientRadial() override;

            virtual void appendTransformationsAndColors(
                const std::function<void(const basegfx::B2DHomMatrix& rMatrix, const basegfx::BColor& rColor)>& rCallback) override;
            virtual void modifyBColor(const basegfx::B2DPoint& rUV, basegfx::BColor& rBColor, double& rfOpacity) const override;
        };

        class GeoTexSvxGradientElliptical final : public GeoTexSvxGradient
        {
        public:
            GeoTexSvxGradientElliptical(
                const basegfx::B2DRange& rDefinitionRange,
                sal_uInt32 nRequestedSteps,
                const basegfx::BColorStops& rColorStops,
                double fBorder,
                double fOffsetX,
                double fOffsetY,
                double fAngle);
            virtual ~GeoTexSvxGradientElliptical() override;

            virtual void appendTransformationsAndColors(
                const std::function<void(const basegfx::B2DHomMatrix& rMatrix, const basegfx::BColor& rColor)>& rCallback) override;
            virtual void modifyBColor(const basegfx::B2DPoint& rUV, basegfx::BColor& rBColor, double& rfOpacity) const override;
        };

        class GeoTexSvxGradientSquare final : public GeoTexSvxGradient
        {
        public:
            GeoTexSvxGradientSquare(
                const basegfx::B2DRange& rDefinitionRange,
                sal_uInt32 nRequestedSteps,
                const basegfx::BColorStops& rColorStops,
                double fBorder,
                double fOffsetX,
                double fOffsetY,
                double fAngle);
            virtual ~GeoTexSvxGradientSquare() override;

            virtual void appendTransformationsAndColors(
                const std::function<void(const basegfx::B2DHomMatrix& rMatrix, const basegfx::BColor& rColor)>& rCallback) override;
            virtual void modifyBColor(const basegfx::B2DPoint& rUV, basegfx::BColor& rBColor, double& rfOpacity) const override;
        };

        class GeoTexSvxGradientRect final : public GeoTexSvxGradient
        {
        public:
            GeoTexSvxGradientRect(
                const basegfx::B2DRange& rDefinitionRange,
                sal_uInt32 nRequestedSteps,
                const basegfx::BColorStops& rColorStops,
                double fBorder,
                double fOffsetX,
                double fOffsetY,
                double fAngle);
            virtual ~GeoTexSvxGradientRect() override;

            virtual void appendTransformationsAndColors(
                const std::function<void(const basegfx::B2DHomMatrix& rMatrix, const basegfx::BColor& rColor)>& rCallback) override;
            virtual void modifyBColor(const basegfx::B2DPoint& rUV, basegfx::BColor& rBColor, double& rfOpacity) const override;
        };

        class GeoTexSvxHatch final : public GeoTexSvx
        {
            basegfx::B2DRange                   maOutputRange;
            basegfx::B2DHomMatrix               maTextureTransform;
            basegfx::B2DHomMatrix               maBackTextureTransform;
            double                              mfDistance;
            double                              mfAngle;
            sal_uInt32                          mnSteps;

            bool                                mbDefinitionRangeEqualsOutputRange : 1;

        public:
            GeoTexSvxHatch(
                const basegfx::B2DRange& rDefinitionRange,
                const basegfx::B2DRange& rOutputRange,
                double fDistance,
                double fAngle);
            virtual ~GeoTexSvxHatch() override;

            // compare operator
            virtual bool operator==(const GeoTexSvx& rGeoTexSvx) const override;

            void appendTransformations(::std::vector< basegfx::B2DHomMatrix >& rMatrices);
            double getDistanceToHatch(const basegfx::B2DPoint& rUV) const;
            const basegfx::B2DHomMatrix& getBackTextureTransform() const;
        };

        // This class applies a tiling to the unit range. The given range
        // will be repeated inside the unit range in X and Y and for each
        // tile a matrix will be created (by appendTransformations) that
        // represents the needed transformation to map a filling in unit
        // coordinates to that tile.
        // When offsetX is given, every 2nd line will be offsetted by the
        // given percentage value (offsetX has to be 0.0 <= offsetX <= 1.0).
        // Accordingly to offsetY. If both are given, offsetX is preferred
        // and offsetY is ignored.
        class GeoTexSvxTiled final : public GeoTexSvx
        {
            basegfx::B2DRange               maRange;
            double                          mfOffsetX;
            double                          mfOffsetY;

        public:
            GeoTexSvxTiled(
                const basegfx::B2DRange& rRange,
                double fOffsetX = 0.0,
                double fOffsetY = 0.0);
            virtual ~GeoTexSvxTiled() override;

            // compare operator
            virtual bool operator==(const GeoTexSvx& rGeoTexSvx) const override;

            // Iterate over created tiles with callback provided.
            void iterateTiles(std::function<void(double fPosX, double fPosY)> aFunc) const;

            void appendTransformations(::std::vector< basegfx::B2DHomMatrix >& rMatrices) const;
            sal_uInt32 getNumberOfTiles() const;
        };
} // end of namespace drawinglayer::texture


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
