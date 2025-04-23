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
#include "vbalistformat.hxx"
#include <utility>
#include <vbahelper/vbahelper.hxx>
#include <ooo/vba/word/WdListApplyTo.hpp>
#include <ooo/vba/word/WdDefaultListBehavior.hpp>
#include <com/sun/star/awt/FontDescriptor.hpp>
#include <com/sun/star/container/XEnumerationAccess.hpp>
#include <com/sun/star/container/XEnumeration.hpp>
#include <com/sun/star/document/XUndoManagerSupplier.hpp>
#include <com/sun/star/beans/XMultiPropertySet.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/style/TabStop.hpp>
#include <com/sun/star/text/PositionAndSpaceMode.hpp>
#include <com/sun/star/text/XTextTable.hpp>
#include <com/sun/star/util/Color.hpp>
#include <comphelper/sequence.hxx>
#include <comphelper/sequenceashashmap.hxx>
#include <comphelper/scopeguard.hxx>
#include <editeng/numitem.hxx>
#include "vbalisttemplate.hxx"
#include "wordvbahelper.hxx"
#include <unotxdoc.hxx>

#include <vector>

using namespace ::ooo::vba;
using namespace ::ooo::vba::word;
using namespace ::com::sun::star;

SwVbaListFormat::SwVbaListFormat( const uno::Reference< ooo::vba::XHelperInterface >& rParent, const uno::Reference< uno::XComponentContext >& rContext, uno::Reference< text::XTextRange >  xTextRange ) : SwVbaListFormat_BASE( rParent, rContext ), mxTextRange(std::move( xTextRange ))
{
}

SwVbaListFormat::~SwVbaListFormat()
{
}

void SAL_CALL SwVbaListFormat::ApplyListTemplate( const css::uno::Reference< word::XListTemplate >& ListTemplate, const css::uno::Any& ContinuePreviousList, const css::uno::Any& ApplyTo, const css::uno::Any& DefaultListBehavior )
{
    bool bContinuePreviousList = true;
    if( ContinuePreviousList.hasValue() )
        ContinuePreviousList >>= bContinuePreviousList;

    // "applyto" must be current selection
    sal_Int32 bApplyTo = word::WdListApplyTo::wdListApplyToSelection;
    if( ApplyTo.hasValue() )
        ApplyTo >>= bApplyTo;
    if( bApplyTo != word::WdListApplyTo::wdListApplyToSelection )
        throw uno::RuntimeException();

    // default behaviour must be wdWord8ListBehavior
    sal_Int32 nDefaultListBehavior = word::WdDefaultListBehavior::wdWord8ListBehavior;
    if( DefaultListBehavior.hasValue() )
        DefaultListBehavior >>= nDefaultListBehavior;
    if( nDefaultListBehavior != word::WdDefaultListBehavior::wdWord8ListBehavior )
        throw uno::RuntimeException();

    uno::Reference< container::XEnumerationAccess > xEnumAccess( mxTextRange, uno::UNO_QUERY_THROW );
    uno::Reference< container::XEnumeration > xEnum = xEnumAccess->createEnumeration();
    if (!xEnum->hasMoreElements())
        return;

    SwVbaListTemplate& rListTemplate = dynamic_cast<SwVbaListTemplate&>(*ListTemplate);

    bool isFirstElement = true;
    do
    {
        uno::Reference< beans::XPropertySet > xProps( xEnum->nextElement(), uno::UNO_QUERY_THROW );
        if( isFirstElement )
        {
            bool isNumberingRestart = !bContinuePreviousList;
            xProps->setPropertyValue(u"ParaIsNumberingRestart"_ustr, uno::Any( isNumberingRestart ) );
            if( isNumberingRestart )
            {
                xProps->setPropertyValue(u"NumberingStartValue"_ustr, uno::Any( sal_Int16(1) ) );
            }
            isFirstElement = false;
        }
        else
        {
            xProps->setPropertyValue(u"ParaIsNumberingRestart"_ustr, uno::Any( false ) );
        }
        rListTemplate.applyListTemplate( xProps );
    }
    while( xEnum->hasMoreElements() );
}

template <class Ref>
static void addParagraphsToList(const Ref& a,
                                std::vector<css::uno::Reference<css::beans::XPropertySet>>& rList)
{
    if (css::uno::Reference<css::lang::XServiceInfo> xInfo{ a, css::uno::UNO_QUERY })
    {
        if (xInfo->supportsService(u"com.sun.star.text.Paragraph"_ustr))
        {
            rList.emplace_back(xInfo, css::uno::UNO_QUERY_THROW);
        }
        else if (xInfo->supportsService(u"com.sun.star.text.TextTable"_ustr))
        {
            css::uno::Reference<css::text::XTextTable> xTable(xInfo, css::uno::UNO_QUERY_THROW);
            const auto aNames = xTable->getCellNames();
            for (const auto& rName : aNames)
            {
                addParagraphsToList(xTable->getCellByName(rName), rList);
            }
        }
    }
    if (css::uno::Reference<css::container::XEnumerationAccess> xEnumAccess{ a,
                                                                             css::uno::UNO_QUERY })
    {
        auto xEnum = xEnumAccess->createEnumeration();
        while (xEnum->hasMoreElements())
            addParagraphsToList(xEnum->nextElement(), rList);
    }
}

void SAL_CALL SwVbaListFormat::ConvertNumbersToText(  )
{
    rtl::Reference<SwXTextDocument> xModel(getThisWordDoc(mxContext));
    css::uno::Reference<css::document::XUndoManager> xUndoManager(xModel->getUndoManager());
    xUndoManager->enterUndoContext(u"ConvertNumbersToText"_ustr);
    xModel->lockControllers();
    comphelper::ScopeGuard g([xModel, xUndoManager]() {
        xModel->unlockControllers();
        xUndoManager->leaveUndoContext();
    });

    std::vector<css::uno::Reference<css::beans::XPropertySet>> aParagraphs;
    addParagraphsToList(mxTextRange, aParagraphs);

    // in reverse order, to get proper label strings
    for (auto iter = aParagraphs.rbegin(); iter != aParagraphs.rend(); ++iter)
    {
        auto& rPropertySet = *iter;
        if (bool bNumber; (rPropertySet->getPropertyValue(u"NumberingIsNumber"_ustr) >>= bNumber) && bNumber)
        {
            css::uno::Reference<css::text::XTextRange> xRange(rPropertySet, css::uno::UNO_QUERY_THROW);
            OUString sLabelString;
            rPropertySet->getPropertyValue(u"ListLabelString"_ustr) >>= sLabelString;
            // sal_Int16 nAdjust = SAL_MAX_INT16; // TODO?
            sal_Int16 nNumberingType = SAL_MAX_INT16; // css::style::NumberingType
            sal_Int16 nPositionAndSpaceMode = SAL_MAX_INT16;
            sal_Int16 nLabelFollowedBy = SAL_MAX_INT16;
            sal_Int32 nListtabStopPosition = SAL_MAX_INT32;
            sal_Int32 nFirstLineIndent = SAL_MAX_INT32;
            sal_Int32 nIndentAt = SAL_MAX_INT32;
            sal_Int32 nLeftMargin = SAL_MAX_INT32;
            sal_Int32 nSymbolTextDistance = SAL_MAX_INT32;
            sal_Int32 nFirstLineOffset = SAL_MAX_INT32;
            OUString sCharStyleName, sBulletChar;
            css::awt::FontDescriptor aBulletFont;
            bool bHasFont;
            css::util::Color aBulletColor = css::util::Color(COL_AUTO);
            bool bHasColor;

            {
                sal_uInt16 nLevel = SAL_MAX_UINT16;
                rPropertySet->getPropertyValue(u"NumberingLevel"_ustr) >>= nLevel;
                css::uno::Reference<css::container::XIndexAccess> xNumberingRules;
                rPropertySet->getPropertyValue(u"NumberingRules"_ustr) >>= xNumberingRules;
                comphelper::SequenceAsHashMap aLevelRule(xNumberingRules->getByIndex(nLevel));

                // See offapi/com/sun/star/text/NumberingLevel.idl
                aLevelRule[u"CharStyleName"_ustr] >>= sCharStyleName;
                aLevelRule[u"NumberingType"_ustr] >>= nNumberingType;
                // TODO: aLevelRule["Adjust"] >>= nAdjust; // HoriOrientation::LEFT/RIGHT/CENTER
                aLevelRule[u"PositionAndSpaceMode"_ustr] >>= nPositionAndSpaceMode;

                // for css::text::PositionAndSpaceMode::LABEL_ALIGNMENT
                aLevelRule[u"LabelFollowedBy"_ustr] >>= nLabelFollowedBy;
                aLevelRule[u"ListtabStopPosition"_ustr] >>= nListtabStopPosition;
                aLevelRule[u"FirstLineIndent"_ustr] >>= nFirstLineIndent;
                aLevelRule[u"IndentAt"_ustr] >>= nIndentAt;

                // for css::text::PositionAndSpaceMode::LABEL_WIDTH_AND_POSITION
                aLevelRule[u"LeftMargin"_ustr] >>= nLeftMargin;
                aLevelRule[u"SymbolTextDistance"_ustr] >>= nSymbolTextDistance;
                aLevelRule[u"FirstLineOffset"_ustr] >>= nFirstLineOffset;

                aLevelRule[u"BulletChar"_ustr] >>= sBulletChar;
                bHasFont = (aLevelRule[u"BulletFont"_ustr] >>= aBulletFont);
                bHasColor = (aLevelRule[u"BulletColor"_ustr] >>= aBulletColor);
            }

            if (nNumberingType != css::style::NumberingType::BITMAP) // TODO
            {
                if (nPositionAndSpaceMode
                    == css::text::PositionAndSpaceMode::LABEL_WIDTH_AND_POSITION)
                {
                    nIndentAt = nLeftMargin;
                    nFirstLineIndent = nFirstLineOffset;
                    nListtabStopPosition = nSymbolTextDistance;
                    nLabelFollowedBy = SvxNumberFormat::LabelFollowedBy::LISTTAB;
                }

                switch (nLabelFollowedBy)
                {
                    case SvxNumberFormat::LabelFollowedBy::LISTTAB:
                        sLabelString += "\t";
                        break;
                    case SvxNumberFormat::LabelFollowedBy::SPACE:
                        sLabelString += " ";
                        break;
                    case SvxNumberFormat::LabelFollowedBy::NEWLINE:
                        sLabelString += "\n";
                        break;
                }

                css::uno::Reference<css::text::XTextRange> xNumberText(xRange->getStart());
                xNumberText->setString(sLabelString);
                css::uno::Reference<css::beans::XPropertySet> xNumberProps(
                    xNumberText, css::uno::UNO_QUERY_THROW);
                if (!sCharStyleName.isEmpty())
                    xNumberProps->setPropertyValue(u"CharStyleName"_ustr, css::uno::Any(sCharStyleName));

                if (nNumberingType == css::style::NumberingType::CHAR_SPECIAL)
                {
                    css::uno::Reference<css::text::XTextRange> xBulletText(xNumberText->getStart());
                    xBulletText->setString(sBulletChar);

                    std::unordered_map<OUString, css::uno::Any> aNameValues;
                    if (bHasFont)
                    {
                        aNameValues.insert({
                            { "CharFontName", css::uno::Any(aBulletFont.Name) },
                            { "CharFontStyleName", css::uno::Any(aBulletFont.StyleName) },
                            { "CharFontFamily", css::uno::Any(aBulletFont.Family) },
                            { "CharFontCharSet", css::uno::Any(aBulletFont.CharSet) },
                            { "CharWeight", css::uno::Any(aBulletFont.Weight) },
                            { "CharUnderline", css::uno::Any(aBulletFont.Underline) },
                            { "CharStrikeout", css::uno::Any(aBulletFont.Strikeout) },
                            { "CharAutoKerning", css::uno::Any(aBulletFont.Kerning) },
                            { "CharFontPitch", css::uno::Any(aBulletFont.Pitch) },
                            { "CharWordMode", css::uno::Any(aBulletFont.WordLineMode) },
                            { "CharRotation", css::uno::Any(static_cast<sal_Int16>(
                                                  std::round(aBulletFont.Orientation * 10))) },
                            });
                        if (aBulletFont.Height)
                            aNameValues[u"CharHeight"_ustr] <<= aBulletFont.Height;
                    }
                    if (bHasColor)
                    {
                        aNameValues[u"CharColor"_ustr] <<= aBulletColor;
                    }

                    if (css::uno::Reference<css::beans::XMultiPropertySet> xBulletMultiProps{
                            xBulletText, css::uno::UNO_QUERY })
                    {
                        xBulletMultiProps->setPropertyValues(
                            comphelper::mapKeysToSequence(aNameValues),
                            comphelper::mapValuesToSequence(aNameValues));
                    }
                    else
                    {
                        css::uno::Reference<css::beans::XPropertySet> xBulletProps(
                            xBulletText, css::uno::UNO_QUERY_THROW);
                        for (const auto& [rName, rVal] : aNameValues)
                            xBulletProps->setPropertyValue(rName, rVal);
                    }
                }
                else
                {
                    // TODO: css::style::NumberingType::BITMAP
                }

                rPropertySet->setPropertyValue(u"ParaLeftMargin"_ustr, css::uno::Any(nIndentAt));
                rPropertySet->setPropertyValue(u"ParaFirstLineIndent"_ustr, css::uno::Any(nFirstLineIndent));
                if (nLabelFollowedBy == SvxNumberFormat::LabelFollowedBy::LISTTAB)
                {
                    css::uno::Sequence<css::style::TabStop> stops;
                    rPropertySet->getPropertyValue(u"ParaTabStops"_ustr) >>= stops;
                    css::style::TabStop tabStop{};
                    tabStop.Position = nListtabStopPosition;
                    tabStop.Alignment = css::style::TabAlign::TabAlign_LEFT;
                    tabStop.FillChar = ' ';
                    rPropertySet->setPropertyValue(u"ParaTabStops"_ustr,
                                    css::uno::Any(comphelper::combineSequences({ tabStop }, stops)));
                    // FIXME: What if added tap stop is greater than already defined ones?
                }
            }
            else
            {
                continue; // for now, keep such lists as is
            }

            // In case of higher outline levels, each assignment of empty value just sets level 1
            while (rPropertySet->getPropertyValue(u"NumberingRules"_ustr) != css::uno::Any())
            {
                rPropertySet->setPropertyValue(u"NumberingRules"_ustr, css::uno::Any());
            }
        }
    }
}

OUString
SwVbaListFormat::getServiceImplName()
{
    return u"SwVbaListFormat"_ustr;
}

uno::Sequence< OUString >
SwVbaListFormat::getServiceNames()
{
    static uno::Sequence< OUString > const aServiceNames
    {
        u"ooo.vba.word.ListFormat"_ustr
    };
    return aServiceNames;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
