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

#include <string_view>

#include "WrappedAxisAndGridExistenceProperties.hxx"
#include <com/sun/star/beans/XPropertySet.hpp>
#include <Axis.hxx>
#include <AxisHelper.hxx>
#include <WrappedProperty.hxx>
#include "Chart2ModelContact.hxx"
#include <TitleHelper.hxx>
#include <utility>
#include <osl/diagnose.h>

using namespace ::com::sun::star;
using ::com::sun::star::uno::Any;
using ::com::sun::star::uno::Reference;

namespace chart::wrapper
{

namespace {

class WrappedAxisAndGridExistenceProperty : public WrappedProperty
{
public:
    WrappedAxisAndGridExistenceProperty( bool bAxis, bool bMain, sal_Int32 nDimensionIndex
        , std::shared_ptr<Chart2ModelContact> spChart2ModelContact );

    virtual void setPropertyValue( const css::uno::Any& rOuterValue, const css::uno::Reference< css::beans::XPropertySet >& xInnerPropertySet ) const override;

    virtual css::uno::Any getPropertyValue( const css::uno::Reference< css::beans::XPropertySet >& xInnerPropertySet ) const override;

    virtual css::uno::Any getPropertyDefault( const css::uno::Reference< css::beans::XPropertyState >& xInnerPropertyState ) const override;

private: //member
    std::shared_ptr< Chart2ModelContact >   m_spChart2ModelContact;
    bool        m_bAxis;
    bool        m_bMain;
    sal_Int32   m_nDimensionIndex;
};

}

void WrappedAxisAndGridExistenceProperties::addWrappedProperties( std::vector< std::unique_ptr<WrappedProperty> >& rList
            , const std::shared_ptr< Chart2ModelContact >& spChart2ModelContact )
{
    rList.emplace_back( new WrappedAxisAndGridExistenceProperty( true, true, 0, spChart2ModelContact ) );//x axis
    rList.emplace_back( new WrappedAxisAndGridExistenceProperty( true, false, 0, spChart2ModelContact ) );//x secondary axis
    rList.emplace_back( new WrappedAxisAndGridExistenceProperty( false, true, 0, spChart2ModelContact ) );//x grid
    rList.emplace_back( new WrappedAxisAndGridExistenceProperty( false, false, 0, spChart2ModelContact ) );//x help grid

    rList.emplace_back( new WrappedAxisAndGridExistenceProperty( true, true, 1, spChart2ModelContact ) );//y axis
    rList.emplace_back( new WrappedAxisAndGridExistenceProperty( true, false, 1, spChart2ModelContact ) );//y secondary axis
    rList.emplace_back( new WrappedAxisAndGridExistenceProperty( false, true, 1, spChart2ModelContact ) );//y grid
    rList.emplace_back( new WrappedAxisAndGridExistenceProperty( false, false, 1, spChart2ModelContact ) );//y help grid

    rList.emplace_back( new WrappedAxisAndGridExistenceProperty( true, true, 2, spChart2ModelContact ) );//z axis
    rList.emplace_back( new WrappedAxisAndGridExistenceProperty( false, true, 2, spChart2ModelContact ) );//z grid
    rList.emplace_back( new WrappedAxisAndGridExistenceProperty( false, false, 2, spChart2ModelContact ) );//z help grid
}

WrappedAxisAndGridExistenceProperty::WrappedAxisAndGridExistenceProperty( bool bAxis, bool bMain, sal_Int32 nDimensionIndex
                , std::shared_ptr<Chart2ModelContact> spChart2ModelContact )
            : WrappedProperty(OUString(),OUString())
            , m_spChart2ModelContact(std::move( spChart2ModelContact ))
            , m_bAxis( bAxis )
            , m_bMain( bMain )
            , m_nDimensionIndex( nDimensionIndex )
{
    switch( m_nDimensionIndex )
    {
        case 0:
        {
            if( m_bAxis )
            {
                if( m_bMain )
                    m_aOuterName = "HasXAxis";
                else
                    m_aOuterName = "HasSecondaryXAxis";
            }
            else
            {
                if( m_bMain )
                    m_aOuterName = "HasXAxisGrid";
                else
                    m_aOuterName = "HasXAxisHelpGrid";
            }
        }
        break;
        case 2:
        {
            if( m_bAxis )
            {
                OSL_ENSURE(m_bMain,"there is no secondary z axis at the old api");
                m_bMain = true;
                m_aOuterName = "HasZAxis";
            }
            else
            {
                if( m_bMain )
                    m_aOuterName = "HasZAxisGrid";
                else
                    m_aOuterName = "HasZAxisHelpGrid";
            }
        }
        break;
        default:
        {
            if( m_bAxis )
            {
                if( m_bMain )
                    m_aOuterName = "HasYAxis";
                else
                    m_aOuterName = "HasSecondaryYAxis";
            }
            else
            {
                if( m_bMain )
                    m_aOuterName = "HasYAxisGrid";
                else
                    m_aOuterName = "HasYAxisHelpGrid";
            }
        }
        break;
    }
}

void WrappedAxisAndGridExistenceProperty::setPropertyValue( const Any& rOuterValue, const Reference< beans::XPropertySet >& xInnerPropertySet ) const
{
    bool bNewValue = false;
    if( ! (rOuterValue >>= bNewValue) )
        throw lang::IllegalArgumentException( u"Has axis or grid properties require boolean values"_ustr, nullptr, 0 );

    bool bOldValue = false;
    getPropertyValue( xInnerPropertySet ) >>= bOldValue;

    if( bOldValue == bNewValue )
        return;

    rtl::Reference< ::chart::Diagram > xDiagram( m_spChart2ModelContact->getDiagram() );
    if( bNewValue )
    {
        if( m_bAxis )
            AxisHelper::showAxis( m_nDimensionIndex, m_bMain, xDiagram, m_spChart2ModelContact->m_xContext );
        else
            AxisHelper::showGrid( m_nDimensionIndex, 0, m_bMain, xDiagram );
    }
    else
    {
        if( m_bAxis )
            AxisHelper::hideAxis( m_nDimensionIndex, m_bMain, xDiagram );
        else
            AxisHelper::hideGrid( m_nDimensionIndex, 0, m_bMain, xDiagram );
    }
}

Any WrappedAxisAndGridExistenceProperty::getPropertyValue( const Reference< beans::XPropertySet >& /* xInnerPropertySet */ ) const
{
    Any aRet;
    rtl::Reference< ::chart::Diagram > xDiagram( m_spChart2ModelContact->getDiagram() );
    if(m_bAxis)
    {
        bool bShown = AxisHelper::isAxisShown( m_nDimensionIndex, m_bMain, xDiagram );
        aRet <<= bShown;
    }
    else
    {
        bool bShown = AxisHelper::isGridShown( m_nDimensionIndex, 0, m_bMain, xDiagram );
        aRet <<= bShown;
    }
    return aRet;
}

Any WrappedAxisAndGridExistenceProperty::getPropertyDefault( const Reference< beans::XPropertyState >& /*xInnerPropertyState*/ ) const
{
    Any aRet;
    aRet <<= false;
    return aRet;
}

namespace {

class WrappedAxisTitleExistenceProperty : public WrappedProperty
{
public:
    WrappedAxisTitleExistenceProperty( sal_Int32 nTitleIndex
        , std::shared_ptr<Chart2ModelContact> spChart2ModelContact );

    virtual void setPropertyValue( const css::uno::Any& rOuterValue, const css::uno::Reference< css::beans::XPropertySet >& xInnerPropertySet ) const override;

    virtual css::uno::Any getPropertyValue( const css::uno::Reference< css::beans::XPropertySet >& xInnerPropertySet ) const override;

    virtual css::uno::Any getPropertyDefault( const css::uno::Reference< css::beans::XPropertyState >& xInnerPropertyState ) const override;

private: //member
    std::shared_ptr< Chart2ModelContact >   m_spChart2ModelContact;
    TitleHelper::eTitleType             m_eTitleType;
};

}

void WrappedAxisTitleExistenceProperties::addWrappedProperties( std::vector< std::unique_ptr<WrappedProperty> >& rList
            , const std::shared_ptr< Chart2ModelContact >& spChart2ModelContact )
{
    rList.emplace_back( new WrappedAxisTitleExistenceProperty( 0, spChart2ModelContact ) );//x axis title
    rList.emplace_back( new WrappedAxisTitleExistenceProperty( 1, spChart2ModelContact ) );//y axis title
    rList.emplace_back( new WrappedAxisTitleExistenceProperty( 2, spChart2ModelContact ) );//z axis title
    rList.emplace_back( new WrappedAxisTitleExistenceProperty( 3, spChart2ModelContact ) );//secondary x axis title
    rList.emplace_back( new WrappedAxisTitleExistenceProperty( 4, spChart2ModelContact ) );//secondary y axis title
}

WrappedAxisTitleExistenceProperty::WrappedAxisTitleExistenceProperty(sal_Int32 nTitleIndex
                , std::shared_ptr<Chart2ModelContact> spChart2ModelContact)
            : WrappedProperty(OUString(),OUString())
            , m_spChart2ModelContact(std::move( spChart2ModelContact ))
            , m_eTitleType( TitleHelper::Y_AXIS_TITLE )
{
    switch( nTitleIndex )
    {
        case 0:
            m_aOuterName = "HasXAxisTitle";
            m_eTitleType = TitleHelper::X_AXIS_TITLE;
            break;
        case 2:
            m_aOuterName = "HasZAxisTitle";
            m_eTitleType = TitleHelper::Z_AXIS_TITLE;
            break;
        case 3:
            m_aOuterName = "HasSecondaryXAxisTitle";
            m_eTitleType = TitleHelper::SECONDARY_X_AXIS_TITLE;
            break;
        case 4:
            m_aOuterName = "HasSecondaryYAxisTitle";
            m_eTitleType = TitleHelper::SECONDARY_Y_AXIS_TITLE;
            break;
        default:
            m_aOuterName = "HasYAxisTitle";
            m_eTitleType = TitleHelper::Y_AXIS_TITLE;
            break;
    }
}

void WrappedAxisTitleExistenceProperty::setPropertyValue( const Any& rOuterValue, const Reference< beans::XPropertySet >& xInnerPropertySet ) const
{
    bool bNewValue = false;
    if( ! (rOuterValue >>= bNewValue) )
        throw lang::IllegalArgumentException( u"Has axis or grid properties require boolean values"_ustr, nullptr, 0 );

    bool bOldValue = false;
    getPropertyValue( xInnerPropertySet ) >>= bOldValue;

    if( bOldValue == bNewValue )
        return;

    if( bNewValue )
    {
        TitleHelper::createTitle(  m_eTitleType, OUString()
            , m_spChart2ModelContact->getDocumentModel(), m_spChart2ModelContact->m_xContext );
    }
    else
    {
        TitleHelper::removeTitle( m_eTitleType, m_spChart2ModelContact->getDocumentModel() );
    }
}

Any WrappedAxisTitleExistenceProperty::getPropertyValue( const Reference< beans::XPropertySet >& /*xInnerPropertySet*/ ) const
{
    bool bHasTitle = false;

    rtl::Reference< Title > xTitle( TitleHelper::getTitle( m_eTitleType, m_spChart2ModelContact->getDocumentModel() ) );
    if( xTitle.is() && !TitleHelper::getCompleteString( xTitle ).isEmpty() )
        bHasTitle = true;

    Any aRet;
    aRet <<= bHasTitle;
    return aRet;

}

Any WrappedAxisTitleExistenceProperty::getPropertyDefault( const Reference< beans::XPropertyState >& /*xInnerPropertyState*/ ) const
{
    Any aRet;
    aRet <<= false;
    return aRet;
}

namespace {

class WrappedAxisLabelExistenceProperty : public WrappedProperty
{
public:
    WrappedAxisLabelExistenceProperty( bool bMain, sal_Int32 nDimensionIndex
        , std::shared_ptr<Chart2ModelContact> spChart2ModelContact );

    virtual void setPropertyValue( const css::uno::Any& rOuterValue, const css::uno::Reference< css::beans::XPropertySet >& xInnerPropertySet ) const override;

    virtual css::uno::Any getPropertyValue( const css::uno::Reference< css::beans::XPropertySet >& xInnerPropertySet ) const override;

    virtual css::uno::Any getPropertyDefault( const css::uno::Reference< css::beans::XPropertyState >& xInnerPropertyState ) const override;

private: //member
    std::shared_ptr< Chart2ModelContact >   m_spChart2ModelContact;
    bool        m_bMain;
    sal_Int32   m_nDimensionIndex;
};

}

void WrappedAxisLabelExistenceProperties::addWrappedProperties( std::vector< std::unique_ptr<WrappedProperty> >& rList
            , const std::shared_ptr< Chart2ModelContact >& spChart2ModelContact )
{
    rList.emplace_back( new WrappedAxisLabelExistenceProperty( true, 0, spChart2ModelContact ) );//x axis
    rList.emplace_back( new WrappedAxisLabelExistenceProperty( true, 1, spChart2ModelContact ) );//y axis
    rList.emplace_back( new WrappedAxisLabelExistenceProperty( true, 2, spChart2ModelContact ) );//z axis
    rList.emplace_back( new WrappedAxisLabelExistenceProperty( false, 0, spChart2ModelContact ) );//secondary x axis
    rList.emplace_back( new WrappedAxisLabelExistenceProperty( false, 1, spChart2ModelContact ) );//secondary y axis
}

WrappedAxisLabelExistenceProperty::WrappedAxisLabelExistenceProperty(bool bMain, sal_Int32 nDimensionIndex
                , std::shared_ptr<Chart2ModelContact> spChart2ModelContact)
            : WrappedProperty(OUString(),OUString())
            , m_spChart2ModelContact(std::move( spChart2ModelContact ))
            , m_bMain( bMain )
            , m_nDimensionIndex( nDimensionIndex )
{
    switch( m_nDimensionIndex )
    {
        case 0:
            m_aOuterName = m_bMain ? std::u16string_view(u"HasXAxisDescription") : std::u16string_view(u"HasSecondaryXAxisDescription");
            break;
        case 2:
            OSL_ENSURE(m_bMain,"there is no description available for a secondary z axis");
            m_aOuterName = "HasZAxisDescription";
            break;
        default:
            m_aOuterName = m_bMain ? std::u16string_view(u"HasYAxisDescription") : std::u16string_view(u"HasSecondaryYAxisDescription");
            break;
    }
}

void WrappedAxisLabelExistenceProperty::setPropertyValue( const Any& rOuterValue, const Reference< beans::XPropertySet >& xInnerPropertySet ) const
{
    bool bNewValue = false;
    if( ! (rOuterValue >>= bNewValue) )
        throw lang::IllegalArgumentException( u"Has axis or grid properties require boolean values"_ustr, nullptr, 0 );

    bool bOldValue = false;
    getPropertyValue( xInnerPropertySet ) >>= bOldValue;

    if( bOldValue == bNewValue )
        return;

    rtl::Reference< ::chart::Diagram > xDiagram( m_spChart2ModelContact->getDiagram() );
    rtl::Reference< Axis > xProp = AxisHelper::getAxis( m_nDimensionIndex, m_bMain, xDiagram );
    if( !xProp.is() && bNewValue )
    {
        //create axis if needed
        xProp = AxisHelper::createAxis( m_nDimensionIndex, m_bMain, xDiagram, m_spChart2ModelContact->m_xContext );
        if( xProp.is() )
            xProp->setPropertyValue( u"Show"_ustr, uno::Any( false ) );
    }
    if( xProp.is() )
        xProp->setPropertyValue( u"DisplayLabels"_ustr, rOuterValue );
}

Any WrappedAxisLabelExistenceProperty::getPropertyValue( const Reference< beans::XPropertySet >& /*xInnerPropertySet*/ ) const
{
    Any aRet;
    rtl::Reference< ::chart::Diagram > xDiagram( m_spChart2ModelContact->getDiagram() );
    rtl::Reference< Axis > xProp = AxisHelper::getAxis( m_nDimensionIndex, m_bMain, xDiagram );
    if( xProp.is() )
        aRet = xProp->getPropertyValue( u"DisplayLabels"_ustr );
    else
        aRet <<= false;
    return aRet;
}

Any WrappedAxisLabelExistenceProperty::getPropertyDefault( const Reference< beans::XPropertyState >& /*xInnerPropertyState*/ ) const
{
    Any aRet;
    aRet <<= true;
    return aRet;
}

} //namespace chart::wrapper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
