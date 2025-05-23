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

#pragma once


// Implementation class for Basic command: Format$( d,formatStr )

/*
  Grammar of format string (a try):
  -----------------------------------------------

  format_string     := {\special_char} general_format | scientific_format {\special_char} {;format_string}
  general_format    := {#[,]}{0[,]}[.{0}{#}]
  scientific_format := {0}[.{0}{#}](e | E)(+ | -){#}{0}

  percent_char      := '%'
  special_char      := \char | + | - | ( | ) | $ | space_char
  char              := all_ascii_chars
  space_char        := ' '

  {}    repeated multiple times (incl. zero times)
  []    exactly one or zero times
  ()    parenthesis, e.g. (e | E) means e or E times

  Additional predefined formats for the format string:
    "General Number"
    "Currency"
    "Fixed"
    "Standard"
    "Percent"
    "Scientific"
    "Yes/No"
    "True/False"
    "On/Off"

 Note: invalid format string are ignored just as in VisualBasic, the output is
       probably 'undefined'. ASCII letters are outputted directly.

 Constraints in VisualBasic:
    - the exponent (scientific syntax) has a maximum of three digits!

 Constraints of new implementation:
    - the '+' sign is not allowed as wildcard in the mantissa

 TODO:
    - Date formatting
        Wildcards are: 'h', 'm', 's', 'y'
        predefined String-Constants/Commands:
            "AMPM", "Long Date", "Long Time"
*/

#include <rtl/ustring.hxx>
#include <rtl/ustrbuf.hxx>

class SbxBasicFormater {
  public:
    // Constructor takes signs for decimal point, thousand separation sign
    // and necessary resource strings.
    SbxBasicFormater( sal_Unicode _cDecPoint, sal_Unicode _cThousandSep,
                      OUString _sOnStrg,
                      OUString _sOffStrg,
                      OUString _sYesStrg,
                      OUString _sNoStrg,
                      OUString _sTrueStrg,
                      OUString _sFalseStrg,
                      OUString _sCurrencyStrg,
                      OUString _sCurrencyFormatStrg );

    /* Basic command: Format$( number,format-string )

       Parameter:
        dNumber     : number to be formatted
        sFormatStrg : the Format-String, e.g. ###0.0###

       Return value:
        String containing the formatted output
    */
    OUString  BasicFormat( double dNumber, const OUString& sFormatStrg );
    static OUString BasicFormatNull( std::u16string_view sFormatStrg );

    static  bool isBasicFormat( std::u16string_view sFormatStrg );

  private:
    static inline void ShiftString( OUStringBuffer& sStrg, sal_uInt16 nStartPos );
    static void AppendDigit( OUStringBuffer& sStrg, short nDigit );
    void   LeftShiftDecimalPoint( OUStringBuffer& sStrg );
    void   StrRoundDigit( OUStringBuffer& sStrg, short nPos, bool& bOverflow );
    void   StrRoundDigit( OUStringBuffer& sStrg, short nPos );
    static void ParseBack( OUStringBuffer& sStrg, std::u16string_view sFormatStrg,
                                       short nFormatPos );
    // Methods for string conversion with sprintf():
    void   InitScan( double _dNum );
    void   InitExp( double _dNewExp );
    short  GetDigitAtPosScan( short nPos, bool& bFoundFirstDigit );
    short  GetDigitAtPosExpScan( double dNewExponent, short nPos,
                                                  bool& bFoundFirstDigit );
    short  GetDigitAtPosExpScan( short nPos, bool& bFoundFirstDigit );
    static OUString GetPosFormatString( std::u16string_view sFormatStrg, bool & bFound );
    static OUString GetNegFormatString( std::u16string_view sFormatStrg, bool & bFound );
    static OUString Get0FormatString( std::u16string_view sFormatStrg, bool & bFound );
    static OUString GetNullFormatString( std::u16string_view sFormatStrg, bool & bFound );
    static void AnalyseFormatString( std::u16string_view sFormatStrg,
                                                 short& nNoOfDigitsLeft, short& nNoOfDigitsRight,
                                                 short& nNoOfOptionalDigitsLeft,
                                                 short& nNoOfExponentDigits,
                                                 short& nNoOfOptionalExponentDigits,
                                                 bool& bPercent, bool& bCurrency, bool& bScientific,
                                                 bool& bGenerateThousandSeparator,
                                                 short& nMultipleThousandSeparators );
    void   ScanFormatString( double dNumber, std::u16string_view sFormatStrg,
                                              OUString& sReturnStrg, bool bCreateSign );

    //*** Data ***
    sal_Unicode cDecPoint;      // sign for the decimal point
    sal_Unicode cThousandSep;   // sign for thousand delimiter
    // Text for output:
    OUString  sOnStrg;
    OUString  sOffStrg;
    OUString  sYesStrg;
    OUString  sNoStrg;
    OUString  sTrueStrg;
    OUString  sFalseStrg;
    OUString  sCurrencyStrg;
    OUString  sCurrencyFormatStrg;

    //*** temporary data for scan loop ***

    // String containing the number in scientific format
    OUString  sSciNumStrg;
    // String containing the exponent of the number
    OUString  sNumExpStrg;
    double  dNum;           // the number that is scanned
    short   nNumExp;        // the exponent of the number
    short   nExpExp;        // the number of digits in the exponent
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
