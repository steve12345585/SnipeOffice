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


#include "unogalitem.hxx"
#include "unogaltheme.hxx"
#include <galleryfilestorage.hxx>
#include <svx/galtheme.hxx>
#include <svx/galmisc.hxx>
#include <svx/fmmodel.hxx>
#include <vcl/svapp.hxx>
#include <vcl/graph.hxx>
#include <svl/itempool.hxx>
#include <comphelper/servicehelper.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <galobj.hxx>

#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/gallery/GalleryItemType.hpp>
#include <memory>

#define UNOGALLERY_GALLERYITEMTYPE  1
#define UNOGALLERY_URL              2
#define UNOGALLERY_TITLE            3
#define UNOGALLERY_THUMBNAIL        4
#define UNOGALLERY_GRAPHIC          5
#define UNOGALLERY_DRAWING          6

using namespace ::com::sun::star;

namespace unogallery {


GalleryItem::GalleryItem( ::unogallery::GalleryTheme& rTheme, const GalleryObject& rObject ) :
    ::comphelper::PropertySetHelper( createPropertySetInfo() ),
    mpTheme( &rTheme ),
    mpGalleryObject( &rObject )
{
    mpTheme->implRegisterGalleryItem( *this );
}


GalleryItem::~GalleryItem()
    noexcept
{
    if( mpTheme )
        mpTheme->implDeregisterGalleryItem( *this );
}


bool GalleryItem::isValid() const
{
    return( mpTheme != nullptr );
}


uno::Any SAL_CALL GalleryItem::queryInterface( const uno::Type & rType )
{
    uno::Any aAny;

    if( rType == cppu::UnoType<lang::XServiceInfo>::get())
        aAny <<= uno::Reference< lang::XServiceInfo >(this);
    else if( rType == cppu::UnoType<lang::XTypeProvider>::get())
        aAny <<= uno::Reference< lang::XTypeProvider >(this);
    else if( rType == cppu::UnoType<gallery::XGalleryItem>::get())
        aAny <<= uno::Reference< gallery::XGalleryItem >(this);
    else if( rType == cppu::UnoType<beans::XPropertySet>::get())
        aAny <<= uno::Reference< beans::XPropertySet >(this);
    else if( rType == cppu::UnoType<beans::XPropertyState>::get())
        aAny <<= uno::Reference< beans::XPropertyState >(this);
    else if( rType == cppu::UnoType<beans::XMultiPropertySet>::get())
        aAny <<= uno::Reference< beans::XMultiPropertySet >(this);
    else
        aAny = OWeakObject::queryInterface( rType );

    return aAny;
}


void SAL_CALL GalleryItem::acquire()
    noexcept
{
    OWeakObject::acquire();
}


void SAL_CALL GalleryItem::release()
    noexcept
{
    OWeakObject::release();
}


OUString SAL_CALL GalleryItem::getImplementationName()
{
    return u"com.sun.star.comp.gallery.GalleryItem"_ustr;
}

sal_Bool SAL_CALL GalleryItem::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

uno::Sequence< OUString > SAL_CALL GalleryItem::getSupportedServiceNames()
{
    return { u"com.sun.star.gallery.GalleryItem"_ustr };
}

uno::Sequence< uno::Type > SAL_CALL GalleryItem::getTypes()
{
    static const uno::Sequence aTypes {
        cppu::UnoType<lang::XServiceInfo>::get(),
        cppu::UnoType<lang::XTypeProvider>::get(),
        cppu::UnoType<gallery::XGalleryItem>::get(),
        cppu::UnoType<beans::XPropertySet>::get(),
        cppu::UnoType<beans::XPropertyState>::get(),
        cppu::UnoType<beans::XMultiPropertySet>::get() };
    return aTypes;
}

uno::Sequence< sal_Int8 > SAL_CALL GalleryItem::getImplementationId()
{
    return css::uno::Sequence<sal_Int8>();
}


sal_Int8 SAL_CALL GalleryItem::getType()
{
    const SolarMutexGuard aGuard;
    sal_Int8            nRet = gallery::GalleryItemType::EMPTY;

    if( isValid() )
    {
        switch( implGetObject()->eObjKind )
        {
            case SgaObjKind::Sound:
                nRet = gallery::GalleryItemType::MEDIA;
            break;

            case SgaObjKind::SvDraw:
                nRet = gallery::GalleryItemType::DRAWING;
            break;

            default:
                nRet = gallery::GalleryItemType::GRAPHIC;
            break;
        }
    }

    return nRet;
}


rtl::Reference<::comphelper::PropertySetInfo> GalleryItem::createPropertySetInfo()
{
    static ::comphelper::PropertyMapEntry const aEntries[] =
    {
        { u"GalleryItemType"_ustr, UNOGALLERY_GALLERYITEMTYPE, cppu::UnoType<sal_Int8>::get(),
          beans::PropertyAttribute::READONLY, 0 },

        { u"URL"_ustr, UNOGALLERY_URL, ::cppu::UnoType<OUString>::get(),
          beans::PropertyAttribute::READONLY, 0 },

        { u"Title"_ustr, UNOGALLERY_TITLE, ::cppu::UnoType<OUString>::get(),
          0, 0 },

        { u"Thumbnail"_ustr, UNOGALLERY_THUMBNAIL, cppu::UnoType<graphic::XGraphic>::get(),
          beans::PropertyAttribute::READONLY, 0 },

        { u"Graphic"_ustr, UNOGALLERY_GRAPHIC, cppu::UnoType<graphic::XGraphic>::get(),
          beans::PropertyAttribute::READONLY, 0 },

        { u"Drawing"_ustr, UNOGALLERY_DRAWING, cppu::UnoType<lang::XComponent>::get(),
          beans::PropertyAttribute::READONLY, 0 },
    };

    return rtl::Reference<::comphelper::PropertySetInfo>( new ::comphelper::PropertySetInfo( aEntries ) );
}

void GalleryItem::_setPropertyValues( const comphelper::PropertyMapEntry** ppEntries, const uno::Any* pValues )
{
    const SolarMutexGuard aGuard;

    while( *ppEntries )
    {
        if( UNOGALLERY_TITLE == (*ppEntries)->mnHandle )
        {
            OUString aNewTitle;

            if( !(*pValues >>= aNewTitle) )
            {
                throw lang::IllegalArgumentException();
            }

            ::GalleryTheme* pGalTheme = ( isValid() ? mpTheme->implGetTheme() : nullptr );

            if( pGalTheme )
            {
                std::unique_ptr<SgaObject> pObj(pGalTheme->getGalleryStorageEngine()->implReadSgaObject( implGetObject() ));

                if( pObj )
                {
                    if( pObj->GetTitle() != aNewTitle )
                    {
                        pObj->SetTitle( aNewTitle );
                        pGalTheme->InsertObject( *pObj );
                    }
                }
            }

        }

        ++ppEntries;
        ++pValues;
    }
}

void GalleryItem::_getPropertyValues( const comphelper::PropertyMapEntry** ppEntries, uno::Any* pValue )
{
    const SolarMutexGuard aGuard;

    while( *ppEntries )
    {
        switch( (*ppEntries)->mnHandle )
        {
            case UNOGALLERY_GALLERYITEMTYPE:
            {
                *pValue <<= getType();
            }
            break;

            case UNOGALLERY_URL:
            {
                ::GalleryTheme* pGalTheme = ( isValid() ? mpTheme->implGetTheme() : nullptr );

                if( pGalTheme )
                    *pValue <<= implGetObject()->m_oStorageUrl->GetMainURL( INetURLObject::DecodeMechanism::NONE );
            }
            break;

            case UNOGALLERY_TITLE:
            {
                ::GalleryTheme* pGalTheme = ( isValid() ? mpTheme->implGetTheme() : nullptr );

                if( pGalTheme )
                {
                    std::unique_ptr<SgaObject> pObj = pGalTheme->AcquireObject( pGalTheme->maGalleryObjectCollection.searchPosWithObject( implGetObject() ) );

                    if( pObj )
                    {
                        *pValue <<= pObj->GetTitle();
                    }
                }
            }
            break;

            case UNOGALLERY_THUMBNAIL:
            {
                ::GalleryTheme* pGalTheme = ( isValid() ? mpTheme->implGetTheme() : nullptr );

                if( pGalTheme )
                {
                    std::unique_ptr<SgaObject> pObj = pGalTheme->AcquireObject( pGalTheme->maGalleryObjectCollection.searchPosWithObject( implGetObject() ) );

                    if( pObj )
                    {
                        Graphic aThumbnail;

                        if( pObj->IsThumbBitmap() )
                            aThumbnail = pObj->GetThumbBmp();
                        else
                            aThumbnail = pObj->GetThumbMtf();

                        *pValue <<= aThumbnail.GetXGraphic();
                    }
                }
            }
            break;

            case UNOGALLERY_GRAPHIC:
            {
                ::GalleryTheme* pGalTheme = ( isValid() ? mpTheme->implGetTheme() : nullptr );
                Graphic         aGraphic;

                if( pGalTheme && pGalTheme->GetGraphic( pGalTheme->maGalleryObjectCollection.searchPosWithObject( implGetObject() ), aGraphic ) )
                    *pValue <<= aGraphic.GetXGraphic();
            }
            break;

            case UNOGALLERY_DRAWING:
            {
                if( gallery::GalleryItemType::DRAWING == getType() )
                {
                    ::GalleryTheme* pGalTheme = ( isValid() ? mpTheme->implGetTheme() : nullptr );
                    FmFormModel*    pModel = new FmFormModel();

                    if( pGalTheme && pGalTheme->GetModel( pGalTheme->maGalleryObjectCollection.searchPosWithObject( implGetObject() ), *pModel ) )
                    {
                        rtl::Reference< GalleryDrawingModel > xDrawing( new GalleryDrawingModel( pModel ) );

                        pModel->setUnoModel( xDrawing );
                        *pValue <<= uno::Reference< lang::XComponent >(xDrawing);
                    }
                    else
                        delete pModel;
                }
            }
            break;
        }

        ++ppEntries;
        ++pValue;
    }
}


void GalleryItem::implSetInvalid()
{
    if( mpTheme )
    {
        mpTheme = nullptr;
        mpGalleryObject = nullptr;
    }
}


GalleryDrawingModel::GalleryDrawingModel( SdrModel* pDoc )
    noexcept :
    SvxUnoDrawingModel( pDoc )
{
}


GalleryDrawingModel::~GalleryDrawingModel()
    noexcept
{
    delete GetDoc();
}


UNO3_GETIMPLEMENTATION_IMPL( GalleryDrawingModel );

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
