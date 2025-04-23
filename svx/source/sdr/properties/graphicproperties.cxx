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

#include <sal/config.h>

#include <sdr/properties/graphicproperties.hxx>
#include <svl/itemset.hxx>
#include <svl/style.hxx>
#include <svx/svddef.hxx>
#include <editeng/eeitem.hxx>
#include <svx/svdograf.hxx>
#include <svx/sdgcpitm.hxx>
#include <svx/svdmodel.hxx>
#include <svx/sdgluitm.hxx>
#include <sdgcoitm.hxx>
#include <svx/sdggaitm.hxx>
#include <sdgtritm.hxx>
#include <sdginitm.hxx>
#include <svx/sdgmoitm.hxx>
#include <svx/xfillit0.hxx>
#include <svx/xlineit0.hxx>

namespace sdr::properties
{
        void GraphicProperties::applyDefaultStyleSheetFromSdrModel()
        {
            SfxStyleSheet* pStyleSheet(GetSdrObject().getSdrModelFromSdrObject().GetDefaultStyleSheetForSdrGrafObjAndSdrOle2Obj());

            if(pStyleSheet)
            {
                // do not delete hard attributes when setting dsefault Style
                SetStyleSheet(pStyleSheet, true, true);
            }
            else
            {
                RectangleProperties::applyDefaultStyleSheetFromSdrModel();
                SetMergedItem(XFillStyleItem(css::drawing::FillStyle_NONE));
                SetMergedItem(XLineStyleItem(css::drawing::LineStyle_NONE));
            }
        }

        // create a new itemset
        SfxItemSet GraphicProperties::CreateObjectSpecificItemSet(SfxItemPool& rPool)
        {
            return SfxItemSet(rPool,

                // range from SdrAttrObj
                svl::Items<SDRATTR_START, SDRATTR_SHADOW_LAST,
                SDRATTR_MISC_FIRST, SDRATTR_MISC_LAST,
                SDRATTR_TEXTDIRECTION, SDRATTR_TEXTDIRECTION,

                // range from SdrGrafObj
                SDRATTR_GRAF_FIRST, SDRATTR_GRAF_LAST,

                SDRATTR_GLOW_FIRST, SDRATTR_SOFTEDGE_LAST,
                SDRATTR_TEXTCOLUMNS_FIRST, SDRATTR_TEXTCOLUMNS_LAST,

                // range from SdrTextObj
                EE_ITEMS_START, EE_ITEMS_END>);
        }

        GraphicProperties::GraphicProperties(SdrObject& rObj)
        :   RectangleProperties(rObj)
        {
        }

        GraphicProperties::GraphicProperties(const GraphicProperties& rProps, SdrObject& rObj)
        :   RectangleProperties(rProps, rObj)
        {
        }

        GraphicProperties::~GraphicProperties()
        {
        }

        std::unique_ptr<BaseProperties> GraphicProperties::Clone(SdrObject& rObj) const
        {
            return std::unique_ptr<BaseProperties>(new GraphicProperties(*this, rObj));
        }

        void GraphicProperties::ItemSetChanged(std::span< const SfxPoolItem* const > aChangedItems, sal_uInt16 nDeletedWhich, bool bAdjustTextFrameWidthAndHeight)
        {
            SdrGrafObj& rObj = static_cast<SdrGrafObj&>(GetSdrObject());

            // local changes
            rObj.SetXPolyDirty();

            // call parent
            RectangleProperties::ItemSetChanged(aChangedItems, nDeletedWhich, bAdjustTextFrameWidthAndHeight);
        }

        void GraphicProperties::SetStyleSheet(SfxStyleSheet* pNewStyleSheet, bool bDontRemoveHardAttr,
                bool bBroadcast, bool bAdjustTextFrameWidthAndHeight)
        {
            // call parent (always first thing to do, may create the SfxItemSet)
            RectangleProperties::SetStyleSheet(pNewStyleSheet, bDontRemoveHardAttr, bBroadcast, bAdjustTextFrameWidthAndHeight);

            // local changes
            SdrGrafObj& rObj = static_cast<SdrGrafObj&>(GetSdrObject());
            rObj.SetXPolyDirty();
        }

        void GraphicProperties::ForceDefaultAttributes()
        {
            // call parent
            RectangleProperties::ForceDefaultAttributes();

            moItemSet->Put( SdrGrafLuminanceItem( 0 ) );
            moItemSet->Put( SdrGrafContrastItem( 0 ) );
            moItemSet->Put( SdrGrafRedItem( 0 ) );
            moItemSet->Put( SdrGrafGreenItem( 0 ) );
            moItemSet->Put( SdrGrafBlueItem( 0 ) );
            moItemSet->Put( SdrGrafGamma100Item( 100 ) );
            moItemSet->Put( SdrGrafTransparenceItem( 0 ) );
            moItemSet->Put( SdrGrafInvertItem( false ) );
            moItemSet->Put( SdrGrafModeItem( GraphicDrawMode::Standard ) );
            moItemSet->Put( SdrGrafCropItem( 0, 0, 0, 0 ) );
        }
} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
