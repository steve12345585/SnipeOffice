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

#ifndef INCLUDED_SVX_INC_SDR_PROPERTIES_GROUPPROPERTIES_HXX
#define INCLUDED_SVX_INC_SDR_PROPERTIES_GROUPPROPERTIES_HXX

#include <svx/sdr/properties/properties.hxx>
#include <svl/itemset.hxx>
#include <optional>

namespace sdr::properties
    {
        class GroupProperties final : public BaseProperties
        {
            // the to be used ItemSet
            mutable std::optional<SfxItemSet> moMergedItemSet;
        public:
            // basic constructor
            explicit GroupProperties(SdrObject& rObj);

            // destructor
            virtual ~GroupProperties() override;

            // create a new object specific itemset with object specific ranges.
            virtual SfxItemSet CreateObjectSpecificItemSet(SfxItemPool& pPool) override;

            // Clone() operator, normally just calls the local copy constructor
            virtual std::unique_ptr<BaseProperties> Clone(SdrObject& rObj) const override;

            // get itemset
            virtual const SfxItemSet& GetObjectItemSet() const override;

            // get merged ItemSet. Normally, this maps directly to GetObjectItemSet(), but may
            // be overridden e.g for group objects to return a merged ItemSet of the object.
            // When using this method the returned ItemSet may contain items in the state
            // SfxItemState::INVALID which means there were several such items with different
            // values.
            virtual const SfxItemSet& GetMergedItemSet() const override;

            // Set merged ItemSet. Normally, this maps to SetObjectItemSet().
            virtual void SetMergedItemSet(const SfxItemSet& rSet, bool bClearAllItems = false, bool bAdjustTextFrameWidthAndHeight = true) override;

            // set single item
            virtual void SetObjectItem(const SfxPoolItem& rItem) override;

            // set single item direct, do not do any notifies or things like that
            virtual void SetObjectItemDirect(const SfxPoolItem& rItem) override;

            // clear single item
            virtual void ClearObjectItem(const sal_uInt16 nWhich = 0) override;

            // clear single item direct, do not do any notifies or things like that.
            // Also supports complete deletion of items when default parameter 0 is used.
            virtual void ClearObjectItemDirect(const sal_uInt16 nWhich) override;

            // Set a single item, iterate over hierarchies if necessary.
            virtual void SetMergedItem(const SfxPoolItem& rItem) override;

            // Clear a single item, iterate over hierarchies if necessary.
            virtual void ClearMergedItem(const sal_uInt16 nWhich) override;

            // set complete item set
            virtual void SetObjectItemSet(const SfxItemSet& rSet, bool bAdjustTextFrameWidthAndHeight = true) override;

            // set a new StyleSheet
            virtual void SetStyleSheet(SfxStyleSheet* pNewStyleSheet, bool bDontRemoveHardAttr,
                bool bBroadcast, bool bAdjustTextFrameWidthAndHeight = true) override;

            // get the local StyleSheet
            virtual SfxStyleSheet* GetStyleSheet() const override;

            // force all attributes which come from styles to hard attributes
            // to be able to live without the style.
            virtual void ForceStyleToHardAttributes() override;
        };
} // end of namespace sdr::properties


#endif // INCLUDED_SVX_INC_SDR_PROPERTIES_GROUPPROPERTIES_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
