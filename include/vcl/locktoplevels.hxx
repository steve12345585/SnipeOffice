/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vcl/dllapi.h>
#include <memory>

namespace weld
{
class Widget;
}

struct TopLevelWindowLockerImpl;

class VCL_DLLPUBLIC TopLevelWindowLocker
{
private:
    std::unique_ptr<TopLevelWindowLockerImpl> m_xImpl;

public:
    TopLevelWindowLocker();
    ~TopLevelWindowLocker();

    // lock all toplevels, except the argument
    void incBusy(const weld::Widget* pIgnore);
    // unlock previous lock
    void decBusy();
    bool isBusy() const;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
