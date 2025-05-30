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

#include <SdUnoDrawView.hxx>

#include <DrawController.hxx>
#include <DrawDocShell.hxx>
#include <DrawViewShell.hxx>
#include <drawdoc.hxx>
#include <unolayer.hxx>
#include <unomodel.hxx>
#include <Window.hxx>
#include <pres.hxx>

#include <comphelper/processfactory.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/viewfrm.hxx>
#include <svx/svdpagv.hxx>
#include <svx/unopage.hxx>
#include <sfx2/zoomitem.hxx>
#include <com/sun/star/drawing/DrawViewMode.hpp>
#include <com/sun/star/drawing/ShapeCollection.hpp>
#include <com/sun/star/drawing/XLayerManager.hpp>
#include <com/sun/star/view/DocumentZoomType.hpp>

#include <vector>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::drawing;

namespace sd {

SdUnoDrawView::SdUnoDrawView(
    DrawViewShell& rViewShell,
    View& rView) noexcept
    :   mrDrawViewShell(rViewShell),
        mrView(rView)
{
}

SdUnoDrawView::~SdUnoDrawView() noexcept
{
}

bool SdUnoDrawView::getMasterPageMode() const noexcept
{
    return (mrDrawViewShell.GetEditMode() == EditMode::MasterPage);
}

void SdUnoDrawView::setMasterPageMode (bool bMasterPageMode) noexcept
{
    if ((mrDrawViewShell.GetEditMode() == EditMode::MasterPage) != bMasterPageMode)
    {
        mrDrawViewShell.ChangeEditMode (
            bMasterPageMode ? EditMode::MasterPage : EditMode::Page,
            mrDrawViewShell.IsLayerModeActive());
    }
}

bool SdUnoDrawView::getLayerMode() const noexcept
{
    return mrDrawViewShell.IsLayerModeActive();
}

void SdUnoDrawView::setLayerMode (bool bLayerMode) noexcept
{
    if (mrDrawViewShell.IsLayerModeActive() != bLayerMode)
    {
        mrDrawViewShell.ChangeEditMode (
            mrDrawViewShell.GetEditMode(),
            bLayerMode);
    }
}

rtl::Reference<SdLayer> SdUnoDrawView::getActiveLayer() const
{
    // Retrieve the layer manager from the model.
    SdXImpressDocument* pModel = GetModel();
    if (pModel == nullptr)
        return nullptr;

    SdDrawDocument* pSdModel = pModel->GetDoc();
    if (pSdModel == nullptr)
        return nullptr;

    // From the model get the current SdrLayer object via the layer admin.
    SdrLayerAdmin& rLayerAdmin = pSdModel->GetLayerAdmin ();
    SdrLayer* pLayer = rLayerAdmin.GetLayer (mrView.GetActiveLayer());
    if (pLayer == nullptr)
        return nullptr;

    // Get the corresponding XLayer object from the implementation
    // object of the layer manager.
    Reference<drawing::XLayerManager> xManager (pModel->getLayerManager(), uno::UNO_QUERY);
    SdLayerManager* pManager = dynamic_cast<SdLayerManager*> (xManager.get());
    if (!pManager)
        return nullptr;

    return pManager->GetLayer (pLayer);
}

void SdUnoDrawView::setActiveLayer (const Reference<drawing::XLayer>& rxLayer)
{
    // Get the SdrLayer object corresponding to the given reference.
    if ( ! rxLayer.is())
        return;

    SdLayer* pLayer = dynamic_cast<SdLayer*> (rxLayer.get());
    if (pLayer == nullptr)
        return;

    SdrLayer* pSdrLayer = pLayer->GetSdrLayer();
    if (pSdrLayer == nullptr)
        return;

    // Set the new active layer and make the change visible.
    mrView.SetActiveLayer (pSdrLayer->GetName());
    mrDrawViewShell.ResetActualLayer ();
}

// XSelectionSupplier

sal_Bool SAL_CALL SdUnoDrawView::select( const Any& aSelection )
{
    bool bOk = true;

    ::std::vector<SdrObject*> aObjects;

    SdrPage* pSdrPage = nullptr;

    Reference< drawing::XShape > xShape;
    aSelection >>= xShape;

    if(xShape.is())
    {
        SdrObject* pObj = SdrObject::getSdrObjectFromXShape( xShape );
        if( pObj )
        {
            pSdrPage = pObj->getSdrPageFromSdrObject();
            aObjects.push_back( pObj );
        }
        else
        {
            bOk = false;
        }
    }
    else
    {
        Reference< drawing::XShapes > xShapes;
        aSelection >>= xShapes;
        if( xShapes.is() )
        {
            const sal_uInt32 nCount = xShapes->getCount();
            for( sal_uInt32 i = 0; i < nCount; i++ )
            {
                xShapes->getByIndex(i) >>= xShape;
                if( xShape.is() )
                {
                    SdrObject* pObj = SdrObject::getSdrObjectFromXShape(xShape);
                    if( !pObj )
                    {
                        bOk = false;
                        break;
                    }

                    if( pSdrPage == nullptr )
                    {
                        pSdrPage = pObj->getSdrPageFromSdrObject();
                    }
                    else if( pSdrPage != pObj->getSdrPageFromSdrObject() )
                    {
                        bOk = false;
                        break;
                    }

                    aObjects.push_back( pObj );
                }
            }
        }
    }

    if( bOk )
    {
        if( pSdrPage )
        {
            setMasterPageMode( pSdrPage->IsMasterPage() );
            mrDrawViewShell.SwitchPage( (pSdrPage->GetPageNum() - 1) >> 1 );
            mrDrawViewShell.WriteFrameViewData();
        }

        SdrPageView *pPV = mrView.GetSdrPageView();

        if(pPV)
        {
            // first deselect all
            mrView.UnmarkAllObj( pPV );

            for( SdrObject* pObj : aObjects )
            {
                mrView.MarkObj( pObj, pPV );
            }
        }
        else
        {
            bOk = false;
        }
    }

    return bOk;
}

Any SAL_CALL SdUnoDrawView::getSelection()
{
    Any aAny;

    if( mrView.IsTextEdit() )
        mrView.getTextSelection( aAny );

    if( !aAny.hasValue() )
    {
        const SdrMarkList& rMarkList = mrView.GetMarkedObjectList();
        const size_t nCount = rMarkList.GetMarkCount();
        if( nCount )
        {
            Reference< drawing::XShapes > xShapes = drawing::ShapeCollection::create(
                        comphelper::getProcessComponentContext());
            for( size_t nNum = 0; nNum < nCount; ++nNum)
            {
                SdrMark *pMark = rMarkList.GetMark(nNum);
                if(pMark==nullptr)
                    continue;

                SdrObject *pObj = pMark->GetMarkedSdrObj();
                if(pObj==nullptr || pObj->getSdrPageFromSdrObject() == nullptr)
                    continue;

                Reference< drawing::XDrawPage > xPage( pObj->getSdrPageFromSdrObject()->getUnoPage(), UNO_QUERY);

                if(!xPage.is())
                    continue;

                SvxDrawPage* pDrawPage = comphelper::getFromUnoTunnel<SvxDrawPage>( xPage );

                if(pDrawPage==nullptr)
                    continue;

                Reference< drawing::XShape > xShape( pObj->getUnoShape(), UNO_QUERY );

                if(xShape.is())
                    xShapes->add(xShape);
            }
            aAny <<= xShapes;
        }
    }

    return aAny;
}

void SAL_CALL SdUnoDrawView::addSelectionChangeListener (
    const css::uno::Reference<css::view::XSelectionChangeListener>&)
{}

void SAL_CALL SdUnoDrawView::removeSelectionChangeListener (
    const css::uno::Reference<css::view::XSelectionChangeListener>&)
{}

void SdUnoDrawView::setFastPropertyValue (
    sal_Int32 nHandle,
        const Any& rValue)
{
    switch( nHandle )
    {
        case DrawController::PROPERTY_CURRENTPAGE:
            {
                Reference< drawing::XDrawPage > xPage;
                rValue >>= xPage;
                setCurrentPage( xPage );
            }
            break;

        case DrawController::PROPERTY_MASTERPAGEMODE:
            {
                bool bValue = false;
                rValue >>= bValue;
                setMasterPageMode( bValue );
            }
            break;

        case DrawController::PROPERTY_LAYERMODE:
            {
                bool bValue = false;
                rValue >>= bValue;
                setLayerMode( bValue );
            }
            break;
        case DrawController::PROPERTY_ACTIVE_LAYER:
            {
                Reference<drawing::XLayer> xLayer;
                rValue >>= xLayer;
                setActiveLayer (xLayer);
            }
            break;
        case DrawController::PROPERTY_ZOOMVALUE:
            {
                sal_Int16 nZoom = 0;
                rValue >>= nZoom;
                SetZoom( nZoom );
            }
            break;
        case DrawController::PROPERTY_ZOOMTYPE:
            {
                sal_Int16 nType = 0;
                rValue >>= nType;
                SetZoomType( nType );
            }
            break;
        case DrawController::PROPERTY_VIEWOFFSET:
            {
                awt::Point aOffset;
                rValue >>= aOffset;
                SetViewOffset( aOffset );
            }
            break;
        default:
            throw beans::UnknownPropertyException( OUString::number(nHandle), static_cast<cppu::OWeakObject*>(this));
    }
}

Any SAL_CALL SdUnoDrawView::getFastPropertyValue (
    sal_Int32 nHandle)
{
    Any aValue;
    switch( nHandle )
    {
        case DrawController::PROPERTY_CURRENTPAGE:
            aValue <<= getCurrentPage();
            break;

        case DrawController::PROPERTY_MASTERPAGEMODE:
            aValue <<= getMasterPageMode();
            break;

        case DrawController::PROPERTY_LAYERMODE:
            aValue <<= getLayerMode();
            break;

        case DrawController::PROPERTY_ACTIVE_LAYER:
            aValue <<= Reference<XLayer>(getActiveLayer());
            break;

        case DrawController::PROPERTY_ZOOMVALUE:
            aValue <<= GetZoom();
            break;
        case DrawController::PROPERTY_ZOOMTYPE:
            aValue <<= sal_Int16(css::view::DocumentZoomType::BY_VALUE);
            break;
        case DrawController::PROPERTY_VIEWOFFSET:
            aValue <<= GetViewOffset();
            break;

        case DrawController::PROPERTY_DRAWVIEWMODE:
            aValue = getDrawViewMode();
            break;

        default:
            throw beans::UnknownPropertyException( OUString::number(nHandle), static_cast<cppu::OWeakObject*>(this));
    }

    return aValue;
}

// XDrawView

void SAL_CALL SdUnoDrawView::setCurrentPage (
    const Reference< drawing::XDrawPage >& xPage )
{
    SvxDrawPage* pDrawPage = comphelper::getFromUnoTunnel<SvxDrawPage>( xPage );
    SdrPage *pSdrPage = pDrawPage ? pDrawPage->GetSdrPage() : nullptr;

    if(pSdrPage)
    {
        // End editing of text.  Otherwise the edited text object would
        // still be visible on the new page.
        mrDrawViewShell.GetView()->SdrEndTextEdit();

        setMasterPageMode( pSdrPage->IsMasterPage() );
        mrDrawViewShell.SwitchPage( (pSdrPage->GetPageNum() - 1) >> 1 );
        mrDrawViewShell.WriteFrameViewData();
    }
}

Reference< drawing::XDrawPage > SAL_CALL SdUnoDrawView::getCurrentPage()
{
    Reference< drawing::XDrawPage >  xPage;

    SdrPageView *pPV = mrView.GetSdrPageView();
    SdrPage* pPage = pPV ? pPV->GetPage() : nullptr;

    if(pPage)
        xPage.set( pPage->getUnoPage(), UNO_QUERY );

    return xPage;
}

sal_Int16 SdUnoDrawView::GetZoom() const
{
    if (mrDrawViewShell.GetActiveWindow() )
    {
        return static_cast<sal_Int16>(mrDrawViewShell.GetActiveWindow()->GetZoom());
    }
    else
    {
        return 0;
    }
}

void SdUnoDrawView::SetZoom( sal_Int16 nZoom )
{
    SvxZoomItem aZoomItem( SvxZoomType::PERCENT, nZoom );

    SfxViewFrame* pViewFrame = mrDrawViewShell.GetViewFrame();
    if( pViewFrame )
    {
        SfxDispatcher* pDispatcher = pViewFrame->GetDispatcher();
        if( pDispatcher )
        {
            pDispatcher->ExecuteList(SID_ATTR_ZOOM, SfxCallMode::SYNCHRON,
                    { &aZoomItem });
        }
    }
}

void SdUnoDrawView::SetViewOffset(const awt::Point& rWinPos )
{
    Point aWinPos( rWinPos.X, rWinPos.Y );
    aWinPos += mrDrawViewShell.GetViewOrigin();
    mrDrawViewShell.SetWinViewPos( aWinPos );
}

awt::Point SdUnoDrawView::GetViewOffset() const
{
    Point aRet = mrDrawViewShell.GetWinViewPos();
    aRet -= mrDrawViewShell.GetViewOrigin();

    return awt::Point( aRet.X(), aRet.Y() );
}

void SdUnoDrawView::SetZoomType ( sal_Int16 nType )
{
    SfxViewFrame* pViewFrame = mrDrawViewShell.GetViewFrame();
    if( !pViewFrame )
        return;

    SfxDispatcher* pDispatcher = pViewFrame->GetDispatcher();
    if( !pDispatcher )
        return;

    SvxZoomType eZoomType;
    switch( nType )
    {
        case css::view::DocumentZoomType::OPTIMAL:
            eZoomType = SvxZoomType::OPTIMAL;
            break;

        case css::view::DocumentZoomType::PAGE_WIDTH:
        case css::view::DocumentZoomType::PAGE_WIDTH_EXACT:
            eZoomType = SvxZoomType::PAGEWIDTH;
            break;

        case css::view::DocumentZoomType::ENTIRE_PAGE:
            eZoomType = SvxZoomType::WHOLEPAGE;
            break;

        default:
            return;
    }
    SvxZoomItem aZoomItem( eZoomType );
    pDispatcher->ExecuteList(SID_ATTR_ZOOM, SfxCallMode::SYNCHRON,
            { &aZoomItem });
}

SdXImpressDocument* SdUnoDrawView::GetModel() const noexcept
{
    if (mrView.GetDocSh()!=nullptr)
    {
        Reference<frame::XModel> xModel (mrView.GetDocSh()->GetModel());
        return comphelper::getFromUnoTunnel<SdXImpressDocument>(xModel);
    }
    else
        return nullptr;
}

Any SdUnoDrawView::getDrawViewMode() const
{
    Any aRet;
    switch( mrDrawViewShell.GetPageKind() )
    {
    case PageKind::Notes:  aRet <<= DrawViewMode_NOTES; break;
    case PageKind::Handout: aRet <<= DrawViewMode_HANDOUT; break;
    case PageKind::Standard: aRet <<= DrawViewMode_DRAW; break;
    }
    return aRet;
}

// XServiceInfo
OUString SAL_CALL SdUnoDrawView::getImplementationName(  )
{
    return u"com.sun.star.comp.sd.SdUnoDrawView"_ustr ;
}

sal_Bool SAL_CALL SdUnoDrawView::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService( this, ServiceName );
}

Sequence< OUString > SAL_CALL SdUnoDrawView::getSupportedServiceNames(  )
{
    return { u"com.sun.star.drawing.DrawingDocumentDrawView"_ustr };
}

} // end of namespace sd

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
