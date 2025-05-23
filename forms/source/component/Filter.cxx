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

#include <config_features.h>
#include <config_fuzzers.h>

#include "Filter.hxx"
#include <strings.hrc>
#include <frm_resource.hxx>
#include <frm_strings.hxx>

#include <com/sun/star/awt/VclWindowPeerAttribute.hpp>
#include <com/sun/star/awt/XCheckBox.hpp>
#include <com/sun/star/awt/XComboBox.hpp>
#include <com/sun/star/awt/XListBox.hpp>
#include <com/sun/star/awt/XRadioButton.hpp>
#include <com/sun/star/awt/XVclWindowPeer.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/container/XChild.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/container/XNamed.hpp>
#include <com/sun/star/form/FormComponentType.hpp>
#include <com/sun/star/sdb/ErrorMessageDialog.hpp>
#include <com/sun/star/sdbc/XConnection.hpp>
#include <com/sun/star/sdbc/XRowSet.hpp>
#include <com/sun/star/sdbcx/XColumnsSupplier.hpp>
#include <com/sun/star/sdbcx/XTablesSupplier.hpp>
#include <com/sun/star/ui/dialogs/XExecutableDialog.hpp>
#include <com/sun/star/util/NumberFormatter.hpp>
#include <com/sun/star/awt/XItemList.hpp>

#include <comphelper/property.hxx>
#include <comphelper/types.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <connectivity/dbtools.hxx>
#include <connectivity/formattedcolumnvalue.hxx>
#include <connectivity/predicateinput.hxx>
#include <o3tl/safeint.hxx>
#include <rtl/ustrbuf.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <tools/gen.hxx>


namespace frm
{
    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::awt;
    using namespace ::com::sun::star::lang;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::sdb;
    using namespace ::com::sun::star::sdbc;
    using namespace ::com::sun::star::sdbcx;
    using namespace ::com::sun::star::util;
    using namespace ::com::sun::star::form;
    using namespace ::com::sun::star::container;
    using namespace ::com::sun::star::ui::dialogs;

    OFilterControl::OFilterControl( const Reference< XComponentContext >& _rxORB )
        :m_aTextListeners( *this )
        ,m_xContext( _rxORB )
        ,m_nControlClass( FormComponentType::TEXTFIELD )
        ,m_bFilterList( false )
        ,m_bMultiLine( false )
        ,m_bFilterListFilled( false )
    {
    }


    bool OFilterControl::ensureInitialized( )
    {
#if HAVE_FEATURE_DBCONNECTIVITY && !ENABLE_FUZZERS
        if ( !m_xField.is() )
        {
            OSL_FAIL( "OFilterControl::ensureInitialized: improperly initialized: no field!" );
            return false;
        }

        if ( !m_xConnection.is() )
        {
            OSL_FAIL( "OFilterControl::ensureInitialized: improperly initialized: no connection!" );
            return false;
        }

        if ( !m_xFormatter.is() )
        {
            // we can create one from the connection, if it's an SDB connection

            Reference< XNumberFormatsSupplier > xFormatSupplier = ::dbtools::getNumberFormats( m_xConnection, true, m_xContext );

            if ( xFormatSupplier.is() )
            {
                m_xFormatter.set(NumberFormatter::create(m_xContext), UNO_QUERY_THROW );
                m_xFormatter->attachNumberFormatsSupplier( xFormatSupplier );
            }
        }
        if ( !m_xFormatter.is() )
        {
            OSL_FAIL( "OFilterControl::ensureInitialized: no number formatter!" );
            // no fallback anymore
            return false;
        }
#endif
        return true;
    }


    Any SAL_CALL OFilterControl::queryAggregation( const Type & rType )
    {
        Any aRet = UnoControl::queryAggregation( rType);
        if(!aRet.hasValue())
            aRet = OFilterControl_BASE::queryInterface(rType);

        return aRet;
    }


    OUString OFilterControl::GetComponentServiceName() const
    {
        OUString aServiceName;
        switch (m_nControlClass)
        {
            case FormComponentType::RADIOBUTTON:
                aServiceName = "radiobutton";
                break;
            case FormComponentType::CHECKBOX:
                aServiceName = "checkbox";
                break;
            case FormComponentType::COMBOBOX:
                aServiceName = "combobox";
                break;
            case FormComponentType::LISTBOX:
                aServiceName = "listbox";
                break;
            default:
                if (m_bMultiLine)
                    aServiceName = "MultiLineEdit";
                else
                    aServiceName = "Edit";
        }
        return aServiceName;
    }

    // XComponent

    void OFilterControl::dispose()
    {
        EventObject aEvt(*this);
        m_aTextListeners.disposeAndClear( aEvt );
        UnoControl::dispose();
    }


    void OFilterControl::createPeer( const Reference< XToolkit > & rxToolkit, const Reference< XWindowPeer >  & rParentPeer )
    {
        UnoControl::createPeer( rxToolkit, rParentPeer );

        try
        {
            Reference< XVclWindowPeer >  xVclWindow( getPeer(), UNO_QUERY_THROW );
            switch ( m_nControlClass )
            {
                case FormComponentType::CHECKBOX:
                {
                    // checkboxes always have a tristate-mode
                    xVclWindow->setProperty( PROPERTY_TRISTATE, Any( true ) );
                    xVclWindow->setProperty( PROPERTY_STATE, Any( sal_Int32( TRISTATE_INDET ) ) );

                    Reference< XCheckBox >  xBox( getPeer(), UNO_QUERY_THROW );
                    xBox->addItemListener( this );

                }
                break;

                case FormComponentType::RADIOBUTTON:
                {
                    xVclWindow->setProperty( PROPERTY_STATE, Any( sal_Int32( TRISTATE_FALSE ) ) );

                    Reference< XRadioButton >  xRadio( getPeer(), UNO_QUERY_THROW );
                    xRadio->addItemListener( this );
                }
                break;

                case FormComponentType::LISTBOX:
                {
                    Reference< XListBox >  xListBox( getPeer(), UNO_QUERY_THROW );
                    xListBox->addItemListener( this );
                    [[fallthrough]];
                }

                case FormComponentType::COMBOBOX:
                {
                    xVclWindow->setProperty(PROPERTY_AUTOCOMPLETE, Any( true ) );
                    [[fallthrough]];
                }

                default:
                {
                    Reference< XWindow >  xWindow( getPeer(), UNO_QUERY );
                    xWindow->addFocusListener( this );

                    Reference< XTextComponent >  xText( getPeer(), UNO_QUERY );
                    if (xText.is())
                        xText->setMaxTextLen(0);
                }
                break;
            }

            // filter controls are _never_ readonly
            Reference< XPropertySet > xModel( getModel(), UNO_QUERY_THROW );
            Reference< XPropertySetInfo > xModelPSI( xModel->getPropertySetInfo(), UNO_SET_THROW );
            if ( xModelPSI->hasPropertyByName( PROPERTY_READONLY ) )
                xVclWindow->setProperty( PROPERTY_READONLY, Any( false ) );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("forms.component");
        }

        if (m_bFilterList)
            m_bFilterListFilled = false;
    }


    void OFilterControl::PrepareWindowDescriptor( WindowDescriptor& rDescr )
    {
        if (m_bFilterList)
            rDescr.WindowAttributes |= VclWindowPeerAttribute::DROPDOWN;
    }


    void OFilterControl::ImplSetPeerProperty( const OUString& rPropName, const Any& rVal )
    {
        // these properties are ignored
        if (rPropName == PROPERTY_TEXT ||
            rPropName == PROPERTY_STATE)
            return;

        UnoControl::ImplSetPeerProperty( rPropName, rVal );
    }

    // XEventListener

    void SAL_CALL OFilterControl::disposing(const EventObject& Source)
    {
        UnoControl::disposing(Source);
    }

    // XItemListener

    void SAL_CALL OFilterControl::itemStateChanged( const ItemEvent& rEvent )
    {
#if !HAVE_FEATURE_DBCONNECTIVITY || ENABLE_FUZZERS
        (void) rEvent;
#else
        OUStringBuffer aText;
        switch (m_nControlClass)
        {
            case FormComponentType::CHECKBOX:
            {
                if ( ( rEvent.Selected == TRISTATE_TRUE ) || ( rEvent.Selected == TRISTATE_FALSE ) )
                {
                    sal_Int32 nBooleanComparisonMode = ::dbtools::DatabaseMetaData( m_xConnection ).getBooleanComparisonMode();

                    bool bSelected = ( rEvent.Selected == TRISTATE_TRUE );

                    OUString sExpressionMarker( u"$expression$"_ustr );
                    ::dbtools::getBooleanComparisonPredicate(
                        sExpressionMarker,
                        bSelected,
                        nBooleanComparisonMode,
                        aText
                    );

                    OUString sText( aText.makeStringAndClear() );
                    sal_Int32 nMarkerPos( sText.indexOf( sExpressionMarker ) );
                    OSL_ENSURE( nMarkerPos == 0, "OFilterControl::itemStateChanged: unsupported boolean comparison mode!" );
                    // If this assertion fails, then getBooleanComparisonPredicate created a predicate which
                    // does not start with the expression we gave it. The only known case is when
                    // the comparison mode is ACCESS_COMPAT, and the value is TRUE. In this case,
                    // the expression is rather complex.
                    // Well, so this is a known issue - the filter controls (and thus the form based filter)
                    // do not work with boolean MS Access fields.
                    // To fix this, we would probably have to revert here to always return "1" or "0" as normalized
                    // filter, and change our client code to properly translate this (which could be some effort).
                    if ( nMarkerPos == 0 )
                        aText.append( sText.subView(sExpressionMarker.getLength()) );
                    else
                    {
                        // fallback
                        aText.appendAscii( bSelected ? "1" : "0" );
                    }
                }
            }
            break;

            case FormComponentType::LISTBOX:
            {
                try
                {
                    const Reference< XItemList > xItemList( getModel(), UNO_QUERY_THROW );
                    OUString sItemText( xItemList->getItemText( rEvent.Selected ) );

                    const MapString2String::const_iterator itemPos = m_aDisplayItemToValueItem.find( sItemText );
                    if ( itemPos != m_aDisplayItemToValueItem.end() )
                    {
                        sItemText = itemPos->second;
                        if ( !sItemText.isEmpty() )
                        {
                            ::dbtools::OPredicateInputController aPredicateInput( m_xContext, m_xConnection, getParseContext() );
                            OUString sErrorMessage;
                            OSL_VERIFY( aPredicateInput.normalizePredicateString( sItemText, m_xField, &sErrorMessage ) );
                        }
                    }
                    aText.append( sItemText );
                }
                catch( const Exception& )
                {
                    DBG_UNHANDLED_EXCEPTION("forms.component");
                }
            }
            break;

            case FormComponentType::RADIOBUTTON:
            {
                if ( rEvent.Selected == TRISTATE_TRUE )
                    aText.append( ::comphelper::getString( Reference< XPropertySet >( getModel(), UNO_QUERY_THROW )->getPropertyValue( PROPERTY_REFVALUE ) ) );
            }
            break;
        }

        OUString sText( aText.makeStringAndClear() );
        if ( m_aText != sText )
        {
            m_aText = sText;
            TextEvent aEvt;
            aEvt.Source = *this;
            m_aTextListeners.notifyEach(&css::awt::XTextListener::textChanged, aEvt);
        }
#endif
    }


    void OFilterControl::implInitFilterList()
    {
#if HAVE_FEATURE_DBCONNECTIVITY && !ENABLE_FUZZERS
        if ( !ensureInitialized( ) )
            // already asserted in ensureInitialized
            return;

        // ensure the cursor and the statement are disposed as soon as we leave
        ::utl::SharedUNOComponent< XResultSet > xListCursor;
        ::utl::SharedUNOComponent< XStatement > xStatement;

        try
        {
            m_bFilterListFilled = true;

            if ( !m_xField.is() )
                return;

            OUString sFieldName;
            m_xField->getPropertyValue( PROPERTY_NAME ) >>= sFieldName;

            // here we need a table to which the field belongs to
            const Reference< XChild > xModelAsChild( getModel(), UNO_QUERY_THROW );
            const Reference< XRowSet > xForm( xModelAsChild->getParent(), UNO_QUERY_THROW );
            const Reference< XPropertySet > xFormProps( xForm, UNO_QUERY_THROW );

            // create a query composer
            Reference< XColumnsSupplier > xSuppColumns;
            xFormProps->getPropertyValue(u"SingleSelectQueryComposer"_ustr) >>= xSuppColumns;

            const Reference< XConnection > xConnection( ::dbtools::getConnection( xForm ), UNO_SET_THROW );
            const Reference< XNameAccess > xFieldNames( xSuppColumns->getColumns(), UNO_SET_THROW );
            if ( !xFieldNames->hasByName( sFieldName ) )
                return;
            OUString sRealFieldName, sTableName;
            const Reference< XPropertySet > xComposerFieldProps( xFieldNames->getByName( sFieldName ), UNO_QUERY_THROW );
            xComposerFieldProps->getPropertyValue( PROPERTY_REALNAME ) >>= sRealFieldName;
            xComposerFieldProps->getPropertyValue( PROPERTY_TABLENAME ) >>= sTableName;

            // obtain the table of the field
            const Reference< XTablesSupplier > xSuppTables( xSuppColumns, UNO_QUERY_THROW );
            const Reference< XNameAccess > xTablesNames( xSuppTables->getTables(), UNO_SET_THROW );
            const Reference< XNamed > xNamedTable( xTablesNames->getByName( sTableName ), UNO_QUERY_THROW );
            sTableName = xNamedTable->getName();

            // create a statement selecting all values for the given field
            OUStringBuffer aStatement;

            const Reference< XDatabaseMetaData >  xMeta( xConnection->getMetaData(), UNO_SET_THROW );
            const OUString sQuoteChar = xMeta->getIdentifierQuoteString();

            aStatement.append(
                "SELECT DISTINCT "
                + sQuoteChar
                + sRealFieldName
                + sQuoteChar );

            // if the field had an alias in our form's statement, give it this alias in the new statement, too
            if ( !sFieldName.isEmpty() && ( sFieldName != sRealFieldName ) )
            {
                aStatement.append(" AS "+ sQuoteChar + sFieldName + sQuoteChar );
            }

            aStatement.append( " FROM " );

            OUString sCatalog, sSchema, sTable;
            ::dbtools::qualifiedNameComponents( xMeta, sTableName, sCatalog, sSchema, sTable, ::dbtools::EComposeRule::InDataManipulation );
            aStatement.append( ::dbtools::composeTableNameForSelect( xConnection, sCatalog, sSchema, sTable ) );

            // execute the statement
            xStatement.reset( xConnection->createStatement() );
            const OUString sSelectStatement( aStatement.makeStringAndClear( ) );
            xListCursor.reset( xStatement->executeQuery( sSelectStatement ) );

            // retrieve the one column which we take the values from
            const Reference< XColumnsSupplier > xSupplyCols( xListCursor, UNO_QUERY_THROW );
            const Reference< XIndexAccess > xFields( xSupplyCols->getColumns(), UNO_QUERY_THROW );
            const Reference< XPropertySet > xDataField( xFields->getByIndex(0), UNO_QUERY_THROW );

            // ensure the values will be  formatted according to the field format
            const ::dbtools::FormattedColumnValue aFormatter( m_xFormatter, xDataField );

            ::std::vector< OUString > aProposals;
            aProposals.reserve(16);

            while ( xListCursor->next() && ( aProposals.size() < o3tl::make_unsigned( SHRT_MAX ) ) )
            {
                const OUString sCurrentValue = aFormatter.getFormattedValue();
                aProposals.push_back( sCurrentValue );
            }

            // fill the list items into our peer
            Sequence< OUString> aStringSeq( comphelper::containerToSequence(aProposals) );

            const Reference< XComboBox >  xComboBox( getPeer(), UNO_QUERY_THROW );
            xComboBox->addItems( aStringSeq, 0 );

            // set the drop down line count to something reasonable
            const sal_Int16 nLineCount = ::std::min( sal_Int16( 16 ), sal_Int16( aStringSeq.getLength() ) );
            xComboBox->setDropDownLineCount( nLineCount );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("forms.component");
        }
#endif
    }

    // XFocusListener

    void SAL_CALL OFilterControl::focusGained(const FocusEvent& /*e*/)
    {
        // should we fill the combobox?
        if (m_bFilterList && !m_bFilterListFilled)
            implInitFilterList();
    }


    void SAL_CALL OFilterControl::focusLost(const FocusEvent& /*e*/)
    {
    }


    sal_Bool SAL_CALL OFilterControl::commit()
    {
#if HAVE_FEATURE_DBCONNECTIVITY && !ENABLE_FUZZERS
        if ( !ensureInitialized( ) )
            // already asserted in ensureInitialized
            return true;

        OUString aText;
        switch (m_nControlClass)
        {
            case FormComponentType::TEXTFIELD:
            case FormComponentType::COMBOBOX:
            {
                Reference< XTextComponent >  xText( getPeer(), UNO_QUERY );
                if (xText.is())
                    aText = xText->getText();
            }   break;
            default:
                return true;
        }
        if ( m_aText == aText )
            return true;
        // check the text with the SQL-Parser
        OUString aNewText = aText.trim();
        if ( !aNewText.isEmpty() )
        {
            ::dbtools::OPredicateInputController aPredicateInput( m_xContext, m_xConnection, getParseContext() );
            OUString sErrorMessage;
            if ( !aPredicateInput.normalizePredicateString( aNewText, m_xField, &sErrorMessage ) )
            {
                // display the error and outta here
                SQLContext aError(ResourceManager::loadString(RID_STR_SYNTAXERROR), {}, {}, 0, {}, sErrorMessage);
                displayException( aError );
                return false;
            }
        }

        setText(aNewText);
        TextEvent aEvt;
        aEvt.Source = *this;
        m_aTextListeners.notifyEach(&css::awt::XTextListener::textChanged, aEvt);
#endif
        return true;
    }

    // XTextComponent

    void SAL_CALL OFilterControl::addTextListener(const Reference< XTextListener > & l)
    {
        m_aTextListeners.addInterface( l );
    }


    void SAL_CALL OFilterControl::removeTextListener(const Reference< XTextListener > & l)
    {
        m_aTextListeners.removeInterface( l );
    }


    void SAL_CALL OFilterControl::setText( const OUString& aText )
    {
        if ( !ensureInitialized( ) )
            // already asserted in ensureInitialized
            return;

        switch (m_nControlClass)
        {
            case FormComponentType::CHECKBOX:
            {
                Reference< XVclWindowPeer >  xVclWindow( getPeer(), UNO_QUERY );
                if (xVclWindow.is())
                {
                    Any aValue;
                    if  (   aText == "1"
                        ||  aText.equalsIgnoreAsciiCase("TRUE")
                        ||  aText.equalsIgnoreAsciiCase("IS TRUE")
                        )
                    {
                        aValue <<= sal_Int32(TRISTATE_TRUE);
                    }
                    else if ( aText == "0" || aText.equalsIgnoreAsciiCase("FALSE") )
                    {
                        aValue <<= sal_Int32(TRISTATE_FALSE);
                    }
                    else
                        aValue <<= sal_Int32(TRISTATE_INDET);

                    m_aText = aText;
                    xVclWindow->setProperty( PROPERTY_STATE, aValue );
                }
            }   break;
            case FormComponentType::RADIOBUTTON:
            {
                Reference< XVclWindowPeer >  xVclWindow( getPeer(), UNO_QUERY );
                if (xVclWindow.is())
                {
                    OUString aRefText = ::comphelper::getString(css::uno::Reference< XPropertySet > (getModel(), UNO_QUERY_THROW)->getPropertyValue(PROPERTY_REFVALUE));
                    Any aValue;
                    if (aText == aRefText)
                        aValue <<= sal_Int32(TRISTATE_TRUE);
                    else
                        aValue <<= sal_Int32(TRISTATE_FALSE);
                    m_aText = aText;
                    xVclWindow->setProperty(PROPERTY_STATE, aValue);
                }
            }   break;
            case FormComponentType::LISTBOX:
            {
                Reference< XListBox >  xListBox( getPeer(), UNO_QUERY );
                if (xListBox.is())
                {
                    m_aText = aText;
                    MapString2String::const_iterator itemPos = m_aDisplayItemToValueItem.find( m_aText );
                    if ( itemPos == m_aDisplayItemToValueItem.end() )
                    {
                        const bool isQuoted =   ( m_aText.getLength() > 1 )
                                            &&  ( m_aText[0] == '\'' )
                                            &&  ( m_aText[ m_aText.getLength() - 1 ] == '\'' );
                        if ( isQuoted )
                        {
                            m_aText = m_aText.copy( 1, m_aText.getLength() - 2 );
                            itemPos = m_aDisplayItemToValueItem.find( m_aText );
                        }
                    }

                    OSL_ENSURE( ( itemPos != m_aDisplayItemToValueItem.end() ) || m_aText.isEmpty(),
                        "OFilterControl::setText: this text is not in my display list!" );
                    if ( itemPos == m_aDisplayItemToValueItem.end() )
                        m_aText.clear();

                    if ( m_aText.isEmpty() )
                    {
                        while ( xListBox->getSelectedItemPos() >= 0 )
                        {
                            xListBox->selectItemPos( xListBox->getSelectedItemPos(), false );
                        }
                    }
                    else
                    {
                        xListBox->selectItem( m_aText, true );
                    }
                }
            }
            break;

            default:
            {
                Reference< XTextComponent >  xText( getPeer(), UNO_QUERY );
                if (xText.is())
                {
                    m_aText = aText;
                    xText->setText(aText);
                }
            }
        }
    }


    void SAL_CALL OFilterControl::insertText( const css::awt::Selection& rSel, const OUString& aText )
    {
        Reference< XTextComponent >  xText( getPeer(), UNO_QUERY );
        if (xText.is())
        {
            xText->insertText(rSel, aText);
            m_aText = xText->getText();
        }
    }


    OUString SAL_CALL OFilterControl::getText()
    {
        return m_aText;
    }


    OUString SAL_CALL OFilterControl::getSelectedText()
    {
        OUString aSelected;
        Reference< XTextComponent >  xText( getPeer(), UNO_QUERY );
        if (xText.is())
            aSelected = xText->getSelectedText();

        return aSelected;
    }


    void SAL_CALL OFilterControl::setSelection( const css::awt::Selection& aSelection )
    {
        Reference< XTextComponent >  xText( getPeer(), UNO_QUERY );
        if (xText.is())
            xText->setSelection( aSelection );
    }


    css::awt::Selection SAL_CALL OFilterControl::getSelection()
    {
        css::awt::Selection aSel;
        Reference< XTextComponent >  xText( getPeer(), UNO_QUERY );
        if (xText.is())
            aSel = xText->getSelection();
        return aSel;
    }


    sal_Bool SAL_CALL OFilterControl::isEditable()
    {
        Reference< XTextComponent >  xText( getPeer(), UNO_QUERY );
        return xText.is() && xText->isEditable();
    }


    void SAL_CALL OFilterControl::setEditable( sal_Bool bEditable )
    {
        Reference< XTextComponent >  xText( getPeer(), UNO_QUERY );
        if (xText.is())
            xText->setEditable(bEditable);
    }


    sal_Int16 SAL_CALL OFilterControl::getMaxTextLen()
    {
        Reference< XTextComponent >  xText( getPeer(), UNO_QUERY );
        return xText.is() ? xText->getMaxTextLen() : 0;
    }


    void SAL_CALL OFilterControl::setMaxTextLen( sal_Int16 nLength )
    {
        Reference< XTextComponent >  xText( getPeer(), UNO_QUERY );
        if (xText.is())
            xText->setMaxTextLen(nLength);
    }


    void OFilterControl::displayException( const css::sdb::SQLContext& _rExcept )
    {
        try
        {
            Reference< XExecutableDialog > xErrorDialog = ErrorMessageDialog::create( m_xContext, u""_ustr,  m_xMessageParent, Any(_rExcept));
            xErrorDialog->execute();
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("forms.component");
        }
    }


    void SAL_CALL OFilterControl::initialize( const Sequence< Any >& aArguments )
    {
        const Any* pArguments = aArguments.getConstArray();
        const Any* pArgumentsEnd = pArguments + aArguments.getLength();

        PropertyValue aProp;
        NamedValue aValue;
        const OUString* pName = nullptr;
        const Any* pValue = nullptr;
        Reference< XPropertySet > xControlModel;

        if (aArguments.getLength() == 3
            && (aArguments[0] >>= m_xMessageParent)
            && (aArguments[1] >>= m_xFormatter)
            && (aArguments[2] >>= xControlModel))
        {
            initControlModel(xControlModel);
        }
        else for ( ; pArguments != pArgumentsEnd; ++pArguments )
        {
            // we recognize PropertyValues and NamedValues
            if ( *pArguments >>= aProp )
            {
                pName = &aProp.Name;
                pValue = &aProp.Value;
            }
            else if ( *pArguments >>= aValue )
            {
                pName = &aValue.Name;
                pValue = &aValue.Value;
            }
            else
            {
                OSL_FAIL( "OFilterControl::initialize: unrecognized argument!" );
                continue;
            }

            if ( *pName == "MessageParent" )
            {
                // the message parent
                *pValue >>= m_xMessageParent;
                OSL_ENSURE( m_xMessageParent.is(), "OFilterControl::initialize: invalid MessageParent!" );
            }
            else if ( *pName == "NumberFormatter" )
            {
                // the number format. This argument is optional.
                *pValue >>= m_xFormatter;
                OSL_ENSURE( m_xFormatter.is(), "OFilterControl::initialize: invalid NumberFormatter!" );
            }
            else if ( *pName == "ControlModel" )
            {
                // the control model for which we act as filter control
                if ( !(*pValue >>= xControlModel ) )
                {
                    OSL_FAIL( "OFilterControl::initialize: invalid control model argument!" );
                    continue;
                }
                initControlModel(xControlModel);
            }
        }
    }

    void OFilterControl::initControlModel(Reference< XPropertySet > const & xControlModel)
    {
#if !HAVE_FEATURE_DBCONNECTIVITY || ENABLE_FUZZERS
        (void) xControlModel;
#else
        if ( !xControlModel.is() )
        {
            OSL_FAIL( "OFilterControl::initialize: invalid control model argument!" );
            return;
        }
        // some properties which are "derived" from the control model we're working for

        // the field
        m_xField.clear();
        OSL_ENSURE( ::comphelper::hasProperty( PROPERTY_BOUNDFIELD, xControlModel ), "OFilterControl::initialize: control model needs a bound field property!" );
        xControlModel->getPropertyValue( PROPERTY_BOUNDFIELD ) >>= m_xField;


        // filter list and control class
        m_bFilterList = ::comphelper::hasProperty( PROPERTY_FILTERPROPOSAL, xControlModel ) && ::comphelper::getBOOL( xControlModel->getPropertyValue( PROPERTY_FILTERPROPOSAL ) );
        if ( m_bFilterList )
            m_nControlClass = FormComponentType::COMBOBOX;
        else
        {
            sal_Int16 nClassId = ::comphelper::getINT16( xControlModel->getPropertyValue( PROPERTY_CLASSID ) );
            switch (nClassId)
            {
                case FormComponentType::CHECKBOX:
                case FormComponentType::RADIOBUTTON:
                case FormComponentType::LISTBOX:
                case FormComponentType::COMBOBOX:
                    m_nControlClass = nClassId;
                    if ( FormComponentType::LISTBOX == nClassId )
                    {
                        Sequence< OUString > aDisplayItems;
                        OSL_VERIFY( xControlModel->getPropertyValue( PROPERTY_STRINGITEMLIST ) >>= aDisplayItems );
                        Sequence< OUString > aValueItems;
                        OSL_VERIFY( xControlModel->getPropertyValue( PROPERTY_VALUE_SEQ ) >>= aValueItems );
                        OSL_ENSURE( aDisplayItems.getLength() == aValueItems.getLength(), "OFilterControl::initialize: inconsistent item lists!" );
                        for ( sal_Int32 i=0; i < ::std::min( aDisplayItems.getLength(), aValueItems.getLength() ); ++i )
                            m_aDisplayItemToValueItem[ aDisplayItems[i] ] = aValueItems[i];
                    }
                    break;
                default:
                    m_bMultiLine = ::comphelper::hasProperty( PROPERTY_MULTILINE, xControlModel ) && ::comphelper::getBOOL( xControlModel->getPropertyValue( PROPERTY_MULTILINE ) );
                    m_nControlClass = FormComponentType::TEXTFIELD;
                    break;
            }
        }


        // the connection meta data for the form which we're working for
        Reference< XChild > xModel( xControlModel, UNO_QUERY );
        Reference< XRowSet > xForm;
        if ( xModel.is() )
            xForm.set(xModel->getParent(), css::uno::UNO_QUERY);
        m_xConnection = ::dbtools::getConnection( xForm );
        OSL_ENSURE( m_xConnection.is(), "OFilterControl::initialize: unable to determine the form's connection!" );
#endif
    }

    OUString SAL_CALL OFilterControl::getImplementationName(  )
    {
        return u"com.sun.star.comp.forms.OFilterControl"_ustr;
    }

    sal_Bool SAL_CALL OFilterControl::supportsService( const OUString& ServiceName )
    {
        return cppu::supportsService(this, ServiceName);
    }

    Sequence< OUString > SAL_CALL OFilterControl::getSupportedServiceNames(  )
    {
        return { u"com.sun.star.form.control.FilterControl"_ustr,
                 u"com.sun.star.awt.UnoControl"_ustr };
    }
}   // namespace frm

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_comp_forms_OFilterControl_get_implementation(css::uno::XComponentContext* context,
        css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new frm::OFilterControl(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
