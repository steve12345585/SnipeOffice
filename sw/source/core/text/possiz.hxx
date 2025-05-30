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

#include <tools/gen.hxx>
#include <swtypes.hxx>

// Compared to the SV sizes SwPositiveSize is always positive
class SwPositiveSize
{
    SwTwips m_nWidth;
    SwTwips m_nHeight;
public:
    SwPositiveSize( const SwTwips nW = 0, const SwTwips nH = 0 )
        : m_nWidth(nW)
        , m_nHeight(nH)
    {
    }
    explicit SwPositiveSize( const Size &rSize )
        : m_nWidth(SwTwips(rSize.Width()))
        , m_nHeight(SwTwips(rSize.Height()))
    {
    }
    virtual ~SwPositiveSize() {}
    SwPositiveSize(SwPositiveSize const &) = default;
    SwPositiveSize(SwPositiveSize &&) = default;
    SwPositiveSize & operator =(SwPositiveSize const &) = default;
    SwPositiveSize & operator =(SwPositiveSize &&) = default;
    SwTwips Height() const { return m_nHeight; }
    virtual void Height(const SwTwips nNew, const bool = true) { m_nHeight = nNew; }
    SwTwips Width() const { return m_nWidth; }
    void Width( const SwTwips nNew ) { m_nWidth = nNew; }
    Size SvLSize() const { return Size( m_nWidth, m_nHeight ); }
    void SvLSize( const Size &rSize )
    {
        m_nWidth  = SwTwips(rSize.Width());
        m_nHeight = SwTwips(rSize.Height());
    }
    void SvXSize( const Size &rSize )
    {
        m_nHeight = SwTwips(rSize.Width());
        m_nWidth = SwTwips(rSize.Height());
    }
    SwPositiveSize& operator=( const Size &rSize )
    {
        m_nWidth  = SwTwips(rSize.Width());
        m_nHeight = SwTwips(rSize.Height());
        return *this;
    }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
