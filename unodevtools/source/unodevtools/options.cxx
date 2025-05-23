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

#include <codemaker/global.hxx>
#include <o3tl/safeint.hxx>
#include <o3tl/string_view.hxx>
#include <rtl/ustring.hxx>
#include <rtl/process.h>
#include <sal/log.hxx>
#include <options.hxx>

namespace unodevtools {


bool readOption( OUString * pValue, const char * pOpt,
                     sal_uInt32 * pnIndex, std::u16string_view aArg)
{
    static constexpr OUString dash = u"-"_ustr;
    if(aArg.find(dash) != 0)
        return false;

    OUString aOpt = OUString::createFromAscii( pOpt );

    if (aArg.size() < o3tl::make_unsigned(aOpt.getLength()))
        return false;

    if (aOpt.equalsIgnoreAsciiCase( aArg.substr(1) )) {
        // take next argument
        ++(*pnIndex);

        rtl_getAppCommandArg(*pnIndex, &pValue->pData);
        if (*pnIndex >= rtl_getAppCommandArgCount() ||
            pValue->subView(1) == dash)
        {
            throw CannotDumpException(
                "incomplete option \"-" + aOpt + "\" given!");
        }
        SAL_INFO("unodevtools", "identified option -" << pOpt << " = " << *pValue);
        ++(*pnIndex);
        return true;
    } else if (aArg.find(aOpt) == 1) {
        *pValue = aArg.substr(1 + aOpt.getLength());
        SAL_INFO("unodevtools", "identified option -" << pOpt << " = " << *pValue);
        ++(*pnIndex);

        return true;
    }
    return false;
}


bool readOption( const char * pOpt,
                     sal_uInt32 * pnIndex, std::u16string_view aArg)
{
    OUString aOpt = OUString::createFromAscii(pOpt);

    if((o3tl::starts_with(aArg, u"-") && aOpt.equalsIgnoreAsciiCase(aArg.substr(1))) ||
       (o3tl::starts_with(aArg, u"--") && aOpt.equalsIgnoreAsciiCase(aArg.substr(2))) )
    {
        ++(*pnIndex);
        SAL_INFO("unodevtools", "identified option --" << pOpt);
        return true;
    }
    return false;
}

} // end of namespace unodevtools

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
