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
#include <editeng/editengdllapi.h>
#include <editeng/outliner.hxx>
#include <editeng/editview.hxx>

class EDITENG_DLLPUBLIC URLFieldHelper
{
public:
    static void RemoveURLField(EditView& pEditView);
    static bool IsCursorAtURLField(const EditView& pEditView, bool bAlsoCheckBeforeCursor = false);
    static bool IsCursorAtURLField(const OutlinerView* pOLV, bool bAlsoCheckBeforeCursor = false)
    {
        return pOLV && IsCursorAtURLField(pOLV->GetEditView(), bAlsoCheckBeforeCursor);
    }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
