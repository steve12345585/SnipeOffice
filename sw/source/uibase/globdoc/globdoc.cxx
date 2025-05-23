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

#include <comphelper/fileformat.h>
#include <comphelper/classids.hxx>
#include <osl/diagnose.h>
#include <tools/globname.hxx>

#include <swtypes.hxx>
#include <globdoc.hxx>
#include <strings.hrc>

// Description: Register all filters


SFX_IMPL_OBJECTFACTORY( SwGlobalDocShell, SvGlobalName(SO3_SWGLOB_CLASSID), u"swriter/GlobalDocument"_ustr )

SwGlobalDocShell::SwGlobalDocShell(SfxObjectCreateMode eMode ) :
        SwDocShell(eMode)
{
}

SwGlobalDocShell::~SwGlobalDocShell()
{
}

void SwGlobalDocShell::FillClass( SvGlobalName * pClassName,
                                   SotClipboardFormatId * pClipFormat,
                                   OUString * pLongUserName,
                                   sal_Int32 nVersion,
                                   bool bTemplate /* = false */) const
{
    if (nVersion == SOFFICE_FILEFORMAT_60)
    {
        *pClassName = SvGlobalName( SO3_SWGLOB_CLASSID_60 );
        *pClipFormat = SotClipboardFormatId::STARWRITERGLOB_60;
        *pLongUserName = SwResId(STR_WRITER_GLOBALDOC_FULLTYPE);
        OSL_ENSURE( !bTemplate, "No template for Writer Global" );
    }
    else if (nVersion == SOFFICE_FILEFORMAT_8)
    {
        *pClassName     = SvGlobalName( SO3_SWGLOB_CLASSID_60 );
        *pClipFormat    = bTemplate ? SotClipboardFormatId::STARWRITERGLOB_8_TEMPLATE : SotClipboardFormatId::STARWRITERGLOB_8;
        *pLongUserName = SwResId(STR_WRITER_GLOBALDOC_FULLTYPE);
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
