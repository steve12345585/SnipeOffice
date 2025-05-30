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

#include <sdr/contact/viewcontactofunocontrol.hxx>
#include <sdr/contact/viewobjectcontactofunocontrol.hxx>
#include <com/sun/star/container/XChild.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/util/XCloneable.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/lang/XEventListener.hpp>
#include <cppuhelper/implbase.hxx>
#include <comphelper/processfactory.hxx>
#include <svx/svdouno.hxx>
#include <svx/svdpagv.hxx>
#include <svx/svdmodel.hxx>
#include <svx/dialmgr.hxx>
#include <svx/strings.hrc>
#include <svx/svdview.hxx>
#include <svx/svdorect.hxx>
#include <svx/svdviter.hxx>
#include <rtl/ref.hxx>
#include <svx/sdrpagewindow.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <tools/debug.hxx>
#include <o3tl/sorted_vector.hxx>

using namespace ::com::sun::star;
using namespace sdr::contact;


class SdrControlEventListenerImpl : public ::cppu::WeakImplHelper< css::lang::XEventListener >
{
protected:
    SdrUnoObj*                  pObj;

public:
    explicit SdrControlEventListenerImpl(SdrUnoObj* _pObj)
    :   pObj(_pObj)
    {}

    // XEventListener
    virtual void SAL_CALL disposing( const css::lang::EventObject& Source ) override;

    void StopListening(const uno::Reference< lang::XComponent >& xComp);
    void StartListening(const uno::Reference< lang::XComponent >& xComp);
};

// XEventListener
void SAL_CALL SdrControlEventListenerImpl::disposing( const css::lang::EventObject& /*Source*/)
{
    if (pObj)
    {
        pObj->m_xUnoControlModel = nullptr;
    }
}

void SdrControlEventListenerImpl::StopListening(const uno::Reference< lang::XComponent >& xComp)
{
    if (xComp.is())
        xComp->removeEventListener(this);
}

void SdrControlEventListenerImpl::StartListening(const uno::Reference< lang::XComponent >& xComp)
{
    if (xComp.is())
        xComp->addEventListener(this);
}


struct SdrUnoObjDataHolder
{
    mutable ::rtl::Reference< SdrControlEventListenerImpl >
                                    pEventListener;
};


namespace
{
    void lcl_ensureControlVisibility( SdrView const * _pView, const SdrUnoObj* _pObject, bool _bVisible )
    {
        assert(_pObject && "lcl_ensureControlVisibility: no object -> no survival!");

        SdrPageView* pPageView = _pView ? _pView->GetSdrPageView() : nullptr;
        DBG_ASSERT( pPageView, "lcl_ensureControlVisibility: no view found!" );
        if ( !pPageView )
            return;

        ViewContact& rUnoControlContact( _pObject->GetViewContact() );

        for ( sal_uInt32 i = 0; i < pPageView->PageWindowCount(); ++i )
        {
            SdrPageWindow* pPageWindow = pPageView->GetPageWindow( i );
            DBG_ASSERT( pPageWindow, "lcl_ensureControlVisibility: invalid PageViewWindow!" );
            if ( !pPageWindow )
                continue;

            if ( !pPageWindow->HasObjectContact() )
                continue;

            ObjectContact& rPageViewContact( pPageWindow->GetObjectContact() );
            const ViewObjectContact& rViewObjectContact( rUnoControlContact.GetViewObjectContact( rPageViewContact ) );
            const ViewObjectContactOfUnoControl* pUnoControlContact = dynamic_cast< const ViewObjectContactOfUnoControl* >( &rViewObjectContact );
            DBG_ASSERT( pUnoControlContact, "lcl_ensureControlVisibility: wrong ViewObjectContact type!" );
            if ( !pUnoControlContact )
                continue;

            pUnoControlContact->ensureControlVisibility( _bVisible );
        }
    }
}

SdrUnoObj::SdrUnoObj(
    SdrModel& rSdrModel,
    const OUString& rModelName)
:   SdrRectObj(rSdrModel),
    m_pImpl( new SdrUnoObjDataHolder )
{
    osl_atomic_increment(&m_refCount); // prevent deletion during creation
    m_bIsUnoObj = true;

    m_pImpl->pEventListener = new SdrControlEventListenerImpl(this);

    // only an owner may create independently
    if (!rModelName.isEmpty())
        CreateUnoControlModel(rModelName);
    osl_atomic_decrement(&m_refCount);
}

SdrUnoObj::SdrUnoObj( SdrModel& rSdrModel, SdrUnoObj const & rSource)
:   SdrRectObj(rSdrModel, rSource),
    m_pImpl( new SdrUnoObjDataHolder )
{
    m_bIsUnoObj = true;

    m_pImpl->pEventListener = new SdrControlEventListenerImpl(this);

    m_aUnoControlModelTypeName = rSource.m_aUnoControlModelTypeName;
    m_aUnoControlTypeName = rSource.m_aUnoControlTypeName;

    // copy the uno control model
    const uno::Reference< awt::XControlModel >& xSourceControlModel = rSource.GetUnoControlModel();
    if ( xSourceControlModel.is() )
    {
        try
        {
            uno::Reference< util::XCloneable > xClone( xSourceControlModel, uno::UNO_QUERY_THROW );
            m_xUnoControlModel.set( xClone->createClone(), uno::UNO_QUERY_THROW );
        }
        catch( const uno::Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("svx");
        }
    }

    // get service name of the control from the control model
    uno::Reference< beans::XPropertySet > xSet(m_xUnoControlModel, uno::UNO_QUERY);
    if (xSet.is())
    {
        uno::Any aValue( xSet->getPropertyValue(u"DefaultControl"_ustr) );
        OUString aStr;

        if( aValue >>= aStr )
            m_aUnoControlTypeName = aStr;
    }

    uno::Reference< lang::XComponent > xComp(m_xUnoControlModel, uno::UNO_QUERY);
    if (xComp.is())
        m_pImpl->pEventListener->StartListening(xComp);
}

SdrUnoObj::SdrUnoObj(
    SdrModel& rSdrModel,
    const OUString& rModelName,
    const uno::Reference< lang::XMultiServiceFactory >& rxSFac)
:   SdrRectObj(rSdrModel),
    m_pImpl( new SdrUnoObjDataHolder )
{
    m_bIsUnoObj = true;

    m_pImpl->pEventListener = new SdrControlEventListenerImpl(this);

    // only an owner may create independently
    if (!rModelName.isEmpty())
        CreateUnoControlModel(rModelName,rxSFac);
}

SdrUnoObj::~SdrUnoObj()
{
    try
    {
        // clean up the control model
        uno::Reference< lang::XComponent > xComp(m_xUnoControlModel, uno::UNO_QUERY);
        if (xComp.is())
        {
            // is the control model owned by its environment?
            uno::Reference< container::XChild > xContent(m_xUnoControlModel, uno::UNO_QUERY);
            if (xContent.is() && !xContent->getParent().is())
                xComp->dispose();
            else
                m_pImpl->pEventListener->StopListening(xComp);
        }
    }
    catch( const uno::Exception& )
    {
        TOOLS_WARN_EXCEPTION( "svx", "SdrUnoObj::~SdrUnoObj" );
    }
}

void SdrUnoObj::TakeObjInfo(SdrObjTransformInfoRec& rInfo) const
{
    rInfo.bRotateFreeAllowed        =   false;
    rInfo.bRotate90Allowed          =   false;
    rInfo.bMirrorFreeAllowed        =   false;
    rInfo.bMirror45Allowed          =   false;
    rInfo.bMirror90Allowed          =   false;
    rInfo.bTransparenceAllowed = false;
    rInfo.bShearAllowed             =   false;
    rInfo.bEdgeRadiusAllowed        =   false;
    rInfo.bNoOrthoDesired           =   false;
    rInfo.bCanConvToPath            =   false;
    rInfo.bCanConvToPoly            =   false;
    rInfo.bCanConvToPathLineToArea  =   false;
    rInfo.bCanConvToPolyLineToArea  =   false;
    rInfo.bCanConvToContour = false;
}

SdrObjKind SdrUnoObj::GetObjIdentifier() const
{
    return SdrObjKind::UNO;
}

void SdrUnoObj::SetContextWritingMode( const sal_Int16 _nContextWritingMode )
{
    try
    {
        uno::Reference< beans::XPropertySet > xModelProperties( GetUnoControlModel(), uno::UNO_QUERY_THROW );
        xModelProperties->setPropertyValue( u"ContextWritingMode"_ustr, uno::Any( _nContextWritingMode ) );
    }
    catch( const uno::Exception& )
    {
        DBG_UNHANDLED_EXCEPTION("svx");
    }
}

OUString SdrUnoObj::TakeObjNameSingul() const
{
    OUString sName(SvxResId(STR_ObjNameSingulUno));

    OUString aName(GetName());
    if (!aName.isEmpty())
        sName += " '" + aName + "'";

    return sName;
}

OUString SdrUnoObj::TakeObjNamePlural() const
{
    return SvxResId(STR_ObjNamePluralUno);
}

rtl::Reference<SdrObject> SdrUnoObj::CloneSdrObject(SdrModel& rTargetModel) const
{
    return new SdrUnoObj(rTargetModel, *this);
}

void SdrUnoObj::NbcResize(const Point& rRef, const Fraction& xFact, const Fraction& yFact)
{
    SdrRectObj::NbcResize(rRef,xFact,yFact);

    if (maGeo.m_nShearAngle==0_deg100 && maGeo.m_nRotationAngle==0_deg100)
        return;

    // small correctors
    if (maGeo.m_nRotationAngle>=9000_deg100 && maGeo.m_nRotationAngle<27000_deg100)
    {
        moveRectangle(getRectangle().Left() - getRectangle().Right(), getRectangle().Top() - getRectangle().Bottom());
    }

    maGeo.m_nRotationAngle  = 0_deg100;
    maGeo.m_nShearAngle = 0_deg100;
    maGeo.mfSinRotationAngle       = 0.0;
    maGeo.mfCosRotationAngle       = 1.0;
    maGeo.mfTanShearAngle       = 0.0;
    SetBoundAndSnapRectsDirty();
}


bool SdrUnoObj::hasSpecialDrag() const
{
    // no special drag; we have no rounding rect and
    // do want frame handles
    return false;
}

void SdrUnoObj::NbcSetLayer( SdrLayerID _nLayer )
{
    if ( GetLayer() == _nLayer )
    {   // redundant call -> not interested in doing anything here
        SdrRectObj::NbcSetLayer( _nLayer );
        return;
    }

    // we need some special handling here in case we're moved from an invisible layer
    // to a visible one, or vice versa
    // (relative to a layer. Remember that the visibility of a layer is a view attribute
    // - the same layer can be visible in one view, and invisible in another view, at the
    // same time)

    // collect all views in which our old layer is visible
    o3tl::sorted_vector< SdrView* > aPreviouslyVisible;

    {
        SdrViewIter::ForAllViews(this,
            [&aPreviouslyVisible] (SdrView* pView)
            {
                aPreviouslyVisible.insert( pView );
                return false;
            });
    }

    SdrRectObj::NbcSetLayer( _nLayer );

    // collect all views in which our new layer is visible
    o3tl::sorted_vector< SdrView* > aNewlyVisible;

    SdrViewIter::ForAllViews( this,
        [&aPreviouslyVisible, &aNewlyVisible] (SdrView* pView)
        {
            if ( aPreviouslyVisible.erase(pView) == 0 )
            {
                // in pView, we were visible _before_ the layer change, and are
                // _not_ visible after the layer change
                // => remember this view, as our visibility there changed
                aNewlyVisible.insert( pView );
            }
        });

    // now aPreviouslyVisible contains all views where we became invisible
    for (const auto& rpView : aPreviouslyVisible)
    {
        lcl_ensureControlVisibility( rpView, this, false );
    }

    // and aNewlyVisible all views where we became visible
    for (const auto& rpView : aNewlyVisible)
    {
        lcl_ensureControlVisibility( rpView, this, true );
    }
}

void SdrUnoObj::CreateUnoControlModel(const OUString& rModelName)
{
    DBG_ASSERT(!m_xUnoControlModel.is(), "model already exists");

    m_aUnoControlModelTypeName = rModelName;

    uno::Reference< awt::XControlModel >   xModel;
    const uno::Reference< uno::XComponentContext >& xContext( ::comphelper::getProcessComponentContext() );
    if (!m_aUnoControlModelTypeName.isEmpty() )
    {
        xModel.set(xContext->getServiceManager()->createInstanceWithContext(
            m_aUnoControlModelTypeName, xContext), uno::UNO_QUERY);

        if (xModel.is())
            SetChanged();
    }

    SetUnoControlModel(xModel);
}

void SdrUnoObj::CreateUnoControlModel(const OUString& rModelName,
                                      const uno::Reference< lang::XMultiServiceFactory >& rxSFac)
{
    DBG_ASSERT(!m_xUnoControlModel.is(), "model already exists");

    m_aUnoControlModelTypeName = rModelName;

    uno::Reference< awt::XControlModel >   xModel;
    if (!m_aUnoControlModelTypeName.isEmpty() && rxSFac.is() )
    {
        xModel.set(rxSFac->createInstance(m_aUnoControlModelTypeName), uno::UNO_QUERY);

        if (xModel.is())
            SetChanged();
    }

    SetUnoControlModel(xModel);
}

void SdrUnoObj::SetUnoControlModel( const uno::Reference< awt::XControlModel >& xModel)
{
    if (m_xUnoControlModel.is())
    {
        uno::Reference< lang::XComponent > xComp(m_xUnoControlModel, uno::UNO_QUERY);
        if (xComp.is())
            m_pImpl->pEventListener->StopListening(xComp);
    }

    m_xUnoControlModel = xModel;

    // control model has to contain service name of the control
    if (m_xUnoControlModel.is())
    {
        uno::Reference< beans::XPropertySet > xSet(m_xUnoControlModel, uno::UNO_QUERY);
        if (xSet.is())
        {
            uno::Any aValue( xSet->getPropertyValue(u"DefaultControl"_ustr) );
            OUString aStr;
            if( aValue >>= aStr )
                m_aUnoControlTypeName = aStr;
        }

        uno::Reference< lang::XComponent > xComp(m_xUnoControlModel, uno::UNO_QUERY);
        if (xComp.is())
            m_pImpl->pEventListener->StartListening(xComp);
    }

    // invalidate all ViewObject contacts
    ViewContactOfUnoControl* pVC = nullptr;
    if ( impl_getViewContact( pVC ) )
    {
        // flushViewObjectContacts() removes all existing VOCs for the local DrawHierarchy. This
        // is always allowed since they will be re-created on demand (and with the changed model)
        GetViewContact().flushViewObjectContacts();
    }
}


uno::Reference< awt::XControl > SdrUnoObj::GetUnoControl(const SdrView& _rView, const OutputDevice& _rOut) const
{
    uno::Reference< awt::XControl > xControl;

    SdrPageView* pPageView = _rView.GetSdrPageView();
    OSL_ENSURE( pPageView && getSdrPageFromSdrObject() == pPageView->GetPage(), "SdrUnoObj::GetUnoControl: This object is not displayed in that particular view!" );
    if ( !pPageView || getSdrPageFromSdrObject() != pPageView->GetPage() )
        return nullptr;

    SdrPageWindow* pPageWindow = pPageView->FindPageWindow( _rOut );
    OSL_ENSURE( pPageWindow, "SdrUnoObj::GetUnoControl: did not find my SdrPageWindow!" );
    if ( !pPageWindow )
        return nullptr;

    ViewObjectContact& rViewObjectContact( GetViewContact().GetViewObjectContact( pPageWindow->GetObjectContact() ) );
    ViewObjectContactOfUnoControl* pUnoContact = dynamic_cast< ViewObjectContactOfUnoControl* >( &rViewObjectContact );
    OSL_ENSURE( pUnoContact, "SdrUnoObj::GetUnoControl: wrong contact type!" );
    if ( pUnoContact )
        xControl = pUnoContact->getControl();

    return xControl;
}


uno::Reference< awt::XControl > SdrUnoObj::GetTemporaryControlForWindow(
    const vcl::Window& _rWindow, uno::Reference< awt::XControlContainer >& _inout_ControlContainer ) const
{
    uno::Reference< awt::XControl > xControl;

    ViewContactOfUnoControl* pVC = nullptr;
    if ( impl_getViewContact( pVC ) )
        xControl = pVC->getTemporaryControlForWindow( _rWindow, _inout_ControlContainer );

    return xControl;
}


bool SdrUnoObj::impl_getViewContact( ViewContactOfUnoControl*& _out_rpContact ) const
{
    ViewContact& rViewContact( GetViewContact() );
    _out_rpContact = dynamic_cast< ViewContactOfUnoControl* >( &rViewContact );
    DBG_ASSERT( _out_rpContact, "SdrUnoObj::impl_getViewContact: could not find my ViewContact!" );
    return ( _out_rpContact != nullptr );
}


std::unique_ptr<sdr::contact::ViewContact> SdrUnoObj::CreateObjectSpecificViewContact()
{
  return std::make_unique<sdr::contact::ViewContactOfUnoControl>( *this );
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
