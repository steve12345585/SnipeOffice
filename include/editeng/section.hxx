/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_EDITENG_SECTION_HXX
#define INCLUDED_EDITENG_SECTION_HXX

#include <sal/types.h>

#include <vector>

class SfxPoolItem;

namespace editeng
{
struct Section
{
    sal_Int32 mnParagraph;
    sal_Int32 mnStart;
    sal_Int32 mnEnd;

    std::vector<const SfxPoolItem*> maAttributes;

    Section(sal_Int32 nPara, sal_Int32 nStart, sal_Int32 nEnd);
};
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
