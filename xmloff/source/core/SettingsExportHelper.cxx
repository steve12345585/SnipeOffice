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


#include <sax/tools/converter.hxx>

#include <xmloff/SettingsExportHelper.hxx>
#include <xmloff/xmltoken.hxx>
#include <rtl/ref.hxx>
#include <sal/log.hxx>
#include <tools/debug.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <comphelper/base64.hxx>
#include <comphelper/extract.hxx>
#include <unotools/securityoptions.hxx>

#include <com/sun/star/linguistic2/XSupportedLocales.hpp>
#include <com/sun/star/i18n/XForbiddenCharacters.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/container/XIndexContainer.hpp>
#include <com/sun/star/util/PathSubstitution.hpp>
#include <com/sun/star/util/DateTime.hpp>
#include <com/sun/star/formula/SymbolDescriptor.hpp>
#include <com/sun/star/document/PrinterIndependentLayout.hpp>
#include <comphelper/indexedpropertyvalues.hxx>
#include <xmloff/XMLSettingsExportContext.hxx>
#include "xmlenums.hxx"

using namespace ::com::sun::star;
using namespace ::xmloff::token;

constexpr OUStringLiteral gsPrinterIndependentLayout( u"PrinterIndependentLayout" );
constexpr OUStringLiteral gsColorTableURL( u"ColorTableURL" );
constexpr OUStringLiteral gsLineEndTableURL( u"LineEndTableURL" );
constexpr OUStringLiteral gsHatchTableURL( u"HatchTableURL" );
constexpr OUStringLiteral gsDashTableURL( u"DashTableURL" );
constexpr OUStringLiteral gsGradientTableURL( u"GradientTableURL" );
constexpr OUStringLiteral gsBitmapTableURL( u"BitmapTableURL" );

XMLSettingsExportHelper::XMLSettingsExportHelper( ::xmloff::XMLSettingsExportContext& i_rContext )
: m_rContext( i_rContext )
{
}

XMLSettingsExportHelper::~XMLSettingsExportHelper()
{
}

void XMLSettingsExportHelper::CallTypeFunction(const uno::Any& rAny,
                                            const OUString& rName) const
{
    uno::Any aAny( rAny );
    ManipulateSetting( aAny, rName );

    uno::TypeClass eClass = aAny.getValueTypeClass();
    switch (eClass)
    {
        case uno::TypeClass_VOID:
        {
            /*
             * This assertion pops up when exporting values which are set to:
             * PropertyAttribute::MAYBEVOID, and thus are _supposed_ to have
             * a VOID value...so I'm removing it ...mtg
             * OSL_FAIL("no type");
             */
        }
        break;
        case uno::TypeClass_BOOLEAN:
        {
            exportBool(::cppu::any2bool(aAny), rName);
        }
        break;
        case uno::TypeClass_BYTE:
        {
            exportByte();
        }
        break;
        case uno::TypeClass_SHORT:
        {
            sal_Int16 nInt16 = 0;
            aAny >>= nInt16;
            exportShort(nInt16, rName);
        }
        break;
        case uno::TypeClass_LONG:
        {
            sal_Int32 nInt32 = 0;
            aAny >>= nInt32;
            exportInt(nInt32, rName);
        }
        break;
        case uno::TypeClass_HYPER:
        {
            sal_Int64 nInt64 = 0;
            aAny >>= nInt64;
            exportLong(nInt64, rName);
        }
        break;
        case uno::TypeClass_DOUBLE:
        {
            double fDouble = 0.0;
            aAny >>= fDouble;
            exportDouble(fDouble, rName);
        }
        break;
        case uno::TypeClass_STRING:
        {
            OUString sString;
            aAny >>= sString;
            exportString(sString, rName);
        }
        break;
        default:
        {
            const uno::Type& aType = aAny.getValueType();
            if (aType.equals(cppu::UnoType<uno::Sequence<beans::PropertyValue>>::get() ) )
            {
                uno::Sequence< beans::PropertyValue> aProps;
                aAny >>= aProps;
                exportSequencePropertyValue(aProps, rName);
            }
            else if( aType.equals(cppu::UnoType<uno::Sequence<sal_Int8>>::get() ) )
            {
                uno::Sequence< sal_Int8 > aProps;
                aAny >>= aProps;
                exportbase64Binary(aProps, rName);
            }
            else if (uno::Reference<container::XNameAccess> aNamed; aAny >>= aNamed)
            {
                exportNameAccess(aNamed, rName);
            }
            else if (uno::Reference<container::XIndexAccess> aIndexed; aAny >>= aIndexed)
            {
                exportIndexAccess(aIndexed, rName);
            }
            else if (aType.equals(cppu::UnoType<util::DateTime>::get()) )
            {
                util::DateTime aDateTime;
                aAny >>= aDateTime;
                exportDateTime(aDateTime, rName);
            }
            else if (uno::Reference<i18n::XForbiddenCharacters> xForbChars; aAny >>= xForbChars)
            {
                exportForbiddenCharacters(xForbChars, rName);
            }
            else if( aType.equals(cppu::UnoType<uno::Sequence<formula::SymbolDescriptor>>::get() ) )
            {
                uno::Sequence< formula::SymbolDescriptor > aProps;
                aAny >>= aProps;
                exportSymbolDescriptors(aProps, rName);
            }
            else {
                SAL_WARN("xmloff", "this type (" << aType.getTypeName() << ") is not implemented now");
            }
        }
        break;
    }
}

void XMLSettingsExportHelper::exportBool(const bool bValue, const OUString& rName) const
{
    DBG_ASSERT(!rName.isEmpty(), "no name");
    m_rContext.AddAttribute( XML_NAME, rName );
    m_rContext.AddAttribute( XML_TYPE, XML_BOOLEAN );
    m_rContext.StartElement( XML_CONFIG_ITEM );
    OUString sValue;
    if (bValue)
        sValue = GetXMLToken(XML_TRUE);
    else
        sValue = GetXMLToken(XML_FALSE);
    m_rContext.Characters( sValue );
    m_rContext.EndElement( false );
}

void XMLSettingsExportHelper::exportByte()
{
    OSL_ENSURE(false, "XMLSettingsExportHelper::exportByte(): #i114162#:\n"
        "config-items of type \"byte\" are not valid ODF, "
        "so storing them is disabled!\n"
        "Use a different type instead (e.g. \"short\").");
}
void XMLSettingsExportHelper::exportShort(const sal_Int16 nValue, const OUString& rName) const
{
    DBG_ASSERT(!rName.isEmpty(), "no name");
    m_rContext.AddAttribute( XML_NAME, rName );
    m_rContext.AddAttribute( XML_TYPE, XML_SHORT );
    m_rContext.StartElement( XML_CONFIG_ITEM );
    m_rContext.Characters( OUString::number(nValue) );
    m_rContext.EndElement( false );
}

void XMLSettingsExportHelper::exportInt(const sal_Int32 nValue, const OUString& rName) const
{
    DBG_ASSERT(!rName.isEmpty(), "no name");
    m_rContext.AddAttribute( XML_NAME, rName );
    m_rContext.AddAttribute( XML_TYPE, XML_INT );
    m_rContext.StartElement( XML_CONFIG_ITEM );
    m_rContext.Characters( OUString::number(nValue) );
    m_rContext.EndElement( false );
}

void XMLSettingsExportHelper::exportLong(const sal_Int64 nValue, const OUString& rName) const
{
    DBG_ASSERT(!rName.isEmpty(), "no name");
    m_rContext.AddAttribute( XML_NAME, rName );
    m_rContext.AddAttribute( XML_TYPE, XML_LONG );
    m_rContext.StartElement( XML_CONFIG_ITEM );
    m_rContext.Characters( OUString::number(nValue) );
    m_rContext.EndElement( false );
}

void XMLSettingsExportHelper::exportDouble(const double fValue, const OUString& rName) const
{
    DBG_ASSERT(!rName.isEmpty(), "no name");
    m_rContext.AddAttribute( XML_NAME, rName );
    m_rContext.AddAttribute( XML_TYPE, XML_DOUBLE );
    m_rContext.StartElement( XML_CONFIG_ITEM );
    OUStringBuffer sBuffer;
    ::sax::Converter::convertDouble(sBuffer, fValue);
    m_rContext.Characters( sBuffer.makeStringAndClear() );
    m_rContext.EndElement( false );
}

void XMLSettingsExportHelper::exportString(const OUString& sValue, const OUString& rName) const
{
    DBG_ASSERT(!rName.isEmpty(), "no name");
    m_rContext.AddAttribute( XML_NAME, rName );
    m_rContext.AddAttribute( XML_TYPE, XML_STRING );
    m_rContext.StartElement( XML_CONFIG_ITEM );
    if (!sValue.isEmpty())
        m_rContext.Characters( sValue );
    m_rContext.EndElement( false );
}

void XMLSettingsExportHelper::exportDateTime(const util::DateTime& aValue, const OUString& rName) const
{
    DBG_ASSERT(!rName.isEmpty(), "no name");
    m_rContext.AddAttribute( XML_NAME, rName );
    m_rContext.AddAttribute( XML_TYPE, XML_DATETIME );
    OUStringBuffer sBuffer;
    ::sax::Converter::convertDateTime(sBuffer, aValue, nullptr);
    m_rContext.StartElement( XML_CONFIG_ITEM );
    m_rContext.Characters( sBuffer.makeStringAndClear() );
    m_rContext.EndElement( false );
}

void XMLSettingsExportHelper::exportSequencePropertyValue(
                    const uno::Sequence<beans::PropertyValue>& aProps,
                    const OUString& rName) const
{
    DBG_ASSERT(!rName.isEmpty(), "no name");
    if(aProps.hasElements())
    {
        m_rContext.AddAttribute( XML_NAME, rName );
        m_rContext.StartElement( XML_CONFIG_ITEM_SET );
        bool bSkipPrinterSettings = SvtSecurityOptions::IsOptionSet(
                                        SvtSecurityOptions::EOption::DocWarnRemovePersonalInfo)
                                    && !SvtSecurityOptions::IsOptionSet(
                                           SvtSecurityOptions::EOption::DocKeepPrinterSettings);
        for (const auto& rProp : aProps)
        {
            if (bSkipPrinterSettings
                && (rProp.Name == "PrinterSetup" || rProp.Name == "PrinterName"))
                continue;
            CallTypeFunction(rProp.Value, rProp.Name);
        }
        m_rContext.EndElement( true );
    }
}
void XMLSettingsExportHelper::exportSymbolDescriptors(
                    const uno::Sequence < formula::SymbolDescriptor > &rProps,
                    const OUString& rName) const
{
    rtl::Reference< comphelper::IndexedPropertyValuesContainer > xBox = new comphelper::IndexedPropertyValuesContainer();

    static constexpr OUStringLiteral sName     ( u"Name" );
    static constexpr OUStringLiteral sExportName ( u"ExportName" );
    static constexpr OUStringLiteral sSymbolSet ( u"SymbolSet" );
    static constexpr OUStringLiteral sCharacter ( u"Character" );
    static constexpr OUStringLiteral sFontName ( u"FontName" );
    static constexpr OUStringLiteral sCharSet  ( u"CharSet" );
    static constexpr OUStringLiteral sFamily   ( u"Family" );
    static constexpr OUStringLiteral sPitch    ( u"Pitch" );
    static constexpr OUStringLiteral sWeight   ( u"Weight" );
    static constexpr OUStringLiteral sItalic   ( u"Italic" );

    sal_Int32 nCount = rProps.getLength();
    const formula::SymbolDescriptor *pDescriptor = rProps.getConstArray();

    for( sal_Int32 nIndex = 0; nIndex < nCount; nIndex++, pDescriptor++ )
    {
        uno::Sequence < beans::PropertyValue > aSequence ( XML_SYMBOL_DESCRIPTOR_MAX );
        beans::PropertyValue *pSymbol = aSequence.getArray();

        pSymbol[XML_SYMBOL_DESCRIPTOR_NAME].Name         = sName;
        pSymbol[XML_SYMBOL_DESCRIPTOR_NAME].Value       <<= pDescriptor->sName;
        pSymbol[XML_SYMBOL_DESCRIPTOR_EXPORT_NAME].Name  = sExportName;
        pSymbol[XML_SYMBOL_DESCRIPTOR_EXPORT_NAME].Value<<= pDescriptor->sExportName;
        pSymbol[XML_SYMBOL_DESCRIPTOR_FONT_NAME].Name    = sFontName;
        pSymbol[XML_SYMBOL_DESCRIPTOR_FONT_NAME].Value  <<= pDescriptor->sFontName;
        pSymbol[XML_SYMBOL_DESCRIPTOR_CHAR_SET].Name      = sCharSet;
        pSymbol[XML_SYMBOL_DESCRIPTOR_CHAR_SET].Value   <<= pDescriptor->nCharSet;
        pSymbol[XML_SYMBOL_DESCRIPTOR_FAMILY].Name       = sFamily;
        pSymbol[XML_SYMBOL_DESCRIPTOR_FAMILY].Value <<= pDescriptor->nFamily;
        pSymbol[XML_SYMBOL_DESCRIPTOR_PITCH].Name        = sPitch;
        pSymbol[XML_SYMBOL_DESCRIPTOR_PITCH].Value      <<= pDescriptor->nPitch;
        pSymbol[XML_SYMBOL_DESCRIPTOR_WEIGHT].Name       = sWeight;
        pSymbol[XML_SYMBOL_DESCRIPTOR_WEIGHT].Value <<= pDescriptor->nWeight;
        pSymbol[XML_SYMBOL_DESCRIPTOR_ITALIC].Name       = sItalic;
        pSymbol[XML_SYMBOL_DESCRIPTOR_ITALIC].Value <<= pDescriptor->nItalic;
        pSymbol[XML_SYMBOL_DESCRIPTOR_SYMBOL_SET].Name       = sSymbolSet;
        pSymbol[XML_SYMBOL_DESCRIPTOR_SYMBOL_SET].Value <<= pDescriptor->sSymbolSet;
        pSymbol[XML_SYMBOL_DESCRIPTOR_CHARACTER].Name       = sCharacter;
        pSymbol[XML_SYMBOL_DESCRIPTOR_CHARACTER].Value  <<= pDescriptor->nCharacter;

        xBox->insertByIndex(nIndex, uno::Any( aSequence ));
    }

    exportIndexAccess( xBox, rName );
}
void XMLSettingsExportHelper::exportbase64Binary(
                    const uno::Sequence<sal_Int8>& aProps,
                    const OUString& rName) const
{
    DBG_ASSERT(!rName.isEmpty(), "no name");
    m_rContext.AddAttribute( XML_NAME, rName );
    m_rContext.AddAttribute( XML_TYPE, XML_BASE64BINARY );
    m_rContext.StartElement( XML_CONFIG_ITEM );
    if(aProps.hasElements())
    {
        OUStringBuffer sBuffer;
        ::comphelper::Base64::encode(sBuffer, aProps);
        m_rContext.Characters( sBuffer.makeStringAndClear() );
    }
    m_rContext.EndElement( false );
}

void XMLSettingsExportHelper::exportMapEntry(const uno::Any& rAny,
                                        const OUString& rName,
                                        const bool bNameAccess) const
{
    DBG_ASSERT((bNameAccess && !rName.isEmpty()) || !bNameAccess, "no name");
    uno::Sequence<beans::PropertyValue> aProps;
    rAny >>= aProps;
    if (aProps.hasElements())
    {
        if (bNameAccess)
            m_rContext.AddAttribute( XML_NAME, rName );
        m_rContext.StartElement( XML_CONFIG_ITEM_MAP_ENTRY );
        for (const auto& rProp : aProps)
            CallTypeFunction(rProp.Value, rProp.Name);
        m_rContext.EndElement( true );
    }
}

void XMLSettingsExportHelper::exportNameAccess(
                    const uno::Reference<container::XNameAccess>& aNamed,
                    const OUString& rName) const
{
    DBG_ASSERT(!rName.isEmpty(), "no name");
    DBG_ASSERT(aNamed->getElementType().equals(cppu::UnoType<uno::Sequence<beans::PropertyValue>>::get() ),
                "wrong NameAccess" );
    if(aNamed->hasElements())
    {
        m_rContext.AddAttribute( XML_NAME, rName );
        m_rContext.StartElement( XML_CONFIG_ITEM_MAP_NAMED );
        const uno::Sequence< OUString > aNames(aNamed->getElementNames());
        for (const auto& rElementName : aNames)
            exportMapEntry(aNamed->getByName(rElementName), rElementName, true);
        m_rContext.EndElement( true );
    }
}

void XMLSettingsExportHelper::exportIndexAccess(
                    const uno::Reference<container::XIndexAccess>& rIndexed,
                    const OUString& rName) const
{
    DBG_ASSERT(!rName.isEmpty(), "no name");
    DBG_ASSERT(rIndexed->getElementType().equals(cppu::UnoType<uno::Sequence<beans::PropertyValue>>::get() ),
                "wrong IndexAccess" );
    if (rIndexed->hasElements())
    {
        m_rContext.AddAttribute( XML_NAME, rName );
        m_rContext.StartElement( XML_CONFIG_ITEM_MAP_INDEXED );
        sal_Int32 nCount = rIndexed->getCount();
        for (sal_Int32 i = 0; i < nCount; i++)
        {
            exportMapEntry(rIndexed->getByIndex(i), u""_ustr, false);
        }
        m_rContext.EndElement( true );
    }
}

void XMLSettingsExportHelper::exportForbiddenCharacters(
                    const uno::Reference<i18n::XForbiddenCharacters>& xForbChars,
                    const OUString& rName) const
{
    uno::Reference<linguistic2::XSupportedLocales> xLocales(xForbChars, css::uno::UNO_QUERY);

    SAL_WARN_IF( !(xForbChars.is() && xLocales.is()), "xmloff","XMLSettingsExportHelper::exportForbiddenCharacters: got illegal forbidden characters!" );

    if( !xForbChars.is() || !xLocales.is() )
        return;

    rtl::Reference< comphelper::IndexedPropertyValuesContainer > xBox = new comphelper::IndexedPropertyValuesContainer();
    const uno::Sequence< lang::Locale > aLocales( xLocales->getLocales() );

    /* FIXME-BCP47: this stupid and counterpart in
     * xmloff/source/core/DocumentSettingsContext.cxx
     * XMLConfigItemMapIndexedContext::EndElement() */

    static constexpr OUStringLiteral sLanguage  ( u"Language" );
    static constexpr OUStringLiteral sCountry   ( u"Country" );
    static constexpr OUStringLiteral sVariant   ( u"Variant" );
    static constexpr OUStringLiteral sBeginLine ( u"BeginLine" );
    static constexpr OUStringLiteral sEndLine   ( u"EndLine" );

    sal_Int32 nPos = 0;
    for( const auto& rLocale : aLocales )
    {
        if( xForbChars->hasForbiddenCharacters( rLocale ) )
        {
            const i18n::ForbiddenCharacters aChars( xForbChars->getForbiddenCharacters( rLocale ) );


            uno::Sequence < beans::PropertyValue > aSequence ( XML_FORBIDDEN_CHARACTER_MAX );
            beans::PropertyValue *pForChar = aSequence.getArray();

            pForChar[XML_FORBIDDEN_CHARACTER_LANGUAGE].Name    = sLanguage;
            pForChar[XML_FORBIDDEN_CHARACTER_LANGUAGE].Value <<= rLocale.Language;
            pForChar[XML_FORBIDDEN_CHARACTER_COUNTRY].Name    = sCountry;
            pForChar[XML_FORBIDDEN_CHARACTER_COUNTRY].Value <<= rLocale.Country;
            pForChar[XML_FORBIDDEN_CHARACTER_VARIANT].Name    = sVariant;
            pForChar[XML_FORBIDDEN_CHARACTER_VARIANT].Value <<= rLocale.Variant;
            pForChar[XML_FORBIDDEN_CHARACTER_BEGIN_LINE].Name    = sBeginLine;
            pForChar[XML_FORBIDDEN_CHARACTER_BEGIN_LINE].Value <<= aChars.beginLine;
            pForChar[XML_FORBIDDEN_CHARACTER_END_LINE].Name    = sEndLine;
            pForChar[XML_FORBIDDEN_CHARACTER_END_LINE].Value <<= aChars.endLine;
            xBox->insertByIndex(nPos++, uno::Any( aSequence ));
        }
    }

    exportIndexAccess( xBox, rName );
}

void XMLSettingsExportHelper::exportAllSettings(
                    const uno::Sequence<beans::PropertyValue>& aProps,
                    const OUString& rName) const
{
    DBG_ASSERT(!rName.isEmpty(), "no name");
    exportSequencePropertyValue(aProps, rName);
}


/** For some settings we may want to change their API representation
 * from their XML settings representation. This is your chance to do
 * so!
 */
void XMLSettingsExportHelper::ManipulateSetting( uno::Any& rAny, std::u16string_view rName ) const
{
    if( rName == gsPrinterIndependentLayout )
    {
        sal_Int16 nTmp = sal_Int16();
        if( rAny >>= nTmp )
        {
            if( nTmp == document::PrinterIndependentLayout::LOW_RESOLUTION )
                rAny <<= u"low-resolution"_ustr;
            else if( nTmp == document::PrinterIndependentLayout::DISABLED )
                rAny <<= u"disabled"_ustr;
            else if( nTmp == document::PrinterIndependentLayout::HIGH_RESOLUTION )
                rAny <<= u"high-resolution"_ustr;
        }
    }
    else if( (rName == gsColorTableURL) || (rName == gsLineEndTableURL) || (rName == gsHatchTableURL) ||
             (rName == gsDashTableURL) || (rName == gsGradientTableURL) || (rName == gsBitmapTableURL ) )
    {
        if( !mxStringSubstitution.is() )
        {
            try
            {
                const_cast< XMLSettingsExportHelper* >(this)->mxStringSubstitution =
                    util::PathSubstitution::create( m_rContext.GetComponentContext() );
            }
            catch( uno::Exception& )
            {
                DBG_UNHANDLED_EXCEPTION("xmloff.core");
            }
        }

        if( mxStringSubstitution.is() )
        {
            OUString aURL;
            rAny >>= aURL;
            aURL = mxStringSubstitution->reSubstituteVariables( aURL );
            rAny <<= aURL;
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
