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

#include <dpitemdata.hxx>
#include <global.hxx>

#include <unotools/collatorwrapper.hxx>
#include <unotools/transliterationwrapper.hxx>
#include <rtl/math.hxx>

const sal_Int32 ScDPItemData::DateFirst = -1;
const sal_Int32 ScDPItemData::DateLast  = 10000;

sal_Int32 ScDPItemData::Compare(const ScDPItemData& rA, const ScDPItemData& rB)
{
    if (rA.meType != rB.meType)
    {
        // group value, value and string in this order. Ensure that the empty
        // type comes last.
        return rA.meType < rB.meType ? -1 : 1;
    }

    switch (rA.meType)
    {
        case GroupValue:
        {
            if (rA.maGroupValue.mnGroupType == rB.maGroupValue.mnGroupType)
            {
                if (rA.maGroupValue.mnValue == rB.maGroupValue.mnValue)
                    return 0;

                return rA.maGroupValue.mnValue < rB.maGroupValue.mnValue ? -1 : 1;
            }

            return rA.maGroupValue.mnGroupType < rB.maGroupValue.mnGroupType ? -1 : 1;
        }
        case Value:
        case RangeStart:
        {
            if (rA.mfValue == rB.mfValue)
                return 0;

            return rA.mfValue < rB.mfValue ? -1 : 1;
        }
        case String:
        case Error:
            if (rA.mpString == rB.mpString)
                // strings may be interned.
                return 0;

            return ScGlobal::GetCollator().compareString(rA.GetString(), rB.GetString());
        default:
            ;
    }
    return 0;
}

ScDPItemData::ScDPItemData() :
    mfValue(0.0), meType(Empty), mbStringInterned(false) {}

ScDPItemData::ScDPItemData(const ScDPItemData& r) :
    meType(r.meType), mbStringInterned(r.mbStringInterned)
{
    switch (r.meType)
    {
        case String:
        case Error:
            mpString = r.mpString;
            if (!mbStringInterned)
                rtl_uString_acquire(mpString);
        break;
        case Value:
        case RangeStart:
            mfValue = r.mfValue;
        break;
        case GroupValue:
            maGroupValue.mnGroupType = r.maGroupValue.mnGroupType;
            maGroupValue.mnValue = r.maGroupValue.mnValue;
        break;
        case Empty:
        default:
            mfValue = 0.0;
    }
}

void ScDPItemData::DisposeString()
{
    if (!mbStringInterned)
    {
        if (meType == String || meType == Error)
            rtl_uString_release(mpString);
    }

    mbStringInterned = false;
}

ScDPItemData::ScDPItemData(const OUString& rStr) :
    mpString(rStr.pData), meType(String), mbStringInterned(false)
{
    rtl_uString_acquire(mpString);
}

ScDPItemData::ScDPItemData(sal_Int32 nGroupType, sal_Int32 nValue) :
    meType(GroupValue), mbStringInterned(false)
{
    maGroupValue.mnGroupType = nGroupType;
    maGroupValue.mnValue = nValue;
}

ScDPItemData::~ScDPItemData()
{
    DisposeString();
}

void ScDPItemData::SetEmpty()
{
    DisposeString();
    meType = Empty;
}

void ScDPItemData::SetString(const OUString& rS)
{
    DisposeString();
    mpString = rS.pData;
    rtl_uString_acquire(mpString);
    meType = String;
}

void ScDPItemData::SetStringInterned( rtl_uString* pS )
{
    DisposeString();
    mpString = pS;
    meType = String;
    mbStringInterned = true;
}

void ScDPItemData::SetValue(double fVal)
{
    DisposeString();
    mfValue = fVal;
    meType = Value;
}

void ScDPItemData::SetRangeStart(double fVal)
{
    DisposeString();
    mfValue = fVal;
    meType = RangeStart;
}

void ScDPItemData::SetRangeFirst()
{
    DisposeString();
    mfValue = -std::numeric_limits<double>::infinity();
    meType = RangeStart;
}

void ScDPItemData::SetRangeLast()
{
    DisposeString();
    mfValue = std::numeric_limits<double>::infinity();
    meType = RangeStart;
}

void ScDPItemData::SetErrorStringInterned( rtl_uString* pS )
{
    SetStringInterned(pS);
    meType = Error;
}

bool ScDPItemData::IsCaseInsEqual(const ScDPItemData& r) const
{
    if (meType != r.meType)
        return false;

    switch (meType)
    {
        case Value:
        case RangeStart:
            return rtl::math::approxEqual(mfValue, r.mfValue);
        case GroupValue:
            return maGroupValue.mnGroupType == r.maGroupValue.mnGroupType &&
                maGroupValue.mnValue == r.maGroupValue.mnValue;
        default:
            ;
    }

    if (mpString == r.mpString)
        // Fast equality check for interned strings.
        return true;

    return ScGlobal::GetTransliteration().isEqual(GetString(), r.GetString());
}

bool ScDPItemData::operator== (const ScDPItemData& r) const
{
    if (meType != r.meType)
        return false;

    switch (meType)
    {
        case Value:
        case RangeStart:
            return rtl::math::approxEqual(mfValue, r.mfValue);
        case GroupValue:
            return maGroupValue.mnGroupType == r.maGroupValue.mnGroupType &&
                maGroupValue.mnValue == r.maGroupValue.mnValue;
        default:
            ;
    }

    // need exact equality until we have a safe case insensitive string hash
    return GetString() == r.GetString();
}

bool ScDPItemData::operator< (const ScDPItemData& r) const
{
    return Compare(*this, r) == -1;
}

ScDPItemData& ScDPItemData::operator= (const ScDPItemData& r)
{
    DisposeString();
    meType = r.meType;
    switch (r.meType)
    {
        case String:
        case Error:
            mbStringInterned = r.mbStringInterned;
            mpString = r.mpString;
            if (!mbStringInterned)
                rtl_uString_acquire(mpString);
        break;
        case Value:
        case RangeStart:
            mfValue = r.mfValue;
        break;
        case GroupValue:
            maGroupValue.mnGroupType = r.maGroupValue.mnGroupType;
            maGroupValue.mnValue = r.maGroupValue.mnValue;
        break;
        case Empty:
        default:
            mfValue = 0.0;
    }
    return *this;
}

ScDPValue::Type ScDPItemData::GetCellType() const
{
    switch (meType)
    {
        case Error:
            return ScDPValue::Error;
        case Empty:
            return ScDPValue::Empty;
        case Value:
            return ScDPValue::Value;
        default:
            ;
    }

    return ScDPValue::String;
}

#if DEBUG_PIVOT_TABLE

void ScDPItemData::Dump(const char* msg) const
{
    printf("--- (%s)\n", msg);
    switch (meType)
    {
        case Empty:
            printf("empty\n");
        break;
        case Error:
            printf("error: %s\n",
                   OUStringToOString(OUString(mpString), RTL_TEXTENCODING_UTF8).getStr());
        break;
        case GroupValue:
            printf("group value: group type = %d  value = %d\n",
                   maGroupValue.mnGroupType, maGroupValue.mnValue);
        break;
        case String:
            printf("string: %s\n",
                   OUStringToOString(OUString(mpString), RTL_TEXTENCODING_UTF8).getStr());
        break;
        case Value:
            printf("value: %g\n", mfValue);
        break;
        case RangeStart:
            printf("range start: %g\n", mfValue);
        break;
        default:
            printf("unknown type\n");
    }
    printf("---\n");
}
#endif

bool ScDPItemData::IsEmpty() const
{
    return meType == Empty;
}

bool ScDPItemData::IsValue() const
{
    return meType == Value;
}

OUString ScDPItemData::GetString() const
{
    switch (meType)
    {
        case String:
        case Error:
            return OUString(mpString);
        case Value:
        case RangeStart:
            return OUString::number(mfValue);
        case GroupValue:
            return OUString::number(maGroupValue.mnValue);
        case Empty:
        default:
            ;
    }

    return OUString();
}

double ScDPItemData::GetValue() const
{
    if (meType == Value || meType == RangeStart)
        return mfValue;

    return 0.0;
}

ScDPItemData::GroupValueAttr ScDPItemData::GetGroupValue() const
{
    if (meType == GroupValue)
        return maGroupValue;

    GroupValueAttr aGV;
    aGV.mnGroupType = -1;
    aGV.mnValue = -1;
    return aGV;
}

bool ScDPItemData::HasStringData() const
{
    return meType == String || meType == Error;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
