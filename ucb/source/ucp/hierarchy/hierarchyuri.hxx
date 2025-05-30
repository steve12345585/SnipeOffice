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

namespace hierarchy_ucp {


class HierarchyUri
{
    mutable OUString m_aUri;
    mutable OUString m_aParentUri;
    mutable OUString m_aService;
    mutable OUString m_aPath;
    mutable bool            m_bValid;

private:
    void init() const;

public:
    explicit HierarchyUri( OUString aUri )
    : m_aUri( std::move(aUri) ), m_bValid( false ) {}

    bool isValid() const
    { init(); return m_bValid; }

    const OUString & getUri() const
    { init(); return m_aUri; }

    const OUString & getParentUri() const
    { init(); return m_aParentUri; }

    const OUString & getService() const
    { init(); return m_aService; }

    const OUString & getPath() const
    { init(); return m_aPath; }

    inline bool isRootFolder() const;
};

inline bool HierarchyUri::isRootFolder() const
{
    init();
    return m_aPath == "/";
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
