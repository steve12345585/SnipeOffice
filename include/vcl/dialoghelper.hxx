/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <vcl/dllapi.h>

namespace vcl
{
class Window;

/* cancel dialogs that are a child of pParent
   this is used by com.sun.star.embed.DocumentCloser which itself is only used by
   extensions/source/activex/SOActiveX.cxx see extensions/source/activex/README.txt
   possibly dubious if this actually works as expected
*/
VCL_DLLPUBLIC void EndAllDialogs(vcl::Window const* pParent);

/* returns true if a vcl PopupMenu is executing. Uses of this outside of vcl/toolkit
   are possibly dubious.
*/
VCL_DLLPUBLIC bool IsInPopupMenuExecute();

/* for SnipeOffice kit */
VCL_DLLPUBLIC void EnableDialogInput(vcl::Window* pDialog);
VCL_DLLPUBLIC void CloseTopLevel(vcl::Window* pDialog);
/// Pre-loads all modules containing UI information
VCL_DLLPUBLIC void VclBuilderPreload();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
