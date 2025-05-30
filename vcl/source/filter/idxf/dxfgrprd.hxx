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

#include <rtl/string.hxx>
#include <sal/types.h>
#include <tools/long.hxx>

class SvStream;

class DXFGroupReader
{
public:
    explicit DXFGroupReader( SvStream & rIStream );

    bool GetStatus() const;

    void SetError();

    sal_uInt16 Read();
        // Reads next group and returns the group code.
        // In case of an error GetStatus() returns sal_False, group code will be set
        // to 0 and SetS(0,"EOF") will be executed.
    bool Read(sal_uInt16 nExpectedG) { return Read() == nExpectedG; }

    sal_uInt16 GetG() const;
        // Return the last group code (the one the last Read() did return).

    tools::Long   GetI() const;
        // Returns the integer value of the group which was read earlier with Read().
        // This read must have returned a group code for datatype Integer.
        // If not 0 is returned

    double GetF() const;
        // Returns the floating point value of the group which was read earlier with Read().
        // This read must have returned a group code for datatype Floatingpoint.
        // If not 0 is returned

    const OString& GetS() const;
        // Returns the string of the group which was read earlier with Read().
        // This read must have returned a group code for datatype String.
        // If not NULL is returned

    sal_uInt64 remainingSize() const;
private:

    tools::Long   ReadI();
    double ReadF();
    void   ReadS();

    SvStream & rIS;
    bool bStatus;
    sal_uInt16 nLastG;

    OString S;
    union {
        double F;
        tools::Long I;
    };
};


inline bool DXFGroupReader::GetStatus() const
{
    return bStatus;
}


inline void DXFGroupReader::SetError()
{
    bStatus=false;
}

inline sal_uInt16 DXFGroupReader::GetG() const
{
    return nLastG;
}

inline tools::Long DXFGroupReader::GetI() const
{
    return I;
}

inline double DXFGroupReader::GetF() const
{
    return F;
}

inline const OString& DXFGroupReader::GetS() const
{
    return S;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
