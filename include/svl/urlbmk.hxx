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

#ifndef INCLUDED_SVL_URLBMK_HXX
#define INCLUDED_SVL_URLBMK_HXX

#include <rtl/ustring.hxx>
#include <utility>


/**
 * This class represents a bookmark that consists of a URL and
 * a corresponding description text.
 *
 * There is an own clipboard format and helper methods to copy
 * to and from clipboard and DragServer.
 */
class INetBookmark
{
    OUString          aUrl;
    OUString          aDescr;

public:
                    INetBookmark( OUString _aUrl, OUString _aDescr )
                        : aUrl(std::move( _aUrl )), aDescr(std::move( _aDescr ))
                    {}
                    INetBookmark()
                    {}

    const OUString& GetURL() const          { return aUrl; }
    const OUString& GetDescription() const  { return aDescr; }
};


#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
