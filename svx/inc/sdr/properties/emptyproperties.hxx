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

#ifndef INCLUDED_SVX_SDR_PROPERTIES_EMPTYPROPERTIES_HXX
#define INCLUDED_SVX_SDR_PROPERTIES_EMPTYPROPERTIES_HXX

#include <sal/config.h>

#include <svx/sdr/properties/properties.hxx>
#include <svl/itemset.hxx>


namespace sdr::properties
    {
        class EmptyProperties final : public BaseProperties
        {
        public:
            // basic constructor
            explicit EmptyProperties(SdrObject& rObj);

            // Clone() operator, normally just calls the local copy constructor
            virtual std::unique_ptr<BaseProperties> Clone(SdrObject& rObj) const override;

            // create a new object specific itemset with object specific ranges.
            virtual SfxItemSet CreateObjectSpecificItemSet(SfxItemPool& pPool) override;

            // get itemset
            virtual const SfxItemSet& GetObjectItemSet() const override;

            // set single item
            virtual void SetObjectItem(const SfxPoolItem& rItem) override;

            // set single item direct, do not do any notifies or things like that
            virtual void SetObjectItemDirect(const SfxPoolItem& rItem) override;

            // clear single item
            virtual void ClearObjectItem(const sal_uInt16 nWhich = 0) override;

            // clear single item direct, do not do any notifies or things like that.
            // Also supports complete deletion of items when default parameter 0 is used.
            virtual void ClearObjectItemDirect(const sal_uInt16 nWhich) override;

            // set complete item set
            virtual void SetObjectItemSet(const SfxItemSet& rSet, bool bAdjustTextFrameWidthAndHeight = true) override;

            // set a new StyleSheet and broadcast
            virtual void SetStyleSheet(SfxStyleSheet* pNewStyleSheet, bool bDontRemoveHardAttr,
                bool bBroadcast, bool bAdjustTextFrameWidthAndHeight = true) override;

            // get the installed StyleSheet
            virtual SfxStyleSheet* GetStyleSheet() const override;
        };
} // end of namespace sdr::properties

#endif // INCLUDED_SVX_SDR_PROPERTIES_EMPTYPROPERTIES_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
