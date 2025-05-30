/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SVL_SHAREDSTRING_HXX
#define INCLUDED_SVL_SHAREDSTRING_HXX

#include <svl/svldllapi.h>
#include <rtl/ustring.hxx>

#include <utility>

namespace svl {

class SVL_DLLPUBLIC SharedString
{
    rtl_uString* mpData = nullptr;
    rtl_uString* mpDataIgnoreCase = nullptr;
public:

    static const SharedString & getEmptyString();
    static const OUString EMPTY_STRING;

    SharedString() = default;
    SharedString( rtl_uString* pData, rtl_uString* pDataIgnoreCase );
    explicit SharedString( const OUString& rStr );
    SharedString( const SharedString& r );
    SharedString(SharedString&& r) noexcept;
    ~SharedString();

    SharedString& operator= ( const SharedString& r );
    SharedString& operator=(SharedString&& r) noexcept;

    bool operator== ( const SharedString& r ) const;
    bool operator!= ( const SharedString& r ) const;

    const OUString & getString() const;
    const OUString & getIgnoreCaseString() const;

    rtl_uString* getData();
    const rtl_uString* getData() const;

    rtl_uString* getDataIgnoreCase();
    const rtl_uString* getDataIgnoreCase() const;

    bool isValid() const;
    bool isEmpty() const;

    sal_Int32 getLength() const;
};

inline SharedString::SharedString( rtl_uString* pData, rtl_uString* pDataIgnoreCase ) :
    mpData(pData), mpDataIgnoreCase(pDataIgnoreCase)
{
    if (mpData)
        rtl_uString_acquire(mpData);
    if (mpDataIgnoreCase)
        rtl_uString_acquire(mpDataIgnoreCase);
}

inline SharedString::SharedString( const OUString& rStr ) : mpData(rStr.pData)
{
    rtl_uString_acquire(mpData);
}

inline SharedString::SharedString( const SharedString& r ) : mpData(r.mpData), mpDataIgnoreCase(r.mpDataIgnoreCase)
{
    if (mpData)
        rtl_uString_acquire(mpData);
    if (mpDataIgnoreCase)
        rtl_uString_acquire(mpDataIgnoreCase);
}

inline SharedString::SharedString(SharedString&& r) noexcept
    : mpData(std::exchange(r.mpData, nullptr))
    , mpDataIgnoreCase(std::exchange(r.mpDataIgnoreCase, nullptr))
{
}

inline SharedString::~SharedString()
{
    if (mpData)
        rtl_uString_release(mpData);
    if (mpDataIgnoreCase)
        rtl_uString_release(mpDataIgnoreCase);
}

inline SharedString& SharedString::operator=(SharedString&& r) noexcept
{
    // Having this inline helps Calc's mdds::multi_type_vector to do some operations
    // much faster.
    if (mpData)
        rtl_uString_release(mpData);
    if (mpDataIgnoreCase)
        rtl_uString_release(mpDataIgnoreCase);

    mpData = std::exchange(r.mpData, nullptr);
    mpDataIgnoreCase = std::exchange(r.mpDataIgnoreCase, nullptr);

    return *this;
}

inline bool SharedString::operator!= ( const SharedString& r ) const
{
    return !operator== (r);
}

inline const OUString & SharedString::getString() const
{
    return mpData ? OUString::unacquired(&mpData) : EMPTY_STRING;
}

inline const OUString & SharedString::getIgnoreCaseString() const
{
    return mpDataIgnoreCase ? OUString::unacquired(&mpDataIgnoreCase) : EMPTY_STRING;
}

inline rtl_uString* SharedString::getData()
{
    return mpData;
}

inline const rtl_uString* SharedString::getData() const
{
    return mpData;
}

inline rtl_uString* SharedString::getDataIgnoreCase()
{
    return mpDataIgnoreCase;
}

inline const rtl_uString* SharedString::getDataIgnoreCase() const
{
    return mpDataIgnoreCase;
}

inline bool SharedString::isValid() const
{
    return mpData != nullptr;
}

inline bool SharedString::isEmpty() const
{
    return mpData == nullptr || mpData->length == 0;
}

inline sal_Int32 SharedString::getLength() const
{
    return mpData ? mpData->length : 0;
}

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
