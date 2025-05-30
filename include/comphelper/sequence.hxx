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

#ifndef INCLUDED_COMPHELPER_SEQUENCE_HXX
#define INCLUDED_COMPHELPER_SEQUENCE_HXX

#include <com/sun/star/uno/Sequence.hxx>
#include <osl/diagnose.h>

#include <vector>

namespace comphelper
{
    /** Search the given value within the given sequence, return the position of the first occurrence.
        Returns -1 if nothing found.
    */
    template <class T1, class T2>
    inline sal_Int32 findValue(const css::uno::Sequence<T1>& _rList, const T2& _rValue)
    {
        // at which position do I find the value?
        for (sal_Int32 i = 0; i < _rList.getLength(); ++i)
        {
            if (_rList[i] == _rValue)
                return i;
        }

        return -1;
    }

    /// concat several sequences
    template <class T, class... Ss>
    inline css::uno::Sequence<T> concatSequences(const css::uno::Sequence<T>& rS1, const Ss&... rSn)
    {
        // unary fold to disallow empty parameter pack: at least have one sequence in rSn
        css::uno::Sequence<T> aReturn(std::size(rS1) + (... + std::size(rSn)));
        T* pReturn = std::copy(std::begin(rS1), std::end(rS1), aReturn.getArray());
        (..., (pReturn = std::copy(std::begin(rSn), std::end(rSn), pReturn)));
        return aReturn;
    }

    /// concat additional elements from right sequence to left sequence
    ///
    /// be aware that this takes time O(|left| * |right|)
    template<typename T> inline css::uno::Sequence<T> combineSequences(
        css::uno::Sequence<T> const & left, css::uno::Sequence<T> const & right)
    {
        sal_Int32 n1 = left.getLength();
        css::uno::Sequence<T> ret(n1 + right.getLength());
            //TODO: check for overflow
        auto pRet = ret.getArray();
        std::copy_n(left.getConstArray(), n1, pRet);
        sal_Int32 n2 = n1;
        for (sal_Int32 i = 0; i != right.getLength(); ++i) {
            bool found = false;
            for (sal_Int32 j = 0; j != n1; ++j) {
                if (right[i] == left[j]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                pRet[n2++] = right[i];
            }
        }
        ret.realloc(n2);
        return ret;
    }

    /// remove a specified element from a sequences
    template<class T>
    inline void removeElementAt(css::uno::Sequence<T>& _rSeq, sal_Int32 _nPos)
    {
        sal_Int32 nLength = _rSeq.getLength();

        OSL_ENSURE(0 <= _nPos && _nPos < nLength, "invalid index");

        T* pPos = _rSeq.getArray() + _nPos;
        std::move(pPos + 1, pPos + nLength - _nPos, pPos);

        _rSeq.realloc(nLength-1);
    }

    /** Copy from a plain C/C++ array into a Sequence.

        @tpl SrcType
        Array element type. Must be assignable to DstType

        @tpl DstType
        Sequence element type. Must be assignable from SrcType

        @param i_pArray
        Valid pointer to at least num elements of type SrcType

        @param nNum
        Number of array elements to copy

        @return the resulting Sequence

        @attention when copying from e.g. a double array to a
        Sequence<int>, no proper rounding will be performed, but the
        values will be truncated. There's currently no measure to
        prevent or detect precision loss, overflow or truncation.
     */
    template < typename DstType, typename SrcType >
    inline css::uno::Sequence< DstType > arrayToSequence( const SrcType* i_pArray, sal_Int32 nNum )
    {
        if constexpr (std::is_same_v< DstType, SrcType >)
        {
            return css::uno::Sequence< DstType >( i_pArray, nNum );
        }
        else
        {
            css::uno::Sequence< DstType > result( nNum );
            ::std::copy( i_pArray, i_pArray+nNum, result.getArray() );
            return result;
        }
    }


    /** Copy from a Sequence into a plain C/C++ array

        @tpl SrcType
        Sequence element type. Must be assignable to DstType

        @tpl DstType
        Array element type. Must be assignable from SrcType

        @param io_pArray
        Valid pointer to at least i_Sequence.getLength() elements of
        type DstType

        @param i_Sequence
        Reference to a Sequence of SrcType elements

        @return a pointer to the array

        @attention when copying from e.g. a Sequence<double> to an int
        array, no proper rounding will be performed, but the values
        will be truncated. There's currently no measure to prevent or
        detect precision loss, overflow or truncation.
     */
    template < typename DstType, typename SrcType >
    inline DstType* sequenceToArray( DstType* io_pArray, const css::uno::Sequence< SrcType >& i_Sequence )
    {
        ::std::copy( i_Sequence.begin(), i_Sequence.end(), io_pArray );
        return io_pArray;
    }


    /** Copy from a container into a Sequence

        @tpl SrcType
        Container type. This type must fulfill the STL container
        concept, in particular, the size(), begin() and end() methods
        must be available and have the usual semantics.

        @tpl DstType
        Sequence element type. Must be assignable from SrcType's
        elements

        @param i_Container
        Reference to the input contain with elements of type SrcType

        @return the generated Sequence

        @attention this function always performs a copy. Furthermore,
        when copying from e.g. a vector<double> to a Sequence<int>, no
        proper rounding will be performed, but the values will be
        truncated. There's currently no measure to prevent or detect
        precision loss, overflow or truncation.
     */
    template < typename DstElementType, typename SrcType >
    inline css::uno::Sequence< DstElementType > containerToSequence( const SrcType& i_Container )
    {
        using ::std::size, ::std::begin, ::std::end;
        css::uno::Sequence< DstElementType > result( size(i_Container) );
        ::std::copy( begin(i_Container), end(i_Container), result.getArray() );
        return result;
    }

    // this one does better type deduction, but does not allow us to copy into a different element type
    template < typename SrcType >
    inline css::uno::Sequence< typename SrcType::value_type > containerToSequence( const SrcType& i_Container )
    {
        return containerToSequence<typename SrcType::value_type, SrcType>(i_Container);
    }

    // handle arrays
    template<typename ElementType, std::size_t SrcSize>
    inline css::uno::Sequence< ElementType > containerToSequence( ElementType const (&i_Array)[ SrcSize ] )
    {
        return css::uno::Sequence< ElementType >( i_Array, SrcSize );
    }

    template <typename T>
    inline css::uno::Sequence<T> containerToSequence(
        ::std::vector<T> const& v )
    {
        return css::uno::Sequence<T>(
            v.data(), static_cast<sal_Int32>(v.size()) );
    }


    /** Copy from a Sequence into a container

        @tpl SrcType
        Sequence element type. Must be assignable to SrcType's
        elements

        @tpl DstType
        Container type. This type must have a constructor taking a pair
        of iterators defining a range to copy from

        @param i_Sequence
        Reference to a Sequence of SrcType elements

        @return the generated container. C++17 copy elision rules apply

        @attention this function always performs a copy. Furthermore,
        when copying from e.g. a Sequence<double> to a vector<int>, no
        proper rounding will be performed, but the values will be
        truncated. There's currently no measure to prevent or detect
        precision loss, overflow or truncation.
     */
    template < typename DstType, typename SrcType >
    inline DstType sequenceToContainer( const css::uno::Sequence< SrcType >& i_Sequence )
    {
        return DstType(i_Sequence.begin(), i_Sequence.end());
    }

    // this one does better type deduction, but does not allow us to copy into a different element type
    template < typename DstType >
    inline DstType sequenceToContainer( const css::uno::Sequence< typename DstType::value_type >& i_Sequence )
    {
        return DstType(i_Sequence.begin(), i_Sequence.end());
    }

    /** Copy from a Sequence into an existing container

        This potentially saves a needless extra copy operation over
        the whole container, as it passes the target object by
        reference.

        @tpl SrcType
        Sequence element type. Must be assignable to SrcType's
        elements

        @tpl DstType
        Container type. This type must fulfill the STL container and
        sequence concepts, in particular, the begin(), end() and
        resize(int) methods must be available and have the usual
        semantics.

        @param o_Output
        Reference to the target container

        @param i_Sequence
        Reference to a Sequence of SrcType elements

        @return a non-const reference to the given container

        @attention this function always performs a copy. Furthermore,
        when copying from e.g. a Sequence<double> to a vector<int>, no
        proper rounding will be performed, but the values will be
        truncated. There's currently no measure to prevent or detect
        precision loss, overflow or truncation.
     */
    template < typename DstType, typename SrcType >
    inline DstType& sequenceToContainer( DstType& o_Output, const css::uno::Sequence< SrcType >& i_Sequence )
    {
        o_Output.resize( i_Sequence.getLength() );
        ::std::copy( i_Sequence.begin(), i_Sequence.end(), o_Output.begin() );
        return o_Output;
    }

    /** Copy (keys or values) from an associate container into a Sequence

        @tpl M map container type eg. std::map/std::unordered_map

        @return the generated Sequence
     */
    template < typename M >
    inline css::uno::Sequence< typename M::key_type > mapKeysToSequence( M const& map )
    {
        css::uno::Sequence< typename M::key_type > ret( static_cast<sal_Int32>(map.size()) );
        std::transform(map.begin(), map.end(), ret.getArray(),
                       [](const auto& i) -> const typename M::key_type& { return i.first; });
        return ret;
    }

    template < typename M >
    inline css::uno::Sequence< typename M::mapped_type > mapValuesToSequence( M const& map )
    {
        css::uno::Sequence< typename M::mapped_type > ret( static_cast<sal_Int32>(map.size()) );
        std::transform(map.begin(), map.end(), ret.getArray(),
                       [](const auto& i) -> const typename M::mapped_type& { return i.second; });
        return ret;
    }

}   // namespace comphelper


#endif // INCLUDED_COMPHELPER_SEQUENCE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
