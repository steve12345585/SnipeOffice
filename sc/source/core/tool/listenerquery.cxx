/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <listenerquery.hxx>
#include <listenerqueryids.hxx>
#include <address.hxx>

namespace sc {

RefQueryFormulaGroup::RefQueryFormulaGroup() :
    SvtListener::QueryBase(SC_LISTENER_QUERY_FORMULA_GROUP_POS),
    maSkipRange(ScAddress::INITIALIZE_INVALID) {}

RefQueryFormulaGroup::~RefQueryFormulaGroup() {}

void RefQueryFormulaGroup::setSkipRange( const ScRange& rRange )
{
    maSkipRange = rRange;
}

void RefQueryFormulaGroup::add( const ScAddress& rPos )
{
    if (!rPos.IsValid())
        return;

    if (maSkipRange.IsValid() && maSkipRange.Contains(rPos))
        // This is within the skip range.  Skip it.
        return;

    TabsType::iterator itTab = maTabs.find(rPos.Tab());
    if (itTab == maTabs.end())
    {
        std::pair<TabsType::iterator,bool> r =
            maTabs.emplace(rPos.Tab(), ColsType());
        if (!r.second)
            // Insertion failed.
            return;

        itTab = r.first;
    }

    ColsType& rCols = itTab->second;
    ColsType::iterator itCol = rCols.find(rPos.Col());
    if (itCol == rCols.end())
    {
        std::pair<ColsType::iterator,bool> r =
            rCols.emplace(rPos.Col(), ColType());
        if (!r.second)
            // Insertion failed.
            return;

        itCol = r.first;
    }

    ColType& rCol = itCol->second;
    rCol.push_back(rPos.Row());
}

const RefQueryFormulaGroup::TabsType& RefQueryFormulaGroup::getAllPositions() const
{
    return maTabs;
}

QueryRange::QueryRange() :
    SvtListener::QueryBase(SC_LISTENER_QUERY_FORMULA_GROUP_RANGE)
{}

QueryRange::~QueryRange()
{
}

void QueryRange::add( const ScRange& rRange )
{
    maRanges.Join(rRange);
}

void QueryRange::swapRanges( ScRangeList& rRanges )
{
    maRanges.swap(rRanges);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
