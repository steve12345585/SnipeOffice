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

#include <sal/config.h>

#include <unordered_map>

#include <comphelper/propagg.hxx>

#include "frm_strings.hxx"

using namespace comphelper;

//= property helper classes

namespace frm
{

// PropertyId's, who have a mapping to a PropertyName
#define PROPERTY_ID_START           0

#define PROPERTY_ID_NAME                (PROPERTY_ID_START + 1)
#define PROPERTY_ID_TABINDEX            (PROPERTY_ID_START + 2)
#define PROPERTY_ID_CONTROLSOURCE       (PROPERTY_ID_START + 3)
#define PROPERTY_ID_MASTERFIELDS        (PROPERTY_ID_START + 4)
#define PROPERTY_ID_DATASOURCE          (PROPERTY_ID_START + 6)
#define PROPERTY_ID_CLASSID             (PROPERTY_ID_START + 9)
#define PROPERTY_ID_CURSORTYPE          (PROPERTY_ID_START + 10)
#define PROPERTY_ID_READONLY            (PROPERTY_ID_START + 11)
#define PROPERTY_ID_NAVIGATION          (PROPERTY_ID_START + 13)
#define PROPERTY_ID_CYCLE               (PROPERTY_ID_START + 14)
#define PROPERTY_ID_ALLOWADDITIONS      (PROPERTY_ID_START + 15)
#define PROPERTY_ID_ALLOWEDITS          (PROPERTY_ID_START + 16)
#define PROPERTY_ID_ALLOWDELETIONS      (PROPERTY_ID_START + 17)
#define PROPERTY_ID_NATIVE_LOOK         (PROPERTY_ID_START + 18)
#define PROPERTY_ID_INPUT_REQUIRED      (PROPERTY_ID_START + 19)
#define PROPERTY_ID_WRITING_MODE        (PROPERTY_ID_START + 20)
#define PROPERTY_ID_CONTEXT_WRITING_MODE    (PROPERTY_ID_START + 21)
#define PROPERTY_ID_VERTICAL_ALIGN      (PROPERTY_ID_START + 22)
#define PROPERTY_ID_GRAPHIC             (PROPERTY_ID_START + 23)
#define PROPERTY_ID_GROUP_NAME          (PROPERTY_ID_START + 24)
#define PROPERTY_ID_STANDARD_THEME      (PROPERTY_ID_START + 25)
    // free
    // free
    // free
    // free
    // free
#define PROPERTY_ID_VALUE               (PROPERTY_ID_START + 31)    // INT32
    // free
#define PROPERTY_ID_FORMATKEY           (PROPERTY_ID_START + 33)    // UINT32
    // free
    // free
    // free
#define PROPERTY_ID_SIZE                (PROPERTY_ID_START + 37)    // UINT32
#define PROPERTY_ID_REFERENCE_DEVICE    (PROPERTY_ID_START + 38)    // XDevice
    // free
    // free
    // free
#define PROPERTY_ID_WIDTH               (PROPERTY_ID_START + 42)    // UINT16
#define PROPERTY_ID_DEFAULTCONTROL      (PROPERTY_ID_START + 43)    // string
#define PROPERTY_ID_BOUNDCOLUMN         (PROPERTY_ID_START + 44)    // UINT16 may be null
#define PROPERTY_ID_LISTSOURCETYPE      (PROPERTY_ID_START + 45)    // UINT16
#define PROPERTY_ID_LISTSOURCE          (PROPERTY_ID_START + 46)    // string
    // FREE
#define PROPERTY_ID_TEXT                (PROPERTY_ID_START + 48)    // string
#define PROPERTY_ID_STRINGITEMLIST      (PROPERTY_ID_START + 49)    // wsstringsequence
#define PROPERTY_ID_LABEL               (PROPERTY_ID_START + 50)    // string
#define PROPERTY_ID_HIDEINACTIVESELECTION (PROPERTY_ID_START + 51)  // sal_Bool
#define PROPERTY_ID_STATE               (PROPERTY_ID_START + 52)    // UINT16
#define PROPERTY_ID_DELAY               (PROPERTY_ID_START + 53)    // sal_Int32
#define PROPERTY_ID_FONT                (PROPERTY_ID_START + 54)    // font
#define PROPERTY_ID_HASNAVIGATION       (PROPERTY_ID_START + 55)
#define PROPERTY_ID_BORDERCOLOR         (PROPERTY_ID_START + 56)    // sal_Int32
#define PROPERTY_ID_ROWHEIGHT           (PROPERTY_ID_START + 57)    // UINT16
#define PROPERTY_ID_BACKGROUNDCOLOR     (PROPERTY_ID_START + 58)    // sal_Int32
#define PROPERTY_ID_FILLCOLOR           (PROPERTY_ID_START + 59)    // UINT32
#define PROPERTY_ID_TEXTCOLOR           (PROPERTY_ID_START + 60)    // UINT32
#define PROPERTY_ID_LINECOLOR           (PROPERTY_ID_START + 61)    // UINT32
#define PROPERTY_ID_BORDER              (PROPERTY_ID_START + 62)    // UINT16
#define PROPERTY_ID_ALIGN               (PROPERTY_ID_START + 63)    // UINT16
#define PROPERTY_ID_DROPDOWN            (PROPERTY_ID_START + 64)    // BOOL
#define PROPERTY_ID_UNCHECKED_REFVALUE  (PROPERTY_ID_START + 65)    // OUString
#define PROPERTY_ID_HSCROLL             (PROPERTY_ID_START + 66)    // BOOL
#define PROPERTY_ID_VSCROLL             (PROPERTY_ID_START + 67)    // BOOL
#define PROPERTY_ID_TABSTOP             (PROPERTY_ID_START + 68)    // BOOL
#define PROPERTY_ID_REFVALUE            (PROPERTY_ID_START + 69)    // OUString
#define PROPERTY_ID_BUTTONTYPE          (PROPERTY_ID_START + 70)    // UINT16
#define PROPERTY_ID_DEFAULT_TEXT        (PROPERTY_ID_START + 71)    // OUString
#define PROPERTY_ID_SUBMIT_ACTION       (PROPERTY_ID_START + 72)    // string
#define PROPERTY_ID_SUBMIT_METHOD       (PROPERTY_ID_START + 73)    // FmSubmitMethod
#define PROPERTY_ID_SUBMIT_ENCODING     (PROPERTY_ID_START + 74)    // FmSubmitEncoding
#define PROPERTY_ID_DEFAULT_VALUE       (PROPERTY_ID_START + 75)    // OUString
#define PROPERTY_ID_SUBMIT_TARGET       (PROPERTY_ID_START + 76)    // OUString
#define PROPERTY_ID_DEFAULT_STATE       (PROPERTY_ID_START + 77)    // UINT16
#define PROPERTY_ID_VALUE_SEQ           (PROPERTY_ID_START + 78)    // StringSeq
#define PROPERTY_ID_IMAGE_URL           (PROPERTY_ID_START + 79)    // OUString
#define PROPERTY_ID_SELECT_VALUE        (PROPERTY_ID_START + 80)    // StringSeq
#define PROPERTY_ID_SELECT_VALUE_SEQ    (PROPERTY_ID_START + 81)    // StringSeq
    // free
    // free
    // free
    // free
    // free
    // free
    // free
    // free
    // free
#define PROPERTY_ID_SELECT_SEQ          (PROPERTY_ID_START + 91)    // INT16Seq
#define PROPERTY_ID_DEFAULT_SELECT_SEQ  (PROPERTY_ID_START + 92)    // INT16Seq
#define PROPERTY_ID_MULTISELECTION      (PROPERTY_ID_START + 93)    // BOOL
#define PROPERTY_ID_MULTILINE           (PROPERTY_ID_START + 94)    // BOOL
#define PROPERTY_ID_DATE                (PROPERTY_ID_START + 95)    // UINT32
#define PROPERTY_ID_DATEMIN             (PROPERTY_ID_START + 96)    // UINT32
#define PROPERTY_ID_DATEMAX             (PROPERTY_ID_START + 97)    // UINT32
#define PROPERTY_ID_DATEFORMAT          (PROPERTY_ID_START + 98)    // UINT16
#define PROPERTY_ID_TIME                (PROPERTY_ID_START + 99)    // UINT32
#define PROPERTY_ID_TIMEMIN             (PROPERTY_ID_START +100)    // UINT32
#define PROPERTY_ID_TIMEMAX             (PROPERTY_ID_START +101)    // UINT32
#define PROPERTY_ID_TIMEFORMAT          (PROPERTY_ID_START +102)    // UINT16
#define PROPERTY_ID_VALUEMIN            (PROPERTY_ID_START +103)    // INT32
#define PROPERTY_ID_VALUEMAX            (PROPERTY_ID_START +104)    // INT32
#define PROPERTY_ID_VALUESTEP           (PROPERTY_ID_START +105)    // INT32
#define PROPERTY_ID_CURRENCYSYMBOL      (PROPERTY_ID_START +106)    // OUString
#define PROPERTY_ID_EDITMASK            (PROPERTY_ID_START +107)    // OUString
#define PROPERTY_ID_LITERALMASK         (PROPERTY_ID_START +108)    // OUString
#define PROPERTY_ID_ENABLED             (PROPERTY_ID_START +109)    // BOOL
#define PROPERTY_ID_AUTOCOMPLETE        (PROPERTY_ID_START +110)    // BOOL
#define PROPERTY_ID_LINECOUNT           (PROPERTY_ID_START +111)    // UINT16
#define PROPERTY_ID_MAXTEXTLEN          (PROPERTY_ID_START +112)    // UINT16
#define PROPERTY_ID_SPIN                (PROPERTY_ID_START +113)    // BOOL
#define PROPERTY_ID_STRICTFORMAT        (PROPERTY_ID_START +114)    // BOOL
#define PROPERTY_ID_SHOWTHOUSANDSEP     (PROPERTY_ID_START +115)    // BOOL
#define PROPERTY_ID_HARDLINEBREAKS      (PROPERTY_ID_START +116)    // BOOL
#define PROPERTY_ID_PRINTABLE           (PROPERTY_ID_START +117)    // BOOL
#define PROPERTY_ID_TARGET_URL          (PROPERTY_ID_START +118)    // OUString
#define PROPERTY_ID_TARGET_FRAME        (PROPERTY_ID_START +119)    // OUString
#define PROPERTY_ID_TAG                 (PROPERTY_ID_START +120)    // OUString
#define PROPERTY_ID_ECHO_CHAR           (PROPERTY_ID_START +121)    // UINT16
#define PROPERTY_ID_SHOW_POSITION       (PROPERTY_ID_START +122)    // sal_Bool
#define PROPERTY_ID_SHOW_NAVIGATION     (PROPERTY_ID_START +123)    // sal_Bool
#define PROPERTY_ID_SHOW_RECORDACTIONS  (PROPERTY_ID_START +124)    // sal_Bool
#define PROPERTY_ID_SHOW_FILTERSORT     (PROPERTY_ID_START +125)    // sal_Bool
#define PROPERTY_ID_EMPTY_IS_NULL       (PROPERTY_ID_START +126)    // Bool
#define PROPERTY_ID_DECIMAL_ACCURACY    (PROPERTY_ID_START +127)    // UINT16
#define PROPERTY_ID_DATE_SHOW_CENTURY   (PROPERTY_ID_START +128)    // Bool
#define PROPERTY_ID_TRISTATE            (PROPERTY_ID_START +129)    // Bool
#define PROPERTY_ID_DEFAULT_BUTTON      (PROPERTY_ID_START +130)    // Bool
#define PROPERTY_ID_HIDDEN_VALUE        (PROPERTY_ID_START +131)    // OUString
#define PROPERTY_ID_DECIMALS            (PROPERTY_ID_START +132)    // UINT16
#define PROPERTY_ID_AUTOINCREMENT       (PROPERTY_ID_START +133)    // UINT16
    // free
#define PROPERTY_ID_FILTER              (PROPERTY_ID_START +135)    // OUString
#define PROPERTY_ID_HAVINGCLAUSE        (PROPERTY_ID_START +136)    // OUString
#define PROPERTY_ID_QUERY               (PROPERTY_ID_START +137)    // OUString
#define PROPERTY_ID_DEFAULT_LONG_VALUE  (PROPERTY_ID_START +138)    // Double
#define PROPERTY_ID_DEFAULT_DATE        (PROPERTY_ID_START +139)    // UINT32
#define PROPERTY_ID_DEFAULT_TIME        (PROPERTY_ID_START +140)
#define PROPERTY_ID_HELPTEXT            (PROPERTY_ID_START +141)
#define PROPERTY_ID_FONT_NAME           (PROPERTY_ID_START +142)
#define PROPERTY_ID_FONT_STYLENAME      (PROPERTY_ID_START +143)
#define PROPERTY_ID_FONT_FAMILY         (PROPERTY_ID_START +144)
#define PROPERTY_ID_FONT_CHARSET        (PROPERTY_ID_START +145)
#define PROPERTY_ID_FONT_HEIGHT         (PROPERTY_ID_START +146)
#define PROPERTY_ID_FONT_WEIGHT         (PROPERTY_ID_START +147)
#define PROPERTY_ID_FONT_SLANT          (PROPERTY_ID_START +148)
#define PROPERTY_ID_FONT_UNDERLINE      (PROPERTY_ID_START +149)
#define PROPERTY_ID_FONT_STRIKEOUT      (PROPERTY_ID_START +150)
#define PROPERTY_ID_ISPASSTHROUGH       (PROPERTY_ID_START +151)
#define PROPERTY_ID_HELPURL             (PROPERTY_ID_START +152)    // OUString
#define PROPERTY_ID_RECORDMARKER        (PROPERTY_ID_START +153)
#define PROPERTY_ID_BOUNDFIELD          (PROPERTY_ID_START +154)
#define PROPERTY_ID_FORMATSSUPPLIER     (PROPERTY_ID_START +155)    // XNumberFormatsSupplier
#define PROPERTY_ID_TREATASNUMERIC      (PROPERTY_ID_START +156)    // BOOL
#define PROPERTY_ID_EFFECTIVE_VALUE     (PROPERTY_ID_START +157)    // ANY (string or double)
#define PROPERTY_ID_EFFECTIVE_DEFAULT   (PROPERTY_ID_START +158)    // ditto
#define PROPERTY_ID_EFFECTIVE_MIN       (PROPERTY_ID_START +159)    // ditto
#define PROPERTY_ID_EFFECTIVE_MAX       (PROPERTY_ID_START +160)    // ditto
#define PROPERTY_ID_HIDDEN              (PROPERTY_ID_START +161)    // BOOL
#define PROPERTY_ID_FILTERPROPOSAL      (PROPERTY_ID_START +162)    // BOOL
#define PROPERTY_ID_FIELDSOURCE         (PROPERTY_ID_START +163)    // String
#define PROPERTY_ID_TABLENAME           (PROPERTY_ID_START +164)    // String
#define PROPERTY_ID_ENABLEVISIBLE       (PROPERTY_ID_START +165)    // BOOL
    // FREE
    // FREE
    // FREE
    // FREE
#define PROPERTY_ID_CONTROLLABEL        (PROPERTY_ID_START +171)    // XPropertySet
#define PROPERTY_ID_CURRSYM_POSITION    (PROPERTY_ID_START +172)    // String
    // FREE
#define PROPERTY_ID_CURSORCOLOR         (PROPERTY_ID_START +174)    // INT32
#define PROPERTY_ID_ALWAYSSHOWCURSOR    (PROPERTY_ID_START +175)    // BOOL
#define PROPERTY_ID_DISPLAYSYNCHRON     (PROPERTY_ID_START +176)    // BOOL
#define PROPERTY_ID_ISMODIFIED          (PROPERTY_ID_START +177)    // BOOL
#define PROPERTY_ID_ISNEW               (PROPERTY_ID_START +178)    // BOOL
#define PROPERTY_ID_PRIVILEGES          (PROPERTY_ID_START +179)    // INT32
#define PROPERTY_ID_DETAILFIELDS        (PROPERTY_ID_START +180)    // Sequence< OUString >
#define PROPERTY_ID_COMMAND             (PROPERTY_ID_START +181)    // String
#define PROPERTY_ID_COMMANDTYPE         (PROPERTY_ID_START +182)    // INT32 (css::sdb::CommandType)
#define PROPERTY_ID_RESULTSET_CONCURRENCY   (PROPERTY_ID_START +183)// INT32 (css::sdbc::ResultSetConcurrency)
#define PROPERTY_ID_INSERTONLY          (PROPERTY_ID_START +184)    // BOOL
#define PROPERTY_ID_RESULTSET_TYPE      (PROPERTY_ID_START +185)    // INT32 (css::sdbc::ResultSetType)
#define PROPERTY_ID_ESCAPE_PROCESSING   (PROPERTY_ID_START +186)    // BOOL
#define PROPERTY_ID_APPLYFILTER         (PROPERTY_ID_START +187)    // BOOL

#define PROPERTY_ID_ISNULLABLE          (PROPERTY_ID_START +188)    // BOOL
#define PROPERTY_ID_ACTIVECOMMAND       (PROPERTY_ID_START +189)    // String
#define PROPERTY_ID_ISCURRENCY          (PROPERTY_ID_START +190)    // BOOL
#define PROPERTY_ID_URL                 (PROPERTY_ID_START +192)    // String
#define PROPERTY_ID_TITLE               (PROPERTY_ID_START +193)    // String
#define PROPERTY_ID_ACTIVE_CONNECTION   (PROPERTY_ID_START +194)    // css::sdbc::XConnection
#define PROPERTY_ID_SCALE               (PROPERTY_ID_START +195)    // INT32
#define PROPERTY_ID_SORT                (PROPERTY_ID_START +196)    // String

    // free
    // free
#define PROPERTY_ID_FETCHSIZE           (PROPERTY_ID_START +199)
    // free
#define PROPERTY_ID_SEARCHABLE          (PROPERTY_ID_START +201)
#define PROPERTY_ID_ISREADONLY          (PROPERTY_ID_START +202)
    // free
#define PROPERTY_ID_FIELDTYPE           (PROPERTY_ID_START +204)
#define PROPERTY_ID_COLUMNSERVICENAME   (PROPERTY_ID_START +205)
#define PROPERTY_ID_CONTROLSOURCEPROPERTY   (PROPERTY_ID_START +206)
#define PROPERTY_ID_REALNAME            (PROPERTY_ID_START +207)
#define PROPERTY_ID_FONT_WORDLINEMODE   (PROPERTY_ID_START +208)
#define PROPERTY_ID_TEXTLINECOLOR       (PROPERTY_ID_START +209)
#define PROPERTY_ID_FONTEMPHASISMARK    (PROPERTY_ID_START +210)
#define PROPERTY_ID_FONTRELIEF          (PROPERTY_ID_START +211)

#define PROPERTY_ID_DISPATCHURLINTERNAL         ( PROPERTY_ID_START + 212 ) // sal_Bool
#define PROPERTY_ID_PERSISTENCE_MAXTEXTLENGTH   ( PROPERTY_ID_START + 213 ) // sal_Int16
#define PROPERTY_ID_DEFAULT_SCROLL_VALUE        ( PROPERTY_ID_START + 214 ) // sal_Int32
#define PROPERTY_ID_DEFAULT_SPIN_VALUE          ( PROPERTY_ID_START + 215 ) // sal_Int32
#define PROPERTY_ID_SCROLL_VALUE                ( PROPERTY_ID_START + 216 ) // sal_Int32
#define PROPERTY_ID_SPIN_VALUE                  ( PROPERTY_ID_START + 217 ) // sal_Int32
#define PROPERTY_ID_ICONSIZE                    ( PROPERTY_ID_START + 218 ) // sal_Int16

#define PROPERTY_ID_FONT_CHARWIDTH              ( PROPERTY_ID_START + 219 ) // float
#define PROPERTY_ID_FONT_KERNING                ( PROPERTY_ID_START + 220 ) // sal_Bool
#define PROPERTY_ID_FONT_ORIENTATION            ( PROPERTY_ID_START + 221 ) // float
#define PROPERTY_ID_FONT_PITCH                  ( PROPERTY_ID_START + 222 ) // sal_Int16
#define PROPERTY_ID_FONT_TYPE                   ( PROPERTY_ID_START + 223 ) // sal_Int16
#define PROPERTY_ID_FONT_WIDTH                  ( PROPERTY_ID_START + 224 ) // sal_Int16
#define PROPERTY_ID_RICH_TEXT                   ( PROPERTY_ID_START + 225 ) // sal_Bool

#define PROPERTY_ID_DYNAMIC_CONTROL_BORDER      ( PROPERTY_ID_START + 226 ) // sal_Bool
#define PROPERTY_ID_CONTROL_BORDER_COLOR_FOCUS  ( PROPERTY_ID_START + 227 ) // sal_Int32
#define PROPERTY_ID_CONTROL_BORDER_COLOR_MOUSE  ( PROPERTY_ID_START + 228 ) // sal_Int32
#define PROPERTY_ID_CONTROL_BORDER_COLOR_INVALID ( PROPERTY_ID_START + 229 ) // sal_Int32

#define PROPERTY_ID_XSD_PATTERN                 ( PROPERTY_ID_START + 230 )
#define PROPERTY_ID_XSD_WHITESPACE              ( PROPERTY_ID_START + 231 )
#define PROPERTY_ID_XSD_LENGTH                  ( PROPERTY_ID_START + 232 )
#define PROPERTY_ID_XSD_MIN_LENGTH              ( PROPERTY_ID_START + 233 )
#define PROPERTY_ID_XSD_MAX_LENGTH              ( PROPERTY_ID_START + 234 )
#define PROPERTY_ID_XSD_TOTAL_DIGITS            ( PROPERTY_ID_START + 235 )
#define PROPERTY_ID_XSD_FRACTION_DIGITS         ( PROPERTY_ID_START + 236 )
#define PROPERTY_ID_XSD_MAX_INCLUSIVE_INT       ( PROPERTY_ID_START + 237 )
#define PROPERTY_ID_XSD_MAX_EXCLUSIVE_INT       ( PROPERTY_ID_START + 238 )
#define PROPERTY_ID_XSD_MIN_INCLUSIVE_INT       ( PROPERTY_ID_START + 239 )
#define PROPERTY_ID_XSD_MIN_EXCLUSIVE_INT       ( PROPERTY_ID_START + 240 )
#define PROPERTY_ID_XSD_MAX_INCLUSIVE_DOUBLE    ( PROPERTY_ID_START + 241 )
#define PROPERTY_ID_XSD_MAX_EXCLUSIVE_DOUBLE    ( PROPERTY_ID_START + 242 )
#define PROPERTY_ID_XSD_MIN_INCLUSIVE_DOUBLE    ( PROPERTY_ID_START + 243 )
#define PROPERTY_ID_XSD_MIN_EXCLUSIVE_DOUBLE    ( PROPERTY_ID_START + 244 )
#define PROPERTY_ID_XSD_MAX_INCLUSIVE_DATE      ( PROPERTY_ID_START + 245 )
#define PROPERTY_ID_XSD_MAX_EXCLUSIVE_DATE      ( PROPERTY_ID_START + 246 )
#define PROPERTY_ID_XSD_MIN_INCLUSIVE_DATE      ( PROPERTY_ID_START + 247 )
#define PROPERTY_ID_XSD_MIN_EXCLUSIVE_DATE      ( PROPERTY_ID_START + 248 )
#define PROPERTY_ID_XSD_MAX_INCLUSIVE_TIME      ( PROPERTY_ID_START + 249 )
#define PROPERTY_ID_XSD_MAX_EXCLUSIVE_TIME      ( PROPERTY_ID_START + 250 )
#define PROPERTY_ID_XSD_MIN_INCLUSIVE_TIME      ( PROPERTY_ID_START + 251 )
#define PROPERTY_ID_XSD_MIN_EXCLUSIVE_TIME      ( PROPERTY_ID_START + 252 )
#define PROPERTY_ID_XSD_MAX_INCLUSIVE_DATE_TIME ( PROPERTY_ID_START + 253 )
#define PROPERTY_ID_XSD_MAX_EXCLUSIVE_DATE_TIME ( PROPERTY_ID_START + 254 )
#define PROPERTY_ID_XSD_MIN_INCLUSIVE_DATE_TIME ( PROPERTY_ID_START + 255 )
#define PROPERTY_ID_XSD_MIN_EXCLUSIVE_DATE_TIME ( PROPERTY_ID_START + 256 )
#define PROPERTY_ID_XSD_IS_BASIC                ( PROPERTY_ID_START + 257 )
#define PROPERTY_ID_XSD_TYPE_CLASS              ( PROPERTY_ID_START + 258 )

#define PROPERTY_ID_LINEEND_FORMAT              ( PROPERTY_ID_START + 259 ) // css.awt.LineEndFormat
#define PROPERTY_ID_GENERATEVBAEVENTS           ( PROPERTY_ID_START + 260 )
#define PROPERTY_ID_CONTROL_TYPE_IN_MSO         ( PROPERTY_ID_START + 261 )
#define PROPERTY_ID_OBJ_ID_IN_MSO           ( PROPERTY_ID_START + 262 )

#define PROPERTY_ID_TYPEDITEMLIST               ( PROPERTY_ID_START + 263 ) // Sequence<Any>

// start ID for aggregated properties
#define PROPERTY_ID_AGGREGATE_ID        (PROPERTY_ID_START + 10000)

//= assignment property handle <-> property name
//= used by the PropertySetAggregationHelper


class PropertyInfoService
{
    typedef std::unordered_map<OUString, sal_Int32> PropertyMap;
    static PropertyMap      s_AllKnownProperties;

public:
    PropertyInfoService() = delete;

    static sal_Int32            getPropertyId(const OUString& _rName);

private:
    static void initialize();
};


// a class implementing the comphelper::IPropertyInfoService
class ConcreteInfoService final : public ::comphelper::IPropertyInfoService
{
public:
    virtual ~ConcreteInfoService() {}

    virtual sal_Int32 getPreferredPropertyId(const OUString& _rName) override;
};

}
//... namespace frm .......................................................

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
