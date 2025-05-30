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

#include "buttonnavigationhandler.hxx"
#include "formstrings.hxx"
#include "formmetadata.hxx"
#include "pushbuttonnavigation.hxx"

#include <com/sun/star/form/inspection/FormComponentPropertyHandler.hpp>

namespace pcr
{


    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::inspection;

    ButtonNavigationHandler::ButtonNavigationHandler( const Reference< XComponentContext >& _rxContext )
        :PropertyHandlerComponent( _rxContext )
    {

        m_xSlaveHandler = css::form::inspection::FormComponentPropertyHandler::create( m_xContext );
    }


    ButtonNavigationHandler::~ButtonNavigationHandler( )
    {
    }


    OUString ButtonNavigationHandler::getImplementationName(  )
    {
        return u"com.sun.star.comp.extensions.ButtonNavigationHandler"_ustr;
    }


    Sequence< OUString > ButtonNavigationHandler::getSupportedServiceNames(  )
    {
        return { u"com.sun.star.form.inspection.ButtonNavigationHandler"_ustr };
    }


    void SAL_CALL ButtonNavigationHandler::inspect( const Reference< XInterface >& _rxIntrospectee )
    {
        PropertyHandlerComponent::inspect( _rxIntrospectee );
        m_xSlaveHandler->inspect( _rxIntrospectee );
    }


    PropertyState  SAL_CALL ButtonNavigationHandler::getPropertyState( const OUString& _rPropertyName )
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        PropertyId nPropId( impl_getPropertyId_throwUnknownProperty( _rPropertyName ) );
        PropertyState eState = PropertyState_DIRECT_VALUE;
        switch ( nPropId )
        {
        case PROPERTY_ID_BUTTONTYPE:
        {
            PushButtonNavigation aHelper( m_xComponent );
            eState = aHelper.getCurrentButtonTypeState();
        }
        break;
        case PROPERTY_ID_TARGET_URL:
        {
            PushButtonNavigation aHelper( m_xComponent );
            eState = aHelper.getCurrentTargetURLState();
        }
        break;

        default:
            OSL_FAIL( "ButtonNavigationHandler::getPropertyState: cannot handle this property!" );
            break;
        }

        return eState;
    }


    Any SAL_CALL ButtonNavigationHandler::getPropertyValue( const OUString& _rPropertyName )
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        PropertyId nPropId( impl_getPropertyId_throwUnknownProperty( _rPropertyName ) );

        Any aReturn;
        switch ( nPropId )
        {
        case PROPERTY_ID_BUTTONTYPE:
        {
            PushButtonNavigation aHelper( m_xComponent );
            aReturn = aHelper.getCurrentButtonType();
        }
        break;

        case PROPERTY_ID_TARGET_URL:
        {
            PushButtonNavigation aHelper( m_xComponent );
            aReturn = aHelper.getCurrentTargetURL();
        }
        break;

        default:
            OSL_FAIL( "ButtonNavigationHandler::getPropertyValue: cannot handle this property!" );
            break;
        }

        return aReturn;
    }


    void SAL_CALL ButtonNavigationHandler::setPropertyValue( const OUString& _rPropertyName, const Any& _rValue )
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        PropertyId nPropId( impl_getPropertyId_throwUnknownProperty( _rPropertyName ) );
        switch ( nPropId )
        {
        case PROPERTY_ID_BUTTONTYPE:
        {
            PushButtonNavigation aHelper( m_xComponent );
            aHelper.setCurrentButtonType( _rValue );
        }
        break;

        case PROPERTY_ID_TARGET_URL:
        {
            PushButtonNavigation aHelper( m_xComponent );
            aHelper.setCurrentTargetURL( _rValue );
        }
        break;

        default:
            OSL_FAIL( "ButtonNavigationHandler::setPropertyValue: cannot handle this id!" );
        }
    }


    bool ButtonNavigationHandler::isNavigationCapableButton( const Reference< XPropertySet >& _rxComponent )
    {
        Reference< XPropertySetInfo > xPSI;
        if ( _rxComponent.is() )
            xPSI = _rxComponent->getPropertySetInfo();

        return xPSI.is()
            && xPSI->hasPropertyByName( PROPERTY_TARGET_URL )
            && xPSI->hasPropertyByName( PROPERTY_BUTTONTYPE );
    }


    Sequence< Property > ButtonNavigationHandler::doDescribeSupportedProperties() const
    {
        std::vector< Property > aProperties;

        if ( isNavigationCapableButton( m_xComponent ) )
        {
            addStringPropertyDescription( aProperties, PROPERTY_TARGET_URL );
            implAddPropertyDescription( aProperties, PROPERTY_BUTTONTYPE, ::cppu::UnoType<sal_Int32>::get() );
        }

        if ( aProperties.empty() )
            return Sequence< Property >();
        return comphelper::containerToSequence(aProperties);
    }


    Sequence< OUString > SAL_CALL ButtonNavigationHandler::getActuatingProperties( )
    {
        Sequence< OUString > aActuating{ PROPERTY_BUTTONTYPE, PROPERTY_TARGET_URL };
        return aActuating;
    }


    InteractiveSelectionResult SAL_CALL ButtonNavigationHandler::onInteractivePropertySelection( const OUString& _rPropertyName, sal_Bool _bPrimary, Any& _rData, const Reference< XObjectInspectorUI >& _rxInspectorUI )
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        PropertyId nPropId( impl_getPropertyId_throwUnknownProperty( _rPropertyName ) );

        InteractiveSelectionResult eReturn( InteractiveSelectionResult_Cancelled );

        switch ( nPropId )
        {
        case PROPERTY_ID_TARGET_URL:
            eReturn = m_xSlaveHandler->onInteractivePropertySelection( _rPropertyName, _bPrimary, _rData, _rxInspectorUI );
            break;
        default:
            eReturn = PropertyHandlerComponent::onInteractivePropertySelection( _rPropertyName, _bPrimary, _rData, _rxInspectorUI );
            break;
        }

        return eReturn;
    }


    void SAL_CALL ButtonNavigationHandler::actuatingPropertyChanged( const OUString& _rActuatingPropertyName, const Any& /*_rNewValue*/, const Any& /*_rOldValue*/, const Reference< XObjectInspectorUI >& _rxInspectorUI, sal_Bool /*_bFirstTimeInit*/ )
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        PropertyId nPropId( impl_getPropertyId_throwRuntime( _rActuatingPropertyName ) );
        switch ( nPropId )
        {
        case PROPERTY_ID_BUTTONTYPE:
        {
            PushButtonNavigation aHelper( m_xComponent );
            _rxInspectorUI->enablePropertyUI( PROPERTY_TARGET_URL, aHelper.currentButtonTypeIsOpenURL() );
        }
        break;

        case PROPERTY_ID_TARGET_URL:
        {
            PushButtonNavigation aHelper( m_xComponent );
            _rxInspectorUI->enablePropertyUI( PROPERTY_TARGET_FRAME, aHelper.hasNonEmptyCurrentTargetURL() );
        }
        break;

        default:
            OSL_FAIL( "ButtonNavigationHandler::actuatingPropertyChanged: cannot handle this id!" );
        }
    }


    LineDescriptor SAL_CALL ButtonNavigationHandler::describePropertyLine( const OUString& _rPropertyName, const Reference< XPropertyControlFactory >& _rxControlFactory )
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        PropertyId nPropId( impl_getPropertyId_throwUnknownProperty( _rPropertyName ) );

        LineDescriptor aReturn;

        switch ( nPropId )
        {
        case PROPERTY_ID_TARGET_URL:
            aReturn = m_xSlaveHandler->describePropertyLine( _rPropertyName, _rxControlFactory );
            break;
        default:
            aReturn = PropertyHandlerComponent::describePropertyLine( _rPropertyName, _rxControlFactory );
            break;
        }

        return aReturn;
    }


}   // namespace pcr

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
extensions_propctrlr_ButtonNavigationHandler_get_implementation(
    css::uno::XComponentContext* context , css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new pcr::ButtonNavigationHandler(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
