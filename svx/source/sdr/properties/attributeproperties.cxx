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

#include <sdr/properties/attributeproperties.hxx>
#include <tools/debug.hxx>
#include <svl/itemset.hxx>
#include <svl/style.hxx>
#include <svl/whiter.hxx>
#include <svl/poolitem.hxx>
#include <svx/svdobj.hxx>
#include <svx/xbtmpit.hxx>
#include <svx/xlndsit.hxx>
#include <svx/xlnstit.hxx>
#include <svx/xlnedit.hxx>
#include <svx/xflgrit.hxx>
#include <svx/xflftrit.hxx>
#include <svx/xflhtit.hxx>
#include <svx/svdmodel.hxx>
#include <svx/svdpage.hxx>
#include <osl/diagnose.h>

namespace sdr::properties
{
        void AttributeProperties::ImpSetParentAtSfxItemSet(bool bDontRemoveHardAttr)
        {
            if(HasSfxItemSet() && mpStyleSheet)
            {
                // Delete hard attributes where items are set in the style sheet
                if(!bDontRemoveHardAttr)
                {
                    const SfxItemSet& rStyle = mpStyleSheet->GetItemSet();
                    SfxWhichIter aIter(rStyle);
                    sal_uInt16 nWhich = aIter.FirstWhich();

                    while(nWhich)
                    {
                        if(SfxItemState::SET == aIter.GetItemState())
                        {
                            moItemSet->ClearItem(nWhich);
                        }

                        nWhich = aIter.NextWhich();
                    }
                }

                // set new stylesheet as parent
                moItemSet->SetParent(&mpStyleSheet->GetItemSet());
            }
            else
            {
                OSL_ENSURE(false, "ImpSetParentAtSfxItemSet called without SfxItemSet/SfxStyleSheet (!)");
            }
        }

        void AttributeProperties::ImpAddStyleSheet(SfxStyleSheet* pNewStyleSheet, bool bDontRemoveHardAttr)
        {
            // test if old StyleSheet is cleared, else it would be lost
            // after this method -> memory leak (!)
            DBG_ASSERT(!mpStyleSheet, "Old style sheet not deleted before setting new one (!)");

            if(!pNewStyleSheet)
                return;

            // local remember
            mpStyleSheet = pNewStyleSheet;

            if(HasSfxItemSet())
            {
                // register as listener
                StartListening(*pNewStyleSheet->GetPool());
                StartListening(*pNewStyleSheet);

                // only apply the following when we have an SfxItemSet already, else
                if(GetStyleSheet())
                {
                    ImpSetParentAtSfxItemSet(bDontRemoveHardAttr);
                }
            }
        }

        void AttributeProperties::ImpRemoveStyleSheet()
        {
            // Check type since it is destroyed when the type is deleted
            if(GetStyleSheet() && mpStyleSheet)
            {
                EndListening(*mpStyleSheet);
                if (auto const pool = mpStyleSheet->GetPool()) { // TTTT
                    EndListening(*pool);
                }

                // reset parent of ItemSet
                if(HasSfxItemSet())
                {
                    moItemSet->SetParent(nullptr);
                }

                SdrObject& rObj = GetSdrObject();
                rObj.SetBoundRectDirty();
                rObj.SetBoundAndSnapRectsDirty(/*bNotMyself*/true);
            }

            mpStyleSheet = nullptr;
        }

        // create a new itemset
        SfxItemSet AttributeProperties::CreateObjectSpecificItemSet(SfxItemPool&)
        {
            assert(false && "this class is effectively abstract, should only be instantiating subclasses");
            abort();
        }

        AttributeProperties::AttributeProperties(SdrObject& rObj)
        :   DefaultProperties(rObj),
            mpStyleSheet(nullptr)
        {
            // Do nothing else, esp. do *not* try to get and set
            // a default SfxStyle sheet. Nothing is allowed to be done
            // that may lead to calls to virtual functions like
            // CreateObjectSpecificItemSet - these would go *wrong*.
            // Thus the rest is lazy-init from here.
        }

        AttributeProperties::AttributeProperties(const AttributeProperties& rProps, SdrObject& rObj)
        :   DefaultProperties(rProps, rObj),
            mpStyleSheet(nullptr)
        {
            SfxStyleSheet* pTargetStyleSheet(rProps.GetStyleSheet());

            if(pTargetStyleSheet)
            {
                const bool bModelChange(&rObj.getSdrModelFromSdrObject() != &rProps.GetSdrObject().getSdrModelFromSdrObject());

                if(bModelChange)
                {
                    // tdf#117506
                    // The error shows that it is definitely necessary to solve this problem.
                    // Interestingly I already had a note here for 'work needed'.
                    // Checked in libreoffice-6-0 what happened there. In principle, the whole
                    // ::Clone of SdrPage and SdrObject happened in the same SdrModel, only
                    // afterwards a ::SetModel was used at the cloned SdrPage which went through
                    // all layers. The StyleSheet-problem was solved in
                    // AttributeProperties::MoveToItemPool at the end. There, a StyleSheet with the
                    // same name was searched for in the target-SdrModel.
                    // Start by resetting the current TargetStyleSheet so that nothing goes wrong
                    // when we do not find a fitting TargetStyleSheet.
                    // Note: The test for SdrModelChange above was wrong (compared the already set
                    // new SdrObject), so this never triggered and pTargetStyleSheet was never set to
                    // nullptr before. This means that a StyleSheet from another SdrModel was used
                    // what of course is very dangerous. Interestingly did not crash since when that
                    // other SdrModel was destroyed the ::Notify mechanism still worked reliably
                    // and de-connected this Properties successfully from the alien-StyleSheet.
                    pTargetStyleSheet = nullptr;

                    // Check if we have a TargetStyleSheetPool at the target-SdrModel. This *should*
                    // be the case already (SdrModel::Merge and SdDrawDocument::InsertBookmarkAsPage
                    // have already cloned the StyleSheets to the target-SdrModel when used in Draw/impress).
                    // If none is found, ImpGetDefaultStyleSheet will be used to set a 'default'
                    // StyleSheet as StyleSheet implicitly later (that's what happened in the task,
                    // thus the FillStyle changed to the 'default' Blue).
                    // Note: It *may* be necessary to do more for StyleSheets, e.g. clone/copy the
                    // StyleSheet Hierarchy from the source SdrModel and/or add the Items from there
                    // as hard attributes. If needed, have a look at the older AttributeProperties::SetModel
                    // implementation from e.g. libreoffice-6-0.
                    SfxStyleSheetBasePool* pTargetStyleSheetPool(rObj.getSdrModelFromSdrObject().GetStyleSheetPool());

                    if(nullptr != pTargetStyleSheetPool)
                    {
                        // If we have a TargetStyleSheetPool, search for the used StyleSheet
                        // in the target SdrModel using the Name from the original StyleSheet
                        // in the source-SdrModel.
                        pTargetStyleSheet = dynamic_cast< SfxStyleSheet* >(
                            pTargetStyleSheetPool->Find(
                                rProps.GetStyleSheet()->GetName(),
                                rProps.GetStyleSheet()->GetFamily()));
                    }
                }
            }

            if(!pTargetStyleSheet)
                return;

            if(HasSfxItemSet())
            {
                // The SfxItemSet has been cloned and exists,
                // we can directly set the SfxStyleSheet at it
                ImpAddStyleSheet(pTargetStyleSheet, true);
            }
            else
            {
                // No SfxItemSet exists yet (there is none in
                // the source, so none was cloned). Remember the
                // SfxStyleSheet to set it when the SfxItemSet
                // got constructed on-demand
                mpStyleSheet = pTargetStyleSheet;
            }
        }

        AttributeProperties::~AttributeProperties()
        {
            ImpRemoveStyleSheet();
        }

        std::unique_ptr<BaseProperties> AttributeProperties::Clone(SdrObject&) const
        {
            assert(false && "this class is effectively abstract, should only be instantiating subclasses");
            abort();
        }

        const SfxItemSet& AttributeProperties::GetObjectItemSet() const
        {
            // remember if we had a SfxItemSet already
            const bool bHadSfxItemSet(HasSfxItemSet());

            // call parent - this will guarantee SfxItemSet existence
            DefaultProperties::GetObjectItemSet();

            if(!bHadSfxItemSet)
            {
                // need to take care for SfxStyleSheet for newly
                // created SfxItemSet
                if(nullptr == mpStyleSheet)
                {
                    // Set missing defaults without removal of hard attributes.
                    // This is more complicated historically than I first thought:
                    // Originally for GetDefaultStyleSheetForSdrGrafObjAndSdrOle2Obj
                    // SetStyleSheet(..., false) was used, while for GetDefaultStyleSheet
                    // SetStyleSheet(..., true) was used. Thus, for SdrGrafObj and SdrOle2Obj
                    // bDontRemoveHardAttr == false -> *do* delete hard attributes was used.
                    // This was probably not done by purpose, adding the method
                    // GetDefaultStyleSheetForSdrGrafObjAndSdrOle2Obj additionally to
                    // GetDefaultStyleSheet was an enhancement to allow for SdrGrafObj/SdrOle2Obj
                    // with full AttributeSet (adding e.g. FillAttributes). To stay as compatible
                    // as possible these SdrObjects got a new default-StyleSheet.
                    // There is no reason to delete the HardAttributes and it anyways has only
                    // AFAIK effects on a single Item - the SdrTextHorzAdjustItem. To get things
                    // unified I will stay with not deleting the HardAttributes and adapt the
                    // UnitTests in CppunitTest_sd_import_tests accordingly.
                    const_cast< AttributeProperties* >(this)->applyDefaultStyleSheetFromSdrModel();
                }
                else
                {
                    // Late-Init of setting parent to SfxStyleSheet after
                    // it's creation. Can only happen from copy-constructor
                    // (where creation of SfxItemSet is avoided due to the
                    // problem with constructors and virtual functions in C++),
                    // thus DontRemoveHardAttr is not needed.
                    const_cast< AttributeProperties* >(this)->SetStyleSheet(
                        mpStyleSheet, true, true);
                }
            }

            return *moItemSet;
        }

        void AttributeProperties::ItemSetChanged(std::span< const SfxPoolItem* const > /*aChangedItems*/, sal_uInt16 /*nDeletedWhich*/, bool /*bAdjustTextFrameWidthAndHeight*/)
        {
            // own modifications
            SdrObject& rObj = GetSdrObject();

            rObj.SetBoundRectDirty();
            rObj.SetBoundAndSnapRectsDirty(/*bNotMyself*/true);
            rObj.SetChanged();
        }

        void AttributeProperties::ItemChange(const sal_uInt16 nWhich, const SfxPoolItem* pNewItem)
        {
            if(pNewItem)
            {
                std::unique_ptr<SfxPoolItem> pResultItem;
                SdrModel& rModel(GetSdrObject().getSdrModelFromSdrObject());

                switch( nWhich )
                {
                    case XATTR_FILLBITMAP:
                    {
                        pResultItem = static_cast<const XFillBitmapItem*>(pNewItem)->checkForUniqueItem( rModel );
                        break;
                    }
                    case XATTR_LINEDASH:
                    {
                        pResultItem = static_cast<const XLineDashItem*>(pNewItem)->checkForUniqueItem( rModel );
                        break;
                    }
                    case XATTR_LINESTART:
                    {
                        pResultItem = static_cast<const XLineStartItem*>(pNewItem)->checkForUniqueItem( rModel );
                        break;
                    }
                    case XATTR_LINEEND:
                    {
                        pResultItem = static_cast<const XLineEndItem*>(pNewItem)->checkForUniqueItem( rModel );
                        break;
                    }
                    case XATTR_FILLGRADIENT:
                    {
                        pResultItem = static_cast<const XFillGradientItem*>(pNewItem)->checkForUniqueItem( rModel );
                        break;
                    }
                    case XATTR_FILLFLOATTRANSPARENCE:
                    {
                        // #85953# allow all kinds of XFillFloatTransparenceItem to be set
                        pResultItem = static_cast<const XFillFloatTransparenceItem*>(pNewItem)->checkForUniqueItem( rModel );
                        break;
                    }
                    case XATTR_FILLHATCH:
                    {
                        pResultItem = static_cast<const XFillHatchItem*>(pNewItem)->checkForUniqueItem( rModel );
                        break;
                    }
                }

                // guarantee SfxItemSet existence
                GetObjectItemSet();

                if(pResultItem)
                {
                    // force ItemSet
                    moItemSet->Put(std::move(pResultItem));
                }
                else
                {
                    moItemSet->Put(*pNewItem);
                }
            }
            else
            {
                // clear item if ItemSet exists
                if(HasSfxItemSet())
                {
                    moItemSet->ClearItem(nWhich);
                }
            }
        }

        void AttributeProperties::SetStyleSheet(SfxStyleSheet* pNewStyleSheet, bool bDontRemoveHardAttr,
                bool /*bBroadcast*/, bool /*bAdjustTextFrameWidthAndHeight*/)
        {
            // Make sure we have a SfxItemSet. We are deliberately bypassing our
            // own AttributeProperties::GetObjectItemSet here, because we don't want to set a default stylesheet,
            // and then immediately remove it, which is costly.
            DefaultProperties::GetObjectItemSet();

            ImpRemoveStyleSheet();
            ImpAddStyleSheet(pNewStyleSheet, bDontRemoveHardAttr);

            SdrObject& rObj = GetSdrObject();
            rObj.SetBoundRectDirty();
            rObj.SetBoundAndSnapRectsDirty(true);
        }

        SfxStyleSheet* AttributeProperties::GetStyleSheet() const
        {
            return mpStyleSheet;
        }

        void AttributeProperties::ForceStyleToHardAttributes()
        {
            if(!GetStyleSheet() || mpStyleSheet == nullptr)
                return;

            // guarantee SfxItemSet existence
            GetObjectItemSet();

            // prepare copied, new itemset, but WITHOUT parent
            SfxItemSet aDestItemSet(*moItemSet);
            aDestItemSet.SetParent(nullptr);

            // prepare forgetting the current stylesheet like in RemoveStyleSheet()
            EndListening(*mpStyleSheet);
            EndListening(*mpStyleSheet->GetPool());

            // prepare the iter; use the mpObjectItemSet which may have less
            // WhichIDs than the style.
            SfxWhichIter aIter(aDestItemSet);
            sal_uInt16 nWhich(aIter.FirstWhich());
            const SfxPoolItem *pItem = nullptr;

            // now set all hard attributes of the current at the new itemset
            while(nWhich)
            {
                // #i61284# use mpItemSet with parents, makes things easier and reduces to
                // one loop
                if(SfxItemState::SET == moItemSet->GetItemState(nWhich, true, &pItem))
                {
                    aDestItemSet.Put(*pItem);
                }

                nWhich = aIter.NextWhich();
            }

            // replace itemsets
            moItemSet.emplace(std::move(aDestItemSet));

            // set necessary changes like in RemoveStyleSheet()
            GetSdrObject().SetBoundRectDirty();
            GetSdrObject().SetBoundAndSnapRectsDirty(/*bNotMyself*/true);

            mpStyleSheet = nullptr;
        }

        void AttributeProperties::Notify(SfxBroadcaster& rBC, const SfxHint& rHint)
        {
            bool bHintUsed(false);

            SfxHintId id = rHint.GetId();
            if (id == SfxHintId::StyleSheetChanged
                || id == SfxHintId::StyleSheetErased
                || id == SfxHintId::StyleSheetModified
                || id == SfxHintId::StyleSheetInDestruction
                || id == SfxHintId::StyleSheetModifiedExtended)
            {
                const SfxStyleSheetHint* pStyleHint = static_cast<const SfxStyleSheetHint*>(&rHint);

                if(pStyleHint->GetStyleSheet() == GetStyleSheet())
                {
                    SdrObject& rObj = GetSdrObject();
                    //SdrPage* pPage = rObj.GetPage();

                    switch(id)
                    {
                        case SfxHintId::StyleSheetModified        :
                        case SfxHintId::StyleSheetModifiedExtended:
                        case SfxHintId::StyleSheetChanged         :
                        {
                            // notify change
                            break;
                        }
                        case SfxHintId::StyleSheetErased          :
                        case SfxHintId::StyleSheetInDestruction   :
                        {
                            // Style needs to be exchanged
                            SfxStyleSheet* pNewStSh = nullptr;
                            SdrModel& rModel(rObj.getSdrModelFromSdrObject());

                            // Do nothing if object is in destruction, else a StyleSheet may be found from
                            // a StyleSheetPool which is just being deleted itself. and thus it would be fatal
                            // to register as listener to that new StyleSheet.
                            if(!rObj.IsInDestruction())
                            {
                                if(SfxStyleSheet* pStyleSheet = GetStyleSheet())
                                {
                                    pNewStSh = static_cast<SfxStyleSheet*>(rModel.GetStyleSheetPool()->Find(
                                        pStyleSheet->GetParent(), pStyleSheet->GetFamily()));
                                }

                                if(!pNewStSh)
                                {
                                    pNewStSh = rModel.GetDefaultStyleSheet();
                                }
                            }

                            // remove used style, it's erased or in destruction
                            ImpRemoveStyleSheet();

                            if(pNewStSh)
                            {
                                ImpAddStyleSheet(pNewStSh, true);
                            }

                            break;
                        }
                        default: break;
                    }

                    // Get old BoundRect. Do this after the style change is handled
                    // in the ItemSet parts because GetBoundRect() may calculate a new
                    tools::Rectangle aBoundRect = rObj.GetLastBoundRect();

                    rObj.SetBoundAndSnapRectsDirty(/*bNotMyself*/true);

                    // tell the object about the change
                    rObj.SetChanged();
                    rObj.BroadcastObjectChange();

                    //if(pPage && pPage->IsInserted())
                    //{
                    //  rObj.BroadcastObjectChange();
                    //}

                    rObj.SendUserCall(SdrUserCallType::ChangeAttr, aBoundRect);

                    bHintUsed = true;
                }
            }
            if(!bHintUsed)
            {
                // forward to SdrObject ATM. Not sure if this will be necessary
                // in the future.
                GetSdrObject().Notify(rBC, rHint);
            }
        }

        bool AttributeProperties::isUsedByModel() const
        {
            const SdrObject& rObj(GetSdrObject());
            if (rObj.IsInserted())
            {
                const SdrPage* const pPage(rObj.getSdrPageFromSdrObject());
                if (pPage && pPage->IsInserted())
                    return true;
            }
            return false;
        }

        void AttributeProperties::applyDefaultStyleSheetFromSdrModel()
        {
            SfxStyleSheet* pDefaultStyleSheet(GetSdrObject().getSdrModelFromSdrObject().GetDefaultStyleSheet());

            // tdf#118139 Only do this when StyleSheet really differs. It may e.g.
            // be the case that nullptr == pDefaultStyleSheet and there is none set yet,
            // so indeed no need to set it (needed for some strange old MSWord2003
            // documents with CustomShape-'Group' and added Text-Frames, see task description)
            if(pDefaultStyleSheet != GetStyleSheet())
            {
                // do not delete hard attributes when setting dsefault Style
                SetStyleSheet(pDefaultStyleSheet, true, true);
            }
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
