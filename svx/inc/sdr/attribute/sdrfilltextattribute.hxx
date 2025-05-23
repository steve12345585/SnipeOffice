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

#ifndef INCLUDED_SVX_INC_SDR_ATTRIBUTE_SDRFILLTEXTATTRIBUTE_HXX
#define INCLUDED_SVX_INC_SDR_ATTRIBUTE_SDRFILLTEXTATTRIBUTE_HXX

#include <drawinglayer/attribute/sdrfillattribute.hxx>
#include <drawinglayer/attribute/fillgradientattribute.hxx>
#include <sdr/attribute/sdrtextattribute.hxx>


namespace drawinglayer::attribute
    {
        class SdrFillTextAttribute
        {
            // shadow and text attributes
            SdrFillAttribute            maFill;                     // fill attributes (if used)
            FillGradientAttribute       maFillFloatTransGradient;   // fill float transparence gradient (if used)
            SdrTextAttribute            maTextAttribute;            // text and text attributes (if used)

        public:
            SdrFillTextAttribute(
                SdrFillAttribute aFill,
                FillGradientAttribute aFillFloatTransGradient,
                SdrTextAttribute aTextAttribute);
            SdrFillTextAttribute();
            SdrFillTextAttribute(const SdrFillTextAttribute& rCandidate);
            SdrFillTextAttribute& operator=(const SdrFillTextAttribute& rCandidate);

            // compare operator
            bool operator==(const SdrFillTextAttribute& rCandidate) const;

            // data access
            const SdrFillAttribute& getFill() const { return maFill; }
            const FillGradientAttribute& getFillFloatTransGradient() const { return maFillFloatTransGradient; }
            const SdrTextAttribute& getText() const { return maTextAttribute; }
        };

} // end of namespace drawinglayer::attribute


#endif // INCLUDED_SVX_INC_SDR_ATTRIBUTE_SDRFILLTEXTATTRIBUTE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
