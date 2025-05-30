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

#include <comphelper/propertyvalue.hxx>
#include <osl/diagnose.h>

#include <svx/zoomslideritem.hxx>
#include <com/sun/star/beans/PropertyValue.hpp>


SfxPoolItem* SvxZoomSliderItem::CreateDefault() { return new SvxZoomSliderItem; }

constexpr OUString ZOOMSLIDER_PARAM_CURRENTZOOM = u"Columns"_ustr;
constexpr OUString ZOOMSLIDER_PARAM_SNAPPINGPOINTS = u"SnappingPoints"_ustr;
constexpr OUString ZOOMSLIDER_PARAM_MINZOOM = u"MinValue"_ustr;
constexpr OUString ZOOMSLIDER_PARAM_MAXZOOM = u"MaxValue"_ustr;
#define ZOOMSLIDER_PARAMS           4


SvxZoomSliderItem::SvxZoomSliderItem( sal_uInt16 nCurrentZoom, sal_uInt16 nMinZoom, sal_uInt16 nMaxZoom, TypedWhichId<SvxZoomSliderItem> _nWhich )
:   SfxUInt16Item( _nWhich, nCurrentZoom ), mnMinZoom( nMinZoom ), mnMaxZoom( nMaxZoom )
{
}

SvxZoomSliderItem* SvxZoomSliderItem::Clone( SfxItemPool * /*pPool*/ ) const
{
    return new SvxZoomSliderItem( *this );
}

bool SvxZoomSliderItem::operator==( const SfxPoolItem& rAttr ) const
{
    assert(SfxPoolItem::operator==(rAttr));

    const SvxZoomSliderItem& rItem = static_cast<const SvxZoomSliderItem&>(rAttr);

    return ( GetValue() == rItem.GetValue() && maValues == rItem.maValues &&
             mnMinZoom == rItem.mnMinZoom && mnMaxZoom == rItem.mnMaxZoom );
}

bool SvxZoomSliderItem::QueryValue( css::uno::Any& rVal, sal_uInt8 nMemberId ) const
{
    nMemberId &= ~CONVERT_TWIPS;
    switch ( nMemberId )
    {
        case 0 :
            {
                css::uno::Sequence< css::beans::PropertyValue > aSeq{
                    comphelper::makePropertyValue(ZOOMSLIDER_PARAM_CURRENTZOOM, sal_Int32( GetValue() )),
                    comphelper::makePropertyValue(ZOOMSLIDER_PARAM_SNAPPINGPOINTS, maValues),
                    comphelper::makePropertyValue(ZOOMSLIDER_PARAM_MINZOOM, mnMinZoom),
                    comphelper::makePropertyValue(ZOOMSLIDER_PARAM_MAXZOOM, mnMaxZoom)
                };
                assert(aSeq.getLength() == ZOOMSLIDER_PARAMS);
                rVal <<= aSeq;
            }
            break;

        case MID_ZOOMSLIDER_CURRENTZOOM :
            {
                rVal <<= static_cast<sal_Int32>(GetValue());
            }
            break;
        case MID_ZOOMSLIDER_SNAPPINGPOINTS:
            {
                 rVal <<= maValues;
            }
            break;
        case MID_ZOOMSLIDER_MINZOOM:
            {
                rVal <<= mnMinZoom;
            }
            break;
        case MID_ZOOMSLIDER_MAXZOOM:
            {
                rVal <<= mnMaxZoom;
            }
            break;
        default:
            OSL_FAIL("svx::SvxZoomSliderItem::QueryValue(), Wrong MemberId!");
            return false;
    }

    return true;
}

bool SvxZoomSliderItem::PutValue( const css::uno::Any& rVal, sal_uInt8 nMemberId )
{
    nMemberId &= ~CONVERT_TWIPS;
    switch ( nMemberId )
    {
        case 0 :
            {
                css::uno::Sequence< css::beans::PropertyValue > aSeq;
                if (( rVal >>= aSeq ) && ( aSeq.getLength() == ZOOMSLIDER_PARAMS ))
                {
                    sal_Int32 nCurrentZoom( 0 );
                    css::uno::Sequence < sal_Int32 > aValues;

                    bool bAllConverted( true );
                    sal_Int16 nConvertedCount( 0 );
                    sal_Int32 nMinZoom( 0 ), nMaxZoom( 0 );

                    for (const auto& rProp : aSeq)
                    {
                        if ( rProp.Name == ZOOMSLIDER_PARAM_CURRENTZOOM )
                        {
                            bAllConverted &= ( rProp.Value >>= nCurrentZoom );
                            ++nConvertedCount;
                        }
                        else if ( rProp.Name == ZOOMSLIDER_PARAM_SNAPPINGPOINTS )
                        {
                            bAllConverted &= ( rProp.Value >>= aValues );
                            ++nConvertedCount;
                        }
                        else if( rProp.Name == ZOOMSLIDER_PARAM_MINZOOM )
                        {
                            bAllConverted &= ( rProp.Value >>= nMinZoom );
                            ++nConvertedCount;
                        }
                        else if( rProp.Name == ZOOMSLIDER_PARAM_MAXZOOM )
                        {
                            bAllConverted &= ( rProp.Value >>= nMaxZoom );
                            ++nConvertedCount;
                        }
                    }

                    if ( bAllConverted && nConvertedCount == ZOOMSLIDER_PARAMS )
                    {
                        SetValue( static_cast<sal_uInt16>(nCurrentZoom) );
                        maValues = std::move(aValues);
                        mnMinZoom = sal::static_int_cast< sal_uInt16 >( nMinZoom );
                        mnMaxZoom = sal::static_int_cast< sal_uInt16 >( nMaxZoom );

                        return true;
                    }
                }

                return false;
            }

        case MID_ZOOMSLIDER_CURRENTZOOM:
            {
                sal_Int32 nVal = 0;
                if ( rVal >>= nVal )
                {
                    SetValue( static_cast<sal_uInt16>(nVal) );
                    return true;
                }
                else
                    return false;
            }

        case MID_ZOOMSLIDER_SNAPPINGPOINTS:
            {
                css::uno::Sequence < sal_Int32 > aValues;
                if ( rVal >>= aValues )
                {
                    maValues = std::move(aValues);
                    return true;
                }
                else
                    return false;
            }
        case MID_ZOOMSLIDER_MINZOOM:
            {
                sal_Int32 nVal = 0;
                if( rVal >>= nVal )
                {
                    mnMinZoom = static_cast<sal_uInt16>(nVal);
                    return true;
                }
                else
                    return false;
            }
        case MID_ZOOMSLIDER_MAXZOOM:
            {
                sal_Int32 nVal = 0;
                if( rVal >>= nVal )
                {
                    mnMaxZoom = static_cast<sal_uInt16>(nVal);
                    return true;
                }
                else
                    return false;
            }
        default:
            OSL_FAIL("svx::SvxZoomSliderItem::PutValue(), Wrong MemberId!");
            return false;
    }
}

void SvxZoomSliderItem::AddSnappingPoint( sal_Int32 nNew )
{
    const sal_Int32 nValues = maValues.getLength();
    maValues.realloc(  nValues + 1 );
    sal_Int32* pValues = maValues.getArray();
    pValues[ nValues ] = nNew;
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
