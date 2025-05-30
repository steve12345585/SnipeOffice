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


#include <com/sun/star/drawing/BitmapMode.hpp>
#include <com/sun/star/style/XStyle.hpp>
#include <com/sun/star/text/WritingMode.hpp>
#include <com/sun/star/table/TableBorder.hpp>
#include <com/sun/star/table/BorderLine2.hpp>
#include <com/sun/star/lang/Locale.hpp>

#include <comphelper/sequence.hxx>
#include <o3tl/any.hxx>
#include <svl/grabbagitem.hxx>
#include <svl/style.hxx>
#include <svl/itemset.hxx>

#include <utility>
#include <vcl/svapp.hxx>
#include <libxml/xmlwriter.h>

#include <sdr/properties/textproperties.hxx>
#include <sdr/properties/cellproperties.hxx>
#include <editeng/outlobj.hxx>
#include <editeng/writingmodeitem.hxx>
#include <svx/sdtfchim.hxx>
#include <svx/svdotable.hxx>
#include <svx/svdoutl.hxx>
#include <svx/unoshtxt.hxx>
#include <svx/svdmodel.hxx>
#include <svx/sdooitm.hxx>
#include <svx/sdtagitm.hxx>
#include <svx/sdmetitm.hxx>
#include <svx/xit.hxx>
#include <getallcharpropids.hxx>
#include "tableundo.hxx"
#include <cell.hxx>
#include <svx/unoshprp.hxx>
#include <svx/unoshape.hxx>
#include <editeng/editobj.hxx>
#include <editeng/borderline.hxx>
#include <editeng/boxitem.hxx>
#include <editeng/charrotateitem.hxx>
#include <svx/xflbstit.hxx>
#include <svx/xflbmtit.hxx>
#include <svx/xlnclit.hxx>
#include <svx/svdpool.hxx>
#include <svx/xflclit.hxx>
#include <comphelper/diagnose_ex.hxx>


using ::editeng::SvxBorderLine;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::text;
using namespace ::com::sun::star::table;
using namespace ::com::sun::star::drawing;
using namespace ::com::sun::star::style;


static const SvxItemPropertySet* ImplGetSvxCellPropertySet()
{
    // property map for an outliner text
    static const SfxItemPropertyMapEntry aSvxCellPropertyMap[] =
    {
        FILL_PROPERTIES
//      { "HasLevels",                    OWN_ATTR_HASLEVELS,             cppu::UnoType<bool>::get(), css::beans::PropertyAttribute::READONLY,      0},
        { u"Style"_ustr,                        OWN_ATTR_STYLE,                 cppu::UnoType< css::style::XStyle >::get(),                                    css::beans::PropertyAttribute::MAYBEVOID, 0},
        { UNO_NAME_TEXT_FONTINDEPENDENTLINESPACING, SDRATTR_TEXT_USEFIXEDCELLHEIGHT, cppu::UnoType<bool>::get(), 0, 0},
        { UNO_NAME_TEXT_WRITINGMODE,      SDRATTR_TEXTDIRECTION,          cppu::UnoType<css::text::WritingMode>::get(),                         0,      0},
        { UNO_NAME_TEXT_HORZADJUST,       SDRATTR_TEXT_HORZADJUST,        cppu::UnoType<css::drawing::TextHorizontalAdjust>::get(),  0,      0},
        { UNO_NAME_TEXT_LEFTDIST,         SDRATTR_TEXT_LEFTDIST,          cppu::UnoType<sal_Int32>::get(),        0,      0, PropertyMoreFlags::METRIC_ITEM},
        { UNO_NAME_TEXT_LOWERDIST,        SDRATTR_TEXT_LOWERDIST,         cppu::UnoType<sal_Int32>::get(),        0,      0, PropertyMoreFlags::METRIC_ITEM},
        { UNO_NAME_TEXT_RIGHTDIST,        SDRATTR_TEXT_RIGHTDIST,         cppu::UnoType<sal_Int32>::get(),        0,      0, PropertyMoreFlags::METRIC_ITEM},
        { UNO_NAME_TEXT_UPPERDIST,        SDRATTR_TEXT_UPPERDIST,         cppu::UnoType<sal_Int32>::get(),        0,      0, PropertyMoreFlags::METRIC_ITEM},
        { UNO_NAME_TEXT_VERTADJUST,       SDRATTR_TEXT_VERTADJUST,        cppu::UnoType<css::drawing::TextVerticalAdjust>::get(),    0,      0},
        { UNO_NAME_TEXT_WORDWRAP,         SDRATTR_TEXT_WORDWRAP,          cppu::UnoType<bool>::get(),        0,      0},

        { u"TableBorder"_ustr,                  OWN_ATTR_TABLEBORDER,           cppu::UnoType<TableBorder>::get(), 0, 0 },
        { u"TopBorder"_ustr,                    SDRATTR_TABLE_BORDER,           cppu::UnoType<BorderLine>::get(), 0, TOP_BORDER },
        { u"BottomBorder"_ustr,                 SDRATTR_TABLE_BORDER,           cppu::UnoType<BorderLine>::get(), 0, BOTTOM_BORDER },
        { u"LeftBorder"_ustr,                   SDRATTR_TABLE_BORDER,           cppu::UnoType<BorderLine>::get(), 0, LEFT_BORDER },
        { u"RightBorder"_ustr,                  SDRATTR_TABLE_BORDER,           cppu::UnoType<BorderLine>::get(), 0, RIGHT_BORDER },
        { u"RotateAngle"_ustr,                  SDRATTR_TABLE_TEXT_ROTATION,    cppu::UnoType<sal_Int32>::get(), 0, 0 },
        { u"CellInteropGrabBag"_ustr,           SDRATTR_TABLE_CELL_GRABBAG,     cppu::UnoType<css::uno::Sequence<css::beans::PropertyValue>>::get(), 0, 0 },

        SVX_UNOEDIT_OUTLINER_PROPERTIES,
        SVX_UNOEDIT_CHAR_PROPERTIES,
        SVX_UNOEDIT_PARA_PROPERTIES,
    };

    static SvxItemPropertySet aSvxCellPropertySet( aSvxCellPropertyMap, SdrObject::GetGlobalDrawObjectItemPool() );
    return &aSvxCellPropertySet;
}

namespace sdr::properties
{

CellTextProvider::CellTextProvider(sdr::table::CellRef xCell)
    : m_xCell(std::move(xCell))
{
}

CellTextProvider::~CellTextProvider()
{
}

sal_Int32 CellTextProvider::getTextCount() const
{
    return 1;
}

SdrText* CellTextProvider::getText(sal_Int32 nIndex) const
{
    (void) nIndex;
    assert(nIndex == 0);
    return m_xCell.get();
}

        // create a new itemset
        SfxItemSet CellProperties::CreateObjectSpecificItemSet(SfxItemPool& rPool)
        {
            return SfxItemSet(rPool,

                // range from SdrAttrObj
                svl::Items<SDRATTR_START, SDRATTR_SHADOW_LAST,
                SDRATTR_MISC_FIRST, SDRATTR_MISC_LAST,
                SDRATTR_TEXTDIRECTION, SDRATTR_TEXTDIRECTION,

                // range for SdrTableObj
                SDRATTR_TABLE_FIRST, SDRATTR_TABLE_LAST,

                // range from SdrTextObj
                EE_ITEMS_START, EE_ITEMS_END>);
        }

        const svx::ITextProvider& CellProperties::getTextProvider() const
        {
            return maTextProvider;
        }

        CellProperties::CellProperties(SdrObject& rObj, sdr::table::Cell* pCell)
        :   TextProperties(rObj)
        ,   mxCell(pCell)
        ,   maTextProvider(mxCell)
        {
        }

        CellProperties::CellProperties(const CellProperties& rProps, SdrObject& rObj, sdr::table::Cell* pCell)
        :   TextProperties(rProps, rObj)
        ,   mxCell( pCell )
        ,   maTextProvider(mxCell)
        {
        }

        CellProperties::~CellProperties()
        {
        }

        std::unique_ptr<BaseProperties> CellProperties::Clone(SdrObject& rObj) const
        {
            OSL_FAIL("CellProperties::Clone(), does not work yet!");
            return std::unique_ptr<BaseProperties>(new CellProperties(*this, rObj,nullptr));
        }

        void CellProperties::ForceDefaultAttributes()
        {
            // deliberately do not run superclass ForceDefaultAttributes, we don't want any default attributes
        }

        void CellProperties::ItemSetChanged(std::span< const SfxPoolItem* const > aChangedItems, sal_uInt16 nDeletedWhich, bool bAdjustTextFrameWidthAndHeight)
        {
            SdrTextObj& rObj = static_cast<SdrTextObj&>(GetSdrObject());

            if( mxCell.is() )
            {
                std::optional<OutlinerParaObject> pParaObj = mxCell->CreateEditOutlinerParaObject();

                if( !pParaObj && mxCell->GetOutlinerParaObject())
                    pParaObj = *mxCell->GetOutlinerParaObject();

                if(pParaObj)
                {
                    // handle outliner attributes
                    Outliner* pOutliner = nullptr;

                    if(mxCell->IsTextEditActive())
                    {
                        pOutliner = rObj.GetTextEditOutliner();
                    }
                    else
                    {
                        pOutliner = &rObj.ImpGetDrawOutliner();
                        pOutliner->SetText(*pParaObj);
                    }

                    sal_Int32 nParaCount(pOutliner->GetParagraphCount());

                    // if the user sets character attributes to the complete
                    // cell we want to remove all hard set character attributes
                    // with same which ids from the text
                    std::vector<sal_uInt16> aCharWhichIds(GetAllCharPropIds(aChangedItems));

                    for(sal_Int32 nPara = 0; nPara < nParaCount; nPara++)
                    {
                        SfxItemSet aSet(pOutliner->GetParaAttribs(nPara));
                        for (const SfxPoolItem* pItem : aChangedItems)
                            aSet.Put(*pItem);
                        if (nDeletedWhich)
                            aSet.ClearItem(nDeletedWhich);

                        for (const auto& rWhichId : aCharWhichIds)
                        {
                            pOutliner->RemoveCharAttribs(nPara, rWhichId);
                        }

                        pOutliner->SetParaAttribs(nPara, aSet);
                    }

                    if(!mxCell->IsTextEditActive())
                    {
                        if(nParaCount)
                        {
                            // force ItemSet
                            GetObjectItemSet();

                            SfxItemSet aNewSet(pOutliner->GetParaAttribs(0));
                            moItemSet->Put(aNewSet);
                        }

                        std::optional<OutlinerParaObject> pTemp = pOutliner->CreateParaObject(0, nParaCount);
                        pOutliner->Clear();
                        mxCell->SetOutlinerParaObject(std::move(pTemp));
                    }

                }
            }

            // call parent
            AttributeProperties::ItemSetChanged(aChangedItems, nDeletedWhich, bAdjustTextFrameWidthAndHeight);

            if( mxCell.is() )
                mxCell->notifyModified();
        }

        void CellProperties::ItemChange(const sal_uInt16 nWhich, const SfxPoolItem* pNewItem)
        {
            if(pNewItem && (SDRATTR_TEXTDIRECTION == nWhich))
            {
                bool bVertical(css::text::WritingMode_TB_RL == static_cast<const SvxWritingModeItem*>(pNewItem)->GetValue());

                sdr::table::SdrTableObj& rObj = static_cast<sdr::table::SdrTableObj&>(GetSdrObject());
                rObj.SetVerticalWriting(bVertical);

                // Set a cell vertical property
                std::optional<OutlinerParaObject> pEditParaObj = mxCell->CreateEditOutlinerParaObject();

                if( !pEditParaObj && mxCell->GetOutlinerParaObject() )
                {
                    OutlinerParaObject* pParaObj = mxCell->GetOutlinerParaObject();
                    if(pParaObj)
                        pParaObj->SetVertical(bVertical);
                }
            }

            if (pNewItem && (SDRATTR_TABLE_TEXT_ROTATION == nWhich))
            {
                const SvxTextRotateItem* pRotateItem = static_cast<const SvxTextRotateItem*>(pNewItem);

                // Set a cell vertical property
                std::optional<OutlinerParaObject> pEditParaObj = mxCell->CreateEditOutlinerParaObject();

                if (!pEditParaObj && mxCell->GetOutlinerParaObject())
                {
                    OutlinerParaObject* pParaObj = mxCell->GetOutlinerParaObject();
                    if (pParaObj)
                    {
                        if(pRotateItem->IsVertical() && pRotateItem->IsTopToBottom())
                            pParaObj->SetRotation(TextRotation::TOPTOBOTTOM);
                        else if (pRotateItem->IsVertical())
                            pParaObj->SetRotation(TextRotation::BOTTOMTOTOP);
                        else
                            pParaObj->SetRotation(TextRotation::NONE);
                    }
                }

                // Change autogrow direction
                SdrTextObj& rObj = static_cast<SdrTextObj&>(GetSdrObject());

                // rescue object size
                tools::Rectangle aObjectRect = rObj.GetSnapRect();

                const SfxItemSet& rSet = rObj.GetObjectItemSet();
                bool bAutoGrowWidth = rSet.Get(SDRATTR_TEXT_AUTOGROWWIDTH).GetValue();
                bool bAutoGrowHeight = rSet.Get(SDRATTR_TEXT_AUTOGROWHEIGHT).GetValue();

                // prepare ItemSet to set exchanged width and height items
                SfxItemSetFixed<SDRATTR_TEXT_AUTOGROWHEIGHT, SDRATTR_TEXT_AUTOGROWHEIGHT> aNewSet(*rSet.GetPool());

                aNewSet.Put(rSet);
                aNewSet.Put(makeSdrTextAutoGrowWidthItem(bAutoGrowHeight));
                aNewSet.Put(makeSdrTextAutoGrowHeightItem(bAutoGrowWidth));
                rObj.SetObjectItemSet(aNewSet);

                // restore object size
                rObj.SetSnapRect(aObjectRect);
            }

            // call parent
            AttributeProperties::ItemChange( nWhich, pNewItem );
        }

} // end of namespace sdr::properties

namespace sdr::table {


// Cell


rtl::Reference< Cell > Cell::create( SdrTableObj& rTableObj )
{
    rtl::Reference< Cell > xCell( new Cell( rTableObj ) );
    if( xCell->mxTable.is() )
    {
        xCell->mxTable->addEventListener( xCell );
    }
    return xCell;
}


Cell::Cell(
    SdrTableObj& rTableObj)
:   SdrText(rTableObj)
    ,SvxUnoTextBase( ImplGetSvxUnoOutlinerTextCursorSvxPropertySet() )
    ,mpPropSet( ImplGetSvxCellPropertySet() )
    ,mpProperties( new sdr::properties::CellProperties( rTableObj, this ) )
    ,mnCellContentType( CellContentType_EMPTY )
    ,mfValue( 0.0 )
    ,mnError( 0 )
    ,mbMerged( false )
    ,mnRowSpan( 1 )
    ,mnColSpan( 1 )
    ,mxTable( rTableObj.getTable() )
{
    // Caution: Old SetModel() indirectly did a very necessary thing here,
    // it created a valid SvxTextEditSource which is needed to bind contained
    // Text to the UNO API and thus to save/load and more. Added version without
    // model change.
    // Also done was (not needed, for reference):
    //         SetStyleSheet( nullptr, true );
    //         ForceOutlinerParaObject( OutlinerMode::TextObject );
    if(nullptr == GetEditSource())
    {
        SetEditSource(new SvxTextEditSource(&GetObject(), this));
    }
}

Cell::~Cell() COVERITY_NOEXCEPT_FALSE
{
    dispose();
}

void Cell::dispose()
{
    if( mxTable.is() )
    {
        try
        {
            Reference< XEventListener > xThis( this );
            mxTable->removeEventListener( xThis );
        }
        catch( Exception& )
        {
            TOOLS_WARN_EXCEPTION("svx.table", "");
        }
        mxTable.clear();
    }

    // tdf#118199 avoid double dispose, detect by using mpProperties
    // as indicator. Only use SetOutlinerParaObject once
    if( mpProperties )
    {
        mpProperties.reset();
        SetOutlinerParaObject( std::nullopt );
    }
}

void Cell::merge( sal_Int32 nColumnSpan, sal_Int32 nRowSpan )
{
    if ((mnColSpan != nColumnSpan) || (mnRowSpan != nRowSpan) || mbMerged)
    {
        mnColSpan = nColumnSpan;
        mnRowSpan = nRowSpan;
        mbMerged = false;
        notifyModified();
    }
}


void Cell::mergeContent( const CellRef& xSourceCell )
{
    SdrTableObj& rTableObj = dynamic_cast< SdrTableObj& >( GetObject() );

    if( !xSourceCell->hasText() )
        return;

    SdrOutliner& rOutliner=rTableObj.ImpGetDrawOutliner();
    rOutliner.SetUpdateLayout(true);

    if( hasText() )
    {
        rOutliner.SetText(*GetOutlinerParaObject());
        rOutliner.AddText(*xSourceCell->GetOutlinerParaObject());
    }
    else
    {
        rOutliner.SetText(*xSourceCell->GetOutlinerParaObject());
    }

    SetOutlinerParaObject( rOutliner.CreateParaObject() );
    rOutliner.Clear();
    xSourceCell->SetOutlinerParaObject(rOutliner.CreateParaObject());
    rOutliner.Clear();
    SetStyleSheet( GetStyleSheet(), true );
}


void Cell::cloneFrom( const CellRef& xCell )
{
    if( xCell.is() )
    {
        replaceContentAndFormatting( xCell );

        mnCellContentType = xCell->mnCellContentType;

        msFormula = xCell->msFormula;
        mfValue = xCell->mfValue;
        mnError = xCell->mnError;

        mbMerged = xCell->mbMerged;
        mnRowSpan = xCell->mnRowSpan;
        mnColSpan = xCell->mnColSpan;

    }
    notifyModified();
}

void Cell::replaceContentAndFormatting( const CellRef& xSourceCell )
{
    if( !(xSourceCell.is() && mpProperties) )
        return;

    mpProperties->SetMergedItemSet( xSourceCell->GetObjectItemSet() );

    // tdf#118354 OutlinerParaObject may be nullptr, do not dereference when
    // not set (!)
    if(xSourceCell->GetOutlinerParaObject())
    {
        SetOutlinerParaObject( *xSourceCell->GetOutlinerParaObject() );
    }

    SdrTableObj& rTableObj = dynamic_cast< SdrTableObj& >( GetObject() );
    SdrTableObj& rSourceTableObj = dynamic_cast< SdrTableObj& >( xSourceCell->GetObject() );

    if(&rSourceTableObj.getSdrModelFromSdrObject() != &rTableObj.getSdrModelFromSdrObject())
    {
        // TTTT should not happen - if, then a clone may be needed
        // Maybe add an assertion here later
        SetStyleSheet( nullptr, true );
    }
}


void Cell::setMerged()
{
    if( !mbMerged )
    {
        mbMerged = true;
        notifyModified();
    }
}


void Cell::copyFormatFrom( const CellRef& xSourceCell )
{
    if( !(xSourceCell.is() && mpProperties) )
        return;

    mpProperties->SetMergedItemSet( xSourceCell->GetObjectItemSet() );
    SdrTableObj& rTableObj = dynamic_cast< SdrTableObj& >( GetObject() );
    SdrTableObj& rSourceTableObj = dynamic_cast< SdrTableObj& >( xSourceCell->GetObject() );

    if(&rSourceTableObj.getSdrModelFromSdrObject() != &rTableObj.getSdrModelFromSdrObject())
    {
        // TTTT should not happen - if, then a clone may be needed
        // Maybe add an assertion here later
        SetStyleSheet( nullptr, true );
    }

    notifyModified();
}


void Cell::notifyModified()
{
    if( mxTable.is() )
        mxTable->setModified( true );
}


// SdrTextShape proxy


bool Cell::IsActiveCell() const
{
    bool isActive = false;
    SdrTableObj& rTableObj = dynamic_cast< SdrTableObj& >( GetObject() );
    if( rTableObj.getActiveCell().get() == this )
        isActive = true;

    return isActive;
}

bool Cell::IsTextEditActive() const
{
    bool isActive = false;
    SdrTableObj& rTableObj = dynamic_cast< SdrTableObj& >( GetObject() );
    if(rTableObj.getActiveCell().get() == this )
    {
        if( rTableObj.CanCreateEditOutlinerParaObject() )
        {
            isActive = true;
        }
    }
    return isActive;
}


bool Cell::hasText() const
{
    const OutlinerParaObject* pParaObj = GetOutlinerParaObject();
    if( pParaObj )
    {
        const EditTextObject& rTextObj = pParaObj->GetTextObject();
        if( rTextObj.GetParagraphCount() >= 1 )
        {
            if( rTextObj.GetParagraphCount() == 1 )
            {
                if( !rTextObj.HasText(0) )
                    return false;
            }
            return true;
        }
    }

    return false;
}

bool Cell::CanCreateEditOutlinerParaObject() const
{
    SdrTableObj& rTableObj = dynamic_cast< SdrTableObj& >( GetObject() );
    if( rTableObj.getActiveCell().get() == this )
        return rTableObj.CanCreateEditOutlinerParaObject();
    return false;
}

std::optional<OutlinerParaObject> Cell::CreateEditOutlinerParaObject() const
{
    SdrTableObj& rTableObj = dynamic_cast< SdrTableObj& >( GetObject() );
    if( rTableObj.getActiveCell().get() == this )
        return rTableObj.CreateEditOutlinerParaObject();
    return std::nullopt;
}


void Cell::SetStyleSheet( SfxStyleSheet* pStyleSheet, bool bDontRemoveHardAttr )
{
    // only allow cell styles for cells
    if( pStyleSheet && pStyleSheet->GetFamily() != SfxStyleFamily::Frame )
        return;

    if( mpProperties && (mpProperties->GetStyleSheet() != pStyleSheet) )
    {
        mpProperties->SetStyleSheet( pStyleSheet, bDontRemoveHardAttr, true );
    }
}


const SfxItemSet& Cell::GetObjectItemSet()
{
    if( mpProperties )
    {
        return mpProperties->GetObjectItemSet();
    }
    else
    {
        OSL_FAIL("Cell::GetObjectItemSet(), called without properties!");
        return GetObject().GetObjectItemSet();
    }
}

void Cell::SetObjectItem(const SfxPoolItem& rItem)
{
    if( mpProperties )
    {
        mpProperties->SetObjectItem( rItem );
        notifyModified();
    }
}

void Cell::SetMergedItem(const SfxPoolItem& rItem)
{
    SetObjectItem(rItem);
}

SfxStyleSheet* Cell::GetStyleSheet() const
{
    if( mpProperties )
        return mpProperties->GetStyleSheet();
    else
        return nullptr;
}

void Cell::TakeTextAnchorRect(tools::Rectangle& rAnchorRect) const
{
    rAnchorRect.SetLeft( maCellRect.Left() + GetTextLeftDistance() );
    rAnchorRect.SetRight( maCellRect.Right() - GetTextRightDistance() );
    rAnchorRect.SetTop( maCellRect.Top() + GetTextUpperDistance() );
    rAnchorRect.SetBottom( maCellRect.Bottom() - GetTextLowerDistance() );
}


void Cell::SetMergedItemSetAndBroadcast(const SfxItemSet& rSet, bool bClearAllItems)
{
    if( mpProperties )
    {
        mpProperties->SetMergedItemSetAndBroadcast(rSet, bClearAllItems);
        notifyModified();
    }
}


sal_Int32 Cell::calcPreferredWidth( const Size aSize )
{
    if ( !hasText() )
        return getMinimumWidth();

    Outliner& rOutliner=static_cast< SdrTableObj& >( GetObject() ).ImpGetDrawOutliner();
    rOutliner.SetPaperSize(aSize);
    rOutliner.SetUpdateLayout(true);
    ForceOutlinerParaObject( OutlinerMode::TextObject );

    if( GetOutlinerParaObject() )
        rOutliner.SetText(*GetOutlinerParaObject());

    sal_Int32 nPreferredWidth = const_cast<EditEngine&>(rOutliner.GetEditEngine()).CalcTextWidth();
    rOutliner.Clear();

    return GetTextLeftDistance() + GetTextRightDistance() + nPreferredWidth;
}

sal_Int32 Cell::getMinimumWidth() const
{
    return GetTextLeftDistance() + GetTextRightDistance() + 100;
}


sal_Int32 Cell::getMinimumHeight()
{
    if( !mpProperties )
        return 0;

    SdrTableObj& rTableObj = dynamic_cast< SdrTableObj& >( GetObject() );
    sal_Int32 nMinimumHeight = 0;

    tools::Rectangle aTextRect;
    TakeTextAnchorRect( aTextRect );
    Size aSize( aTextRect.GetSize() );
    aSize.setHeight(0x0FFFFFFF );

    SdrOutliner* pEditOutliner = rTableObj.GetCellTextEditOutliner( *this );
    if(pEditOutliner)
    {
        pEditOutliner->SetMaxAutoPaperSize(aSize);
        nMinimumHeight = pEditOutliner->GetTextHeight()+1;
    }
    else
    {
        Outliner& rOutliner=rTableObj.ImpGetDrawOutliner();
        rOutliner.SetPaperSize(aSize);
        ForceOutlinerParaObject( OutlinerMode::TextObject );

        if( GetOutlinerParaObject() )
        {
            rOutliner.SetFixedCellHeight(
                GetItemSet().Get(SDRATTR_TEXT_USEFIXEDCELLHEIGHT).GetValue());
            rOutliner.SetText(*GetOutlinerParaObject());
        }

        rOutliner.SetUpdateLayout(true);
        nMinimumHeight=rOutliner.GetTextHeight()+1;

        // cleanup outliner
        rOutliner.Clear();
        rOutliner.SetFixedCellHeight(false);
    }

    nMinimumHeight += GetTextUpperDistance() + GetTextLowerDistance();
    return nMinimumHeight;
}


tools::Long Cell::GetTextLeftDistance() const
{
    return GetItemSet().Get(SDRATTR_TEXT_LEFTDIST).GetValue();
}


tools::Long Cell::GetTextRightDistance() const
{
    return GetItemSet().Get(SDRATTR_TEXT_RIGHTDIST).GetValue();
}


tools::Long Cell::GetTextUpperDistance() const
{
    return GetItemSet().Get(SDRATTR_TEXT_UPPERDIST).GetValue();
}


tools::Long Cell::GetTextLowerDistance() const
{
    return GetItemSet().Get(SDRATTR_TEXT_LOWERDIST).GetValue();
}


SdrTextVertAdjust Cell::GetTextVerticalAdjust() const
{
    return GetItemSet().Get(SDRATTR_TEXT_VERTADJUST).GetValue();
}


SdrTextHorzAdjust Cell::GetTextHorizontalAdjust() const
{
    return GetItemSet().Get(SDRATTR_TEXT_HORZADJUST).GetValue();
}


void Cell::SetOutlinerParaObject( std::optional<OutlinerParaObject> pTextObject )
{
    bool bNullTextObject = !pTextObject;
    SdrText::SetOutlinerParaObject( std::move(pTextObject) );
    maSelection.start.nPara = EE_PARA_MAX;

    if( bNullTextObject )
        ForceOutlinerParaObject( OutlinerMode::TextObject );
}


void Cell::AddUndo()
{
    SdrObject& rObj = GetObject();

    if( rObj.IsInserted() && rObj.getSdrModelFromSdrObject().IsUndoEnabled() )
    {
        CellRef xCell( this );
        rObj.getSdrModelFromSdrObject().AddUndo( std::make_unique<CellUndo>( &rObj, xCell ) );

        // Undo action for the after-text-edit-ended stack.
        SdrTableObj* pTableObj = dynamic_cast<sdr::table::SdrTableObj*>(&rObj);
        if (pTableObj && pTableObj->IsTextEditActive())
            pTableObj->AddUndo(new CellUndo(pTableObj, xCell));
    }
}

sdr::properties::CellProperties* Cell::CloneProperties( SdrObject& rNewObj, Cell& rNewCell )
{
    if (!mpProperties)
        return nullptr;
    return new sdr::properties::CellProperties( *mpProperties, rNewObj, &rNewCell );
}


// XInterface


Any SAL_CALL Cell::queryInterface( const Type & rType )
{
    if( rType == cppu::UnoType<XMergeableCell>::get() )
        return Any( Reference< XMergeableCell >( this ) );

    if( rType == cppu::UnoType<XCell>::get() )
        return Any( Reference< XCell >( this ) );

    if( rType == cppu::UnoType<XLayoutConstrains>::get() )
        return Any( Reference< XLayoutConstrains >( this ) );

    if( rType == cppu::UnoType<XEventListener>::get() )
        return Any( Reference< XEventListener >( this ) );

    Any aRet( SvxUnoTextBase::queryAggregation( rType ) );
    if( aRet.hasValue() )
        return aRet;

    return ::cppu::OWeakObject::queryInterface( rType );
}


void SAL_CALL Cell::acquire() noexcept
{
    SdrText::acquire();
}


void SAL_CALL Cell::release() noexcept
{
    SdrText::release();
}


// XTypeProvider


Sequence< Type > SAL_CALL Cell::getTypes(  )
{
    return comphelper::concatSequences( SvxUnoTextBase::getTypes(),
        std::initializer_list<Type>{
            cppu::UnoType<XMergeableCell>::get(),
            cppu::UnoType<XLayoutConstrains>::get() });
}


Sequence< sal_Int8 > SAL_CALL Cell::getImplementationId(  )
{
    return css::uno::Sequence<sal_Int8>();
}

// XLayoutConstrains
css::awt::Size SAL_CALL Cell::getMinimumSize()
{
    return css::awt::Size( getMinimumWidth(),  getMinimumHeight() );
}


css::awt::Size SAL_CALL Cell::getPreferredSize()
{
    return getMinimumSize();
}


css::awt::Size SAL_CALL Cell::calcAdjustedSize( const css::awt::Size& aNewSize )
{
    return aNewSize;
}


// XMergeableCell


sal_Int32 SAL_CALL Cell::getRowSpan()
{
    return mnRowSpan;
}


sal_Int32 SAL_CALL Cell::getColumnSpan()
{
    return mnColSpan;
}


sal_Bool SAL_CALL Cell::isMerged()
{
    return mbMerged;
}


// XCell


OUString SAL_CALL Cell::getFormula(  )
{
    return msFormula;
}


void SAL_CALL Cell::setFormula( const OUString& aFormula )
{
    if( msFormula != aFormula )
    {
        msFormula = aFormula;
    }
}


double SAL_CALL Cell::getValue(  )
{
    return mfValue;
}


void SAL_CALL Cell::setValue( double nValue )
{
    if( mfValue != nValue )
    {
        mfValue = nValue;
        mnCellContentType = CellContentType_VALUE;
    }
}


CellContentType SAL_CALL Cell::getType()
{
    return mnCellContentType;
}


sal_Int32 SAL_CALL Cell::getError(  )
{
    return mnError;
}


// XPropertySet


Any Cell::GetAnyForItem( SfxItemSet const & aSet, const SfxItemPropertyMapEntry* pMap )
{
    Any aAny( SvxItemPropertySet_getPropertyValue( pMap, aSet ) );

    if( pMap->aType != aAny.getValueType() )
    {
        // since the sfx uint16 item now exports a sal_Int32, we may have to fix this here
        if( ( pMap->aType == ::cppu::UnoType<sal_Int16>::get()) && aAny.getValueType() == ::cppu::UnoType<sal_Int32>::get() )
        {
            sal_Int32 nValue = 0;
            aAny >>= nValue;
            aAny <<= static_cast<sal_Int16>(nValue);
        }
        else
        {
            OSL_FAIL("GetAnyForItem() Returnvalue has wrong Type!" );
        }
    }

    return aAny;
}

Reference< XPropertySetInfo > SAL_CALL Cell::getPropertySetInfo()
{
    return mpPropSet->getPropertySetInfo();
}


void SAL_CALL Cell::setPropertyValue( const OUString& rPropertyName, const Any& rValue )
{
    ::SolarMutexGuard aGuard;

    if(mpProperties == nullptr)
        throw DisposedException();

    const SfxItemPropertyMapEntry* pMap = mpPropSet->getPropertyMapEntry(rPropertyName);
    if( pMap )
    {
        if( (pMap->nFlags & PropertyAttribute::READONLY ) != 0 )
            throw PropertyVetoException();

        switch( pMap->nWID )
        {
        case OWN_ATTR_STYLE:
        {
            Reference< XStyle > xStyle;
            if( !( rValue >>= xStyle ) )
                throw IllegalArgumentException();

            SfxUnoStyleSheet* pStyle = SfxUnoStyleSheet::getUnoStyleSheet(xStyle);
            SetStyleSheet( pStyle, true );
            return;
        }
        case OWN_ATTR_TABLEBORDER:
        {
            auto pBorder = o3tl::tryAccess<TableBorder>(rValue);
            if(!pBorder)
                break;

            SvxBoxItem aBox( SDRATTR_TABLE_BORDER );
            SvxBoxInfoItem aBoxInfo( SDRATTR_TABLE_BORDER_INNER );
            SvxBorderLine aLine;

            bool bSet = SvxBoxItem::LineToSvxLine(pBorder->TopLine, aLine, false);
            aBox.SetLine(bSet ? &aLine : nullptr, SvxBoxItemLine::TOP);
            aBoxInfo.SetValid(SvxBoxInfoItemValidFlags::TOP, pBorder->IsTopLineValid);

            bSet = SvxBoxItem::LineToSvxLine(pBorder->BottomLine, aLine, false);
            aBox.SetLine(bSet ? &aLine : nullptr, SvxBoxItemLine::BOTTOM);
            aBoxInfo.SetValid(SvxBoxInfoItemValidFlags::BOTTOM, pBorder->IsBottomLineValid);

            bSet = SvxBoxItem::LineToSvxLine(pBorder->LeftLine, aLine, false);
            aBox.SetLine(bSet ? &aLine : nullptr, SvxBoxItemLine::LEFT);
            aBoxInfo.SetValid(SvxBoxInfoItemValidFlags::LEFT, pBorder->IsLeftLineValid);

            bSet = SvxBoxItem::LineToSvxLine(pBorder->RightLine, aLine, false);
            aBox.SetLine(bSet ? &aLine : nullptr, SvxBoxItemLine::RIGHT);
            aBoxInfo.SetValid(SvxBoxInfoItemValidFlags::RIGHT, pBorder->IsRightLineValid);

            bSet = SvxBoxItem::LineToSvxLine(pBorder->HorizontalLine, aLine, false);
            aBoxInfo.SetLine(bSet ? &aLine : nullptr, SvxBoxInfoItemLine::HORI);
            aBoxInfo.SetValid(SvxBoxInfoItemValidFlags::HORI, pBorder->IsHorizontalLineValid);

            bSet = SvxBoxItem::LineToSvxLine(pBorder->VerticalLine, aLine, false);
            aBoxInfo.SetLine(bSet ? &aLine : nullptr, SvxBoxInfoItemLine::VERT);
            aBoxInfo.SetValid(SvxBoxInfoItemValidFlags::VERT, pBorder->IsVerticalLineValid);

            aBox.SetAllDistances(pBorder->Distance); //TODO
            aBoxInfo.SetValid(SvxBoxInfoItemValidFlags::DISTANCE, pBorder->IsDistanceValid);

            mpProperties->SetObjectItem(aBox);
            mpProperties->SetObjectItem(aBoxInfo);
            return;
        }
        case OWN_ATTR_FILLBMP_MODE:
        {
            BitmapMode eMode;
            if(!(rValue >>= eMode) )
            {
                sal_Int32 nMode = 0;
                if(!(rValue >>= nMode))
                    throw IllegalArgumentException();

                eMode = static_cast<BitmapMode>(nMode);
            }

            mpProperties->SetObjectItem( XFillBmpStretchItem( eMode == BitmapMode_STRETCH ) );
            mpProperties->SetObjectItem( XFillBmpTileItem( eMode == BitmapMode_REPEAT ) );
            return;
        }
        case SDRATTR_TABLE_TEXT_ROTATION:
        {
            sal_Int32 nRotVal = 0;
            if (!(rValue >>= nRotVal))
                throw IllegalArgumentException();

            if (nRotVal != 27000 && nRotVal != 9000 && nRotVal != 0)
                throw IllegalArgumentException();

            mpProperties->SetObjectItem(SvxTextRotateItem(Degree10(nRotVal/10), SDRATTR_TABLE_TEXT_ROTATION));
            return;
        }
        case SDRATTR_TABLE_CELL_GRABBAG:
        {
            if (mpGrabBagItem == nullptr)
                mpGrabBagItem.reset(new SfxGrabBagItem);

            mpGrabBagItem->PutValue(rValue, 0);
            return;
        }
        default:
        {
            SfxItemSet aSet(GetObject().getSdrModelFromSdrObject().GetItemPool(), pMap->nWID, pMap->nWID);
            aSet.Put(mpProperties->GetItem(pMap->nWID));

            bool bSpecial = false;

            switch( pMap->nWID )
            {
                case XATTR_FILLBITMAP:
                case XATTR_FILLGRADIENT:
                case XATTR_FILLHATCH:
                case XATTR_FILLFLOATTRANSPARENCE:
                case XATTR_LINEEND:
                case XATTR_LINESTART:
                case XATTR_LINEDASH:
                {
                    if( pMap->nMemberId == MID_NAME )
                    {
                        OUString aApiName;
                        if( rValue >>= aApiName )
                        {
                            if(SvxShape::SetFillAttribute(pMap->nWID, aApiName, aSet, &GetObject().getSdrModelFromSdrObject()))
                                bSpecial = true;
                        }
                    }
                }
                break;
            }

            if( !bSpecial )
            {

                if( !SvxUnoTextRangeBase::SetPropertyValueHelper( pMap, rValue, aSet ))
                {
                    if( aSet.GetItemState( pMap->nWID ) != SfxItemState::SET )
                    {
                        // fetch the default from ItemPool
                        if(SfxItemPool::IsWhich(pMap->nWID))
                            aSet.Put(GetObject().getSdrModelFromSdrObject().GetItemPool().GetUserOrPoolDefaultItem(pMap->nWID));
                    }

                    if( aSet.GetItemState( pMap->nWID ) == SfxItemState::SET )
                    {
                        SvxItemPropertySet_setPropertyValue( pMap, rValue, aSet );
                    }
                }
            }

            GetObject().getSdrModelFromSdrObject().SetChanged();
            mpProperties->SetMergedItemSetAndBroadcast( aSet );
            return;
        }
        }
    }
    throw UnknownPropertyException( rPropertyName, getXWeak());
}


Any SAL_CALL Cell::getPropertyValue( const OUString& PropertyName )
{
    ::SolarMutexGuard aGuard;

    if(mpProperties == nullptr)
        throw DisposedException();

    const SfxItemPropertyMapEntry* pMap = mpPropSet->getPropertyMapEntry(PropertyName);
    if( pMap )
    {
        switch( pMap->nWID )
        {
        case OWN_ATTR_STYLE:
        {
            return Any( Reference< XStyle >( dynamic_cast< SfxUnoStyleSheet* >( GetStyleSheet() ) ) );
        }
        case OWN_ATTR_TABLEBORDER:
        {
            const SvxBoxInfoItem& rBoxInfoItem = mpProperties->GetItem(SDRATTR_TABLE_BORDER_INNER);
            const SvxBoxItem& rBox = mpProperties->GetItem(SDRATTR_TABLE_BORDER);

            TableBorder aTableBorder;
            aTableBorder.TopLine                = SvxBoxItem::SvxLineToLine(rBox.GetTop(), false);
            aTableBorder.IsTopLineValid         = rBoxInfoItem.IsValid(SvxBoxInfoItemValidFlags::TOP);
            aTableBorder.BottomLine             = SvxBoxItem::SvxLineToLine(rBox.GetBottom(), false);
            aTableBorder.IsBottomLineValid      = rBoxInfoItem.IsValid(SvxBoxInfoItemValidFlags::BOTTOM);
            aTableBorder.LeftLine               = SvxBoxItem::SvxLineToLine(rBox.GetLeft(), false);
            aTableBorder.IsLeftLineValid        = rBoxInfoItem.IsValid(SvxBoxInfoItemValidFlags::LEFT);
            aTableBorder.RightLine              = SvxBoxItem::SvxLineToLine(rBox.GetRight(), false);
            aTableBorder.IsRightLineValid       = rBoxInfoItem.IsValid(SvxBoxInfoItemValidFlags::RIGHT );
            aTableBorder.HorizontalLine         = SvxBoxItem::SvxLineToLine(rBoxInfoItem.GetHori(), false);
            aTableBorder.IsHorizontalLineValid  = rBoxInfoItem.IsValid(SvxBoxInfoItemValidFlags::HORI);
            aTableBorder.VerticalLine           = SvxBoxItem::SvxLineToLine(rBoxInfoItem.GetVert(), false);
            aTableBorder.IsVerticalLineValid    = rBoxInfoItem.IsValid(SvxBoxInfoItemValidFlags::VERT);
            aTableBorder.Distance               = rBox.GetSmallestDistance();
            aTableBorder.IsDistanceValid        = rBoxInfoItem.IsValid(SvxBoxInfoItemValidFlags::DISTANCE);

            return Any( aTableBorder );
        }
        case OWN_ATTR_FILLBMP_MODE:
        {
            const XFillBmpStretchItem& rStretchItem = mpProperties->GetItem(XATTR_FILLBMP_STRETCH);
            const XFillBmpTileItem& rTileItem = mpProperties->GetItem(XATTR_FILLBMP_TILE);
            if( rTileItem.GetValue() )
            {
                return Any( BitmapMode_REPEAT );
            }
            else if( rStretchItem.GetValue() )
            {
                return Any(  BitmapMode_STRETCH );
            }
            else
            {
                return Any(  BitmapMode_NO_REPEAT );
            }
        }
        case SDRATTR_TABLE_TEXT_ROTATION:
        {
            const SvxTextRotateItem& rTextRotate = mpProperties->GetItem(SDRATTR_TABLE_TEXT_ROTATION);
            return Any(sal_Int32(to<Degree100>(rTextRotate.GetValue())));
        }
        case SDRATTR_TABLE_CELL_GRABBAG:
        {
            if (mpGrabBagItem != nullptr)
            {
                Any aGrabBagSequence;
                mpGrabBagItem->QueryValue(aGrabBagSequence);
                return aGrabBagSequence;
            }
            else
                return Any{css::uno::Sequence<css::beans::PropertyValue>()};
        }
        default:
        {
            SfxItemSet aSet(GetObject().getSdrModelFromSdrObject().GetItemPool(), pMap->nWID, pMap->nWID);
            aSet.Put(mpProperties->GetItem(pMap->nWID));

            Any aAny;
            if(!SvxUnoTextRangeBase::GetPropertyValueHelper( aSet, pMap, aAny ))
            {
                if(!aSet.Count())
                {
                    // fetch the default from ItemPool
                    if(SfxItemPool::IsWhich(pMap->nWID))
                        aSet.Put(GetObject().getSdrModelFromSdrObject().GetItemPool().GetUserOrPoolDefaultItem(pMap->nWID));
                }

                if( aSet.Count() )
                    aAny = GetAnyForItem( aSet, pMap );
            }

            return aAny;
        }
        }
    }
    throw UnknownPropertyException( PropertyName, getXWeak());
}


void SAL_CALL Cell::addPropertyChangeListener( const OUString& /*aPropertyName*/, const Reference< XPropertyChangeListener >& /*xListener*/ )
{
}


void SAL_CALL Cell::removePropertyChangeListener( const OUString& /*aPropertyName*/, const Reference< XPropertyChangeListener >& /*aListener*/ )
{
}


void SAL_CALL Cell::addVetoableChangeListener( const OUString& /*PropertyName*/, const Reference< XVetoableChangeListener >& /*aListener*/ )
{
}


void SAL_CALL Cell::removeVetoableChangeListener( const OUString& /*PropertyName*/, const Reference< XVetoableChangeListener >& /*aListener*/ )
{
}


// XMultiPropertySet


void SAL_CALL Cell::setPropertyValues( const Sequence< OUString >& aPropertyNames, const Sequence< Any >& aValues )
{
    ::SolarMutexGuard aSolarGuard;

    if(mpProperties == nullptr)
        throw DisposedException();

    const sal_Int32 nCount = aPropertyNames.getLength();
    if (nCount != aValues.getLength())
        throw css::lang::IllegalArgumentException(u"lengths do not match"_ustr,
                                                  getXWeak(), -1);

    const OUString* pNames = aPropertyNames.getConstArray();
    const Any* pValues = aValues.getConstArray();

    for( sal_Int32 nIdx = 0; nIdx < nCount; nIdx++, pNames++, pValues++ )
    {
        try
        {
            setPropertyValue( *pNames, *pValues );
        }
        catch( UnknownPropertyException& )
        {
            TOOLS_WARN_EXCEPTION("svx.table", "unknown property!");
        }
        catch( Exception& )
        {
            TOOLS_WARN_EXCEPTION("svx.table", "");
        }
    }
}


Sequence< Any > SAL_CALL Cell::getPropertyValues( const Sequence< OUString >& aPropertyNames )
{
    ::SolarMutexGuard aSolarGuard;

    if(mpProperties == nullptr)
        throw DisposedException();

    const sal_Int32 nCount = aPropertyNames.getLength();
    Sequence< Any > aRet( nCount );
    Any* pValue = aRet.getArray();

    for( const OUString& rName : aPropertyNames )
    {
        try
        {
            *pValue = getPropertyValue( rName );
        }
        catch( UnknownPropertyException& )
        {
            TOOLS_WARN_EXCEPTION("svx.table", "unknown property!");
        }
        catch( Exception& )
        {
            TOOLS_WARN_EXCEPTION("svx.table", "");
        }
        pValue++;
    }

    return aRet;
}


void SAL_CALL Cell::addPropertiesChangeListener( const Sequence< OUString >& /*aPropertyNames*/, const Reference< XPropertiesChangeListener >& /*xListener*/ )
{
}


void SAL_CALL Cell::removePropertiesChangeListener( const Reference< XPropertiesChangeListener >& /*xListener*/ )
{
}


void SAL_CALL Cell::firePropertiesChangeEvent( const Sequence< OUString >& /*aPropertyNames*/, const Reference< XPropertiesChangeListener >& /*xListener*/ )
{
}


// XPropertyState


PropertyState SAL_CALL Cell::getPropertyState( const OUString& PropertyName )
{
    ::SolarMutexGuard aGuard;

    if(mpProperties == nullptr)
        throw DisposedException();

    const SfxItemPropertyMapEntry* pMap = mpPropSet->getPropertyMapEntry(PropertyName);

    if( pMap )
    {
        PropertyState eState;
        switch( pMap->nWID )
        {
        case OWN_ATTR_FILLBMP_MODE:
        {
            const SfxItemSet& rSet = mpProperties->GetMergedItemSet();

            const bool bStretch = rSet.GetItemState( XATTR_FILLBMP_STRETCH, false ) == SfxItemState::SET;
            const bool bTile = rSet.GetItemState( XATTR_FILLBMP_TILE, false ) == SfxItemState::SET;
            if( bStretch || bTile )
            {
                eState = PropertyState_DIRECT_VALUE;
            }
            else
            {
                eState = PropertyState_DEFAULT_VALUE;
            }
            break;
        }
        case OWN_ATTR_STYLE:
        {
            return PropertyState_DIRECT_VALUE;
        }
        case OWN_ATTR_TABLEBORDER:
        {
            const SfxItemSet& rSet = mpProperties->GetMergedItemSet();
            if( (rSet.GetItemState( SDRATTR_TABLE_BORDER_INNER, false ) == SfxItemState::DEFAULT) && (rSet.GetItemState( SDRATTR_TABLE_BORDER, false ) == SfxItemState::DEFAULT) )
                return PropertyState_DEFAULT_VALUE;

            return PropertyState_DIRECT_VALUE;
        }
        default:
        {
            const SfxItemSet& rSet = mpProperties->GetMergedItemSet();

            switch( rSet.GetItemState( pMap->nWID, false ) )
            {
            case SfxItemState::SET:
                eState = PropertyState_DIRECT_VALUE;
                break;
            case SfxItemState::DEFAULT:
                eState = PropertyState_DEFAULT_VALUE;
                break;
            default:
                eState = PropertyState_AMBIGUOUS_VALUE;
                break;
            }

            // if an item is set, this doesn't mean we want it :)
            if( PropertyState_DIRECT_VALUE == eState )
            {
                switch( pMap->nWID )
                {
                // the following items are disabled by changing the
                // fill style or the line style. so there is no need
                // to export items without names which should be empty
                case XATTR_FILLBITMAP:
                case XATTR_FILLGRADIENT:
                case XATTR_FILLHATCH:
                case XATTR_LINEDASH:
                    {
                        const NameOrIndex* pItem = rSet.GetItem<NameOrIndex>(pMap->nWID);
                        if( ( pItem == nullptr ) || pItem->GetName().isEmpty() )
                            eState = PropertyState_DEFAULT_VALUE;
                    }
                    break;

                // #i36115#
                // If e.g. the LineStart is on NONE and thus the string has length 0, it still
                // may be a hard attribute covering the set LineStart of the parent (Style).
                // #i37644#
                // same is for fill float transparency
                case XATTR_LINEEND:
                case XATTR_LINESTART:
                case XATTR_FILLFLOATTRANSPARENCE:
                    {
                        const NameOrIndex* pItem = rSet.GetItem<NameOrIndex>(pMap->nWID);
                        if( pItem == nullptr )
                            eState = PropertyState_DEFAULT_VALUE;
                    }
                    break;
                case XATTR_FILLCOLOR:
                    if (pMap->nMemberId == MID_COLOR_THEME_INDEX)
                    {
                        auto const* pColor = rSet.GetItem<XFillColorItem>(pMap->nWID);
                        if (!pColor->getComplexColor().isValidThemeType())
                        {
                            eState = PropertyState_DEFAULT_VALUE;
                        }
                    }
                    else if (pMap->nMemberId == MID_COLOR_LUM_MOD)
                    {
                        auto const* pColor = rSet.GetItem<XFillColorItem>(pMap->nWID);
                        sal_Int16 nLumMod = 10000;
                        for (auto const& rTransform : pColor->getComplexColor().getTransformations())
                        {
                            if (rTransform.meType == model::TransformationType::LumMod)
                                nLumMod = rTransform.mnValue;
                        }
                        if (nLumMod == 10000)
                        {
                            eState = PropertyState_DEFAULT_VALUE;
                        }
                    }
                    else if (pMap->nMemberId == MID_COLOR_LUM_OFF)
                    {
                        auto const* pColor = rSet.GetItem<XFillColorItem>(pMap->nWID);
                        sal_Int16 nLumOff = 0;
                        for (auto const& rTransform : pColor->getComplexColor().getTransformations())
                        {
                            if (rTransform.meType == model::TransformationType::LumOff)
                                nLumOff = rTransform.mnValue;
                        }
                        if (nLumOff == 0)
                        {
                            eState = PropertyState_DEFAULT_VALUE;
                        }
                    }
                    else if (pMap->nMemberId == MID_COMPLEX_COLOR)
                    {
                        auto const* pColor = rSet.GetItem<XFillColorItem>(pMap->nWID);
                        if (pColor->getComplexColor().getType() == model::ColorType::Unused)
                        {
                            eState = PropertyState_DEFAULT_VALUE;
                        }
                    }
                    break;
                case XATTR_LINECOLOR:
                    if (pMap->nMemberId == MID_COMPLEX_COLOR)
                    {
                        auto const* pColor = rSet.GetItem<XLineColorItem>(pMap->nWID);
                        if (pColor->getComplexColor().getType() == model::ColorType::Unused)
                        {
                            eState = PropertyState_DEFAULT_VALUE;
                        }
                    }
                    break;
                }
            }
        }
        }
        return eState;
    }
    throw UnknownPropertyException(PropertyName);
}


Sequence< PropertyState > SAL_CALL Cell::getPropertyStates( const Sequence< OUString >& aPropertyName )
{
    ::SolarMutexGuard aGuard;

    if(mpProperties == nullptr)
        throw DisposedException();

    const sal_Int32 nCount = aPropertyName.getLength();
    Sequence< PropertyState > aRet( nCount );

    std::transform(aPropertyName.begin(), aPropertyName.end(), aRet.getArray(),
        [this](const OUString& rName) -> PropertyState {
            try
            {
                return getPropertyState( rName );
            }
            catch( Exception& )
            {
                return PropertyState_AMBIGUOUS_VALUE;
            }
        });

    return aRet;
}


void SAL_CALL Cell::setPropertyToDefault( const OUString& PropertyName )
{
    ::SolarMutexGuard aGuard;

    if(mpProperties == nullptr)
        throw DisposedException();

    const SfxItemPropertyMapEntry* pMap = mpPropSet->getPropertyMapEntry(PropertyName);
    if( pMap )
    {
        switch( pMap->nWID )
        {
        case OWN_ATTR_FILLBMP_MODE:
        {
            mpProperties->ClearObjectItem( XATTR_FILLBMP_STRETCH );
            mpProperties->ClearObjectItem( XATTR_FILLBMP_TILE );
            break;
        }
        case OWN_ATTR_STYLE:
            break;

        case OWN_ATTR_TABLEBORDER:
        {
            mpProperties->ClearObjectItem( SDRATTR_TABLE_BORDER_INNER );
            mpProperties->ClearObjectItem( SDRATTR_TABLE_BORDER );
            break;
        }

        default:
        {
            mpProperties->ClearObjectItem( pMap->nWID );
        }
        }

        GetObject().getSdrModelFromSdrObject().SetChanged();
        return;
    }
    throw UnknownPropertyException( PropertyName, getXWeak());
}


Any SAL_CALL Cell::getPropertyDefault( const OUString& aPropertyName )
{
    ::SolarMutexGuard aGuard;

    if(mpProperties == nullptr)
        throw DisposedException();

    const SfxItemPropertyMapEntry* pMap = mpPropSet->getPropertyMapEntry(aPropertyName);
    if( pMap )
    {
        switch( pMap->nWID )
        {
        case OWN_ATTR_FILLBMP_MODE:
            return Any(  BitmapMode_NO_REPEAT );

        case OWN_ATTR_STYLE:
        {
            Reference< XStyle > xStyle;
            return Any( xStyle );
        }

        case OWN_ATTR_TABLEBORDER:
        {
            TableBorder aBorder;
            return Any( aBorder );
        }

        default:
        {
            if( SfxItemPool::IsWhich(pMap->nWID) )
            {
                SfxItemSet aSet(GetObject().getSdrModelFromSdrObject().GetItemPool(), pMap->nWID, pMap->nWID);
                aSet.Put(GetObject().getSdrModelFromSdrObject().GetItemPool().GetUserOrPoolDefaultItem(pMap->nWID));
                return GetAnyForItem( aSet, pMap );
            }
        }
        }
    }
    throw UnknownPropertyException( aPropertyName, getXWeak());
}


// XMultiPropertyStates


void SAL_CALL Cell::setAllPropertiesToDefault()
{
    mpProperties.reset(new sdr::properties::CellProperties( static_cast< SdrTableObj& >( GetObject() ), this ));

    SdrOutliner& rOutliner = GetObject().ImpGetDrawOutliner();

    OutlinerParaObject* pParaObj = GetOutlinerParaObject();
    if( !pParaObj )
        return;

    rOutliner.SetText(*pParaObj);
    sal_Int32 nParaCount(rOutliner.GetParagraphCount());

    if(nParaCount)
    {
        auto aSelection = ESelection::All();
        rOutliner.RemoveAttribs(aSelection, true, 0);

        std::optional<OutlinerParaObject> pTemp = rOutliner.CreateParaObject(0, nParaCount);
        rOutliner.Clear();

        SetOutlinerParaObject(std::move(pTemp));
    }
}


void SAL_CALL Cell::setPropertiesToDefault( const Sequence< OUString >& aPropertyNames )
{
    for(const OUString& rName : aPropertyNames)
        setPropertyToDefault( rName );
}


Sequence< Any > SAL_CALL Cell::getPropertyDefaults( const Sequence< OUString >& aPropertyNames )
{
    sal_Int32 nCount = aPropertyNames.getLength();
    Sequence< Any > aDefaults( nCount );

    std::transform(aPropertyNames.begin(), aPropertyNames.end(), aDefaults.getArray(),
        [this](const OUString& rName) -> Any { return getPropertyDefault(rName); });

    return aDefaults;
}


// XText


void SAL_CALL Cell::insertTextContent( const Reference< XTextRange >& xRange, const Reference< XTextContent >& xContent, sal_Bool bAbsorb )
{
    SvxUnoTextBase::insertTextContent( xRange, xContent, bAbsorb );
    notifyModified();
}


void SAL_CALL Cell::removeTextContent( const Reference< XTextContent >& xContent )
{
    SvxUnoTextBase::removeTextContent( xContent );
    notifyModified();
}


// XSimpleText


void SAL_CALL Cell::insertString( const Reference< XTextRange >& xRange, const OUString& aString, sal_Bool bAbsorb )
{
    SvxUnoTextBase::insertString( xRange, aString, bAbsorb );
    notifyModified();
}


void SAL_CALL Cell::insertControlCharacter( const Reference< XTextRange >& xRange, sal_Int16 nControlCharacter, sal_Bool bAbsorb )
{
    SvxUnoTextBase::insertControlCharacter( xRange, nControlCharacter, bAbsorb );
    notifyModified();
}


// XTextRange


OUString SAL_CALL Cell::getString(  )
{
    maSelection.start.nPara = EE_PARA_MAX;
    return SvxUnoTextBase::getString();
}


void SAL_CALL Cell::setString( const OUString& aString )
{
    SvxUnoTextBase::setString( aString );
    notifyModified();
}

// XEventListener
void SAL_CALL Cell::disposing( const EventObject& /*Source*/ )
{
    mxTable.clear();
    dispose();
}

void Cell::dumpAsXml(xmlTextWriterPtr pWriter, sal_Int32 nRow, sal_Int32 nCol) const
{
    (void)xmlTextWriterStartElement(pWriter, BAD_CAST("Cell"));
    (void)xmlTextWriterWriteFormatAttribute(pWriter, BAD_CAST("row"), "%" SAL_PRIdINT32, nRow);
    (void)xmlTextWriterWriteFormatAttribute(pWriter, BAD_CAST("col"), "%" SAL_PRIdINT32, nCol);
    SdrText::dumpAsXml(pWriter);
    //SvxUnoTextBase::dumpAsXml(pWriter);
    //mpPropSet->dumpAsXml(pWriter);
    mpProperties->dumpAsXml(pWriter);
    (void)xmlTextWriterEndElement(pWriter);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
