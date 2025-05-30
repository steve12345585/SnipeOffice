/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#include <svx/ClassificationField.hxx>

#include <svl/itempool.hxx>
#include <svl/itemset.hxx>
#include <editeng/wghtitem.hxx>
#include <editeng/eeitem.hxx>

#include "ClassificationEditView.hxx"

namespace svx {

ClassificationEditEngine::ClassificationEditEngine(SfxItemPool* pItemPool)
    : EditEngine(pItemPool)
{}

OUString ClassificationEditEngine::CalcFieldValue(const SvxFieldItem& rField, sal_Int32 /*nPara*/,
                                                  sal_Int32 /*nPos*/, std::optional<Color>& /*rTxtColor*/, std::optional<Color>& /*rFldColor*/, std::optional<FontLineStyle>& /*rFldLineStyle*/)
{
    OUString aString;
    const ClassificationField* pClassificationField = dynamic_cast<const ClassificationField*>(rField.GetField());
    if (pClassificationField)
        aString = pClassificationField->msDescription;
    else
        aString = "Unknown";
    return aString;
}

ClassificationEditView::ClassificationEditView()
{
}

void ClassificationEditView::makeEditEngine()
{
    m_xEditEngine.reset(new ClassificationEditEngine(EditEngine::CreatePool().get()));
}

ClassificationEditView::~ClassificationEditView()
{
}

void ClassificationEditView::InsertField(const SvxFieldItem& rFieldItem)
{
    m_xEditView->InsertField(rFieldItem);
    m_xEditView->Invalidate();
}

void ClassificationEditView::InvertSelectionWeight()
{
    ESelection aSelection = m_xEditView->GetSelection();

    for (sal_Int32 nParagraph = aSelection.start.nPara; nParagraph <= aSelection.end.nPara; ++nParagraph)
    {
        FontWeight eFontWeight = WEIGHT_BOLD;

        SfxItemSet aSet(m_xEditEngine->GetParaAttribs(nParagraph));
        if (const SfxPoolItem* pItem = aSet.GetItem(EE_CHAR_WEIGHT, false))
        {
            const SvxWeightItem* pWeightItem = dynamic_cast<const SvxWeightItem*>(pItem);
            if (pWeightItem && pWeightItem->GetWeight() == WEIGHT_BOLD)
                eFontWeight = WEIGHT_NORMAL;
        }
        SvxWeightItem aWeight(eFontWeight, EE_CHAR_WEIGHT);
        aSet.Put(aWeight);
        m_xEditEngine->SetParaAttribs(nParagraph, aSet);
    }

    m_xEditView->Invalidate();
}

} // end sfx2

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
