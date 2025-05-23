/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <rtl/ustring.hxx>

// no need to translate this
// the items in this string have to be in the same order as the STR_FIELD_* strings are added to the
// field label list of the dialog
inline constexpr OUString STR_LOGICAL_FIELD_NAMES = u"FirstName;LastName;Company;Department;Street;Zip;City;State;Country;PhonePriv;PhoneComp;PhoneOffice;PhoneCell;PhoneOther;Pager;Fax;EMail;URL;Title;Position;Code;AddrForm;AddrFormMail;Id;CalendarURL;InviteParticipant;Note;Altfield1;Altfield2;Altfield3;Altfield4"_ustr;

#define STR_DESCRIPTION_SMATH_DOC       "StarMath 2.0 - 5.0"
#define STR_DESCRIPTION_SCHART_DOC      "StarChart 3.0 - 5.0"
#define STR_DESCRIPTION_SDRAW_DOC       "StarDraw 3.0 / 5.0 (StarImpress)"
#define STR_DESCRIPTION_SCALC_DOC       "StarCalc 3.0 - 5.0"
#define STR_DESCRIPTION_SIMPRESS_DOC    "StarImpress 4.0 / 5.0"
#define STR_DESCRIPTION_SWRITER_DOC     "StarWriter 3.0 - 5.0"


/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
