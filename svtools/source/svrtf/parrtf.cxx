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
#include <sal/log.hxx>

#include <comphelper/scopeguard.hxx>

#include <rtl/character.hxx>
#include <rtl/strbuf.hxx>
#include <rtl/tencinfo.h>
#include <rtl/ustrbuf.hxx>
#include <tools/stream.hxx>
#include <tools/debug.hxx>
#include <svtools/rtftoken.h>
#include <svtools/parrtf.hxx>

const int MAX_STRING_LEN = 1024;

#define RTF_ISDIGIT( c ) rtl::isAsciiDigit(c)
#define RTF_ISALPHA( c ) rtl::isAsciiAlpha(c)

SvRTFParser::SvRTFParser( SvStream& rIn, sal_uInt8 nStackSize )
    : SvParser<int>( rIn, nStackSize )
    , nOpenBrackets(0)
    , nUPRLevel(0)
    , eCodeSet(RTL_TEXTENCODING_MS_1252)
    , nUCharOverread(1)
{
    // default is ANSI-CodeSet
    SetSrcEncoding( RTL_TEXTENCODING_MS_1252 );
    bRTF_InTextRead = false;
}

SvRTFParser::~SvRTFParser()
{
}


int SvRTFParser::GetNextToken_()
{
    int nRet = 0;
    do {
        bool bNextCh = true;
        switch( nNextCh )
        {
        case '\\':
            {
                // control characters
                nNextCh = GetNextChar();
                switch( nNextCh )
                {
                case '{':
                case '}':
                case '\\':
                case '+':       // I found it in a RTF-file
                case '~':       // nonbreaking space
                case '-':       // optional hyphen
                case '_':       // nonbreaking hyphen
                case '\'':      // HexValue
                    nNextCh = '\\';
                    rInput.SeekRel( -1 );
                    ScanText();
                    nRet = RTF_TEXTTOKEN;
                    bNextCh = 0 == nNextCh;
                    break;

                case '*':       // ignoreflag
                    nRet = RTF_IGNOREFLAG;
                    break;
                case ':':       // subentry in an index entry
                    nRet = RTF_SUBENTRYINDEX;
                    break;
                case '|':       // formula-character
                    nRet = RTF_FORMULA;
                    break;

                case 0x0a:
                case 0x0d:
                    nRet = RTF_PAR;
                    break;

                default:
                    if( RTF_ISALPHA( nNextCh ) )
                    {
                        aToken = "\\";
                        {
                            do {
                                aToken.appendUtf32(nNextCh);
                                nNextCh = GetNextChar();
                            } while( RTF_ISALPHA( nNextCh ) );
                        }

                        // minus before numeric parameters
                        bool bNegValue = false;
                        if( '-' == nNextCh )
                        {
                            bNegValue = true;
                            nNextCh = GetNextChar();
                        }

                        // possible numeric parameter
                        if( RTF_ISDIGIT( nNextCh ) )
                        {
                            OUStringBuffer aNumber;
                            do {
                                aNumber.append(static_cast<sal_Unicode>(nNextCh));
                                nNextCh = GetNextChar();
                            } while( RTF_ISDIGIT( nNextCh ) );
                            nTokenValue = OUString::unacquired(aNumber).toInt32();
                            if( bNegValue )
                                nTokenValue = -nTokenValue;
                            bTokenHasValue=true;
                        }
                        else if( bNegValue )        // restore minus
                        {
                            nNextCh = '-';
                            rInput.SeekRel( -1 );
                        }
                        if( ' ' == nNextCh )        // blank is part of token!
                            nNextCh = GetNextChar();

                        // search for the token in the table:
                        if( 0 == (nRet = GetRTFToken( aToken )) )
                            // Unknown Control
                            nRet = RTF_UNKNOWNCONTROL;

                        // bug 76812 - unicode token handled as normal text
                        bNextCh = false;
                        switch( nRet )
                        {
                        case RTF_UC:
                            if( 0 <= nTokenValue )
                            {
                                nUCharOverread = static_cast<sal_uInt8>(nTokenValue);
                                if (!aParserStates.empty())
                                {
                                    //cmc: other ifdef breaks #i3584
                                    aParserStates.top().nUCharOverread = nUCharOverread;
                                }
                            }
                            aToken.setLength( 0 ); // #i47831# erase token to prevent the token from being treated as text
                            // read next token
                            nRet = 0;
                            break;

                        case RTF_UPR:
                            if (!_inSkipGroup)
                            {
                                if (nUPRLevel > 256) // fairly sure > 1 is probably an error, but provide some leeway
                                {
                                    SAL_WARN("svtools", "urp stack too deep");
                                    eState = SvParserState::Error;
                                    break;
                                }

                                ++nUPRLevel;

                                // UPR - overread the group with the ansi
                                //       information
                                int nNextToken;
                                do
                                {
                                    nNextToken = GetNextToken_();
                                }
                                while (nNextToken != '{' && nNextToken != sal_Unicode(EOF) && IsParserWorking());

                                SkipGroup();
                                GetNextToken_();  // overread the last bracket
                                nRet = 0;

                                --nUPRLevel;
                            }
                            break;

                        case RTF_U:
                            if( !bRTF_InTextRead )
                            {
                                nRet = RTF_TEXTTOKEN;
                                aToken = OUStringChar( static_cast<sal_Unicode>(nTokenValue) );

                                // overread the next n "RTF" characters. This
                                // can be also \{, \}, \'88
                                for( sal_uInt8 m = 0; m < nUCharOverread; ++m )
                                {
                                    sal_uInt32 cAnsi = nNextCh;
                                    while( 0xD == cAnsi )
                                        cAnsi = GetNextChar();
                                    while( 0xA == cAnsi )
                                        cAnsi = GetNextChar();

                                    if( '\\' == cAnsi &&
                                        '\'' == GetNextChar() )
                                        // skip HexValue
                                        GetHexValue();
                                    nNextCh = GetNextChar();
                                }
                                ScanText();
                                bNextCh = 0 == nNextCh;
                            }
                            break;
                        }
                    }
                    else if( SvParserState::Pending != eState )
                    {
                        // Bug 34631 - "\ " read on - Blank as character
                        // eState = SvParserState::Error;
                        bNextCh = false;
                    }
                    break;
                }
            }
            break;

        case sal_Unicode(EOF):
            eState = SvParserState::Accepted;
            nRet = nNextCh;
            break;

        case '{':
            {
                if( 0 <= nOpenBrackets )
                {
                    RtfParserState_Impl aState( nUCharOverread, GetSrcEncoding() );
                    aParserStates.push( aState );
                }
                ++nOpenBrackets;
                DBG_ASSERT(
                    static_cast<size_t>(nOpenBrackets) == aParserStates.size(),
                    "ParserStateStack unequal to bracket count" );
                nRet = nNextCh;
            }
            break;

        case '}':
            --nOpenBrackets;
            if( 0 <= nOpenBrackets )
            {
                aParserStates.pop();
                if( !aParserStates.empty() )
                {
                    const RtfParserState_Impl& rRPS =
                            aParserStates.top();
                    nUCharOverread = rRPS.nUCharOverread;
                    SetSrcEncoding( rRPS.eCodeSet );
                }
                else
                {
                    nUCharOverread = 1;
                    SetSrcEncoding( GetCodeSet() );
                }
            }
            DBG_ASSERT(
                static_cast<size_t>(nOpenBrackets) == aParserStates.size(),
                "ParserStateStack unequal to bracket count" );
            nRet = nNextCh;
            break;

        case 0x0d:
        case 0x0a:
            break;

        default:
            // now normal text follows
            ScanText();
            nRet = RTF_TEXTTOKEN;
            bNextCh = 0 == nNextCh;
            break;
        }

        if( bNextCh )
            nNextCh = GetNextChar();

    } while( !nRet && SvParserState::Working == eState );
    return nRet;
}


sal_Unicode SvRTFParser::GetHexValue()
{
    // collect Hex values
    int n;
    sal_Unicode nHexVal = 0;

    for( n = 0; n < 2; ++n )
    {
        nHexVal *= 16;
        nNextCh = GetNextChar();
        if( nNextCh >= '0' && nNextCh <= '9' )
            nHexVal += (nNextCh - 48);
        else if( nNextCh >= 'a' && nNextCh <= 'f' )
            nHexVal += (nNextCh - 87);
        else if( nNextCh >= 'A' && nNextCh <= 'F' )
            nHexVal += (nNextCh - 55);
    }
    return nHexVal;
}

void SvRTFParser::ScanText()
{
    const sal_Unicode cBreak = 0;
    OUStringBuffer aStrBuffer;
    bool bContinue = true;
    while( bContinue && IsParserWorking() && aStrBuffer.getLength() < MAX_STRING_LEN)
    {
        bool bNextCh = true;
        switch( nNextCh )
        {
        case '\\':
            {
                nNextCh = GetNextChar();
                switch (nNextCh)
                {
                case '\'':
                    {

                        OStringBuffer aByteString;
                        while (true)
                        {
                            char c = static_cast<char>(GetHexValue());
                            /*
                             * Note: \'00 is a valid internal character in  a
                             * string in RTF. OStringBuffer supports
                             * appending nulls fine
                             */
                            aByteString.append(c);

                            bool bBreak = false;
                            bool bEOF = false;
                            char nSlash = '\\';
                            while (!bBreak)
                            {
                                auto next = GetNextChar();
                                if (sal_Unicode(EOF) == next)
                                {
                                    bEOF = true;
                                    break;
                                }
                                if (next>0xFF) // fix for #i43933# and #i35653#
                                {
                                    if (!aByteString.isEmpty())
                                    {
                                        aStrBuffer.append( OStringToOUString(aByteString, GetSrcEncoding()) );
                                        aByteString.setLength(0);
                                    }
                                    aStrBuffer.append(static_cast<sal_Unicode>(next));

                                    continue;
                                }
                                nSlash = static_cast<char>(next);
                                while (nSlash == 0xD || nSlash == 0xA)
                                    nSlash = static_cast<char>(GetNextChar());

                                switch (nSlash)
                                {
                                    case '{':
                                    case '}':
                                    case '\\':
                                        bBreak = true;
                                        break;
                                    default:
                                        aByteString.append(nSlash);
                                        break;
                                }
                            }

                            if (bEOF)
                            {
                                bContinue = false;        // abort, string together
                                break;
                            }

                            nNextCh = GetNextChar();

                            if (nSlash != '\\' || nNextCh != '\'')
                            {
                                rInput.SeekRel(-1);
                                nNextCh = static_cast<unsigned char>(nSlash);
                                break;
                            }
                        }

                        bNextCh = false;

                        if (!aByteString.isEmpty())
                        {
                            aStrBuffer.append( OStringToOUString(aByteString, GetSrcEncoding()) );
                            aByteString.setLength(0);
                        }
                    }
                    break;
                case '\\':
                case '}':
                case '{':
                case '+':       // I found in a RTF file
                    aStrBuffer.append(sal_Unicode(nNextCh));
                    break;
                case '~':       // nonbreaking space
                    aStrBuffer.append(u'\x00A0');
                    break;
                case '-':       // optional hyphen
                    aStrBuffer.append(u'\x00AD');
                    break;
                case '_':       // nonbreaking hyphen
                    aStrBuffer.append(u'\x2011');
                    break;

                case 'u':
                    // read UNI-Code characters
                    {
                        nNextCh = GetNextChar();
                        rInput.SeekRel( -2 );

                        if( '-' == nNextCh || RTF_ISDIGIT( nNextCh ) )
                        {
                            bRTF_InTextRead = true;

                            OUString sSave( aToken ); // GetNextToken_() overwrites this
                            nNextCh = '\\';
                            int nToken = GetNextToken_();
                            DBG_ASSERT( RTF_U == nToken, "still not a UNI-Code character" );
                            // don't convert symbol chars
                            aStrBuffer.append(static_cast< sal_Unicode >(nTokenValue));

                            // overread the next n "RTF" characters. This
                            // can be also \{, \}, \'88
                            for( sal_uInt8 m = 0; m < nUCharOverread; ++m )
                            {
                                sal_Unicode cAnsi = nNextCh;
                                while( 0xD == cAnsi )
                                    cAnsi = GetNextChar();
                                while( 0xA == cAnsi )
                                    cAnsi = GetNextChar();

                                if( '\\' == cAnsi &&
                                    '\'' == GetNextChar() )
                                    // skip HexValue
                                    GetHexValue();
                                nNextCh = GetNextChar();
                            }
                            bNextCh = false;
                            aToken = sSave;
                            bRTF_InTextRead = false;
                        }
                        else if ( 'c' == nNextCh )
                        {
                            // Prevent text breaking into multiple tokens.
                            rInput.SeekRel( 2 );
                            nNextCh = GetNextChar();
                            if (RTF_ISDIGIT( nNextCh ))
                            {
                                sal_uInt8 nNewOverread = 0 ;
                                do {
                                    nNewOverread *= 10;
                                    nNewOverread += nNextCh - '0';
                                    nNextCh = GetNextChar();
                                } while ( RTF_ISDIGIT( nNextCh ) );
                                nUCharOverread = nNewOverread;
                                if (!aParserStates.empty())
                                    aParserStates.top().nUCharOverread = nNewOverread;
                            }
                            bNextCh = 0x20 == nNextCh;
                        }
                        else
                        {
                            nNextCh = '\\';
                            bContinue = false;        // abort, string together
                        }
                    }
                    break;

                default:
                    rInput.SeekRel( -1 );
                    nNextCh = '\\';
                    bContinue = false;        // abort, string together
                    break;
                }
            }
            break;

        case sal_Unicode(EOF):
            eState = SvParserState::Error;
            [[fallthrough]];
        case '{':
        case '}':
            bContinue = false;
            break;

        case 0x0a:
        case 0x0d:
            break;

        default:
            if( nNextCh == cBreak || aStrBuffer.getLength() >= MAX_STRING_LEN)
                bContinue = false;
            else
            {
                do {
                    // all other characters end up in the text
                    aStrBuffer.appendUtf32(nNextCh);

                    if (sal_Unicode(EOF) == (nNextCh = GetNextChar()))
                    {
                        if (!aStrBuffer.isEmpty())
                            aToken.append( aStrBuffer );
                        return;
                    }
                } while
                (
                    (RTF_ISALPHA(nNextCh) || RTF_ISDIGIT(nNextCh)) &&
                    (aStrBuffer.getLength() < MAX_STRING_LEN)
                );
                bNextCh = false;
            }
        }

        if( bContinue && bNextCh )
            nNextCh = GetNextChar();
    }

    if (!aStrBuffer.isEmpty())
        aToken.append( aStrBuffer );
}


short SvRTFParser::_inSkipGroup=0;

void SvRTFParser::SkipGroup()
{
    short nBrackets=1;
    if (_inSkipGroup>0)
        return;
    _inSkipGroup++;
//#i16185# faking \bin keyword
    do
    {
        switch (nNextCh)
        {
            case '{':
                ++nBrackets;
                break;
            case '}':
                if (!--nBrackets) {
                    _inSkipGroup--;
                    return;
                }
                break;
        }
        int nToken = GetNextToken_();
        if (nToken == RTF_BIN)
        {
            rInput.SeekRel(-1);
            SAL_WARN_IF(nTokenValue < 0, "svtools", "negative value argument for rtf \\bin keyword");
            if (nTokenValue > 0)
                rInput.SeekRel(nTokenValue);
            nNextCh = GetNextChar();
        }
        while (nNextCh==0xa || nNextCh==0xd)
        {
            nNextCh = GetNextChar();
        }
    } while (sal_Unicode(EOF) != nNextCh && IsParserWorking());

    if( SvParserState::Pending != eState && '}' != nNextCh )
        eState = SvParserState::Error;
    _inSkipGroup--;
}

void SvRTFParser::ReadUnknownData() { SkipGroup(); }
void SvRTFParser::ReadBitmapData()  { SkipGroup(); }


SvParserState SvRTFParser::CallParser()
{
    char cFirstCh(0);
    nNextChPos = rInput.Tell();
    rInput.ReadChar( cFirstCh );
    nNextCh = static_cast<unsigned char>(cFirstCh);
    eState = SvParserState::Working;
    nOpenBrackets = 0;
    eCodeSet = RTL_TEXTENCODING_MS_1252;
    SetSrcEncoding( eCodeSet );

    // the first two tokens should be '{' and \\rtf !!
    if( '{' == GetNextToken() && RTF_RTF == GetNextToken() )
    {
        AddFirstRef();
        // call ReleaseRef at end of this scope, even in the face of exceptions
        comphelper::ScopeGuard g([this] {
            if( SvParserState::Pending != eState )
                ReleaseRef();       // now parser is not needed anymore
        });
        Continue( 0 );
    }
    else
        eState = SvParserState::Error;

    return eState;
}

void SvRTFParser::Continue( int nToken )
{
//  DBG_ASSERT( SVPAR_CS_DONTKNOW == GetCharSet(),
//              "Characterset was changed." );

    if( !nToken )
        nToken = GetNextToken();

    bool bLooping = false;

    while (IsParserWorking() && !bLooping)
    {
        auto nCurrentTokenIndex = m_nTokenIndex;
        auto nCurrentToken = nToken;

        SaveState( nToken );
        switch( nToken )
        {
        case '}':
            if( nOpenBrackets )
                goto NEXTTOKEN;
            eState = SvParserState::Accepted;
            break;

        case '{':
            // an unknown group ?
            {
                if( RTF_IGNOREFLAG != GetNextToken() )
                    nToken = SkipToken();
                else if( RTF_UNKNOWNCONTROL != GetNextToken() )
                    nToken = SkipToken( -2 );
                else
                {
                    // filter immediately
                    ReadUnknownData();
                    nToken = GetNextToken();
                    if( '}' != nToken )
                        eState = SvParserState::Error;
                    break;      // move to next token!!
                }
            }
            goto NEXTTOKEN;

        case RTF_UNKNOWNCONTROL:
            break;      // skip unknown token
        case RTF_NEXTTYPE:
        case RTF_ANSITYPE:
            eCodeSet = RTL_TEXTENCODING_MS_1252;
            SetSrcEncoding( eCodeSet );
            break;
        case RTF_MACTYPE:
            eCodeSet = RTL_TEXTENCODING_APPLE_ROMAN;
            SetSrcEncoding( eCodeSet );
            break;
        case RTF_PCTYPE:
            eCodeSet = RTL_TEXTENCODING_IBM_437;
            SetSrcEncoding( eCodeSet );
            break;
        case RTF_PCATYPE:
            eCodeSet = RTL_TEXTENCODING_IBM_850;
            SetSrcEncoding( eCodeSet );
            break;
        case RTF_ANSICPG:
            eCodeSet = rtl_getTextEncodingFromWindowsCodePage(nTokenValue);
            SetSrcEncoding(eCodeSet);
            break;
        default:
NEXTTOKEN:
            NextToken( nToken );
            break;
        }
        if( IsParserWorking() )
            SaveState( 0 );         // processed till here,
                                    // continue with new token!
        nToken = GetNextToken();
        bLooping = nCurrentTokenIndex == m_nTokenIndex && nToken == nCurrentToken;
    }
    if( SvParserState::Accepted == eState && 0 < nOpenBrackets )
        eState = SvParserState::Error;
}

void SvRTFParser::SetEncoding( rtl_TextEncoding eEnc )
{
    if (eEnc == RTL_TEXTENCODING_DONTKNOW)
        eEnc = GetCodeSet();

    if (!aParserStates.empty())
        aParserStates.top().eCodeSet = eEnc;
    SetSrcEncoding(eEnc);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
