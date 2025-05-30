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

#include "XMLLineNumberingExport.hxx"
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/text/XLineNumberingProperties.hpp>
#include <com/sun/star/style/LineNumberPosition.hpp>
#include <o3tl/any.hxx>
#include <xmloff/xmlexp.hxx>
#include <xmloff/xmluconv.hxx>
#include <xmloff/xmlnamespace.hxx>
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmlement.hxx>


using namespace ::com::sun::star::uno;
using namespace ::com::sun::star;
using namespace ::xmloff::token;

using ::com::sun::star::beans::XPropertySet;
using ::com::sun::star::text::XLineNumberingProperties;


XMLLineNumberingExport::XMLLineNumberingExport(SvXMLExport& rExp)
: rExport(rExp)
{
}

SvXMLEnumMapEntry<sal_uInt16> const aLineNumberPositionMap[] =
{
    { XML_LEFT,     style::LineNumberPosition::LEFT },
    { XML_RIGHT,    style::LineNumberPosition::RIGHT },
    { XML_INSIDE,   style::LineNumberPosition::INSIDE },
    { XML_OUTSIDE,  style::LineNumberPosition::OUTSIDE },
    { XML_TOKEN_INVALID, 0 }
};


void XMLLineNumberingExport::Export()
{
    // export element if we have line numbering info
    Reference<XLineNumberingProperties> xSupplier(rExport.GetModel(),
                                                  UNO_QUERY);
    if (!xSupplier.is())
        return;

    Reference<XPropertySet> xLineNumbering =
        xSupplier->getLineNumberingProperties();

    if (!xLineNumbering.is())
        return;

    // char style
    Any aAny = xLineNumbering->getPropertyValue(u"CharStyleName"_ustr);
    OUString sTmp;
    aAny >>= sTmp;
    if (!sTmp.isEmpty())
    {
        rExport.AddAttribute(XML_NAMESPACE_TEXT, XML_STYLE_NAME,
                             rExport.EncodeStyleName( sTmp ));
    }

    // enable
    aAny = xLineNumbering->getPropertyValue(u"IsOn"_ustr);
    if (! *o3tl::doAccess<bool>(aAny))
    {
        rExport.AddAttribute(XML_NAMESPACE_TEXT,
                             XML_NUMBER_LINES, XML_FALSE);
    }

    // count empty lines
    aAny = xLineNumbering->getPropertyValue(u"CountEmptyLines"_ustr);
    if (! *o3tl::doAccess<bool>(aAny))
    {
        rExport.AddAttribute(XML_NAMESPACE_TEXT,
                             XML_COUNT_EMPTY_LINES, XML_FALSE);
    }

    // count in frames
    aAny = xLineNumbering->getPropertyValue(u"CountLinesInFrames"_ustr);
    if (*o3tl::doAccess<bool>(aAny))
    {
        rExport.AddAttribute(XML_NAMESPACE_TEXT,
                             XML_COUNT_IN_TEXT_BOXES, XML_TRUE);
    }

    // restart numbering
    aAny = xLineNumbering->getPropertyValue(u"RestartAtEachPage"_ustr);
    if (*o3tl::doAccess<bool>(aAny))
    {
        rExport.AddAttribute(XML_NAMESPACE_TEXT,
                             XML_RESTART_ON_PAGE, XML_TRUE);
    }

    // Distance
    aAny = xLineNumbering->getPropertyValue(u"Distance"_ustr);
    sal_Int32 nLength = 0;
    aAny >>= nLength;
    if (nLength != 0)
    {
        OUStringBuffer sBuf;
        rExport.GetMM100UnitConverter().convertMeasureToXML(
                sBuf, nLength);
        rExport.AddAttribute(XML_NAMESPACE_TEXT, XML_OFFSET,
                             sBuf.makeStringAndClear());
    }

    // NumberingType
    OUStringBuffer sNumPosBuf;
    aAny = xLineNumbering->getPropertyValue(u"NumberingType"_ustr);
    sal_Int16 nFormat = 0;
    aAny >>= nFormat;
    rExport.GetMM100UnitConverter().convertNumFormat( sNumPosBuf, nFormat );
    rExport.AddAttribute(XML_NAMESPACE_STYLE, XML_NUM_FORMAT,
                         sNumPosBuf.makeStringAndClear());
    SvXMLUnitConverter::convertNumLetterSync( sNumPosBuf, nFormat );
    if( !sNumPosBuf.isEmpty() )
    {
        rExport.AddAttribute(XML_NAMESPACE_STYLE,
                             XML_NUM_LETTER_SYNC,
                             sNumPosBuf.makeStringAndClear() );
    }

    // number position
    aAny = xLineNumbering->getPropertyValue(u"NumberPosition"_ustr);
    sal_uInt16 nPosition = 0;
    aAny >>= nPosition;
    if (SvXMLUnitConverter::convertEnum(sNumPosBuf, nPosition,
                                        aLineNumberPositionMap))
    {
        rExport.AddAttribute(XML_NAMESPACE_TEXT, XML_NUMBER_POSITION,
                             sNumPosBuf.makeStringAndClear());
    }

    // sInterval
    aAny = xLineNumbering->getPropertyValue(u"Interval"_ustr);
    sal_Int16 nLineInterval = 0;
    aAny >>= nLineInterval;
    rExport.AddAttribute(XML_NAMESPACE_TEXT, XML_INCREMENT,
                         OUString::number(nLineInterval));

    SvXMLElementExport aConfigElem(rExport, XML_NAMESPACE_TEXT,
                                   XML_LINENUMBERING_CONFIGURATION,
                                   true, true);

    // line separator
    aAny = xLineNumbering->getPropertyValue(u"SeparatorText"_ustr);
    OUString sSeparator;
    aAny >>= sSeparator;
    if (sSeparator.isEmpty())
        return;

    // SeparatorInterval
    aAny = xLineNumbering->getPropertyValue(u"SeparatorInterval"_ustr);
    sal_Int16 nLineDistance = 0;
    aAny >>= nLineDistance;
    rExport.AddAttribute(XML_NAMESPACE_TEXT, XML_INCREMENT,
                         OUString::number(nLineDistance));

    SvXMLElementExport aSeparatorElem(rExport, XML_NAMESPACE_TEXT,
                                      XML_LINENUMBERING_SEPARATOR,
                                      true, false);
    rExport.Characters(sSeparator);
    // else: no configuration: don't save -> default
    // can't even get supplier: don't save -> default
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
