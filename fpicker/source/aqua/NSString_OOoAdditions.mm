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

#include <sal/config.h>

#include <memory>

#include "NSString_OOoAdditions.hxx"

@implementation NSString (OOoAdditions) 

+ (id) stringWithOUString:(const OUString&)ouString
{
    NSString *string = [[NSString alloc] initWithCharacters:reinterpret_cast<unichar const *>(ouString.getStr()) length:ouString.getLength()];

    return [string autorelease];
}

- (OUString) OUString
{
    unsigned int nFileNameLength = [self length];

    auto const unichars = std::make_unique<UniChar[]>(nFileNameLength+1);

    //'close' the string buffer correctly
    unichars[nFileNameLength] = '\0';

    [self getCharacters:unichars.get()];

    return OUString(reinterpret_cast<sal_Unicode *>(unichars.get()));
}

@end

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
