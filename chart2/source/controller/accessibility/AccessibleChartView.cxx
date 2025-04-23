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

#include <AccessibleChartView.hxx>
#include <ObjectHierarchy.hxx>
#include <ObjectIdentifier.hxx>
#include <ResId.hxx>
#include <strings.hrc>
#include "AccessibleViewForwarder.hxx"
#include <ChartModel.hxx>
#include <ChartView.hxx>
#include <ChartController.hxx>

#include <com/sun/star/accessibility/AccessibleStateType.hpp>
#include <com/sun/star/accessibility/AccessibleRole.hpp>
#include <com/sun/star/view/XSelectionSupplier.hpp>

#include <rtl/ustring.hxx>
#include <vcl/window.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <vcl/svapp.hxx>
#include <osl/mutex.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::accessibility;

using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::WeakReference;
using osl::MutexGuard;

namespace chart
{

AccessibleChartView::AccessibleChartView(SdrView* pView ) :
        impl::AccessibleChartView_Base(
            AccessibleElementInfo(), // empty for now
            true, // has children
            true  // always transparent
            ),
        m_pSdrView( pView )
{
    AddState( AccessibleStateType::OPAQUE );
}

AccessibleChartView::~AccessibleChartView()
{
}

awt::Rectangle AccessibleChartView::GetWindowPosSize() const
{
    SolarMutexGuard aSolarGuard;

    VclPtr<vcl::Window> pWindow = GetInfo().m_pWindow;
    if (!pWindow)
        return awt::Rectangle();

    awt::Rectangle aBBox;
    Point aVCLPoint( pWindow->OutputToAbsoluteScreenPixel( Point( 0, 0 ) ));
    const Size aSize = pWindow->GetSizePixel();
    aBBox.X = aVCLPoint.getX();
    aBBox.Y = aVCLPoint.getY();
    aBBox.Width = aSize.Width();
    aBBox.Height = aSize.Height();

    return aBBox;
}

awt::Point AccessibleChartView::GetUpperLeftOnScreen() const
{
    awt::Point aParentPosition;

    awt::Rectangle aBBox( GetWindowPosSize() );
    aParentPosition.X = aBBox.X;
    aParentPosition.Y = aBBox.Y;

    return aParentPosition;
}

// ________ XAccessibleContext ________
OUString SAL_CALL AccessibleChartView::getAccessibleName()
{
    return SchResId(STR_OBJECT_DIAGRAM);
}

OUString SAL_CALL AccessibleChartView::getAccessibleDescription()
{
    return getAccessibleName();
}

Reference< XAccessible > SAL_CALL AccessibleChartView::getAccessibleParent()
{
    return Reference< XAccessible >( m_xParent );
}

sal_Int64 SAL_CALL AccessibleChartView::getAccessibleIndexInParent()
{
    // the document is always the only child of the window
    return 0;
}

sal_Int16 SAL_CALL AccessibleChartView::getAccessibleRole()
{
    return AccessibleRole::DOCUMENT;
}

awt::Rectangle AccessibleChartView::implGetBounds()
{
    awt::Rectangle aResult( GetWindowPosSize());
    Reference< XAccessible > xParent( m_xParent );
    if( xParent.is())
    {
        Reference< XAccessibleComponent > xContext( xParent->getAccessibleContext(), uno::UNO_QUERY );
        if( xContext.is())
        {
            awt::Point aParentPosition = xContext->getLocationOnScreen();
            aResult.X -= aParentPosition.X;
            aResult.Y -= aParentPosition.Y;
        }
    }
    return aResult;
}


void SAL_CALL AccessibleChartView::disposing()
{
    m_pChartWindow.clear();

    AccessibleBase::disposing();
}

void AccessibleChartView::initialize( ChartController& rNewChartController,
                     const rtl::Reference<::chart::ChartModel>& xNewChartModel,
                     const rtl::Reference<::chart::ChartView>& xNewChartView,
                     const uno::Reference< XAccessible >& xNewParent,
                     ChartWindow* pNewChartWindow)
{
    //0: view::XSelectionSupplier offers notifications for selection changes and access to the selection itself
    //1: frame::XModel representing the chart model - offers access to object data
    //2: lang::XInterface representing the normal chart view - offers access to some extra object data

    //all arguments are only valid until next initialization
    bool bChanged = false;
    bool bOldInvalid = false;
    bool bNewInvalid = false;

    rtl::Reference< ::chart::ChartController > xChartController;
    rtl::Reference<::chart::ChartModel> xChartModel;
    rtl::Reference<::chart::ChartView> xChartView;
    Reference< XAccessible > xParent;
    VclPtr<ChartWindow> pChartWindow;
    {
        MutexGuard aGuard( m_aMutex);
        xChartController = m_xChartController;
        xChartModel = m_xChartModel;
        xChartView = m_xChartView;
        xParent.set( m_xParent );
        pChartWindow = m_pChartWindow;
    }

    if( !xChartController.is() || !xChartModel.is() || !xChartView.is() )
    {
        bOldInvalid = true;
    }

    if( xNewChartModel.get() != xChartModel.get() )
    {
        xChartModel = xNewChartModel;
        bChanged = true;
    }

    if( xNewChartView != xChartView )
    {
        xChartView = xNewChartView;
        bChanged = true;
    }

    if( xNewParent != xParent )
    {
        xParent = xNewParent;
        bChanged = true;
    }

    if (pNewChartWindow != pChartWindow)
    {
        pChartWindow = pNewChartWindow;
        bChanged = true;
    }

    if(xChartController != &rNewChartController)
    {
        if (xChartController)
            xChartController->removeSelectionChangeListener(this);
        rNewChartController.addSelectionChangeListener(this);
        xChartController = &rNewChartController;
        bChanged = true;
    }

    if( !xChartController.is() || !xChartModel.is() || !xChartView.is() )
    {
        if(xChartController.is())
        {
            xChartController->removeSelectionChangeListener(this);
            xChartController.clear();
        }
        xChartModel.clear();
        xChartView.clear();
        xParent.clear();
        pChartWindow.clear();

        bNewInvalid = true;
    }

    {
        MutexGuard aGuard( m_aMutex);
        m_xChartController = xChartController.get();
        m_xChartModel = xChartModel.get();
        m_xChartView = xChartView.get();
        m_xParent = xParent;
        m_pChartWindow = std::move(pChartWindow);
    }

    if( bOldInvalid && bNewInvalid )
        bChanged = false;

    if( !bChanged )
        return;

    {
        //before notification we prepare for creation of new context
        //the old context will be deleted after notification than
        MutexGuard aGuard( m_aMutex);
        if( xChartModel.is())
            m_spObjectHierarchy =
                std::make_shared<ObjectHierarchy>( xChartModel, m_xChartView.get().get() );
        else
            m_spObjectHierarchy.reset();
    }

    {
        AccessibleElementInfo aAccInfo;
        aAccInfo.m_aOID = ObjectIdentifier(u"ROOT"_ustr);
        aAccInfo.m_xChartDocument = m_xChartModel;
        aAccInfo.m_xChartController = m_xChartController;
        aAccInfo.m_xView = m_xChartView;
        aAccInfo.m_pWindow = m_pChartWindow;
        aAccInfo.m_pParent = nullptr;
        aAccInfo.m_spObjectHierarchy = m_spObjectHierarchy;
        aAccInfo.m_pSdrView = m_pSdrView;
        m_pViewForwarder.reset(new AccessibleViewForwarder(this, m_pChartWindow));
        aAccInfo.m_pViewForwarder = m_pViewForwarder.get();
        // broadcasts an INVALIDATE_ALL_CHILDREN event globally
        SetInfo( aAccInfo );
    }
}

void AccessibleChartView::initialize()
{
    //0: view::XSelectionSupplier offers notifications for selection changes and access to the selection itself
    //1: frame::XModel representing the chart model - offers access to object data
    //2: lang::XInterface representing the normal chart view - offers access to some extra object data

    //all arguments are only valid until next initialization
    bool bChanged = false;
    bool bOldInvalid = false;

    rtl::Reference< ::chart::ChartController > xChartController;
    rtl::Reference<::chart::ChartModel> xChartModel;
    rtl::Reference<::chart::ChartView> xChartView;
    {
        MutexGuard aGuard( m_aMutex);
        xChartController = m_xChartController;
        xChartModel = m_xChartModel;
        xChartView = m_xChartView;
    }

    if( !xChartController.is() || !xChartModel.is() || !xChartView.is() )
    {
        bOldInvalid = true;
    }

    if( xChartModel.is() )
    {
        bChanged = true;
        xChartModel = nullptr;
    }

    if( xChartView.is() )
    {
        bChanged = true;
        xChartView = nullptr;
    }

    if( xChartController.is() )
    {
        bChanged = true;
        xChartController->removeSelectionChangeListener(this);
        xChartController = nullptr;
    }

    {
        MutexGuard aGuard( m_aMutex);
        m_xChartController = xChartController.get();
        m_xChartModel = xChartModel.get();
        m_xChartView = xChartView.get();
        m_xParent.clear();
        m_pChartWindow.clear();
    }

    if( bOldInvalid )
        bChanged = false;

    if( !bChanged )
        return;

    {
        //before notification we prepare for creation of new context
        //the old context will be deleted after notification than
        MutexGuard aGuard( m_aMutex);
        if( xChartModel.is())
            m_spObjectHierarchy =
                std::make_shared<ObjectHierarchy>( xChartModel, m_xChartView.get().get() );
        else
            m_spObjectHierarchy.reset();
    }

    {
        AccessibleElementInfo aAccInfo;
        aAccInfo.m_aOID = ObjectIdentifier(u"ROOT"_ustr);
        aAccInfo.m_xChartDocument = m_xChartModel;
        aAccInfo.m_xChartController = m_xChartController;
        aAccInfo.m_xView = m_xChartView;
        aAccInfo.m_pWindow.clear();
        aAccInfo.m_pParent = nullptr;
        aAccInfo.m_spObjectHierarchy = m_spObjectHierarchy;
        aAccInfo.m_pSdrView = m_pSdrView;
        m_pViewForwarder.reset(new AccessibleViewForwarder(this, nullptr));
        aAccInfo.m_pViewForwarder = m_pViewForwarder.get();
        // broadcasts an INVALIDATE_ALL_CHILDREN event globally
        SetInfo( aAccInfo );
    }
}

// view::XSelectionChangeListener

void SAL_CALL AccessibleChartView::selectionChanged( const lang::EventObject& /*rEvent*/ )
{
    rtl::Reference< ::chart::ChartController > xChartController;
    {
        MutexGuard aGuard( m_aMutex);
        xChartController = m_xChartController.get();
    }

    if( !xChartController.is() )
        return;

    ObjectIdentifier aSelectedOID( xChartController->getSelection() );
    if ( m_aCurrentSelectionOID.isValid() )
    {
        NotifyEvent( EventType::LOST_SELECTION, m_aCurrentSelectionOID );
    }
    if( aSelectedOID.isValid() )
    {
        NotifyEvent( EventType::GOT_SELECTION, aSelectedOID );
    }
    m_aCurrentSelectionOID = std::move(aSelectedOID);
}

// XEventListener
void SAL_CALL AccessibleChartView::disposing( const lang::EventObject& /*Source*/ )
{
}

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
