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

#include <oox/vml/vmlinputstream.hxx>

#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/io/XTextInputStream2.hpp>
#include <map>
#include <string.h>
#include <rtl/strbuf.hxx>
#include <osl/diagnose.h>
#include <oox/helper/textinputstream.hxx>

namespace oox::vml {

using namespace ::com::sun::star::io;
using namespace ::com::sun::star::uno;

namespace {

const char* lclFindCharacter( const char* pcBeg, const char* pcEnd, char cChar )
{
    sal_Int32 nIndex = rtl_str_indexOfChar_WithLength( pcBeg, static_cast< sal_Int32 >( pcEnd - pcBeg ), cChar );
    return (nIndex < 0) ? pcEnd : (pcBeg + nIndex);
}

bool lclIsWhiteSpace( char cChar )
{
    return cChar >= 0 && cChar <= 32;
}

const char* lclFindWhiteSpace( const char* pcBeg, const char* pcEnd )
{
    for( ; pcBeg < pcEnd; ++pcBeg )
        if( lclIsWhiteSpace( *pcBeg ) )
            return pcBeg;
    return pcEnd;
}

const char* lclFindNonWhiteSpace( const char* pcBeg, const char* pcEnd )
{
    for( ; pcBeg < pcEnd; ++pcBeg )
        if( !lclIsWhiteSpace( *pcBeg ) )
            return pcBeg;
    return pcEnd;
}

const char* lclTrimWhiteSpaceFromEnd( const char* pcBeg, const char* pcEnd )
{
    while( (pcBeg < pcEnd) && lclIsWhiteSpace( pcEnd[ -1 ] ) )
        --pcEnd;
    return pcEnd;
}

void lclAppendToBuffer( OStringBuffer& rBuffer, const char* pcBeg, const char* pcEnd )
{
    rBuffer.append( pcBeg, static_cast< sal_Int32 >( pcEnd - pcBeg ) );
}

void lclProcessAttribs( OStringBuffer& rBuffer, const char* pcBeg, const char* pcEnd )
{
    /*  Map attribute names to char-pointer of all attributes. This map is used
        to find multiple occurrences of attributes with the same name. The
        mapped pointers are used as map key in the next map below. */
    typedef ::std::map< OString, const char* > AttributeNameMap;
    AttributeNameMap aAttributeNames;

    /*  Map the char-pointers of all attributes to the full attribute definition
        string. This preserves the original order of the used attributes. */
    typedef ::std::map< const char*, OString > AttributeDataMap;
    AttributeDataMap aAttributes;

    bool bOk = true;
    const char* pcNameBeg = pcBeg;
    while( bOk && (pcNameBeg < pcEnd) )
    {
        // pcNameBeg points to begin of attribute name, find equality sign
        const char* pcEqualSign = lclFindCharacter( pcNameBeg, pcEnd, '=' );
        bOk = (pcEqualSign < pcEnd);
        if (bOk)
        {
            // find end of attribute name (ignore whitespace between name and equality sign)
            const char* pcNameEnd = lclTrimWhiteSpaceFromEnd( pcNameBeg, pcEqualSign );
            bOk = (pcNameBeg < pcNameEnd);
            if( bOk )
            {
                // find begin of attribute value (must be single or double quote)
                const char* pcValueBeg = lclFindNonWhiteSpace( pcEqualSign + 1, pcEnd );
                bOk = (pcValueBeg < pcEnd) && ((*pcValueBeg == '\'') || (*pcValueBeg == '"'));
                if( bOk )
                {
                    // find end of attribute value (matching quote character)
                    const char* pcValueEnd = lclFindCharacter( pcValueBeg + 1, pcEnd, *pcValueBeg );
                    bOk = (pcValueEnd < pcEnd);
                    if( bOk )
                    {
                        ++pcValueEnd;
                        OString aAttribName( pcNameBeg, static_cast< sal_Int32 >( pcNameEnd - pcNameBeg ) );
                        OString aAttribData( pcNameBeg, static_cast< sal_Int32 >( pcValueEnd - pcNameBeg ) );
                        // search for an existing attribute with the same name
                        AttributeNameMap::iterator aIt = aAttributeNames.find( aAttribName );
                        // remove its definition from the data map
                        if( aIt != aAttributeNames.end() )
                            aAttributes.erase( aIt->second );
                        // insert the attribute into both maps
                        aAttributeNames[ aAttribName ] = pcNameBeg;
                        aAttributes[ pcNameBeg ] = aAttribData;
                        // continue with next attribute (skip whitespace after this attribute)
                        pcNameBeg = pcValueEnd;
                        if( pcNameBeg < pcEnd )
                        {
                            bOk = lclIsWhiteSpace( *pcNameBeg );
                            if( bOk )
                                pcNameBeg = lclFindNonWhiteSpace( pcNameBeg + 1, pcEnd );
                        }
                    }
                }
            }
        }
    }

    // if no error has occurred, build the resulting attribute list
    if( bOk )
        for (auto const& attrib : aAttributes)
            rBuffer.append( " " + attrib.second );
    // on error, just append the complete passed string
    else
        lclAppendToBuffer( rBuffer, pcBeg, pcEnd );
}

void lclProcessElement( OStringBuffer& rBuffer, const OString& rElement )
{
    // check that passed string starts and ends with the brackets of an XML element
    sal_Int32 nElementLen = rElement.getLength();
    if( nElementLen == 0 )
        return;

    const char* pcOpen = rElement.getStr();
    const char* pcClose = pcOpen + nElementLen - 1;

    // no complete element found
    if( (pcOpen >= pcClose) || (*pcOpen != '<') || (*pcClose != '>') )
    {
        // just append all passed characters
        rBuffer.append( rElement );
    }

    // skip parser instructions: '<![...]>'
    else if( (nElementLen >= 5) && (pcOpen[ 1 ] == '!') && (pcOpen[ 2 ] == '[') && (pcClose[ -1 ] == ']') )
    {
        // do nothing
    }

    // just append any xml prolog (text directive) or processing instructions: <?...?>
    else if( (nElementLen >= 4) && (pcOpen[ 1 ] == '?') && (pcClose[ -1 ] == '?') )
    {
        rBuffer.append( rElement );
    }

    // replace '<br>' element with newline
    else if( (nElementLen >= 4) && (pcOpen[ 1 ] == 'b') && (pcOpen[ 2 ] == 'r') && (lclFindNonWhiteSpace( pcOpen + 3, pcClose ) == pcClose) )
    {
        rBuffer.append( '\n' );
    }

    // check start elements and simple elements for repeated attributes
    else if( pcOpen[ 1 ] != '/' )
    {
        // find positions of text content inside brackets, exclude '/' in '<simpleelement/>'
        const char* pcContentBeg = pcOpen + 1;
        bool bIsEmptyElement = pcClose[ -1 ] == '/';
        const char* pcContentEnd = bIsEmptyElement ? (pcClose - 1) : pcClose;
        // append opening bracket and element name to buffer
        const char* pcWhiteSpace = lclFindWhiteSpace( pcContentBeg, pcContentEnd );
        lclAppendToBuffer( rBuffer, pcOpen, pcWhiteSpace );
        // find begin of attributes, and process all attributes
        const char* pcAttribBeg = lclFindNonWhiteSpace( pcWhiteSpace, pcContentEnd );
        if( pcAttribBeg < pcContentEnd )
            lclProcessAttribs( rBuffer, pcAttribBeg, pcContentEnd );
        // close the element
        if( bIsEmptyElement )
            rBuffer.append( '/' );
        rBuffer.append( '>' );
    }

    // append end elements without further processing
    else
    {
        rBuffer.append( rElement );
    }
}

bool lclProcessCharacters( OStringBuffer& rBuffer, const OString& rChars )
{
    /*  MSO has a very weird way to store and handle whitespaces. The stream
        may contain lots of spaces, tabs, and newlines which have to be handled
        as single space character. This will be done in this function.

        If the element text contains a literal line break, it will be stored as
        <br> tag (without matching </br> element). This input stream wrapper
        will replace this element with a literal LF character (see below).

        A single space character for its own is stored as is. Example: The
        element
            <font> </font>
        represents a single space character. The XML parser will ignore this
        space character completely without issuing a 'characters' event. The
        VML import filter implementation has to react on this case manually.

        A single space character following another character is stored
        literally and must not be stripped away here. Example: The element
            <font>abc </font>
        contains the three letters a, b, and c, followed by a space character.

        Consecutive space characters, or a leading single space character, are
        stored in a <span> element. If there are N space characters (N > 1),
        then the <span> element contains exactly (N-1) NBSP (non-breaking
        space) characters, followed by a regular space character. Examples:
        The element
            <font><span style='mso-spacerun:yes'>\xA0\xA0\xA0 </span></font>
        represents 4 consecutive space characters. Has to be handled by the
        implementation. The element
            <font><span style='mso-spacerun:yes'> abc</span></font>
        represents a space characters followed by the letters a, b, c. These
        strings have to be handled by the VML import filter implementation.
     */

    // passed string ends with the leading opening bracket of an XML element
    const char* pcBeg = rChars.getStr();
    const char* pcEnd = pcBeg + rChars.getLength();
    bool bHasBracket = (pcBeg < pcEnd) && (pcEnd[ -1 ] == '<');
    if( bHasBracket ) --pcEnd;

    // skip leading whitespace
    const char* pcContentsBeg = lclFindNonWhiteSpace( pcBeg, pcEnd );
    while( pcContentsBeg < pcEnd )
    {
        const char* pcWhitespaceBeg = lclFindWhiteSpace( pcContentsBeg + 1, pcEnd );
        lclAppendToBuffer( rBuffer, pcContentsBeg, pcWhitespaceBeg );
        if( pcWhitespaceBeg < pcEnd )
            rBuffer.append( ' ' );
        pcContentsBeg = lclFindNonWhiteSpace( pcWhitespaceBeg, pcEnd );
    }

    return bHasBracket;
}

} // namespace

constexpr OStringLiteral gaOpeningCData( "<![CDATA[" );
constexpr OString gaClosingCData( "]]>"_ostr );

InputStream::InputStream( const Reference< XComponentContext >& rxContext, const Reference< XInputStream >& rxInStrm ) :
    // use single-byte ISO-8859-1 encoding which maps all byte characters to the first 256 Unicode characters
    mxTextStrm( TextInputStream::createXTextInputStream( rxContext, rxInStrm, RTL_TEXTENCODING_ISO_8859_1 ) ),
    maOpeningBracket{ '<' },
    maClosingBracket{ '>' },
    mnBufferPos( 0 )
{
    if (!mxTextStrm.is())
        throw IOException();
}

InputStream::~InputStream()
{
}

sal_Int32 SAL_CALL InputStream::readBytes( Sequence< sal_Int8 >& rData, sal_Int32 nBytesToRead )
{
    if( nBytesToRead < 0 )
        throw IOException();

    rData.realloc( nBytesToRead );
    sal_Int8* pcDest = rData.getArray();
    sal_Int32 nRet = 0;
    while( (nBytesToRead > 0) && !mxTextStrm->isEOF() )
    {
        updateBuffer();
        sal_Int32 nReadSize = ::std::min( nBytesToRead, maBuffer.getLength() - mnBufferPos );
        if( nReadSize > 0 )
        {
            memcpy( pcDest + nRet, maBuffer.getStr() + mnBufferPos, static_cast< size_t >( nReadSize ) );
            mnBufferPos += nReadSize;
            nBytesToRead -= nReadSize;
            nRet += nReadSize;
        }
    }
    if( nRet < rData.getLength() )
        rData.realloc( nRet );
    return nRet;
}

sal_Int32 SAL_CALL InputStream::readSomeBytes( Sequence< sal_Int8 >& rData, sal_Int32 nMaxBytesToRead )
{
    return readBytes( rData, nMaxBytesToRead );
}

void SAL_CALL InputStream::skipBytes( sal_Int32 nBytesToSkip )
{
    if( nBytesToSkip < 0 )
        throw IOException();

    while( (nBytesToSkip > 0) && !mxTextStrm->isEOF() )
    {
        updateBuffer();
        sal_Int32 nSkipSize = ::std::min( nBytesToSkip, maBuffer.getLength() - mnBufferPos );
        mnBufferPos += nSkipSize;
        nBytesToSkip -= nSkipSize;
    }
}

sal_Int32 SAL_CALL InputStream::available()
{
    updateBuffer();
    return maBuffer.getLength() - mnBufferPos;
}

void SAL_CALL InputStream::closeInput()
{
    mxTextStrm->closeInput();
}

// private --------------------------------------------------------------------

void InputStream::updateBuffer()
{
    while( (mnBufferPos >= maBuffer.getLength()) && !mxTextStrm->isEOF() )
    {
        // collect new contents in a string buffer
        OStringBuffer aBuffer;

        // read and process characters until the opening bracket of the next XML element
        OString aChars = readToElementBegin();
        bool bHasOpeningBracket = lclProcessCharacters( aBuffer, aChars );

        // read and process characters until (and including) closing bracket (an XML element)
        OSL_ENSURE( bHasOpeningBracket || mxTextStrm->isEOF(), "InputStream::updateBuffer - missing opening bracket of XML element" );
        if( bHasOpeningBracket && !mxTextStrm->isEOF() )
        {
            // read the element text (add the leading opening bracket manually)
            OString aElement = "<" + readToElementEnd();
            // check for CDATA part, starting with '<![CDATA['
            if( aElement.match( gaOpeningCData ) )
            {
                // search the end tag ']]>'
                while( ((aElement.getLength() < gaClosingCData.getLength()) || !aElement.endsWith( gaClosingCData )) && !mxTextStrm->isEOF() )
                    aElement += readToElementEnd();
                // copy the entire CDATA part
                aBuffer.append( aElement );
            }
            else
            {
                // no CDATA part - process the contents of the element
                lclProcessElement( aBuffer, aElement );
            }
        }

        maBuffer = aBuffer.makeStringAndClear();
        mnBufferPos = 0;
    }
}

OString InputStream::readToElementBegin()
{
    return OUStringToOString( mxTextStrm->readString( maOpeningBracket, false ), RTL_TEXTENCODING_ISO_8859_1 );
}

OString InputStream::readToElementEnd()
{
    OString aText = OUStringToOString( mxTextStrm->readString( maClosingBracket, false ), RTL_TEXTENCODING_ISO_8859_1 );
    OSL_ENSURE( aText.endsWith(">"), "InputStream::readToElementEnd - missing closing bracket of XML element" );
    return aText;
}

} // namespace oox::vml

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
