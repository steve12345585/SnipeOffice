/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <svl/grabbagitem.hxx>
#include <sal/config.h>

#include <sal/log.hxx>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/uno/Sequence.hxx>
#include <o3tl/hash_combine.hxx>

using namespace com::sun::star;

SfxGrabBagItem::SfxGrabBagItem()
    : SfxPoolItem(0)
{
}

SfxGrabBagItem::SfxGrabBagItem(sal_uInt16 nWhich)
    : SfxPoolItem(nWhich)
{
}

SfxGrabBagItem::SfxGrabBagItem(sal_uInt16 nWhich, std::map<OUString, css::uno::Any> aMap)
    : SfxPoolItem(nWhich)
    , m_aMap(std::move(aMap))
{
}

SfxGrabBagItem::~SfxGrabBagItem() = default;

bool SfxGrabBagItem::operator==(const SfxPoolItem& rItem) const
{
    return SfxPoolItem::operator==(rItem)
           && m_aMap == static_cast<const SfxGrabBagItem*>(&rItem)->m_aMap;
}

size_t SfxGrabBagItem::hashCode() const
{
    std::size_t seed(0);
    for (const auto& rPair : m_aMap)
        o3tl::hash_combine(seed, rPair.first.hashCode());
    return seed;
}

SfxGrabBagItem* SfxGrabBagItem::Clone(SfxItemPool* /*pPool*/) const
{
    return new SfxGrabBagItem(*this);
}

bool SfxGrabBagItem::PutValue(const uno::Any& rVal, sal_uInt8 /*nMemberId*/)
{
    ASSERT_CHANGE_REFCOUNTED_ITEM;
    uno::Sequence<beans::PropertyValue> aValue;
    if (rVal >>= aValue)
    {
        m_aMap.clear();
        for (beans::PropertyValue const& aPropertyValue : aValue)
        {
            m_aMap[aPropertyValue.Name] = aPropertyValue.Value;
        }
        return true;
    }

    SAL_WARN("svl", "SfxGrabBagItem::PutValue: wrong type");
    return false;
}

bool SfxGrabBagItem::QueryValue(uno::Any& rVal, sal_uInt8 /*nMemberId*/) const
{
    uno::Sequence<beans::PropertyValue> aValue(m_aMap.size());
    beans::PropertyValue* pValue = aValue.getArray();
    for (const auto& i : m_aMap)
    {
        pValue[0].Name = i.first;
        pValue[0].Value = i.second;
        ++pValue;
    }
    rVal <<= aValue;
    return true;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
