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
#pragma once

#include <rtl/ustring.hxx>
#include <sal/config.h>
#include <sfx2/dllapi.h>
#include <sfx2/shell.hxx>

class SfxViewFrame;
class SfxViewShell;

typedef SfxViewShell* (*SfxViewCtor)(SfxViewFrame&, SfxViewShell*);

// CLASS -----------------------------------------------------------------
class SFX2_DLLPUBLIC SfxViewFactory
{
public:
    SfxViewFactory( SfxViewCtor fnC,
                    SfxInterfaceId nOrdinal, const char* asciiViewName );

    SfxViewShell*  CreateInstance(SfxViewFrame& rViewFrame, SfxViewShell *pOldSh);
    SfxInterfaceId GetOrdinal() const { return nOrd; }

    /// returns a legacy view name. This is "view" with an appended ordinal/ID.
    OUString      GetLegacyViewName() const;

    /** returns an API-compatible view name.

        For details on which view names are specified, see the XModel2.getAvailableViewControllerNames
        documentation.
    */
    OUString      GetAPIViewName() const;

private:
    SfxViewCtor     fnCreate;
    SfxInterfaceId  nOrd;
    const OUString  m_sViewName;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
