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

#include <drawinglayer/attribute/strokeattribute.hxx>
#include <numeric>


namespace drawinglayer::attribute
{
        class ImpStrokeAttribute
        {
        public:
            // data definitions
            std::vector< double >                     maDotDashArray;         // array of double which defines the dot-dash pattern
            double                                      mfFullDotDashLen;       // sum of maDotDashArray (for convenience)

            ImpStrokeAttribute(
                std::vector< double >&& rDotDashArray,
                double fFullDotDashLen)
            :   maDotDashArray(std::move(rDotDashArray)),
                mfFullDotDashLen(fFullDotDashLen)
            {
            }

            ImpStrokeAttribute()
            :   mfFullDotDashLen(0.0)
            {
            }

            // data read access
            const std::vector< double >& getDotDashArray() const { return maDotDashArray; }
            double getFullDotDashLen() const
            {
                if(0.0 == mfFullDotDashLen && !maDotDashArray.empty())
                {
                    // calculate length on demand
                    const double fAccumulated(std::accumulate(maDotDashArray.begin(), maDotDashArray.end(), 0.0));
                    const_cast< ImpStrokeAttribute* >(this)->mfFullDotDashLen = fAccumulated;
                }

                return mfFullDotDashLen;
            }

            bool operator==(const ImpStrokeAttribute& rCandidate) const
            {
                return (getDotDashArray() == rCandidate.getDotDashArray()
                    && getFullDotDashLen() == rCandidate.getFullDotDashLen());
            }
        };

        namespace
        {
            StrokeAttribute::ImplType& theGlobalDefault()
            {
                static StrokeAttribute::ImplType SINGLETON;
                return SINGLETON;
            }
        }

        StrokeAttribute::StrokeAttribute(
            std::vector< double >&& rDotDashArray,
            double fFullDotDashLen)
        :   mpStrokeAttribute(ImpStrokeAttribute(
                std::move(rDotDashArray), fFullDotDashLen))
        {
        }

        StrokeAttribute::StrokeAttribute()
        :   mpStrokeAttribute(theGlobalDefault())
        {
        }

        StrokeAttribute::StrokeAttribute(const StrokeAttribute&) = default;

        StrokeAttribute::StrokeAttribute(StrokeAttribute&&) = default;

        StrokeAttribute::~StrokeAttribute() = default;

        bool StrokeAttribute::isDefault() const
        {
            return mpStrokeAttribute.same_object(theGlobalDefault());
        }

        StrokeAttribute& StrokeAttribute::operator=(const StrokeAttribute&) = default;

        StrokeAttribute& StrokeAttribute::operator=(StrokeAttribute&&) = default;

        bool StrokeAttribute::operator==(const StrokeAttribute& rCandidate) const
        {
            // tdf#87509 default attr is always != non-default attr, even with same values
            if(rCandidate.isDefault() != isDefault())
                return false;

            return rCandidate.mpStrokeAttribute == mpStrokeAttribute;
        }

        const std::vector< double >& StrokeAttribute::getDotDashArray() const
        {
            return mpStrokeAttribute->getDotDashArray();
        }

        double StrokeAttribute::getFullDotDashLen() const
        {
            return mpStrokeAttribute->getFullDotDashLen();
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
