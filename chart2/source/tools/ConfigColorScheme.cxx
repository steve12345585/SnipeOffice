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

#include <ConfigColorScheme.hxx>

#include <unotools/configitem.hxx>
#include <cppuhelper/supportsservice.hxx>

using namespace ::com::sun::star;

using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;

namespace
{

constexpr OUString aSeriesPropName = u"Series"_ustr;

} // anonymous namespace

namespace chart
{

uno::Reference< chart2::XColorScheme > createConfigColorScheme( const uno::Reference< uno::XComponentContext > & xContext )
{
    return new ConfigColorScheme( xContext );
}

namespace impl
{
class ChartConfigItem : public ::utl::ConfigItem
{
public:
    explicit ChartConfigItem( ConfigColorScheme & rListener );

    uno::Any getProperty( const OUString & aPropertyName );

protected:
    // ____ ::utl::ConfigItem ____
    virtual void ImplCommit() override;
    virtual void Notify( const Sequence< OUString > & aPropertyNames ) override;

private:
    ConfigColorScheme &      m_rListener;
};

ChartConfigItem::ChartConfigItem( ConfigColorScheme & rListener ) :
        ::utl::ConfigItem( u"Office.Chart/DefaultColor"_ustr ),
    m_rListener( rListener )
{
    EnableNotification( { aSeriesPropName } );
}

void ChartConfigItem::Notify( const Sequence< OUString > & aPropertyNames )
{
    for( OUString const & s : aPropertyNames )
    {
        if( s == aSeriesPropName )
            m_rListener.notify();
    }
}

void ChartConfigItem::ImplCommit()
{}

uno::Any ChartConfigItem::getProperty( const OUString & aPropertyName )
{
    Sequence< uno::Any > aValues(
        GetProperties( Sequence< OUString >( &aPropertyName, 1 )));
    if( ! aValues.hasElements())
        return uno::Any();
    return aValues[0];
}

} // namespace impl

// explicit
ConfigColorScheme::ConfigColorScheme(
    const Reference< uno::XComponentContext > & xContext ) :
        m_xContext( xContext  ),
        m_nNumberOfColors( 0 ),
        m_bNeedsUpdate( true )
{
}

ConfigColorScheme::~ConfigColorScheme()
{}

void ConfigColorScheme::retrieveConfigColors()
{
    if( ! m_xContext.is())
        return;

    // create config item if necessary
    if (!m_apChartConfigItem)
    {
        m_apChartConfigItem.reset(
            new impl::ChartConfigItem( *this ));
    }
    assert(m_apChartConfigItem && "this can only be set at this point");

    // retrieve colors
    uno::Any aValue(
        m_apChartConfigItem->getProperty( aSeriesPropName ));
    if( aValue >>= m_aColorSequence )
        m_nNumberOfColors = m_aColorSequence.getLength();
    m_bNeedsUpdate = false;
}

// ____ XColorScheme ____
::sal_Int32 SAL_CALL ConfigColorScheme::getColorByIndex( ::sal_Int32 nIndex )
{
    if( m_bNeedsUpdate )
        retrieveConfigColors();

    if( m_nNumberOfColors > 0 )
        return static_cast< sal_Int32 >( m_aColorSequence[ nIndex % m_nNumberOfColors ] );

    // fall-back: hard-coded standard colors
    static const sal_Int32 nDefaultColors[] =  {
        0x9999ff, 0x993366, 0xffffcc,
        0xccffff, 0x660066, 0xff8080,
        0x0066cc, 0xccccff, 0x000080,
        0xff00ff, 0x00ffff, 0xffff00
    };

    static const sal_Int32 nMaxDefaultColors = std::size( nDefaultColors );
    return nDefaultColors[ nIndex % nMaxDefaultColors ];
}

void ConfigColorScheme::notify()
{
    m_bNeedsUpdate = true;
}

OUString SAL_CALL ConfigColorScheme::getImplementationName()
{
    return u"com.sun.star.comp.chart2.ConfigDefaultColorScheme"_ustr ;
}

sal_Bool SAL_CALL ConfigColorScheme::supportsService( const OUString& rServiceName )
{
    return cppu::supportsService(this, rServiceName);
}

css::uno::Sequence< OUString > SAL_CALL ConfigColorScheme::getSupportedServiceNames()
{
    return { u"com.sun.star.chart2.ColorScheme"_ustr };
}

} //  namespace chart

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
com_sun_star_comp_chart2_ConfigDefaultColorScheme_get_implementation(css::uno::XComponentContext *context,
        css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new ::chart::ConfigColorScheme(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
