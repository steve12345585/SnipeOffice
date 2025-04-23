/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <DocumentModelAccessor.hxx>

#include <document.hxx>
#include <docpool.hxx>
#include <svl/intitem.hxx>
#include <svl/zformat.hxx>
#include <svl/zforlist.hxx>
#include <svl/numformat.hxx>
#include <svl/itempool.hxx>

namespace sc
{
std::vector<sfx::CurrencyID> DocumentModelAccessor::getDocumentCurrencies() const
{
    std::vector<sfx::CurrencyID> aCurrencyIDs;

    ItemSurrogates aSurrogates;
    m_pDocument->GetPool()->GetItemSurrogates(aSurrogates, ATTR_VALUE_FORMAT);
    for (const SfxPoolItem* pItem : aSurrogates)
    {
        auto* pIntItem = static_cast<const SfxUInt32Item*>(pItem);
        sal_Int32 nFormat = pIntItem->GetValue();
        SvNumberFormatter* pFormatter = m_pDocument->GetFormatTable();
        if (pFormatter)
        {
            SvNumberformat const* pEntry = pFormatter->GetEntry(nFormat);
            if (pEntry && pEntry->GetMaskedType() == SvNumFormatType::CURRENCY
                && pEntry->HasNewCurrency() && pEntry->GetLanguage() != LANGUAGE_SYSTEM)
            {
                OUString aSymbol;
                OUString aExtension;
                pEntry->GetNewCurrencySymbol(aSymbol, aExtension);
                aCurrencyIDs.push_back({ aSymbol, aExtension, pEntry->GetLanguage() });
            }
        }
    }
    return aCurrencyIDs;
}

} // end sc

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
