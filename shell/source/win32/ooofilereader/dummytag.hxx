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


#ifndef INCLUDED_SHELL_SOURCE_WIN32_OOOFILEREADER_DUMMYTAG_HXX
#define INCLUDED_SHELL_SOURCE_WIN32_OOOFILEREADER_DUMMYTAG_HXX

#include "itag.hxx"

/***************************   dummy tag readers   ***************************/

/** Implements the ITag interface but does
    nothing (Null object pattern), may be used for
    tags we are not interested in to avoid if-else
    branches.
*/
class CDummyTag : public ITag
{
    public:
        virtual void startTag() override {};

        virtual void endTag() override {};

        virtual void addCharacters(const std::wstring& /*characters*/) override {};

        virtual void addAttributes(const XmlTagAttributes_t& /*attributes*/) override {};

        virtual std::wstring getTagContent() override
        {
            return EMPTY_STRING;
        };

        virtual ::std::wstring getTagAttribute( ::std::wstring  const & /*attrname*/ ) override { return ::std::wstring(EMPTY_STRING); };
};


#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
