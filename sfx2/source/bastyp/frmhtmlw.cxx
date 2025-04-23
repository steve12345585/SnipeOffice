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

#include <svtools/htmlkywd.hxx>

#include <rtl/tencinfo.h>
#include <sal/log.hxx>

#include <svl/urihelper.hxx>
#include <tools/stream.hxx>
#include <tools/debug.hxx>
#include <unotools/resmgr.hxx>
#include <svtools/htmlout.hxx>

#include <sfx2/frmdescr.hxx>
#include <sfx2/frmhtmlw.hxx>
#include <strings.hxx>

#include <comphelper/processfactory.hxx>
#include <comphelper/string.hxx>

#include <com/sun/star/script/Converter.hpp>
#include <com/sun/star/document/XDocumentProperties.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>

#include <rtl/bootstrap.hxx>
#include <rtl/strbuf.hxx>
#include <sax/tools/converter.hxx>

using namespace ::com::sun::star;

char const sHTML_SC_yes[] =  "YES";
char const sHTML_SC_no[] =       "NO";

void SfxFrameHTMLWriter::OutMeta( SvStream& rStrm,
                                  const char *pIndent,
                                  std::u16string_view rName,
                                  std::u16string_view rContent,
                                  bool bHTTPEquiv,
                                  OUString *pNonConvertableChars  )
{
    rStrm.WriteOString( SAL_NEWLINE_STRING );
    if( pIndent )
        rStrm.WriteOString( pIndent );

    OStringBuffer sOut;
    sOut.append("<" OOO_STRING_SVTOOLS_HTML_meta " ")
        .append(bHTTPEquiv ? OOO_STRING_SVTOOLS_HTML_O_httpequiv : OOO_STRING_SVTOOLS_HTML_O_name).append("=\"");
    rStrm.WriteOString( sOut );
    sOut.setLength(0);

    HTMLOutFuncs::Out_String( rStrm, rName, pNonConvertableChars );

    sOut.append("\" " OOO_STRING_SVTOOLS_HTML_O_content "=\"");
    rStrm.WriteOString( sOut );
    sOut.setLength(0);

    HTMLOutFuncs::Out_String( rStrm, rContent, pNonConvertableChars ).WriteOString( "\"/>" );
}

void SfxFrameHTMLWriter::Out_DocInfo( SvStream& rStrm, const OUString& rBaseURL,
        const uno::Reference<document::XDocumentProperties> & i_xDocProps,
        const char *pIndent,
        OUString *pNonConvertableChars    )
{
    OutMeta( rStrm, pIndent, OOO_STRING_SVTOOLS_HTML_META_content_type, u"text/html; charset=utf-8", true,
                 pNonConvertableChars );

    // Title (regardless if empty)
    rStrm.WriteOString( SAL_NEWLINE_STRING );
    if( pIndent )
        rStrm.WriteOString( pIndent );
    HTMLOutFuncs::Out_AsciiTag( rStrm, OOO_STRING_SVTOOLS_HTML_title );
    if( i_xDocProps.is() )
    {
        const OUString aTitle = i_xDocProps->getTitle();
        if( !aTitle.isEmpty() )
            HTMLOutFuncs::Out_String( rStrm, aTitle, pNonConvertableChars );
    }
    HTMLOutFuncs::Out_AsciiTag( rStrm, OOO_STRING_SVTOOLS_HTML_title, false );

    // Target-Frame
    if( i_xDocProps.is() )
    {
        const OUString aTarget = i_xDocProps->getDefaultTarget();
        if( !aTarget.isEmpty() )
        {
            rStrm.WriteOString( SAL_NEWLINE_STRING );
            if( pIndent )
                rStrm.WriteOString( pIndent );

            rStrm.WriteOString( "<" OOO_STRING_SVTOOLS_HTML_base " "
                OOO_STRING_SVTOOLS_HTML_O_target "=\"" );
            HTMLOutFuncs::Out_String( rStrm, aTarget, pNonConvertableChars )
               .WriteOString( "\">" );
        }
    }

    // Who we are
    OUString sGenerator(Translate::ExpandVariables(STR_HTML_GENERATOR));
    OUString os( u"$_OS"_ustr );
    ::rtl::Bootstrap::expandMacros(os);
    sGenerator = sGenerator.replaceFirst( "%1", os );
    OutMeta( rStrm, pIndent, OOO_STRING_SVTOOLS_HTML_META_generator, sGenerator, false, pNonConvertableChars );

    if( !i_xDocProps.is() )
        return;

    // Reload
    if( (i_xDocProps->getAutoloadSecs() != 0) ||
        !i_xDocProps->getAutoloadURL().isEmpty() )
    {
        OUString sContent = OUString::number(
                            i_xDocProps->getAutoloadSecs() );

        const OUString aReloadURL = i_xDocProps->getAutoloadURL();
        if( !aReloadURL.isEmpty() )
        {
            sContent += ";URL=" + URIHelper::simpleNormalizedMakeRelative(
                          rBaseURL, aReloadURL);
        }

        OutMeta( rStrm, pIndent, OOO_STRING_SVTOOLS_HTML_META_refresh, sContent, true,
                 pNonConvertableChars );
    }

    // Author
    const OUString aAuthor = i_xDocProps->getAuthor();
    if( !aAuthor.isEmpty() )
        OutMeta( rStrm, pIndent, OOO_STRING_SVTOOLS_HTML_META_author, aAuthor, false,
                 pNonConvertableChars );

    // created
    ::util::DateTime uDT = i_xDocProps->getCreationDate();
    OUStringBuffer aBuffer;
    ::sax::Converter::convertTimeOrDateTime(aBuffer, uDT);

    OutMeta( rStrm, pIndent, OOO_STRING_SVTOOLS_HTML_META_created, aBuffer.makeStringAndClear(), false,
             pNonConvertableChars );

    // changedby
    const OUString aChangedBy = i_xDocProps->getModifiedBy();
    if( !aChangedBy.isEmpty() )
        OutMeta( rStrm, pIndent, OOO_STRING_SVTOOLS_HTML_META_changedby, aChangedBy, false,
                 pNonConvertableChars );

    // changed
    uDT = i_xDocProps->getModificationDate();
    ::sax::Converter::convertTimeOrDateTime(aBuffer, uDT);

    OutMeta( rStrm, pIndent, OOO_STRING_SVTOOLS_HTML_META_changed, aBuffer.makeStringAndClear(), false,
             pNonConvertableChars );

    // Subject
    const OUString aTheme = i_xDocProps->getSubject();
    if( !aTheme.isEmpty() )
        OutMeta( rStrm, pIndent, OOO_STRING_SVTOOLS_HTML_META_classification, aTheme, false,
                 pNonConvertableChars );

    // Description
    const OUString aComment = i_xDocProps->getDescription();
    if( !aComment.isEmpty() )
        OutMeta( rStrm, pIndent, OOO_STRING_SVTOOLS_HTML_META_description, aComment, false,
                 pNonConvertableChars);

    // Keywords
    OUString Keywords = ::comphelper::string::convertCommaSeparated(
        i_xDocProps->getKeywords());
    if( !Keywords.isEmpty() )
        OutMeta( rStrm, pIndent, OOO_STRING_SVTOOLS_HTML_META_keywords, Keywords, false,
                 pNonConvertableChars);

    uno::Reference < script::XTypeConverter > xConverter( script::Converter::create(
        ::comphelper::getProcessComponentContext() ) );
    uno::Reference<beans::XPropertySet> xUserDefinedProps(
        i_xDocProps->getUserDefinedProperties(), uno::UNO_QUERY_THROW);
    uno::Reference<beans::XPropertySetInfo> xPropInfo =
        xUserDefinedProps->getPropertySetInfo();
    DBG_ASSERT(xPropInfo.is(), "UserDefinedProperties Info is null");
    const uno::Sequence<beans::Property> props = xPropInfo->getProperties();
    for (const auto& rProp : props)
    {
        try
        {
            OUString name = rProp.Name;
            uno::Any aStr = xConverter->convertToSimpleType(
                    xUserDefinedProps->getPropertyValue(name),
                    uno::TypeClass_STRING);
            OUString str;
            aStr >>= str;
            OUString valstr(comphelper::string::stripEnd(str, ' '));
            OutMeta( rStrm, pIndent, name, valstr, false,
                     pNonConvertableChars );
        }
        catch (const uno::Exception&)
        {
            // may happen with concurrent modification...
            SAL_INFO("sfx", "SfxFrameHTMLWriter::Out_DocInfo: exception");
        }
    }
}

void SfxFrameHTMLWriter::Out_FrameDescriptor(
    SvStream& rOut, const OUString& rBaseURL, const uno::Reference < beans::XPropertySet >& xSet )
{
    try
    {
        OStringBuffer sOut;
        OUString aStr;
        uno::Any aAny = xSet->getPropertyValue(u"FrameURL"_ustr);
        if ( (aAny >>= aStr) && !aStr.isEmpty() )
        {
            OUString aURL = INetURLObject( aStr ).GetMainURL( INetURLObject::DecodeMechanism::ToIUri );
            if( !aURL.isEmpty() )
            {
                aURL = URIHelper::simpleNormalizedMakeRelative(
                    rBaseURL, aURL );
                sOut.append(" " OOO_STRING_SVTOOLS_HTML_O_src "=\"");
                rOut.WriteOString( sOut );
                sOut.setLength(0);
                HTMLOutFuncs::Out_String( rOut, aURL );
                sOut.append('\"');
            }
        }

        aAny = xSet->getPropertyValue(u"FrameName"_ustr);
        if ( (aAny >>= aStr) && !aStr.isEmpty() )
        {
            sOut.append(" " OOO_STRING_SVTOOLS_HTML_O_name "=\"");
            rOut.WriteOString( sOut );
            sOut.setLength(0);
            HTMLOutFuncs::Out_String( rOut, aStr );
            sOut.append('\"');
        }

        sal_Int32 nVal = SIZE_NOT_SET;
        aAny = xSet->getPropertyValue(u"FrameMarginWidth"_ustr);
        if ( (aAny >>= nVal) && nVal != SIZE_NOT_SET )
        {
            sOut.append(" " OOO_STRING_SVTOOLS_HTML_O_marginwidth
                    "=" + OString::number(nVal));
        }
        aAny = xSet->getPropertyValue(u"FrameMarginHeight"_ustr);
        if ( (aAny >>= nVal) && nVal != SIZE_NOT_SET )
        {
            sOut.append(" " OOO_STRING_SVTOOLS_HTML_O_marginheight
                    "=" + OString::number(nVal));
        }

        bool bVal = true;
        aAny = xSet->getPropertyValue(u"FrameIsAutoScroll"_ustr);
        if ( (aAny >>= bVal) && !bVal )
        {
            aAny = xSet->getPropertyValue(u"FrameIsScrollingMode"_ustr);
            if ( aAny >>= bVal )
            {
                const char *pStr = bVal ? sHTML_SC_yes : sHTML_SC_no;
                sOut.append(OString::Concat(" " OOO_STRING_SVTOOLS_HTML_O_scrolling) +
                        pStr);
            }
        }

        // frame border (MS+Netscape-Extension)
        aAny = xSet->getPropertyValue(u"FrameIsAutoBorder"_ustr);
        if ( (aAny >>= bVal) && !bVal )
        {
            aAny = xSet->getPropertyValue(u"FrameIsBorder"_ustr);
            if ( aAny >>= bVal )
            {
                const char* pStr = bVal ? sHTML_SC_yes : sHTML_SC_no;
                sOut.append(OString::Concat(" " OOO_STRING_SVTOOLS_HTML_O_frameborder
                        "=") + pStr);
            }
        }
        rOut.WriteOString( sOut );
        sOut.setLength(0);
    }
    catch (const uno::Exception&)
    {
    }
}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
