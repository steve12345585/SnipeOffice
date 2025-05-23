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

#include <msvc/except.hxx>

#pragma pack(push, 8)

struct ExceptionType final
{
    sal_Int32 _n0; // flags
    sal_uInt32 _pTypeInfo; // typeinfo
    sal_Int32 _n1, _n2, _n3; // thiscast
    sal_Int32 _n4; // object_size
    sal_uInt32 _pCopyCtor; // copyctor
    ExceptionTypeInfo exc_type_info;

    explicit ExceptionType(unsigned char* pCode, sal_uInt64 pCodeBase,
                           typelib_TypeDescription* pTD) noexcept;

    ExceptionType(const ExceptionType&) = delete;
    ExceptionType& operator=(const ExceptionType&) = delete;
};

struct RaiseInfo final
{
    sal_Int32 _n0;
    sal_uInt32 _pDtor;
    sal_Int32 _n2;
    sal_uInt32 _types;

    // Additional fields
    typelib_TypeDescription* _pTD;
    unsigned char* _code;
    sal_uInt64 _codeBase;

    explicit RaiseInfo(typelib_TypeDescription* pTD) noexcept;
};

#pragma pack(pop)

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
