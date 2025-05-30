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

#include <comphelper/string.hxx>
#include <svtools/parhtml.hxx>
#include <svtools/htmltokn.h>
#include <svtools/htmlkywd.hxx>
#include <tools/urlobj.hxx>

// Table for converting option values into strings
HTMLOptionEnum<HTMLScriptLanguage> const aScriptLangOptEnums[] =
{
    { OOO_STRING_SVTOOLS_HTML_LG_starbasic,    HTMLScriptLanguage::StarBasic     },
    { OOO_STRING_SVTOOLS_HTML_LG_javascript,   HTMLScriptLanguage::JavaScript    },
    { OOO_STRING_SVTOOLS_HTML_LG_javascript11, HTMLScriptLanguage::JavaScript    },
    { OOO_STRING_SVTOOLS_HTML_LG_livescript,   HTMLScriptLanguage::JavaScript    },
    { nullptr,                                 HTMLScriptLanguage(0) }
};

void HTMLParser::ParseScriptOptions( OUString& rLangString, std::u16string_view rBaseURL,
                                     HTMLScriptLanguage& rLang,
                                     OUString& rSrc,
                                     OUString& rLibrary,
                                     OUString& rModule )
{
    const HTMLOptions& aScriptOptions = GetOptions();

    rLangString.clear();
    rLang = HTMLScriptLanguage::JavaScript;
    rSrc.clear();
    rLibrary.clear();
    rModule.clear();

    for( size_t i = aScriptOptions.size(); i; )
    {
        const HTMLOption& aOption = aScriptOptions[--i];
        switch( aOption.GetToken() )
        {
        case HtmlOptionId::LANGUAGE:
            {
                rLangString = aOption.GetString();
                HTMLScriptLanguage nLang;
                if( aOption.GetEnum( nLang, aScriptLangOptEnums ) )
                    rLang = nLang;
                else
                    rLang = HTMLScriptLanguage::Unknown;
            }
            break;

        case HtmlOptionId::SRC:
            rSrc = INetURLObject::GetAbsURL( rBaseURL, aOption.GetString() );
            break;
        case HtmlOptionId::SDLIBRARY:
            rLibrary = aOption.GetString();
            break;

        case HtmlOptionId::SDMODULE:
            rModule = aOption.GetString();
            break;
        default: break;
        }
    }
}

void HTMLParser::RemoveSGMLComment( OUString &rString )
{
    sal_Unicode c = 0;
    sal_Int32 idx = 0;
    while (idx < rString.getLength())
    {
        c = rString[idx];
        if (!( c==' ' || c=='\t' || c=='\r' || c=='\n' ) )
            break;
        idx++;
    }
    if (idx)
        rString = rString.copy( idx );

    idx = rString.getLength() - 1;
    while (idx > 0)
        // Can never get to 0 because that would mean the string contains only whitespace, and the first
        // loop would already have removed all of those.
    {
        c = rString[idx];
        if (!( c==' ' || c=='\t' || c=='\r' || c=='\n' ) )
            break;
        idx--;
    }
    if (idx != rString.getLength() - 1)
        rString = rString.copy( 0, idx + 1 );

    // remove SGML comments
    if( rString.startsWith( "<!--" ) )
    {
        // the whole line
        sal_Int32 nPos = 4;
        while( nPos < rString.getLength() )
        {
            c = rString[nPos];
            if (c == '\r' || c == '\n')
                break;
            ++nPos;
        }
        if( c == '\r' && nPos+1 < rString.getLength() &&
            '\n' == rString[nPos+1] )
            ++nPos;
        else if( c != '\n' )
            nPos = 3;
        ++nPos;
        rString = rString.copy( nPos );
    }

    if( !rString.endsWith("-->") )
        return;

    rString = rString.copy( 0, rString.getLength()-3 );
    // "//" or "'", maybe preceding CR/LF
    rString = comphelper::string::stripEnd(rString, ' ');
    sal_Int32 nDel = 0, nLen = rString.getLength();
    if( nLen >= 2 &&
        rString.endsWith("//") )
    {
        nDel = 2;
    }
    else if( nLen && '\'' == rString[nLen-1] )
    {
        nDel = 1;
    }
    if( nDel && nLen >= nDel+1 )
    {
        c = rString[nLen-(nDel+1)];
        if( '\r'==c || '\n'==c )
        {
            nDel++;
            if( '\n'==c && nLen >= nDel+1 &&
                '\r'==rString[nLen-(nDel+1)] )
                nDel++;
        }
    }
    rString = rString.copy( 0, nLen-nDel );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
