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

#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <dp_shared.hxx>
#include <strings.hrc>
#include <tools/config.hxx>
#include <lockfile.hxx>

namespace desktop {

bool Lockfile_execWarning( Lockfile const * that )
{
    // read information from lock
    OUString aLockname = that->m_aLockname;
    Config aConfig(aLockname);
    aConfig.SetGroup( LOCKFILE_GROUP ""_ostr );
    OString aHost  = aConfig.ReadKey( LOCKFILE_HOSTKEY ""_ostr );
    OString aUser  = aConfig.ReadKey( LOCKFILE_USERKEY ""_ostr );
    OString aTime  = aConfig.ReadKey( LOCKFILE_TIMEKEY ""_ostr );

    // display warning and return response
    std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(nullptr,
                                              VclMessageType::Question, VclButtonsType::YesNo, DpResId(STR_QUERY_USERDATALOCKED)));
    // set box title
    OUString aTitle = DpResId(STR_TITLE_USERDATALOCKED);
    xBox->set_title( aTitle );
    // insert values...
    OUString aMsgText = xBox->get_primary_text();
    aMsgText = aMsgText.replaceFirst(
        "$u", OStringToOUString( aUser, RTL_TEXTENCODING_ASCII_US) );
    aMsgText = aMsgText.replaceFirst(
        "$h", OStringToOUString( aHost, RTL_TEXTENCODING_ASCII_US) );
    aMsgText = aMsgText.replaceFirst(
        "$t", OStringToOUString( aTime, RTL_TEXTENCODING_ASCII_US) );
    xBox->set_primary_text(aMsgText);
    // do it
    return xBox->run() == RET_YES;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
