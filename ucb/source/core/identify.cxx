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

/**************************************************************************
                                TODO
 **************************************************************************

 *************************************************************************/

#include <utility>

#include "identify.hxx"

using namespace com::sun::star::lang;
using namespace com::sun::star::ucb;

// ContentIdentifier Implementation.
ContentIdentifier::ContentIdentifier(OUString ContentId)
    : m_aContentId(std::move(ContentId))
{
}

// virtual
ContentIdentifier::~ContentIdentifier() {}

// XContentIdentifier methods.
// virtual
OUString SAL_CALL ContentIdentifier::getContentIdentifier() { return m_aContentId; }

// virtual
OUString SAL_CALL ContentIdentifier::getContentProviderScheme()
{
    if (m_aProviderScheme.isEmpty() && !m_aContentId.isEmpty())
    {
        // The content provider scheme is the part before the first ':'
        // within the content id.
        sal_Int32 nPos = m_aContentId.indexOf(':');
        if (nPos != -1)
        {
            OUString aScheme(m_aContentId.copy(0, nPos));
            m_aProviderScheme = aScheme.toAsciiLowerCase();
        }
    }

    return m_aProviderScheme;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
