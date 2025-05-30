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

#ifndef INCLUDED_SVX_SDR_PROPERTIES_RECTANGLEPROPERTIES_HXX
#define INCLUDED_SVX_SDR_PROPERTIES_RECTANGLEPROPERTIES_HXX

#include <sdr/properties/textproperties.hxx>


namespace sdr::properties
    {
        class RectangleProperties : public TextProperties
        {
        protected:
            // react on ItemSet changes
            virtual void ItemSetChanged(std::span< const SfxPoolItem* const > aChangedItems, sal_uInt16 nDeletedWhich, bool bAdjustTextFrameWidthAndHeight = true) override;

        public:
            // basic constructor
            explicit RectangleProperties(SdrObject& rObj);

            // constructor for copying, but using new object
            RectangleProperties(const RectangleProperties& rProps, SdrObject& rObj);

            // destructor
            virtual ~RectangleProperties() override;

            // Clone() operator, normally just calls the local copy constructor
            virtual std::unique_ptr<BaseProperties> Clone(SdrObject& rObj) const override;

            // set a new StyleSheet and broadcast
            virtual void SetStyleSheet(SfxStyleSheet* pNewStyleSheet, bool bDontRemoveHardAttr,
                bool bBroadcast, bool bAdjustTextFrameWidthAndHeight = true) override;
        };
} // end of namespace sdr::properties


#endif // INCLUDED_SVX_SDR_PROPERTIES_RECTANGLEPROPERTIES_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
