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

#include <sal/config.h>

#include "xsdvalidationpropertyhandler.hxx"
#include "formstrings.hxx"
#include "formmetadata.hxx"
#include "xsddatatypes.hxx"
#include "modulepcr.hxx"
#include <strings.hrc>
#include <propctrlr.h>
#include "newdatatype.hxx"
#include "xsdvalidationhelper.hxx"
#include "pcrcommon.hxx"
#include "handlerhelper.hxx"

#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/lang/NullPointerException.hpp>
#include <com/sun/star/xsd/WhiteSpaceTreatment.hpp>
#include <com/sun/star/xsd/DataTypeClass.hpp>
#include <com/sun/star/inspection/PropertyControlType.hpp>
#include <com/sun/star/beans/Optional.hpp>
#include <com/sun/star/inspection/XObjectInspectorUI.hpp>
#include <com/sun/star/inspection/PropertyLineElement.hpp>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <tools/debug.hxx>
#include <sal/macros.h>

#include <algorithm>
#include <limits>


namespace pcr
{


    using namespace ::com::sun::star;
    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::lang;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::xsd;
    using namespace ::com::sun::star::inspection;

    using ::com::sun::star::beans::PropertyAttribute::MAYBEVOID;


    //= XSDValidationPropertyHandler

    XSDValidationPropertyHandler::XSDValidationPropertyHandler( const Reference< XComponentContext >& _rxContext )
        :PropertyHandlerComponent( _rxContext )
    {
    }


    XSDValidationPropertyHandler::~XSDValidationPropertyHandler()
    {
    }


    OUString XSDValidationPropertyHandler::getImplementationName(  )
    {
        return u"com.sun.star.comp.extensions.XSDValidationPropertyHandler"_ustr;
    }


    Sequence< OUString > XSDValidationPropertyHandler::getSupportedServiceNames(  )
    {
        return{ u"com.sun.star.form.inspection.XSDValidationPropertyHandler"_ustr };
    }


    Any SAL_CALL XSDValidationPropertyHandler::getPropertyValue( const OUString& _rPropertyName )
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        PropertyId nPropId( impl_getPropertyId_throwUnknownProperty( _rPropertyName ) );

        OSL_ENSURE(m_pHelper, "XSDValidationPropertyHandler::getPropertyValue: inconsistency!");
        // if we survived impl_getPropertyId_throwUnknownProperty, we should have a helper, since no helper implies no properties

        Any aReturn;
        ::rtl::Reference< XSDDataType > pType = m_pHelper->getValidatingDataType();
        switch ( nPropId )
        {
        // common facets
        case PROPERTY_ID_XSD_DATA_TYPE:  aReturn = pType.is() ? pType->getFacet( PROPERTY_NAME ) : Any( OUString() ); break;
        case PROPERTY_ID_XSD_WHITESPACES:aReturn = pType.is() ? pType->getFacet( PROPERTY_XSD_WHITESPACES ) : Any( WhiteSpaceTreatment::Preserve ); break;
        case PROPERTY_ID_XSD_PATTERN:    aReturn = pType.is() ? pType->getFacet( PROPERTY_XSD_PATTERN ) : Any( OUString() ); break;

        // all other properties are simply forwarded, if they exist at the given type
        default:
        {
            if ( pType.is() && pType->hasFacet( _rPropertyName ) )
                aReturn = pType->getFacet( _rPropertyName );
        }
        break;
        }

        return aReturn;
    }


    void SAL_CALL XSDValidationPropertyHandler::setPropertyValue( const OUString& _rPropertyName, const Any& _rValue )
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        PropertyId nPropId( impl_getPropertyId_throwUnknownProperty( _rPropertyName ) );

        OSL_ENSURE(m_pHelper, "XSDValidationPropertyHandler::getPropertyValue: inconsistency!");
        // if we survived impl_getPropertyId_throwUnknownProperty, we should have a helper, since no helper implies no properties

        if ( PROPERTY_ID_XSD_DATA_TYPE == nPropId )
        {
            OUString sTypeName;
            OSL_VERIFY( _rValue >>= sTypeName );
            m_pHelper->setValidatingDataTypeByName( sTypeName );
            impl_setContextDocumentModified_nothrow();
            return;
        }

        ::rtl::Reference< XSDDataType > pType = m_pHelper->getValidatingDataType();
        if ( !pType.is() )
        {
            OSL_FAIL( "XSDValidationPropertyHandler::setPropertyValue: you're trying to set a type facet, without a current type!" );
            return;
        }

        pType->setFacet( _rPropertyName, _rValue );
        impl_setContextDocumentModified_nothrow();
    }


    void XSDValidationPropertyHandler::onNewComponent()
    {
        PropertyHandlerComponent::onNewComponent();

        Reference< frame::XModel > xDocument( impl_getContextDocument_nothrow() );
        DBG_ASSERT( xDocument.is(), "XSDValidationPropertyHandler::onNewComponent: no document!" );
        if ( EFormsHelper::isEForm( xDocument ) )
            m_pHelper.reset( new XSDValidationHelper( m_aMutex, m_xComponent, xDocument ) );
        else
            m_pHelper.reset();
    }


    Sequence< Property > XSDValidationPropertyHandler::doDescribeSupportedProperties() const
    {
        std::vector< Property > aProperties;

        if (m_pHelper)
        {
            bool bAllowBinding = m_pHelper->canBindToAnyDataType();

            if ( bAllowBinding )
            {
                aProperties.reserve( 28 );

                addStringPropertyDescription( aProperties, PROPERTY_XSD_DATA_TYPE   );
                addInt16PropertyDescription ( aProperties, PROPERTY_XSD_WHITESPACES );
                addStringPropertyDescription( aProperties, PROPERTY_XSD_PATTERN     );

                // string facets
                addInt32PropertyDescription( aProperties, PROPERTY_XSD_LENGTH,          MAYBEVOID );
                addInt32PropertyDescription( aProperties, PROPERTY_XSD_MIN_LENGTH,      MAYBEVOID );
                addInt32PropertyDescription( aProperties, PROPERTY_XSD_MAX_LENGTH,      MAYBEVOID );

                // decimal facets
                addInt32PropertyDescription( aProperties, PROPERTY_XSD_TOTAL_DIGITS,    MAYBEVOID );
                addInt32PropertyDescription( aProperties, PROPERTY_XSD_FRACTION_DIGITS, MAYBEVOID );

                // facets for different types
                addInt16PropertyDescription( aProperties, PROPERTY_XSD_MAX_INCLUSIVE_INT, MAYBEVOID );
                addInt16PropertyDescription( aProperties, PROPERTY_XSD_MAX_EXCLUSIVE_INT, MAYBEVOID );
                addInt16PropertyDescription( aProperties, PROPERTY_XSD_MIN_INCLUSIVE_INT, MAYBEVOID );
                addInt16PropertyDescription( aProperties, PROPERTY_XSD_MIN_EXCLUSIVE_INT, MAYBEVOID );
                addDoublePropertyDescription( aProperties, PROPERTY_XSD_MAX_INCLUSIVE_DOUBLE, MAYBEVOID );
                addDoublePropertyDescription( aProperties, PROPERTY_XSD_MAX_EXCLUSIVE_DOUBLE, MAYBEVOID );
                addDoublePropertyDescription( aProperties, PROPERTY_XSD_MIN_INCLUSIVE_DOUBLE, MAYBEVOID );
                addDoublePropertyDescription( aProperties, PROPERTY_XSD_MIN_EXCLUSIVE_DOUBLE, MAYBEVOID );
                addDatePropertyDescription( aProperties, PROPERTY_XSD_MAX_INCLUSIVE_DATE, MAYBEVOID );
                addDatePropertyDescription( aProperties, PROPERTY_XSD_MAX_EXCLUSIVE_DATE, MAYBEVOID );
                addDatePropertyDescription( aProperties, PROPERTY_XSD_MIN_INCLUSIVE_DATE, MAYBEVOID );
                addDatePropertyDescription( aProperties, PROPERTY_XSD_MIN_EXCLUSIVE_DATE, MAYBEVOID );
                addTimePropertyDescription( aProperties, PROPERTY_XSD_MAX_INCLUSIVE_TIME, MAYBEVOID );
                addTimePropertyDescription( aProperties, PROPERTY_XSD_MAX_EXCLUSIVE_TIME, MAYBEVOID );
                addTimePropertyDescription( aProperties, PROPERTY_XSD_MIN_INCLUSIVE_TIME, MAYBEVOID );
                addTimePropertyDescription( aProperties, PROPERTY_XSD_MIN_EXCLUSIVE_TIME, MAYBEVOID );
                addDateTimePropertyDescription( aProperties, PROPERTY_XSD_MAX_INCLUSIVE_DATE_TIME, MAYBEVOID );
                addDateTimePropertyDescription( aProperties, PROPERTY_XSD_MAX_EXCLUSIVE_DATE_TIME, MAYBEVOID );
                addDateTimePropertyDescription( aProperties, PROPERTY_XSD_MIN_INCLUSIVE_DATE_TIME, MAYBEVOID );
                addDateTimePropertyDescription( aProperties, PROPERTY_XSD_MIN_EXCLUSIVE_DATE_TIME, MAYBEVOID );
            }
        }

        return comphelper::containerToSequence( aProperties );
    }


    Sequence< OUString > SAL_CALL XSDValidationPropertyHandler::getSupersededProperties( )
    {
        ::osl::MutexGuard aGuard( m_aMutex );

        std::vector< OUString > aSuperfluous;
        if (m_pHelper)
        {
            aSuperfluous.push_back(  PROPERTY_CONTROLSOURCE );
            aSuperfluous.push_back(  PROPERTY_EMPTY_IS_NULL );
            aSuperfluous.push_back(  PROPERTY_FILTERPROPOSAL );
            aSuperfluous.push_back(  PROPERTY_LISTSOURCETYPE );
            aSuperfluous.push_back(  PROPERTY_LISTSOURCE );
            aSuperfluous.push_back(  PROPERTY_BOUNDCOLUMN );

            bool bAllowBinding = m_pHelper->canBindToAnyDataType();

            if ( bAllowBinding )
            {
                aSuperfluous.push_back(  PROPERTY_MAXTEXTLEN );
                aSuperfluous.push_back(  PROPERTY_VALUEMIN );
                aSuperfluous.push_back(  PROPERTY_VALUEMAX );
                aSuperfluous.push_back(  PROPERTY_DECIMAL_ACCURACY );
                aSuperfluous.push_back(  PROPERTY_TIMEMIN );
                aSuperfluous.push_back(  PROPERTY_TIMEMAX );
                aSuperfluous.push_back(  PROPERTY_DATEMIN );
                aSuperfluous.push_back(  PROPERTY_DATEMAX );
                aSuperfluous.push_back(  PROPERTY_EFFECTIVE_MIN );
                aSuperfluous.push_back(  PROPERTY_EFFECTIVE_MAX );
            }
        }

        return comphelper::containerToSequence( aSuperfluous );
    }


    Sequence< OUString > SAL_CALL XSDValidationPropertyHandler::getActuatingProperties( )
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        std::vector< OUString > aInterestedInActuations;
        if (m_pHelper)
        {
            aInterestedInActuations.push_back(  PROPERTY_XSD_DATA_TYPE );
            aInterestedInActuations.push_back(  PROPERTY_XML_DATA_MODEL );
        }
        return comphelper::containerToSequence( aInterestedInActuations );
    }


    namespace
    {
        void showPropertyUI( const Reference< XObjectInspectorUI >& _rxInspectorUI, const OUString& _rPropertyName, bool _bShow )
        {
            if ( _bShow )
                _rxInspectorUI->showPropertyUI( _rPropertyName );
            else
                _rxInspectorUI->hidePropertyUI( _rPropertyName );
        }
    }


    LineDescriptor SAL_CALL XSDValidationPropertyHandler::describePropertyLine( const OUString& _rPropertyName,
        const Reference< XPropertyControlFactory >& _rxControlFactory )
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        if ( !_rxControlFactory.is() )
            throw NullPointerException();
        if (!m_pHelper)
            throw RuntimeException();

        PropertyId nPropId( impl_getPropertyId_throwUnknownProperty( _rPropertyName ) );

        LineDescriptor aDescriptor;
        if ( nPropId != PROPERTY_ID_XSD_DATA_TYPE )
            aDescriptor.IndentLevel = 1;

        // collect some information about the to-be-created control
        sal_Int16 nControlType = PropertyControlType::TextField;
        std::vector< OUString > aListEntries;
        Optional< double > aMinValue( false, 0 );
        Optional< double > aMaxValue( false, 0 );

        switch ( nPropId )
        {
        case PROPERTY_ID_XSD_DATA_TYPE:
            nControlType = PropertyControlType::ListBox;

            implGetAvailableDataTypeNames( aListEntries );

            aDescriptor.PrimaryButtonId = UID_PROP_ADD_DATA_TYPE;
            aDescriptor.SecondaryButtonId = UID_PROP_REMOVE_DATA_TYPE;
            aDescriptor.HasPrimaryButton = aDescriptor.HasSecondaryButton = true;
            aDescriptor.PrimaryButtonImageURL = "private:graphicrepository/extensions/res/buttonplus.png";
            aDescriptor.SecondaryButtonImageURL = "private:graphicrepository/extensions/res/buttonminus.png";
            break;

        case PROPERTY_ID_XSD_WHITESPACES:
        {
            nControlType = PropertyControlType::ListBox;
            aListEntries = m_pInfoService->getPropertyEnumRepresentations( PROPERTY_ID_XSD_WHITESPACES );
        }
        break;

        case PROPERTY_ID_XSD_PATTERN:
            nControlType = PropertyControlType::TextField;
            break;

        case PROPERTY_ID_XSD_LENGTH:
        case PROPERTY_ID_XSD_MIN_LENGTH:
        case PROPERTY_ID_XSD_MAX_LENGTH:
            nControlType = PropertyControlType::NumericField;
            break;

        case PROPERTY_ID_XSD_TOTAL_DIGITS:
        case PROPERTY_ID_XSD_FRACTION_DIGITS:
            nControlType = PropertyControlType::NumericField;
            break;

        case PROPERTY_ID_XSD_MAX_INCLUSIVE_INT:
        case PROPERTY_ID_XSD_MAX_EXCLUSIVE_INT:
        case PROPERTY_ID_XSD_MIN_INCLUSIVE_INT:
        case PROPERTY_ID_XSD_MIN_EXCLUSIVE_INT:
        {
            nControlType = PropertyControlType::NumericField;

            // handle limits for various 'INT' types according to
            // their actual semantics (year, month, day)

            ::rtl::Reference< XSDDataType > xDataType( m_pHelper->getValidatingDataType() );
            sal_Int16 nTypeClass = xDataType.is() ? xDataType->classify() : DataTypeClass::STRING;

            aMinValue.IsPresent = aMaxValue.IsPresent = true;
            aMinValue.Value = DataTypeClass::gYear == nTypeClass ? 0 : 1;
            aMaxValue.Value = std::numeric_limits< sal_Int32 >::max();
            if ( DataTypeClass::gMonth == nTypeClass )
                aMaxValue.Value = 12;
            else if ( DataTypeClass::gDay == nTypeClass )
                aMaxValue.Value = 31;
        }
        break;

        case PROPERTY_ID_XSD_MAX_INCLUSIVE_DOUBLE:
        case PROPERTY_ID_XSD_MAX_EXCLUSIVE_DOUBLE:
        case PROPERTY_ID_XSD_MIN_INCLUSIVE_DOUBLE:
        case PROPERTY_ID_XSD_MIN_EXCLUSIVE_DOUBLE:
            nControlType = PropertyControlType::NumericField;
            // TODO/eForms: do we have "auto-digits"?
            break;

        case PROPERTY_ID_XSD_MAX_INCLUSIVE_DATE:
        case PROPERTY_ID_XSD_MAX_EXCLUSIVE_DATE:
        case PROPERTY_ID_XSD_MIN_INCLUSIVE_DATE:
        case PROPERTY_ID_XSD_MIN_EXCLUSIVE_DATE:
            nControlType = PropertyControlType::DateField;
            break;

        case PROPERTY_ID_XSD_MAX_INCLUSIVE_TIME:
        case PROPERTY_ID_XSD_MAX_EXCLUSIVE_TIME:
        case PROPERTY_ID_XSD_MIN_INCLUSIVE_TIME:
        case PROPERTY_ID_XSD_MIN_EXCLUSIVE_TIME:
            nControlType = PropertyControlType::TimeField;
            break;

        case PROPERTY_ID_XSD_MAX_INCLUSIVE_DATE_TIME:
        case PROPERTY_ID_XSD_MAX_EXCLUSIVE_DATE_TIME:
        case PROPERTY_ID_XSD_MIN_INCLUSIVE_DATE_TIME:
        case PROPERTY_ID_XSD_MIN_EXCLUSIVE_DATE_TIME:
            nControlType = PropertyControlType::DateTimeField;
            break;

        default:
            OSL_FAIL( "XSDValidationPropertyHandler::describePropertyLine: cannot handle this property!" );
            break;
        }

        switch ( nControlType )
        {
        case PropertyControlType::ListBox:
            aDescriptor.Control = PropertyHandlerHelper::createListBoxControl( _rxControlFactory, std::move(aListEntries), false, false );
            break;
        case PropertyControlType::NumericField:
            aDescriptor.Control = PropertyHandlerHelper::createNumericControl( _rxControlFactory, 0, aMinValue, aMaxValue );
            break;
        default:
            aDescriptor.Control = _rxControlFactory->createPropertyControl( nControlType, false );
            break;
        }

        aDescriptor.Category = "Data";
        aDescriptor.DisplayName = m_pInfoService->getPropertyTranslation( nPropId );
        aDescriptor.HelpURL = HelpIdUrl::getHelpURL( m_pInfoService->getPropertyHelpId( nPropId ) );

        return aDescriptor;
    }


    InteractiveSelectionResult SAL_CALL XSDValidationPropertyHandler::onInteractivePropertySelection( const OUString& _rPropertyName, sal_Bool _bPrimary, Any& /*_rData*/, const Reference< XObjectInspectorUI >& _rxInspectorUI )
    {
        if ( !_rxInspectorUI.is() )
            throw NullPointerException();

        ::osl::MutexGuard aGuard( m_aMutex );
        OSL_ENSURE(m_pHelper, "XSDValidationPropertyHandler::onInteractivePropertySelection: we "
                              "don't have any SupportedProperties!");
        if (!m_pHelper)
            return InteractiveSelectionResult_Cancelled;

        PropertyId nPropId( impl_getPropertyId_throwUnknownProperty( _rPropertyName ) );

        switch ( nPropId )
        {
        case PROPERTY_ID_XSD_DATA_TYPE:
        {
            if ( _bPrimary )
            {
                OUString sNewDataTypeName;
                if ( implPrepareCloneDataCurrentType( sNewDataTypeName ) )
                {
                    implDoCloneCurrentDataType( sNewDataTypeName );
                    return InteractiveSelectionResult_Success;
                }
            }
            else
                return implPrepareRemoveCurrentDataType() && implDoRemoveCurrentDataType() ? InteractiveSelectionResult_Success : InteractiveSelectionResult_Cancelled;
        }
        break;

        default:
            OSL_FAIL( "XSDValidationPropertyHandler::onInteractivePropertySelection: unexpected property to build a dedicated UI!" );
            break;
        }
        return InteractiveSelectionResult_Cancelled;
    }


    void SAL_CALL XSDValidationPropertyHandler::addPropertyChangeListener( const Reference< XPropertyChangeListener >& _rxListener )
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        PropertyHandlerComponent::addPropertyChangeListener( _rxListener );
        if (m_pHelper)
            m_pHelper->registerBindingListener( _rxListener );
    }


    void SAL_CALL XSDValidationPropertyHandler::removePropertyChangeListener( const Reference< XPropertyChangeListener >& _rxListener )
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        if (m_pHelper)
            m_pHelper->revokeBindingListener( _rxListener );
        PropertyHandlerComponent::removePropertyChangeListener( _rxListener );
    }


    bool XSDValidationPropertyHandler::implPrepareCloneDataCurrentType( OUString& _rNewName )
    {
        OSL_PRECOND(
            m_pHelper,
            "XSDValidationPropertyHandler::implPrepareCloneDataCurrentType: this will crash!");

        ::rtl::Reference< XSDDataType > pType = m_pHelper->getValidatingDataType();
        if ( !pType.is() )
        {
            OSL_FAIL( "XSDValidationPropertyHandler::implPrepareCloneDataCurrentType: invalid current data type!" );
            return false;
        }

        std::vector< OUString > aExistentNames;
        m_pHelper->getAvailableDataTypeNames( aExistentNames );

        NewDataTypeDialog aDialog( nullptr, pType->getName(), aExistentNames );  // TODO/eForms: proper parent
        if (aDialog.run() != RET_OK)
            return false;

        _rNewName = aDialog.GetName();
        return true;
    }


    void XSDValidationPropertyHandler::implDoCloneCurrentDataType( const OUString& _rNewName )
    {
        OSL_PRECOND(m_pHelper,
                    "XSDValidationPropertyHandler::implDoCloneCurrentDataType: this will crash!");

        ::rtl::Reference< XSDDataType > pType = m_pHelper->getValidatingDataType();
        if ( !pType.is() )
            return;

        if ( !m_pHelper->cloneDataType( pType, _rNewName ) )
            return;

        m_pHelper->setValidatingDataTypeByName( _rNewName );
    }

    bool XSDValidationPropertyHandler::implPrepareRemoveCurrentDataType()
    {
        OSL_PRECOND(
            m_pHelper,
            "XSDValidationPropertyHandler::implPrepareRemoveCurrentDataType: this will crash!");

        ::rtl::Reference< XSDDataType > pType = m_pHelper->getValidatingDataType();
        if ( !pType.is() )
        {
            OSL_FAIL( "XSDValidationPropertyHandler::implPrepareRemoveCurrentDataType: invalid current data type!" );
            return false;
        }

        // confirmation message
        OUString sConfirmation( PcrRes( RID_STR_CONFIRM_DELETE_DATA_TYPE ) );
        sConfirmation = sConfirmation.replaceFirst( "#type#", pType->getName() );

        std::unique_ptr<weld::MessageDialog> xQueryBox(Application::CreateMessageDialog(nullptr, // TODO/eForms: proper parent
                                                       VclMessageType::Question, VclButtonsType::YesNo,
                                                       sConfirmation));
        return xQueryBox->run() == RET_YES;
    }

    bool XSDValidationPropertyHandler::implDoRemoveCurrentDataType()
    {
        OSL_PRECOND(m_pHelper,
                    "XSDValidationPropertyHandler::implDoRemoveCurrentDataType: this will crash!");

        ::rtl::Reference< XSDDataType > pType = m_pHelper->getValidatingDataType();
        if ( !pType.is() )
            return false;

        // set a new data type at the binding, which is the "basic" type for the one
        // we are going to delete
        // (do this before the actual deletion, so the old type is still valid for property change
        // notifications)
        m_pHelper->setValidatingDataTypeByName( m_pHelper->getBasicTypeNameForClass( pType->classify() ) );
        // now remove the type
        m_pHelper->removeDataTypeFromRepository( pType->getName() );

        return true;
    }


    void SAL_CALL XSDValidationPropertyHandler::actuatingPropertyChanged( const OUString& _rActuatingPropertyName, const Any& _rNewValue, const Any& _rOldValue, const Reference< XObjectInspectorUI >& _rxInspectorUI, sal_Bool _bFirstTimeInit )
    {
        if ( !_rxInspectorUI.is() )
            throw NullPointerException();

        ::osl::MutexGuard aGuard( m_aMutex );
        PropertyId nActuatingPropId( impl_getPropertyId_throwRuntime( _rActuatingPropertyName ) );
        if (!m_pHelper)
            throw RuntimeException();
            // if we survived impl_getPropertyId_throwRuntime, we should have a helper, since no helper implies no properties

        switch ( nActuatingPropId )
        {
        case PROPERTY_ID_XSD_DATA_TYPE:
        {
            ::rtl::Reference< XSDDataType > xDataType( m_pHelper->getValidatingDataType() );

            // is removal of this type possible?
            bool bIsBasicType = xDataType.is() && xDataType->isBasicType();
            _rxInspectorUI->enablePropertyUIElements( PROPERTY_XSD_DATA_TYPE, PropertyLineElement::PrimaryButton, xDataType.is() );
            _rxInspectorUI->enablePropertyUIElements( PROPERTY_XSD_DATA_TYPE, PropertyLineElement::SecondaryButton, xDataType.is() && !bIsBasicType );


            // show the facets which are available at the data type
            OUString aFacets[] = {
                PROPERTY_XSD_WHITESPACES, PROPERTY_XSD_PATTERN,
                PROPERTY_XSD_LENGTH, PROPERTY_XSD_MIN_LENGTH, PROPERTY_XSD_MAX_LENGTH, PROPERTY_XSD_TOTAL_DIGITS,
                PROPERTY_XSD_FRACTION_DIGITS,
                PROPERTY_XSD_MAX_INCLUSIVE_INT,
                PROPERTY_XSD_MAX_EXCLUSIVE_INT,
                PROPERTY_XSD_MIN_INCLUSIVE_INT,
                PROPERTY_XSD_MIN_EXCLUSIVE_INT,
                PROPERTY_XSD_MAX_INCLUSIVE_DOUBLE,
                PROPERTY_XSD_MAX_EXCLUSIVE_DOUBLE,
                PROPERTY_XSD_MIN_INCLUSIVE_DOUBLE,
                PROPERTY_XSD_MIN_EXCLUSIVE_DOUBLE,
                PROPERTY_XSD_MAX_INCLUSIVE_DATE,
                PROPERTY_XSD_MAX_EXCLUSIVE_DATE,
                PROPERTY_XSD_MIN_INCLUSIVE_DATE,
                PROPERTY_XSD_MIN_EXCLUSIVE_DATE,
                PROPERTY_XSD_MAX_INCLUSIVE_TIME,
                PROPERTY_XSD_MAX_EXCLUSIVE_TIME,
                PROPERTY_XSD_MIN_INCLUSIVE_TIME,
                PROPERTY_XSD_MIN_EXCLUSIVE_TIME,
                PROPERTY_XSD_MAX_INCLUSIVE_DATE_TIME,
                PROPERTY_XSD_MAX_EXCLUSIVE_DATE_TIME,
                PROPERTY_XSD_MIN_INCLUSIVE_DATE_TIME,
                PROPERTY_XSD_MIN_EXCLUSIVE_DATE_TIME
            };

            for ( const auto& rFacet : aFacets)
            {
                showPropertyUI( _rxInspectorUI, rFacet, xDataType.is() && xDataType->hasFacet( rFacet ) );
                _rxInspectorUI->enablePropertyUI( rFacet, !bIsBasicType );
            }
        }
        break;

        case PROPERTY_ID_XML_DATA_MODEL:
        {
            // The data type which the current binding works with may not be present in the
            // new model. Thus, transfer it.
            OUString sOldModelName; _rOldValue >>= sOldModelName;
            OUString sNewModelName; _rNewValue >>= sNewModelName;
            OUString sDataType = m_pHelper->getValidatingDataTypeName();
            m_pHelper->copyDataType( sOldModelName, sNewModelName, sDataType );

            // the list of available data types depends on the chosen model, so update this
            if ( !_bFirstTimeInit )
                _rxInspectorUI->rebuildPropertyUI( PROPERTY_XSD_DATA_TYPE );
        }
        break;

        default:
            OSL_FAIL( "XSDValidationPropertyHandler::actuatingPropertyChanged: cannot handle this property!" );
            return;
        }

        // in both cases, we need to care for the current value of the XSD_DATA_TYPE property,
        // and update the FormatKey of the formatted field we're inspecting (if any)
        if ( !_bFirstTimeInit && m_pHelper->isInspectingFormattedField() )
            m_pHelper->findDefaultFormatForIntrospectee();
    }


    void XSDValidationPropertyHandler::implGetAvailableDataTypeNames( std::vector< OUString >& /* [out] */ _rNames ) const
    {
        OSL_PRECOND(
            m_pHelper,
            "XSDValidationPropertyHandler::implGetAvailableDataTypeNames: this will crash!");
        // start with *all* types which are available at the model
        std::vector< OUString > aAllTypes;
        m_pHelper->getAvailableDataTypeNames( aAllTypes );
        _rNames.clear();
        _rNames.reserve( aAllTypes.size() );

        // then allow only those which are "compatible" with our control
        for (auto const& dataType : aAllTypes)
        {
            ::rtl::Reference< XSDDataType > pType = m_pHelper->getDataTypeByName(dataType);
            if ( pType.is() && m_pHelper->canBindToDataType( pType->classify() ) )
                _rNames.push_back(dataType);
        }
    }


} // namespace pcr

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
extensions_propctrlr_XSDValidationPropertyHandler_get_implementation(
    css::uno::XComponentContext* context , css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new pcr::XSDValidationPropertyHandler(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
