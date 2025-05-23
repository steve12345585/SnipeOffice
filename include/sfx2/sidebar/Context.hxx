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

#include <sfx2/dllapi.h>

namespace sfx2::sidebar
{
class SFX2_DLLPUBLIC Context
{
public:
    OUString msApplication;
    OUString msContext;

    Context();
    Context(OUString sApplication, OUString sContext);

    /** When two contexts are matched against each other, then
        application or context name may have the wildcard value 'any'.
        In order to prefer matches without wildcards over matches with
        wildcards we introduce an integer evaluation for matches.
    */
    const static sal_Int32 NoMatch;
    const static sal_Int32 OptimalMatch;
    const static sal_Int32 ApplicationWildcardMatch;
    const static sal_Int32 ContextWildcardMatch;

    /** Return the numeric value that describes how good the match
        between two contexts is.
        Smaller values represent better matches.
    */
    sal_Int32 EvaluateMatch(const Context& rOther) const;

    bool operator==(const Context& rOther) const;
    bool operator!=(const Context& rOther) const;
};

} // end of namespace sfx2::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
