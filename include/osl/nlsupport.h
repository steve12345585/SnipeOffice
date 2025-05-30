/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

/*
 * This file is part of LibreOffice published API.
 */

#ifndef INCLUDED_OSL_NLSUPPORT_H
#define INCLUDED_OSL_NLSUPPORT_H

#include "sal/config.h"

#include "rtl/locale.h"
#include "rtl/textenc.h"
#include "sal/saldllapi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
    Determines the text encoding used by the underlying platform for the
    specified locale.

    @param pLocale
    the locale to return the text encoding for. If this parameter is NULL,
    the default locale of the current process is used.

    @returns the rtl_TextEncoding that matches the platform specific encoding
    description or RTL_TEXTENCODING_DONTKNOW if no mapping is available.
*/

SAL_DLLPUBLIC rtl_TextEncoding SAL_CALL osl_getTextEncodingFromLocale(
        rtl_Locale * pLocale );


#ifdef __cplusplus
}
#endif

#endif // INCLUDED_OSL_NLSUPPORT_H


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
