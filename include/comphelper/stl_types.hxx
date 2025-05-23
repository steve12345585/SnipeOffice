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
#ifndef INCLUDED_COMPHELPER_STL_TYPES_HXX
#define INCLUDED_COMPHELPER_STL_TYPES_HXX

#include <sal/config.h>

#include <algorithm>
#include <memory>
#include <string_view>

#include <rtl/ustrbuf.hxx>
#include <o3tl/string_view.hxx>

namespace comphelper
{

// comparison functors

struct UStringMixLess
{
private:
    bool m_bCaseSensitive;
public:
    explicit UStringMixLess(bool bCaseSensitive = true):m_bCaseSensitive(bCaseSensitive){}
    bool operator() (std::u16string_view x, std::u16string_view y) const
    {
        if (m_bCaseSensitive)
            return x < y;
        else
            return o3tl::compareToIgnoreAsciiCase(x, y) < 0;
    }

    bool isCaseSensitive() const {return m_bCaseSensitive;}
};

class UStringMixEqual
{
    bool const m_bCaseSensitive;

public:
    explicit UStringMixEqual(bool bCaseSensitive = true):m_bCaseSensitive(bCaseSensitive){}
    bool operator() (std::u16string_view lhs, std::u16string_view rhs) const
    {
        return m_bCaseSensitive ? lhs == rhs : o3tl::equalsIgnoreAsciiCase( lhs, rhs );
    }
    bool isCaseSensitive() const {return m_bCaseSensitive;}
};

/// by-value less functor for std::set<std::unique_ptr<T>>
template<class T> struct UniquePtrValueLess
{
        bool operator()(std::unique_ptr<T> const& lhs,
                        std::unique_ptr<T> const& rhs) const
        {
            assert(lhs.get());
            assert(rhs.get());
            return (*lhs) < (*rhs);
        }
        // The following are so we can search in std::set without allocating a temporary entry on the heap
        typedef bool is_transparent;
        bool operator()(T const& lhs,
                        std::unique_ptr<T> const& rhs) const
        {
            assert(rhs.get());
            return lhs < (*rhs);
        }
        bool operator()(std::unique_ptr<T> const& lhs,
                        T const& rhs) const
        {
            assert(lhs.get());
            return (*lhs) < rhs;
        }
};

/// by-value implementation of std::foo<std::unique_ptr<T>>::operator==
template<template<typename, typename...> class C, typename T, typename... Etc>
bool ContainerUniquePtrEquals(
        C<std::unique_ptr<T>, Etc...> const& lhs,
        C<std::unique_ptr<T>, Etc...> const& rhs)
{
    return lhs.size() == rhs.size()
           && std::equal(lhs.begin(), lhs.end(), rhs.begin(),
                         [](const auto& p1, const auto& p2) { return *p1 == *p2; });
};


template <class Tp, class Arg>
class mem_fun1_t
{
    typedef void (Tp::*_fun_type)(Arg);
public:
    explicit mem_fun1_t(_fun_type pf) : M_f(pf) {}
    void operator()(Tp* p, Arg x) const { (p->*M_f)(x); }
private:
    _fun_type const M_f;
};

template <class Tp, class Arg>
inline mem_fun1_t<Tp,Arg> mem_fun(void (Tp::*f)(Arg))
{
    return mem_fun1_t<Tp,Arg>(f);
}

/** output iterator that appends OUStrings into an OUStringBuffer.
 */
class OUStringBufferAppender
{
public:
    typedef OUStringBufferAppender Self;
    typedef ::std::output_iterator_tag iterator_category;
    typedef void value_type;
    typedef void reference;
    typedef void pointer;
    typedef size_t difference_type;

    OUStringBufferAppender(OUStringBuffer & i_rBuffer)
        : m_rBuffer(&i_rBuffer) { }
    Self & operator=(std::u16string_view i_rStr)
    {
        m_rBuffer->append( i_rStr );
        return *this;
    }
    Self & operator*() { return *this; } // so operator= works
    Self & operator++() { return *this; }

private:
    OUStringBuffer * m_rBuffer;
};

/** algorithm similar to std::copy, but inserts a separator between elements.
 */
template< typename ForwardIter, typename OutputIter, typename T >
OutputIter intersperse(
    ForwardIter start, ForwardIter end, OutputIter out, T const & separator)
{
    if (start != end) {
        *out = *start;
        ++start;
        ++out;
    }

    while (start != end) {
        *out = separator;
        ++out;
        *out = *start;
        ++start;
        ++out;
    }

    return out;
}

}

#endif // INCLUDED_COMPHELPER_STL_TYPES_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
