/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <PivotTableFormat.hxx>
#include <pivottablebuffer.hxx>
#include <oox/token/properties.hxx>
#include <oox/token/tokens.hxx>
#include <pivot/PivotTableFormats.hxx>
#include <dpsave.hxx>
#include <dpobject.hxx>

namespace oox::xls
{
PivotTableFormat::PivotTableFormat(PivotTable& rPivotTable)
    : WorkbookHelper(rPivotTable)
    , mrPivotTable(rPivotTable)
{
}

void PivotTableFormat::importFormat(const oox::AttributeList& rAttribs)
{
    mnDxfId = rAttribs.getInteger(XML_dxfId, -1);
}

void PivotTableFormat::importPivotArea(const oox::AttributeList& rAttribs)
{
    mnField = rAttribs.getInteger(XML_field);

    auto oType = rAttribs.getToken(XML_type);
    if (oType)
    {
        switch (*oType)
        {
            case XML_none:
                meType = PivotAreaType::None;
                break;
            case XML_data:
                meType = PivotAreaType::Data;
                break;
            case XML_all:
                meType = PivotAreaType::All;
                break;
            case XML_origin:
                meType = PivotAreaType::Origin;
                break;
            case XML_button:
                meType = PivotAreaType::Button;
                break;
            case XML_topRight:
                meType = PivotAreaType::TopRight;
                break;
            default:
            case XML_normal:
                meType = PivotAreaType::Normal;
                break;
        }
    }

    moField = rAttribs.getUnsigned(XML_field);
    mbDataOnly = rAttribs.getBool(XML_dataOnly, true);
    mbLabelOnly = rAttribs.getBool(XML_labelOnly, false);
    mbGrandRow = rAttribs.getBool(XML_grandRow, false);
    mbGrandCol = rAttribs.getBool(XML_grandCol, false);
    mbCacheIndex = rAttribs.getBool(XML_cacheIndex, false);
    mbOutline = rAttribs.getBool(XML_outline, true);
    moOffset = rAttribs.getXString(XML_offset);
    mbCollapsedLevelsAreSubtotals = rAttribs.getBool(XML_collapsedLevelsAreSubtotals, false);
    moFieldPosition = rAttribs.getUnsigned(XML_fieldPosition);
}

void PivotTableFormat::finalizeImport()
{
    auto pPattern = std::make_shared<ScPatternAttr>(getScDocument().getCellAttributeHelper());

    if (DxfRef pDxf = getStyles().getDxf(mnDxfId))
        pDxf->fillToItemSet(pPattern->GetItemSet());

    ScDPObject* pDPObj = mrPivotTable.getDPObject();
    ScDPSaveData* pSaveData = pDPObj->GetSaveData();

    sc::PivotTableFormats aFormats;

    if (pSaveData->hasFormats())
        aFormats = pSaveData->getFormats();

    // Resolve references - TODO

    sc::PivotTableFormat aFormat;
    if (mbDataOnly)
        aFormat.eType = sc::FormatType::Data;
    else if (mbLabelOnly)
        aFormat.eType = sc::FormatType::Label;

    aFormat.bDataOnly = mbDataOnly;
    aFormat.bLabelOnly = mbLabelOnly;
    aFormat.bOutline = mbOutline;
    aFormat.oFieldPosition = moFieldPosition;

    aFormat.pPattern = std::move(pPattern);
    for (auto& rReference : maReferences)
    {
        if (rReference->mnField)
        {
            aFormat.aSelections.push_back(
                sc::Selection{ .bSelected = rReference->mbSelected,
                               .nField = sal_Int32(*rReference->mnField),
                               .nIndices = rReference->maFieldItemsIndices });
        }
    }
    aFormats.add(aFormat);

    pSaveData->setFormats(aFormats);
}

PivotTableReference& PivotTableFormat::createReference()
{
    auto xReference = std::make_shared<PivotTableReference>(*this);
    maReferences.push_back(xReference);
    return *xReference;
}

PivotTableReference::PivotTableReference(const PivotTableFormat& rFormat)
    : WorkbookHelper(rFormat)
{
}
void PivotTableReference::importReference(const oox::AttributeList& rAttribs)
{
    mnField = rAttribs.getUnsigned(XML_field);
    mnCount = rAttribs.getUnsigned(XML_count);
    mbSelected = rAttribs.getBool(XML_selected, true);
    mbByPosition = rAttribs.getBool(XML_byPosition, false);
    mbRelative = rAttribs.getBool(XML_relative, false);
    mbDefaultSubtotal = rAttribs.getBool(XML_defaultSubtotal, false);
    mbSumSubtotal = rAttribs.getBool(XML_sumSubtotal, false);
    mbCountASubtotal = rAttribs.getBool(XML_countASubtotal, false);
    mbAvgSubtotal = rAttribs.getBool(XML_avgSubtotal, false);
    mbMaxSubtotal = rAttribs.getBool(XML_maxSubtotal, false);
    mbMinSubtotal = rAttribs.getBool(XML_minSubtotal, false);
    mbProductSubtotal = rAttribs.getBool(XML_productSubtotal, false);
    mbCountSubtotal = rAttribs.getBool(XML_countSubtotal, false);
    mbStdDevSubtotal = rAttribs.getBool(XML_stdDevSubtotal, false);
    mbStdDevPSubtotal = rAttribs.getBool(XML_stdDevPSubtotal, false);
    mbVarSubtotal = rAttribs.getBool(XML_varSubtotal, false);
    mbVarPSubtotal = rAttribs.getBool(XML_varPSubtotal, false);
}

void PivotTableReference::addFieldItem(const oox::AttributeList& rAttribs)
{
    auto oSharedItemsIndex = rAttribs.getUnsigned(XML_v); // XML_v - shared items index
    if (oSharedItemsIndex)
    {
        maFieldItemsIndices.push_back(*oSharedItemsIndex);
    }
}

} // namespace oox::xls

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
