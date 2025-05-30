/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_COMPHELPER_PROPERTYSEQUENCE_HXX
#define INCLUDED_COMPHELPER_PROPERTYSEQUENCE_HXX

#include <utility>
#include <algorithm>
#include <initializer_list>
#include <vector>

#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/beans/PropertyValue.hpp>

#include <comphelper/comphelperdllapi.h>

namespace comphelper
{
    /// Init list for property sequences.
    inline css::uno::Sequence< css::beans::PropertyValue > InitPropertySequence(
        ::std::initializer_list< ::std::pair< OUString, css::uno::Any > > vInit)
    {
        css::uno::Sequence< css::beans::PropertyValue> vResult{static_cast<sal_Int32>(vInit.size())};
        std::transform(vInit.begin(), vInit.end(), vResult.getArray(),
                       [](const std::pair<OUString, css::uno::Any>& rInit) {
                           return css::beans::PropertyValue(rInit.first, -1, rInit.second,
                                                            css::beans::PropertyState_DIRECT_VALUE);
                       });
        return vResult;
    }

    /// Init list for property sequences that wrap the PropertyValues in Anys.
    ///
    /// This is particularly useful for creation of sequences that are later
    /// unwrapped using comphelper::SequenceAsHashMap.
    inline css::uno::Sequence< css::uno::Any > InitAnyPropertySequence(
        ::std::initializer_list< ::std::pair< OUString, css::uno::Any > > vInit)
    {
        css::uno::Sequence<css::uno::Any> vResult{static_cast<sal_Int32>(vInit.size())};
        std::transform(vInit.begin(), vInit.end(), vResult.getArray(),
                       [](const std::pair<OUString, css::uno::Any>& rInit) {
                           return css::uno::Any(
                               css::beans::PropertyValue(rInit.first, -1, rInit.second,
                                                         css::beans::PropertyState_DIRECT_VALUE));
                       });
        return vResult;
    }

    COMPHELPER_DLLPUBLIC std::vector<css::beans::PropertyValue> JsonToPropertyValues(std::string_view rJson);
}   // namespace comphelper


#endif // INCLUDED_COMPHELPER_PROPERTYSEQUENCE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
