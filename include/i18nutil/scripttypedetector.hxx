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
#ifndef INCLUDED_I18NUTIL_SCRIPTTYPEDETECTOR_HXX
#define INCLUDED_I18NUTIL_SCRIPTTYPEDETECTOR_HXX

#include <i18nutil/i18nutildllapi.h>

#include <string_view>

class I18NUTIL_DLLPUBLIC ScriptTypeDetector
{
public:
    static sal_Int32 beginOfScriptDirection( std::u16string_view Text, sal_Int32 nPos, sal_Int16 scriptDirection );
    static sal_Int32 endOfScriptDirection( std::u16string_view Text, sal_Int32 nPos, sal_Int16 scriptDirection );
    static sal_Int16 getScriptDirection( std::u16string_view Text, sal_Int32 nPos, sal_Int16 defaultScriptDirection );
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
