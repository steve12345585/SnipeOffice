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

#include "accessiblelistbox.hxx"

class AccessibleIconView final : public AccessibleListBox
{
public:
    AccessibleIconView(SvTreeListBox& _rListBox,
                       const css::uno::Reference<css::accessibility::XAccessible>& _xParent);

protected:
    // VCLXAccessibleComponent
    virtual void ProcessWindowEvent(const VclWindowEvent& rVclWindowEvent) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
