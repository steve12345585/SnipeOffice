/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SFX2_SOURCE_INC_OPENURIEXTERNALLY_HXX
#define INCLUDED_SFX2_SOURCE_INC_OPENURIEXTERNALLY_HXX

#include <sal/config.h>

#include <rtl/ustring.hxx>

namespace weld
{
class Widget;
}

namespace sfx2
{
/** Open a URI via com.sun.star.system.SystemShellExecute

    Handles XSystemShellExecute.execute's IllegalArgumentException (throwing a
    RuntimeException if it is unexpected, i.e., not caused by the given sURI not
    being an absolute URI reference).

    Handles XSystemShellExecute.execute's SystemShellExecuteException unless the
    given bHandleSystemShellExecuteException is false (in which case the
    exception is re-thrown).
*/
void openUriExternally(const OUString& sURI, bool bHandleSystemShellExecuteException,
                       weld::Widget* pDialogParent);
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
