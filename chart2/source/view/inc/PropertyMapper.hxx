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

#include <com/sun/star/uno/Sequence.h>
#include <com/sun/star/uno/Reference.h>

namespace com::sun::star::beans { class XPropertySet; }
class SvxShape;

namespace chart
{

typedef std::unordered_map<OUString, OUString> tPropertyNameMap;
typedef std::unordered_map<OUString, css::uno::Any> tPropertyNameValueMap;
typedef css::uno::Sequence< OUString >                   tNameSequence;
typedef css::uno::Sequence< css::uno::Any > tAnySequence;

/**
 * PropertyMapper provides easy mapping of the property names of various
 * objects in the chart model, to the property names of the destination
 * shape objects (those whose service names begin with
 * com.sun.star.drawing.).
 */
class PropertyMapper
{
public:
    static void setMappedProperties(
          const css::uno::Reference< css::beans::XPropertySet >& xTarget
        , const css::uno::Reference< css::beans::XPropertySet >& xSource
        , const tPropertyNameMap& rMap );

    static void setMappedProperties(
          SvxShape& xTarget
        , const css::uno::Reference< css::beans::XPropertySet >& xSource
        , const tPropertyNameMap& rMap );

    /**
     * Fetch property values from the source object and map it to the
     * destination container.  Only those properties that are explicitly set
     * will be inserted into the destination container.
     *
     * @param rValueMap destination container
     * @param rNameMap property name mapping rule
     * @param xSourceProp source object from which the property values are
     *                    pulled.
     */
    static void getValueMap(
          tPropertyNameValueMap& rValueMap
        , const tPropertyNameMap& rNameMap
        , const css::uno::Reference< css::beans::XPropertySet >& xSourceProp
        );

    static void getMultiPropertyListsFromValueMap(
                  tNameSequence& rNames
                , tAnySequence&  rValues
                , const tPropertyNameValueMap& rValueMap
                );

    static css::uno::Any*
                getValuePointer( tAnySequence& rPropValues
                         , const tNameSequence& rPropNames
                         , std::u16string_view rPropName );

    static css::uno::Any*
                getValuePointerForLimitedSpace( tAnySequence& rPropValues
                         , const tNameSequence& rPropNames
                         , bool bLimitedHeight );

    static void setMultiProperties(
                  const tNameSequence& rNames
                , const tAnySequence&  rValues
                , SvxShape& xTarget );

    static const tPropertyNameMap& getPropertyNameMapForCharacterProperties();
    static const tPropertyNameMap& getPropertyNameMapForParagraphProperties();
    static const tPropertyNameMap& getPropertyNameMapForFillProperties();
    static const tPropertyNameMap& getPropertyNameMapForLineProperties();
    static const tPropertyNameMap& getPropertyNameMapForFillAndLineProperties();
    static const tPropertyNameMap& getPropertyNameMapForTextShapeProperties();

    static const tPropertyNameMap& getPropertyNameMapForFilledSeriesProperties();
    static const tPropertyNameMap& getPropertyNameMapForLineSeriesProperties();
    static const tPropertyNameMap& getPropertyNameMapForTextLabelProperties();

    static void getTextLabelMultiPropertyLists(
                const css::uno::Reference< css::beans::XPropertySet >& xSourceProp
                , tNameSequence& rPropNames, tAnySequence& rPropValues
                , bool bName=true
                , sal_Int32 nLimitedSpace=-1
                , bool bLimitedHeight=false
                , bool bSupportsLabelBorder = true);

    /** adds line-, fill- and character properties and sets some suitable
        defaults for auto-grow properties
     */
    static void getPreparedTextShapePropertyLists(
        const css::uno::Reference< css::beans::XPropertySet >& xSourceProp
        , tNameSequence& rPropNames
        , tAnySequence& rPropValues );
};

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
