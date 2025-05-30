/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SFX2_DIALOGHELPER_HXX
#define INCLUDED_SFX2_DIALOGHELPER_HXX

#include <sfx2/dllapi.h>
#include <rtl/ustring.hxx>
#include <tools/gen.hxx>

class DateTime;
class LocaleDataWrapper;
class OutputDevice;

//when two tab pages both have the same basic layout with a preview on the
//right, get both of their non-preview areas to request the same size so that
//the preview appears in the same place in each one so flipping between tabs
//isn't distracting as it jumps around

Size SFX2_DLLPUBLIC getParagraphPreviewOptimalSize(const OutputDevice& rReference);

Size SFX2_DLLPUBLIC getDrawPreviewOptimalSize(const OutputDevice& rReference);

Size SFX2_DLLPUBLIC getPreviewStripSize(const OutputDevice& rReference);

Size SFX2_DLLPUBLIC getPreviewOptionsSize(const OutputDevice& rReference);

OUString SFX2_DLLPUBLIC getWidestDateTime(const LocaleDataWrapper& rWrapper, bool bWithSec);

OUString SFX2_DLLPUBLIC formatDateTime(const DateTime& rDateTime, const LocaleDataWrapper& rWrapper,
                                       bool bWithSec);

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
