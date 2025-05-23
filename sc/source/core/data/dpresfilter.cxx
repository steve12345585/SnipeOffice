/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <dpresfilter.hxx>
#include <global.hxx>

#include <unotools/charclass.hxx>
#include <sal/log.hxx>
#include <o3tl/hash_combine.hxx>

#include <com/sun/star/sheet/DataPilotFieldFilter.hpp>
#include <com/sun/star/uno/Sequence.hxx>

#include <limits>
#include <utility>

using namespace com::sun::star;

ScDPResultFilter::ScDPResultFilter(OUString aDimName, bool bDataLayout) :
    maDimName(std::move(aDimName)), mbHasValue(false), mbDataLayout(bDataLayout) {}

ScDPResultFilterContext::ScDPResultFilterContext() :
    mnCol(0), mnRow(0) {}

size_t ScDPResultTree::NamePairHash::operator() (const NamePairType& rPair) const
{
    std::size_t seed = 0;
    o3tl::hash_combine(seed, rPair.first.hashCode());
    o3tl::hash_combine(seed, rPair.second.hashCode());
    return seed;
}

#if DEBUG_PIVOT_TABLE
void ScDPResultTree::DimensionNode::dump(int nLevel) const
{
    string aIndent(nLevel*2, ' ');
    MembersType::const_iterator it = maChildMembersValueNames.begin(), itEnd = maChildMembersValueNames.end();
    for (; it != itEnd; ++it)
    {
        cout << aIndent << "member: ";
        const ScDPItemData& rVal = it->first;
        if (rVal.IsValue())
            cout << rVal.GetValue();
        else
            cout << rVal.GetString();
        cout << endl;

        it->second->dump(nLevel+1);
    }
}
#endif

ScDPResultTree::MemberNode::MemberNode() {}

ScDPResultTree::MemberNode::~MemberNode() {}

#if DEBUG_PIVOT_TABLE
void ScDPResultTree::MemberNode::dump(int nLevel) const
{
    string aIndent(nLevel*2, ' ');
    for (const auto& rValue : maValues)
        cout << aIndent << "value: " << rValue << endl;

    for (const auto& [rName, rxDim] : maChildDimensions)
    {
        cout << aIndent << "dimension: " << rName << endl;
        rxDim->dump(nLevel+1);
    }
}
#endif

ScDPResultTree::ScDPResultTree() : mpRoot(new MemberNode) {}
ScDPResultTree::~ScDPResultTree()
{
}

void ScDPResultTree::add(
    const std::vector<ScDPResultFilter>& rFilters, double fVal)
{
    // TODO: I'll work on the col / row to value node mapping later.

    const OUString* pDimName = nullptr;
    const OUString* pMemName = nullptr;
    MemberNode* pMemNode = mpRoot.get();

    for (const ScDPResultFilter& filter : rFilters)
    {
        if (filter.mbDataLayout)
            continue;

        if (maPrimaryDimName.isEmpty())
            maPrimaryDimName = filter.maDimName;

        // See if this dimension exists.
        auto& rDims = pMemNode->maChildDimensions;
        OUString aUpperName = ScGlobal::getCharClass().uppercase(filter.maDimName);
        auto itDim = rDims.find(aUpperName);
        if (itDim == rDims.end())
        {
            // New dimension.  Insert it.
            auto r = rDims.emplace(aUpperName, DimensionNode());
            assert(r.second);
            itDim = r.first;
        }

        pDimName = &itDim->first;

        // Now, see if this dimension member exists.
        DimensionNode& rDim = itDim->second;
        MembersType& rMembersValueNames = rDim.maChildMembersValueNames;
        aUpperName = ScGlobal::getCharClass().uppercase(filter.maValueName);
        MembersType::iterator itMem = rMembersValueNames.find(aUpperName);
        if (itMem == rMembersValueNames.end())
        {
            // New member.  Insert it.
            auto pNode = std::make_shared<MemberNode>();
            std::pair<MembersType::iterator, bool> r =
                rMembersValueNames.emplace(aUpperName, pNode);

            if (!r.second)
                // Insertion failed!
                return;

            itMem = r.first;

            // If the locale independent value string isn't any different it
            // makes no sense to add it to the separate mapping.
            if (!filter.maValue.isEmpty() && filter.maValue != filter.maValueName)
            {
                MembersType& rMembersValues = rDim.maChildMembersValues;
                aUpperName = ScGlobal::getCharClass().uppercase(filter.maValue);
                MembersType::iterator itMemVal = rMembersValues.find(aUpperName);
                if (itMemVal == rMembersValues.end())
                {
                    // New member.  Insert it.
                    std::pair<MembersType::iterator, bool> it =
                        rMembersValues.emplace(aUpperName, pNode);
                    // If insertion failed do not bail out anymore.
                    SAL_WARN_IF( !it.second, "sc.core", "ScDPResultTree::add - rMembersValues.insert failed");
                }
            }
        }

        pMemName = &itMem->first;
        pMemNode = itMem->second.get();
    }

    if (pDimName && pMemName)
    {
        NamePairType aNames(
            ScGlobal::getCharClass().uppercase(*pDimName),
            ScGlobal::getCharClass().uppercase(*pMemName));

        LeafValuesType::iterator it = maLeafValues.find(aNames);
        if (it == maLeafValues.end())
        {
            // This name pair doesn't exist.  Associate a new value for it.
            maLeafValues.emplace(aNames, fVal);
        }
        else
        {
            // This name pair already exists. Set the value to NaN.
            it->second = std::numeric_limits<double>::quiet_NaN();
        }
    }

    pMemNode->maValues.push_back(fVal);
}

void ScDPResultTree::swap(ScDPResultTree& rOther)
{
    std::swap(maPrimaryDimName, rOther.maPrimaryDimName);
    std::swap(mpRoot, rOther.mpRoot);
    maLeafValues.swap(rOther.maLeafValues);
}

bool ScDPResultTree::empty() const
{
    return mpRoot->maChildDimensions.empty();
}

void ScDPResultTree::clear()
{
    maPrimaryDimName.clear();
    mpRoot.reset( new MemberNode );
}

const ScDPResultTree::ValuesType* ScDPResultTree::getResults(
    const uno::Sequence<sheet::DataPilotFieldFilter>& rFilters) const
{
    const MemberNode* pMember = mpRoot.get();
    for (const sheet::DataPilotFieldFilter& rFilter : rFilters)
    {
        auto itDim = pMember->maChildDimensions.find(
            ScGlobal::getCharClass().uppercase(rFilter.FieldName));

        if (itDim == pMember->maChildDimensions.end())
            // Specified dimension not found.
            return nullptr;

        const DimensionNode& rDim = itDim->second;
        MembersType::const_iterator itMem( rDim.maChildMembersValueNames.find(
                    ScGlobal::getCharClass().uppercase( rFilter.MatchValueName)));

        if (itMem == rDim.maChildMembersValueNames.end())
        {
            // Specified member name not found, try locale independent value.
            itMem = rDim.maChildMembersValues.find( ScGlobal::getCharClass().uppercase( rFilter.MatchValue));

            if (itMem == rDim.maChildMembersValues.end())
                // Specified member not found.
                return nullptr;
        }

        pMember = itMem->second.get();
    }

    if (pMember->maValues.empty())
    {
        // Descend into dimension member children while there is no result and
        // exactly one dimension field with exactly one member item, for which
        // no further constraint (filter) has to match.
        const MemberNode* pFieldMember = pMember;
        while (pFieldMember->maChildDimensions.size() == 1)
        {
            auto itDim( pFieldMember->maChildDimensions.begin());
            const DimensionNode& rDim = itDim->second;
            if (rDim.maChildMembersValueNames.size() != 1)
                break;  // while
            pFieldMember = rDim.maChildMembersValueNames.begin()->second.get();
            if (!pFieldMember->maValues.empty())
                return &pFieldMember->maValues;
        }
    }

    return &pMember->maValues;
}

double ScDPResultTree::getLeafResult(const css::sheet::DataPilotFieldFilter& rFilter) const
{
    NamePairType aPair(
        ScGlobal::getCharClass().uppercase(rFilter.FieldName),
        ScGlobal::getCharClass().uppercase(rFilter.MatchValueName));

    LeafValuesType::const_iterator it = maLeafValues.find(aPair);
    if (it != maLeafValues.end())
        // Found!
        return it->second;

    // Not found.  Return an NaN.
    return std::numeric_limits<double>::quiet_NaN();
}

#if DEBUG_PIVOT_TABLE
void ScDPResultTree::dump() const
{
    cout << "primary dimension name: " << maPrimaryDimName << endl;
    mpRoot->dump(0);
}
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
