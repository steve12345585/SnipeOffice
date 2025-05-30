/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vcl/dllapi.h>

#define VCL_PRIORITY_DEFAULT -1

namespace vcl
{

class VCL_DLLPUBLIC SAL_LOPLUGIN_ANNOTATE("crosscast") IPrioritable
{
protected:
    IPrioritable() : m_nPriority(VCL_PRIORITY_DEFAULT)
    {
    }

public:
    virtual ~IPrioritable()
    {
    }

    int GetPriority() const
    {
        return m_nPriority;
    }

    void SetPriority(int nPriority)
    {
        m_nPriority = nPriority;
    }

    virtual void HideContent() = 0;
    virtual void ShowContent() = 0;

private:
    int m_nPriority;
};

} // namespace vcl

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
