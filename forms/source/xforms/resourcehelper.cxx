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


#include "resourcehelper.hxx"
#include <frm_resource.hxx>

#include <osl/diagnose.h>
#include <rtl/ustring.hxx>


namespace xforms
{

OUString getResource(TranslateId pResourceId)
{
    return getResource(pResourceId, {}, {}, {});
}

OUString getResource(TranslateId pResourceId,
                     std::u16string_view rInfo1)
{
    return getResource(pResourceId, rInfo1, {}, {});
}

OUString getResource(TranslateId pResourceId,
                     std::u16string_view rInfo1,
                     std::u16string_view rInfo2)
{
    return getResource(pResourceId, rInfo1, rInfo2, {});
}

OUString getResource(TranslateId pResourceId,
                     std::u16string_view rInfo1,
                     std::u16string_view rInfo2,
                     std::u16string_view rInfo3)
{
    OUString sResource = frm::ResourceManager::loadString(pResourceId);
    OSL_ENSURE( !sResource.isEmpty(), "resource not found?" );

    OUString sString = sResource.replaceAll( "$1", rInfo1 );
    sString = sString.replaceAll( "$2", rInfo2 );
    sString = sString.replaceAll( "$3", rInfo3 );
    return sString;
}

}   // namespace xforms

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
