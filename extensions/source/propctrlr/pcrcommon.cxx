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

#include "pcrcommon.hxx"

#include <com/sun/star/util/MeasureUnit.hpp>
#include <rtl/ustrbuf.hxx>
#include <tools/urlobj.hxx>


namespace pcr
{


    //= HelpIdUrl


    OUString HelpIdUrl::getHelpId( std::u16string_view _rHelpURL )
    {
        INetURLObject aHID( _rHelpURL );
        if ( aHID.GetProtocol() == INetProtocol::Hid )
            return aHID.GetURLPath();
        else
            return OUString(_rHelpURL);
    }


    OUString HelpIdUrl::getHelpURL( std::u16string_view sHelpId )
    {
        OUStringBuffer aBuffer;
        INetURLObject aHID( sHelpId );
        if ( aHID.GetProtocol() == INetProtocol::NotValid )
            aBuffer.append( INET_HID_SCHEME );
        aBuffer.append( sHelpId );
        return aBuffer.makeStringAndClear();
    }

} // namespace pcr


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
