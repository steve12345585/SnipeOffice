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

#include <basegfx/range/basicrange.hxx>


namespace basegfx
{
    /** Explicitly different from BasicRange, handling the inside predicates
        differently.

        This is modelled after how polygon fill algorithms set pixel -
        typically excluding rightmost and bottommost ones.
     */
    class BasicBox : public BasicRange< sal_Int32, Int32Traits >
    {
        typedef BasicRange< sal_Int32, Int32Traits > Base;
    public:
        BasicBox() {}

        explicit BasicBox( sal_Int32 nValue ) :
            Base( nValue )
        {
        }

        bool isEmpty() const
        {
            return mnMinimum >= mnMaximum;
        }

        using Base::isInside;

        bool isInside(sal_Int32 nValue) const
        {
            if(isEmpty())
            {
                return false;
            }
            else
            {
                return (nValue >= mnMinimum) && (nValue < mnMaximum);
            }
        }

        using Base::overlaps;
    };

} // end of namespace basegfx

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
