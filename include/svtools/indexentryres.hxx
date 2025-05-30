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

#include <svtools/svtdllapi.h>
#include <rtl/ustring.hxx>
#include <utility>
#include <vector>

class SVT_DLLPUBLIC IndexEntryResource
{
private:
    //  wrapper for locale specific translations data of indexentry algorithm
    class IndexEntryResourceData
    {
        friend class IndexEntryResource;

    private:
        OUString m_aName;
        OUString m_aTranslation;

    public:
        IndexEntryResourceData(OUString aAlgorithm, OUString aTranslation)
            : m_aName(std::move(aAlgorithm))
            , m_aTranslation(std::move(aTranslation))
        {
        }
        const OUString& GetAlgorithm() const { return m_aName; }
        const OUString& GetTranslation() const { return m_aTranslation; }
    };
    std::vector<IndexEntryResourceData> m_aData;

public:
    IndexEntryResource();
    const OUString& GetTranslation(const OUString& rAlgorithm);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
