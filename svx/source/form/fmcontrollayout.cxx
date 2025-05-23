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


#include <fmcontrollayout.hxx>
#include <fmprop.hxx>

#include <com/sun/star/form/FormComponentType.hpp>
#include <com/sun/star/awt/VisualEffect.hpp>
#include <com/sun/star/i18n/ScriptType.hpp>
#include <com/sun/star/lang/Locale.hpp>
#include <com/sun/star/awt/FontDescriptor.hpp>
#include <com/sun/star/style/XStyleFamiliesSupplier.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/container/XChild.hpp>

#include <comphelper/processfactory.hxx>
#include <i18nlangtag/mslangid.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <unotools/confignode.hxx>
#include <unotools/syslocale.hxx>
#include <unotools/localedatawrapper.hxx>

#include <toolkit/helper/vclunohelper.hxx>
#include <tools/debug.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <vcl/outdev.hxx>


namespace svxform
{


    using namespace ::utl;
    using ::com::sun::star::uno::Reference;
    using ::com::sun::star::uno::XInterface;
    using ::com::sun::star::uno::UNO_QUERY;
    using ::com::sun::star::uno::UNO_QUERY_THROW;
    using ::com::sun::star::uno::UNO_SET_THROW;
    using ::com::sun::star::uno::Exception;
    using ::com::sun::star::uno::RuntimeException;
    using ::com::sun::star::uno::Any;
    using ::com::sun::star::beans::XPropertySet;
    using ::com::sun::star::beans::XPropertySetInfo;
    using ::com::sun::star::lang::Locale;
    using ::com::sun::star::awt::FontDescriptor;
    using ::com::sun::star::style::XStyleFamiliesSupplier;
    using ::com::sun::star::lang::XServiceInfo;
    using ::com::sun::star::container::XNameAccess;
    using ::com::sun::star::container::XChild;

    namespace FormComponentType = ::com::sun::star::form::FormComponentType;
    namespace VisualEffect = ::com::sun::star::awt::VisualEffect;
    namespace ScriptType = ::com::sun::star::i18n::ScriptType;


    namespace
    {
        ::utl::OConfigurationNode getLayoutSettings( DocumentType _eDocType )
        {
            OUString sConfigName = "/org.openoffice.Office.Common/Forms/ControlLayout/" +
                DocumentClassification::getModuleIdentifierForDocumentType( _eDocType );
            return OConfigurationTreeRoot::createWithComponentContext(
                ::comphelper::getProcessComponentContext(),    // TODO
                sConfigName );
        }

        template< class INTERFACE_TYPE >
        Reference< INTERFACE_TYPE > getTypedModelNode( const Reference< XInterface >& _rxModelNode )
        {
            Reference< INTERFACE_TYPE > xTypedNode( _rxModelNode, UNO_QUERY );
            if ( xTypedNode.is() )
                return xTypedNode;
            else
            {
                Reference< XChild > xChild( _rxModelNode, UNO_QUERY );
                if ( xChild.is() )
                    return getTypedModelNode< INTERFACE_TYPE >( xChild->getParent() );
                else
                    return nullptr;
            }
        }


        bool lcl_getDocumentDefaultStyleAndFamily( const Reference< XInterface >& _rxDocument, OUString& _rFamilyName, OUString& _rStyleName )
        {
            bool bSuccess = true;
            Reference< XServiceInfo > xDocumentSI( _rxDocument, UNO_QUERY );
            if ( xDocumentSI.is() )
            {
                if (  xDocumentSI->supportsService(u"com.sun.star.text.TextDocument"_ustr)
                   || xDocumentSI->supportsService(u"com.sun.star.text.WebDocument"_ustr)
                   )
                {
                    _rFamilyName = "ParagraphStyles";
                    _rStyleName = "Standard";
                }
                else if ( xDocumentSI->supportsService(u"com.sun.star.sheet.SpreadsheetDocument"_ustr) )
                {
                    _rFamilyName = "CellStyles";
                    _rStyleName = "Default";
                }
                else if (  xDocumentSI->supportsService(u"com.sun.star.drawing.DrawingDocument"_ustr)
                        || xDocumentSI->supportsService(u"com.sun.star.presentation.PresentationDocument"_ustr)
                        )
                {
                    _rFamilyName = "graphics";
                    _rStyleName = "standard";
                }
                else
                    bSuccess = false;
            }
            return bSuccess;
        }


        void lcl_initializeControlFont( const Reference< XPropertySet >& _rxModel )
        {
            try
            {
                Reference< XPropertySet > xStyle( ControlLayouter::getDefaultDocumentTextStyle( _rxModel ), UNO_SET_THROW );
                Reference< XPropertySetInfo > xStylePSI( xStyle->getPropertySetInfo(), UNO_SET_THROW );

                // determine the script type associated with the system locale
                const SvtSysLocale aSysLocale;
                const LocaleDataWrapper& rSysLocaleData = aSysLocale.GetLocaleData();
                const sal_Int16 eSysLocaleScriptType = MsLangId::getScriptType( rSysLocaleData.getLanguageTag().getLanguageType() );

                // depending on this script type, use the right property from the document's style which controls the
                // default locale for document content
                OUString sCharLocalePropertyName = u"CharLocale"_ustr;
                switch ( eSysLocaleScriptType )
                {
                case ScriptType::LATIN:
                    // already defaulted above
                    break;
                case ScriptType::ASIAN:
                    sCharLocalePropertyName = u"CharLocaleAsian"_ustr;
                    break;
                case ScriptType::COMPLEX:
                    sCharLocalePropertyName = u"CharLocaleComplex"_ustr;
                    break;
                default:
                    OSL_FAIL( "lcl_initializeControlFont: unexpected script type for system locale!" );
                    break;
                }

                Locale aDocumentCharLocale;
                if ( xStylePSI->hasPropertyByName( sCharLocalePropertyName ) )
                {
                    OSL_VERIFY( xStyle->getPropertyValue( sCharLocalePropertyName ) >>= aDocumentCharLocale );
                }
                // fall back to CharLocale property at the style
                if ( aDocumentCharLocale.Language.isEmpty() )
                {
                    sCharLocalePropertyName = "CharLocale";
                    if ( xStylePSI->hasPropertyByName( sCharLocalePropertyName ) )
                    {
                        OSL_VERIFY( xStyle->getPropertyValue( sCharLocalePropertyName ) >>= aDocumentCharLocale );
                    }
                }
                // fall back to the system locale
                if ( aDocumentCharLocale.Language.isEmpty() )
                {
                    aDocumentCharLocale = rSysLocaleData.getLanguageTag().getLocale();
                }

                // retrieve a default font for this locale, and set it at the control
                vcl::Font aFont = OutputDevice::GetDefaultFont( DefaultFontType::SANS, LanguageTag::convertToLanguageType( aDocumentCharLocale ), GetDefaultFontFlags::OnlyOne );
                FontDescriptor aFontDesc = VCLUnoHelper::CreateFontDescriptor( aFont );
                _rxModel->setPropertyValue(u"FontDescriptor"_ustr, Any( aFontDesc )
                );
            }
            catch( const Exception& )
            {
                DBG_UNHANDLED_EXCEPTION("svx");
            }
        }
    }


    //= ControlLayouter


    Reference< XPropertySet > ControlLayouter::getDefaultDocumentTextStyle( const Reference< XPropertySet >& _rxModel )
    {
        // the style family collection
        Reference< XStyleFamiliesSupplier > xSuppStyleFamilies( getTypedModelNode< XStyleFamiliesSupplier >( _rxModel ), UNO_SET_THROW );
        Reference< XNameAccess > xStyleFamilies( xSuppStyleFamilies->getStyleFamilies(), UNO_SET_THROW );

        // the names of the family, and the style - depends on the document type we live in
        OUString sFamilyName, sStyleName;
        if ( !lcl_getDocumentDefaultStyleAndFamily( xSuppStyleFamilies, sFamilyName, sStyleName ) )
            throw RuntimeException(u"unknown document type!"_ustr);

        // the concrete style
        Reference< XNameAccess > xStyleFamily( xStyleFamilies->getByName( sFamilyName ), UNO_QUERY_THROW );
        return Reference< XPropertySet >( xStyleFamily->getByName( sStyleName ), UNO_QUERY_THROW );
    }


    void ControlLayouter::initializeControlLayout( const Reference< XPropertySet >& _rxControlModel, DocumentType _eDocType )
    {
        DBG_ASSERT( _rxControlModel.is(), "ControlLayouter::initializeControlLayout: invalid model!" );
        if ( !_rxControlModel.is() )
            return;

        try
        {
            Reference< XPropertySetInfo > xPSI( _rxControlModel->getPropertySetInfo(), UNO_SET_THROW );

            // the control type
            sal_Int16 nClassId = FormComponentType::CONTROL;
            _rxControlModel->getPropertyValue( FM_PROP_CLASSID ) >>= nClassId;

            // the document type
            if ( _eDocType == eUnknownDocumentType )
                _eDocType = DocumentClassification::classifyHostDocument( _rxControlModel );

            // let's see what the configuration says about the visual effect
            OConfigurationNode  aConfig = getLayoutSettings( _eDocType );
            Any aVisualEffect = aConfig.getNodeValue( u"VisualEffect"_ustr );
            if ( aVisualEffect.hasValue() )
            {
                OUString sVisualEffect;
                OSL_VERIFY( aVisualEffect >>= sVisualEffect );

                sal_Int16 nVisualEffect = VisualEffect::NONE;
                if ( sVisualEffect == "flat" )
                    nVisualEffect = VisualEffect::FLAT;
                else if ( sVisualEffect == "3D" )
                    nVisualEffect = VisualEffect::LOOK3D;

                if ( xPSI->hasPropertyByName( FM_PROP_BORDER ) )
                {
                    if  (  ( nClassId != FormComponentType::COMMANDBUTTON )
                        && ( nClassId != FormComponentType::RADIOBUTTON )
                        && ( nClassId != FormComponentType::CHECKBOX    )
                        && ( nClassId != FormComponentType::GROUPBOX )
                        && ( nClassId != FormComponentType::FIXEDTEXT )
                        && ( nClassId != FormComponentType::SCROLLBAR )
                        && ( nClassId != FormComponentType::SPINBUTTON )
                        )
                    {
                        _rxControlModel->setPropertyValue( FM_PROP_BORDER, Any( nVisualEffect ) );
                        if  (   ( nVisualEffect == VisualEffect::FLAT )
                            &&  ( xPSI->hasPropertyByName( FM_PROP_BORDERCOLOR ) )
                            )
                            // light gray flat border
                            _rxControlModel->setPropertyValue( FM_PROP_BORDERCOLOR, Any( sal_Int32(0x00C0C0C0) ) );
                    }
                }
                if ( xPSI->hasPropertyByName( FM_PROP_VISUALEFFECT ) )
                    _rxControlModel->setPropertyValue( FM_PROP_VISUALEFFECT, Any( nVisualEffect ) );
            }

            // the font (only if we use the document's ref devices for rendering control text, otherwise, the
            // default font from application or standard style is assumed to be fine)
            if  (   useDocumentReferenceDevice( _eDocType )
                &&  xPSI->hasPropertyByName( FM_PROP_FONT )
                )
                lcl_initializeControlFont( _rxControlModel );
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "svx", "ControlLayouter::initializeControlLayout" );
        }
    }

    bool ControlLayouter::useDynamicBorderColor( DocumentType _eDocType )
    {
        OConfigurationNode aConfig = getLayoutSettings( _eDocType );
        Any aDynamicBorderColor = aConfig.getNodeValue( u"DynamicBorderColors"_ustr );
        bool bDynamicBorderColor = false;
        OSL_VERIFY( aDynamicBorderColor >>= bDynamicBorderColor );
        return bDynamicBorderColor;
    }


    bool ControlLayouter::useDocumentReferenceDevice( DocumentType _eDocType )
    {
        if ( _eDocType == eUnknownDocumentType )
            return false;
        OConfigurationNode aConfig = getLayoutSettings( _eDocType );
        Any aUseRefDevice = aConfig.getNodeValue( u"UseDocumentTextMetrics"_ustr );
        bool bUseRefDevice = false;
        OSL_VERIFY( aUseRefDevice >>= bUseRefDevice );
        return bUseRefDevice;
    }


}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
