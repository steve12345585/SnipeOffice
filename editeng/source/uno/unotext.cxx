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

#include <vcl/svapp.hxx>
#include <com/sun/star/text/ControlCharacter.hpp>
#include <com/sun/star/text/XTextField.hpp>
#include <com/sun/star/text/TextRangeSelection.hpp>
#include <com/sun/star/lang/Locale.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/container/XNameContainer.hpp>

#include <svl/itemset.hxx>
#include <svl/itempool.hxx>
#include <svl/eitem.hxx>
#include <tools/debug.hxx>

#include <editeng/unoprnms.hxx>
#include <editeng/unotext.hxx>
#include <editeng/unoedsrc.hxx>
#include <editeng/unonrule.hxx>
#include <editeng/unofdesc.hxx>
#include <editeng/unofield.hxx>
#include <editeng/flditem.hxx>
#include <editeng/numitem.hxx>
#include <editeng/editeng.hxx>
#include <editeng/outliner.hxx>
#include <editeng/unoipset.hxx>
#include <editeng/colritem.hxx>
#include <comphelper/sequence.hxx>
#include <comphelper/servicehelper.hxx>
#include <cppuhelper/supportsservice.hxx>

#include <editeng/unonames.hxx>

#include <initializer_list>
#include <memory>
#include <string_view>

using namespace ::cppu;
using namespace ::com::sun::star;

namespace {

ESelection toESelection(const text::TextRangeSelection& rSel)
{
    ESelection aESel;
    aESel.start.nPara = rSel.Start.Paragraph;
    aESel.start.nIndex = rSel.Start.PositionInParagraph;
    aESel.end.nPara = rSel.End.Paragraph;
    aESel.end.nIndex = rSel.End.PositionInParagraph;
    return aESel;
}

}

#define QUERYINT( xint ) \
    if( rType == cppu::UnoType<xint>::get() ) \
        return uno::Any(uno::Reference< xint >(this))

const SvxItemPropertySet* ImplGetSvxUnoOutlinerTextCursorSvxPropertySet()
{
    static SvxItemPropertySet aTextCursorSvxPropertySet( ImplGetSvxUnoOutlinerTextCursorPropertyMap(), EditEngine::GetGlobalItemPool() );
    return &aTextCursorSvxPropertySet;
}

std::span<const SfxItemPropertyMapEntry> ImplGetSvxTextPortionPropertyMap()
{
    // Propertymap for an Outliner Text
    static const SfxItemPropertyMapEntry aSvxTextPortionPropertyMap[] =
    {
        SVX_UNOEDIT_CHAR_PROPERTIES,
        SVX_UNOEDIT_FONT_PROPERTIES,
        SVX_UNOEDIT_OUTLINER_PROPERTIES,
        SVX_UNOEDIT_PARA_PROPERTIES,
        { u"TextField"_ustr,                     EE_FEATURE_FIELD,   cppu::UnoType<text::XTextField>::get(),   beans::PropertyAttribute::READONLY, 0 },
        { u"TextPortionType"_ustr,               WID_PORTIONTYPE,    ::cppu::UnoType<OUString>::get(), beans::PropertyAttribute::READONLY, 0 },
        { u"TextUserDefinedAttributes"_ustr,     EE_CHAR_XMLATTRIBS,     cppu::UnoType<css::container::XNameContainer>::get(),        0,     0},
        { u"ParaUserDefinedAttributes"_ustr,     EE_PARA_XMLATTRIBS,     cppu::UnoType<css::container::XNameContainer>::get(),        0,     0},
    };
    return aSvxTextPortionPropertyMap;
}
const SvxItemPropertySet* ImplGetSvxTextPortionSvxPropertySet()
{
    static SvxItemPropertySet aSvxTextPortionPropertySet( ImplGetSvxTextPortionPropertyMap(), EditEngine::GetGlobalItemPool() );
    return &aSvxTextPortionPropertySet;
}

static const SfxItemPropertySet* ImplGetSvxTextPortionSfxPropertySet()
{
    static SfxItemPropertySet aSvxTextPortionSfxPropertySet( ImplGetSvxTextPortionPropertyMap() );
    return &aSvxTextPortionSfxPropertySet;
}

std::span<const SfxItemPropertyMapEntry> ImplGetSvxUnoOutlinerTextCursorPropertyMap()
{
    // Propertymap for an Outliner Text
    static const SfxItemPropertyMapEntry aSvxUnoOutlinerTextCursorPropertyMap[] =
    {
        SVX_UNOEDIT_CHAR_PROPERTIES,
        SVX_UNOEDIT_FONT_PROPERTIES,
        SVX_UNOEDIT_OUTLINER_PROPERTIES,
        SVX_UNOEDIT_PARA_PROPERTIES,
        { u"TextUserDefinedAttributes"_ustr,         EE_CHAR_XMLATTRIBS,     cppu::UnoType<css::container::XNameContainer>::get(),        0,     0},
        { u"ParaUserDefinedAttributes"_ustr,         EE_PARA_XMLATTRIBS,     cppu::UnoType<css::container::XNameContainer>::get(),        0,     0},
    };

    return aSvxUnoOutlinerTextCursorPropertyMap;
}
static const SfxItemPropertySet* ImplGetSvxUnoOutlinerTextCursorSfxPropertySet()
{
    static SfxItemPropertySet aTextCursorSfxPropertySet( ImplGetSvxUnoOutlinerTextCursorPropertyMap() );
    return &aTextCursorSfxPropertySet;
}


// helper for Item/Property conversion


void GetSelection( struct ESelection& rSel, SvxTextForwarder const * pForwarder ) noexcept
{
    DBG_ASSERT( pForwarder, "I need a valid SvxTextForwarder!" );
    if( pForwarder )
    {
        sal_Int32 nParaCount = pForwarder->GetParagraphCount();
        if(nParaCount>0)
            nParaCount--;

        rSel = ESelection( 0,0, nParaCount, pForwarder->GetTextLen( nParaCount ));
    }
}

void CheckSelection( struct ESelection& rSel, SvxTextForwarder const * pForwarder ) noexcept
{
    DBG_ASSERT( pForwarder, "I need a valid SvxTextForwarder!" );
    if( !pForwarder )
        return;

    if (rSel.start.nPara == EE_PARA_MAX)
    {
        ::GetSelection( rSel, pForwarder );
    }
    else
    {
        ESelection aMaxSelection;
        GetSelection( aMaxSelection, pForwarder );

        // check start position
        if (rSel.start.nPara < aMaxSelection.start.nPara)
        {
            rSel.start = aMaxSelection.start;
        }
        else if (rSel.start.nPara > aMaxSelection.end.nPara)
        {
            rSel.start = aMaxSelection.end;
        }
        else if (rSel.start.nIndex > pForwarder->GetTextLen(rSel.start.nPara))
        {
            rSel.start.nIndex = pForwarder->GetTextLen(rSel.start.nPara);
        }

        // check end position
        if (rSel.end.nPara < aMaxSelection.start.nPara)
        {
            rSel.end = aMaxSelection.start;
        }
        else if (rSel.end.nPara > aMaxSelection.end.nPara)
        {
            rSel.end = aMaxSelection.end;
        }
        else if (rSel.end.nIndex > pForwarder->GetTextLen(rSel.end.nPara))
        {
            rSel.end.nIndex = pForwarder->GetTextLen(rSel.end.nPara);
        }
    }
}

static void CheckSelection( struct ESelection& rSel, SvxEditSource *pEdit ) noexcept
{
    if (!pEdit)
        return;
    CheckSelection( rSel, pEdit->GetTextForwarder() );
}




UNO3_GETIMPLEMENTATION_IMPL( SvxUnoTextRangeBase );

SvxUnoTextRangeBase::SvxUnoTextRangeBase(const SvxItemPropertySet* _pSet)
    : mpPropSet(_pSet)
{
}

SvxUnoTextRangeBase::SvxUnoTextRangeBase(const SvxEditSource* pSource, const SvxItemPropertySet* _pSet)
: mpPropSet(_pSet)
{
    SolarMutexGuard aGuard;

    assert(pSource && "SvxUnoTextRangeBase: I need a valid SvxEditSource!");

    mpEditSource = pSource->Clone();
    if (mpEditSource != nullptr)
    {
        ESelection aSelection;
        ::GetSelection( aSelection, mpEditSource->GetTextForwarder() );
        SetSelection( aSelection );

        mpEditSource->addRange( this );
    }
}

SvxUnoTextRangeBase::SvxUnoTextRangeBase(const SvxUnoTextRangeBase& rRange)
:   text::XTextRange()
,   beans::XPropertySet()
,   beans::XMultiPropertySet()
,   beans::XMultiPropertyStates()
,   beans::XPropertyState()
,   lang::XServiceInfo()
,   text::XTextRangeCompare()
,   lang::XUnoTunnel()
,   osl::DebugBase<SvxUnoTextRangeBase>()
,   mpPropSet(rRange.getPropertySet())
{
    SolarMutexGuard aGuard;

    if (rRange.mpEditSource)
        mpEditSource = rRange.mpEditSource->Clone();

    SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : nullptr;
    if( pForwarder )
    {
        maSelection  = rRange.maSelection;
        CheckSelection( maSelection, pForwarder );
    }

    if( mpEditSource )
        mpEditSource->addRange( this );
}

SvxUnoTextRangeBase::~SvxUnoTextRangeBase() noexcept
{
    if( mpEditSource )
        mpEditSource->removeRange( this );
}

void SvxUnoTextRangeBase::SetEditSource( SvxEditSource* pSource ) noexcept
{
    DBG_ASSERT(pSource,"SvxUnoTextRangeBase: I need a valid SvxEditSource!");
    DBG_ASSERT(mpEditSource==nullptr,"SvxUnoTextRangeBase::SetEditSource called while SvxEditSource already set" );

    mpEditSource.reset( pSource );

    maSelection.start.nPara = EE_PARA_MAX;

    if( mpEditSource )
        mpEditSource->addRange( this );
}

/** puts a field item with a copy of the given FieldData into the itemset
    corresponding with this range */
void SvxUnoTextRangeBase::attachField( std::unique_ptr<SvxFieldData> pData ) noexcept
{
    SolarMutexGuard aGuard;

    SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : nullptr;
    if( pForwarder )
    {
        SvxFieldItem aField( std::move(pData), EE_FEATURE_FIELD );
        pForwarder->QuickInsertField( std::move(aField), maSelection );
    }
}

void SvxUnoTextRangeBase::SetSelection( const ESelection& rSelection ) noexcept
{
    SolarMutexGuard aGuard;

    maSelection = rSelection;
    CheckSelection( maSelection, mpEditSource.get() );
}

// Interface XTextRange ( XText )

uno::Reference< text::XTextRange > SAL_CALL SvxUnoTextRangeBase::getStart()
{
    SolarMutexGuard aGuard;

    SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : nullptr;
    if( !pForwarder )
        return nullptr;

    CheckSelection( maSelection, pForwarder );

    SvxUnoTextBase* pText = comphelper::getFromUnoTunnel<SvxUnoTextBase>( getText() );

    if(pText == nullptr)
        throw uno::RuntimeException(u"Failed to retrieve a valid text base object from the Uno Tunnel"_ustr);

    rtl::Reference<SvxUnoTextRange> pRange = new SvxUnoTextRange( *pText );

    ESelection aNewSel = maSelection;
    aNewSel.CollapseToStart();
    pRange->SetSelection( aNewSel );

    return pRange;
}

uno::Reference< text::XTextRange > SAL_CALL SvxUnoTextRangeBase::getEnd()
{
    SolarMutexGuard aGuard;

    SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : nullptr;
    if( !pForwarder )
        return nullptr;

    CheckSelection( maSelection, pForwarder );

    SvxUnoTextBase* pText = comphelper::getFromUnoTunnel<SvxUnoTextBase>( getText() );

    if(pText == nullptr)
        throw uno::RuntimeException(u"Failed to retrieve a valid text base object from the Uno Tunnel"_ustr);

    rtl::Reference<SvxUnoTextRange> pNew = new SvxUnoTextRange( *pText );

    ESelection aNewSel = maSelection;
    aNewSel.CollapseToEnd();
    pNew->SetSelection( aNewSel );
    return pNew;
}

OUString SAL_CALL SvxUnoTextRangeBase::getString()
{
    SolarMutexGuard aGuard;

    SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : nullptr;
    if( pForwarder )
    {
        CheckSelection( maSelection, pForwarder );

        return pForwarder->GetText( maSelection );
    }
    else
    {
        return OUString();
    }
}

void SAL_CALL SvxUnoTextRangeBase::setString(const OUString& aString)
{
    SolarMutexGuard aGuard;

    SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : nullptr;
    if( !pForwarder )
        return;

    CheckSelection( maSelection, pForwarder );

    OUString aConverted(convertLineEnd(aString, LINEEND_LF));  // Simply count the number of line endings

    pForwarder->QuickInsertText( aConverted, maSelection );
    mpEditSource->UpdateData();

    //  Adapt selection
    //! It would be easier if the EditEngine would return the selection
    //! on QuickInsertText...
    CollapseToStart();

    sal_Int32 nLen = aConverted.getLength();
    if (nLen)
        GoRight( nLen, true );
}

// Interface beans::XPropertySet
uno::Reference< beans::XPropertySetInfo > SAL_CALL SvxUnoTextRangeBase::getPropertySetInfo()
{
    return mpPropSet->getPropertySetInfo();
}

void SAL_CALL SvxUnoTextRangeBase::setPropertyValue(const OUString& PropertyName, const uno::Any& aValue)
{
    if (PropertyName == UNO_TR_PROP_SELECTION)
    {
        text::TextRangeSelection aSel = aValue.get<text::TextRangeSelection>();
        SetSelection(toESelection(aSel));

        return;
    }

    _setPropertyValue( PropertyName, aValue );
}

void SvxUnoTextRangeBase::_setPropertyValue( const OUString& PropertyName, const uno::Any& aValue, sal_Int32 nPara )
{
    SolarMutexGuard aGuard;

    SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : nullptr;
    if( pForwarder )
    {
        CheckSelection( maSelection, pForwarder );

        const SfxItemPropertyMapEntry* pMap = mpPropSet->getPropertyMapEntry(PropertyName );
        if ( pMap )
        {
            ESelection aSel( GetSelection() );
            bool bParaAttrib = (pMap->nWID >= EE_PARA_START) && ( pMap->nWID <= EE_PARA_END );

            if (pMap->nWID == WID_PARASTYLENAME)
            {
                OUString aStyle = aValue.get<OUString>();

                sal_Int32 nEndPara;

                if( nPara == -1 )
                {
                    nPara = aSel.start.nPara;
                    nEndPara = aSel.end.nPara;
                }
                else
                {
                    // only one paragraph
                    nEndPara = nPara;
                }

                while( nPara <= nEndPara )
                {
                    pForwarder->SetStyleSheet(nPara, aStyle);
                    nPara++;
                }
            }
            else if ( nPara == -1 && !bParaAttrib )
            {
                SfxItemSet aOldSet( pForwarder->GetAttribs( aSel ) );
                // we have a selection and no para attribute
                SfxItemSet aNewSet( *aOldSet.GetPool(), aOldSet.GetRanges() );

                setPropertyValue( pMap, aValue, maSelection, aOldSet, aNewSet );


                pForwarder->QuickSetAttribs( aNewSet, GetSelection() );
            }
            else
            {
                sal_Int32 nEndPara;

                if( nPara == -1 )
                {
                    nPara = aSel.start.nPara;
                    nEndPara = aSel.end.nPara;
                }
                else
                {
                    // only one paragraph
                    nEndPara = nPara;
                }

                while( nPara <= nEndPara )
                {
                    // we have a paragraph
                    SfxItemSet aSet( pForwarder->GetParaAttribs( nPara ) );
                    setPropertyValue( pMap, aValue, maSelection, aSet, aSet );
                    pForwarder->SetParaAttribs( nPara, aSet );
                    nPara++;
                }
            }

            GetEditSource()->UpdateData();
            return;
        }
    }

    throw beans::UnknownPropertyException(PropertyName);
}

void SvxUnoTextRangeBase::setPropertyValue( const SfxItemPropertyMapEntry* pMap, const uno::Any& rValue, const ESelection& rSelection, const SfxItemSet& rOldSet, SfxItemSet& rNewSet )
{
    if(!SetPropertyValueHelper( pMap, rValue, rNewSet, &rSelection, GetEditSource() ))
    {
        // For parts of composite items with multiple properties (eg background)
        // must be taken from the document before the old item.
        rNewSet.Put(rOldSet.Get(pMap->nWID));  // Old Item in new Set
        SvxItemPropertySet::setPropertyValue(pMap, rValue, rNewSet, false );
    }
}

bool SvxUnoTextRangeBase::SetPropertyValueHelper( const SfxItemPropertyMapEntry* pMap, const uno::Any& aValue, SfxItemSet& rNewSet, const ESelection* pSelection /* = NULL */, SvxEditSource* pEditSource /* = NULL*/ )
{
    switch( pMap->nWID )
    {
    case WID_FONTDESC:
        {
            awt::FontDescriptor aDesc;
            if(aValue >>= aDesc)
            {
                SvxUnoFontDescriptor::FillItemSet( aDesc, rNewSet );
                return true;
            }
        }
        break;

    case EE_PARA_NUMBULLET:
        {
            uno::Reference< container::XIndexReplace > xRule;
            return !aValue.hasValue() || ((aValue >>= xRule) && !xRule.is());
        }

    case EE_PARA_OUTLLEVEL:
        {
            SvxTextForwarder* pForwarder = pEditSource? pEditSource->GetTextForwarder() : nullptr;
            if(pForwarder && pSelection)
            {
                if (!pForwarder->SupportsOutlineDepth())
                    return false;

                sal_Int16 nLevel = sal_Int16();
                if( aValue >>= nLevel )
                {
                    // #101004# Call interface method instead of unsafe cast
                    if (!pForwarder->SetDepth(pSelection->start.nPara, nLevel))
                        throw lang::IllegalArgumentException();

                    // If valid, then not yet finished. Also needs to be added to paragraph props.
                    return nLevel < -1 || nLevel > 9;
                }
            }
        }
        break;
    case WID_NUMBERINGSTARTVALUE:
        {
            SvxTextForwarder* pForwarder = pEditSource? pEditSource->GetTextForwarder() : nullptr;
            if(pForwarder && pSelection)
            {
                sal_Int16 nStartValue = -1;
                if( aValue >>= nStartValue )
                {
                    pForwarder->SetNumberingStartValue(pSelection->start.nPara, nStartValue);
                    return true;
                }
            }
        }
        break;
    case WID_PARAISNUMBERINGRESTART:
        {
            SvxTextForwarder* pForwarder = pEditSource? pEditSource->GetTextForwarder() : nullptr;
            if(pForwarder && pSelection)
            {
                bool bParaIsNumberingRestart = false;
                if( aValue >>= bParaIsNumberingRestart )
                {
                    pForwarder->SetParaIsNumberingRestart( pSelection->start.nPara, bParaIsNumberingRestart );
                    return true;
                }
            }
        }
        break;
    case EE_PARA_BULLETSTATE:
        {
            bool bBullet = true;
            if( aValue >>= bBullet )
            {
                SfxBoolItem aItem( EE_PARA_BULLETSTATE, bBullet );
                rNewSet.Put(aItem);
                return true;
            }
        }
        break;

    default:
        return false;
    }

    throw lang::IllegalArgumentException();
}

uno::Any SAL_CALL SvxUnoTextRangeBase::getPropertyValue(const OUString& PropertyName)
{
    if (PropertyName == UNO_TR_PROP_SELECTION)
    {
        const ESelection& rSel = GetSelection();
        text::TextRangeSelection aSel;
        aSel.Start.Paragraph = rSel.start.nPara;
        aSel.Start.PositionInParagraph = rSel.start.nIndex;
        aSel.End.Paragraph = rSel.end.nPara;
        aSel.End.PositionInParagraph = rSel.end.nIndex;
        return uno::Any(aSel);
    }

    return _getPropertyValue( PropertyName );
}

uno::Any SvxUnoTextRangeBase::_getPropertyValue(const OUString& PropertyName, sal_Int32 nPara )
{
    SolarMutexGuard aGuard;

    uno::Any aAny;

    SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : nullptr;
    if( pForwarder )
    {
        const SfxItemPropertyMapEntry* pMap = mpPropSet->getPropertyMapEntry(PropertyName );
        if( pMap )
        {
            std::optional<SfxItemSet> oAttribs;
            if( nPara != -1 )
                oAttribs.emplace(pForwarder->GetParaAttribs( nPara ));
            else
                oAttribs.emplace(pForwarder->GetAttribs( GetSelection() ));

            //  Replace Dontcare with Default, so that one always has a mirror
            oAttribs->ClearInvalidItems();

            getPropertyValue( pMap, aAny, *oAttribs );

            return aAny;
        }
    }

    throw beans::UnknownPropertyException(PropertyName);
}

void SvxUnoTextRangeBase::getPropertyValue( const SfxItemPropertyMapEntry* pMap, uno::Any& rAny, const SfxItemSet& rSet )
{
    switch( pMap->nWID )
    {
    case EE_FEATURE_FIELD:
    {
        const SvxFieldItem* pItem = nullptr;
        if ( rSet.GetItemState( EE_FEATURE_FIELD, false, &pItem ) == SfxItemState::SET )
        {
            const SvxFieldData* pData = pItem->GetField();
            uno::Reference< text::XTextRange > xAnchor( this );

            // get presentation string for field
            std::optional<Color> pTColor;
            std::optional<Color> pFColor;
            std::optional<FontLineStyle> pFldLineStyle;

            SvxTextForwarder* pForwarder = mpEditSource->GetTextForwarder();
            OUString aPresentation( pForwarder->CalcFieldValue( SvxFieldItem(*pData, EE_FEATURE_FIELD), maSelection.start.nPara, maSelection.start.nIndex, pTColor, pFColor, pFldLineStyle ) );

            uno::Reference< text::XTextField > xField( new SvxUnoTextField( xAnchor, aPresentation, pData ) );
            rAny <<= xField;
        }
        break;
    }
    case WID_PORTIONTYPE:
        if ( rSet.GetItemState( EE_FEATURE_FIELD, false ) == SfxItemState::SET )
        {
            rAny <<= u"TextField"_ustr;
        }
        else
        {
            rAny <<= u"Text"_ustr;
        }
        break;

    case WID_PARASTYLENAME:
        {
            rAny <<= GetEditSource()->GetTextForwarder()->GetStyleSheet(maSelection.start.nPara);
        }
        break;

    default:
        if(!GetPropertyValueHelper( *const_cast<SfxItemSet*>(&rSet), pMap, rAny, &maSelection, GetEditSource() ))
            rAny = SvxItemPropertySet::getPropertyValue(pMap, rSet, true, false );
    }
}

bool SvxUnoTextRangeBase::GetPropertyValueHelper(  SfxItemSet const & rSet, const SfxItemPropertyMapEntry* pMap, uno::Any& aAny, const ESelection* pSelection /* = NULL */, SvxEditSource* pEditSource /* = NULL */ )
{
    switch( pMap->nWID )
    {
    case WID_FONTDESC:
        {
            awt::FontDescriptor aDesc;
            SvxUnoFontDescriptor::FillFromItemSet( rSet, aDesc );
            aAny <<= aDesc;
        }
        break;

    case EE_PARA_NUMBULLET:
        {
            SfxItemState eState = rSet.GetItemState( EE_PARA_NUMBULLET );
            if( eState != SfxItemState::SET && eState != SfxItemState::DEFAULT)
                throw uno::RuntimeException(u"Invalid item state for paragraph numbering/bullet. Expected SET or DEFAULT."_ustr);

            const SvxNumBulletItem* pBulletItem = rSet.GetItem( EE_PARA_NUMBULLET );

            if( pBulletItem == nullptr )
                throw uno::RuntimeException(u"Unable to retrieve paragraph numbering/bullet item."_ustr);

            aAny <<= SvxCreateNumRule( pBulletItem->GetNumRule() );
        }
        break;

    case EE_PARA_OUTLLEVEL:
        {
            SvxTextForwarder* pForwarder = pEditSource? pEditSource->GetTextForwarder() : nullptr;
            if(pForwarder && pSelection)
            {
                if (!pForwarder->SupportsOutlineDepth())
                    return false;

                sal_Int16 nLevel = pForwarder->GetDepth(pSelection->start.nPara);
                if( nLevel >= 0 )
                    aAny <<= nLevel;
            }
        }
        break;
    case WID_NUMBERINGSTARTVALUE:
        {
            SvxTextForwarder* pForwarder = pEditSource? pEditSource->GetTextForwarder() : nullptr;
            if(pForwarder && pSelection)
                aAny <<= pForwarder->GetNumberingStartValue(pSelection->start.nPara);
        }
        break;
    case WID_PARAISNUMBERINGRESTART:
        {
            SvxTextForwarder* pForwarder = pEditSource? pEditSource->GetTextForwarder() : nullptr;
            if(pForwarder && pSelection)
                aAny <<= pForwarder->IsParaIsNumberingRestart(pSelection->start.nPara);
        }
        break;

    case EE_PARA_BULLETSTATE:
        {
            bool bState = false;
            SfxItemState eState = rSet.GetItemState( EE_PARA_BULLETSTATE );
            if( eState == SfxItemState::SET || eState == SfxItemState::DEFAULT )
            {
                const SfxBoolItem* pItem = rSet.GetItem<SfxBoolItem>( EE_PARA_BULLETSTATE );
                bState = pItem->GetValue();
            }

            aAny <<= bState;
        }
        break;
    default:

        return false;
    }

    return true;
}

// is not (yet) supported
void SAL_CALL SvxUnoTextRangeBase::addPropertyChangeListener( const OUString& , const uno::Reference< beans::XPropertyChangeListener >& ) {}
void SAL_CALL SvxUnoTextRangeBase::removePropertyChangeListener( const OUString& , const uno::Reference< beans::XPropertyChangeListener >& ) {}
void SAL_CALL SvxUnoTextRangeBase::addVetoableChangeListener( const OUString& , const uno::Reference< beans::XVetoableChangeListener >& ) {}
void SAL_CALL SvxUnoTextRangeBase::removeVetoableChangeListener( const OUString& , const uno::Reference< beans::XVetoableChangeListener >& ) {}

// XMultiPropertySet
void SAL_CALL SvxUnoTextRangeBase::setPropertyValues( const uno::Sequence< OUString >& aPropertyNames, const uno::Sequence< uno::Any >& aValues )
{
    _setPropertyValues( aPropertyNames, aValues );
}

void SvxUnoTextRangeBase::_setPropertyValues( const uno::Sequence< OUString >& aPropertyNames, const uno::Sequence< uno::Any >& aValues, sal_Int32 nPara )
{
    if (aPropertyNames.getLength() != aValues.getLength())
        throw lang::IllegalArgumentException(u"lengths do not match"_ustr,
                                             static_cast<css::beans::XPropertySet*>(this), -1);

    SolarMutexGuard aGuard;

    SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : nullptr;
    if( !pForwarder )
        return;

    CheckSelection( maSelection, pForwarder );

    ESelection aSel( GetSelection() );

    const OUString* pPropertyNames = aPropertyNames.getConstArray();
    const uno::Any* pValues = aValues.getConstArray();
    sal_Int32 nCount = aPropertyNames.getLength();

    sal_Int32 nEndPara = nPara;
    sal_Int32 nTempPara = nPara;

    if( nTempPara == -1 )
    {
        nTempPara = aSel.start.nPara;
        nEndPara = aSel.end.nPara;
    }

    std::optional<SfxItemSet> pOldAttrSet;
    std::optional<SfxItemSet> pNewAttrSet;

    std::optional<SfxItemSet> pOldParaSet;
    std::optional<SfxItemSet> pNewParaSet;

    std::optional<OUString> aStyleName;

    for( ; nCount; nCount--, pPropertyNames++, pValues++ )
    {
        const SfxItemPropertyMapEntry* pMap = mpPropSet->getPropertyMapEntry( *pPropertyNames );

        if( pMap )
        {
            bool bParaAttrib = (pMap->nWID >= EE_PARA_START) && ( pMap->nWID <= EE_PARA_END );

            if (pMap->nWID == WID_PARASTYLENAME)
            {
                aStyleName.emplace((*pValues).get<OUString>());
            }
            else if( (nPara == -1) && !bParaAttrib )
            {
                if( !pNewAttrSet )
                {
                    pOldAttrSet.emplace( pForwarder->GetAttribs( aSel ) );
                    pNewAttrSet.emplace( *pOldAttrSet->GetPool(), pOldAttrSet->GetRanges() );
                }

                setPropertyValue( pMap, *pValues, GetSelection(), *pOldAttrSet, *pNewAttrSet );

                if( pMap->nWID >= EE_ITEMS_START && pMap->nWID <= EE_ITEMS_END )
                {
                    const SfxPoolItem* pItem;
                    if( pNewAttrSet->GetItemState( pMap->nWID, true, &pItem ) == SfxItemState::SET )
                    {
                        pOldAttrSet->Put( *pItem );
                    }
                }
            }
            else
            {
                if( !pNewParaSet )
                {
                    pOldParaSet.emplace( pForwarder->GetParaAttribs( nTempPara ) );
                    pNewParaSet.emplace( *pOldParaSet->GetPool(), pOldParaSet->GetRanges() );
                }

                setPropertyValue( pMap, *pValues, GetSelection(), *pOldParaSet, *pNewParaSet );

                if( pMap->nWID >= EE_ITEMS_START && pMap->nWID <= EE_ITEMS_END )
                {
                    const SfxPoolItem* pItem;
                    if( pNewParaSet->GetItemState( pMap->nWID, true, &pItem ) == SfxItemState::SET )
                    {
                        pOldParaSet->Put( *pItem );
                    }
                }

            }
        }
    }

    bool bNeedsUpdate = false;

    if( pNewParaSet || aStyleName )
    {
        if( pNewParaSet->Count() )
        {
            while( nTempPara <= nEndPara )
            {
                SfxItemSet aSet( pForwarder->GetParaAttribs( nTempPara ) );
                aSet.Put( *pNewParaSet );
                pForwarder->SetParaAttribs( nTempPara, aSet );
                if (aStyleName)
                    pForwarder->SetStyleSheet(nTempPara, *aStyleName);
                nTempPara++;
            }
            bNeedsUpdate = true;
        }

        pNewParaSet.reset();
        pOldParaSet.reset();
    }

    if( pNewAttrSet )
    {
        if( pNewAttrSet->Count() )
        {
            pForwarder->QuickSetAttribs( *pNewAttrSet, GetSelection() );
            bNeedsUpdate = true;
        }
        pNewAttrSet.reset();
        pOldAttrSet.reset();
    }

    if( bNeedsUpdate )
        GetEditSource()->UpdateData();
}

uno::Sequence< uno::Any > SAL_CALL SvxUnoTextRangeBase::getPropertyValues( const uno::Sequence< OUString >& aPropertyNames )
{
    return _getPropertyValues( aPropertyNames );
}

uno::Sequence< uno::Any > SvxUnoTextRangeBase::_getPropertyValues( const uno::Sequence< OUString >& aPropertyNames, sal_Int32 nPara )
{
    SolarMutexGuard aGuard;

    sal_Int32 nCount = aPropertyNames.getLength();


    uno::Sequence< uno::Any > aValues( nCount );

    SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : nullptr;
    if( pForwarder )
    {
        std::optional<SfxItemSet> oAttribs;
        if( nPara != -1 )
            oAttribs.emplace(pForwarder->GetParaAttribs( nPara ));
        else
            oAttribs.emplace(pForwarder->GetAttribs( GetSelection() ));

        oAttribs->ClearInvalidItems();

        const OUString* pPropertyNames = aPropertyNames.getConstArray();
        uno::Any* pValues = aValues.getArray();

        for( ; nCount; nCount--, pPropertyNames++, pValues++ )
        {
            const SfxItemPropertyMapEntry* pMap = mpPropSet->getPropertyMapEntry( *pPropertyNames );
            if( pMap )
            {
                getPropertyValue( pMap, *pValues, *oAttribs );
            }
        }
    }

    return aValues;
}

void SAL_CALL SvxUnoTextRangeBase::addPropertiesChangeListener( const uno::Sequence< OUString >& , const uno::Reference< beans::XPropertiesChangeListener >& )
{
}

void SAL_CALL SvxUnoTextRangeBase::removePropertiesChangeListener( const uno::Reference< beans::XPropertiesChangeListener >& )
{
}

void SAL_CALL SvxUnoTextRangeBase::firePropertiesChangeEvent( const uno::Sequence< OUString >& , const uno::Reference< beans::XPropertiesChangeListener >& )
{
}

// beans::XPropertyState
beans::PropertyState SAL_CALL SvxUnoTextRangeBase::getPropertyState( const OUString& PropertyName )
{
    return _getPropertyState( PropertyName );
}

const sal_uInt16 aSvxUnoFontDescriptorWhichMap[] = { EE_CHAR_FONTINFO, EE_CHAR_FONTHEIGHT, EE_CHAR_ITALIC,
                                                  EE_CHAR_UNDERLINE, EE_CHAR_WEIGHT, EE_CHAR_STRIKEOUT, EE_CHAR_CASEMAP,
                                                  EE_CHAR_WLM, 0 };

beans::PropertyState SvxUnoTextRangeBase::_getPropertyState(const SfxItemPropertyMapEntry* pMap, sal_Int32 nPara)
{
    if ( pMap )
    {
        SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : nullptr;
        if( pForwarder )
        {
            SfxItemState eItemState(SfxItemState::DEFAULT);
            bool bItemStateSet(false);

            switch( pMap->nWID )
            {
            case WID_FONTDESC:
                {
                    const sal_uInt16* pWhichId = aSvxUnoFontDescriptorWhichMap;
                    while( *pWhichId )
                    {
                        const SfxItemState eTempItemState(nPara != -1
                            ? pForwarder->GetItemState( nPara, *pWhichId )
                            : pForwarder->GetItemState( GetSelection(), *pWhichId ));

                        switch( eTempItemState )
                        {
                        case SfxItemState::DISABLED:
                        case SfxItemState::INVALID:
                            eItemState = SfxItemState::INVALID;
                            bItemStateSet = true;
                            break;

                        case SfxItemState::DEFAULT:
                            if( !bItemStateSet )
                            {
                                eItemState = SfxItemState::DEFAULT;
                                bItemStateSet = true;
                            }
                            break;

                        case SfxItemState::SET:
                            if( !bItemStateSet )
                            {
                                eItemState = SfxItemState::SET;
                                bItemStateSet = true;
                            }
                            break;
                        default:
                            throw beans::UnknownPropertyException();
                        }

                        pWhichId++;
                    }
                }
                break;

            case WID_NUMBERINGSTARTVALUE:
            case WID_PARAISNUMBERINGRESTART:
            case WID_PARASTYLENAME:
                eItemState = SfxItemState::SET;
                bItemStateSet = true;
                break;

            default:
                if(0 != pMap->nWID)
                {
                    if( nPara != -1 )
                        eItemState = pForwarder->GetItemState( nPara, pMap->nWID );
                    else
                        eItemState = pForwarder->GetItemState( GetSelection(), pMap->nWID );

                    bItemStateSet = true;
                }
                break;
            }

            if(bItemStateSet)
            {
                switch( eItemState )
                {
                case SfxItemState::INVALID:
                case SfxItemState::DISABLED:
                    return beans::PropertyState_AMBIGUOUS_VALUE;
                case SfxItemState::SET:
                    return beans::PropertyState_DIRECT_VALUE;
                case SfxItemState::DEFAULT:
                    return beans::PropertyState_DEFAULT_VALUE;
                default: break;
                }
            }
        }
    }
    throw beans::UnknownPropertyException();
}

beans::PropertyState SvxUnoTextRangeBase::_getPropertyState(std::u16string_view PropertyName, sal_Int32 nPara /* = -1 */)
{
    SolarMutexGuard aGuard;

    return _getPropertyState( mpPropSet->getPropertyMapEntry( PropertyName ), nPara);
}

uno::Sequence< beans::PropertyState > SAL_CALL SvxUnoTextRangeBase::getPropertyStates( const uno::Sequence< OUString >& aPropertyName )
{
    return _getPropertyStates( aPropertyName );
}

uno::Sequence< beans::PropertyState > SvxUnoTextRangeBase::_getPropertyStates(const uno::Sequence< OUString >& PropertyName, sal_Int32 nPara /* = -1 */)
{
    uno::Sequence< beans::PropertyState > aRet( PropertyName.getLength() );

    SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : nullptr;
    if( pForwarder )
    {
        std::optional<SfxItemSet> pSet;
        if( nPara != -1 )
        {
            pSet.emplace( pForwarder->GetParaAttribs( nPara ) );
        }
        else
        {
            ESelection aSel( GetSelection() );
            CheckSelection( aSel, pForwarder );
            pSet.emplace( pForwarder->GetAttribs( aSel, EditEngineAttribs::OnlyHard ) );
        }

        beans::PropertyState* pState = aRet.getArray();
        for( const OUString& rName : PropertyName )
        {
            const SfxItemPropertyMapEntry* pMap = mpPropSet->getPropertyMapEntry( rName );
            if( !_getOnePropertyStates(*pSet, pMap, *pState++) )
            {
                throw beans::UnknownPropertyException(rName);
            }
        }
    }

    return aRet;
}

bool SvxUnoTextRangeBase::_getOnePropertyStates(const SfxItemSet& rSet, const SfxItemPropertyMapEntry* pMap, beans::PropertyState& rState)
{
    if (!pMap)
        return true;
    SfxItemState eItemState = SfxItemState::DEFAULT;
    bool bItemStateSet(false);

    bool bUnknownPropertyFound = false;
    switch( pMap->nWID )
    {
        case WID_FONTDESC:
            {
                const sal_uInt16* pWhichId = aSvxUnoFontDescriptorWhichMap;
                while( *pWhichId )
                {
                    const SfxItemState eTempItemState(rSet.GetItemState( *pWhichId ));

                    switch( eTempItemState )
                    {
                    case SfxItemState::DISABLED:
                    case SfxItemState::INVALID:
                        eItemState = SfxItemState::INVALID;
                        bItemStateSet = true;
                        break;

                    case SfxItemState::DEFAULT:
                        if( !bItemStateSet )
                        {
                            eItemState = SfxItemState::DEFAULT;
                            bItemStateSet = true;
                        }
                        break;

                    case SfxItemState::SET:
                        if( !bItemStateSet )
                        {
                            eItemState = SfxItemState::SET;
                            bItemStateSet = true;
                        }
                        break;
                    default:
                        bUnknownPropertyFound = true;
                        break;
                    }

                    pWhichId++;
                }
            }
            break;

        case WID_NUMBERINGSTARTVALUE:
        case WID_PARAISNUMBERINGRESTART:
        case WID_PARASTYLENAME:
            eItemState = SfxItemState::SET;
            bItemStateSet = true;
            break;

        default:
            if(0 != pMap->nWID)
            {
                eItemState = rSet.GetItemState( pMap->nWID, false );
                bItemStateSet = true;
            }
            break;
    }

    if( bUnknownPropertyFound )
        return false;

    if(bItemStateSet)
    {
        if (pMap->nWID == EE_CHAR_COLOR)
        {
            // Theme & effects can be DEFAULT_VALUE, even if the same pool item has a color
            // which is a DIRECT_VALUE.
            const SvxColorItem* pColor = rSet.GetItem<SvxColorItem>(EE_CHAR_COLOR);
            if (!pColor)
            {
                SAL_WARN("editeng", "Missing EE_CHAR_COLOR SvxColorItem");
                return false;
            }
            switch (pMap->nMemberId)
            {
                case MID_COLOR_THEME_INDEX:
                    if (!pColor->getComplexColor().isValidThemeType())
                    {
                        eItemState = SfxItemState::DEFAULT;
                    }
                    break;
                case MID_COLOR_LUM_MOD:
                {
                    sal_Int16 nLumMod = 10000;
                    for (auto const& rTransform : pColor->getComplexColor().getTransformations())
                    {
                        if (rTransform.meType == model::TransformationType::LumMod)
                            nLumMod = rTransform.mnValue;
                    }
                    if (nLumMod == 10000)
                    {
                        eItemState = SfxItemState::DEFAULT;
                    }
                    break;
                }
                case MID_COLOR_LUM_OFF:
                {
                    sal_Int16 nLumOff = 0;
                    for (auto const& rTransform : pColor->getComplexColor().getTransformations())
                    {
                        if (rTransform.meType == model::TransformationType::LumOff)
                            nLumOff = rTransform.mnValue;
                    }
                    if (nLumOff == 0)
                    {
                        eItemState = SfxItemState::DEFAULT;
                    }
                    break;
                }
                case MID_COMPLEX_COLOR:
                    if (pColor->getComplexColor().getType() == model::ColorType::Unused)
                    {
                        eItemState = SfxItemState::DEFAULT;
                    }
                    break;
            }
        }

        switch( eItemState )
        {
            case SfxItemState::SET:
                rState = beans::PropertyState_DIRECT_VALUE;
                break;
            case SfxItemState::DEFAULT:
                rState = beans::PropertyState_DEFAULT_VALUE;
                break;
//                  case SfxItemState::INVALID:
//                  case SfxItemState::DISABLED:
            default:
                rState = beans::PropertyState_AMBIGUOUS_VALUE;
        }
    }
    else
    {
        rState = beans::PropertyState_AMBIGUOUS_VALUE;
    }
    return true;
}

void SAL_CALL SvxUnoTextRangeBase::setPropertyToDefault( const OUString& PropertyName )
{
    _setPropertyToDefault( PropertyName );
}

void SvxUnoTextRangeBase::_setPropertyToDefault(const OUString& PropertyName, sal_Int32 nPara /* = -1 */)
{
    SolarMutexGuard aGuard;

    SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : nullptr;

    if( pForwarder )
    {
        const SfxItemPropertyMapEntry* pMap = mpPropSet->getPropertyMapEntry( PropertyName );
        if ( pMap )
        {
            CheckSelection( maSelection, mpEditSource->GetTextForwarder() );
            _setPropertyToDefault( pForwarder, pMap, nPara );
            return;
        }
    }

    throw beans::UnknownPropertyException(PropertyName);
}

void SvxUnoTextRangeBase::_setPropertyToDefault(SvxTextForwarder* pForwarder, const SfxItemPropertyMapEntry* pMap, sal_Int32 nPara )
{
    do
    {
        SfxItemSet aSet(*pForwarder->GetPool());

        if( pMap->nWID == WID_FONTDESC )
        {
            SvxUnoFontDescriptor::setPropertyToDefault( aSet );
        }
        else if( pMap->nWID == WID_NUMBERINGSTARTVALUE )
        {
            pForwarder->SetNumberingStartValue(maSelection.start.nPara, -1);
        }
        else if( pMap->nWID == WID_PARAISNUMBERINGRESTART )
        {
            pForwarder->SetParaIsNumberingRestart(maSelection.start.nPara, false);
        }
        else
        {
            aSet.InvalidateItem( pMap->nWID );
        }

        if(nPara != -1)
            pForwarder->SetParaAttribs( nPara, aSet );
        else
            pForwarder->QuickSetAttribs( aSet, GetSelection() );

        GetEditSource()->UpdateData();

        return;
    }
    while(false);
}

uno::Any SAL_CALL SvxUnoTextRangeBase::getPropertyDefault( const OUString& aPropertyName )
{
    SolarMutexGuard aGuard;

    SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : nullptr;
    if( pForwarder )
    {
        const SfxItemPropertyMapEntry* pMap = mpPropSet->getPropertyMapEntry( aPropertyName );
        if( pMap )
        {
            SfxItemPool* pPool = pForwarder->GetPool();

            switch( pMap->nWID )
            {
            case WID_FONTDESC:
                return SvxUnoFontDescriptor::getPropertyDefault( pPool );

            case EE_PARA_OUTLLEVEL:
                {
                    uno::Any aAny;
                    return aAny;
                }

            case WID_NUMBERINGSTARTVALUE:
                return uno::Any( sal_Int16(-1) );

            case WID_PARAISNUMBERINGRESTART:
                return uno::Any( false );

            default:
                {
                    // Get Default from ItemPool
                    if(SfxItemPool::IsWhich(pMap->nWID))
                    {
                        SfxItemSet aSet( *pPool, pMap->nWID, pMap->nWID );
                        aSet.Put(pPool->GetUserOrPoolDefaultItem(pMap->nWID));
                        return SvxItemPropertySet::getPropertyValue(pMap, aSet, true, false );
                    }
                }
            }
        }
    }
    throw beans::UnknownPropertyException(aPropertyName);
}

// beans::XMultiPropertyStates
void SAL_CALL SvxUnoTextRangeBase::setAllPropertiesToDefault()
{
    SolarMutexGuard aGuard;

    SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : nullptr;

    if( pForwarder )
    {
        for (const SfxItemPropertyMapEntry* entry : mpPropSet->getPropertyMap().getPropertyEntries())
        {
            _setPropertyToDefault( pForwarder, entry, -1 );
        }
    }
}

void SAL_CALL SvxUnoTextRangeBase::setPropertiesToDefault( const uno::Sequence< OUString >& aPropertyNames )
{
    for( const OUString& rName : aPropertyNames )
    {
        setPropertyToDefault( rName );
    }
}

uno::Sequence< uno::Any > SAL_CALL SvxUnoTextRangeBase::getPropertyDefaults( const uno::Sequence< OUString >& aPropertyNames )
{
    uno::Sequence< uno::Any > ret( aPropertyNames.getLength() );
    uno::Any* pDefaults = ret.getArray();

    for( const OUString& rName : aPropertyNames )
    {
        *pDefaults++ = getPropertyDefault( rName );
    }

    return ret;
}

// internal
void SvxUnoTextRangeBase::CollapseToStart() noexcept
{
    CheckSelection( maSelection, mpEditSource.get() );

    maSelection.CollapseToStart();
}

void SvxUnoTextRangeBase::CollapseToEnd() noexcept
{
    CheckSelection( maSelection, mpEditSource.get() );

    maSelection.CollapseToEnd();
}

bool SvxUnoTextRangeBase::IsCollapsed() noexcept
{
    CheckSelection( maSelection, mpEditSource.get() );

    return !maSelection.HasRange();
}

bool SvxUnoTextRangeBase::GoLeft(sal_Int32 nCount, bool Expand) noexcept
{
    CheckSelection( maSelection, mpEditSource.get() );

    //  #75098# use end position, as in Writer (start is anchor, end is cursor)
    sal_Int32 nNewPos = maSelection.end.nIndex;
    sal_Int32 nNewPar = maSelection.end.nPara;

    bool bOk = true;
    SvxTextForwarder* pForwarder = nullptr;
    while ( nCount > nNewPos && bOk )
    {
        if ( nNewPar == 0 )
            bOk = false;
        else
        {
            if ( !pForwarder )
                pForwarder = mpEditSource->GetTextForwarder();  // first here, it is necessary...
            assert(pForwarder);
            --nNewPar;
            nCount -= nNewPos + 1;
            nNewPos = pForwarder->GetTextLen( nNewPar );
        }
    }

    if ( bOk )
    {
        nNewPos = nNewPos - nCount;
        maSelection.start.nPara = nNewPar;
        maSelection.start.nIndex  = nNewPos;
    }

    if (!Expand)
        CollapseToStart();

    return bOk;
}

bool SvxUnoTextRangeBase::GoRight(sal_Int32 nCount, bool Expand)  noexcept
{
    if (!mpEditSource)
        return false;
    SvxTextForwarder* pForwarder = mpEditSource->GetTextForwarder();
    if( !pForwarder )
        return false;

    CheckSelection( maSelection, pForwarder );

    sal_Int32 nNewPos = maSelection.end.nIndex + nCount;
    sal_Int32 nNewPar = maSelection.end.nPara;

    bool bOk = true;
    sal_Int32 nParCount = pForwarder->GetParagraphCount();
    sal_Int32 nThisLen = pForwarder->GetTextLen( nNewPar );
    while ( nNewPos > nThisLen && bOk )
    {
        if ( nNewPar + 1 >= nParCount )
            bOk = false;
        else
        {
            nNewPos -= nThisLen+1;
            ++nNewPar;
            nThisLen = pForwarder->GetTextLen( nNewPar );
        }
    }

    if (bOk)
    {
        maSelection.end.nPara = nNewPar;
        maSelection.end.nIndex  = nNewPos;
    }

    if (!Expand)
        CollapseToEnd();

    return bOk;
}

void SvxUnoTextRangeBase::GotoStart(bool Expand) noexcept
{
    maSelection.start.nPara = 0;
    maSelection.start.nIndex  = 0;

    if (!Expand)
        CollapseToStart();
}

void SvxUnoTextRangeBase::GotoEnd(bool Expand) noexcept
{
    CheckSelection( maSelection, mpEditSource.get() );

    SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : nullptr;
    if( !pForwarder )
        return;

    sal_Int32 nPar = pForwarder->GetParagraphCount();
    if (nPar)
        --nPar;

    maSelection.end.nPara = nPar;
    maSelection.end.nIndex = pForwarder->GetTextLen(nPar);

    if (!Expand)
        CollapseToEnd();
}

// lang::XServiceInfo
sal_Bool SAL_CALL SvxUnoTextRangeBase::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService( this, ServiceName );
}

uno::Sequence< OUString > SAL_CALL SvxUnoTextRangeBase::getSupportedServiceNames()
{
    return getSupportedServiceNames_Static();
}

uno::Sequence< OUString > SvxUnoTextRangeBase::getSupportedServiceNames_Static()
{
    return { u"com.sun.star.style.CharacterProperties"_ustr,
             u"com.sun.star.style.CharacterPropertiesComplex"_ustr,
             u"com.sun.star.style.CharacterPropertiesAsian"_ustr };
}

// XTextRangeCompare
sal_Int16 SAL_CALL SvxUnoTextRangeBase::compareRegionStarts( const uno::Reference< text::XTextRange >& xR1, const uno::Reference< text::XTextRange >& xR2 )
{
    SvxUnoTextRangeBase* pR1 = comphelper::getFromUnoTunnel<SvxUnoTextRangeBase>( xR1 );
    SvxUnoTextRangeBase* pR2 = comphelper::getFromUnoTunnel<SvxUnoTextRangeBase>( xR2 );

    if( (pR1 == nullptr) || (pR2 == nullptr) )
        throw lang::IllegalArgumentException();

    const ESelection& r1 = pR1->maSelection;
    const ESelection& r2 = pR2->maSelection;

    return r1.start == r2.start ? 0 : r1.start < r2.start ? 1 : -1;
}

sal_Int16 SAL_CALL SvxUnoTextRangeBase::compareRegionEnds( const uno::Reference< text::XTextRange >& xR1, const uno::Reference< text::XTextRange >& xR2 )
{
    SvxUnoTextRangeBase* pR1 = comphelper::getFromUnoTunnel<SvxUnoTextRangeBase>( xR1 );
    SvxUnoTextRangeBase* pR2 = comphelper::getFromUnoTunnel<SvxUnoTextRangeBase>( xR2 );

    if( (pR1 == nullptr) || (pR2 == nullptr) )
        throw lang::IllegalArgumentException();

    const ESelection& r1 = pR1->maSelection;
    const ESelection& r2 = pR2->maSelection;

    return r1.end == r2.end ? 0 : r1.end < r2.end ? 1 : -1;
}

SvxUnoTextRange::SvxUnoTextRange(const SvxUnoTextBase& rParent, bool bPortion /* = false */)
:SvxUnoTextRangeBase( rParent.GetEditSource(), bPortion ? ImplGetSvxTextPortionSvxPropertySet() : rParent.getPropertySet() ),
 mbPortion( bPortion )
{
    xParentText =  static_cast<text::XText*>(const_cast<SvxUnoTextBase *>(&rParent));
}

SvxUnoTextRange::~SvxUnoTextRange() noexcept
{
}

uno::Any SAL_CALL SvxUnoTextRange::queryAggregation( const uno::Type & rType )
{
    QUERYINT( text::XTextRange );
    else if( rType == cppu::UnoType<beans::XMultiPropertyStates>::get())
        return uno::Any(uno::Reference< beans::XMultiPropertyStates >(this));
    else if( rType == cppu::UnoType<beans::XPropertySet>::get())
        return uno::Any(uno::Reference< beans::XPropertySet >(this));
    else QUERYINT( beans::XPropertyState );
    else QUERYINT( text::XTextRangeCompare );
    else if( rType == cppu::UnoType<beans::XMultiPropertySet>::get())
        return uno::Any(uno::Reference< beans::XMultiPropertySet >(this));
    else QUERYINT( lang::XServiceInfo );
    else QUERYINT( lang::XTypeProvider );
    else QUERYINT( lang::XUnoTunnel );
    else
        return OWeakAggObject::queryAggregation( rType );
}

uno::Any SAL_CALL SvxUnoTextRange::queryInterface( const uno::Type & rType )
{
    return OWeakAggObject::queryInterface(rType);
}

void SAL_CALL SvxUnoTextRange::acquire()
    noexcept
{
    OWeakAggObject::acquire();
}

void SAL_CALL SvxUnoTextRange::release()
    noexcept
{
    OWeakAggObject::release();
}

// XTypeProvider

uno::Sequence< uno::Type > SAL_CALL SvxUnoTextRange::getTypes()
{
    static const uno::Sequence< uno::Type > TYPES {
            cppu::UnoType<text::XTextRange>::get(),
            cppu::UnoType<beans::XPropertySet>::get(),
            cppu::UnoType<beans::XMultiPropertySet>::get(),
            cppu::UnoType<beans::XMultiPropertyStates>::get(),
            cppu::UnoType<beans::XPropertyState>::get(),
            cppu::UnoType<lang::XServiceInfo>::get(),
            cppu::UnoType<lang::XTypeProvider>::get(),
            cppu::UnoType<lang::XUnoTunnel>::get(),
            cppu::UnoType<text::XTextRangeCompare>::get() };
    return TYPES;
}

uno::Sequence< sal_Int8 > SAL_CALL SvxUnoTextRange::getImplementationId()
{
    return css::uno::Sequence<sal_Int8>();
}

// XTextRange
uno::Reference< text::XText > SAL_CALL SvxUnoTextRange::getText()
{
    return xParentText;
}

// lang::XServiceInfo
OUString SAL_CALL SvxUnoTextRange::getImplementationName()
{
    return u"SvxUnoTextRange"_ustr;
}




SvxUnoTextBase::SvxUnoTextBase(const SvxItemPropertySet* _pSet)
    : SvxUnoTextRangeBase(_pSet)
{
}

SvxUnoTextBase::SvxUnoTextBase(const SvxEditSource* pSource, const SvxItemPropertySet* _pSet, uno::Reference < text::XText > const & xParent)
    : SvxUnoTextRangeBase(pSource, _pSet)
{
    xParentText = xParent;
    ESelection aSelection;
    ::GetSelection( aSelection, GetEditSource()->GetTextForwarder() );
    SetSelection( aSelection );
}

SvxUnoTextBase::SvxUnoTextBase(const SvxUnoTextBase& rText)
:   SvxUnoTextRangeBase( rText )
, text::XTextAppend()
,   text::XTextCopy()
,   container::XEnumerationAccess()
,   text::XTextRangeMover()
,   lang::XTypeProvider()
{
    xParentText = rText.xParentText;
}

SvxUnoTextBase::~SvxUnoTextBase() noexcept
{
}

// XInterface
uno::Any SAL_CALL SvxUnoTextBase::queryAggregation( const uno::Type & rType )
{
    QUERYINT( text::XText );
    QUERYINT( text::XSimpleText );
    if( rType == cppu::UnoType<text::XTextRange>::get())
        return uno::Any(uno::Reference< text::XTextRange >(static_cast<text::XText*>(this)));
    QUERYINT(container::XEnumerationAccess );
    QUERYINT( container::XElementAccess );
    QUERYINT( beans::XMultiPropertyStates );
    QUERYINT( beans::XPropertySet );
    QUERYINT( beans::XMultiPropertySet );
    QUERYINT( beans::XPropertyState );
    QUERYINT( text::XTextRangeCompare );
    QUERYINT( lang::XServiceInfo );
    QUERYINT( text::XTextRangeMover );
    QUERYINT( text::XTextCopy );
    QUERYINT( text::XTextAppend );
    QUERYINT( text::XParagraphAppend );
    QUERYINT( text::XTextPortionAppend );
    QUERYINT( lang::XTypeProvider );
    QUERYINT( lang::XUnoTunnel );

    return uno::Any();
}

// XTypeProvider

uno::Sequence< uno::Type > SAL_CALL SvxUnoTextBase::getTypes()
{
    static const uno::Sequence< uno::Type > TYPES {
            cppu::UnoType<text::XText>::get(),
            cppu::UnoType<container::XEnumerationAccess>::get(),
            cppu::UnoType<beans::XPropertySet>::get(),
            cppu::UnoType<beans::XMultiPropertySet>::get(),
            cppu::UnoType<beans::XMultiPropertyStates>::get(),
            cppu::UnoType<beans::XPropertyState>::get(),
            cppu::UnoType<text::XTextRangeMover>::get(),
            cppu::UnoType<text::XTextAppend>::get(),
            cppu::UnoType<text::XTextCopy>::get(),
            cppu::UnoType<text::XParagraphAppend>::get(),
            cppu::UnoType<text::XTextPortionAppend>::get(),
            cppu::UnoType<lang::XServiceInfo>::get(),
            cppu::UnoType<lang::XTypeProvider>::get(),
            cppu::UnoType<lang::XUnoTunnel>::get(),
            cppu::UnoType<text::XTextRangeCompare>::get() };
    return TYPES;
}

uno::Sequence< sal_Int8 > SAL_CALL SvxUnoTextBase::getImplementationId()
{
    return css::uno::Sequence<sal_Int8>();
}

uno::Reference< text::XTextCursor > SvxUnoTextBase::createTextCursorBySelection( const ESelection& rSel )
{
    rtl::Reference<SvxUnoTextCursor> pCursor = new SvxUnoTextCursor( *this );
    pCursor->SetSelection( rSel );
    return pCursor;
}

// XSimpleText

uno::Reference< text::XTextCursor > SAL_CALL SvxUnoTextBase::createTextCursor()
{
    SolarMutexGuard aGuard;
    return new SvxUnoTextCursor( *this );
}

uno::Reference< text::XTextCursor > SAL_CALL SvxUnoTextBase::createTextCursorByRange( const uno::Reference< text::XTextRange >& aTextPosition )
{
    SolarMutexGuard aGuard;

    uno::Reference< text::XTextCursor >  xCursor;

    if( aTextPosition.is() )
    {
        SvxUnoTextRangeBase* pRange = comphelper::getFromUnoTunnel<SvxUnoTextRangeBase>( aTextPosition );
        if(pRange)
            xCursor = createTextCursorBySelection( pRange->GetSelection() );
    }

    return xCursor;
}

void SAL_CALL SvxUnoTextBase::insertString( const uno::Reference< text::XTextRange >& xRange, const OUString& aString, sal_Bool bAbsorb )
{
    SolarMutexGuard aGuard;

    if( !xRange.is() )
        return;

    SvxUnoTextRangeBase* pRange = comphelper::getFromUnoTunnel<SvxUnoTextRange>( xRange );
    if(!pRange)
        return;

    // setString on SvxUnoTextRangeBase instead of itself QuickInsertText
    // and UpdateData, so that the selection will be adjusted to
    // SvxUnoTextRangeBase. Actually all cursor objects of this Text must
    // to be statement to be adapted!

    if (!bAbsorb)                   // do not replace -> append on tail
        pRange->CollapseToEnd();

    pRange->setString( aString );

    pRange->CollapseToEnd();

    if (GetEditSource())
    {
        ESelection aSelection;
        ::GetSelection( aSelection, GetEditSource()->GetTextForwarder() );
        SetSelection( aSelection );
    }
}

void SAL_CALL SvxUnoTextBase::insertControlCharacter( const uno::Reference< text::XTextRange >& xRange, sal_Int16 nControlCharacter, sal_Bool bAbsorb )
{
    SolarMutexGuard aGuard;

    SvxTextForwarder* pForwarder = GetEditSource() ? GetEditSource()->GetTextForwarder() : nullptr;

    if( !pForwarder )
        return;

    ESelection aSelection;
    ::GetSelection( aSelection, pForwarder );
    SetSelection( aSelection );

    switch( nControlCharacter )
    {
    case text::ControlCharacter::PARAGRAPH_BREAK:
    {
        insertString( xRange, u"\x0D"_ustr, bAbsorb );

        return;
    }
    case text::ControlCharacter::LINE_BREAK:
    {
        SvxUnoTextRangeBase* pRange = comphelper::getFromUnoTunnel<SvxUnoTextRange>( xRange );
        if(pRange)
        {
            ESelection aRange = pRange->GetSelection();

            if( bAbsorb )
            {
                pForwarder->QuickInsertText( u""_ustr, aRange );

                aRange.CollapseToStart();
            }
            else
            {
                aRange.CollapseToEnd();
            }

            pForwarder->QuickInsertLineBreak( aRange );
            GetEditSource()->UpdateData();

            aRange.end.nIndex += 1;
            if( !bAbsorb )
                aRange.start.nIndex += 1;

            pRange->SetSelection( aRange );
        }
        return;
    }
    case text::ControlCharacter::APPEND_PARAGRAPH:
    {
        SvxUnoTextRangeBase* pRange = comphelper::getFromUnoTunnel<SvxUnoTextRange>( xRange );
        if(pRange)
        {
            ESelection aRange = pRange->GetSelection();
//              ESelection aOldSelection = aRange;

            aRange.start.nIndex  = pForwarder->GetTextLen( aRange.start.nPara );

            aRange.CollapseToStart();

            pRange->SetSelection( aRange );
            static constexpr OUStringLiteral CR = u"\x0D";
            pRange->setString( CR );

            aRange.start.nIndex = 0;
            aRange.start.nPara += 1;
            aRange.end.nIndex = 0;
            aRange.end.nPara += 1;

            pRange->SetSelection( aRange );

            return;
        }
        [[fallthrough]];
    }
    default:
        throw lang::IllegalArgumentException();
    }
}

// XText
void SAL_CALL SvxUnoTextBase::insertTextContent( const uno::Reference< text::XTextRange >& xRange, const uno::Reference< text::XTextContent >& xContent, sal_Bool bAbsorb )
{
    SolarMutexGuard aGuard;

    SvxTextForwarder* pForwarder = GetEditSource() ? GetEditSource()->GetTextForwarder() : nullptr;
    if (!pForwarder)
        return;

    uno::Reference<beans::XPropertySet> xPropSet(xRange, uno::UNO_QUERY);
    if (!xPropSet.is())
        throw lang::IllegalArgumentException();

    uno::Any aAny = xPropSet->getPropertyValue(UNO_TR_PROP_SELECTION);
    text::TextRangeSelection aSel = aAny.get<text::TextRangeSelection>();
    if (!bAbsorb)
        aSel.Start = aSel.End;

    std::unique_ptr<SvxFieldData> pFieldData(SvxFieldData::Create(xContent));
    if (!pFieldData)
        throw lang::IllegalArgumentException();

    SvxFieldItem aField( *pFieldData, EE_FEATURE_FIELD );
    pForwarder->QuickInsertField(aField, toESelection(aSel));
    GetEditSource()->UpdateData();

    uno::Reference<beans::XPropertySet> xPropSetContent(xContent, uno::UNO_QUERY);
    if (!xPropSetContent.is())
        throw lang::IllegalArgumentException();

    xPropSetContent->setPropertyValue(UNO_TC_PROP_ANCHOR, uno::Any(xRange));

    aSel.End.PositionInParagraph += 1;
    aSel.Start.PositionInParagraph = aSel.End.PositionInParagraph;
    xPropSet->setPropertyValue(UNO_TR_PROP_SELECTION, uno::Any(aSel));
}

void SAL_CALL SvxUnoTextBase::removeTextContent( const uno::Reference< text::XTextContent >& )
{
}

// XTextRange

uno::Reference< text::XText > SAL_CALL SvxUnoTextBase::getText()
{
    SolarMutexGuard aGuard;

    if (GetEditSource())
    {
        ESelection aSelection;
        ::GetSelection( aSelection, GetEditSource()->GetTextForwarder() );
        SetSelection( aSelection );
    }

    return static_cast<text::XText*>(this);
}

uno::Reference< text::XTextRange > SAL_CALL SvxUnoTextBase::getStart()
{
    return SvxUnoTextRangeBase::getStart();
}

uno::Reference< text::XTextRange > SAL_CALL SvxUnoTextBase::getEnd()
{
    return SvxUnoTextRangeBase::getEnd();
}

OUString SAL_CALL SvxUnoTextBase::getString()
{
    return SvxUnoTextRangeBase::getString();
}

void SAL_CALL SvxUnoTextBase::setString( const OUString& aString )
{
    SvxUnoTextRangeBase::setString(aString);
}


// XEnumerationAccess
uno::Reference< container::XEnumeration > SAL_CALL SvxUnoTextBase::createEnumeration()
{
    SolarMutexGuard aGuard;

    if (!GetEditSource())
        return uno::Reference< container::XEnumeration >();

    if (maSelection == ESelection(0, 0, 0, 0) || maSelection == ESelection(EE_PARA_MAX, 0, 0, 0))
    {
        ESelection aSelection;
        ::GetSelection( aSelection, GetEditSource()->GetTextForwarder() );
        return new SvxUnoTextContentEnumeration(*this, aSelection);
    }
    else
    {
        return new SvxUnoTextContentEnumeration(*this, maSelection);
    }
}

// XElementAccess ( container::XEnumerationAccess )
uno::Type SAL_CALL SvxUnoTextBase::getElementType(  )
{
    return cppu::UnoType<text::XTextRange>::get();
}

sal_Bool SAL_CALL SvxUnoTextBase::hasElements(  )
{
    SolarMutexGuard aGuard;

    if(GetEditSource())
    {
        SvxTextForwarder* pForwarder = GetEditSource()->GetTextForwarder();
        if(pForwarder)
            return pForwarder->GetParagraphCount() != 0;
    }

    return false;
}

// text::XTextRangeMover
void SAL_CALL SvxUnoTextBase::moveTextRange( const uno::Reference< text::XTextRange >&, sal_Int16 )
{
}

/// @throws lang::IllegalArgumentException
/// @throws beans::UnknownPropertyException
/// @throws uno::RuntimeException
static void SvxPropertyValuesToItemSet(
        SfxItemSet &rItemSet,
        const uno::Sequence< beans::PropertyValue >& rPropertyValues,
        const SfxItemPropertySet *pPropSet,
        SvxTextForwarder *pForwarder,
        sal_Int32 nPara)
{
    for (const beans::PropertyValue& rProp : rPropertyValues)
    {
        const SfxItemPropertyMapEntry *pEntry = pPropSet->getPropertyMap().getByName( rProp.Name );
        if (!pEntry)
            throw beans::UnknownPropertyException( "Unknown property: " + rProp.Name );
        // Note: there is no need to take special care of the properties
        //      TextField (EE_FEATURE_FIELD) and
        //      TextPortionType (WID_PORTIONTYPE)
        //  since they are read-only and thus are already taken care of below.

        if (pEntry->nFlags & beans::PropertyAttribute::READONLY)
            // should be PropertyVetoException which is not yet defined for the new import API's functions
            throw uno::RuntimeException("Property is read-only: " + rProp.Name );
            //throw PropertyVetoException ("Property is read-only: " + rProp.Name );

        if (pEntry->nWID == WID_FONTDESC)
        {
            awt::FontDescriptor aDesc;
            if (rProp.Value >>= aDesc)
                SvxUnoFontDescriptor::FillItemSet( aDesc, rItemSet );
        }
        else if (pEntry->nWID == WID_NUMBERINGSTARTVALUE )
        {
            if( pForwarder )
            {
                sal_Int16 nStartValue = -1;
                if( !(rProp.Value >>= nStartValue) )
                    throw lang::IllegalArgumentException();

                pForwarder->SetNumberingStartValue( nPara, nStartValue );
            }
        }
        else if (pEntry->nWID == WID_PARAISNUMBERINGRESTART )
        {
            if( pForwarder )
            {
                bool bParaIsNumberingRestart = false;
                if( !(rProp.Value >>= bParaIsNumberingRestart) )
                    throw lang::IllegalArgumentException();

                pForwarder->SetParaIsNumberingRestart( nPara, bParaIsNumberingRestart );
            }
        }
        else
            pPropSet->setPropertyValue( rProp.Name, rProp.Value, rItemSet );
    }
}

uno::Reference< text::XTextRange > SAL_CALL SvxUnoTextBase::finishParagraphInsert(
        const uno::Sequence< beans::PropertyValue >& /*rCharAndParaProps*/,
        const uno::Reference< text::XTextRange >& /*rTextRange*/ )
{
    uno::Reference< text::XTextRange > xRet;
    return xRet;
}

uno::Reference< text::XTextRange > SAL_CALL SvxUnoTextBase::finishParagraph(
        const uno::Sequence< beans::PropertyValue >& rCharAndParaProps )
{
    SolarMutexGuard aGuard;

    SvxEditSource *pEditSource = GetEditSource();
    SvxTextForwarder *pTextForwarder = pEditSource ? pEditSource->GetTextForwarder() : nullptr;
    if (!pTextForwarder)
        return nullptr;

    sal_Int32 nParaCount = pTextForwarder->GetParagraphCount();
    DBG_ASSERT( nParaCount > 0, "paragraph count is 0 or negative" );
    pTextForwarder->AppendParagraph();

    // set properties for the previously last paragraph
    sal_Int32 nPara = nParaCount - 1;
    ESelection aSel(nPara, 0);
    SfxItemSet aItemSet( *pTextForwarder->GetEmptyItemSetPtr() );
    SvxPropertyValuesToItemSet( aItemSet, rCharAndParaProps,
            ImplGetSvxUnoOutlinerTextCursorSfxPropertySet(), pTextForwarder, nPara );
    pTextForwarder->QuickSetAttribs( aItemSet, aSel );
    pEditSource->UpdateData();
    rtl::Reference<SvxUnoTextRange> pRange = new SvxUnoTextRange( *this );
    pRange->SetSelection( aSel );
    return pRange;
}

uno::Reference< text::XTextRange > SAL_CALL SvxUnoTextBase::insertTextPortion(
        const OUString& rText,
        const uno::Sequence< beans::PropertyValue >& rCharAndParaProps,
        const uno::Reference< text::XTextRange>& rTextRange )
{
    SolarMutexGuard aGuard;

    if (!rTextRange.is())
        return nullptr;

    SvxUnoTextRangeBase* pRange = comphelper::getFromUnoTunnel<SvxUnoTextRange>(rTextRange);
    if (!pRange)
        return nullptr;

    SvxEditSource *pEditSource = GetEditSource();
    SvxTextForwarder *pTextForwarder = pEditSource ? pEditSource->GetTextForwarder() : nullptr;

    if (!pTextForwarder)
        return nullptr;

    pRange->setString(rText);

    ESelection aSelection(pRange->GetSelection());

    pTextForwarder->RemoveAttribs(aSelection);
    pEditSource->UpdateData();

    SfxItemSet aItemSet( *pTextForwarder->GetEmptyItemSetPtr() );
    SvxPropertyValuesToItemSet( aItemSet, rCharAndParaProps,
            ImplGetSvxTextPortionSfxPropertySet(), pTextForwarder, aSelection.start.nPara );
    pTextForwarder->QuickSetAttribs( aItemSet, aSelection);
    rtl::Reference<SvxUnoTextRange> pNewRange = new SvxUnoTextRange( *this );
    pNewRange->SetSelection(aSelection);
    for( const beans::PropertyValue& rProp : rCharAndParaProps )
        pNewRange->setPropertyValue( rProp.Name, rProp.Value );
    return pNewRange;
}

// css::text::XTextPortionAppend (new import API)
uno::Reference< text::XTextRange > SAL_CALL SvxUnoTextBase::appendTextPortion(
        const OUString& rText,
        const uno::Sequence< beans::PropertyValue >& rCharAndParaProps )
{
    SolarMutexGuard aGuard;

    SvxEditSource *pEditSource = GetEditSource();
    SvxTextForwarder *pTextForwarder = pEditSource ? pEditSource->GetTextForwarder() : nullptr;
    if (!pTextForwarder)
        return nullptr;

    sal_Int32 nParaCount = pTextForwarder->GetParagraphCount();
    DBG_ASSERT( nParaCount > 0, "paragraph count is 0 or negative" );
    sal_Int32 nPara = nParaCount - 1;
    SfxItemSet aSet( pTextForwarder->GetParaAttribs( nPara ) );
    sal_Int32 nStart = pTextForwarder->AppendTextPortion( nPara, rText, aSet );
    pEditSource->UpdateData();
    sal_Int32 nEnd   = pTextForwarder->GetTextLen( nPara );

    // set properties for the new text portion
    ESelection aSel( nPara, nStart, nPara, nEnd );
    pTextForwarder->RemoveAttribs( aSel );
    pEditSource->UpdateData();

    SfxItemSet aItemSet( *pTextForwarder->GetEmptyItemSetPtr() );
    SvxPropertyValuesToItemSet( aItemSet, rCharAndParaProps,
            ImplGetSvxTextPortionSfxPropertySet(), pTextForwarder, nPara );
    pTextForwarder->QuickSetAttribs( aItemSet, aSel );
    rtl::Reference<SvxUnoTextRange> pRange = new SvxUnoTextRange( *this );
    pRange->SetSelection( aSel );
    for( const beans::PropertyValue& rProp : rCharAndParaProps )
        pRange->setPropertyValue( rProp.Name, rProp.Value );
    return pRange;
}

void SvxUnoTextBase::copyText(
    const uno::Reference< text::XTextCopy >& xSource )
{
    SolarMutexGuard aGuard;
    uno::Reference< lang::XUnoTunnel > xUT( xSource, uno::UNO_QUERY );
    SvxEditSource *pEditSource = GetEditSource();
    SvxTextForwarder *pTextForwarder = pEditSource ? pEditSource->GetTextForwarder() : nullptr;
    if( !pTextForwarder )
        return;
    if (auto pSource = comphelper::getFromUnoTunnel<SvxUnoTextBase>(xUT))
    {
        SvxEditSource *pSourceEditSource = pSource->GetEditSource();
        SvxTextForwarder *pSourceTextForwarder = pSourceEditSource ? pSourceEditSource->GetTextForwarder() : nullptr;
        if( pSourceTextForwarder )
        {
            pTextForwarder->CopyText( *pSourceTextForwarder );
            pEditSource->UpdateData();
            SetSelection(pSource->GetSelection());
        }
    }
    else
    {
        uno::Reference< text::XText > xSourceText( xSource, uno::UNO_QUERY );
        if( xSourceText.is() )
        {
            setString( xSourceText->getString() );
        }
    }
}

// lang::XServiceInfo
OUString SAL_CALL SvxUnoTextBase::getImplementationName()
{
    return u"SvxUnoTextBase"_ustr;
}

uno::Sequence< OUString > SAL_CALL SvxUnoTextBase::getSupportedServiceNames(  )
{
    return getSupportedServiceNames_Static();
}

uno::Sequence< OUString > SAL_CALL SvxUnoTextBase::getSupportedServiceNames_Static(  )
{
    return comphelper::concatSequences(
        SvxUnoTextRangeBase::getSupportedServiceNames_Static(),
        std::initializer_list<OUString>{ u"com.sun.star.text.Text"_ustr });
}

const uno::Sequence< sal_Int8 > & SvxUnoTextBase::getUnoTunnelId() noexcept
{
    static const comphelper::UnoIdInit theSvxUnoTextBaseUnoTunnelId;
    return theSvxUnoTextBaseUnoTunnelId.getSeq();
}

sal_Int64 SAL_CALL SvxUnoTextBase::getSomething( const uno::Sequence< sal_Int8 >& rId )
{
    return comphelper::getSomethingImpl(
        rId, this, comphelper::FallbackToGetSomethingOf<SvxUnoTextRangeBase>{});
}

SvxUnoText::SvxUnoText( const SvxItemPropertySet* _pSet ) noexcept
: SvxUnoTextBase( _pSet )
{
}

SvxUnoText::SvxUnoText( const SvxEditSource* pSource, const SvxItemPropertySet* _pSet, uno::Reference < text::XText > const & xParent ) noexcept
: SvxUnoTextBase( pSource, _pSet, xParent )
{
}

SvxUnoText::SvxUnoText( const SvxUnoText& rText ) noexcept
: SvxUnoTextBase( rText )
, cppu::OWeakAggObject()
{
}

SvxUnoText::~SvxUnoText() noexcept
{
}

// uno::XInterface
uno::Any SAL_CALL SvxUnoText::queryAggregation( const uno::Type & rType )
{
    uno::Any aAny( SvxUnoTextBase::queryAggregation( rType ) );
    if( !aAny.hasValue() )
        aAny = OWeakAggObject::queryAggregation( rType );

    return aAny;
}

uno::Any SAL_CALL SvxUnoText::queryInterface( const uno::Type & rType )
{
    return OWeakAggObject::queryInterface( rType );
}

void SAL_CALL SvxUnoText::acquire() noexcept
{
    OWeakAggObject::acquire();
}

void SAL_CALL SvxUnoText::release() noexcept
{
    OWeakAggObject::release();
}

// lang::XTypeProvider
uno::Sequence< uno::Type > SAL_CALL SvxUnoText::getTypes(  )
{
    return SvxUnoTextBase::getTypes();
}

uno::Sequence< sal_Int8 > SAL_CALL SvxUnoText::getImplementationId(  )
{
    return css::uno::Sequence<sal_Int8>();
}

const uno::Sequence< sal_Int8 > & SvxUnoText::getUnoTunnelId() noexcept
{
    static const comphelper::UnoIdInit theSvxUnoTextUnoTunnelId;
    return theSvxUnoTextUnoTunnelId.getSeq();
}

sal_Int64 SAL_CALL SvxUnoText::getSomething( const uno::Sequence< sal_Int8 >& rId )
{
    return comphelper::getSomethingImpl(rId, this,
                                        comphelper::FallbackToGetSomethingOf<SvxUnoTextBase>{});
}


SvxDummyTextSource::~SvxDummyTextSource()
{
};

std::unique_ptr<SvxEditSource> SvxDummyTextSource::Clone() const
{
    return std::unique_ptr<SvxEditSource>(new SvxDummyTextSource);
}

SvxTextForwarder* SvxDummyTextSource::GetTextForwarder()
{
    return this;
}

void SvxDummyTextSource::UpdateData()
{
}

sal_Int32 SvxDummyTextSource::GetParagraphCount() const
{
    return 0;
}

sal_Int32 SvxDummyTextSource::GetTextLen( sal_Int32 ) const
{
    return 0;
}

OUString SvxDummyTextSource::GetText( const ESelection& ) const
{
    return OUString();
}

SfxItemSet SvxDummyTextSource::GetAttribs( const ESelection&, EditEngineAttribs ) const
{
    // Very dangerous: The former implementation used a SfxItemPool created on the
    // fly which of course was deleted again ASAP. Thus, the returned SfxItemSet was using
    // a deleted Pool by design.
    return SfxItemSet(EditEngine::GetGlobalItemPool());
}

SfxItemSet SvxDummyTextSource::GetParaAttribs( sal_Int32 ) const
{
    return GetAttribs(ESelection());
}

void SvxDummyTextSource::SetParaAttribs( sal_Int32, const SfxItemSet& )
{
}

void SvxDummyTextSource::RemoveAttribs( const ESelection& )
{
}

void SvxDummyTextSource::GetPortions( sal_Int32, std::vector<sal_Int32>& ) const
{
}

OUString SvxDummyTextSource::GetStyleSheet(sal_Int32) const
{
    return OUString();
}

void SvxDummyTextSource::SetStyleSheet(sal_Int32, const OUString&)
{
}

SfxItemState SvxDummyTextSource::GetItemState( const ESelection&, sal_uInt16 ) const
{
    return SfxItemState::UNKNOWN;
}

SfxItemState SvxDummyTextSource::GetItemState( sal_Int32, sal_uInt16 ) const
{
    return SfxItemState::UNKNOWN;
}

SfxItemPool* SvxDummyTextSource::GetPool() const
{
    return nullptr;
}

void SvxDummyTextSource::QuickInsertText( const OUString&, const ESelection& )
{
}

void SvxDummyTextSource::QuickInsertField( const SvxFieldItem&, const ESelection& )
{
}

void SvxDummyTextSource::QuickSetAttribs( const SfxItemSet&, const ESelection& )
{
}

void SvxDummyTextSource::QuickInsertLineBreak( const ESelection& )
{
};

OUString SvxDummyTextSource::CalcFieldValue( const SvxFieldItem&, sal_Int32, sal_Int32, std::optional<Color>&, std::optional<Color>&, std::optional<FontLineStyle>& )
{
    return OUString();
}

void SvxDummyTextSource::FieldClicked( const SvxFieldItem& )
{
}

bool SvxDummyTextSource::IsValid() const
{
    return false;
}

LanguageType SvxDummyTextSource::GetLanguage( sal_Int32, sal_Int32 ) const
{
    return LANGUAGE_DONTKNOW;
}

std::vector<EFieldInfo> SvxDummyTextSource::GetFieldInfo( sal_Int32 ) const
{
    return {};
}

EBulletInfo SvxDummyTextSource::GetBulletInfo( sal_Int32 ) const
{
    return EBulletInfo();
}

tools::Rectangle SvxDummyTextSource::GetCharBounds( sal_Int32, sal_Int32 ) const
{
    return tools::Rectangle();
}

tools::Rectangle SvxDummyTextSource::GetParaBounds( sal_Int32 ) const
{
    return tools::Rectangle();
}

MapMode SvxDummyTextSource::GetMapMode() const
{
    return MapMode();
}

OutputDevice* SvxDummyTextSource::GetRefDevice() const
{
    return nullptr;
}

bool SvxDummyTextSource::GetIndexAtPoint( const Point&, sal_Int32&, sal_Int32& ) const
{
    return false;
}

bool SvxDummyTextSource::GetWordIndices( sal_Int32, sal_Int32, sal_Int32&, sal_Int32& ) const
{
    return false;
}

bool SvxDummyTextSource::GetAttributeRun( sal_Int32&, sal_Int32&, sal_Int32, sal_Int32, bool ) const
{
    return false;
}

sal_Int32 SvxDummyTextSource::GetLineCount( sal_Int32 ) const
{
    return 0;
}

sal_Int32 SvxDummyTextSource::GetLineLen( sal_Int32, sal_Int32 ) const
{
    return 0;
}

void SvxDummyTextSource::GetLineBoundaries( /*out*/sal_Int32 &rStart, /*out*/sal_Int32 &rEnd, sal_Int32 /*nParagraph*/, sal_Int32 /*nLine*/ ) const
{
    rStart = rEnd = 0;
}

sal_Int32 SvxDummyTextSource::GetLineNumberAtIndex( sal_Int32 /*nPara*/, sal_Int32 /*nIndex*/ ) const
{
    return 0;
}

bool SvxDummyTextSource::QuickFormatDoc( bool )
{
    return false;
}

bool SvxDummyTextSource::SupportsOutlineDepth() const
{
    return false;
}

sal_Int16 SvxDummyTextSource::GetDepth( sal_Int32 ) const
{
    return -1;
}

bool SvxDummyTextSource::SetDepth( sal_Int32, sal_Int16 nNewDepth )
{
    return nNewDepth == 0;
}

bool SvxDummyTextSource::Delete( const ESelection& )
{
    return false;
}

bool SvxDummyTextSource::InsertText( const OUString&, const ESelection& )
{
    return false;
}

const SfxItemSet * SvxDummyTextSource::GetEmptyItemSetPtr()
{
    return nullptr;
}

void SvxDummyTextSource::AppendParagraph()
{
}

sal_Int32 SvxDummyTextSource::AppendTextPortion( sal_Int32, const OUString &, const SfxItemSet & )
{
    return 0;
}

void  SvxDummyTextSource::CopyText(const SvxTextForwarder& )
{
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
