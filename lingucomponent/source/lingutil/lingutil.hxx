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

#ifndef INCLUDED_LINGUCOMPONENT_SOURCE_LINGUTIL_LINGUTIL_HXX
#define INCLUDED_LINGUCOMPONENT_SOURCE_LINGUTIL_LINGUTIL_HXX

#include <rtl/string.hxx>

#include <vector>

#define OU2ENC(rtlOUString, rtlEncoding) \
    OString((rtlOUString).getStr(), (rtlOUString).getLength(), \
    rtlEncoding, RTL_UNICODETOTEXT_FLAGS_UNDEFINED_QUESTIONMARK)

struct SvtLinguConfigDictionaryEntry;

#if defined(_WIN32)

// to be use to get a path name with long path prefix
// under Windows for Hunspell, Hyphen and MyThes libraries
OString Win_AddLongPathPrefix( const OString &rPathName );
#endif


// temporary function, to be removed when new style dictionaries
// using configuration entries are fully implemented and provided
std::vector< SvtLinguConfigDictionaryEntry > GetOldStyleDics( const char * pDicType );
void MergeNewStyleDicsAndOldStyleDics( std::vector< SvtLinguConfigDictionaryEntry > &rNewStyleDics, const std::vector< SvtLinguConfigDictionaryEntry > &rOldStyleDics );

//Find an encoding from a charset string, using
//rtl_getTextEncodingFromMimeCharset and falling back to
//rtl_getTextEncodingFromUnixCharset with the addition of
//ISCII-DEVANAGARI. On failure will return final fallback of
//RTL_TEXTENCODING_ISO_8859_1
rtl_TextEncoding getTextEncodingFromCharset(const char* pCharset);

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
