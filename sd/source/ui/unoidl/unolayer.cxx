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

#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>

#include <unolayer.hxx>

#include <comphelper/extract.hxx>
#include <editeng/unoipset.hxx>
#include <osl/diagnose.h>
#include <svl/itemprop.hxx>
#include <svx/svdpagv.hxx>
#include <svx/svdobj.hxx>
#include <cppuhelper/supportsservice.hxx>

// following ones for InsertSdPage()
#include <svx/svdlayer.hxx>

#include <DrawDocShell.hxx>
#include <drawdoc.hxx>
#include <unomodel.hxx>
#include <unoprnms.hxx>
#include <com/sun/star/lang/NoSupportException.hpp>
#include <svx/svdpool.hxx>
#include <FrameView.hxx>
#include <DrawViewShell.hxx>
#include <View.hxx>
#include <ViewShell.hxx>
#include <strings.hrc>
#include <sdresid.hxx>

#include "unowcntr.hxx"
#include <vcl/svapp.hxx>

using namespace ::com::sun::star;

// class SdLayer
#define WID_LAYER_LOCKED    1
#define WID_LAYER_PRINTABLE 2
#define WID_LAYER_VISIBLE   3
#define WID_LAYER_NAME      4
#define WID_LAYER_TITLE     5
#define WID_LAYER_DESC      6

static const SvxItemPropertySet* ImplGetSdLayerPropertySet()
{
    static const SfxItemPropertyMapEntry aSdLayerPropertyMap_Impl[] =
    {
        { u"" UNO_NAME_LAYER_LOCKED ""_ustr,      WID_LAYER_LOCKED,   cppu::UnoType<bool>::get(),            0, 0 },
        { u"" UNO_NAME_LAYER_PRINTABLE ""_ustr,   WID_LAYER_PRINTABLE,cppu::UnoType<bool>::get(),            0, 0 },
        { u"" UNO_NAME_LAYER_VISIBLE ""_ustr,     WID_LAYER_VISIBLE,  cppu::UnoType<bool>::get(),            0, 0 },
        { u"" UNO_NAME_LAYER_NAME ""_ustr,        WID_LAYER_NAME,     ::cppu::UnoType<OUString>::get(), 0, 0 },
        { u"Title"_ustr,                    WID_LAYER_TITLE,    ::cppu::UnoType<OUString>::get(), 0, 0 },
        { u"Description"_ustr,              WID_LAYER_DESC,     ::cppu::UnoType<OUString>::get(), 0, 0 },
    };
    static SvxItemPropertySet aSDLayerPropertySet_Impl( aSdLayerPropertyMap_Impl, SdrObject::GetGlobalDrawObjectItemPool() );
    return &aSDLayerPropertySet_Impl;
}

SdLayer::SdLayer(SdLayerManager* pLayerManager_, SdrLayer* pSdrLayer_)
: mxLayerManager(pLayerManager_)
, pLayer(pSdrLayer_)
, pPropSet(ImplGetSdLayerPropertySet())
{
    // no defaults possible yet, a "set" would overwrite existing information
    // in view, which is currently needed for saving, because pLayer is not updated
    // from view.
}

SdLayer::~SdLayer() noexcept
{
}

// XServiceInfo
OUString SAL_CALL SdLayer::getImplementationName()
{
    return u"SdUnoLayer"_ustr;
}

sal_Bool SAL_CALL SdLayer::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService( this, ServiceName );
}

uno::Sequence< OUString > SAL_CALL SdLayer::getSupportedServiceNames()
{
    return { u"com.sun.star.drawing.Layer"_ustr };
}

// beans::XPropertySet
uno::Reference< beans::XPropertySetInfo > SAL_CALL SdLayer::getPropertySetInfo(  )
{
    SolarMutexGuard aGuard;
    return pPropSet->getPropertySetInfo();
}

void SAL_CALL SdLayer::setPropertyValue( const OUString& aPropertyName, const uno::Any& aValue )
{
    SolarMutexGuard aGuard;

    if(pLayer == nullptr || mxLayerManager == nullptr)
        throw lang::DisposedException();

    const SfxItemPropertyMapEntry* pEntry = pPropSet->getPropertyMapEntry(aPropertyName);

    switch( pEntry ? pEntry->nWID : -1 )
    {
    case WID_LAYER_LOCKED:
    {
        pLayer->SetLockedODF( cppu::any2bool(aValue) );
        set(LOCKED, cppu::any2bool(aValue)); // changes the View, if any exists
        break;
    }
    case WID_LAYER_PRINTABLE:
    {
        pLayer->SetPrintableODF( cppu::any2bool(aValue) );
        set(PRINTABLE, cppu::any2bool(aValue)); // changes the View, if any exists
        break;
    }
    case WID_LAYER_VISIBLE:
    {
        pLayer->SetVisibleODF( cppu::any2bool(aValue) );
        set(VISIBLE, cppu::any2bool(aValue)); // changes the View, if any exists
        break;
    }
    case WID_LAYER_NAME:
    {
        OUString aName;
        if(!(aValue >>= aName))
            throw lang::IllegalArgumentException();

        pLayer->SetName(aName);
        mxLayerManager->UpdateLayerView();
        break;
    }

    case WID_LAYER_TITLE:
    {
        OUString sTitle;
        if(!(aValue >>= sTitle))
            throw lang::IllegalArgumentException();

        pLayer->SetTitle(sTitle);
        break;
    }

    case WID_LAYER_DESC:
    {
        OUString sDescription;
        if(!(aValue >>= sDescription))
            throw lang::IllegalArgumentException();

        pLayer->SetDescription(sDescription);
        break;
    }

    default:
        throw beans::UnknownPropertyException( aPropertyName, static_cast<cppu::OWeakObject*>(this));
    }

    if( mxLayerManager->GetDocShell() )
        mxLayerManager->GetDocShell()->SetModified();
}

uno::Any SAL_CALL SdLayer::getPropertyValue( const OUString& PropertyName )
{
    SolarMutexGuard aGuard;

    if(pLayer == nullptr || mxLayerManager == nullptr)
        throw lang::DisposedException();

    const SfxItemPropertyMapEntry* pEntry = pPropSet->getPropertyMapEntry(PropertyName);

    uno::Any aValue;

    switch( pEntry ? pEntry->nWID : -1 )
    {
    case WID_LAYER_LOCKED:
        aValue <<= get( LOCKED );
        break;
    case WID_LAYER_PRINTABLE:
        aValue <<= get( PRINTABLE );
        break;
    case WID_LAYER_VISIBLE:
        aValue <<= get( VISIBLE );
        break;
    case WID_LAYER_NAME:
    {
        OUString aRet(pLayer->GetName());
        aValue <<= aRet;
        break;
    }
    case WID_LAYER_TITLE:
        aValue <<= pLayer->GetTitle();
        break;
    case WID_LAYER_DESC:
        aValue <<= pLayer->GetDescription();
        break;
    default:
        throw beans::UnknownPropertyException( PropertyName, static_cast<cppu::OWeakObject*>(this));
    }

    return aValue;
}

void SAL_CALL SdLayer::addPropertyChangeListener( const OUString& , const uno::Reference< beans::XPropertyChangeListener >& ) {}
void SAL_CALL SdLayer::removePropertyChangeListener( const OUString& , const uno::Reference< beans::XPropertyChangeListener >& ) {}
void SAL_CALL SdLayer::addVetoableChangeListener( const OUString& , const uno::Reference< beans::XVetoableChangeListener >& ) {}
void SAL_CALL SdLayer::removeVetoableChangeListener( const OUString& , const uno::Reference< beans::XVetoableChangeListener >& ) {}

bool SdLayer::get( LayerAttribute what ) noexcept
{
    if(pLayer && mxLayerManager.is())
    {
        // Try 1. is an arbitrary page open?
        ::sd::View *pView = mxLayerManager->GetView();
        SdrPageView* pSdrPageView = nullptr;
        if(pView)
            pSdrPageView = pView->GetSdrPageView();

        if(pSdrPageView)
        {
            OUString aLayerName = pLayer->GetName();
            switch(what)
            {
            case VISIBLE:   return pSdrPageView->IsLayerVisible(aLayerName);
            case PRINTABLE: return pSdrPageView->IsLayerPrintable(aLayerName);
            case LOCKED:    return pSdrPageView->IsLayerLocked(aLayerName);
            }
        }

        // Try 2. get info from FrameView
        if(mxLayerManager->GetDocShell())
        {
            ::sd::FrameView *pFrameView = mxLayerManager->GetDocShell()->GetFrameView();
            if(pFrameView)
                switch(what)
                {
                case VISIBLE:   return pFrameView->GetVisibleLayers().IsSet(pLayer->GetID());
                case PRINTABLE: return pFrameView->GetPrintableLayers().IsSet(pLayer->GetID());
                case LOCKED:    return pFrameView->GetLockedLayers().IsSet(pLayer->GetID());
                }
        }

        // no view at all, e.g. Draw embedded as OLE in text document, ODF default values
        switch(what)
        {
            case VISIBLE:   return true;
            case PRINTABLE: return true;
            case LOCKED:    return false;
        }

    }
    return false; //TODO: uno::Exception?
}

void SdLayer::set( LayerAttribute what, bool flag ) noexcept
{
    if(!(pLayer && mxLayerManager.is()))
        return;

    // Try 1. is an arbitrary page open?
    ::sd::View *pView = mxLayerManager->GetView();
    SdrPageView* pSdrPageView = nullptr;
    if(pView)
        pSdrPageView = pView->GetSdrPageView();

    if(pSdrPageView)
    {
        OUString aLayerName(pLayer->GetName());
        switch(what)
        {
        case VISIBLE:   pSdrPageView->SetLayerVisible(aLayerName,flag);
                        break;
        case PRINTABLE: pSdrPageView->SetLayerPrintable(aLayerName,flag);
                        break;
        case LOCKED:    pSdrPageView->SetLayerLocked(aLayerName,flag);
                        break;
        }
    }

    // Try 2. get info from FrameView
    if(!mxLayerManager->GetDocShell())
        return;

    ::sd::FrameView *pFrameView = mxLayerManager->GetDocShell()->GetFrameView();

    if(!pFrameView)
        return;

    SdrLayerIDSet aNewLayers;
    switch(what)
    {
    case VISIBLE:   aNewLayers = pFrameView->GetVisibleLayers();
                    break;
    case PRINTABLE: aNewLayers = pFrameView->GetPrintableLayers();
                    break;
    case LOCKED:    aNewLayers = pFrameView->GetLockedLayers();
                    break;
    }

    aNewLayers.Set(pLayer->GetID(),flag);

    switch(what)
    {
    case VISIBLE:   pFrameView->SetVisibleLayers(aNewLayers);
                    break;
    case PRINTABLE: pFrameView->SetPrintableLayers(aNewLayers);
                    break;
    case LOCKED:    pFrameView->SetLockedLayers(aNewLayers);
                    break;
    }
    return;
    //TODO: uno::Exception?
}

// css::container::XChild
uno::Reference<uno::XInterface> SAL_CALL SdLayer::getParent()
{
    SolarMutexGuard aGuard;

    if( !mxLayerManager.is() )
        throw lang::DisposedException();

    return uno::Reference<uno::XInterface> (static_cast<cppu::OWeakObject*>(mxLayerManager.get()), uno::UNO_QUERY);
}

void SAL_CALL SdLayer::setParent (const uno::Reference<uno::XInterface >& )
{
    throw lang::NoSupportException ();
}

// XComponent
void SAL_CALL SdLayer::dispose(  )
{
    mxLayerManager.clear();
    pLayer = nullptr;
}

void SAL_CALL SdLayer::addEventListener( const uno::Reference< lang::XEventListener >& )
{
    OSL_FAIL("not implemented!");
}

void SAL_CALL SdLayer::removeEventListener( const uno::Reference< lang::XEventListener >& )
{
    OSL_FAIL("not implemented!");
}

// class SdLayerManager
SdLayerManager::SdLayerManager( SdXImpressDocument& rMyModel ) noexcept
:mpModel( &rMyModel)
{
    mpLayers.reset(new SvUnoWeakContainer);
}

SdLayerManager::~SdLayerManager() noexcept
{
    dispose();
}

// XComponent
void SAL_CALL SdLayerManager::dispose(  )
{
    mpModel = nullptr;
    if( mpLayers )
    {
        mpLayers->dispose();
        mpLayers.reset();
    }
}

void SAL_CALL SdLayerManager::addEventListener( const uno::Reference< lang::XEventListener >& )
{
    OSL_FAIL("not implemented!");
}

void SAL_CALL SdLayerManager::removeEventListener( const uno::Reference< lang::XEventListener >& )
{
    OSL_FAIL("not implemented!");
}

// XServiceInfo
OUString SAL_CALL SdLayerManager::getImplementationName()
{
    return u"SdUnoLayerManager"_ustr;
}

sal_Bool SAL_CALL SdLayerManager::supportsService( const OUString& ServiceName )
{
 return cppu::supportsService( this, ServiceName );
}

uno::Sequence< OUString > SAL_CALL SdLayerManager::getSupportedServiceNames()
{
    return {u"com.sun.star.drawing.LayerManager"_ustr};
}

// XLayerManager
uno::Reference< drawing::XLayer > SAL_CALL SdLayerManager::insertNewByIndex( sal_Int32 nIndex )
{
    SolarMutexGuard aGuard;

    if( mpModel == nullptr )
        throw lang::DisposedException();

    rtl::Reference< SdLayer > xLayer;

    if( mpModel->mpDoc )
    {
        SdrLayerAdmin& rLayerAdmin = mpModel->mpDoc->GetLayerAdmin();
        sal_uInt16 nLayerCnt = rLayerAdmin.GetLayerCount();
        sal_Int32 nLayer = nLayerCnt - 2 + 1;
        OUString aLayerName;

        // Test for existing names
        while( aLayerName.isEmpty() || rLayerAdmin.GetLayer( aLayerName ) )
        {
            aLayerName = SdResId(STR_LAYER) + OUString::number(nLayer);
            ++nLayer;
        }

        SdrLayerAdmin& rLA=mpModel->mpDoc->GetLayerAdmin();
        const sal_Int32 nMax=rLA.GetLayerCount();
        if (nIndex>nMax) nIndex=nMax;
        xLayer = GetLayer (rLA.NewLayer(aLayerName,static_cast<sal_uInt16>(nIndex)));
        mpModel->SetModified();
    }
    return xLayer;
}

void SAL_CALL SdLayerManager::remove( const uno::Reference< drawing::XLayer >& xLayer )
{
    SolarMutexGuard aGuard;

    if( mpModel == nullptr )
        throw lang::DisposedException();

    SdLayer* pSdLayer = dynamic_cast<SdLayer*>(xLayer.get());

    if(pSdLayer && GetView())
    {
        const SdrLayer* pSdrLayer = pSdLayer->GetSdrLayer();
        GetView()->DeleteLayer( pSdrLayer->GetName() );

        UpdateLayerView();
    }

    mpModel->SetModified();
}

void SAL_CALL SdLayerManager::attachShapeToLayer( const uno::Reference< drawing::XShape >& xShape, const uno::Reference< drawing::XLayer >& xLayer )
{
    SolarMutexGuard aGuard;

    if( mpModel == nullptr )
        throw lang::DisposedException();

    SdLayer* pSdLayer = dynamic_cast<SdLayer*>(xLayer.get());
    if(pSdLayer==nullptr)
        return;
    SdrLayer* pSdrLayer = pSdLayer->GetSdrLayer();
    if(pSdrLayer==nullptr)
        return;

    SdrObject* pSdrObject = SdrObject::getSdrObjectFromXShape( xShape );

    if(pSdrObject)
        pSdrObject->SetLayer(pSdrLayer->GetID());

    mpModel->SetModified();
}

uno::Reference< drawing::XLayer > SAL_CALL SdLayerManager::getLayerForShape( const uno::Reference< drawing::XShape >& xShape )
{
    SolarMutexGuard aGuard;

    if( mpModel == nullptr )
        throw lang::DisposedException();

    if(!mpModel->mpDoc)
        return nullptr;

    SdrObject* pObj = SdrObject::getSdrObjectFromXShape( xShape );
    if(!pObj)
        return nullptr;

    SdrLayerID aId = pObj->GetLayer();
    SdrLayerAdmin& rLayerAdmin = mpModel->mpDoc->GetLayerAdmin();

    rtl::Reference< SdLayer > xLayer = GetLayer (rLayerAdmin.GetLayerPerID(aId));
    return xLayer;
}

// XIndexAccess
sal_Int32 SAL_CALL SdLayerManager::getCount()
{
    SolarMutexGuard aGuard;

    if( mpModel == nullptr )
        throw lang::DisposedException();

    if( mpModel->mpDoc )
    {
        SdrLayerAdmin& rLayerAdmin = mpModel->mpDoc->GetLayerAdmin();
        return rLayerAdmin.GetLayerCount();
    }

    return 0;
}

uno::Any SAL_CALL SdLayerManager::getByIndex( sal_Int32 nLayer )
{
    SolarMutexGuard aGuard;

    if( mpModel == nullptr )
        throw lang::DisposedException();

    if( nLayer >= getCount() || nLayer < 0 )
        throw lang::IndexOutOfBoundsException();

    uno::Any aAny;

    if( mpModel->mpDoc )
    {
        SdrLayerAdmin& rLayerAdmin = mpModel->mpDoc->GetLayerAdmin();
        rtl::Reference<SdLayer> xLayer (GetLayer (rLayerAdmin.GetLayer(static_cast<sal_uInt16>(nLayer))));
        aAny <<= uno::Reference<drawing::XLayer>(xLayer);
    }
    return aAny;
}

// XNameAccess
uno::Any SAL_CALL SdLayerManager::getByName( const OUString& aName )
{
    SolarMutexGuard aGuard;

    if( (mpModel == nullptr) || (mpModel->mpDoc == nullptr ) )
        throw lang::DisposedException();

    SdrLayerAdmin& rLayerAdmin = mpModel->mpDoc->GetLayerAdmin();
    SdrLayer* pLayer = rLayerAdmin.GetLayer(aName);
    if( pLayer == nullptr )
        throw container::NoSuchElementException();

    return uno::Any( css::uno::Reference< css::drawing::XLayer>(GetLayer(pLayer)) );
}

uno::Sequence< OUString > SAL_CALL SdLayerManager::getElementNames()
{
    SolarMutexGuard aGuard;

    if( mpModel == nullptr )
        throw lang::DisposedException();

    SdrLayerAdmin& rLayerAdmin = mpModel->mpDoc->GetLayerAdmin();
    const sal_uInt16 nLayerCount = rLayerAdmin.GetLayerCount();

    uno::Sequence< OUString > aSeq( nLayerCount );

    OUString* pStrings = aSeq.getArray();

    for( sal_uInt16 nLayer = 0; nLayer < nLayerCount; nLayer++ )
    {
        SdrLayer* pLayer = rLayerAdmin.GetLayer( nLayer );
        if( pLayer )
            *pStrings++ = pLayer->GetName();
    }

    return aSeq;
}

sal_Bool SAL_CALL SdLayerManager::hasByName( const OUString& aName )
{
    SolarMutexGuard aGuard;

    if( mpModel == nullptr )
        throw lang::DisposedException();

    SdrLayerAdmin& rLayerAdmin = mpModel->mpDoc->GetLayerAdmin();

    return nullptr != rLayerAdmin.GetLayer(aName);
}

// XElementAccess
uno::Type SAL_CALL SdLayerManager::getElementType()
{
    return cppu::UnoType<drawing::XLayer>::get();
}

sal_Bool SAL_CALL SdLayerManager::hasElements()
{
    return getCount() > 0;
}

/**
 * If something was changed at the layers, this methods takes care that the
 * changes are made visible in sdbcx::View.
 */
void SdLayerManager::UpdateLayerView() const noexcept
{
    if(!mpModel->mpDocShell)
        return;

    ::sd::DrawViewShell* pDrViewSh = dynamic_cast< ::sd::DrawViewShell* >( mpModel->mpDocShell->GetViewShell());

    if(pDrViewSh)
    {
        bool bLayerMode = pDrViewSh->IsLayerModeActive();
        pDrViewSh->ChangeEditMode(pDrViewSh->GetEditMode(), !bLayerMode);
        pDrViewSh->ChangeEditMode(pDrViewSh->GetEditMode(), bLayerMode);
    }

    mpModel->mpDoc->SetChanged();
}

/** */
::sd::View* SdLayerManager::GetView() const noexcept
{
    if( mpModel->mpDocShell )
    {
        ::sd::ViewShell* pViewSh = mpModel->mpDocShell->GetViewShell();
        if(pViewSh)
            return pViewSh->GetView();
    }
    return nullptr;
}

/** Use the <member>mpLayers</member> container of weak references to either
    retrieve and return a previously created <type>XLayer</type> object for
    the given <type>SdrLayer</type> object or create and remember a new one.
*/
rtl::Reference<SdLayer> SdLayerManager::GetLayer (SdrLayer* pLayer)
{
    rtl::Reference<SdLayer> xLayer;

    // Search existing xLayer for the given pLayer.
    if (mpLayers->findRef(xLayer, pLayer))
        return xLayer;

    // Create the xLayer if necessary.
    xLayer = new SdLayer (this, pLayer);

    // Remember the new xLayer for future calls.
    mpLayers->insert(xLayer);

    return xLayer;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
