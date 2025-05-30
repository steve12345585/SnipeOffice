/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/config.h>

#include <memory>

#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/uno/Sequence.hxx>

#include <svl/itemset.hxx>

#include "istyleaccess.hxx"
#include "swatrset.hxx"

class SwDoc;

std::shared_ptr<SfxItemSet>
PropValuesToAutoStyleItemSet(SwDoc& rDoc, IStyleAccess::SwAutoStyleFamily eFamily,
                             const css::uno::Sequence<css::beans::PropertyValue>& Values,
                             SwAttrSet& rSet);

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
