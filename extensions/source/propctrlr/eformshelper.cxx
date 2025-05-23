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

#include <string_view>

#include "eformshelper.hxx"
#include "formstrings.hxx"
#include <strings.hrc>
#include "modulepcr.hxx"
#include "propeventtranslation.hxx"
#include "formbrowsertools.hxx"

#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/form/FormComponentType.hpp>
#include <com/sun/star/xforms/XFormsUIHelper1.hpp>
#include <com/sun/star/xsd/DataTypeClass.hpp>
#include <com/sun/star/form/binding/XListEntrySink.hpp>
#include <comphelper/diagnose_ex.hxx>

#include <algorithm>
#include <o3tl/functional.hxx>

namespace pcr
{


    using namespace ::com::sun::star;
    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::container;
    using namespace ::com::sun::star::form::binding;
    using namespace ::com::sun::star::xsd;
    using namespace ::com::sun::star::lang;
    using namespace ::com::sun::star::form;


    //= file-local helpers

    namespace
    {

        OUString composeModelElementUIName( std::u16string_view _rModelName, std::u16string_view _rElementName )
        {
            OUString a = OUString::Concat("[")
                       + _rModelName + "] "
                       + _rElementName;
            return a;
        }
    }


    //= EFormsHelper


    EFormsHelper::EFormsHelper( ::osl::Mutex& _rMutex, const Reference< XPropertySet >& _rxControlModel, const Reference< frame::XModel >& _rxContextDocument )
        :m_xControlModel( _rxControlModel )
        ,m_aPropertyListeners( _rMutex )
    {
        OSL_ENSURE( _rxControlModel.is(), "EFormsHelper::EFormsHelper: invalid control model!" );
        m_xBindableControl.set(_rxControlModel, css::uno::UNO_QUERY);

        m_xDocument.set(_rxContextDocument, css::uno::UNO_QUERY);
        OSL_ENSURE( m_xDocument.is(), "EFormsHelper::EFormsHelper: invalid document!" );

    }


    bool EFormsHelper::isEForm( const Reference< frame::XModel >& _rxContextDocument )
    {
        try
        {
            Reference< xforms::XFormsSupplier > xDocument( _rxContextDocument, UNO_QUERY );
            if ( !xDocument.is() )
                return false;

            return xDocument->getXForms().is();
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::isEForm" );
        }
        return false;
    }


    bool EFormsHelper::canBindToDataType( sal_Int32 _nDataType ) const
    {
        if ( !m_xBindableControl.is() )
            // cannot bind at all
            return false;

        // some types cannot be bound, independent from the control type
        if (  ( DataTypeClass::hexBinary == _nDataType )
           || ( DataTypeClass::base64Binary == _nDataType )
           || ( DataTypeClass::QName == _nDataType )
           || ( DataTypeClass::NOTATION == _nDataType )
           )
           return false;

        bool bCan = false;
        try
        {
            // classify the control model
            sal_Int16 nControlType = FormComponentType::CONTROL;
            OSL_VERIFY( m_xControlModel->getPropertyValue( PROPERTY_CLASSID ) >>= nControlType );

            // some lists
            sal_Int16 const nNumericCompatibleTypes[] = { DataTypeClass::DECIMAL, DataTypeClass::FLOAT, DataTypeClass::DOUBLE, 0 };
            sal_Int16 const nDateCompatibleTypes[] = { DataTypeClass::DATE, 0 };
            sal_Int16 const nTimeCompatibleTypes[] = { DataTypeClass::TIME, 0 };
            sal_Int16 const nCheckboxCompatibleTypes[] = { DataTypeClass::BOOLEAN, DataTypeClass::STRING, DataTypeClass::anyURI, 0 };
            sal_Int16 const nRadiobuttonCompatibleTypes[] = { DataTypeClass::STRING, DataTypeClass::anyURI, 0 };
            sal_Int16 const nFormattedCompatibleTypes[] = { DataTypeClass::DECIMAL, DataTypeClass::FLOAT, DataTypeClass::DOUBLE, DataTypeClass::DATETIME, DataTypeClass::DATE, DataTypeClass::TIME, 0 };

            sal_Int16 const * pCompatibleTypes = nullptr;
            switch ( nControlType )
            {
            case FormComponentType::SPINBUTTON:
            case FormComponentType::NUMERICFIELD:
                pCompatibleTypes = nNumericCompatibleTypes;
                break;
            case FormComponentType::DATEFIELD:
                pCompatibleTypes = nDateCompatibleTypes;
                break;
            case FormComponentType::TIMEFIELD:
                pCompatibleTypes = nTimeCompatibleTypes;
                break;
            case FormComponentType::CHECKBOX:
                pCompatibleTypes = nCheckboxCompatibleTypes;
                break;
            case FormComponentType::RADIOBUTTON:
                pCompatibleTypes = nRadiobuttonCompatibleTypes;
                break;

            case FormComponentType::TEXTFIELD:
            {
                // both the normal text field, and the formatted field, claim to be a TEXTFIELD
                // need to distinguish by service name
                Reference< XServiceInfo > xSI( m_xControlModel, UNO_QUERY );
                OSL_ENSURE( xSI.is(), "EFormsHelper::canBindToDataType: a control model which has no service info?" );
                if ( xSI.is() )
                {
                    if ( xSI->supportsService( SERVICE_COMPONENT_FORMATTEDFIELD ) )
                    {
                        pCompatibleTypes = nFormattedCompatibleTypes;
                        break;
                    }
                }
                [[fallthrough]];
            }
            case FormComponentType::LISTBOX:
            case FormComponentType::COMBOBOX:
                // edit fields and list/combo boxes can be bound to anything
                bCan = true;
            }

            if ( !bCan && pCompatibleTypes )
            {
                if ( _nDataType == -1 )
                {
                    // the control can be bound to at least one type, and exactly this is being asked for
                    bCan = true;
                }
                else
                {
                    while ( *pCompatibleTypes && !bCan )
                        bCan = ( *pCompatibleTypes++ == _nDataType );
                }
            }
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::canBindToDataType" );
        }

        return bCan;
    }


    bool EFormsHelper::isListEntrySink() const
    {
        bool bIs = false;
        try
        {
            Reference< XListEntrySink > xAsSink( m_xControlModel, UNO_QUERY );
            bIs = xAsSink.is();
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::isListEntrySink" );
        }
        return bIs;
    }


    void EFormsHelper::impl_switchBindingListening_throw( bool _bDoListening, const Reference< XPropertyChangeListener >& _rxListener )
    {
        Reference< XPropertySet > xBindingProps;
        if ( m_xBindableControl.is() )
            xBindingProps.set(m_xBindableControl->getValueBinding(), css::uno::UNO_QUERY);
        if ( !xBindingProps.is() )
            return;

        if ( _bDoListening )
        {
            xBindingProps->addPropertyChangeListener( OUString(), _rxListener );
        }
        else
        {
            xBindingProps->removePropertyChangeListener( OUString(), _rxListener );
        }
    }


    void EFormsHelper::registerBindingListener( const Reference< XPropertyChangeListener >& _rxBindingListener )
    {
        if ( !_rxBindingListener.is() )
            return;
        impl_toggleBindingPropertyListening_throw( true, _rxBindingListener );
    }


    void EFormsHelper::impl_toggleBindingPropertyListening_throw( bool _bDoListen, const Reference< XPropertyChangeListener >& _rxConcreteListenerOrNull )
    {
        if ( !_bDoListen )
        {
            ::comphelper::OInterfaceIteratorHelper3 aListenerIterator(m_aPropertyListeners);
            while ( aListenerIterator.hasMoreElements() )
            {
                PropertyEventTranslation* pTranslator = dynamic_cast< PropertyEventTranslation* >( aListenerIterator.next().get() );
                OSL_ENSURE( pTranslator, "EFormsHelper::impl_toggleBindingPropertyListening_throw: invalid listener element in my container!" );
                if ( !pTranslator )
                    continue;

                Reference< XPropertyChangeListener > xEventSourceTranslator( pTranslator );
                if ( _rxConcreteListenerOrNull.is() )
                {
                    if ( pTranslator->getDelegator() == _rxConcreteListenerOrNull )
                    {
                        impl_switchBindingListening_throw( false, xEventSourceTranslator );
                        m_aPropertyListeners.removeInterface( xEventSourceTranslator );
                        break;
                    }
                }
                else
                {
                    impl_switchBindingListening_throw( false, xEventSourceTranslator );
                }
            }
        }
        else
        {
            if ( _rxConcreteListenerOrNull.is() )
            {
                Reference< XPropertyChangeListener > xEventSourceTranslator( new PropertyEventTranslation( _rxConcreteListenerOrNull, m_xBindableControl ) );
                m_aPropertyListeners.addInterface( xEventSourceTranslator );
                impl_switchBindingListening_throw( true, xEventSourceTranslator );
            }
            else
            {
                ::comphelper::OInterfaceIteratorHelper3 aListenerIterator(m_aPropertyListeners);
                while ( aListenerIterator.hasMoreElements() )
                    impl_switchBindingListening_throw( true, aListenerIterator.next() );
            }
        }
    }


    void EFormsHelper::revokeBindingListener( const Reference< XPropertyChangeListener >& _rxBindingListener )
    {
        impl_toggleBindingPropertyListening_throw( false, _rxBindingListener );
    }


    void EFormsHelper::getFormModelNames( std::vector< OUString >& /* [out] */ _rModelNames ) const
    {
        if ( !m_xDocument.is() )
            return;

        try
        {
            _rModelNames.resize( 0 );

            Reference< XNameContainer > xForms( m_xDocument->getXForms() );
            OSL_ENSURE( xForms.is(), "EFormsHelper::getFormModelNames: invalid forms container!" );
            if ( xForms.is() )
            {
                const Sequence< OUString > aModelNames = xForms->getElementNames();
                _rModelNames.resize( aModelNames.getLength() );
                std::copy( aModelNames.begin(), aModelNames.end(), _rModelNames.begin() );
            }
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::getFormModelNames" );
        }
    }


    void EFormsHelper::getBindingNames( const OUString& _rModelName, std::vector< OUString >& /* [out] */ _rBindingNames ) const
    {
        _rBindingNames.resize( 0 );
        try
        {
            Reference< xforms::XModel > xModel( getFormModelByName( _rModelName ) );
            if ( xModel.is() )
            {
                Reference< XNameAccess > xBindings( xModel->getBindings(), UNO_QUERY );
                OSL_ENSURE( xBindings.is(), "EFormsHelper::getBindingNames: invalid bindings container obtained from the model!" );
                if ( xBindings.is() )
                {
                    const Sequence< OUString > aNames = xBindings->getElementNames();
                    _rBindingNames.resize( aNames.getLength() );
                    std::copy( aNames.begin(), aNames.end(), _rBindingNames.begin() );
                }
            }
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::getBindingNames" );
        }
    }


    Reference< xforms::XModel > EFormsHelper::getFormModelByName( const OUString& _rModelName ) const
    {
        Reference< xforms::XModel > xReturn;
        try
        {
            Reference< XNameContainer > xForms( m_xDocument->getXForms() );
            OSL_ENSURE( xForms.is(), "EFormsHelper::getFormModelByName: invalid forms container!" );
            if ( xForms.is() )
                OSL_VERIFY( xForms->getByName( _rModelName ) >>= xReturn );
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::getFormModelByName" );
        }
        return xReturn;
    }


    Reference< xforms::XModel > EFormsHelper::getCurrentFormModel() const
    {
        Reference< xforms::XModel > xModel;
        try
        {
            Reference< XPropertySet > xBinding( getCurrentBinding() );
            if ( xBinding.is() )
            {
                OSL_VERIFY( xBinding->getPropertyValue( PROPERTY_MODEL ) >>= xModel );
            }
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::getCurrentFormModel" );
        }
        return xModel;
    }

    OUString EFormsHelper::getCurrentFormModelName() const
    {
        OUString sModelName;
        try
        {
            Reference< xforms::XModel > xFormsModel( getCurrentFormModel() );
            if ( xFormsModel.is() )
                sModelName = xFormsModel->getID();
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::getCurrentFormModel" );
        }
        return sModelName;
    }

    Reference< XPropertySet > EFormsHelper::getCurrentBinding() const
    {
        Reference< XPropertySet > xBinding;

        try
        {
            if ( m_xBindableControl.is() )
                xBinding.set(m_xBindableControl->getValueBinding(), css::uno::UNO_QUERY);
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::getCurrentBinding" );
        }

        return xBinding;
    }

    OUString EFormsHelper::getCurrentBindingName() const
    {
        OUString sBindingName;
        try
        {
            Reference< XPropertySet > xBinding( getCurrentBinding() );
            if ( xBinding.is() )
                xBinding->getPropertyValue( PROPERTY_BINDING_ID ) >>= sBindingName;
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::getCurrentBindingName" );
        }
        return sBindingName;
    }


    Reference< XListEntrySource > EFormsHelper::getCurrentListSourceBinding() const
    {
        try
        {
            Reference< XListEntrySink > xAsSink( m_xControlModel, UNO_QUERY );
            OSL_ENSURE( xAsSink.is(), "EFormsHelper::getCurrentListSourceBinding: you should have used isListEntrySink before!" );
            if (xAsSink.is())
                return xAsSink->getListEntrySource();
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::getCurrentListSourceBinding" );
        }
        return Reference<XListEntrySource>();
    }


    void EFormsHelper::setListSourceBinding( const Reference< XListEntrySource >& _rxListSource )
    {
        try
        {
            Reference< XListEntrySink > xAsSink( m_xControlModel, UNO_QUERY );
            OSL_ENSURE( xAsSink.is(), "EFormsHelper::setListSourceBinding: you should have used isListEntrySink before!" );
            if ( xAsSink.is() )
                xAsSink->setListEntrySource( _rxListSource );
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::setListSourceBinding" );
        }
    }

    void EFormsHelper::setBinding( const Reference< css::beans::XPropertySet >& _rxBinding )
    {
        if ( !m_xBindableControl.is() )
            return;

        try
        {
            Reference< XPropertySet > xOldBinding( m_xBindableControl->getValueBinding(), UNO_QUERY );

            Reference< XValueBinding > xBinding( _rxBinding, UNO_QUERY );
            OSL_ENSURE( xBinding.is() || !_rxBinding.is(), "EFormsHelper::setBinding: invalid binding!" );

            impl_toggleBindingPropertyListening_throw( false, nullptr );
            m_xBindableControl->setValueBinding( xBinding );
            impl_toggleBindingPropertyListening_throw( true, nullptr );

            std::set< OUString > aSet;
            firePropertyChanges( xOldBinding, _rxBinding, aSet );
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::setBinding" );
        }
    }


    Reference< XPropertySet > EFormsHelper::getOrCreateBindingForModel( const OUString& _rTargetModel, const OUString& _rBindingName ) const
    {
        OSL_ENSURE( !_rBindingName.isEmpty(), "EFormsHelper::getOrCreateBindingForModel: invalid binding name!" );
        return implGetOrCreateBinding( _rTargetModel, _rBindingName );
    }


    Reference< XPropertySet > EFormsHelper::implGetOrCreateBinding( const OUString& _rTargetModel, const OUString& _rBindingName ) const
    {
        OSL_ENSURE( !( _rTargetModel.isEmpty() && !_rBindingName.isEmpty() ), "EFormsHelper::implGetOrCreateBinding: no model, but a binding name?" );

        Reference< XPropertySet > xBinding;
        try
        {
            OUString sTargetModel( _rTargetModel );
            // determine the model which the binding should belong to
            if ( sTargetModel.isEmpty() )
            {
                std::vector< OUString > aModelNames;
                getFormModelNames( aModelNames );
                if ( !aModelNames.empty() )
                    sTargetModel = *aModelNames.begin();
                OSL_ENSURE( !sTargetModel.isEmpty(), "EFormsHelper::implGetOrCreateBinding: unable to obtain a default model!" );
            }
            Reference< xforms::XModel > xModel( getFormModelByName( sTargetModel ) );
            Reference< XNameAccess > xBindingNames( xModel.is() ? xModel->getBindings() : Reference< XSet >(), UNO_QUERY );
            if ( xBindingNames.is() )
            {
                // get or create the binding instance
                if ( !_rBindingName.isEmpty() )
                {
                    if ( xBindingNames->hasByName( _rBindingName ) )
                        OSL_VERIFY( xBindingNames->getByName( _rBindingName ) >>= xBinding );
                    else
                    {
                        xBinding = xModel->createBinding( );
                        if ( xBinding.is() )
                        {
                            xBinding->setPropertyValue( PROPERTY_BINDING_ID, Any( _rBindingName ) );
                            xModel->getBindings()->insert( Any( xBinding ) );
                        }
                    }
                }
                else
                {
                    xBinding = xModel->createBinding( );
                    if ( xBinding.is() )
                    {
                        // find a nice name for it
                        OUString sBaseName(PcrRes(RID_STR_BINDING_NAME) + " ");
                        OUString sNewName;
                        sal_Int32 nNumber = 1;
                        do
                        {
                            sNewName = sBaseName + OUString::number( nNumber++ );
                        }
                        while ( xBindingNames->hasByName( sNewName ) );
                        Reference< XNamed > xName( xBinding, UNO_QUERY_THROW );
                        xName->setName( sNewName );
                        // and insert into the model
                        xModel->getBindings()->insert( Any( xBinding ) );
                    }
                }
            }
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("extensions.propctrlr");
        }

        return xBinding;
    }


    namespace
    {

        struct PropertyBagInserter
        {
        private:
            PropertyBag& m_rProperties;

        public:
            explicit PropertyBagInserter( PropertyBag& rProperties ) : m_rProperties( rProperties ) { }

            void operator()( const Property& _rProp )
            {
                m_rProperties.insert( _rProp );
            }
        };


        Reference< XPropertySetInfo > collectPropertiesGetInfo( const Reference< XPropertySet >& _rxProps, PropertyBag& _rBag )
        {
            Reference< XPropertySetInfo > xInfo;
            if ( _rxProps.is() )
                xInfo = _rxProps->getPropertySetInfo();
            if ( xInfo.is() )
            {
                const Sequence< Property > aProperties = xInfo->getProperties();
                std::for_each( aProperties.begin(), aProperties.end(),
                    PropertyBagInserter( _rBag )
                );
            }
            return xInfo;
        }
    }


    OUString EFormsHelper::getModelElementUIName( const EFormsHelper::ModelElementType _eType, const Reference< XPropertySet >& _rxElement )
    {
        OUString sUIName;
        try
        {
            // determine the model which the element belongs to
            Reference< xforms::XFormsUIHelper1 > xHelper;
            if ( _rxElement.is() )
                _rxElement->getPropertyValue( PROPERTY_MODEL ) >>= xHelper;
            OSL_ENSURE( xHelper.is(), "EFormsHelper::getModelElementUIName: invalid element or model!" );
            if ( xHelper.is() )
            {
                OUString sElementName = ( _eType == Submission ) ? xHelper->getSubmissionName( _rxElement, true ) : xHelper->getBindingName( _rxElement, true );
                Reference< xforms::XModel > xModel( xHelper, UNO_QUERY_THROW );
                sUIName = composeModelElementUIName( xModel->getID(), sElementName );
            }
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::getModelElementUIName" );
        }

        return sUIName;
    }


    Reference< XPropertySet > EFormsHelper::getModelElementFromUIName( const EFormsHelper::ModelElementType _eType, const OUString& _rUIName ) const
    {
        const MapStringToPropertySet& rMapUINameToElement( ( _eType == Submission ) ? m_aSubmissionUINames : m_aBindingUINames );
        MapStringToPropertySet::const_iterator pos = rMapUINameToElement.find( _rUIName );
        OSL_ENSURE( pos != rMapUINameToElement.end(), "EFormsHelper::getModelElementFromUIName: didn't find it!" );

        return ( pos != rMapUINameToElement.end() ) ? pos->second : Reference< XPropertySet >();
    }


    void EFormsHelper::getAllElementUINames( const ModelElementType _eType, std::vector< OUString >& /* [out] */ _rElementNames, bool _bPrepentEmptyEntry )
    {
        MapStringToPropertySet& rMapUINameToElement( ( _eType == Submission ) ? m_aSubmissionUINames : m_aBindingUINames );
        rMapUINameToElement.clear();
        _rElementNames.resize( 0 );

        if ( _bPrepentEmptyEntry )
            rMapUINameToElement[ OUString() ].clear();

        try
        {
            // obtain the model names
            std::vector< OUString > aModels;
            getFormModelNames( aModels );
            _rElementNames.reserve( aModels.size() * 2 );    // heuristics

            // for every model, obtain the element
            for (auto const& modelName : aModels)
            {
                Reference< xforms::XModel > xModel = getFormModelByName(modelName);
                OSL_ENSURE( xModel.is(), "EFormsHelper::getAllElementUINames: inconsistency in the models!" );
                Reference< xforms::XFormsUIHelper1 > xHelper( xModel, UNO_QUERY );

                Reference< XIndexAccess > xElements;
                if ( xModel.is() )
                    xElements.set(( _eType == Submission ) ? xModel->getSubmissions() : xModel->getBindings(), css::uno::UNO_QUERY);
                if ( !xElements.is() )
                    break;

                sal_Int32 nElementCount = xElements->getCount();
                for ( sal_Int32 i = 0; i < nElementCount; ++i )
                {
                    Reference< XPropertySet > xElement( xElements->getByIndex( i ), UNO_QUERY );
                    OSL_ENSURE( xElement.is(), "EFormsHelper::getAllElementUINames: empty element!" );
                    if ( !xElement.is() )
                        continue;
#if OSL_DEBUG_LEVEL > 0
                    {
                        Reference< xforms::XModel > xElementsModel;
                        xElement->getPropertyValue( PROPERTY_MODEL ) >>= xElementsModel;
                        OSL_ENSURE( xElementsModel == xModel, "EFormsHelper::getAllElementUINames: inconsistency in the model-element relationship!" );
                        if ( xElementsModel != xModel )
                            xElement->setPropertyValue( PROPERTY_MODEL, Any( xModel ) );
                    }
#endif
                    OUString sElementName = ( _eType == Submission ) ? xHelper->getSubmissionName( xElement, true ) : xHelper->getBindingName( xElement, true );
                    OUString sUIName = composeModelElementUIName( modelName, sElementName );

                    OSL_ENSURE( rMapUINameToElement.find( sUIName ) == rMapUINameToElement.end(), "EFormsHelper::getAllElementUINames: duplicate name!" );
                    rMapUINameToElement.emplace( sUIName, xElement );
                }
            }
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::getAllElementUINames" );
        }

        _rElementNames.resize( rMapUINameToElement.size() );
        std::transform( rMapUINameToElement.begin(), rMapUINameToElement.end(), _rElementNames.begin(),
                ::o3tl::select1st< MapStringToPropertySet::value_type >() );
    }


    void EFormsHelper::firePropertyChange( const OUString& _rName, const Any& _rOldValue, const Any& _rNewValue ) const
    {
        if ( m_aPropertyListeners.getLength() == 0 )
            return;

        if ( _rOldValue == _rNewValue )
            return;

        try
        {
            PropertyChangeEvent aEvent;

            aEvent.Source = m_xBindableControl.get();
            aEvent.PropertyName = _rName;
            aEvent.OldValue = _rOldValue;
            aEvent.NewValue = _rNewValue;

            const_cast< EFormsHelper* >( this )->m_aPropertyListeners.notifyEach( &XPropertyChangeListener::propertyChange, aEvent );
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::firePropertyChange" );
        }
    }


    void EFormsHelper::firePropertyChanges( const Reference< XPropertySet >& _rxOldProps, const Reference< XPropertySet >& _rxNewProps, std::set< OUString >& _rFilter ) const
    {
        if ( m_aPropertyListeners.getLength() == 0 )
            return;

        try
        {
            PropertyBag aProperties;
            Reference< XPropertySetInfo > xOldInfo = collectPropertiesGetInfo( _rxOldProps, aProperties );
            Reference< XPropertySetInfo > xNewInfo = collectPropertiesGetInfo( _rxNewProps, aProperties );

            for (auto const& property : aProperties)
            {
                if ( _rFilter.find( property.Name ) != _rFilter.end() )
                    continue;

                Any aOldValue( nullptr, property.Type );
                if ( xOldInfo.is() && xOldInfo->hasPropertyByName( property.Name ) )
                    aOldValue = _rxOldProps->getPropertyValue( property.Name );

                Any aNewValue( nullptr, property.Type );
                if ( xNewInfo.is() && xNewInfo->hasPropertyByName( property.Name ) )
                    aNewValue = _rxNewProps->getPropertyValue( property.Name );

                firePropertyChange( property.Name, aOldValue, aNewValue );
            }
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.propctrlr", "EFormsHelper::firePropertyChanges" );
        }
    }


} // namespace pcr


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
