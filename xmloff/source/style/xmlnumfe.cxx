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

#include <comphelper/sequence.hxx>
#include <comphelper/string.hxx>
#include <svl/numformat.hxx>
#include <svl/zforlist.hxx>
#include <svl/zformat.hxx>
#include <svl/numuno.hxx>
#include <i18nlangtag/mslangid.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <tools/debug.hxx>
#include <rtl/math.hxx>
#include <unotools/calendarwrapper.hxx>
#include <unotools/charclass.hxx>
#include <com/sun/star/lang/Locale.hpp>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>
#include <osl/diagnose.h>
#include <tools/color.hxx>
#include <sax/tools/converter.hxx>

#include <com/sun/star/i18n/NativeNumberXmlAttributes2.hpp>

#include <utility>
#include <xmloff/xmlnumfe.hxx>
#include <xmloff/xmlnamespace.hxx>
#include <xmloff/xmlnumfi.hxx>

#include <svl/nfsymbol.hxx>
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmlexp.hxx>
#include <o3tl/string_view.hxx>

#include <float.h>
#include <set>
#include <string_view>
#include <vector>

using namespace ::com::sun::star;
using namespace ::xmloff::token;
using namespace ::svt;

typedef std::set< sal_uInt32 >  SvXMLuInt32Set;

namespace {

struct SvXMLEmbeddedTextEntry
{
    sal_uInt16      nSourcePos;     // position in NumberFormat (to skip later)
    sal_Int32       nFormatPos;     // resulting position in embedded-text element
    OUString   aText;
    bool            isBlankWidth;   // "_x"

    SvXMLEmbeddedTextEntry( sal_uInt16 nSP, sal_Int32 nFP, OUString aT, bool bBW = false ) :
        nSourcePos(nSP), nFormatPos(nFP), aText(std::move(aT)), isBlankWidth( bBW ) {}
};

}

class SvXMLEmbeddedTextEntryArr
{
    typedef std::vector<SvXMLEmbeddedTextEntry> DataType;
    DataType maData;

public:

    void push_back( SvXMLEmbeddedTextEntry const& r )
    {
        maData.push_back(r);
    }

    const SvXMLEmbeddedTextEntry& operator[] ( size_t i ) const
    {
        return maData[i];
    }

    size_t size() const
    {
        return maData.size();
    }
};

class SvXMLNumUsedList_Impl
{
    SvXMLuInt32Set              aUsed;
    SvXMLuInt32Set              aWasUsed;
    SvXMLuInt32Set::iterator    aCurrentUsedPos;
    sal_uInt32                  nUsedCount;
    sal_uInt32                  nWasUsedCount;

public:
            SvXMLNumUsedList_Impl();

    void    SetUsed( sal_uInt32 nKey );
    bool    IsUsed( sal_uInt32 nKey ) const;
    bool    IsWasUsed( sal_uInt32 nKey ) const;
    void    Export();

    bool    GetFirstUsed(sal_uInt32& nKey);
    bool    GetNextUsed(sal_uInt32& nKey);

    uno::Sequence<sal_Int32> GetWasUsed() const;
    void SetWasUsed(const uno::Sequence<sal_Int32>& rWasUsed);
};

//! SvXMLNumUsedList_Impl should be optimized!

SvXMLNumUsedList_Impl::SvXMLNumUsedList_Impl() :
    nUsedCount(0),
    nWasUsedCount(0)
{
}

void SvXMLNumUsedList_Impl::SetUsed( sal_uInt32 nKey )
{
    if ( !IsWasUsed(nKey) )
    {
        std::pair<SvXMLuInt32Set::iterator, bool> aPair = aUsed.insert( nKey );
        if (aPair.second)
            nUsedCount++;
    }
}

bool SvXMLNumUsedList_Impl::IsUsed( sal_uInt32 nKey ) const
{
    SvXMLuInt32Set::const_iterator aItr = aUsed.find(nKey);
    return (aItr != aUsed.end());
}

bool SvXMLNumUsedList_Impl::IsWasUsed( sal_uInt32 nKey ) const
{
    SvXMLuInt32Set::const_iterator aItr = aWasUsed.find(nKey);
    return (aItr != aWasUsed.end());
}

void SvXMLNumUsedList_Impl::Export()
{
    SvXMLuInt32Set::const_iterator aItr = aUsed.begin();
    while (aItr != aUsed.end())
    {
        std::pair<SvXMLuInt32Set::const_iterator, bool> aPair = aWasUsed.insert( *aItr );
        if (aPair.second)
            nWasUsedCount++;
        ++aItr;
    }
    aUsed.clear();
    nUsedCount = 0;
}

bool SvXMLNumUsedList_Impl::GetFirstUsed(sal_uInt32& nKey)
{
    bool bRet(false);
    aCurrentUsedPos = aUsed.begin();
    if(nUsedCount)
    {
        DBG_ASSERT(aCurrentUsedPos != aUsed.end(), "something went wrong");
        nKey = *aCurrentUsedPos;
        bRet = true;
    }
    return bRet;
}

bool SvXMLNumUsedList_Impl::GetNextUsed(sal_uInt32& nKey)
{
    bool bRet(false);
    if (aCurrentUsedPos != aUsed.end())
    {
        ++aCurrentUsedPos;
        if (aCurrentUsedPos != aUsed.end())
        {
            nKey = *aCurrentUsedPos;
            bRet = true;
        }
    }
    return bRet;
}

uno::Sequence<sal_Int32> SvXMLNumUsedList_Impl::GetWasUsed() const
{
    return comphelper::containerToSequence<sal_Int32>(aWasUsed);
}

void SvXMLNumUsedList_Impl::SetWasUsed(const uno::Sequence<sal_Int32>& rWasUsed)
{
    DBG_ASSERT(nWasUsedCount == 0, "WasUsed should be empty");
    for (const auto nWasUsed : rWasUsed)
    {
        std::pair<SvXMLuInt32Set::const_iterator, bool> aPair = aWasUsed.insert( nWasUsed );
        if (aPair.second)
            nWasUsedCount++;
    }
}

SvXMLNumFmtExport::SvXMLNumFmtExport(
            SvXMLExport& rExp,
            const uno::Reference< util::XNumberFormatsSupplier >& rSupp ) :
    m_rExport( rExp ),
    m_sPrefix( u"N"_ustr ),
    m_pFormatter( nullptr ),
    m_bHasText( false )
{
    //  supplier must be SvNumberFormatsSupplierObj
    SvNumberFormatsSupplierObj* pObj =
                    comphelper::getFromUnoTunnel<SvNumberFormatsSupplierObj>( rSupp );
    if (pObj)
        m_pFormatter = pObj->GetNumberFormatter();

    if ( m_pFormatter )
    {
        m_pLocaleData = LocaleDataWrapper::get( m_pFormatter->GetLanguageTag() );
    }
    else
    {
        LanguageTag aLanguageTag( MsLangId::getConfiguredSystemLanguage() );

        m_pLocaleData = LocaleDataWrapper::get( std::move(aLanguageTag) );
    }

    m_pUsedList.reset(new SvXMLNumUsedList_Impl);
}

SvXMLNumFmtExport::SvXMLNumFmtExport(
                       SvXMLExport& rExp,
                       const css::uno::Reference< css::util::XNumberFormatsSupplier >& rSupp,
                       OUString aPrefix ) :
    m_rExport( rExp ),
    m_sPrefix(std::move( aPrefix )),
    m_pFormatter( nullptr ),
    m_bHasText( false )
{
    //  supplier must be SvNumberFormatsSupplierObj
    SvNumberFormatsSupplierObj* pObj =
                    comphelper::getFromUnoTunnel<SvNumberFormatsSupplierObj>( rSupp );
    if (pObj)
        m_pFormatter = pObj->GetNumberFormatter();

    if ( m_pFormatter )
    {
        m_pLocaleData = LocaleDataWrapper::get( m_pFormatter->GetLanguageTag() );
    }
    else
    {
        LanguageTag aLanguageTag( MsLangId::getConfiguredSystemLanguage() );

        m_pLocaleData = LocaleDataWrapper::get( std::move(aLanguageTag) );
    }

    m_pUsedList.reset(new SvXMLNumUsedList_Impl);
}

SvXMLNumFmtExport::~SvXMLNumFmtExport()
{
}

//  helper methods

static OUString lcl_CreateStyleName( sal_Int32 nKey, sal_Int32 nPart, bool bDefPart, std::u16string_view rPrefix )
{
    if (bDefPart)
        return rPrefix + OUString::number(nKey);
    else
        return rPrefix + OUString::number(nKey) + "P" + OUString::number( nPart );
}

void SvXMLNumFmtExport::AddCalendarAttr_Impl( const OUString& rCalendar )
{
    if ( !rCalendar.isEmpty() )
    {
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_CALENDAR, rCalendar );
    }
}

void SvXMLNumFmtExport::AddStyleAttr_Impl( bool bLong )
{
    if ( bLong )            // short is default
    {
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_STYLE, XML_LONG );
    }
}

void SvXMLNumFmtExport::AddLanguageAttr_Impl( LanguageType nLang )
{
    if ( nLang != LANGUAGE_SYSTEM )
    {
        m_rExport.AddLanguageTagAttributes( XML_NAMESPACE_NUMBER, XML_NAMESPACE_NUMBER,
                LanguageTag( nLang), false);
    }
}

//  methods to write individual elements within a format

void SvXMLNumFmtExport::AddToTextElement_Impl( std::u16string_view rString )
{
    //  append to sTextContent, write element in FinishTextElement_Impl
    //  to avoid several text elements following each other

    m_sTextContent.append( rString );
    // Also empty string leads to a number:text element as it may separate
    // keywords of the same letter (e.g. MM""MMM) that otherwise would be
    // concatenated when reading back in.
    m_bHasText = true;
}

void SvXMLNumFmtExport::FinishTextElement_Impl(bool bUseExtensionNS)
{
    if ( m_bHasText )
    {
        if ( !m_sBlankWidthString.isEmpty() )
        {
            // Export only for 1.3 with extensions and later.
            SvtSaveOptions::ODFSaneDefaultVersion eVersion = m_rExport.getSaneDefaultVersion();
            if (eVersion > SvtSaveOptions::ODFSVER_013 && ( (eVersion & SvtSaveOptions::ODFSVER_EXTENDED) != 0 ))
            {
                m_rExport.AddAttribute( XML_NAMESPACE_LO_EXT, XML_BLANK_WIDTH_CHAR,
                                      m_sBlankWidthString.makeStringAndClear() );
            }
        }
        sal_uInt16 nNS = bUseExtensionNS ? XML_NAMESPACE_LO_EXT : XML_NAMESPACE_NUMBER;
        SvXMLElementExport aElem( m_rExport, nNS, XML_TEXT,
                                  true, false );
        m_rExport.Characters( m_sTextContent.makeStringAndClear() );
        m_bHasText = false;
    }
}

void SvXMLNumFmtExport::WriteColorElement_Impl( const Color& rColor )
{
    FinishTextElement_Impl();

    OUStringBuffer aColStr( 7 );
    ::sax::Converter::convertColor( aColStr, rColor );
    m_rExport.AddAttribute( XML_NAMESPACE_FO, XML_COLOR,
                          aColStr.makeStringAndClear() );

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_STYLE, XML_TEXT_PROPERTIES,
                              true, false );
}

void SvXMLNumFmtExport::WriteCurrencyElement_Impl( const OUString& rString,
                                                    std::u16string_view rExt )
{
    FinishTextElement_Impl();

    if ( !rExt.empty() )
    {
        // rExt should be a 16-bit hex value max FFFF which may contain a
        // leading "-" separator (that is not a minus sign, but toInt32 can be
        // used to parse it, with post-processing as necessary):
        sal_Int32 nLang = o3tl::toInt32(rExt, 16);
        if ( nLang < 0 )
            nLang = -nLang;
        SAL_WARN_IF(nLang > 0xFFFF, "xmloff.style", "Out of range Lang Id: " << nLang << " from input string: " << OUString(rExt));
        AddLanguageAttr_Impl( LanguageType(nLang & 0xFFFF) );          // adds to pAttrList
    }

    SvXMLElementExport aElem( m_rExport,
                              XML_NAMESPACE_NUMBER, XML_CURRENCY_SYMBOL,
                              true, false );
    m_rExport.Characters( rString );
}

void SvXMLNumFmtExport::WriteBooleanElement_Impl()
{
    FinishTextElement_Impl();

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_NUMBER, XML_BOOLEAN,
                              true, false );
}

void SvXMLNumFmtExport::WriteTextContentElement_Impl()
{
    FinishTextElement_Impl();

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_NUMBER, XML_TEXT_CONTENT,
                              true, false );
}

//  date elements

void SvXMLNumFmtExport::WriteDayElement_Impl( const OUString& rCalendar, bool bLong )
{
    FinishTextElement_Impl();

    AddCalendarAttr_Impl( rCalendar ); // adds to pAttrList
    AddStyleAttr_Impl( bLong );     // adds to pAttrList

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_NUMBER, XML_DAY,
                              true, false );
}

void SvXMLNumFmtExport::WriteMonthElement_Impl( const OUString& rCalendar, bool bLong, bool bText )
{
    FinishTextElement_Impl();

    AddCalendarAttr_Impl( rCalendar ); // adds to pAttrList
    AddStyleAttr_Impl( bLong );     // adds to pAttrList
    if ( bText )
    {
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_TEXTUAL, XML_TRUE );
    }

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_NUMBER, XML_MONTH,
                              true, false );
}

void SvXMLNumFmtExport::WriteYearElement_Impl( const OUString& rCalendar, bool bLong )
{
    FinishTextElement_Impl();

    AddCalendarAttr_Impl( rCalendar ); // adds to pAttrList
    AddStyleAttr_Impl( bLong );     // adds to pAttrList

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_NUMBER, XML_YEAR,
                              true, false );
}

void SvXMLNumFmtExport::WriteEraElement_Impl( const OUString& rCalendar, bool bLong )
{
    FinishTextElement_Impl();

    AddCalendarAttr_Impl( rCalendar ); // adds to pAttrList
    AddStyleAttr_Impl( bLong );     // adds to pAttrList

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_NUMBER, XML_ERA,
                              true, false );
}

void SvXMLNumFmtExport::WriteDayOfWeekElement_Impl( const OUString& rCalendar, bool bLong )
{
    FinishTextElement_Impl();

    AddCalendarAttr_Impl( rCalendar ); // adds to pAttrList
    AddStyleAttr_Impl( bLong );     // adds to pAttrList

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_NUMBER, XML_DAY_OF_WEEK,
                              true, false );
}

void SvXMLNumFmtExport::WriteWeekElement_Impl( const OUString& rCalendar )
{
    FinishTextElement_Impl();

    AddCalendarAttr_Impl( rCalendar ); // adds to pAttrList

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_NUMBER, XML_WEEK_OF_YEAR,
                              true, false );
}

void SvXMLNumFmtExport::WriteQuarterElement_Impl( const OUString& rCalendar, bool bLong )
{
    FinishTextElement_Impl();

    AddCalendarAttr_Impl( rCalendar ); // adds to pAttrList
    AddStyleAttr_Impl( bLong );     // adds to pAttrList

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_NUMBER, XML_QUARTER,
                              true, false );
}

//  time elements

void SvXMLNumFmtExport::WriteHoursElement_Impl( bool bLong )
{
    FinishTextElement_Impl();

    AddStyleAttr_Impl( bLong );     // adds to pAttrList

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_NUMBER, XML_HOURS,
                              true, false );
}

void SvXMLNumFmtExport::WriteMinutesElement_Impl( bool bLong )
{
    FinishTextElement_Impl();

    AddStyleAttr_Impl( bLong );     // adds to pAttrList

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_NUMBER, XML_MINUTES,
                              true, false );
}

void SvXMLNumFmtExport::WriteRepeatedElement_Impl( sal_Unicode nChar )
{
    // Export only for 1.2 with extensions or 1.3 and later.
    SvtSaveOptions::ODFSaneDefaultVersion eVersion = m_rExport.getSaneDefaultVersion();
    if (eVersion > SvtSaveOptions::ODFSVER_012)
    {
        FinishTextElement_Impl(eVersion < SvtSaveOptions::ODFSVER_013);
        // OFFICE-3765 For 1.2+ use loext namespace, for 1.3 use number namespace.
        SvXMLElementExport aElem( m_rExport,
                                  ((eVersion < SvtSaveOptions::ODFSVER_013) ? XML_NAMESPACE_LO_EXT : XML_NAMESPACE_NUMBER),
                                  XML_FILL_CHARACTER, true, false );
        m_rExport.Characters( OUString( nChar ) );
    }
}

namespace {
void lcl_WriteBlankWidthString( std::u16string_view rBlankWidthChar, OUStringBuffer& rBlankWidthString, OUStringBuffer& rTextContent )
{
    // export "_x"
    if ( rBlankWidthString.isEmpty() )
    {
        rBlankWidthString.append( rBlankWidthChar );
        if ( !rTextContent.isEmpty() )
        {
            // add position in rTextContent
            rBlankWidthString.append( rTextContent.getLength() );
        }
    }
    else
    {
        // add "_" as separator if there are several blank width char
        rBlankWidthString.append( "_" );
        rBlankWidthString.append( rBlankWidthChar );
        rBlankWidthString.append( rTextContent.getLength() );
    }
    // for previous versions, turn "_x" into the number of spaces used for x in InsertBlanks in the NumberFormat
    if ( !rBlankWidthChar.empty() )
    {
        OUString aBlanks;
        SvNumberformat::InsertBlanks( aBlanks, 0, rBlankWidthChar[0] );
        rTextContent.append( aBlanks );
    }
}
}

void SvXMLNumFmtExport::WriteSecondsElement_Impl( bool bLong, sal_uInt16 nDecimals )
{
    FinishTextElement_Impl();

    AddStyleAttr_Impl( bLong );     // adds to pAttrList
    if ( nDecimals > 0 )
    {
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_DECIMAL_PLACES,
                              OUString::number(  nDecimals ) );
    }

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_NUMBER, XML_SECONDS,
                              true, false );
}

void SvXMLNumFmtExport::WriteAMPMElement_Impl()
{
    FinishTextElement_Impl();

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_NUMBER, XML_AM_PM,
                              true, false );
}

//  numbers

void SvXMLNumFmtExport::WriteIntegerElement_Impl(
                            sal_Int32 nInteger, sal_Int32 nBlankInteger, bool bGrouping )
{
    //  integer digits: '0' and '?'
    if ( nInteger >= 0 )    // negative = automatic
    {
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_MIN_INTEGER_DIGITS,
                              OUString::number( nInteger ) );
    }
    SvtSaveOptions::ODFSaneDefaultVersion eVersion = m_rExport.getSaneDefaultVersion();
    //  blank integer digits: '?'
    if ( nBlankInteger > 0 && ( (eVersion & SvtSaveOptions::ODFSVER_EXTENDED) != 0 ) )
    {
        m_rExport.AddAttribute( XML_NAMESPACE_LO_EXT, XML_MAX_BLANK_INTEGER_DIGITS,
                              OUString::number( nBlankInteger ) );
    }
    //  (automatic) grouping separator
    if ( bGrouping )
    {
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_GROUPING, XML_TRUE );
    }
}

void SvXMLNumFmtExport::WriteEmbeddedEntries_Impl( const SvXMLEmbeddedTextEntryArr& rEmbeddedEntries )
{
    auto nEntryCount = rEmbeddedEntries.size();
    SvtSaveOptions::ODFSaneDefaultVersion eVersion = m_rExport.getSaneDefaultVersion();
    for (decltype(nEntryCount) nEntry=0; nEntry < nEntryCount; ++nEntry)
    {
        const SvXMLEmbeddedTextEntry* pObj = &rEmbeddedEntries[nEntry];

        //  position attribute
        // position == 0 is between first integer digit and decimal separator
        // position < 0 is inside decimal part
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_POSITION,
                                OUString::number( pObj->nFormatPos ) );

        //  text as element content
        OUStringBuffer aContent;
        OUStringBuffer aBlankWidthString;
        do
        {
            pObj = &rEmbeddedEntries[nEntry];
            if ( pObj->isBlankWidth  )
            {
                //  (#i20396# the spaces may also be in embedded-text elements)
                lcl_WriteBlankWidthString( pObj->aText, aBlankWidthString, aContent );
            }
            else
            {
                // The array can contain several elements for the same position in the number.
                // Literal texts are merged into a single embedded-text element.
                aContent.append( pObj->aText );
            }
            ++nEntry;
        }
        while ( nEntry < nEntryCount
            && rEmbeddedEntries[nEntry].nFormatPos == pObj->nFormatPos );
        --nEntry;

        // Export only for 1.3 with extensions and later.
        if ( !aBlankWidthString.isEmpty() && eVersion > SvtSaveOptions::ODFSVER_013 && ( (eVersion & SvtSaveOptions::ODFSVER_EXTENDED) != 0 ) )
            m_rExport.AddAttribute( XML_NAMESPACE_LO_EXT, XML_BLANK_WIDTH_CHAR, aBlankWidthString.makeStringAndClear() );
        SvXMLElementExport aChildElem( m_rExport, XML_NAMESPACE_NUMBER, XML_EMBEDDED_TEXT,
                                          true, false );
        m_rExport.Characters( aContent.makeStringAndClear() );
    }
}

void SvXMLNumFmtExport::WriteNumberElement_Impl(
                            sal_Int32 nDecimals, sal_Int32 nMinDecimals,
                            sal_Int32 nInteger, sal_Int32 nBlankInteger, const OUString& rDashStr,
                            bool bGrouping, sal_Int32 nTrailingThousands,
                            const SvXMLEmbeddedTextEntryArr& rEmbeddedEntries )
{
    FinishTextElement_Impl();

    //  decimals
    if ( nDecimals >= 0 )   // negative = automatic
    {
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_DECIMAL_PLACES,
                              OUString::number( nDecimals ) );
    }

    if ( nMinDecimals >= 0 )   // negative = automatic
    {
        // Export only for 1.2 with extensions or 1.3 and later.
        SvtSaveOptions::ODFSaneDefaultVersion eVersion = m_rExport.getSaneDefaultVersion();
        if (eVersion > SvtSaveOptions::ODFSVER_012)
        {
            // OFFICE-3860 For 1.2+ use loext namespace, for 1.3 use number namespace.
            m_rExport.AddAttribute(
                ((eVersion < SvtSaveOptions::ODFSVER_013) ? XML_NAMESPACE_LO_EXT : XML_NAMESPACE_NUMBER),
                                 XML_MIN_DECIMAL_PLACES,
                                 OUString::number( nMinDecimals ) );
        }
    }
    //  decimal replacement (dashes) or variable decimals (#)
    if ( !rDashStr.isEmpty() ||  nMinDecimals < nDecimals )
    {
        // full variable decimals means an empty replacement string
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_DECIMAL_REPLACEMENT,
                              rDashStr );
    }

    WriteIntegerElement_Impl( nInteger, nBlankInteger, bGrouping );

    //  display-factor if there are trailing thousands separators
    if ( nTrailingThousands )
    {
        //  each separator character removes three digits
        double fFactor = ::rtl::math::pow10Exp( 1.0, 3 * nTrailingThousands );

        OUStringBuffer aFactStr;
        ::sax::Converter::convertDouble( aFactStr, fFactor );
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_DISPLAY_FACTOR, aFactStr.makeStringAndClear() );
    }

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_NUMBER, XML_NUMBER,
                              true, true );

    //  number:embedded-text as child elements
    WriteEmbeddedEntries_Impl( rEmbeddedEntries );
}

void SvXMLNumFmtExport::WriteScientificElement_Impl(
                            sal_Int32 nDecimals, sal_Int32 nMinDecimals, sal_Int32 nInteger, sal_Int32 nBlankInteger,
                            bool bGrouping, sal_Int32 nExp, sal_Int32 nExpInterval, bool bExpSign, bool bExponentLowercase, sal_Int32 nBlankExp,
                            const SvXMLEmbeddedTextEntryArr& rEmbeddedEntries )
{
    FinishTextElement_Impl();

    //  decimals
    if ( nDecimals >= 0 )   // negative = automatic
    {
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_DECIMAL_PLACES,
                              OUString::number( nDecimals ) );
    }

    SvtSaveOptions::ODFSaneDefaultVersion eVersion = m_rExport.getSaneDefaultVersion();
    if ( nMinDecimals >= 0 )   // negative = automatic
    {
        // Export only for 1.2 with extensions or 1.3 and later.
        if (eVersion > SvtSaveOptions::ODFSVER_012)
        {
            // OFFICE-3860 For 1.2+ use loext namespace, for 1.3 use number namespace.
            m_rExport.AddAttribute(
                ((eVersion < SvtSaveOptions::ODFSVER_013) ? XML_NAMESPACE_LO_EXT : XML_NAMESPACE_NUMBER),
                                 XML_MIN_DECIMAL_PLACES,
                                 OUString::number( nMinDecimals ) );
        }
    }

    WriteIntegerElement_Impl( nInteger, nBlankInteger, bGrouping );

    //  exponent digits
    if ( nExp >= 0 )
    {
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_MIN_EXPONENT_DIGITS,
                              OUString::number( nExp ) );
    }

    //  exponent interval for engineering notation
    if ( nExpInterval >= 0 )
    {
        // Export only for 1.2 with extensions or 1.3 and later.
        if (eVersion > SvtSaveOptions::ODFSVER_012)
        {
            // OFFICE-1828 For 1.2+ use loext namespace, for 1.3 use number namespace.
            m_rExport.AddAttribute(
                    ((eVersion < SvtSaveOptions::ODFSVER_013) ? XML_NAMESPACE_LO_EXT : XML_NAMESPACE_NUMBER),
                    XML_EXPONENT_INTERVAL, OUString::number( nExpInterval ) );
        }
    }

    //  exponent sign
    // Export only for 1.2 with extensions or 1.3 and later.
    if (eVersion > SvtSaveOptions::ODFSVER_012)
    {
        // OFFICE-3860 For 1.2+ use loext namespace, for 1.3 use number namespace.
        m_rExport.AddAttribute(
            ((eVersion < SvtSaveOptions::ODFSVER_013) ? XML_NAMESPACE_LO_EXT : XML_NAMESPACE_NUMBER),
                             XML_FORCED_EXPONENT_SIGN,
                             bExpSign? XML_TRUE : XML_FALSE );
    }
    //  exponent string
    // Export only for 1.x with extensions
    if (eVersion & SvtSaveOptions::ODFSVER_EXTENDED)
    {
        if (bExponentLowercase)
            m_rExport.AddAttribute( XML_NAMESPACE_LO_EXT, XML_EXPONENT_LOWERCASE, XML_TRUE );
        if (nBlankExp > 0)
        {
            if (nBlankExp >= nExp)
                nBlankExp = nExp - 1; // preserve at least one '0' in exponent
            m_rExport.AddAttribute( XML_NAMESPACE_LO_EXT, XML_BLANK_EXPONENT_DIGITS, OUString::number( nBlankExp ) );
        }
    }

    SvXMLElementExport aElem( m_rExport,
                              XML_NAMESPACE_NUMBER, XML_SCIENTIFIC_NUMBER,
                              true, false );

    //  number:embedded-text as child elements
    // Export only for 1.x with extensions
    if (eVersion & SvtSaveOptions::ODFSVER_EXTENDED)
        WriteEmbeddedEntries_Impl( rEmbeddedEntries );
}

void SvXMLNumFmtExport::WriteFractionElement_Impl(
                            sal_Int32 nInteger, sal_Int32 nBlankInteger, bool bGrouping,
                            const SvNumberformat& rFormat, sal_uInt16 nPart )
{
    FinishTextElement_Impl();
    WriteIntegerElement_Impl( nInteger, nBlankInteger, bGrouping );

    const OUString aNumeratorString = rFormat.GetNumeratorString( nPart );
    const OUString aDenominatorString = rFormat.GetDenominatorString( nPart );
    const OUString aIntegerFractionDelimiterString = rFormat.GetIntegerFractionDelimiterString( nPart );
    sal_Int32 nMaxNumeratorDigits = aNumeratorString.getLength();
    // Count '0' as '?'
    sal_Int32 nMinNumeratorDigits = aNumeratorString.replaceAll("0","?").indexOf('?');
    sal_Int32 nZerosNumeratorDigits = aNumeratorString.indexOf('0');
    if ( nMinNumeratorDigits >= 0 )
        nMinNumeratorDigits = nMaxNumeratorDigits - nMinNumeratorDigits;
    else
        nMinNumeratorDigits = 0;
    if ( nZerosNumeratorDigits >= 0 )
        nZerosNumeratorDigits = nMaxNumeratorDigits - nZerosNumeratorDigits;
    else
        nZerosNumeratorDigits = 0;
    sal_Int32 nMaxDenominatorDigits = aDenominatorString.getLength();
    sal_Int32 nMinDenominatorDigits = aDenominatorString.replaceAll("0","?").indexOf('?');
    sal_Int32 nZerosDenominatorDigits = aDenominatorString.indexOf('0');
    if ( nMinDenominatorDigits >= 0 )
        nMinDenominatorDigits = nMaxDenominatorDigits - nMinDenominatorDigits;
    else
        nMinDenominatorDigits = 0;
    if ( nZerosDenominatorDigits >= 0 )
        nZerosDenominatorDigits = nMaxDenominatorDigits - nZerosDenominatorDigits;
    else
        nZerosDenominatorDigits = 0;
    sal_Int32 nDenominator = aDenominatorString.toInt32();

    SvtSaveOptions::ODFSaneDefaultVersion eVersion = m_rExport.getSaneDefaultVersion();

    // integer/fraction delimiter
    if ( !aIntegerFractionDelimiterString.isEmpty() && aIntegerFractionDelimiterString != " "
        && ((eVersion & SvtSaveOptions::ODFSVER_EXTENDED) != 0) )
    {   // Export only for 1.2/1.3 with extensions.
        m_rExport.AddAttribute( XML_NAMESPACE_LO_EXT, XML_INTEGER_FRACTION_DELIMITER,
                              aIntegerFractionDelimiterString );
    }

    //  numerator digits
    if ( nMinNumeratorDigits == 0 ) // at least one digit to keep compatibility with previous versions
        nMinNumeratorDigits++;
    m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_MIN_NUMERATOR_DIGITS,
                          OUString::number( nMinNumeratorDigits ) );
    // Export only for 1.2/1.3 with extensions.
    if ((eVersion & SvtSaveOptions::ODFSVER_EXTENDED) != 0)
    {
        // For extended ODF use loext namespace
        m_rExport.AddAttribute( XML_NAMESPACE_LO_EXT, XML_MAX_NUMERATOR_DIGITS,
                              OUString::number( nMaxNumeratorDigits ) );
    }
    if ( nZerosNumeratorDigits && ((eVersion & SvtSaveOptions::ODFSVER_EXTENDED) != 0) )
        m_rExport.AddAttribute( XML_NAMESPACE_LO_EXT, XML_ZEROS_NUMERATOR_DIGITS,
                              OUString::number( nZerosNumeratorDigits ) );

    if ( nDenominator )
    {
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_DENOMINATOR_VALUE,
                              OUString::number( nDenominator) );
    }
    //  it's not necessary to export nDenominatorDigits
    //  if we have a forced denominator
    else
    {
        if ( nMinDenominatorDigits == 0 ) // at least one digit to keep compatibility with previous versions
            nMinDenominatorDigits++;
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_MIN_DENOMINATOR_DIGITS,
                              OUString::number( nMinDenominatorDigits ) );
        if (eVersion > SvtSaveOptions::ODFSVER_012)
        {
            // OFFICE-3695 For 1.2+ use loext namespace, for 1.3 use number namespace.
            m_rExport.AddAttribute(
                ((eVersion < SvtSaveOptions::ODFSVER_013) ? XML_NAMESPACE_LO_EXT : XML_NAMESPACE_NUMBER),
                                 XML_MAX_DENOMINATOR_VALUE,
                                 OUString::number( pow ( 10.0, nMaxDenominatorDigits ) - 1 ) ); // 9, 99 or 999
        }
        if ( nZerosDenominatorDigits && ((eVersion & SvtSaveOptions::ODFSVER_EXTENDED) != 0) )
            m_rExport.AddAttribute( XML_NAMESPACE_LO_EXT, XML_ZEROS_DENOMINATOR_DIGITS,
                                  OUString::number( nZerosDenominatorDigits ) );
    }

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_NUMBER, XML_FRACTION,
                              true, false );
}

//  mapping (condition)

void SvXMLNumFmtExport::WriteMapElement_Impl( sal_Int32 nOp, double fLimit,
                                                sal_Int32 nKey, sal_Int32 nPart )
{
    FinishTextElement_Impl();

    if ( nOp == NUMBERFORMAT_OP_NO )
        return;

    // style namespace

    OUStringBuffer aCondStr(20);
    aCondStr.append( "value()" );          //! define constant
    switch ( nOp )
    {
        case NUMBERFORMAT_OP_EQ: aCondStr.append( '=' );  break;
        case NUMBERFORMAT_OP_NE: aCondStr.append( "!=" );          break;
        case NUMBERFORMAT_OP_LT: aCondStr.append( '<' );  break;
        case NUMBERFORMAT_OP_LE: aCondStr.append( "<=" );          break;
        case NUMBERFORMAT_OP_GT: aCondStr.append( '>' );  break;
        case NUMBERFORMAT_OP_GE: aCondStr.append( ">=" );          break;
        default:
            OSL_FAIL("unknown operator");
    }
    ::rtl::math::doubleToUStringBuffer( aCondStr, fLimit,
            rtl_math_StringFormat_Automatic, rtl_math_DecimalPlaces_Max,
            '.', true );

    m_rExport.AddAttribute( XML_NAMESPACE_STYLE, XML_CONDITION,
                          aCondStr.makeStringAndClear() );

    m_rExport.AddAttribute( XML_NAMESPACE_STYLE, XML_APPLY_STYLE_NAME,
                          m_rExport.EncodeStyleName( lcl_CreateStyleName( nKey, nPart, false,
                                               m_sPrefix ) ) );

    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_STYLE, XML_MAP,
                              true, false );
}

//  for old (automatic) currency formats: parse currency symbol from text

static sal_Int32 lcl_FindSymbol( const OUString& sUpperStr, std::u16string_view sCurString )
{
    //  search for currency symbol
    //  Quoting as in ImpSvNumberformatScan::Symbol_Division

    sal_Int32 nCPos = 0;
    while (nCPos >= 0)
    {
        nCPos = sUpperStr.indexOf( sCurString, nCPos );
        if (nCPos >= 0)
        {
            // in Quotes?
            sal_Int32 nQ = SvNumberformat::GetQuoteEnd( sUpperStr, nCPos );
            if ( nQ < 0 )
            {
                //  dm can be escaped as "dm or \d
                sal_Unicode c;
                if ( nCPos == 0 )
                    return nCPos;                   // found
                c = sUpperStr[nCPos-1];
                if ( c != '"' && c != '\\')
                {
                    return nCPos;                   // found
                }
                else
                {
                    nCPos++;                        // continue
                }
            }
            else
            {
                nCPos = nQ + 1;                     // continue after quote end
            }
        }
    }
    return -1;
}

bool SvXMLNumFmtExport::WriteTextWithCurrency_Impl( const OUString& rString,
                            const css::lang::Locale& rLocale )
{
    //  returns true if currency element was written

    bool bRet = false;

    LanguageTag aLanguageTag( rLocale );
    m_pFormatter->ChangeIntl( aLanguageTag.getLanguageType( false) );
    OUString sCurString, sDummy;
    m_pFormatter->GetCompatibilityCurrency( sCurString, sDummy );

    OUString sUpperStr = m_pFormatter->GetCharClass()->uppercase(rString);
    sal_Int32 nPos = lcl_FindSymbol( sUpperStr, sCurString );
    if ( nPos >= 0 )
    {
        sal_Int32 nLength = rString.getLength();
        sal_Int32 nCurLen = sCurString.getLength();
        sal_Int32 nCont = nPos + nCurLen;

        //  text before currency symbol
        if ( nPos > 0 )
        {
            AddToTextElement_Impl( rString.subView( 0, nPos ) );
        }
        //  currency symbol (empty string -> default)
        WriteCurrencyElement_Impl( u""_ustr, u"" );
        bRet = true;

        //  text after currency symbol
        if ( nCont < nLength )
        {
            AddToTextElement_Impl( rString.subView( nCont, nLength-nCont ) );
        }
    }
    else
    {
        AddToTextElement_Impl( rString );       // simple text
    }

    return bRet;        // true: currency element written
}

static OUString lcl_GetDefaultCalendar( SvNumberFormatter const * pFormatter, LanguageType nLang )
{
    //  get name of first non-gregorian calendar for the language

    OUString aCalendar;
    CalendarWrapper* pCalendar = pFormatter->GetCalendar();
    if (pCalendar)
    {
        lang::Locale aLocale( LanguageTag::convertToLocale( nLang ) );

        const uno::Sequence<OUString> aCals = pCalendar->getAllCalendars( aLocale );
        auto pCal = std::find_if(aCals.begin(), aCals.end(),
            [](const OUString& rCal) { return rCal != "gregorian"; });
        if (pCal != aCals.end())
            aCalendar = *pCal;
    }
    return aCalendar;
}

static bool lcl_IsInEmbedded( const SvXMLEmbeddedTextEntryArr& rEmbeddedEntries, sal_uInt16 nPos )
{
    auto nCount = rEmbeddedEntries.size();
    for (decltype(nCount) i=0; i<nCount; i++)
        if ( rEmbeddedEntries[i].nSourcePos == nPos )
            return true;

    return false;       // not found
}

static bool lcl_IsDefaultDateFormat( const SvNumberformat& rFormat, bool bSystemDate, NfIndexTableOffset eBuiltIn )
{
    //  make an extra loop to collect date elements, to check if it is a default format
    //  before adding the automatic-order attribute

    SvXMLDateElementAttributes eDateDOW = XML_DEA_NONE;
    SvXMLDateElementAttributes eDateDay = XML_DEA_NONE;
    SvXMLDateElementAttributes eDateMonth = XML_DEA_NONE;
    SvXMLDateElementAttributes eDateYear = XML_DEA_NONE;
    SvXMLDateElementAttributes eDateHours = XML_DEA_NONE;
    SvXMLDateElementAttributes eDateMins = XML_DEA_NONE;
    SvXMLDateElementAttributes eDateSecs = XML_DEA_NONE;
    bool bDateNoDefault = false;

    sal_uInt16 nPos = 0;
    bool bEnd = false;
    short nLastType = 0;
    while (!bEnd)
    {
        short nElemType = rFormat.GetNumForType( 0, nPos );
        switch ( nElemType )
        {
            case 0:
                if ( nLastType == NF_SYMBOLTYPE_STRING )
                    bDateNoDefault = true;  // text at the end -> no default date format
                bEnd = true;                // end of format reached
                break;
            case NF_SYMBOLTYPE_STRING:
            case NF_SYMBOLTYPE_DATESEP:
            case NF_SYMBOLTYPE_TIMESEP:
            case NF_SYMBOLTYPE_TIME100SECSEP:
                // text is ignored, except at the end
                break;
            // same mapping as in SvXMLNumFormatContext::AddNfKeyword:
            case NF_KEY_NN:     eDateDOW = XML_DEA_SHORT;       break;
            case NF_KEY_NNN:
            case NF_KEY_NNNN:   eDateDOW = XML_DEA_LONG;        break;
            case NF_KEY_D:      eDateDay = XML_DEA_SHORT;       break;
            case NF_KEY_DD:     eDateDay = XML_DEA_LONG;        break;
            case NF_KEY_M:      eDateMonth = XML_DEA_SHORT;     break;
            case NF_KEY_MM:     eDateMonth = XML_DEA_LONG;      break;
            case NF_KEY_MMM:    eDateMonth = XML_DEA_TEXTSHORT; break;
            case NF_KEY_MMMM:   eDateMonth = XML_DEA_TEXTLONG;  break;
            case NF_KEY_YY:     eDateYear = XML_DEA_SHORT;      break;
            case NF_KEY_YYYY:   eDateYear = XML_DEA_LONG;       break;
            case NF_KEY_H:      eDateHours = XML_DEA_SHORT;     break;
            case NF_KEY_HH:     eDateHours = XML_DEA_LONG;      break;
            case NF_KEY_MI:     eDateMins = XML_DEA_SHORT;      break;
            case NF_KEY_MMI:    eDateMins = XML_DEA_LONG;       break;
            case NF_KEY_S:      eDateSecs = XML_DEA_SHORT;      break;
            case NF_KEY_SS:     eDateSecs = XML_DEA_LONG;       break;
            case NF_KEY_AP:
            case NF_KEY_AMPM:   break;          // AM/PM may or may not be in date/time formats -> ignore by itself
            default:
                bDateNoDefault = true;      // any other element -> no default format
        }
        nLastType = nElemType;
        ++nPos;
    }

    if ( bDateNoDefault )
        return false;                       // additional elements
    else
    {
        NfIndexTableOffset eFound = static_cast<NfIndexTableOffset>(SvXMLNumFmtDefaults::GetDefaultDateFormat(
                eDateDOW, eDateDay, eDateMonth, eDateYear, eDateHours, eDateMins, eDateSecs, bSystemDate ));

        return ( eFound == eBuiltIn );
    }
}

//  export one part (condition)

void SvXMLNumFmtExport::ExportPart_Impl( const SvNumberformat& rFormat, sal_uInt32 nKey, sal_uInt32 nRealKey,
                                            sal_uInt16 nPart, bool bDefPart )
{
    //! for the default part, pass the conditions from the other parts!

    //  element name

    NfIndexTableOffset eBuiltIn = SvNumberFormatter::GetIndexTableOffset( nRealKey );

    SvNumFormatType nFmtType = SvNumFormatType::ALL;
    bool bThousand = false;
    sal_uInt16 nPrecision = 0;
    sal_uInt16 nLeading = 0;
    rFormat.GetNumForInfo( nPart, nFmtType, bThousand, nPrecision, nLeading);
    nFmtType &= ~SvNumFormatType::DEFINED;

    //  special treatment of builtin formats that aren't detected by normal parsing
    //  (the same formats that get the type set in SvNumberFormatter::ImpGenerateFormats)
    if ( eBuiltIn == NF_NUMBER_STANDARD )
        nFmtType = SvNumFormatType::NUMBER;
    else if ( eBuiltIn == NF_BOOLEAN )
        nFmtType = SvNumFormatType::LOGICAL;
    else if ( eBuiltIn == NF_TEXT )
        nFmtType = SvNumFormatType::TEXT;

    // #101606# An empty subformat is a valid number-style resulting in an
    // empty display string for the condition of the subformat.

    XMLTokenEnum eType = XML_TOKEN_INVALID;
    switch ( nFmtType )
    {
        // Type UNDEFINED likely is a crappy format string for that we could
        // not decide on any format type (and maybe could try harder?), but the
        // resulting XMLTokenEnum should be something valid, so make that
        // number-style.
        case SvNumFormatType::UNDEFINED:
            SAL_WARN("xmloff.style","UNDEFINED number format: '" << rFormat.GetFormatstring() << "'");
            [[fallthrough]];
        // Type is 0 if a format contains no recognized elements
        // (like text only) - this is handled as a number-style.
        case SvNumFormatType::ALL:
        case SvNumFormatType::EMPTY:
        case SvNumFormatType::NUMBER:
        case SvNumFormatType::SCIENTIFIC:
        case SvNumFormatType::FRACTION:
            eType = XML_NUMBER_STYLE;
            break;
        case SvNumFormatType::PERCENT:
            eType = XML_PERCENTAGE_STYLE;
            break;
        case SvNumFormatType::CURRENCY:
            eType = XML_CURRENCY_STYLE;
            break;
        case SvNumFormatType::DATE:
        case SvNumFormatType::DATETIME:
            eType = XML_DATE_STYLE;
            break;
        case SvNumFormatType::TIME:
            eType = XML_TIME_STYLE;
            break;
        case SvNumFormatType::TEXT:
            eType = XML_TEXT_STYLE;
            break;
        case SvNumFormatType::LOGICAL:
            eType = XML_BOOLEAN_STYLE;
            break;
        default: break;
    }
    SAL_WARN_IF( eType == XML_TOKEN_INVALID, "xmloff.style", "unknown format type" );

    OUString sAttrValue;
    bool bUserDef( rFormat.GetType() & SvNumFormatType::DEFINED );

    //  common attributes for format

    //  format name (generated from key) - style namespace
    m_rExport.AddAttribute( XML_NAMESPACE_STYLE, XML_NAME,
                        lcl_CreateStyleName( nKey, nPart, bDefPart, m_sPrefix ) );

    //  "volatile" attribute for styles used only in maps
    if ( !bDefPart )
        m_rExport.AddAttribute( XML_NAMESPACE_STYLE, XML_VOLATILE, XML_TRUE );

    //  language / country
    LanguageType nLang = rFormat.GetLanguage();
    AddLanguageAttr_Impl( nLang );                  // adds to pAttrList

    //  title (comment)
    //  titles for builtin formats are not written
    sAttrValue = rFormat.GetComment();
    if ( !sAttrValue.isEmpty() && bUserDef && bDefPart )
    {
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_TITLE, sAttrValue );
    }

    //  automatic ordering for currency and date formats
    //  only used for some built-in formats
    bool bAutoOrder = ( eBuiltIn == NF_CURRENCY_1000INT     || eBuiltIn == NF_CURRENCY_1000DEC2 ||
                        eBuiltIn == NF_CURRENCY_1000INT_RED || eBuiltIn == NF_CURRENCY_1000DEC2_RED ||
                        eBuiltIn == NF_CURRENCY_1000DEC2_DASHED ||
                        eBuiltIn == NF_DATE_SYSTEM_SHORT    || eBuiltIn == NF_DATE_SYSTEM_LONG ||
                        eBuiltIn == NF_DATE_SYS_MMYY        || eBuiltIn == NF_DATE_SYS_DDMMM ||
                        eBuiltIn == NF_DATE_SYS_DDMMYYYY    || eBuiltIn == NF_DATE_SYS_DDMMYY ||
                        eBuiltIn == NF_DATE_SYS_DMMMYY      || eBuiltIn == NF_DATE_SYS_DMMMYYYY ||
                        eBuiltIn == NF_DATE_SYS_DMMMMYYYY   || eBuiltIn == NF_DATE_SYS_NNDMMMYY ||
                        eBuiltIn == NF_DATE_SYS_NNDMMMMYYYY || eBuiltIn == NF_DATE_SYS_NNNNDMMMMYYYY ||
                        eBuiltIn == NF_DATETIME_SYSTEM_SHORT_HHMM || eBuiltIn == NF_DATETIME_SYS_DDMMYYYY_HHMM ||
                        eBuiltIn == NF_DATETIME_SYS_DDMMYYYY_HHMMSS );

    //  format source (for date and time formats)
    //  only used for some built-in formats
    bool bSystemDate = ( eBuiltIn == NF_DATE_SYSTEM_SHORT ||
                         eBuiltIn == NF_DATE_SYSTEM_LONG  ||
                         eBuiltIn == NF_DATETIME_SYSTEM_SHORT_HHMM );
    bool bLongSysDate = ( eBuiltIn == NF_DATE_SYSTEM_LONG );

    // check if the format definition matches the key
    if ( bAutoOrder && ( nFmtType == SvNumFormatType::DATE || nFmtType == SvNumFormatType::DATETIME ) &&
            !lcl_IsDefaultDateFormat( rFormat, bSystemDate, eBuiltIn ) )
    {
        bAutoOrder = bSystemDate = bLongSysDate = false;        // don't write automatic-order attribute then
    }

    if ( bAutoOrder &&
        ( nFmtType == SvNumFormatType::CURRENCY || nFmtType == SvNumFormatType::DATE || nFmtType == SvNumFormatType::DATETIME ) )
    {
        //  #85109# format type must be checked to avoid dtd errors if
        //  locale data contains other format types at the built-in positions

        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_AUTOMATIC_ORDER,
                              XML_TRUE );
    }

    if ( bSystemDate && bAutoOrder &&
        ( nFmtType == SvNumFormatType::DATE || nFmtType == SvNumFormatType::DATETIME ) )
    {
        //  #85109# format type must be checked to avoid dtd errors if
        //  locale data contains other format types at the built-in positions

        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_FORMAT_SOURCE,
                              XML_LANGUAGE );
    }

    //  overflow for time formats as in [hh]:mm
    //  controlled by bThousand from number format info
    //  default for truncate-on-overflow is true
    if ( nFmtType == SvNumFormatType::TIME && bThousand )
    {
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_TRUNCATE_ON_OVERFLOW,
                              XML_FALSE );
    }

    // Native number transliteration
    css::i18n::NativeNumberXmlAttributes2 aAttr;
    rFormat.GetNatNumXml( aAttr, nPart, m_pFormatter->GetNatNum() );
    if ( !aAttr.Format.isEmpty() )
    {
        assert(aAttr.Spellout.isEmpty());   // mutually exclusive

        /* FIXME-BCP47: ODF defines no transliteration-script or
         * transliteration-rfc-language-tag */
        LanguageTag aLanguageTag( aAttr.Locale);
        OUString aLanguage, aScript, aCountry;
        aLanguageTag.getIsoLanguageScriptCountry( aLanguage, aScript, aCountry);
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_TRANSLITERATION_FORMAT,
                              aAttr.Format );
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_TRANSLITERATION_LANGUAGE,
                              aLanguage );
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_TRANSLITERATION_COUNTRY,
                              aCountry );
        m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_TRANSLITERATION_STYLE,
                              aAttr.Style );
    }

    SvtSaveOptions::ODFSaneDefaultVersion eVersion = m_rExport.getSaneDefaultVersion();
    if ( !aAttr.Spellout.isEmpty() )
    {
        const bool bWriteSpellout = aAttr.Format.isEmpty();
        assert(bWriteSpellout);     // mutually exclusive

        // Export only for 1.2 and later with extensions
        // Also ensure that duplicated transliteration-language and
        // transliteration-country attributes never escape into the wild with
        // releases.
        if ( (eVersion & SvtSaveOptions::ODFSVER_EXTENDED) && bWriteSpellout )
        {
            /* FIXME-BCP47: ODF defines no transliteration-script or
             * transliteration-rfc-language-tag */
            LanguageTag aLanguageTag( aAttr.Locale);
            OUString aLanguage, aScript, aCountry;
            aLanguageTag.getIsoLanguageScriptCountry( aLanguage, aScript, aCountry);
            // For 1.2/1.3+ use loext namespace.
            m_rExport.AddAttribute( /*((eVersion < SvtSaveOptions::ODFSVER_)
                        ? */ XML_NAMESPACE_LO_EXT /*: XML_NAMESPACE_NUMBER)*/,
                    XML_TRANSLITERATION_SPELLOUT, aAttr.Spellout );
            m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_TRANSLITERATION_LANGUAGE,
                                  aLanguage );
            m_rExport.AddAttribute( XML_NAMESPACE_NUMBER, XML_TRANSLITERATION_COUNTRY,
                                  aCountry );
        }
    }

    // The element
    SvXMLElementExport aElem( m_rExport, XML_NAMESPACE_NUMBER, eType,
                              true, true );

    //  color (properties element)

    const Color* pCol = rFormat.GetColor( nPart );
    if (pCol)
        WriteColorElement_Impl(*pCol);

    //  detect if there is "real" content, excluding color and maps
    //! move to implementation of Write... methods?
    bool bAnyContent = false;

    //  format elements

    SvXMLEmbeddedTextEntryArr aEmbeddedEntries;
    if ( eBuiltIn == NF_NUMBER_STANDARD )
    {
        //  default number format contains just one number element
        WriteNumberElement_Impl( -1, -1, 1, -1, OUString(), false, 0, aEmbeddedEntries );
        bAnyContent = true;
    }
    else if ( eBuiltIn == NF_BOOLEAN )
    {
        //  boolean format contains just one boolean element
        WriteBooleanElement_Impl();
        bAnyContent = true;
    }
    else if (eType == XML_BOOLEAN_STYLE)
    {
        // <number:boolean-style> may contain only <number:boolean> and
        // <number:text> elements.
        sal_uInt16 nPos = 0;
        bool bEnd = false;
        while (!bEnd)
        {
            const short nElemType = rFormat.GetNumForType( nPart, nPos );
            switch (nElemType)
            {
                case 0:
                    bEnd = true;                // end of format reached
                    if (m_bHasText && m_sTextContent.isEmpty())
                        m_bHasText = false;       // don't write trailing empty text
                break;
                case NF_SYMBOLTYPE_STRING:
                    {
                        const OUString* pElemStr = rFormat.GetNumForString( nPart, nPos );
                        if (pElemStr)
                            AddToTextElement_Impl( *pElemStr );
                    }
                break;
                case NF_KEY_BOOLEAN:
                    WriteBooleanElement_Impl();
                    bAnyContent = true;
                break;
            }
            ++nPos;
        }
    }
    else
    {
        //  first loop to collect attributes

        bool bDecDashes  = false;
        bool bExpFound   = false;
        bool bCurrFound  = false;
        bool bInInteger  = true;
        bool bExpSign = true;
        bool bExponentLowercase = false;        // 'e' or 'E' for scientific notation
        bool bDecAlign   = false;               // decimal alignment with "?"
        sal_Int32 nExpDigits = 0;               // '0' and '?' in exponent
        sal_Int32 nBlankExp = 0;                // only '?' in exponent
        sal_Int32 nIntegerSymbols = 0;          // for embedded-text, including "#"
        sal_Int32 nTrailingThousands = 0;       // thousands-separators after all digits
        sal_Int32 nMinDecimals = nPrecision;
        sal_Int32 nBlankInteger = 0;
        OUString sCurrExt;
        OUString aCalendar;
        bool bImplicitOtherCalendar = false;
        bool bExplicitCalendar = false;
        sal_uInt16 nPos = 0;
        bool bEnd = false;
        while (!bEnd)
        {
            short nElemType = rFormat.GetNumForType( nPart, nPos );
            const OUString* pElemStr = rFormat.GetNumForString( nPart, nPos );

            switch ( nElemType )
            {
                case 0:
                    bEnd = true;                // end of format reached
                    break;
                case NF_SYMBOLTYPE_DIGIT:
                    if ( bExpFound && pElemStr )
                    {
                        nExpDigits += pElemStr->getLength();
                        for ( sal_Int32 i = pElemStr->getLength()-1; i >= 0 ; i-- )
                        {
                            if ( (*pElemStr)[i] == '?' )
                                nBlankExp ++;
                        }
                    }
                    else if ( !bDecDashes && pElemStr && (*pElemStr)[0] == '-' )
                    {
                        bDecDashes = true;
                        nMinDecimals = 0;
                    }
                    else if ( nFmtType != SvNumFormatType::FRACTION && !bInInteger && pElemStr )
                    {
                        for ( sal_Int32 i = pElemStr->getLength()-1; i >= 0 ; i-- )
                        {
                            sal_Unicode aChar = (*pElemStr)[i];
                            if ( aChar == '#' || aChar == '?' )
                            {
                                nMinDecimals --;
                                if ( aChar == '?' )
                                    bDecAlign = true;
                            }
                            else
                                break;
                        }
                    }
                    if ( bInInteger && pElemStr )
                    {
                        nIntegerSymbols += pElemStr->getLength();
                        for ( sal_Int32 i = pElemStr->getLength()-1; i >= 0 ; i-- )
                        {
                            if ( (*pElemStr)[i] == '?' )
                                nBlankInteger ++;
                        }
                    }
                    nTrailingThousands = 0;
                    break;
                case NF_SYMBOLTYPE_FRACBLANK:
                case NF_SYMBOLTYPE_DECSEP:
                    bInInteger = false;
                    break;
                case NF_SYMBOLTYPE_THSEP:
                    if (pElemStr)
                        nTrailingThousands += pElemStr->getLength();      // is reset to 0 if digits follow
                    break;
                case NF_SYMBOLTYPE_EXP:
                    bExpFound = true;           // following digits are exponent digits
                    bInInteger = false;
                    if ( pElemStr && ( pElemStr->getLength() == 1
                                  || ( pElemStr->getLength() == 2 && (*pElemStr)[1] == '-' ) ) )
                        bExpSign = false;       // for 0.00E0 or 0.00E-00
                    if ( pElemStr && (*pElemStr)[0] == 'e' )
                        bExponentLowercase = true;   // for 0.00e+00
                    break;
                case NF_SYMBOLTYPE_CURRENCY:
                    bCurrFound = true;
                    break;
                case NF_SYMBOLTYPE_CURREXT:
                    if (pElemStr)
                        sCurrExt = *pElemStr;
                    break;

                // E, EE, R, RR: select non-gregorian calendar
                // AAA, AAAA: calendar is switched at the position of the element
                case NF_KEY_EC:
                case NF_KEY_EEC:
                case NF_KEY_R:
                case NF_KEY_RR:
                    if (aCalendar.isEmpty())
                    {
                        aCalendar = lcl_GetDefaultCalendar( m_pFormatter, nLang );
                        bImplicitOtherCalendar = true;
                    }
                    break;
            }
            ++nPos;
        }

        //  collect strings for embedded-text (must be known before number element is written)
        bool bAllowEmbedded = ( nFmtType == SvNumFormatType::ALL || nFmtType == SvNumFormatType::NUMBER ||
                                        nFmtType == SvNumFormatType::CURRENCY ||
                                        // Export only for 1.x with extensions
                                        ( nFmtType == SvNumFormatType::SCIENTIFIC && (eVersion & SvtSaveOptions::ODFSVER_EXTENDED) )||
                                        nFmtType == SvNumFormatType::PERCENT );
        if ( bAllowEmbedded )
        {
            sal_Int32 nDigitsPassed = 0;
            sal_Int32 nEmbeddedPositionsMax = nIntegerSymbols;
            // Enable embedded text in decimal part only if there's a decimal part
            if ( nPrecision )
                nEmbeddedPositionsMax += nPrecision + 1;
            // Enable embedded text in exponent in scientific number
            if ( nFmtType == SvNumFormatType::SCIENTIFIC )
                nEmbeddedPositionsMax += 1 + nExpDigits;
            nPos = 0;
            bEnd = false;
            bExpFound = false;
            while (!bEnd)
            {
                short nElemType = rFormat.GetNumForType( nPart, nPos );
                const OUString* pElemStr = rFormat.GetNumForString( nPart, nPos );

                switch ( nElemType )
                {
                    case 0:
                        bEnd = true;                // end of format reached
                        break;
                    case NF_SYMBOLTYPE_DIGIT:
                        if ( pElemStr )
                            nDigitsPassed += pElemStr->getLength();
                        break;
                    case NF_SYMBOLTYPE_EXP:
                        bExpFound = true;
                        [[fallthrough]];
                    case NF_SYMBOLTYPE_DECSEP:
                        nDigitsPassed++;
                        break;
                    case NF_SYMBOLTYPE_STRING:
                    case NF_SYMBOLTYPE_BLANK:
                    case NF_SYMBOLTYPE_PERCENT:
                        if ( 0 < nDigitsPassed && nDigitsPassed < nEmbeddedPositionsMax && pElemStr )
                        {
                            //  text (literal or underscore) within the integer (>=0) or decimal (<0) part of a number:number element

                            OUString aEmbeddedStr;
                            bool bSaveBlankWidthSymbol = false;
                            if ( nElemType == NF_SYMBOLTYPE_STRING || nElemType == NF_SYMBOLTYPE_PERCENT )
                            {
                                aEmbeddedStr = *pElemStr;
                            }
                            else if (pElemStr->getLength() >= 2)
                            {
                                if ( eVersion > SvtSaveOptions::ODFSVER_013 && ( (eVersion & SvtSaveOptions::ODFSVER_EXTENDED) != 0 ) )
                                {
                                    aEmbeddedStr = pElemStr->copy( 1, 1 );
                                    bSaveBlankWidthSymbol = true;
                                }
                                else //  turn "_x" into the number of spaces used for x in InsertBlanks in the NumberFormat
                                    SvNumberformat::InsertBlanks( aEmbeddedStr, 0, (*pElemStr)[1] );
                            }
                            sal_Int32 nEmbedPos = nIntegerSymbols - nDigitsPassed;

                            aEmbeddedEntries.push_back(
                                SvXMLEmbeddedTextEntry( nPos, nEmbedPos, aEmbeddedStr, bSaveBlankWidthSymbol ));
                            // exponent sign is required with embedded text in exponent
                            if ( bExpFound && !bExpSign )
                                bExpSign = true;
                        }
                        break;
                }
                ++nPos;
            }
        }

        //  final loop to write elements

        bool bNumWritten = false;
        bool bCurrencyWritten = false;
        short nPrevType = 0;
        nPos = 0;
        bEnd = false;
        while (!bEnd)
        {
            short nElemType = rFormat.GetNumForType( nPart, nPos );
            const OUString* pElemStr = rFormat.GetNumForString( nPart, nPos );

            switch ( nElemType )
            {
                case 0:
                    bEnd = true;                // end of format reached
                    if (m_bHasText && m_sTextContent.isEmpty())
                        m_bHasText = false;       // don't write trailing empty text
                    break;
                case NF_SYMBOLTYPE_STRING:
                case NF_SYMBOLTYPE_DATESEP:
                case NF_SYMBOLTYPE_TIMESEP:
                case NF_SYMBOLTYPE_TIME100SECSEP:
                case NF_SYMBOLTYPE_PERCENT:
                    if (pElemStr)
                    {
                        if ( ( nElemType == NF_SYMBOLTYPE_TIME100SECSEP ) &&
                             ( nPrevType == NF_KEY_S || nPrevType == NF_KEY_SS ||
                               ( nPos > 0 && (*rFormat.GetNumForString( nPart, nPos-1 ))[0] == ']' &&
                               ( nFmtType == SvNumFormatType::TIME || nFmtType == SvNumFormatType::DATETIME ) ) ) &&
                             nPrecision > 0 )
                        {
                            //  decimal separator after seconds or [SS] is implied by
                            //  "decimal-places" attribute and must not be written
                            //  as text element
                            //! difference between '.' and ',' is lost here
                        }
                        else if ( lcl_IsInEmbedded( aEmbeddedEntries, nPos ) )
                        {
                            //  text is written as embedded-text child of the number,
                            //  don't create a text element
                        }
                        else if ( nFmtType == SvNumFormatType::CURRENCY && !bCurrFound && !bCurrencyWritten )
                        {
                            //  automatic currency symbol is implemented as part of
                            //  normal text -> search for the symbol
                            bCurrencyWritten = WriteTextWithCurrency_Impl( *pElemStr,
                                LanguageTag::convertToLocale( nLang ) );
                            bAnyContent = true;
                        }
                        else
                            AddToTextElement_Impl( *pElemStr );
                    }
                    break;
                case NF_SYMBOLTYPE_BLANK:
                    if ( pElemStr && !lcl_IsInEmbedded( aEmbeddedEntries, nPos ) )
                    {
                        if ( pElemStr->getLength() == 2 )
                        {
                            OUString aBlankWidthChar = pElemStr->copy( 1 );
                            lcl_WriteBlankWidthString( aBlankWidthChar, m_sBlankWidthString, m_sTextContent );
                            m_bHasText = true;
                        }
                    }
                    break;
                case NF_KEY_GENERAL :
                        WriteNumberElement_Impl( -1, -1, 1, -1, OUString(), false, 0, aEmbeddedEntries );
                        bAnyContent = true;
                    break;
                case NF_KEY_CCC:
                    if (pElemStr)
                    {
                        if ( bCurrencyWritten )
                            AddToTextElement_Impl( *pElemStr );     // never more than one currency element
                        else
                        {
                            //! must be different from short automatic format
                            //! but should still be empty (meaning automatic)
                            //  pElemStr is "CCC"

                            WriteCurrencyElement_Impl( *pElemStr, u"" );
                            bAnyContent = true;
                            bCurrencyWritten = true;
                        }
                    }
                    break;
                case NF_SYMBOLTYPE_CURRENCY:
                    if (pElemStr)
                    {
                        if ( bCurrencyWritten )
                            AddToTextElement_Impl( *pElemStr );     // never more than one currency element
                        else
                        {
                            WriteCurrencyElement_Impl( *pElemStr, sCurrExt );
                            bAnyContent = true;
                            bCurrencyWritten = true;
                        }
                    }
                    break;
                case NF_SYMBOLTYPE_DIGIT:
                    if (!bNumWritten)           // write number part
                    {
                        switch ( nFmtType )
                        {
                            // for type 0 (not recognized as a special type),
                            // write a "normal" number
                            case SvNumFormatType::ALL:
                            case SvNumFormatType::NUMBER:
                            case SvNumFormatType::CURRENCY:
                            case SvNumFormatType::PERCENT:
                                {
                                    //  decimals
                                    //  only some built-in formats have automatic decimals
                                    sal_Int32 nDecimals = nPrecision;   // from GetFormatSpecialInfo
                                    if ( eBuiltIn == NF_NUMBER_STANDARD ||
                                         eBuiltIn == NF_CURRENCY_1000DEC2 ||
                                         eBuiltIn == NF_CURRENCY_1000DEC2_RED ||
                                         eBuiltIn == NF_CURRENCY_1000DEC2_CCC ||
                                         eBuiltIn == NF_CURRENCY_1000DEC2_DASHED )
                                        nDecimals = -1;

                                    //  integer digits
                                    //  only one built-in format has automatic integer digits
                                    sal_Int32 nInteger = nLeading;
                                    if ( eBuiltIn == NF_NUMBER_SYSTEM )
                                    {
                                        nInteger = -1;
                                        nBlankInteger = -1;
                                    }

                                    //  string for decimal replacement
                                    //  has to be taken from nPrecision
                                    //  (positive number even for automatic decimals)
                                    OUStringBuffer sDashStr;
                                    if (bDecDashes && nPrecision > 0)
                                        comphelper::string::padToLength(sDashStr, nPrecision, '-');
                                    // "?" in decimal part are replaced by space character
                                    if (bDecAlign && nPrecision > 0)
                                        sDashStr = " ";

                                    WriteNumberElement_Impl(nDecimals, nMinDecimals, nInteger, nBlankInteger, sDashStr.makeStringAndClear(),
                                        bThousand, nTrailingThousands, aEmbeddedEntries);
                                    bAnyContent = true;
                                }
                                break;
                            case SvNumFormatType::SCIENTIFIC:
                                // #i43959# for scientific numbers, count all integer symbols ("0", "?" and "#")
                                // as integer digits: use nIntegerSymbols instead of nLeading
                                // nIntegerSymbols represents exponent interval (for engineering notation)
                                WriteScientificElement_Impl( nPrecision, nMinDecimals, nLeading, nBlankInteger, bThousand, nExpDigits, nIntegerSymbols, bExpSign,
                                    bExponentLowercase, nBlankExp, aEmbeddedEntries );
                                bAnyContent = true;
                                break;
                            case SvNumFormatType::FRACTION:
                                {
                                    sal_Int32 nInteger = nLeading;
                                    if ( rFormat.GetNumForNumberElementCount( nPart ) == 3 )
                                    {
                                        //  If there is only two numbers + fraction in format string
                                        //  the fraction doesn't have an integer part, and no
                                        //  min-integer-digits attribute must be written.
                                        nInteger = -1;
                                        nBlankInteger = -1;
                                    }
                                    WriteFractionElement_Impl( nInteger, nBlankInteger, bThousand,  rFormat, nPart );
                                    bAnyContent = true;
                                }
                                break;
                            default: break;
                        }

                        bNumWritten = true;
                    }
                    break;
                case NF_SYMBOLTYPE_DECSEP:
                    if ( pElemStr && nPrecision == 0 )
                    {
                        //  A decimal separator after the number, without following decimal digits,
                        //  isn't modelled as part of the number element, so it's written as text
                        //  (the distinction between a quoted and non-quoted, locale-dependent
                        //  character is lost here).

                        AddToTextElement_Impl( *pElemStr );
                    }
                    break;
                case NF_SYMBOLTYPE_DEL:
                    if ( pElemStr && *pElemStr == "@" )
                    {
                        WriteTextContentElement_Impl();
                        bAnyContent = true;
                    }
                    break;

                case NF_SYMBOLTYPE_CALENDAR:
                    if ( pElemStr )
                    {
                        aCalendar = *pElemStr;
                        bExplicitCalendar = true;
                    }
                    break;

                // date elements:

                case NF_KEY_D:
                case NF_KEY_DD:
                    {
                        bool bLong = ( nElemType == NF_KEY_DD );
                        WriteDayElement_Impl( aCalendar, ( bSystemDate ? bLongSysDate : bLong ) );
                        bAnyContent = true;
                    }
                    break;
                case NF_KEY_DDD:
                case NF_KEY_DDDD:
                case NF_KEY_NN:
                case NF_KEY_NNN:
                case NF_KEY_NNNN:
                case NF_KEY_AAA:
                case NF_KEY_AAAA:
                    {
                        OUString aCalAttr = aCalendar;
                        if ( nElemType == NF_KEY_AAA || nElemType == NF_KEY_AAAA )
                        {
                            //  calendar attribute for AAA and AAAA is switched only for this element
                            if (aCalAttr.isEmpty())
                                aCalAttr = lcl_GetDefaultCalendar( m_pFormatter, nLang );
                        }

                        bool bLong = ( nElemType == NF_KEY_NNN || nElemType == NF_KEY_NNNN ||
                                           nElemType == NF_KEY_DDDD || nElemType == NF_KEY_AAAA );
                        WriteDayOfWeekElement_Impl( aCalAttr, ( bSystemDate ? bLongSysDate : bLong ) );
                        bAnyContent = true;
                        if ( nElemType == NF_KEY_NNNN )
                        {
                            //  write additional text element for separator
                            m_pLocaleData = LocaleDataWrapper::get( LanguageTag( nLang ) );
                            AddToTextElement_Impl( m_pLocaleData->getLongDateDayOfWeekSep() );
                        }
                    }
                    break;
                case NF_KEY_M:
                case NF_KEY_MM:
                case NF_KEY_MMM:
                case NF_KEY_MMMM:
                case NF_KEY_MMMMM:      //! first letter of month name, no attribute available
                    {
                        bool bLong = ( nElemType == NF_KEY_MM  || nElemType == NF_KEY_MMMM );
                        bool bText = ( nElemType == NF_KEY_MMM || nElemType == NF_KEY_MMMM ||
                                            nElemType == NF_KEY_MMMMM );
                        WriteMonthElement_Impl( aCalendar, ( bSystemDate ? bLongSysDate : bLong ), bText );
                        bAnyContent = true;
                    }
                    break;
                case NF_KEY_YY:
                case NF_KEY_YYYY:
                case NF_KEY_EC:
                case NF_KEY_EEC:
                case NF_KEY_R:      //! R acts as EE, no attribute available
                    {
                        //! distinguish EE and R
                        // Calendar attribute for E and EE and R is set in
                        // first loop. If set and not an explicit calendar and
                        // YY or YYYY is encountered, switch temporarily to
                        // Gregorian.
                        bool bLong = ( nElemType == NF_KEY_YYYY || nElemType == NF_KEY_EEC ||
                                            nElemType == NF_KEY_R );
                        WriteYearElement_Impl(
                                ((bImplicitOtherCalendar && !bExplicitCalendar
                                  && (nElemType == NF_KEY_YY || nElemType == NF_KEY_YYYY)) ? u"gregorian"_ustr : aCalendar),
                                (bSystemDate ? bLongSysDate : bLong));
                        bAnyContent = true;
                    }
                    break;
                case NF_KEY_G:
                case NF_KEY_GG:
                case NF_KEY_GGG:
                case NF_KEY_RR:     //! RR acts as GGGEE, no attribute available
                    {
                        //! distinguish GG and GGG and RR
                        bool bLong = ( nElemType == NF_KEY_GGG || nElemType == NF_KEY_RR );
                        WriteEraElement_Impl( aCalendar, ( bSystemDate ? bLongSysDate : bLong ) );
                        bAnyContent = true;
                        if ( nElemType == NF_KEY_RR )
                        {
                            //  calendar attribute for RR is set in first loop
                            WriteYearElement_Impl( aCalendar, ( bSystemDate || bLongSysDate ) );
                        }
                    }
                    break;
                case NF_KEY_Q:
                case NF_KEY_QQ:
                    {
                        bool bLong = ( nElemType == NF_KEY_QQ );
                        WriteQuarterElement_Impl( aCalendar, ( bSystemDate ? bLongSysDate : bLong ) );
                        bAnyContent = true;
                    }
                    break;
                case NF_KEY_WW:
                    WriteWeekElement_Impl( aCalendar );
                    bAnyContent = true;
                    break;

                // time elements (bSystemDate is not used):

                case NF_KEY_H:
                case NF_KEY_HH:
                    WriteHoursElement_Impl( nElemType == NF_KEY_HH );
                    bAnyContent = true;
                    break;
                case NF_KEY_MI:
                case NF_KEY_MMI:
                    WriteMinutesElement_Impl( nElemType == NF_KEY_MMI );
                    bAnyContent = true;
                    break;
                case NF_KEY_S:
                case NF_KEY_SS:
                    WriteSecondsElement_Impl( ( nElemType == NF_KEY_SS ), nPrecision );
                    bAnyContent = true;
                    break;
                case NF_KEY_AMPM:
                case NF_KEY_AP:
                    WriteAMPMElement_Impl();        // short/long?
                    bAnyContent = true;
                    break;
                case NF_SYMBOLTYPE_STAR :
                    // export only if ODF 1.2 extensions are enabled
                    if (m_rExport.getSaneDefaultVersion() > SvtSaveOptions::ODFSVER_012)
                    {
                        if ( pElemStr && pElemStr->getLength() > 1 )
                            WriteRepeatedElement_Impl( (*pElemStr)[1] );
                    }
                    break;
            }
            nPrevType = nElemType;
            ++nPos;
        }
    }

    if ( !m_sTextContent.isEmpty() )
        bAnyContent = true;     // element written in FinishTextElement_Impl

    FinishTextElement_Impl();       // final text element - before maps

    if ( !bAnyContent )
    {
        //  for an empty format, write an empty text element
        SvXMLElementExport aTElem( m_rExport, XML_NAMESPACE_NUMBER, XML_TEXT,
                                   true, false );
    }

    //  mapping (conditions) must be last elements

    if (!bDefPart)
        return;

    SvNumberformatLimitOps eOp1, eOp2;
    double fLimit1, fLimit2;
    rFormat.GetConditions( eOp1, fLimit1, eOp2, fLimit2 );

    WriteMapElement_Impl( eOp1, fLimit1, nKey, 0 );
    WriteMapElement_Impl( eOp2, fLimit2, nKey, 1 );

    if ( !rFormat.HasTextFormat() )
        return;

    //  4th part is for text -> make an "all other numbers" condition for the 3rd part
    //  by reversing the 2nd condition.
    //  For a trailing text format like  0;@  that has no conditions
    //  use a "less or equal than biggest" condition for the number
    //  part, ODF can't store subformats (style maps) without
    //  conditions.

    SvNumberformatLimitOps eOp3 = NUMBERFORMAT_OP_NO;
    double fLimit3 = fLimit2;
    sal_uInt16 nLastPart = 2;
    SvNumberformatLimitOps eOpLast = eOp2;
    if (eOp2 == NUMBERFORMAT_OP_NO)
    {
        eOpLast = eOp1;
        fLimit3 = fLimit1;
        nLastPart = (eOp1 == NUMBERFORMAT_OP_NO) ? 0 : 1;
    }
    switch ( eOpLast )
    {
        case NUMBERFORMAT_OP_EQ: eOp3 = NUMBERFORMAT_OP_NE; break;
        case NUMBERFORMAT_OP_NE: eOp3 = NUMBERFORMAT_OP_EQ; break;
        case NUMBERFORMAT_OP_LT: eOp3 = NUMBERFORMAT_OP_GE; break;
        case NUMBERFORMAT_OP_LE: eOp3 = NUMBERFORMAT_OP_GT; break;
        case NUMBERFORMAT_OP_GT: eOp3 = NUMBERFORMAT_OP_LE; break;
        case NUMBERFORMAT_OP_GE: eOp3 = NUMBERFORMAT_OP_LT; break;
        case NUMBERFORMAT_OP_NO: eOp3 = NUMBERFORMAT_OP_LE; fLimit3 = DBL_MAX; break;
    }

    if ( fLimit1 == fLimit2 &&
            ( ( eOp1 == NUMBERFORMAT_OP_LT && eOp2 == NUMBERFORMAT_OP_GT ) ||
              ( eOp1 == NUMBERFORMAT_OP_GT && eOp2 == NUMBERFORMAT_OP_LT ) ) )
    {
        //  For <x and >x, add =x as last condition
        //  (just for readability, <=x would be valid, too)

        eOp3 = NUMBERFORMAT_OP_EQ;
    }

    WriteMapElement_Impl( eOp3, fLimit3, nKey, nLastPart );
}

//  export one format

void SvXMLNumFmtExport::ExportFormat_Impl( const SvNumberformat& rFormat, sal_uInt32 nKey, sal_uInt32 nRealKey )
{
    const sal_uInt16 XMLNUM_MAX_PARTS = 4;
    bool bParts[XMLNUM_MAX_PARTS] = { false, false, false, false };
    sal_uInt16 nUsedParts = 0;
    for (sal_uInt16 nPart=0; nPart<XMLNUM_MAX_PARTS; ++nPart)
    {
        if (rFormat.GetNumForInfoScannedType( nPart) != SvNumFormatType::UNDEFINED)
        {
            bParts[nPart] = true;
            nUsedParts = nPart + 1;
        }
    }

    SvNumberformatLimitOps eOp1, eOp2;
    double fLimit1, fLimit2;
    rFormat.GetConditions( eOp1, fLimit1, eOp2, fLimit2 );

    //  if conditions are set, even empty formats must be written

    if ( eOp1 != NUMBERFORMAT_OP_NO )
    {
        bParts[1] = true;
        if (nUsedParts < 2)
            nUsedParts = 2;
    }
    if ( eOp2 != NUMBERFORMAT_OP_NO )
    {
        bParts[2] = true;
        if (nUsedParts < 3)
            nUsedParts = 3;
    }
    if ( rFormat.HasTextFormat() )
    {
        bParts[3] = true;
        if (nUsedParts < 4)
            nUsedParts = 4;
    }

    for (sal_uInt16 nPart=0; nPart<XMLNUM_MAX_PARTS; ++nPart)
    {
        if (bParts[nPart])
        {
            bool bDefault = ( nPart+1 == nUsedParts );          // last = default
            ExportPart_Impl( rFormat, nKey, nRealKey, nPart, bDefault );
        }
    }
}

//  export method called by application

void SvXMLNumFmtExport::Export( bool bIsAutoStyle )
{
    if ( !m_pFormatter )
        return;                         // no formatter -> no entries

    sal_uInt32 nKey;
    const SvNumberformat* pFormat = nullptr;
    bool bNext(m_pUsedList->GetFirstUsed(nKey));
    while(bNext)
    {
        // ODF has its notation of system formats, so obtain the "real" already
        // substituted format but use the original key for style name.
        sal_uInt32 nRealKey = nKey;
        pFormat = m_pFormatter->GetSubstitutedEntry( nKey, nRealKey);
        if(pFormat)
            ExportFormat_Impl( *pFormat, nKey, nRealKey );
        bNext = m_pUsedList->GetNextUsed(nKey);
    }
    if (!bIsAutoStyle)
    {
        std::vector<LanguageType> aLanguages;
        m_pFormatter->GetUsedLanguages( aLanguages );
        for (const auto& nLang : aLanguages)
        {
            sal_uInt32 nDefaultIndex = 0;
            SvNumberFormatTable& rTable = m_pFormatter->GetEntryTable(
                                         SvNumFormatType::DEFINED, nDefaultIndex, nLang );
            for (const auto& rTableEntry : rTable)
            {
                nKey = rTableEntry.first;
                pFormat = rTableEntry.second;
                if (!m_pUsedList->IsUsed(nKey))
                {
                    DBG_ASSERT((pFormat->GetType() & SvNumFormatType::DEFINED), "a not user defined numberformat found");
                    sal_uInt32 nRealKey = nKey;
                    if (pFormat->IsSubstituted())
                    {
                        pFormat = m_pFormatter->GetSubstitutedEntry( nKey, nRealKey); // export the "real" format
                        assert(pFormat);
                    }
                    //  user-defined and used formats are exported
                    ExportFormat_Impl( *pFormat, nKey, nRealKey );
                    // if it is a user-defined Format it will be added else nothing will happen
                    m_pUsedList->SetUsed(nKey);
                }
            }
        }
    }
    m_pUsedList->Export();
}

OUString SvXMLNumFmtExport::GetStyleName( sal_uInt32 nKey )
{
    if(m_pUsedList->IsUsed(nKey) || m_pUsedList->IsWasUsed(nKey))
        return lcl_CreateStyleName( nKey, 0, true, m_sPrefix );
    else
    {
        OSL_FAIL("There is no written Data-Style");
        return OUString();
    }
}

void SvXMLNumFmtExport::SetUsed( sal_uInt32 nKey )
{
    SAL_WARN_IF( m_pFormatter == nullptr, "xmloff.style", "missing formatter" );
    if( !m_pFormatter )
        return;

    if (m_pFormatter->GetEntry(nKey))
        m_pUsedList->SetUsed( nKey );
    else {
        OSL_FAIL("no existing Numberformat found with this key");
    }
}

uno::Sequence<sal_Int32> SvXMLNumFmtExport::GetWasUsed() const
{
    if (m_pUsedList)
        return m_pUsedList->GetWasUsed();
    return uno::Sequence<sal_Int32>();
}

void SvXMLNumFmtExport::SetWasUsed(const uno::Sequence<sal_Int32>& rWasUsed)
{
    if (m_pUsedList)
        m_pUsedList->SetWasUsed(rWasUsed);
}

static const SvNumberformat* lcl_GetFormat( SvNumberFormatter const * pFormatter,
                           sal_uInt32 nKey )
{
    return ( pFormatter != nullptr ) ? pFormatter->GetEntry( nKey ) : nullptr;
}

sal_uInt32 SvXMLNumFmtExport::ForceSystemLanguage( sal_uInt32 nKey )
{
    sal_uInt32 nRet = nKey;

    const SvNumberformat* pFormat = lcl_GetFormat( m_pFormatter, nKey );
    if( pFormat != nullptr )
    {
        SAL_WARN_IF( m_pFormatter == nullptr, "xmloff.style", "format without formatter?" );

        SvNumFormatType nType = pFormat->GetType();

        sal_uInt32 nNewKey = m_pFormatter->GetFormatForLanguageIfBuiltIn(
                       nKey, LANGUAGE_SYSTEM );

        if( nNewKey != nKey )
        {
            nRet = nNewKey;
        }
        else
        {
            OUString aFormatString( pFormat->GetFormatstring() );
            sal_Int32 nErrorPos;
            m_pFormatter->PutandConvertEntry(
                            aFormatString,
                            nErrorPos, nType, nNewKey,
                            pFormat->GetLanguage(), LANGUAGE_SYSTEM, true);

            // success? Then use new key.
            if( nErrorPos == 0 )
                nRet = nNewKey;
        }
    }

    return nRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
