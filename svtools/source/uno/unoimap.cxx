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

#include <com/sun/star/container/XIndexContainer.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/document/XEventsSupplier.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/awt/Rectangle.hpp>
#include <com/sun/star/awt/Point.hpp>
#include <com/sun/star/drawing/PointSequence.hpp>
#include <comphelper/servicehelper.hxx>
#include <comphelper/propertysethelper.hxx>
#include <comphelper/propertysetinfo.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <algorithm>
#include <osl/diagnose.h>
#include <rtl/ref.hxx>
#include <svtools/unoevent.hxx>
#include <svtools/unoimap.hxx>
#include <vcl/imap.hxx>
#include <vcl/imapcirc.hxx>
#include <vcl/imaprect.hxx>
#include <vcl/imappoly.hxx>

using namespace comphelper;
using namespace cppu;
using namespace com::sun::star;
using namespace css::uno;
using namespace css::lang;
using namespace css::container;
using namespace css::beans;
using namespace css::document;
using namespace css::drawing;

const sal_Int32 HANDLE_URL = 1;
const sal_Int32 HANDLE_DESCRIPTION = 2;
const sal_Int32 HANDLE_TARGET = 3;
const sal_Int32 HANDLE_NAME = 4;
const sal_Int32 HANDLE_ISACTIVE = 5;
const sal_Int32 HANDLE_POLYGON = 6;
const sal_Int32 HANDLE_CENTER = 7;
const sal_Int32 HANDLE_RADIUS = 8;
const sal_Int32 HANDLE_BOUNDARY = 9;
const sal_Int32 HANDLE_TITLE = 10;

namespace {

class SvUnoImageMapObject : public OWeakObject,
                            public XEventsSupplier,
                            public XServiceInfo,
                            public PropertySetHelper,
                            public XTypeProvider
{
public:
    SvUnoImageMapObject( IMapObjectType nType, const SvEventDescription* pSupportedMacroItems );
    SvUnoImageMapObject( const IMapObject& rMapObject, const SvEventDescription* pSupportedMacroItems );

    std::unique_ptr<IMapObject> createIMapObject() const;

    rtl::Reference<SvMacroTableEventDescriptor> mxEvents;

    // overridden helpers from PropertySetHelper
    virtual void _setPropertyValues( const PropertyMapEntry** ppEntries, const Any* pValues ) override;
    virtual void _getPropertyValues( const PropertyMapEntry** ppEntries, Any* pValue ) override;

    // XInterface
    virtual Any SAL_CALL queryInterface( const Type & rType ) override;
    virtual void SAL_CALL acquire() noexcept override;
    virtual void SAL_CALL release() noexcept override;

    // XTypeProvider
    virtual Sequence< Type > SAL_CALL getTypes(  ) override;
    virtual Sequence< sal_Int8 > SAL_CALL getImplementationId(  ) override;

    // XEventsSupplier
    virtual Reference< css::container::XNameReplace > SAL_CALL getEvents(  ) override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName(  ) override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;

private:
    static rtl::Reference<PropertySetInfo> createPropertySetInfo( IMapObjectType nType );


    IMapObjectType mnType;

    OUString maURL;
    OUString maAltText;
    OUString maDesc;
    OUString maTarget;
    OUString maName;
    bool mbIsActive;
    awt::Rectangle maBoundary;
    awt::Point maCenter;
    sal_Int32 mnRadius;
    PointSequence maPolygon;
};

}

rtl::Reference<PropertySetInfo> SvUnoImageMapObject::createPropertySetInfo( IMapObjectType nType )
{
    switch( nType )
    {
    case IMapObjectType::Polygon:
        {
            static PropertyMapEntry const aPolygonObj_Impl[] =
            {
                { u"URL"_ustr,         HANDLE_URL,         cppu::UnoType<OUString>::get(),     0, 0 },
                { u"Title"_ustr,       HANDLE_TITLE,       cppu::UnoType<OUString>::get(),     0, 0 },
                { u"Description"_ustr, HANDLE_DESCRIPTION, cppu::UnoType<OUString>::get(),     0, 0 },
                { u"Target"_ustr,      HANDLE_TARGET,      cppu::UnoType<OUString>::get(),     0, 0 },
                { u"Name"_ustr,        HANDLE_NAME,        cppu::UnoType<OUString>::get(),     0, 0 },
                { u"IsActive"_ustr,    HANDLE_ISACTIVE,    cppu::UnoType<bool>::get(),                0, 0 },
                { u"Polygon"_ustr,     HANDLE_POLYGON,     cppu::UnoType<PointSequence>::get(),    0, 0 },
            };

            return rtl::Reference<PropertySetInfo>(new PropertySetInfo( aPolygonObj_Impl ));
        }
    case IMapObjectType::Circle:
        {
            static PropertyMapEntry const aCircleObj_Impl[] =
            {
                { u"URL"_ustr,         HANDLE_URL,         cppu::UnoType<OUString>::get(),     0, 0 },
                { u"Title"_ustr,       HANDLE_TITLE,       cppu::UnoType<OUString>::get(),     0, 0 },
                { u"Description"_ustr, HANDLE_DESCRIPTION, cppu::UnoType<OUString>::get(),     0, 0 },
                { u"Target"_ustr,      HANDLE_TARGET,      cppu::UnoType<OUString>::get(),     0, 0 },
                { u"Name"_ustr,        HANDLE_NAME,        cppu::UnoType<OUString>::get(),     0, 0 },
                { u"IsActive"_ustr,    HANDLE_ISACTIVE,    cppu::UnoType<bool>::get(),                0, 0 },
                { u"Center"_ustr,      HANDLE_CENTER,      cppu::UnoType<awt::Point>::get(),   0, 0 },
                { u"Radius"_ustr,      HANDLE_RADIUS,      cppu::UnoType<sal_Int32>::get(),    0, 0 },
            };

            return rtl::Reference<PropertySetInfo>(new PropertySetInfo( aCircleObj_Impl ));
        }
    case IMapObjectType::Rectangle:
    default:
        {
            static PropertyMapEntry const aRectangleObj_Impl[] =
            {
                { u"URL"_ustr,         HANDLE_URL,         cppu::UnoType<OUString>::get(), 0, 0 },
                { u"Title"_ustr,       HANDLE_TITLE,       cppu::UnoType<OUString>::get(),     0, 0 },
                { u"Description"_ustr, HANDLE_DESCRIPTION, cppu::UnoType<OUString>::get(), 0, 0 },
                { u"Target"_ustr,      HANDLE_TARGET,      cppu::UnoType<OUString>::get(), 0, 0 },
                { u"Name"_ustr,        HANDLE_NAME,        cppu::UnoType<OUString>::get(), 0, 0 },
                { u"IsActive"_ustr,    HANDLE_ISACTIVE,    cppu::UnoType<bool>::get(),            0, 0 },
                { u"Boundary"_ustr,    HANDLE_BOUNDARY,    cppu::UnoType<awt::Rectangle>::get(),   0, 0 },
            };

            return rtl::Reference<PropertySetInfo>(new PropertySetInfo( aRectangleObj_Impl ));
        }
    }
}

SvUnoImageMapObject::SvUnoImageMapObject( IMapObjectType nType, const SvEventDescription* pSupportedMacroItems )
:   PropertySetHelper( createPropertySetInfo( nType ) ),
    mnType( nType )
,   mbIsActive( true )
,   mnRadius( 0 )
{
    mxEvents = new SvMacroTableEventDescriptor( pSupportedMacroItems );
}

SvUnoImageMapObject::SvUnoImageMapObject( const IMapObject& rMapObject, const SvEventDescription* pSupportedMacroItems )
:   PropertySetHelper( createPropertySetInfo( rMapObject.GetType() ) ),
    mnType( rMapObject.GetType() )
,   mbIsActive( true )
,   mnRadius( 0 )
{
    maURL = rMapObject.GetURL();
    maAltText = rMapObject.GetAltText();
    maDesc = rMapObject.GetDesc();
    maTarget = rMapObject.GetTarget();
    maName = rMapObject.GetName();
    mbIsActive = rMapObject.IsActive();

    switch( mnType )
    {
    case IMapObjectType::Rectangle:
        {
            const tools::Rectangle aRect( static_cast<const IMapRectangleObject*>(&rMapObject)->GetRectangle(false) );
            maBoundary.X = aRect.Left();
            maBoundary.Y = aRect.Top();
            maBoundary.Width = aRect.GetWidth();
            maBoundary.Height = aRect.GetHeight();
        }
        break;
    case IMapObjectType::Circle:
        {
            mnRadius = static_cast<const IMapCircleObject*>(&rMapObject)->GetRadius(false);
            const Point aPoint( static_cast<const IMapCircleObject*>(&rMapObject)->GetCenter(false) );

            maCenter.X = aPoint.X();
            maCenter.Y = aPoint.Y();
        }
        break;
    case IMapObjectType::Polygon:
    default:
        {
            const tools::Polygon aPoly( static_cast<const IMapPolygonObject*>(&rMapObject)->GetPolygon(false) );

            const sal_uInt16 nCount = aPoly.GetSize();
            maPolygon.realloc( nCount );
            awt::Point* pPoints = maPolygon.getArray();

            for( sal_uInt16 nPoint = 0; nPoint < nCount; nPoint++ )
            {
                const Point& rPoint = aPoly.GetPoint( nPoint );
                pPoints->X = rPoint.X();
                pPoints->Y = rPoint.Y();

                pPoints++;
            }
        }
    }

    mxEvents = new SvMacroTableEventDescriptor( rMapObject.GetMacroTable(), pSupportedMacroItems );
}

std::unique_ptr<IMapObject> SvUnoImageMapObject::createIMapObject() const
{
    const OUString aURL( maURL );
    const OUString aAltText( maAltText );
    const OUString aDesc( maDesc );
    const OUString aTarget( maTarget );
    const OUString aName( maName );

    std::unique_ptr<IMapObject> pNewIMapObject;

    switch( mnType )
    {
    case IMapObjectType::Rectangle:
        {
            const tools::Rectangle aRect( maBoundary.X, maBoundary.Y, maBoundary.X + maBoundary.Width - 1, maBoundary.Y + maBoundary.Height - 1 );
            pNewIMapObject.reset(new IMapRectangleObject( aRect, aURL, aAltText, aDesc, aTarget, aName, mbIsActive, false ));
        }
        break;

    case IMapObjectType::Circle:
        {
            const Point aCenter( maCenter.X, maCenter.Y );
            pNewIMapObject.reset(new IMapCircleObject( aCenter, mnRadius, aURL, aAltText, aDesc, aTarget, aName, mbIsActive, false ));
        }
        break;

    case IMapObjectType::Polygon:
    default:
        {
            const sal_uInt16 nCount = static_cast<sal_uInt16>(maPolygon.getLength());

            tools::Polygon aPoly( nCount );
            for( sal_uInt16 nPoint = 0; nPoint < nCount; nPoint++ )
            {
                Point aPoint( maPolygon[nPoint].X, maPolygon[nPoint].Y );
                aPoly.SetPoint( aPoint, nPoint );
            }

            aPoly.Optimize( PolyOptimizeFlags::CLOSE );
            pNewIMapObject.reset(new IMapPolygonObject( aPoly, aURL, aAltText, aDesc, aTarget, aName, mbIsActive, false ));
        }
        break;
    }

    SvxMacroTableDtor aMacroTable;
    mxEvents->copyMacrosIntoTable(aMacroTable);
    pNewIMapObject->SetMacroTable( aMacroTable );

    return pNewIMapObject;
}

// XInterface

Any SAL_CALL SvUnoImageMapObject::queryInterface( const Type & rType )
{
    Any aAny;

    if( rType == cppu::UnoType<XServiceInfo>::get())
        aAny <<= Reference< XServiceInfo >(this);
    else if( rType == cppu::UnoType<XTypeProvider>::get())
        aAny <<= Reference< XTypeProvider >(this);
    else if( rType == cppu::UnoType<XPropertySet>::get())
        aAny <<= Reference< XPropertySet >(this);
    else if( rType == cppu::UnoType<XEventsSupplier>::get())
        aAny <<= Reference< XEventsSupplier >(this);
    else if( rType == cppu::UnoType<XMultiPropertySet>::get())
        aAny <<= Reference< XMultiPropertySet >(this);
    else
        aAny = OWeakObject::queryInterface( rType );

    return aAny;
}

void SAL_CALL SvUnoImageMapObject::acquire() noexcept
{
    OWeakObject::acquire();
}

void SAL_CALL SvUnoImageMapObject::release() noexcept
{
    OWeakObject::release();
}

uno::Sequence< uno::Type > SAL_CALL SvUnoImageMapObject::getTypes()
{
    static const uno::Sequence< uno::Type > aTypes {
        cppu::UnoType<XEventsSupplier>::get(),
        cppu::UnoType<XServiceInfo>::get(),
        cppu::UnoType<XPropertySet>::get(),
        cppu::UnoType<XMultiPropertySet>::get(),
        cppu::UnoType<XTypeProvider>::get() };
    return aTypes;
}

uno::Sequence< sal_Int8 > SAL_CALL SvUnoImageMapObject::getImplementationId()
{
    return css::uno::Sequence<sal_Int8>();
}

// XServiceInfo
sal_Bool SAL_CALL SvUnoImageMapObject::supportsService( const  OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

Sequence< OUString > SAL_CALL SvUnoImageMapObject::getSupportedServiceNames()
{
    Sequence< OUString > aSNS( 2 );
    aSNS.getArray()[0] = "com.sun.star.image.ImageMapObject";
    switch( mnType )
    {
    case IMapObjectType::Polygon:
    default:
        aSNS.getArray()[1] = "com.sun.star.image.ImageMapPolygonObject";
        break;
    case IMapObjectType::Rectangle:
        aSNS.getArray()[1] = "com.sun.star.image.ImageMapRectangleObject";
        break;
    case IMapObjectType::Circle:
        aSNS.getArray()[1] = "com.sun.star.image.ImageMapCircleObject";
        break;
    }
    return aSNS;
}

OUString SAL_CALL SvUnoImageMapObject::getImplementationName()
{
    switch( mnType )
    {
    case IMapObjectType::Polygon:
    default:
        return u"org.openoffice.comp.svt.ImageMapPolygonObject"_ustr;
    case IMapObjectType::Circle:
        return u"org.openoffice.comp.svt.ImageMapCircleObject"_ustr;
    case IMapObjectType::Rectangle:
        return u"org.openoffice.comp.svt.ImageMapRectangleObject"_ustr;
    }
}

// overridden helpers from PropertySetHelper
void SvUnoImageMapObject::_setPropertyValues( const PropertyMapEntry** ppEntries, const Any* pValues )
{
    bool bOk = false;

    while( *ppEntries )
    {
        switch( (*ppEntries)->mnHandle )
        {
        case HANDLE_URL:
            bOk = *pValues >>= maURL;
            break;
        case HANDLE_TITLE:
            bOk = *pValues >>= maAltText;
            break;
        case HANDLE_DESCRIPTION:
            bOk = *pValues >>= maDesc;
            break;
        case HANDLE_TARGET:
            bOk = *pValues >>= maTarget;
            break;
        case HANDLE_NAME:
            bOk = *pValues >>= maName;
            break;
        case HANDLE_ISACTIVE:
            bOk = *pValues >>= mbIsActive;
            break;
        case HANDLE_BOUNDARY:
            bOk = *pValues >>= maBoundary;
            break;
        case HANDLE_CENTER:
            bOk = *pValues >>= maCenter;
            break;
        case HANDLE_RADIUS:
            bOk = *pValues >>= mnRadius;
            break;
        case HANDLE_POLYGON:
            bOk = *pValues >>= maPolygon;
            break;
        default:
            OSL_FAIL( "SvUnoImageMapObject::_setPropertyValues: unexpected property handle" );
            break;
        }

        if( !bOk )
            throw IllegalArgumentException();

        ppEntries++;
        pValues++;
    }
}

void SvUnoImageMapObject::_getPropertyValues( const PropertyMapEntry** ppEntries, Any* pValues )
{
    while( *ppEntries )
    {
        switch( (*ppEntries)->mnHandle )
        {
        case HANDLE_URL:
            *pValues <<= maURL;
            break;
        case HANDLE_TITLE:
            *pValues <<= maAltText;
            break;
        case HANDLE_DESCRIPTION:
            *pValues <<= maDesc;
            break;
        case HANDLE_TARGET:
            *pValues <<= maTarget;
            break;
        case HANDLE_NAME:
            *pValues <<= maName;
            break;
        case HANDLE_ISACTIVE:
            *pValues <<= mbIsActive;
            break;
        case HANDLE_BOUNDARY:
            *pValues <<= maBoundary;
            break;
        case HANDLE_CENTER:
            *pValues <<= maCenter;
            break;
        case HANDLE_RADIUS:
            *pValues <<= mnRadius;
            break;
        case HANDLE_POLYGON:
            *pValues <<= maPolygon;
            break;
        default:
            OSL_FAIL( "SvUnoImageMapObject::_getPropertyValues: unexpected property handle" );
            break;
        }

        ppEntries++;
        pValues++;
    }
}


Reference< XNameReplace > SAL_CALL SvUnoImageMapObject::getEvents()
{
    return mxEvents;
}

namespace {

class SvUnoImageMap : public WeakImplHelper< XIndexContainer, XServiceInfo >
{
public:
    explicit SvUnoImageMap();
    SvUnoImageMap( const ImageMap& rMap, const SvEventDescription* pSupportedMacroItems );

    void fillImageMap( ImageMap& rMap ) const;
    /// @throws IllegalArgumentException
    static SvUnoImageMapObject* getObject( const Any& aElement );

    // XIndexContainer
    virtual void SAL_CALL insertByIndex( sal_Int32 Index, const Any& Element ) override;
    virtual void SAL_CALL removeByIndex( sal_Int32 Index ) override;

    // XIndexReplace
    virtual void SAL_CALL replaceByIndex( sal_Int32 Index, const Any& Element ) override;

    // XIndexAccess
    virtual sal_Int32 SAL_CALL getCount(  ) override;
    virtual Any SAL_CALL getByIndex( sal_Int32 Index ) override;

    // XElementAccess
    virtual Type SAL_CALL getElementType(  ) override;
    virtual sal_Bool SAL_CALL hasElements(  ) override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName(  ) override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;

private:
    OUString maName;

    std::vector< rtl::Reference<SvUnoImageMapObject> > maObjectList;
};

}

SvUnoImageMap::SvUnoImageMap()
{
}

SvUnoImageMap::SvUnoImageMap( const ImageMap& rMap, const SvEventDescription* pSupportedMacroItems )
{
    maName = rMap.GetName();

    const std::size_t nCount = rMap.GetIMapObjectCount();
    for( std::size_t nPos = 0; nPos < nCount; nPos++ )
    {
        IMapObject* pMapObject = rMap.GetIMapObject( nPos );
        rtl::Reference<SvUnoImageMapObject> xUnoObj = new SvUnoImageMapObject( *pMapObject, pSupportedMacroItems );
        maObjectList.push_back( xUnoObj );
    }
}

SvUnoImageMapObject* SvUnoImageMap::getObject( const Any& aElement )
{
    Reference< XInterface > xObject;
    aElement >>= xObject;

    SvUnoImageMapObject* pObject = dynamic_cast<SvUnoImageMapObject*>( xObject.get() );
    if( nullptr == pObject )
        throw IllegalArgumentException();

    return pObject;
}

// XIndexContainer
void SAL_CALL SvUnoImageMap::insertByIndex( sal_Int32 nIndex, const Any& Element )
{
    SvUnoImageMapObject* pObject = getObject( Element );
    const sal_Int32 nCount = maObjectList.size();
    if( nullptr == pObject || nIndex > nCount )
        throw IndexOutOfBoundsException();

    if( nIndex == nCount )
        maObjectList.emplace_back(pObject );
    else
    {
        auto aIter = maObjectList.begin();
        std::advance(aIter, nIndex);
        maObjectList.insert( aIter, pObject );
    }
}

void SAL_CALL SvUnoImageMap::removeByIndex( sal_Int32 nIndex )
{
    const sal_Int32 nCount = maObjectList.size();
    if( nIndex >= nCount )
        throw IndexOutOfBoundsException();

    if( nCount - 1 == nIndex )
    {
        maObjectList.pop_back();
    }
    else
    {
        auto aIter = maObjectList.begin();
        std::advance(aIter, nIndex);
        maObjectList.erase( aIter );
    }
}

// XIndexReplace
void SAL_CALL SvUnoImageMap::replaceByIndex( sal_Int32 nIndex, const Any& Element )
{
    SvUnoImageMapObject* pObject = getObject( Element );
    const sal_Int32 nCount = maObjectList.size();
    if( nullptr == pObject || nIndex >= nCount )
        throw IndexOutOfBoundsException();

    auto aIter = maObjectList.begin();
    std::advance(aIter, nIndex);
    *aIter = pObject;
}

// XIndexAccess
sal_Int32 SAL_CALL SvUnoImageMap::getCount(  )
{
    return maObjectList.size();
}

Any SAL_CALL SvUnoImageMap::getByIndex( sal_Int32 nIndex )
{
    const sal_Int32 nCount = maObjectList.size();
    if( nIndex >= nCount )
        throw IndexOutOfBoundsException();

    auto aIter = maObjectList.begin();
    std::advance(aIter, nIndex);

    return Any( Reference< XPropertySet >( *aIter ) );
}

// XElementAccess
Type SAL_CALL SvUnoImageMap::getElementType(  )
{
    return cppu::UnoType<XPropertySet>::get();
}

sal_Bool SAL_CALL SvUnoImageMap::hasElements(  )
{
    return (!maObjectList.empty());
}

// XServiceInfo
OUString SAL_CALL SvUnoImageMap::getImplementationName(  )
{
    return u"org.openoffice.comp.svt.SvUnoImageMap"_ustr;
}

sal_Bool SAL_CALL SvUnoImageMap::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

Sequence< OUString > SAL_CALL SvUnoImageMap::getSupportedServiceNames(  )
{
    return { u"com.sun.star.image.ImageMap"_ustr };
}

void SvUnoImageMap::fillImageMap( ImageMap& rMap ) const
{
    rMap.ClearImageMap();

    rMap.SetName( maName );

    for (auto const& elem : maObjectList)
    {
        std::unique_ptr<IMapObject> pNewMapObject = elem->createIMapObject();
        rMap.InsertIMapObject( std::move(pNewMapObject) );
    }
}


// factory helper methods


Reference< XInterface > SvUnoImageMapRectangleObject_createInstance( const SvEventDescription* pSupportedMacroItems )
{
    return getXWeak(new SvUnoImageMapObject( IMapObjectType::Rectangle, pSupportedMacroItems ));
}

Reference< XInterface > SvUnoImageMapCircleObject_createInstance( const SvEventDescription* pSupportedMacroItems )
{
    return getXWeak(new SvUnoImageMapObject( IMapObjectType::Circle, pSupportedMacroItems ));
}

Reference< XInterface > SvUnoImageMapPolygonObject_createInstance( const SvEventDescription* pSupportedMacroItems )
{
    return getXWeak(new SvUnoImageMapObject( IMapObjectType::Polygon, pSupportedMacroItems ));
}

Reference< XInterface > SvUnoImageMap_createInstance()
{
    return getXWeak(new SvUnoImageMap);
}

Reference< XInterface > SvUnoImageMap_createInstance( const ImageMap& rMap, const SvEventDescription* pSupportedMacroItems )
{
    return getXWeak(new SvUnoImageMap( rMap, pSupportedMacroItems ));
}

bool SvUnoImageMap_fillImageMap( const Reference< XInterface >& xImageMap, ImageMap& rMap )
{
    SvUnoImageMap* pUnoImageMap = dynamic_cast<SvUnoImageMap*>( xImageMap.get() );
    if( nullptr == pUnoImageMap )
        return false;

    pUnoImageMap->fillImageMap( rMap );
    return true;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
