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

/*
 * This file is part of LibreOffice published API.
 */
#ifndef INCLUDED_COM_SUN_STAR_UNO_REFERENCE_H
#define INCLUDED_COM_SUN_STAR_UNO_REFERENCE_H

#include "sal/config.h"

#include <cassert>
#include <cstddef>

#if defined LIBO_INTERNAL_ONLY
#include <type_traits>
#endif

#include "rtl/alloc.h"

namespace com
{
namespace sun
{
namespace star
{
namespace uno
{

class RuntimeException;
class XInterface;
class Type;
class Any;

/** Enum defining UNO_REF_NO_ACQUIRE for setting reference without acquiring a given interface.
    Deprecated, please use SAL_NO_ACQUIRE.
    @deprecated
*/
enum UnoReference_NoAcquire
{
    /** This enum value can be used for creating a reference granting a given interface,
        i.e. transferring ownership to it.
    */
    UNO_REF_NO_ACQUIRE
};

/** This base class serves as a base class for all template reference classes and
    has been introduced due to compiler problems with templated operators ==, =!.
*/
class BaseReference
{
protected:
    /** the interface pointer
    */
    XInterface * _pInterface;

    /** Queries given interface for type rType.

        @param pInterface interface pointer
        @param rType interface type
        @return interface of demanded type (may be null)
    */
    inline static XInterface * SAL_CALL iquery( XInterface * pInterface, const Type & rType );
    /** Queries given interface for type rType.
        Throws a RuntimeException if the demanded interface cannot be queried.

        @param pInterface interface pointer
        @param rType interface type
        @return interface of demanded type
    */
    inline static XInterface * SAL_CALL iquery_throw( XInterface * pInterface, const Type & rType );

public:
    /** Gets interface pointer. This call does not acquire the interface.

        @return UNacquired interface pointer
    */
    XInterface * SAL_CALL get() const
        { return _pInterface; }

    /** Checks if reference is null.

        @return true if reference acquires an interface, i.e. true if it is not null
    */
    bool SAL_CALL is() const
        { return (NULL != _pInterface); }

#if defined LIBO_INTERNAL_ONLY
    /** Checks if reference is null.

        @return true if reference acquires an interface, i.e. true if it is not null
    */
    explicit operator bool() const
        { return is(); }
#endif

    /** Equality operator: compares two interfaces
        Checks if both references are null or refer to the same object.

        @param pInterface another interface
        @return true if both references are null or refer to the same object, false otherwise
    */
    inline bool SAL_CALL operator == ( XInterface * pInterface ) const;
    /** Inequality operator: compares two interfaces
        Checks if both references are null or refer to the same object.

        @param pInterface another interface
        @return false if both references are null or refer to the same object, true otherwise
    */
    inline bool SAL_CALL operator != ( XInterface * pInterface ) const;

    /** Equality operator: compares two interfaces
        Checks if both references are null or refer to the same object.

        @param rRef another reference
        @return true if both references are null or refer to the same object, false otherwise
    */
    inline bool SAL_CALL operator == ( const BaseReference & rRef ) const;
    /** Inequality operator: compares two interfaces
        Checks if both references are null or refer to the same object.

        @param rRef another reference
        @return false if both references are null or refer to the same object, true otherwise
    */
    inline bool SAL_CALL operator != ( const BaseReference & rRef ) const;

    /** Needed by some STL containers.

        @param rRef another reference
        @return true, if this reference is less than rRef
    */
    inline bool SAL_CALL operator < ( const BaseReference & rRef ) const;
};

/** Enum defining UNO_QUERY for implicit interface query.
*/
enum UnoReference_Query
{
    /** This enum value can be used for implicit interface query.
    */
    UNO_QUERY
};
/** Enum defining UNO_QUERY_THROW for implicit interface query.
    If the demanded interface is unavailable, then a RuntimeException is thrown.
*/
enum UnoReference_QueryThrow
{
    /** This enum value can be used for implicit interface query.
    */
    UNO_QUERY_THROW
};
/** Enum defining UNO_SET_THROW for throwing if attempts are made to assign a null
    interface

    @since UDK 3.2.8
*/
enum UnoReference_SetThrow
{
    UNO_SET_THROW
};

/** Template reference class for interface type derived from BaseReference.
    A special constructor given the UNO_QUERY identifier queries interfaces
    for reference type.
*/
template< class interface_type >
class SAL_DLLPUBLIC_RTTI Reference : public BaseReference
{
    /** Queries given interface for type interface_type.

        @param pInterface interface pointer
        @return interface of demanded type (may be null)
    */
    inline static XInterface * SAL_CALL iquery( XInterface * pInterface );
    /** Queries given interface for type interface_type.
        Throws a RuntimeException if the demanded interface cannot be queried.

        @param pInterface interface pointer
        @return interface of demanded type
    */
    inline static XInterface * SAL_CALL iquery_throw( XInterface * pInterface );
    /** Returns the given interface if it is not <NULL/>, throws a RuntimeException otherwise.

        @param pInterface interface pointer
        @return pInterface
    */
    inline static interface_type * SAL_CALL iset_throw( interface_type * pInterface );

    /** Cast from an "interface pointer" (e.g., BaseReference::_pInterface) to a
        pointer to this interface_type.

        To work around ambiguities in the case of multiple-inheritance interface
        types (which inherit XInterface more than once), use reinterpret_cast
        (resp. a sequence of two static_casts, to avoid warnings about
        reinterpret_cast used between related classes) to switch from a pointer
        to XInterface to a pointer to this derived interface_type.  In
        principle, this is not guaranteed to work.  In practice, it seems to
        work on all supported platforms.
    */
    static interface_type * castFromXInterface(XInterface * p) {
        return static_cast< interface_type * >(static_cast< void * >(p));
    }

    /** Cast from a pointer to this interface_type to an "interface pointer"
        (e.g., BaseReference::_pInterface).

        To work around ambiguities in the case of multiple-inheritance interface
        types (which inherit XInterface more than once), use reinterpret_cast
        (resp. a sequence of two static_casts, to avoid warnings about
        reinterpret_cast used between related classes) to switch from a pointer
        to this derived interface_type to a pointer to XInterface.  In
        principle, this is not guaranteed to work.  In practice, it seems to
        work on all supported platforms.
    */
    static XInterface * castToXInterface(interface_type * p) {
        return static_cast< XInterface * >(static_cast< void * >(p));
    }

public:
    /// @cond INTERNAL
    // these are here to force memory de/allocation to sal lib.
    static void * SAL_CALL operator new ( ::size_t nSize )
        { return ::rtl_allocateMemory( nSize ); }
    static void SAL_CALL operator delete ( void * pMem )
        { ::rtl_freeMemory( pMem ); }
    static void * SAL_CALL operator new ( ::size_t, void * pMem )
        { return pMem; }
    static void SAL_CALL operator delete ( void *, void * )
        {}
    /// @endcond

    /** Destructor: Releases interface if set.
    */
    inline ~Reference() COVERITY_NOEXCEPT_FALSE;

    /** Default Constructor: Sets null reference.
    */
    inline Reference();

    /** Copy constructor: Copies interface reference.

        @param rRef another reference
    */
    inline Reference( const Reference< interface_type > & rRef );

#if defined LIBO_INTERNAL_ONLY
    /** Move constructor

        @param rRef another reference
    */
#if !defined(__COVERITY__) // suppress COPY_INSTEAD_OF_MOVE suggestions
    inline Reference( Reference< interface_type > && rRef ) noexcept;
#endif

    /** Up-casting conversion constructor: Copies interface reference.

        Does not work for up-casts to ambiguous bases.  For the special case of
        up-casting to Reference< XInterface >, see the corresponding conversion
        operator.

        @param rRef another reference
    */
    template< class derived_type >
    inline Reference(
        const Reference< derived_type > & rRef,
        std::enable_if_t<
            std::is_base_of_v<interface_type, derived_type>
            && !std::is_same_v<interface_type, XInterface>, void *> = nullptr);
#endif

    /** Constructor: Sets given interface pointer.

        @param pInterface an interface pointer
    */
    inline Reference( interface_type * pInterface );

    /** Constructor: Sets given interface pointer without acquiring it.

        @param pInterface another reference
        @param dummy SAL_NO_ACQUIRE to force obvious distinction to other constructors
    */
    inline Reference( interface_type * pInterface, __sal_NoAcquire dummy);
    /** Constructor: Sets given interface pointer without acquiring it.
        Deprecated, please use SAL_NO_ACQUIRE version.

        @deprecated
        @param pInterface another reference
        @param dummy UNO_REF_NO_ACQUIRE to force obvious distinction to other constructors
    */
    inline SAL_DEPRECATED("use SAL_NO_ACQUIRE version") Reference( interface_type * pInterface, UnoReference_NoAcquire dummy );

    /** Constructor: Queries given interface for reference interface type (interface_type).

        @param rRef another reference
        @param dummy UNO_QUERY to force obvious distinction to other constructors
    */
    inline Reference( const BaseReference & rRef, UnoReference_Query dummy );
    /** Constructor: Queries given interface for reference interface type (interface_type).

        @param pInterface an interface pointer
        @param dummy UNO_QUERY to force obvious distinction to other constructors
    */
    inline Reference( XInterface * pInterface, UnoReference_Query dummy);
    /** Constructor: Queries given any for reference interface type (interface_type).

        @param rAny an any
        @param dummy UNO_QUERY to force obvious distinction to other constructors
    */
    inline Reference( const Any & rAny, UnoReference_Query dummy);
    /** Constructor: Queries given interface for reference interface type (interface_type).
        Throws a RuntimeException if the demanded interface cannot be queried.

        @param rRef another reference
        @param dummy UNO_QUERY_THROW to force obvious distinction
                     to other constructors
    */
    inline Reference( const BaseReference & rRef, UnoReference_QueryThrow dummy );
#ifdef LIBO_INTERNAL_ONLY
    /**
        Prevent code from calling the QUERY_THROW constructor, when they meant to use the SET_THROW constructor.
    */
    Reference( const Reference< interface_type > & rRef, UnoReference_QueryThrow dummy ) = delete;
#endif
    /** Constructor: Queries given interface for reference interface type (interface_type).
        Throws a RuntimeException if the demanded interface cannot be queried.

        @param pInterface an interface pointer
        @param dummy UNO_QUERY_THROW to force obvious distinction
                     to other constructors
    */
    inline Reference( XInterface * pInterface, UnoReference_QueryThrow dummy );
    /** Constructor: Queries given any for reference interface type (interface_type).
        Throws a RuntimeException if the demanded interface cannot be queried.

        @param rAny an any
        @param dummy UNO_QUERY_THROW to force obvious distinction
                     to other constructors
    */
    inline Reference( const Any & rAny, UnoReference_QueryThrow dummy );
    /** Constructor: assigns from the given interface of the same type. Throws a RuntimeException
        if the source interface is NULL.

        @param rRef another interface reference of the same type
        @param dummy UNO_SET_THROW to distinguish from default copy constructor

        @since UDK 3.2.8
    */
    inline Reference( const Reference< interface_type > & rRef, UnoReference_SetThrow dummy );
    /** Constructor: assigns from the given interface of the same type. Throws a RuntimeException
        if the source interface is NULL.

        @param pInterface an interface pointer
        @param dummy UNO_SET_THROW to distinguish from default assignment constructor

        @since UDK 3.2.8
    */
    inline Reference( interface_type * pInterface, UnoReference_SetThrow dummy );

    /** Cast operator to Reference< XInterface >: Reference objects are binary compatible and
        any interface must be derived from com.sun.star.uno.XInterface.
        This a useful direct cast possibility.
    */
    SAL_CALL operator const Reference< XInterface > & () const
        { return * reinterpret_cast< const Reference< XInterface > * >( this ); }

    /** Dereference operator: Used to call interface methods.

        @return UNacquired interface pointer
    */
    interface_type * SAL_CALL operator -> () const {
        assert(_pInterface != NULL);
        return castFromXInterface(_pInterface);
    }

    /** Indirection operator.

        @since LibreOffice 6.3
        @return UNacquired interface reference
    */
    interface_type & SAL_CALL operator * () const {
        assert(_pInterface != NULL);
        return *castFromXInterface(_pInterface);
    }

    /** Gets interface pointer. This call does not acquire the interface.

        @return UNacquired interface pointer
    */
    interface_type * SAL_CALL get() const
        { return castFromXInterface(_pInterface); }

    /** Clears reference, i.e. releases interface. Reference is null after clear() call.
    */
    inline void SAL_CALL clear();

    /** Sets the given interface. An interface already set will be released.

        @param rRef another reference
        @return true, if non-null interface was set
    */
    inline bool SAL_CALL set( const Reference< interface_type > & rRef );
    /** Sets the given interface. An interface already set will be released.

        @param pInterface another interface
        @return true, if non-null interface was set
    */
    inline bool SAL_CALL set( interface_type * pInterface );

    /** Sets interface pointer without acquiring it. An interface already set will be released.

        @param pInterface an interface pointer
        @param dummy SAL_NO_ACQUIRE to force obvious distinction to set methods
        @return true, if non-null interface was set
    */
    inline bool SAL_CALL set( interface_type * pInterface, __sal_NoAcquire dummy);
    /** Sets interface pointer without acquiring it. An interface already set will be released.
        Deprecated, please use SAL_NO_ACQUIRE version.

        @deprecated
        @param pInterface an interface pointer
        @param dummy UNO_REF_NO_ACQUIRE to force obvious distinction to set methods
        @return true, if non-null interface was set
    */
    inline SAL_DEPRECATED("use SAL_NO_ACQUIRE version") bool SAL_CALL set( interface_type * pInterface, UnoReference_NoAcquire dummy);

    /** Queries given interface for reference interface type (interface_type) and sets it.
        An interface already set will be released.

        @param pInterface an interface pointer
        @param dummy UNO_QUERY to force obvious distinction to set methods
        @return true, if non-null interface was set
    */
    inline bool SAL_CALL set( XInterface * pInterface, UnoReference_Query dummy );
    /** Queries given interface for reference interface type (interface_type) and sets it.
        An interface already set will be released.

        @param rRef another reference
        @param dummy UNO_QUERY to force obvious distinction to set methods
        @return true, if non-null interface was set
    */
    inline bool SAL_CALL set( const BaseReference & rRef, UnoReference_Query dummy);

    /** Queries given any for reference interface type (interface_type)
        and sets it.  An interface already set will be released.

        @param rAny
               an Any containing an interface
        @param dummy
               UNO_QUERY to force obvious distinction
               to set methods
        @return
                true, if non-null interface was set
    */
    inline bool set( Any const & rAny, UnoReference_Query dummy );

    /** Queries given interface for reference interface type (interface_type) and sets it.
        An interface already set will be released.
        Throws a RuntimeException if the demanded interface cannot be set.

        @param pInterface an interface pointer
        @param dummy UNO_QUERY_THROW to force obvious distinction
                     to set methods
    */
    inline void SAL_CALL set( XInterface * pInterface, UnoReference_QueryThrow dummy );
    /** Queries given interface for reference interface type (interface_type) and sets it.
        An interface already set will be released.
        Throws a RuntimeException if the demanded interface cannot be set.

        @param rRef another reference
        @param dummy UNO_QUERY_THROW to force obvious distinction
               to set methods
    */
    inline void SAL_CALL set( const BaseReference & rRef, UnoReference_QueryThrow dummy );
#ifdef LIBO_INTERNAL_ONLY
    /**
        Prevent code from calling the QUERY_THROW version, when they meant to use the SET_THROW version.
    */
    void set( const Reference< interface_type > & rRef, UnoReference_QueryThrow dummy ) = delete;
#endif

    /** Queries given any for reference interface type (interface_type) and
        sets it.  An interface already set will be released.
        Throws a RuntimeException if the demanded interface cannot be set.

        @param rAny
               an Any containing an interface
        @param dummy
               UNO_QUERY_THROW to force obvious distinction to set methods
    */
    inline void set( Any const & rAny, UnoReference_QueryThrow dummy);
    /** sets the given interface
        An interface already set will be released.
        Throws a RuntimeException if the source interface is @b NULL.

        @param pInterface an interface pointer
        @param dummy UNO_SET_THROW to force obvious distinction to other set methods

        @since UDK 3.2.8
    */
    inline void SAL_CALL set( interface_type * pInterface, UnoReference_SetThrow dummy);
    /** sets the given interface
        An interface already set will be released.
        Throws a RuntimeException if the source interface is @b NULL.

        @param rRef an interface reference
        @param dummy UNO_SET_THROW to force obvious distinction to other set methods

        @since UDK 3.2.8
    */
    inline void SAL_CALL set( const Reference< interface_type > & rRef, UnoReference_SetThrow dummy);


    /** Assignment operator: Acquires given interface pointer and sets reference.
        An interface already set will be released.

        @param pInterface an interface pointer
        @return this reference
    */
    inline Reference< interface_type > & SAL_CALL operator = ( interface_type * pInterface );
    /** Assignment operator: Acquires given interface reference and sets reference.
        An interface already set will be released.

        @param rRef an interface reference
        @return this reference
    */
    inline Reference< interface_type > & SAL_CALL operator = ( const Reference< interface_type > & rRef );
#if defined LIBO_INTERNAL_ONLY
    /** Assignment move operator: Acquires given interface reference and sets reference.
        An interface already set will be released.

        @param rRef an interface reference
        @return this reference
    */
    inline Reference< interface_type > & operator = ( Reference< interface_type > && rRef ) noexcept;
#endif
    /** Queries given interface reference for type interface_type.

        @param rRef interface reference
        @return interface reference of demanded type (may be null)
    */
    SAL_WARN_UNUSED_RESULT inline static Reference< interface_type > SAL_CALL query( const BaseReference & rRef );
    /** Queries given interface for type interface_type.

        @param pInterface interface pointer
        @return interface reference of demanded type (may be null)
    */
    SAL_WARN_UNUSED_RESULT inline static Reference< interface_type > SAL_CALL query( XInterface * pInterface );
#if defined LIBO_INTERNAL_ONLY
    /** Queries this for the required interface, and returns the requested reference, possibly empty.
        A syntactic sugar for 'Reference< other_type > xOther(xThis, UNO_QUERY)' that avoids some
        verbocity.

        @return new reference
    */
    template< class other_type > inline Reference< other_type > query() const;
    /** Queries this for the required interface, and returns the requested reference, or throws
        on failure. A syntactic sugar for 'Reference< other_type > xOther(xThis, UNO_QUERY_THROW)'
        that avoids some verbocity.

        @return new reference
    */
    template< class other_type > inline Reference< other_type > queryThrow() const;
#endif
};

}
}
}
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
