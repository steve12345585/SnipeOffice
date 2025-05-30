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


#include <editeng/eeitem.hxx>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/awt/FontDescriptor.hpp>

#include <editeng/fontitem.hxx>
#include <editeng/fhgtitem.hxx>
#include <editeng/postitem.hxx>
#include <editeng/udlnitem.hxx>
#include <editeng/wghtitem.hxx>
#include <editeng/crossedoutitem.hxx>
#include <editeng/wrlmitem.hxx>
#include <editeng/memberids.h>
#include <svl/itempool.hxx>
#include <vcl/font.hxx>
#include <vcl/unohelp.hxx>
#include <tools/gen.hxx>

#include <editeng/unofdesc.hxx>

using namespace ::com::sun::star;


void SvxUnoFontDescriptor::ConvertToFont( const awt::FontDescriptor& rDesc, vcl::Font& rFont )
{
    rFont.SetFamilyName( rDesc.Name );
    rFont.SetStyleName( rDesc.StyleName );
    rFont.SetFontSize( Size( rDesc.Width, rDesc.Height ) );
    rFont.SetFamily( static_cast<FontFamily>(rDesc.Family) );
    rFont.SetCharSet( static_cast<rtl_TextEncoding>(rDesc.CharSet) );
    rFont.SetPitch( static_cast<FontPitch>(rDesc.Pitch) );
    rFont.SetOrientation( Degree10(static_cast<sal_Int16>(rDesc.Orientation*10)) );
    rFont.SetKerning( rDesc.Kerning ? FontKerning::FontSpecific : FontKerning::NONE );
    rFont.SetWeight( vcl::unohelper::ConvertFontWeight(rDesc.Weight) );
    rFont.SetItalic( static_cast<FontItalic>(rDesc.Slant) );
    rFont.SetUnderline( static_cast<FontLineStyle>(rDesc.Underline) );
    rFont.SetStrikeout( static_cast<FontStrikeout>(rDesc.Strikeout) );
    rFont.SetWordLineMode( rDesc.WordLineMode );
}

void SvxUnoFontDescriptor::ConvertFromFont( const vcl::Font& rFont, awt::FontDescriptor& rDesc )
{
    rDesc.Name = rFont.GetFamilyName();
    rDesc.StyleName = rFont.GetStyleName();
    rDesc.Width = sal::static_int_cast< sal_Int16 >(rFont.GetFontSize().Width());
    rDesc.Height = sal::static_int_cast< sal_Int16 >(rFont.GetFontSize().Height());
    rDesc.Family = sal::static_int_cast< sal_Int16 >(rFont.GetFamilyType());
    rDesc.CharSet = rFont.GetCharSet();
    rDesc.Pitch = sal::static_int_cast< sal_Int16 >(rFont.GetPitch());
    rDesc.Orientation = static_cast< float >(rFont.GetOrientation().get() / 10);
    rDesc.Kerning = rFont.IsKerning();
    rDesc.Weight = vcl::unohelper::ConvertFontWeight( rFont.GetWeight() );
    rDesc.Slant = vcl::unohelper::ConvertFontSlant( rFont.GetItalic() );
    rDesc.Underline = sal::static_int_cast< sal_Int16 >(rFont.GetUnderline());
    rDesc.Strikeout = sal::static_int_cast< sal_Int16 >(rFont.GetStrikeout());
    rDesc.WordLineMode = rFont.IsWordLineMode();
}

void SvxUnoFontDescriptor::FillItemSet( const awt::FontDescriptor& rDesc, SfxItemSet& rSet )
{
    uno::Any aTemp;

    {
        SvxFontItem aFontItem( EE_CHAR_FONTINFO );
        aFontItem.SetFamilyName( rDesc.Name);
        aFontItem.SetStyleName( rDesc.StyleName);
        aFontItem.SetFamily( static_cast<FontFamily>(rDesc.Family));
        aFontItem.SetCharSet( rDesc.CharSet );
        aFontItem.SetPitch( static_cast<FontPitch>(rDesc.Pitch));
        rSet.Put(aFontItem);
    }

    {
        SvxFontHeightItem aFontHeightItem( 0, 100, EE_CHAR_FONTHEIGHT );
        aTemp <<= static_cast<float>(rDesc.Height);
        static_cast<SfxPoolItem*>(&aFontHeightItem)->PutValue( aTemp, MID_FONTHEIGHT|CONVERT_TWIPS );
        rSet.Put(aFontHeightItem);
    }

    {
        SvxPostureItem aPostureItem( ITALIC_NONE, EE_CHAR_ITALIC );
        aTemp <<= rDesc.Slant;
        static_cast<SfxPoolItem*>(&aPostureItem)->PutValue( aTemp, MID_POSTURE );
        rSet.Put(aPostureItem);
    }

    {
        SvxUnderlineItem aUnderlineItem( LINESTYLE_NONE, EE_CHAR_UNDERLINE );
        aTemp <<= rDesc.Underline;
        static_cast<SfxPoolItem*>(&aUnderlineItem)->PutValue( aTemp, MID_TL_STYLE );
        rSet.Put( aUnderlineItem );
    }

    {
        SvxWeightItem aWeightItem( WEIGHT_DONTKNOW, EE_CHAR_WEIGHT );
        aTemp <<= rDesc.Weight;
        static_cast<SfxPoolItem*>(&aWeightItem)->PutValue( aTemp, MID_WEIGHT );
        rSet.Put( aWeightItem );
    }

    {
        SvxCrossedOutItem aCrossedOutItem( STRIKEOUT_NONE, EE_CHAR_STRIKEOUT );
        aTemp <<= rDesc.Strikeout;
        static_cast<SfxPoolItem*>(&aCrossedOutItem)->PutValue( aTemp, MID_CROSS_OUT );
        rSet.Put( aCrossedOutItem );
    }

    {
        SvxWordLineModeItem aWLMItem( rDesc.WordLineMode, EE_CHAR_WLM );
        rSet.Put( aWLMItem );
    }
}

void SvxUnoFontDescriptor::FillFromItemSet( const SfxItemSet& rSet, awt::FontDescriptor& rDesc )
{
    const SfxPoolItem* pItem = nullptr;
    {
        const SvxFontItem* pFontItem = &rSet.Get( EE_CHAR_FONTINFO );
        rDesc.Name      = pFontItem->GetFamilyName();
        rDesc.StyleName = pFontItem->GetStyleName();
        rDesc.Family    = sal::static_int_cast< sal_Int16 >(
            pFontItem->GetFamily());
        rDesc.CharSet   = pFontItem->GetCharSet();
        rDesc.Pitch     = sal::static_int_cast< sal_Int16 >(
            pFontItem->GetPitch());
    }
    {
        pItem = &rSet.Get( EE_CHAR_FONTHEIGHT );
        uno::Any aHeight;
        if( pItem->QueryValue( aHeight, MID_FONTHEIGHT ) )
            aHeight >>= rDesc.Height;
    }
    {
        pItem = &rSet.Get( EE_CHAR_ITALIC );
        uno::Any aFontSlant;
        if(pItem->QueryValue( aFontSlant, MID_POSTURE ))
            aFontSlant >>= rDesc.Slant;
    }
    {
        pItem = &rSet.Get( EE_CHAR_UNDERLINE );
        uno::Any aUnderline;
        if(pItem->QueryValue( aUnderline, MID_TL_STYLE ))
            aUnderline >>= rDesc.Underline;
    }
    {
        pItem = &rSet.Get( EE_CHAR_WEIGHT );
        uno::Any aWeight;
        if(pItem->QueryValue( aWeight, MID_WEIGHT ))
            aWeight >>= rDesc.Weight;
    }
    {
        pItem = &rSet.Get( EE_CHAR_STRIKEOUT );
        uno::Any aStrikeOut;
        if(pItem->QueryValue( aStrikeOut, MID_CROSS_OUT ))
            aStrikeOut >>= rDesc.Strikeout;
    }
    {
        const SvxWordLineModeItem* pWLMItem = &rSet.Get( EE_CHAR_WLM );
        rDesc.WordLineMode = pWLMItem->GetValue();
    }
}

void SvxUnoFontDescriptor::setPropertyToDefault( SfxItemSet& rSet )
{
    rSet.InvalidateItem( EE_CHAR_FONTINFO );
    rSet.InvalidateItem( EE_CHAR_FONTHEIGHT );
    rSet.InvalidateItem( EE_CHAR_ITALIC );
    rSet.InvalidateItem( EE_CHAR_UNDERLINE );
    rSet.InvalidateItem( EE_CHAR_WEIGHT );
    rSet.InvalidateItem( EE_CHAR_STRIKEOUT );
    rSet.InvalidateItem( EE_CHAR_WLM );
}

uno::Any SvxUnoFontDescriptor::getPropertyDefault( SfxItemPool* pPool )
{
    SfxItemSetFixed<
            EE_CHAR_FONTINFO, EE_CHAR_FONTHEIGHT,
            EE_CHAR_WEIGHT, EE_CHAR_ITALIC,
            EE_CHAR_WLM, EE_CHAR_WLM>  aSet(*pPool);

    uno::Any aAny;

    if(!SfxItemPool::IsWhich(EE_CHAR_FONTINFO)||
        !SfxItemPool::IsWhich(EE_CHAR_FONTHEIGHT)||
        !SfxItemPool::IsWhich(EE_CHAR_ITALIC)||
        !SfxItemPool::IsWhich(EE_CHAR_UNDERLINE)||
        !SfxItemPool::IsWhich(EE_CHAR_WEIGHT)||
        !SfxItemPool::IsWhich(EE_CHAR_STRIKEOUT)||
        !SfxItemPool::IsWhich(EE_CHAR_WLM))
        return aAny;

    aSet.Put(pPool->GetUserOrPoolDefaultItem(EE_CHAR_FONTINFO));
    aSet.Put(pPool->GetUserOrPoolDefaultItem(EE_CHAR_FONTHEIGHT));
    aSet.Put(pPool->GetUserOrPoolDefaultItem(EE_CHAR_ITALIC));
    aSet.Put(pPool->GetUserOrPoolDefaultItem(EE_CHAR_UNDERLINE));
    aSet.Put(pPool->GetUserOrPoolDefaultItem(EE_CHAR_WEIGHT));
    aSet.Put(pPool->GetUserOrPoolDefaultItem(EE_CHAR_STRIKEOUT));
    aSet.Put(pPool->GetUserOrPoolDefaultItem(EE_CHAR_WLM));

    awt::FontDescriptor aDesc;

    FillFromItemSet( aSet, aDesc );

    aAny <<= aDesc;

    return aAny;
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
