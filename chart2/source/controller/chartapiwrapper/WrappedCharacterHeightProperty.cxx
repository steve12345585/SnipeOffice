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

#include "WrappedCharacterHeightProperty.hxx"
#include <RelativeSizeHelper.hxx>
#include "ReferenceSizePropertyProvider.hxx"
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XPropertyState.hpp>
#include <osl/diagnose.h>

using namespace ::com::sun::star;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Any;

namespace chart::wrapper
{
WrappedCharacterHeightProperty_Base::WrappedCharacterHeightProperty_Base(
                            const OUString& rOuterEqualsInnerName
                            , ReferenceSizePropertyProvider* pRefSizePropProvider )
                            : WrappedProperty( rOuterEqualsInnerName, rOuterEqualsInnerName )
                           , m_pRefSizePropProvider( pRefSizePropProvider )
{
}
WrappedCharacterHeightProperty_Base::~WrappedCharacterHeightProperty_Base()
{
}

void WrappedCharacterHeightProperty::addWrappedProperties( std::vector< std::unique_ptr<WrappedProperty> >& rList
            , ReferenceSizePropertyProvider* pRefSizePropProvider  )
{
    rList.emplace_back( new WrappedCharacterHeightProperty( pRefSizePropProvider ) );
    rList.emplace_back( new WrappedAsianCharacterHeightProperty( pRefSizePropProvider ) );
    rList.emplace_back( new WrappedComplexCharacterHeightProperty( pRefSizePropProvider ) );
}

void WrappedCharacterHeightProperty_Base::setPropertyValue( const Any& rOuterValue, const Reference< beans::XPropertySet >& xInnerPropertySet ) const
{
    if(xInnerPropertySet.is())
    {
        if( m_pRefSizePropProvider )
            m_pRefSizePropProvider->updateReferenceSize();
        xInnerPropertySet->setPropertyValue( m_aInnerName, rOuterValue );
    }
}

Any WrappedCharacterHeightProperty_Base::getPropertyValue( const Reference< beans::XPropertySet >& xInnerPropertySet ) const
{
    Any aRet;
    if( xInnerPropertySet.is() )
    {
        aRet = xInnerPropertySet->getPropertyValue( m_aInnerName );
        float fHeight = 0;
        if( aRet >>= fHeight )
        {
            if( m_pRefSizePropProvider )
            {
                awt::Size aReferenceSize;
                if( m_pRefSizePropProvider->getReferenceSize() >>= aReferenceSize )
                {
                    awt::Size aCurrentSize = m_pRefSizePropProvider->getCurrentSizeForReference();
                    aRet <<= static_cast< float >(
                            RelativeSizeHelper::calculate( fHeight, aReferenceSize, aCurrentSize ));
                }
            }
        }
    }
    return aRet;
}

Any WrappedCharacterHeightProperty_Base::getPropertyDefault( const Reference< beans::XPropertyState >& xInnerPropertyState ) const
{
    Any aRet;
    if( xInnerPropertyState.is() )
    {
        aRet = xInnerPropertyState->getPropertyDefault( m_aInnerName );
    }
    return aRet;
}

beans::PropertyState WrappedCharacterHeightProperty_Base::getPropertyState( const Reference< beans::XPropertyState >& /*xInnerPropertyState*/ ) const
{
    return beans::PropertyState_DIRECT_VALUE;
}

Any WrappedCharacterHeightProperty_Base::convertInnerToOuterValue( const Any& rInnerValue ) const
{
    OSL_FAIL("should not be used: WrappedCharacterHeightProperty_Base::convertInnerToOuterValue - check if you miss data");
    return rInnerValue;
}
Any WrappedCharacterHeightProperty_Base::convertOuterToInnerValue( const Any& rOuterValue ) const
{
    OSL_FAIL("should not be used: WrappedCharacterHeightProperty_Base::convertOuterToInnerValue - check if you miss data");
    return rOuterValue;
}

WrappedCharacterHeightProperty::WrappedCharacterHeightProperty( ReferenceSizePropertyProvider* pRefSizePropProvider )
        : WrappedCharacterHeightProperty_Base( u"CharHeight"_ustr, pRefSizePropProvider )
{
}
WrappedCharacterHeightProperty::~WrappedCharacterHeightProperty()
{
}

WrappedAsianCharacterHeightProperty::WrappedAsianCharacterHeightProperty( ReferenceSizePropertyProvider* pRefSizePropProvider )
        : WrappedCharacterHeightProperty_Base( u"CharHeightAsian"_ustr, pRefSizePropProvider )
{
}
WrappedAsianCharacterHeightProperty::~WrappedAsianCharacterHeightProperty()
{
}

WrappedComplexCharacterHeightProperty::WrappedComplexCharacterHeightProperty( ReferenceSizePropertyProvider* pRefSizePropProvider )
        : WrappedCharacterHeightProperty_Base( u"CharHeightComplex"_ustr, pRefSizePropProvider )
{
}
WrappedComplexCharacterHeightProperty::~WrappedComplexCharacterHeightProperty()
{
}

} //namespace chart::wrapper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
