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

#include <sfx2/sidebar/Context.hxx>
#include <rtl/ustring.hxx>

#include <vector>

namespace sfx2::sidebar {

/** Per context data for deck and panel descriptors.
*/
class ContextList
{
public:
    ContextList();

    class Entry
    {
    public:
        Context maContext;
        bool mbIsInitiallyVisible;
        OUString msMenuCommand;
    };

    /** Return <TRUE/> when the given context matches any of the stored contexts.
    */
    const Entry* GetMatch (
        const Context& rContext) const;
    Entry* GetMatch (
        const Context& rContext);

    void AddContextDescription (
        const Context& rContext,
        const bool bIsInitiallyVisible,
        const OUString& rsMenuCommand);

    void ToggleVisibilityForContext( const Context& rContext,const bool bIsInitiallyVisible );
    const ::std::vector<Entry>& GetEntries() const {return maEntries;};

private:
    ::std::vector<Entry> maEntries;

    ::std::vector<Entry>::const_iterator FindBestMatch (const Context& rContext) const;
};

} // end of namespace sfx2::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
