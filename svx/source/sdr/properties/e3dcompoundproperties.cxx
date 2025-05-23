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

#include <sdr/properties/e3dcompoundproperties.hxx>
#include <svl/itemset.hxx>
#include <svx/obj3d.hxx>
#include <svx/scene3d.hxx>


namespace sdr::properties
{
        E3dCompoundProperties::E3dCompoundProperties(SdrObject& rObj)
        :   E3dProperties(rObj)
        {
        }

        E3dCompoundProperties::E3dCompoundProperties(const E3dCompoundProperties& rProps, SdrObject& rObj)
        :   E3dProperties(rProps, rObj)
        {
        }

        E3dCompoundProperties::~E3dCompoundProperties()
        {
        }

        std::unique_ptr<BaseProperties> E3dCompoundProperties::Clone(SdrObject& rObj) const
        {
            return std::unique_ptr<BaseProperties>(new E3dCompoundProperties(*this, rObj));
        }

        const SfxItemSet& E3dCompoundProperties::GetMergedItemSet() const
        {
            // include Items of scene this object belongs to
            const E3dCompoundObject& rObj = static_cast<const E3dCompoundObject&>(GetSdrObject());
            E3dScene* pScene(rObj.getRootE3dSceneFromE3dObject());

            if(nullptr != pScene)
            {
                // force ItemSet
                GetObjectItemSet();

                // add filtered scene properties (SDRATTR_3DSCENE_) to local itemset
                SfxItemSet aSet(SfxItemSet::makeFixedSfxItemSet<SDRATTR_3DSCENE_FIRST, SDRATTR_3DSCENE_LAST>(*moItemSet->GetPool()));
                aSet.Put(pScene->GetProperties().GetObjectItemSet());
                moItemSet->Put(aSet);
            }

            // call parent
            return E3dProperties::GetMergedItemSet();
        }

        void E3dCompoundProperties::SetMergedItemSet(const SfxItemSet& rSet, bool bClearAllItems, bool bAdjustTextFrameWidthAndHeight)
        {
            // Set scene specific items at scene
            E3dCompoundObject& rObj = static_cast<E3dCompoundObject&>(GetSdrObject());
            E3dScene* pScene(rObj.getRootE3dSceneFromE3dObject());

            if(nullptr != pScene)
            {
                // force ItemSet
                GetObjectItemSet();

                // Generate filtered scene properties (SDRATTR_3DSCENE_) itemset
                SfxItemSet aSet(SfxItemSet::makeFixedSfxItemSet<SDRATTR_3DSCENE_FIRST, SDRATTR_3DSCENE_LAST>(*moItemSet->GetPool()));
                aSet.Put(rSet);

                if(bClearAllItems)
                {
                    pScene->GetProperties().ClearObjectItem();
                }

                if(aSet.Count())
                {
                    pScene->GetProperties().SetObjectItemSet(aSet);
                }
            }

            // call parent. This will set items on local object, too.
            E3dProperties::SetMergedItemSet(rSet, bClearAllItems, bAdjustTextFrameWidthAndHeight);
        }

        void E3dCompoundProperties::PostItemChange(const sal_uInt16 nWhich)
        {
            // call parent
            E3dProperties::PostItemChange(nWhich);

            // handle value change
            E3dCompoundObject& rObj = static_cast<E3dCompoundObject&>(GetSdrObject());

            switch(nWhich)
            {
                // #i28528#
                // Added extra Item (Bool) for chart2 to be able to show reduced line geometry
                case SDRATTR_3DOBJ_REDUCED_LINE_GEOMETRY:
                {
                    rObj.ActionChanged();
                    break;
                }
                case SDRATTR_3DOBJ_DOUBLE_SIDED:
                {
                    rObj.ActionChanged();
                    break;
                }
                case SDRATTR_3DOBJ_NORMALS_KIND:
                {
                    rObj.ActionChanged();
                    break;
                }
                case SDRATTR_3DOBJ_NORMALS_INVERT:
                {
                    rObj.ActionChanged();
                    break;
                }
                case SDRATTR_3DOBJ_TEXTURE_PROJ_X:
                {
                    rObj.ActionChanged();
                    break;
                }
                case SDRATTR_3DOBJ_TEXTURE_PROJ_Y:
                {
                    rObj.ActionChanged();
                    break;
                }
            }
        }
} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
