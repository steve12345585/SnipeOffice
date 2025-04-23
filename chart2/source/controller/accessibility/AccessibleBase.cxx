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

#include <AccessibleBase.hxx>
#include "AccessibleChartShape.hxx"
#include <ObjectHierarchy.hxx>
#include <ObjectIdentifier.hxx>
#include <ChartView.hxx>
#include <ChartController.hxx>

#include <com/sun/star/accessibility/AccessibleEventId.hpp>
#include <com/sun/star/accessibility/AccessibleStateType.hpp>
#include <com/sun/star/accessibility/AccessibleRole.hpp>
#include <com/sun/star/drawing/LineStyle.hpp>
#include <com/sun/star/drawing/FillStyle.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <sal/log.hxx>
#include <utility>
#include <vcl/svapp.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <vcl/window.hxx>
#include <vcl/settings.hxx>
#include <o3tl/functional.hxx>
#include <o3tl/safeint.hxx>
#include <comphelper/diagnose_ex.hxx>

#include <algorithm>
#include <iterator>

#include "ChartElementFactory.hxx"

using namespace ::com::sun::star;
using namespace ::com::sun::star::accessibility;

using ::com::sun::star::uno::UNO_QUERY;
using ::com::sun::star::uno::Reference;
using ::osl::MutexGuard;
using ::osl::ClearableMutexGuard;
using ::com::sun::star::uno::Any;

namespace chart
{

/** @param bMayHaveChildren is false per default
 */
AccessibleBase::AccessibleBase(
    AccessibleElementInfo aAccInfo,
    bool bMayHaveChildren,
    bool bAlwaysTransparent /* default: false */ ) :
        m_bMayHaveChildren( bMayHaveChildren ),
        m_bChildrenInitialized( false ),
        m_nStateSet( 0 ),
        m_aAccInfo(std::move( aAccInfo )),
        m_bAlwaysTransparent( bAlwaysTransparent ),
        m_bStateSetInitialized( false )
{
    // initialize some states
    m_nStateSet |= AccessibleStateType::ENABLED;
    m_nStateSet |= AccessibleStateType::SHOWING;
    m_nStateSet |= AccessibleStateType::VISIBLE;
    m_nStateSet |= AccessibleStateType::SELECTABLE;
    m_nStateSet |= AccessibleStateType::FOCUSABLE;
}

AccessibleBase::~AccessibleBase()
{
    OSL_ASSERT(!isAlive());
}

bool AccessibleBase::NotifyEvent( EventType eEventType, const AccessibleUniqueId & rId )
{
    if( GetId() == rId )
    {
        // event is addressed to this object

        css::uno::Any aEmpty;
        css::uno::Any aSelected;
        aSelected <<= AccessibleStateType::SELECTED;
        switch( eEventType )
        {
            case EventType::GOT_SELECTION:
                {
                    AddState( AccessibleStateType::SELECTED );
                    NotifyAccessibleEvent(AccessibleEventId::STATE_CHANGED, aEmpty, aSelected);

                    AddState( AccessibleStateType::FOCUSED );
                    aSelected <<= AccessibleStateType::FOCUSED;
                    NotifyAccessibleEvent(AccessibleEventId::STATE_CHANGED, aEmpty, aSelected);

                    SAL_INFO("chart2.accessibility", "Selection acquired by: " << getAccessibleName());
                }
                break;

            case EventType::LOST_SELECTION:
                {
                    RemoveState( AccessibleStateType::SELECTED );
                    NotifyAccessibleEvent(AccessibleEventId::STATE_CHANGED, aSelected, aEmpty);

                    AddState( AccessibleStateType::FOCUSED );
                    aSelected <<= AccessibleStateType::FOCUSED;
                    NotifyAccessibleEvent(AccessibleEventId::STATE_CHANGED, aSelected, aEmpty);
                    SAL_INFO("chart2.accessibility", "Selection lost by: " << getAccessibleName());
                }
                break;
        }
        return true;
    }
    else if( m_bMayHaveChildren )
    {
        bool bStop = false;

        ClearableMutexGuard aGuard( m_aMutex );
        // make local copy for notification
        std::vector<rtl::Reference<AccessibleBase>> aLocalChildList(m_aChildList);
        aGuard.clear();

        for (auto const& localChild : aLocalChildList)
        {
            bStop = localChild->NotifyEvent(eEventType, rId);
            if (bStop)
                break;
        }
        return bStop;
    }

    return false;
}

void AccessibleBase::AddState( sal_Int64 aState )
{
    ensureAlive();
    m_nStateSet |= aState;
}

void AccessibleBase::RemoveState( sal_Int64 aState )
{
    ensureAlive();
    m_nStateSet &= ~aState;
}

bool AccessibleBase::UpdateChildren()
{
    bool bMustUpdateChildren = false;
    {
        MutexGuard aGuard( m_aMutex );
        if (!m_bMayHaveChildren || !isAlive())
            return false;

        bMustUpdateChildren = ( m_bMayHaveChildren &&
                                ! m_bChildrenInitialized );
    }

    // update unguarded
    if( bMustUpdateChildren )
        m_bChildrenInitialized = ImplUpdateChildren();

    return m_bChildrenInitialized;
}

bool AccessibleBase::ImplUpdateChildren()
{
    bool bResult = false;

    if( m_aAccInfo.m_spObjectHierarchy )
    {
        ObjectHierarchy::tChildContainer aModelChildren(
            m_aAccInfo.m_spObjectHierarchy->getChildren( GetId() ));
        std::vector< ChildOIDMap::key_type > aAccChildren;
        aAccChildren.reserve( aModelChildren.size());
        std::transform( m_aChildOIDMap.begin(), m_aChildOIDMap.end(),
                          std::back_inserter( aAccChildren ),
                          ::o3tl::select1st< ChildOIDMap::value_type >() );

        std::sort( aModelChildren.begin(), aModelChildren.end());

        std::vector< ObjectIdentifier > aChildrenToRemove, aChildrenToAdd;
        std::set_difference( aModelChildren.begin(), aModelChildren.end(),
                               aAccChildren.begin(), aAccChildren.end(),
                               std::back_inserter( aChildrenToAdd ));
        std::set_difference( aAccChildren.begin(), aAccChildren.end(),
                               aModelChildren.begin(), aModelChildren.end(),
                               std::back_inserter( aChildrenToRemove ));

        for (auto const& childToRemove : aChildrenToRemove)
        {
            RemoveChildByOId(childToRemove);
        }

        AccessibleElementInfo aAccInfo( GetInfo());
        aAccInfo.m_pParent = this;

        for (auto const& childToAdd : aChildrenToAdd)
        {
            aAccInfo.m_aOID = childToAdd;
            if ( childToAdd.isAutoGeneratedObject() )
            {
                AddChild( ChartElementFactory::CreateChartElement( aAccInfo ).get() );
            }
            else if ( childToAdd.isAdditionalShape() )
            {
                AddChild( new AccessibleChartShape( aAccInfo ) );
            }
        }
        bResult = true;
    }

    return bResult;
}

void AccessibleBase::AddChild( AccessibleBase * pChild  )
{
    OSL_ENSURE( pChild != nullptr, "Invalid Child" );
    if( !pChild )
        return;

    ClearableMutexGuard aGuard( m_aMutex );

    rtl::Reference<AccessibleBase> xChild(pChild);
    m_aChildList.push_back( xChild );

    m_aChildOIDMap[ pChild->GetId() ] = xChild;

    // inform listeners of new child
    if( m_bChildrenInitialized )
    {
        Any aEmpty, aNew;
        aNew <<= uno::Reference<XAccessible>(xChild);

        aGuard.clear();
        NotifyAccessibleEvent(AccessibleEventId::CHILD, aEmpty, aNew);
    }
}

void AccessibleBase::RemoveChildByOId( const ObjectIdentifier& rOId )
{
    ClearableMutexGuard aGuard( m_aMutex );

    ChildOIDMap::iterator aIt( m_aChildOIDMap.find( rOId ));
    if( aIt == m_aChildOIDMap.end())
        return;

    rtl::Reference<AccessibleBase> xChild(aIt->second);

    // remove from map
    m_aChildOIDMap.erase( aIt );

    // search child in vector
    auto aVecIter = std::find(m_aChildList.begin(), m_aChildList.end(), xChild);

    OSL_ENSURE( aVecIter != m_aChildList.end(),
                "Inconsistent ChildMap" );

    // remove child from vector
    m_aChildList.erase( aVecIter );
    bool bInitialized = m_bChildrenInitialized;

    // call listeners unguarded
    aGuard.clear();

    // inform listeners of removed child
    if( bInitialized )
    {
        Any aEmpty, aOld;
        aOld <<= uno::Reference<XAccessible>(xChild);

        NotifyAccessibleEvent(AccessibleEventId::CHILD, aOld, aEmpty);
    }

    // dispose the child
    if (xChild.is())
        xChild->dispose();
}

awt::Point AccessibleBase::GetUpperLeftOnScreen() const
{
    awt::Point aResult;
    if( m_aAccInfo.m_pParent )
    {
        ClearableMutexGuard aGuard( m_aMutex );
        AccessibleBase * pParent = m_aAccInfo.m_pParent;
        aGuard.clear();

        if( pParent )
        {
            aResult = pParent->GetUpperLeftOnScreen();
        }
        else
            OSL_FAIL( "Default position used is probably incorrect." );
    }

    return aResult;
}

void AccessibleBase::KillAllChildren()
{
    ClearableMutexGuard aGuard( m_aMutex );

    // make local copy for notification, and remove all children
    std::vector<rtl::Reference<AccessibleBase>> aLocalChildList;
    aLocalChildList.swap( m_aChildList );
    m_aChildOIDMap.clear();

    aGuard.clear();

    // call dispose for all children
    // and notify listeners
    Any aEmpty, aOld;
    for (auto const& localChild : aLocalChildList)
    {
        aOld <<= uno::Reference<XAccessible>(localChild);
        NotifyAccessibleEvent(AccessibleEventId::CHILD, aOld, aEmpty);

        if (localChild.is())
            localChild->dispose();
    }
    m_bChildrenInitialized = false;
}

void AccessibleBase::SetInfo( const AccessibleElementInfo & rNewInfo )
{
    m_aAccInfo = rNewInfo;
    if( m_bMayHaveChildren )
    {
        KillAllChildren();
    }
    NotifyAccessibleEvent(AccessibleEventId::INVALIDATE_ALL_CHILDREN, uno::Any(), uno::Any());
}

// ________ (XComponent::dispose) ________
void SAL_CALL AccessibleBase::disposing()
{
    {
        MutexGuard aGuard(m_aMutex);
        OSL_ENSURE(isAlive(), "dispose() called twice");

        OAccessibleComponentHelper::disposing();

        // reset pointers
        m_aAccInfo.m_pWindow.clear();
        m_aAccInfo.m_pParent = nullptr;

        m_nStateSet = AccessibleStateType::DEFUNC;

    }
    // call listeners unguarded

    if( m_bMayHaveChildren )
    {
        KillAllChildren();
    }
    else
        OSL_ENSURE( m_aChildList.empty(), "Child list should be empty" );
}

// ________ XAccessible ________
Reference< XAccessibleContext > SAL_CALL AccessibleBase::getAccessibleContext()
{
    return this;
}

// ________ AccessibleBase::XAccessibleContext ________
sal_Int64 SAL_CALL AccessibleBase::getAccessibleChildCount()
{
    ClearableMutexGuard aGuard( m_aMutex );
    if (!m_bMayHaveChildren || !isAlive())
        return 0;

    bool bMustUpdateChildren = ( m_bMayHaveChildren &&
                                 ! m_bChildrenInitialized );

    aGuard.clear();

    // update unguarded
    if( bMustUpdateChildren )
        UpdateChildren();

    return ImplGetAccessibleChildCount();
}

sal_Int64 AccessibleBase::ImplGetAccessibleChildCount() const
{
    return m_aChildList.size();
}

Reference< XAccessible > SAL_CALL AccessibleBase::getAccessibleChild( sal_Int64 i )
{
    ensureAlive();
    Reference< XAccessible > xResult;

    ClearableMutexGuard aGuard( m_aMutex );
    bool bMustUpdateChildren = ( m_bMayHaveChildren &&
                                 ! m_bChildrenInitialized );

    aGuard.clear();

    if( bMustUpdateChildren )
        UpdateChildren();

    xResult.set( ImplGetAccessibleChildById( i ));

    return xResult;
}

Reference< XAccessible > AccessibleBase::ImplGetAccessibleChildById( sal_Int64 i ) const
{
    rtl::Reference<AccessibleBase> xResult;

    MutexGuard aGuard( m_aMutex);
    if( ! m_bMayHaveChildren ||
        i < 0 ||
        o3tl::make_unsigned( i ) >= m_aChildList.size() )
    {
        OUString aBuf = "Index " + OUString::number( i ) + " is invalid for range [ 0, " +
                        OUString::number( m_aChildList.size() - 1 ) +
                        " ]";
        lang::IndexOutOfBoundsException aEx( aBuf,
                                             const_cast< ::cppu::OWeakObject * >(
                                                 static_cast< const ::cppu::OWeakObject * >( this )));
        throw aEx;
    }
    else
        xResult = m_aChildList[i];

    return xResult;
}

Reference< XAccessible > SAL_CALL AccessibleBase::getAccessibleParent()
{
    ensureAlive();
    Reference< XAccessible > aResult;
    if( m_aAccInfo.m_pParent )
        aResult.set( m_aAccInfo.m_pParent );

    return aResult;
}

sal_Int64 SAL_CALL AccessibleBase::getAccessibleIndexInParent()
{
    ensureAlive();

    if( m_aAccInfo.m_spObjectHierarchy )
        return m_aAccInfo.m_spObjectHierarchy->getIndexInParent( GetId() );
    return -1;
}

sal_Int16 SAL_CALL AccessibleBase::getAccessibleRole()
{
    return AccessibleRole::SHAPE;
}

Reference< XAccessibleRelationSet > SAL_CALL AccessibleBase::getAccessibleRelationSet()
{
    Reference< XAccessibleRelationSet > aResult;
    return aResult;
}

sal_Int64 SAL_CALL AccessibleBase::getAccessibleStateSet()
{
    if( ! m_bStateSetInitialized )
    {
        rtl::Reference< ::chart::ChartController > xSelSupp( GetInfo().m_xChartController );
        if ( xSelSupp.is() )
        {
            ObjectIdentifier aOID( xSelSupp->getSelection() );
            if ( aOID.isValid() && GetId() == aOID )
            {
                AddState( AccessibleStateType::SELECTED );
                AddState( AccessibleStateType::FOCUSED );
            }
        }
        m_bStateSetInitialized = true;
    }

    return m_nStateSet;
}

lang::Locale SAL_CALL AccessibleBase::getLocale()
{
    ensureAlive();

    return Application::GetSettings().GetLanguageTag().getLocale();
}

// ________ AccessibleBase::XAccessibleComponent ________

Reference< XAccessible > SAL_CALL AccessibleBase::getAccessibleAtPoint( const awt::Point& aPoint )
{
    ensureAlive();
    rtl::Reference< AccessibleBase > aResult;
    awt::Rectangle aRect( implGetBounds());

    // children are positioned relative to this object, so translate bound rect
    aRect.X = 0;
    aRect.Y = 0;

    // children must be inside the own bound rect
    if( ( aRect.X <= aPoint.X && aPoint.X <= (aRect.X + aRect.Width) ) &&
        ( aRect.Y <= aPoint.Y && aPoint.Y <= (aRect.Y + aRect.Height)))
    {
        ClearableMutexGuard aGuard( m_aMutex );
        std::vector<rtl::Reference<AccessibleBase>> aLocalChildList(m_aChildList);
        aGuard.clear();

        for (const rtl::Reference<AccessibleBase>& xLocalChild : aLocalChildList)
        {
            if (xLocalChild.is())
            {
                aRect = xLocalChild->implGetBounds();
                if( ( aRect.X <= aPoint.X && aPoint.X <= (aRect.X + aRect.Width) ) &&
                    ( aRect.Y <= aPoint.Y && aPoint.Y <= (aRect.Y + aRect.Height)))
                {
                    aResult = xLocalChild;
                    break;
                }
            }
        }
    }

    return aResult;
}

css::awt::Rectangle AccessibleBase::implGetBounds()
{
    rtl::Reference<ChartView> pChartView = m_aAccInfo.m_xView.get();
    if( pChartView )
    {
        VclPtr<vcl::Window> pWindow = m_aAccInfo.m_pWindow;
        awt::Rectangle aLogicRect( pChartView->getRectangleOfObject( m_aAccInfo.m_aOID.getObjectCID() ));
        if( pWindow )
        {
            tools::Rectangle aRect( aLogicRect.X, aLogicRect.Y,
                             aLogicRect.X + aLogicRect.Width,
                             aLogicRect.Y + aLogicRect.Height );
            SolarMutexGuard aSolarGuard;
            aRect = pWindow->LogicToPixel( aRect );

            // aLogicRect is relative to the page, but we need a value relative
            // to the parent object
            awt::Point aParentLocOnScreen;
            uno::Reference< XAccessibleComponent > xParent( getAccessibleParent(), uno::UNO_QUERY );
            if( xParent.is() )
                aParentLocOnScreen = xParent->getLocationOnScreen();

            awt::Point aULOnScreen = GetUpperLeftOnScreen();
            awt::Point aOffset( aParentLocOnScreen.X - aULOnScreen.X,
                                aParentLocOnScreen.Y - aULOnScreen.Y );

            return awt::Rectangle( aRect.Left() - aOffset.X, aRect.Top() - aOffset.Y,
                                   aRect.getOpenWidth(), aRect.getOpenHeight());
        }
    }

    return awt::Rectangle();
}

void SAL_CALL AccessibleBase::grabFocus()
{
    ensureAlive();

    rtl::Reference< ::chart::ChartController > xSelSupp( GetInfo().m_xChartController );
    if ( xSelSupp.is() )
    {
        xSelSupp->select( GetId().getAny() );
    }
}

sal_Int32 SAL_CALL AccessibleBase::getForeground()
{
    return sal_Int32(getColor( ACC_BASE_FOREGROUND ));
}

sal_Int32 SAL_CALL AccessibleBase::getBackground()
{
    return sal_Int32(getColor( ACC_BASE_BACKGROUND ));
}

Color AccessibleBase::getColor( eColorType eColType )
{
    Color nResult = COL_TRANSPARENT;
    if( m_bAlwaysTransparent )
        return nResult;

    ObjectIdentifier aOID( m_aAccInfo.m_aOID );
    ObjectType eType( aOID.getObjectType() );
    Reference< beans::XPropertySet > xObjProp;
    OUString aObjectCID = aOID.getObjectCID();
    if( eType == OBJECTTYPE_LEGEND_ENTRY )
    {
        // for colors get the data series/point properties
        std::u16string_view aParentParticle( ObjectIdentifier::getFullParentParticle( aObjectCID ));
        aObjectCID = ObjectIdentifier::createClassifiedIdentifierForParticle( aParentParticle );
    }

    xObjProp =
        ObjectIdentifier::getObjectPropertySet(
            aObjectCID, m_aAccInfo.m_xChartDocument );
    if( xObjProp.is())
    {
        try
        {
            OUString aPropName;
            OUString aStylePropName;

            switch( eType )
            {
                case OBJECTTYPE_LEGEND_ENTRY:
                case OBJECTTYPE_DATA_SERIES:
                case OBJECTTYPE_DATA_POINT:
                    if( eColType == ACC_BASE_FOREGROUND )
                    {
                        aPropName = "BorderColor";
                        aStylePropName = "BorderTransparency";
                    }
                    else
                    {
                        aPropName = "Color";
                        aStylePropName = "Transparency";
                    }
                    break;
                default:
                    if( eColType == ACC_BASE_FOREGROUND )
                    {
                        aPropName = "LineColor";
                        aStylePropName = "LineTransparence";
                    }
                    else
                    {
                        aPropName = "FillColor";
                        aStylePropName = "FillTransparence";
                    }
                    break;
            }

            bool bTransparent = m_bAlwaysTransparent;
            Reference< beans::XPropertySetInfo > xInfo = xObjProp->getPropertySetInfo();
            if( xInfo.is() &&
                xInfo->hasPropertyByName( aStylePropName ))
            {
                if( eColType == ACC_BASE_FOREGROUND )
                {
                    drawing::LineStyle aLStyle = drawing::LineStyle_SOLID;
                    if( xObjProp->getPropertyValue( aStylePropName ) >>= aLStyle )
                        bTransparent = (aLStyle == drawing::LineStyle_NONE);
                }
                else
                {
                    drawing::FillStyle aFStyle = drawing::FillStyle_SOLID;
                    if( xObjProp->getPropertyValue( aStylePropName ) >>= aFStyle )
                        bTransparent = (aFStyle == drawing::FillStyle_NONE);
                }
            }

            if( !bTransparent &&
                xInfo.is() &&
                xInfo->hasPropertyByName( aPropName ))
            {
                xObjProp->getPropertyValue( aPropName ) >>= nResult;
            }
        }
        catch( const uno::Exception & )
        {
            DBG_UNHANDLED_EXCEPTION("chart2");
        }
    }

    return nResult;
}

// ________ AccessibleBase::XServiceInfo ________
OUString SAL_CALL AccessibleBase::getImplementationName()
{
    return u"AccessibleBase"_ustr;
}

sal_Bool SAL_CALL AccessibleBase::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService( this, ServiceName );
}

uno::Sequence< OUString > SAL_CALL AccessibleBase::getSupportedServiceNames()
{
    return {
        u"com.sun.star.accessibility.Accessible"_ustr,
        u"com.sun.star.accessibility.AccessibleContext"_ustr
    };
}

} // namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
