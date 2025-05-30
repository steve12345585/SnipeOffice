/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_O3TL_SORTED_VECTOR_HXX
#define INCLUDED_O3TL_SORTED_VECTOR_HXX

#include <vector>
#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>

namespace o3tl
{

/** the elements are totally ordered by Compare,
    for no 2 elements !Compare(a,b) && !Compare(b,a) is true
  */
template <class Compare> struct find_unique
{
    template <typename Iterator, typename Comparable>
    auto operator()(Iterator first, Iterator last, Comparable const& v)
    {
        auto const it = std::lower_bound(first, last, v, Compare());
        return std::make_pair(it, (it != last && !Compare()(v, *it)));
    }
};

/** Represents a sorted vector of values.

    @tpl Value class of item to be stored in container
    @tpl Compare comparison method
    @tpl Find   look up index of a Value in the array
*/
template<
     typename Value,
     typename Compare = std::less<Value>,
     template<typename> class Find = find_unique >
class sorted_vector
{
private:
    typedef Find<Compare> Find_t;
    typedef typename std::vector<Value> vector_t;
    typedef typename std::vector<Value>::iterator  iterator;
public:
    typedef typename std::vector<Value>::const_iterator const_iterator;
    typedef typename std::vector<Value>::const_reverse_iterator const_reverse_iterator;
    typedef typename std::vector<Value>::difference_type difference_type;
    typedef typename std::vector<Value>::size_type size_type;
    typedef Value value_type;

    constexpr sorted_vector( std::initializer_list<Value> init )
        : m_vector(init)
    {
        std::sort(m_vector.begin(), m_vector.end(), Compare());
    }
    sorted_vector() = default;
    sorted_vector(sorted_vector const&) requires std::is_copy_constructible_v<Value> = default;
    sorted_vector(sorted_vector&&) = default;

    sorted_vector& operator=(sorted_vector const&) requires std::is_copy_constructible_v<Value> = default;
    sorted_vector& operator=(sorted_vector&&) = default;

    // MODIFIERS

    std::pair<const_iterator,bool> insert( Value&& x )
    {
        std::pair<const_iterator, bool> const ret(Find_t()(m_vector.begin(), m_vector.end(), x));
        if (!ret.second)
        {
            const_iterator const it = m_vector.insert(m_vector.begin() + (ret.first - m_vector.begin()), std::move(x));
            return std::make_pair(it, true);
        }
        return std::make_pair(ret.first, false);
    }

    std::pair<const_iterator,bool> insert( const Value& x )
    {
        std::pair<const_iterator, bool> const ret(Find_t()(m_vector.begin(), m_vector.end(), x));
        if (!ret.second)
        {
            const_iterator const it = m_vector.insert(m_vector.begin() + (ret.first - m_vector.begin()), x);
            return std::make_pair(it, true);
        }
        return std::make_pair(ret.first, false);
    }

    size_type erase( const Value& x )
    {
        std::pair<const_iterator, bool> const ret(Find_t()(m_vector.begin(), m_vector.end(), x));
        if (ret.second)
        {
            m_vector.erase(m_vector.begin() + (ret.first - m_vector.begin()));
            return 1;
        }
        return 0;
    }

    void erase_at(size_t index)
    {
        m_vector.erase(m_vector.begin() + index);
    }

    // like C++ 2011: erase with const_iterator (doesn't change sort order)
    const_iterator erase(const_iterator const& position)
    {   // C++98 has vector::erase(iterator), so call that
        return m_vector.erase(m_vector.begin() + (position - m_vector.begin()));
    }

    void erase(const_iterator const& first, const_iterator const& last)
    {
        m_vector.erase(m_vector.begin() + (first - m_vector.begin()),
                       m_vector.begin() + (last - m_vector.begin()));
    }

    /**
     * make erase return the removed element, otherwise there is no useful way of extracting a std::unique_ptr
     * from this.
     */
    Value erase_extract( size_t index )
    {
        Value val = std::move(m_vector[index]);
        m_vector.erase(m_vector.begin() + index);
        return val;
    }

    void clear()
    {
        m_vector.clear();
    }

    void swap(sorted_vector & other)
    {
        m_vector.swap(other.m_vector);
    }

    void reserve(size_type amount)
    {
        m_vector.reserve(amount);
    }

    // ACCESSORS

    size_type size() const
    {
        return m_vector.size();
    }

    bool empty() const
    {
        return m_vector.empty();
    }

    // Only return a const iterator, so that the vector cannot be directly updated.
    const_iterator begin() const
    {
        return m_vector.begin();
    }

    // Only return a const iterator, so that the vector cannot be directly updated.
    const_iterator end() const
    {
        return m_vector.end();
    }

    // Only return a const iterator, so that the vector cannot be directly updated.
    const_reverse_iterator rbegin() const
    {
        return m_vector.rbegin();
    }

    // Only return a const iterator, so that the vector cannot be directly updated.
    const_reverse_iterator rend() const
    {
        return m_vector.rend();
    }

    const Value& front() const
    {
        return m_vector.front();
    }

    const Value& back() const
    {
        return m_vector.back();
    }

    const Value& operator[]( size_t index ) const
    {
        return m_vector.operator[]( index );
    }

    // OPERATIONS

    template <typename Comparable> const_iterator lower_bound(const Comparable& x) const
    {
        return std::lower_bound( m_vector.begin(), m_vector.end(), x, Compare() );
    }

    template <typename Comparable> const_iterator upper_bound(const Comparable& x) const
    {
        return std::upper_bound( m_vector.begin(), m_vector.end(), x, Compare() );
    }

    /* Searches the container for an element with a value of x
     * and returns an iterator to it if found, otherwise it returns an
     * iterator to sorted_vector::end (the element past the end of the container).
     *
     * Only return a const iterator, so that the vector cannot be directly updated.
     */
    template <typename Comparable> const_iterator find(const Comparable& x) const
    {
        std::pair<const_iterator, bool> const ret(Find_t()(m_vector.begin(), m_vector.end(), x));
        return (ret.second) ? ret.first : m_vector.end();
    }

    size_type count(const Value& v) const
    {
        return find(v) != end() ? 1 : 0;
    }

    bool operator==(const sorted_vector & other) const
    {
        return m_vector == other.m_vector;
    }

    bool operator!=(const sorted_vector & other) const
    {
        return m_vector != other.m_vector;
    }

    void insert(const sorted_vector& rOther)
    {
        // optimization for the rather common case that we are overwriting this with the contents
        // of another sorted vector
        if ( empty() )
            m_vector.insert(m_vector.begin(), rOther.m_vector.begin(), rOther.m_vector.end());
        else
            insert_internal( rOther.m_vector );
    }

    void insert_sorted_unique_vector(const std::vector<Value>& rOther)
    {
        assert( std::is_sorted(rOther.begin(), rOther.end(), Compare()));
        assert( std::unique(rOther.begin(), rOther.end(), compare_equal) == rOther.end());
        if ( empty() )
            m_vector.insert(m_vector.begin(), rOther.m_vector.begin(), rOther.m_vector.end());
        else
            insert_internal( rOther );
    }

    void insert_sorted_unique_vector(std::vector<Value>&& rOther)
    {
        assert( std::is_sorted(rOther.begin(), rOther.end(), Compare()));
        assert( std::unique(rOther.begin(), rOther.end(), compare_equal) == rOther.end());
        if ( empty() )
            m_vector.swap( rOther );
        else
            insert_internal( rOther );
    }

    /* Clear() elements in the vector, and free them one by one. */
    void DeleteAndDestroyAll()
    {
        for (const_iterator it = m_vector.begin(); it != m_vector.end(); ++it)
        {
            delete *it;
        }

        clear();
    }

    // fdo#58793: some existing code in Writer (SwpHintsArray)
    // routinely modifies the members of the vector in a way that
    // violates the sort order, and then re-sorts the array.
    // This is a kludge to enable that code to work.
    // If you are calling this function, you are Doing It Wrong!
    void Resort()
    {
        std::stable_sort(m_vector.begin(), m_vector.end(), Compare());
    }

private:
    static bool compare_equal( const Value& v1, const Value& v2 )
    {   // Synthetize == check from < check for std::unique asserts above.
        return !Compare()( v1, v2 ) && !Compare()( v2, v1 );
    }

    void insert_internal( const std::vector<Value>& rOther )
    {
        // Do a union in one pass rather than repeated insert() that could repeatedly
        // move large amounts of data.
        vector_t tmp;
        tmp.reserve( m_vector.size() + rOther.size());
        std::set_union( m_vector.begin(), m_vector.end(),
                        rOther.begin(), rOther.end(),
                        std::back_inserter( tmp ), Compare());
        m_vector.swap( tmp );
    }

    vector_t m_vector;
};


/** Implements an ordering function over a pointer, where the comparison uses the < operator on the pointed-to types.
    Very useful for the cases where we put pointers to objects inside a sorted_vector.
*/
struct less_ptr_to
{
    template <class T1, class T2> bool operator()(const T1& lhs, const T2& rhs) const
    {
        return (*lhs) < (*rhs);
    }
};

/** the elements are partially ordered by Compare,
    2 elements are allowed if they are not the same element (pointer equal)
  */
template <class Compare> struct find_partialorder_ptrequals
{
    template <typename Iterator, typename Comparable>
    auto operator()(Iterator first, Iterator last, Comparable const& v)
    {
        auto const& [begin, end] = std::equal_range(first, last, v, Compare());
        for (auto it = begin; it != end; ++it)
        {
            if (&*v == &**it)
            {
                return std::make_pair(it, true);
            }
        }
        return std::make_pair(begin, false);
    }
};

template <class Ref, class Referenced>
concept is_reference_to = std::is_convertible_v<decltype(*std::declval<Ref>()), Referenced>;

}   // namespace o3tl
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
