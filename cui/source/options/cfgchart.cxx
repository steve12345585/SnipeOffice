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

#include <com/sun/star/uno/Sequence.hxx>
#include <tools/debug.hxx>
#include <sal/log.hxx>
#include "cfgchart.hxx"
#include <dialmgr.hxx>
#include <strings.hrc>
#include <utility>
#include <officecfg/Office/Chart.hxx>

#define ROW_COLOR_COUNT 12

using namespace com::sun::star;

// accessors
size_t SvxChartColorTable::size() const
{
    return m_aColorEntries.size();
}

const XColorEntry & SvxChartColorTable::operator[]( size_t _nIndex ) const
{
    if ( _nIndex >= m_aColorEntries.size() )
    {
        SAL_WARN( "cui.options", "SvxChartColorTable::[] invalid index" );
        return m_aColorEntries[ 0 ];
    }

    return m_aColorEntries[ _nIndex ];
}

Color SvxChartColorTable::getColor( size_t _nIndex ) const
{
    if ( _nIndex >= m_aColorEntries.size() )
    {
        SAL_WARN( "cui.options", "SvxChartColorTable::getColorData invalid index" );
        return COL_BLACK;
    }

    return m_aColorEntries[ _nIndex ].GetColor().GetRGBColor();
}

// mutators
void SvxChartColorTable::clear()
{
    m_aColorEntries.clear();
}

void SvxChartColorTable::append( const XColorEntry & _rEntry )
{
    m_aColorEntries.push_back( _rEntry );
}

void SvxChartColorTable::remove( size_t _nIndex )
{
    if (!m_aColorEntries.empty())
        m_aColorEntries.erase( m_aColorEntries.begin() + _nIndex);

    for (size_t i=0 ; i<m_aColorEntries.size(); i++)
    {
        m_aColorEntries[ i ].SetName( getDefaultName( i ) );
    }
}

void SvxChartColorTable::replace( size_t _nIndex, const XColorEntry & _rEntry )
{
    DBG_ASSERT( _nIndex <= m_aColorEntries.size(),
                "SvxChartColorTable::replace invalid index" );

    m_aColorEntries[ _nIndex ] = _rEntry;
}

void SvxChartColorTable::useDefault()
{
    static const Color aColors[] = {
        Color( 0x00, 0x45, 0x86 ),
        Color( 0xff, 0x42, 0x0e ),
        Color( 0xff, 0xd3, 0x20 ),
        Color( 0x57, 0x9d, 0x1c ),
        Color( 0x7e, 0x00, 0x21 ),
        Color( 0x83, 0xca, 0xff ),
        Color( 0x31, 0x40, 0x04 ),
        Color( 0xae, 0xcf, 0x00 ),
        Color( 0x4b, 0x1f, 0x6f ),
        Color( 0xff, 0x95, 0x0e ),
        Color( 0xc5, 0x00, 0x0b ),
        Color( 0x00, 0x84, 0xd1 )
    };

    clear();

    for( sal_Int32 i=0; i<ROW_COLOR_COUNT; i++ )
    {
        append( XColorEntry( aColors[ i % sizeof( aColors ) ], getDefaultName( i ) ));
    }
}

OUString SvxChartColorTable::getDefaultName( size_t _nIndex )
{
    OUString aName;

    std::u16string_view sDefaultNamePrefix;
    std::u16string_view sDefaultNamePostfix;
    OUString aResName( CuiResId( RID_CUISTR_DIAGRAM_ROW ) );
    sal_Int32 nPos = aResName.indexOf( "$(ROW)" );
    if( nPos != -1 )
    {
        sDefaultNamePrefix = aResName.subView( 0, nPos );
        sDefaultNamePostfix = aResName.subView( nPos + sizeof( "$(ROW)" ) - 1 );
    }
    else
    {
        sDefaultNamePrefix = aResName;
    }

    aName = sDefaultNamePrefix + OUString::number(_nIndex + 1) + sDefaultNamePostfix;

    return aName;
}

// comparison
bool SvxChartColorTable::operator==( const SvxChartColorTable & _rOther ) const
{
    // note: XColorEntry has no operator ==
    bool bEqual = ( m_aColorEntries.size() == _rOther.m_aColorEntries.size() );

    if( bEqual )
    {
        for( size_t i = 0; i < m_aColorEntries.size(); ++i )
        {
            if( getColor( i ) != _rOther.getColor( i ))
            {
                bEqual = false;
                break;
            }
        }
    }

    return bEqual;
}




SvxChartColorTable SvxChartOptions::GetDefaultColors()
{
    // 1. default colors for series
    uno::Sequence< sal_Int64 > aColorSeq = officecfg::Office::Chart::DefaultColor::Series::get();

    sal_Int32 nCount = aColorSeq.getLength();
    Color aCol;

    // create strings for entry names
    OUString aResName( CuiResId( RID_CUISTR_DIAGRAM_ROW ) );
    std::u16string_view aPrefix, aPostfix;
    OUString aName;
    sal_Int32 nPos = aResName.indexOf( "$(ROW)" );
    if( nPos != -1 )
    {
        aPrefix = aResName.subView( 0, nPos );
        sal_Int32 idx = nPos + sizeof( "$(ROW)" ) - 1;
        aPostfix = aResName.subView( idx );
    }
    else
        aPrefix = aResName;

    // set color values
    SvxChartColorTable  aDefColors;
    for( sal_Int32 i=0; i < nCount; i++ )
    {
        aCol = Color(ColorTransparency, aColorSeq[ i ]);

        aName = aPrefix + OUString::number(i + 1) + aPostfix;

        aDefColors.append( XColorEntry( aCol, aName ));
    }

    return aDefColors;
}

void SvxChartOptions::SetDefaultColors( const SvxChartColorTable& rDefColors )
{
    // 1. default colors for series
    // convert list to sequence
    const size_t nCount = rDefColors.size();
    uno::Sequence< sal_Int64 > aColors( nCount );
    auto aColorsRange = asNonConstRange(aColors);
    for( size_t i=0; i < nCount; i++ )
    {
        Color aData = rDefColors.getColor( i );
        aColorsRange[ i ] = sal_uInt32(aData);
    }
    std::shared_ptr<comphelper::ConfigurationChanges> batch(comphelper::ConfigurationChanges::create());
    officecfg::Office::Chart::DefaultColor::Series::set(aColors, batch);
    batch->commit();
}


SvxChartColorTableItem::SvxChartColorTableItem( sal_uInt16 nWhich_, SvxChartColorTable aTable ) :
    SfxPoolItem( nWhich_ ),
    m_aColorTable(std::move( aTable ))
{
}

SvxChartColorTableItem* SvxChartColorTableItem::Clone( SfxItemPool * ) const
{
    return new SvxChartColorTableItem( *this );
}

bool SvxChartColorTableItem::operator==( const SfxPoolItem& rAttr ) const
{
    assert(SfxPoolItem::operator==(rAttr));

    return static_cast<const SvxChartColorTableItem & >( rAttr ).m_aColorTable == m_aColorTable;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
