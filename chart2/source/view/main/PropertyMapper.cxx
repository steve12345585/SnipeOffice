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

#include <PropertyMapper.hxx>
#include <unonames.hxx>

#include <com/sun/star/beans/XMultiPropertySet.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/drawing/TextVerticalAdjust.hpp>
#include <com/sun/star/drawing/TextHorizontalAdjust.hpp>
#include <com/sun/star/drawing/LineJoint.hpp>
#include <com/sun/star/style/ParagraphAdjust.hpp>
#include <comphelper/diagnose_ex.hxx>
#include <svx/unoshape.hxx>

namespace chart
{
using namespace ::com::sun::star;

void PropertyMapper::setMappedProperties(
          SvxShape& xTarget
        , const uno::Reference< beans::XPropertySet >& xSource
        , const tPropertyNameMap& rMap )
{
    if( !xSource.is() )
        return;

    sal_Int32 nPropertyCount = rMap.size();
    tNameSequence aNames(nPropertyCount);
    tAnySequence  aValues(nPropertyCount);
    auto pNames = aNames.getArray();
    auto pValues = aValues.getArray();
    sal_Int32 nN=0;

    for (auto const& elem : rMap)
    {
        const OUString & rTarget = elem.first;
        const OUString & rSource = elem.second;
        try
        {
            uno::Any aAny( xSource->getPropertyValue(rSource) );
            if( aAny.hasValue() )
            {
                //do not set empty anys because of performance (otherwise SdrAttrObj::ItemChange will take much longer)
                pNames[nN]  = rTarget;
                pValues[nN] = std::move(aAny);
                ++nN;
            }
        }
        catch( const uno::Exception& )
        {
            TOOLS_WARN_EXCEPTION("chart2", "" );
        }
    }
    if (nN == 0)
        return;
    //reduce to real property count
    aNames.realloc(nN);
    aValues.realloc(nN);

    uno::Reference< beans::XMultiPropertySet > xShapeMultiProp( xTarget, uno::UNO_QUERY_THROW );
    try
    {
        xShapeMultiProp->setPropertyValues( aNames, aValues );
    }
    catch( const uno::Exception& )
    {
        TOOLS_WARN_EXCEPTION("chart2", "" ); //if this occurs more often think of removing the XMultiPropertySet completely for better performance
    }
}

void PropertyMapper::setMappedProperties(
          const uno::Reference< beans::XPropertySet >& xTarget
        , const uno::Reference< beans::XPropertySet >& xSource
        , const tPropertyNameMap& rMap )
{
    if( !xTarget.is() || !xSource.is() )
        return;

    tNameSequence aNames;
    tAnySequence  aValues;
    sal_Int32 nN=0;
    sal_Int32 nPropertyCount = rMap.size();
    aNames.realloc(nPropertyCount);
    auto pNames = aNames.getArray();
    aValues.realloc(nPropertyCount);
    auto pValues = aValues.getArray();

    for (auto const& elem : rMap)
    {
        const OUString & rTarget = elem.first;
        const OUString & rSource = elem.second;
        try
        {
            uno::Any aAny( xSource->getPropertyValue(rSource) );
            if( aAny.hasValue() )
            {
                //do not set empty anys because of performance (otherwise SdrAttrObj::ItemChange will take much longer)
                pNames[nN]  = rTarget;
                pValues[nN] = std::move(aAny);
                ++nN;
            }
        }
        catch( const uno::Exception& )
        {
            TOOLS_WARN_EXCEPTION("chart2", "exception mapping property from " << rSource << " to " << rTarget);
        }
    }
    if (nN == 0)
        return;

    uno::Reference< beans::XMultiPropertySet > xShapeMultiProp( xTarget, uno::UNO_QUERY );
    if (xShapeMultiProp)
        try
        {
            //reduce to real property count
            aNames.realloc(nN);
            aValues.realloc(nN);
            xShapeMultiProp->setPropertyValues( aNames, aValues );
            return; // successful
        }
        catch( const uno::Exception& )
        {
            TOOLS_WARN_EXCEPTION("chart2", "" ); //if this occurs more often think of removing the XMultiPropertySet completely for better performance
        }

    // fall back to one at a time
    try
    {
        for( sal_Int32 i = 0; i < nN; i++ )
        {
            try
            {
                xTarget->setPropertyValue( aNames[i], aValues[i] );
            }
            catch( const uno::Exception& )
            {
                TOOLS_WARN_EXCEPTION("chart2", "" );
            }
        }
    }
    catch( const uno::Exception& )
    {
        TOOLS_WARN_EXCEPTION("chart2", "" );
    }
}

void PropertyMapper::getValueMap(
                  tPropertyNameValueMap& rValueMap
                , const tPropertyNameMap& rNameMap
                , const uno::Reference< beans::XPropertySet >& xSourceProp
                )
{
    uno::Reference< beans::XMultiPropertySet > xMultiPropSet(xSourceProp, uno::UNO_QUERY);
    if((false) && xMultiPropSet.is())
    {
        uno::Sequence< OUString > aPropSourceNames(rNameMap.size());
        auto aPropSourceNamesRange = asNonConstRange(aPropSourceNames);
        uno::Sequence< OUString > aPropTargetNames(rNameMap.size());
        auto aPropTargetNamesRange = asNonConstRange(aPropTargetNames);
        sal_Int32 i = 0;
        for (auto const& elem : rNameMap)
        {
            aPropTargetNamesRange[i] = elem.first;
            aPropSourceNamesRange[i] = elem.second;
            ++i;
        }

        uno::Sequence< uno::Any > xValues = xMultiPropSet->getPropertyValues(aPropSourceNames);
        sal_Int32 n = rNameMap.size();
        for(i = 0;i < n; ++i)
        {
            if( xValues[i].hasValue() )
                rValueMap.emplace(  aPropTargetNames[i], xValues[i] );
        }
    }
    else
    {
        for (auto const& elem : rNameMap)
        {
            const OUString & rTarget = elem.first;
            const OUString & rSource = elem.second;
            try
            {
                uno::Any aAny( xSourceProp->getPropertyValue(rSource) );
                if( aAny.hasValue() )
                    rValueMap.emplace(  rTarget, aAny );
            }
            catch( const uno::Exception& )
            {
                TOOLS_WARN_EXCEPTION("chart2", "" );
            }
        }
    }
}

void PropertyMapper::getMultiPropertyListsFromValueMap(
                  tNameSequence& rNames
                , tAnySequence&  rValues
                , const tPropertyNameValueMap& rValueMap
                )
{
    sal_Int32 nPropertyCount = rValueMap.size();
    rNames.realloc(nPropertyCount);
    auto pNames = rNames.getArray();
    rValues.realloc(nPropertyCount);
    auto pValues = rValues.getArray();

    //fill sequences
    sal_Int32 nN=0;
    for (auto const& elem : rValueMap)
    {
        const uno::Any& rAny = elem.second;
        if( rAny.hasValue() )
        {
            //do not set empty anys because of performance (otherwise SdrAttrObj::ItemChange will take much longer)
            pNames[nN]  = elem.first;
            pValues[nN] = rAny;
            ++nN;
        }
    }
    //reduce to real property count
    rNames.realloc(nN);
    rValues.realloc(nN);
}

uno::Any* PropertyMapper::getValuePointer( tAnySequence& rPropValues
                         , const tNameSequence& rPropNames
                         , std::u16string_view rPropName )
{
    sal_Int32 nCount = rPropNames.getLength();
    for( sal_Int32 nN = 0; nN < nCount; nN++ )
    {
        if(rPropNames[nN] == rPropName)
            return &rPropValues.getArray()[nN];
    }
    return nullptr;
}

uno::Any* PropertyMapper::getValuePointerForLimitedSpace( tAnySequence& rPropValues
                         , const tNameSequence& rPropNames
                         , bool bLimitedHeight)
{
    return PropertyMapper::getValuePointer( rPropValues, rPropNames
        , bLimitedHeight ? u"TextMaximumFrameHeight"_ustr : u"TextMaximumFrameWidth"_ustr );
}

const tPropertyNameMap& PropertyMapper::getPropertyNameMapForCharacterProperties()
{
    //shape property -- chart model object property
    static tPropertyNameMap s_aShapePropertyMapForCharacterProperties{
        {"CharColor",                "CharColor"},
        {"CharContoured",            "CharContoured"},
        {"CharEmphasis",             "CharEmphasis"},//the service style::CharacterProperties  describes a property called 'CharEmphasize' which is nowhere implemented
        {"CharEscapement",           "CharEscapement"},
        {"CharEscapementHeight",     "CharEscapementHeight"},
        {"CharFontFamily",           "CharFontFamily"},
        {"CharFontFamilyAsian",      "CharFontFamilyAsian"},
        {"CharFontFamilyComplex",    "CharFontFamilyComplex"},
        {"CharFontCharSet",          "CharFontCharSet"},
        {"CharFontCharSetAsian",     "CharFontCharSetAsian"},
        {"CharFontCharSetComplex",   "CharFontCharSetComplex"},
        {"CharFontName",             "CharFontName"},
        {"CharFontNameAsian",        "CharFontNameAsian"},
        {"CharFontNameComplex",      "CharFontNameComplex"},
        {"CharFontPitch",            "CharFontPitch"},
        {"CharFontPitchAsian",       "CharFontPitchAsian"},
        {"CharFontPitchComplex",     "CharFontPitchComplex"},
        {"CharFontStyleName",        "CharFontStyleName"},
        {"CharFontStyleNameAsian",   "CharFontStyleNameAsian"},
        {"CharFontStyleNameComplex", "CharFontStyleNameComplex"},

        {"CharHeight",               "CharHeight"},
        {"CharHeightAsian",          "CharHeightAsian"},
        {"CharHeightComplex",        "CharHeightComplex"},
        {"CharKerning",              "CharKerning"},
        {"CharLocale",               "CharLocale"},
        {"CharLocaleAsian",          "CharLocaleAsian"},
        {"CharLocaleComplex",        "CharLocaleComplex"},
        {"CharPosture",              "CharPosture"},
        {"CharPostureAsian",         "CharPostureAsian"},
        {"CharPostureComplex",       "CharPostureComplex"},
        {"CharRelief",               "CharRelief"},
        {"CharShadowed",             "CharShadowed"},
        {"CharStrikeout",            "CharStrikeout"},
        {"CharUnderline",            "CharUnderline"},
        {"CharUnderlineColor",       "CharUnderlineColor"},
        {"CharUnderlineHasColor",    "CharUnderlineHasColor"},
        {"CharOverline",             "CharOverline"},
        {"CharOverlineColor",        "CharOverlineColor"},
        {"CharOverlineHasColor",     "CharOverlineHasColor"},
        {"CharWeight",               "CharWeight"},
        {"CharWeightAsian",          "CharWeightAsian"},
        {"CharWeightComplex",        "CharWeightComplex"},
        {"CharWordMode",             "CharWordMode"},

        {"WritingMode",              "WritingMode"},

        {"ParaIsCharacterDistance",  "ParaIsCharacterDistance"}};

    return s_aShapePropertyMapForCharacterProperties;
}

const tPropertyNameMap& PropertyMapper::getPropertyNameMapForParagraphProperties()
{
    //shape property -- chart model object property
    static tPropertyNameMap s_aShapePropertyMapForParagraphProperties{
        {"ParaAdjust",          "ParaAdjust"},
        {"ParaBottomMargin",    "ParaBottomMargin"},
        {"ParaIsHyphenation",   "ParaIsHyphenation"},
        {"ParaLastLineAdjust",  "ParaLastLineAdjust"},
        {"ParaLeftMargin",      "ParaLeftMargin"},
        {"ParaRightMargin",     "ParaRightMargin"},
        {"ParaTopMargin",       "ParaTopMargin"}};
    return s_aShapePropertyMapForParagraphProperties;
}

const tPropertyNameMap& PropertyMapper::getPropertyNameMapForFillProperties()
{
    //shape property -- chart model object property
    static tPropertyNameMap s_aShapePropertyMapForFillProperties{
        {"FillBackground",               "FillBackground"},
        {"FillBitmapName",               "FillBitmapName"},
        {"FillColor",                    "FillColor"},
        {"FillGradientName",             "FillGradientName"},
        {"FillGradientStepCount",        "FillGradientStepCount"},
        {"FillHatchName",                "FillHatchName"},
        {"FillStyle",                    "FillStyle"},
        {"FillTransparence",             "FillTransparence"},
        {"FillTransparenceGradientName", "FillTransparenceGradientName"},
        //bitmap properties
        {"FillBitmapMode",               "FillBitmapMode"},
        {"FillBitmapSizeX",              "FillBitmapSizeX"},
        {"FillBitmapSizeY",              "FillBitmapSizeY"},
        {"FillBitmapLogicalSize",        "FillBitmapLogicalSize"},
        {"FillBitmapOffsetX",            "FillBitmapOffsetX"},
        {"FillBitmapOffsetY",            "FillBitmapOffsetY"},
        {"FillBitmapRectanglePoint",     "FillBitmapRectanglePoint"},
        {"FillBitmapPositionOffsetX",    "FillBitmapPositionOffsetX"},
        {"FillBitmapPositionOffsetY",    "FillBitmapPositionOffsetY"}};
    return s_aShapePropertyMapForFillProperties;
}

const tPropertyNameMap& PropertyMapper::getPropertyNameMapForLineProperties()
{
    //shape property -- chart model object property
    static tPropertyNameMap s_aShapePropertyMapForLineProperties{
        {"LineColor",              "LineColor"},
        {"LineDashName",           "LineDashName"},
        {"LineJoint",              "LineJoint"},
        {"LineStyle",              "LineStyle"},
        {"LineTransparence",       "LineTransparence"},
        {"LineWidth",              "LineWidth"},
        {"LineCap",                "LineCap"}};
    return s_aShapePropertyMapForLineProperties;
}

namespace {
    tPropertyNameMap getPropertyNameMapForFillAndLineProperties_() {
        auto map = PropertyMapper::getPropertyNameMapForFillProperties();
        auto const & add
            = PropertyMapper::getPropertyNameMapForLineProperties();
        map.insert(add.begin(), add.end());
        return map;
    }
}
const tPropertyNameMap& PropertyMapper::getPropertyNameMapForFillAndLineProperties()
{
    static tPropertyNameMap s_aShapePropertyMapForFillAndLineProperties
        = getPropertyNameMapForFillAndLineProperties_();
    return s_aShapePropertyMapForFillAndLineProperties;
}

namespace {
    tPropertyNameMap getPropertyNameMapForTextShapeProperties_() {
        auto map = PropertyMapper::getPropertyNameMapForCharacterProperties();
        auto const & add1
            = PropertyMapper::getPropertyNameMapForFillProperties();
        map.insert(add1.begin(), add1.end());
        auto const & add2
            = PropertyMapper::getPropertyNameMapForLineProperties();
        map.insert(add2.begin(), add2.end());
        return map;
    }
}
const tPropertyNameMap& PropertyMapper::getPropertyNameMapForTextShapeProperties()
{
    static tPropertyNameMap s_aShapePropertyMapForTextShapeProperties
        = getPropertyNameMapForTextShapeProperties_();
    return s_aShapePropertyMapForTextShapeProperties;
}

const tPropertyNameMap& PropertyMapper::getPropertyNameMapForLineSeriesProperties()
{
    //shape property -- chart model object property
    static tPropertyNameMap s_aShapePropertyMapForLineSeriesProperties{
        {"LineColor",           "Color"},
        {"LineDashName",        "LineDashName"},
        {"LineStyle",           "LineStyle"},
        {"LineTransparence",    "Transparency"},
        {"LineWidth",           "LineWidth"},
        {"LineCap",             "LineCap"}};
    return s_aShapePropertyMapForLineSeriesProperties;
}

namespace {
    tPropertyNameMap getPropertyNameMapForTextLabelProperties_() {
        auto map = PropertyMapper::getPropertyNameMapForCharacterProperties();
        map.insert({
            {"LineStyle", CHART_UNONAME_LABEL_BORDER_STYLE},
            {"LineWidth", CHART_UNONAME_LABEL_BORDER_WIDTH},
            {"LineColor", CHART_UNONAME_LABEL_BORDER_COLOR},
            {"LineTransparence", CHART_UNONAME_LABEL_BORDER_TRANS},
            {"FillStyle", CHART_UNONAME_LABEL_FILL_STYLE},
            {"FillColor", CHART_UNONAME_LABEL_FILL_COLOR},
            {"FillBackground", CHART_UNONAME_LABEL_FILL_BACKGROUND},
            {"FillHatchName", CHART_UNONAME_LABEL_FILL_HATCH_NAME}
            });
                // fix the spelling!
        return map;
    }
}
const tPropertyNameMap& PropertyMapper::getPropertyNameMapForTextLabelProperties()
{
    // target name (drawing layer) : source name (chart model)
    static tPropertyNameMap aMap = getPropertyNameMapForTextLabelProperties_();
    return aMap;
}

const tPropertyNameMap& PropertyMapper::getPropertyNameMapForFilledSeriesProperties()
{
    //shape property -- chart model object property
    static tPropertyNameMap s_aShapePropertyMapForFilledSeriesProperties{
        {"FillBackground",               "FillBackground"},
        {"FillBitmapName",               "FillBitmapName"},
        {"FillColor",                    "Color"},
        {"FillGradientName",             "GradientName"},
        {"FillGradientStepCount",        "GradientStepCount"},
        {"FillHatchName",                "HatchName"},
        {"FillStyle",                    "FillStyle"},
        {"FillTransparence",             "Transparency"},
        {"FillTransparenceGradientName", "TransparencyGradientName"},
        //bitmap properties
        {"FillBitmapMode",               "FillBitmapMode"},
        {"FillBitmapSizeX",              "FillBitmapSizeX"},
        {"FillBitmapSizeY",              "FillBitmapSizeY"},
        {"FillBitmapLogicalSize",        "FillBitmapLogicalSize"},
        {"FillBitmapOffsetX",            "FillBitmapOffsetX"},
        {"FillBitmapOffsetY",            "FillBitmapOffsetY"},
        {"FillBitmapRectanglePoint",     "FillBitmapRectanglePoint"},
        {"FillBitmapPositionOffsetX",    "FillBitmapPositionOffsetX"},
        {"FillBitmapPositionOffsetY",    "FillBitmapPositionOffsetY"},
        //line properties
        {"LineColor",                    "BorderColor"},
        {"LineDashName",                 "BorderDashName"},
        {"LineStyle",                    "BorderStyle"},
        {"LineTransparence",             "BorderTransparency"},
        {"LineWidth",                    "BorderWidth"},
        {"LineCap",                      "LineCap"}};
    return s_aShapePropertyMapForFilledSeriesProperties;
}

void PropertyMapper::setMultiProperties(
                  const tNameSequence& rNames
                , const tAnySequence&  rValues
                , SvxShape& xTarget )
{
    try
    {
        xTarget.setPropertyValues( rNames, rValues );
    }
    catch( const uno::Exception& )
    {
        TOOLS_WARN_EXCEPTION("chart2", "" ); //if this occurs more often think of removing the XMultiPropertySet completely for better performance
    }
}

void PropertyMapper::getTextLabelMultiPropertyLists(
    const uno::Reference< beans::XPropertySet >& xSourceProp
    , tNameSequence& rPropNames, tAnySequence& rPropValues
    , bool bName
    , sal_Int32 nLimitedSpace
    , bool bLimitedHeight
    , bool bSupportsLabelBorder)
{
    //fill character properties into the ValueMap
    tPropertyNameValueMap aValueMap;
    tPropertyNameMap const & aNameMap = bSupportsLabelBorder ? PropertyMapper::getPropertyNameMapForTextLabelProperties() : getPropertyNameMapForCharacterProperties();

    PropertyMapper::getValueMap(aValueMap, aNameMap, xSourceProp);

    //some more shape properties apart from character properties, position-matrix and label string
    aValueMap.emplace( "TextHorizontalAdjust", uno::Any(drawing::TextHorizontalAdjust_CENTER) ); // drawing::TextHorizontalAdjust - needs to be overwritten
    aValueMap.emplace( "TextVerticalAdjust", uno::Any(drawing::TextVerticalAdjust_CENTER) ); //drawing::TextVerticalAdjust - needs to be overwritten
    aValueMap.emplace( "TextAutoGrowHeight", uno::Any(true) ); // sal_Bool
    aValueMap.emplace( "TextAutoGrowWidth", uno::Any(true) ); // sal_Bool
    aValueMap.emplace( "ParaAdjust", uno::Any(style::ParagraphAdjust_CENTER) ); // style::ParagraphAdjust_CENTER - needs to be overwritten
    if( bName )
        aValueMap.emplace( "Name", uno::Any( OUString() ) ); //CID OUString - needs to be overwritten for each point

    if( nLimitedSpace > 0 )
    {
        if(bLimitedHeight)
            aValueMap.emplace( "TextMaximumFrameHeight", uno::Any(nLimitedSpace) ); //sal_Int32
        else
            aValueMap.emplace( "TextMaximumFrameWidth", uno::Any(nLimitedSpace) ); //sal_Int32
        aValueMap.emplace( "ParaIsHyphenation", uno::Any(true) );
    }

    PropertyMapper::getMultiPropertyListsFromValueMap( rPropNames, rPropValues, aValueMap );
}

void PropertyMapper::getPreparedTextShapePropertyLists(
    const uno::Reference< beans::XPropertySet >& xSourceProp
    , tNameSequence& rPropNames, tAnySequence& rPropValues )
{
    //fill character, line and fill properties into the ValueMap
    tPropertyNameValueMap aValueMap;
    PropertyMapper::getValueMap( aValueMap
            , PropertyMapper::getPropertyNameMapForTextShapeProperties()
            , xSourceProp );

    // auto-grow makes sure the shape has the correct size after setting text
    aValueMap.emplace( "TextHorizontalAdjust", uno::Any( drawing::TextHorizontalAdjust_CENTER ));
    aValueMap.emplace( "TextVerticalAdjust", uno::Any( drawing::TextVerticalAdjust_CENTER ));
    aValueMap.emplace( "TextAutoGrowHeight", uno::Any( true ));
    aValueMap.emplace( "TextAutoGrowWidth", uno::Any( true ));

    // set some distance to the border, in case it is shown
    const sal_Int32 nWidthDist  = 250;
    const sal_Int32 nHeightDist = 125;
    aValueMap.emplace( "TextLeftDistance",  uno::Any( nWidthDist ));
    aValueMap.emplace( "TextRightDistance", uno::Any( nWidthDist ));
    aValueMap.emplace( "TextUpperDistance", uno::Any( nHeightDist ));
    aValueMap.emplace( "TextLowerDistance", uno::Any( nHeightDist ));

    // use a line-joint showing the border of thick lines like two rectangles
    // filled in between.
    aValueMap[u"LineJoint"_ustr] <<= drawing::LineJoint_ROUND;

    PropertyMapper::getMultiPropertyListsFromValueMap( rPropNames, rPropValues, aValueMap );
}

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
