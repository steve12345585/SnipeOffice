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
#ifndef INCLUDED_CPPUHELPER_IMPLBASE2_HXX
#define INCLUDED_CPPUHELPER_IMPLBASE2_HXX

#include "cppuhelper/implbase_ex.hxx"
#include "rtl/instance.hxx"
#include "cppuhelper/weak.hxx"
#include "cppuhelper/weakagg.hxx"
#include "com/sun/star/lang/XTypeProvider.hpp"

namespace cppu
{
    /// @cond INTERNAL

    struct class_data2
    {
        sal_Int16 m_nTypes;
        sal_Bool m_storedTypeRefs;
        sal_Bool m_storedId;
        sal_Int8 m_id[ 16 ];
        type_entry m_typeEntries[ 2 + 1 ];
    };

    template< typename Ifc1, typename Ifc2, typename Impl > struct SAL_WARN_UNUSED ImplClassData2
    {
        class_data* operator ()()
        {
            static class_data2 s_cd =
            {
                2 +1, false, false,
                { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                {
                    CPPUHELPER_DETAIL_TYPEENTRY(Ifc1),
                    CPPUHELPER_DETAIL_TYPEENTRY(Ifc2),
                    CPPUHELPER_DETAIL_TYPEENTRY(css::lang::XTypeProvider)
                }
            };
            return reinterpret_cast< class_data * >(&s_cd);
        }
    };

    /// @endcond

    /** Implementation helper implementing interface css::lang::XTypeProvider
        and queryInterface(), but no reference counting.

        @derive
        Inherit from this class giving your interface(s) to be implemented as template argument(s).
        Your base class defines method implementations, acquire(), release() and delegates incoming
        queryInterface() calls to this base class.
    */
    template< class Ifc1, class Ifc2 >
    class SAL_NO_VTABLE SAL_DLLPUBLIC_TEMPLATE ImplHelper2
        : public css::lang::XTypeProvider
        , public Ifc1, public Ifc2
    {
        struct cd : public rtl::StaticAggregate< class_data, ImplClassData2 < Ifc1, Ifc2, ImplHelper2<Ifc1, Ifc2> > > {};
    public:
#if defined LIBO_INTERNAL_ONLY
        ImplHelper2() = default;
        ImplHelper2(ImplHelper2 const &) = default;
        ImplHelper2(ImplHelper2 &&) = default;
        ImplHelper2 & operator =(ImplHelper2 const &) = default;
        ImplHelper2 & operator =(ImplHelper2 &&) = default;
#endif

        virtual css::uno::Any SAL_CALL queryInterface( css::uno::Type const & rType ) SAL_OVERRIDE
            { return ImplHelper_query( rType, cd::get(), this ); }
        virtual css::uno::Sequence< css::uno::Type > SAL_CALL getTypes() SAL_OVERRIDE
            { return ImplHelper_getTypes( cd::get() ); }
        virtual css::uno::Sequence< sal_Int8 > SAL_CALL getImplementationId() SAL_OVERRIDE
            { return ImplHelper_getImplementationId( cd::get() ); }

#if !defined _MSC_VER // public -> protected changes mangled names there
    protected:
#endif
        ~ImplHelper2() SAL_NOEXCEPT {}
    };
    /** Implementation helper implementing interfaces css::lang::XTypeProvider and
        css::uno::XInterface which supports weak mechanism to be held weakly
        (supporting css::uno::XWeak through ::cppu::OWeakObject).

        @derive
        Inherit from this class giving your interface(s) to be implemented as template argument(s).
        Your sub class defines method implementations for these interface(s).
    */
    template< class Ifc1, class Ifc2 >
    class SAL_NO_VTABLE SAL_DLLPUBLIC_TEMPLATE SAL_DLLPUBLIC_EXPORT WeakImplHelper2
        : public OWeakObject
        , public css::lang::XTypeProvider
        , public Ifc1, public Ifc2
    {
        struct cd : public rtl::StaticAggregate< class_data, ImplClassData2 < Ifc1, Ifc2, WeakImplHelper2<Ifc1, Ifc2> > > {};
    public:
        virtual css::uno::Any SAL_CALL queryInterface( css::uno::Type const & rType ) SAL_OVERRIDE
            { return WeakImplHelper_query( rType, cd::get(), this, static_cast<OWeakObject *>(this) ); }
        virtual void SAL_CALL acquire() SAL_NOEXCEPT SAL_OVERRIDE
            { OWeakObject::acquire(); }
        virtual void SAL_CALL release() SAL_NOEXCEPT SAL_OVERRIDE
            { OWeakObject::release(); }
        virtual css::uno::Sequence< css::uno::Type > SAL_CALL getTypes() SAL_OVERRIDE
            { return WeakImplHelper_getTypes( cd::get() ); }
        virtual css::uno::Sequence< sal_Int8 > SAL_CALL getImplementationId() SAL_OVERRIDE
            { return ImplHelper_getImplementationId( cd::get() ); }
    };
    /** Implementation helper implementing interfaces css::lang::XTypeProvider and
        css::uno::XInterface which supports weak mechanism to be held weakly
        (supporting css::uno::XWeak through ::cppu::OWeakAggObject).
        In addition, it supports also aggregation meaning object of this class can be aggregated
        (css::uno::XAggregation through ::cppu::OWeakAggObject).
        If a delegator is set (this object is aggregated), then incoming queryInterface()
        calls are delegated to the delegator object. If the delegator does not support the
        demanded interface, it calls queryAggregation() on its aggregated objects.

        @derive
        Inherit from this class giving your interface(s) to be implemented as template argument(s).
        Your sub class defines method implementations for these interface(s).
    */
    template< class Ifc1, class Ifc2 >
    class SAL_NO_VTABLE SAL_DLLPUBLIC_TEMPLATE WeakAggImplHelper2
        : public OWeakAggObject
        , public css::lang::XTypeProvider
        , public Ifc1, public Ifc2
    {
        struct cd : public rtl::StaticAggregate< class_data, ImplClassData2 < Ifc1, Ifc2, WeakAggImplHelper2<Ifc1, Ifc2> > > {};
    public:
        virtual css::uno::Any SAL_CALL queryInterface( css::uno::Type const & rType ) SAL_OVERRIDE
            { return OWeakAggObject::queryInterface( rType ); }
        virtual css::uno::Any SAL_CALL queryAggregation( css::uno::Type const & rType ) SAL_OVERRIDE
            { return WeakAggImplHelper_queryAgg( rType, cd::get(), this, static_cast<OWeakAggObject *>(this) ); }
        virtual void SAL_CALL acquire() SAL_NOEXCEPT SAL_OVERRIDE
            { OWeakAggObject::acquire(); }
        virtual void SAL_CALL release() SAL_NOEXCEPT SAL_OVERRIDE
            { OWeakAggObject::release(); }
        virtual css::uno::Sequence< css::uno::Type > SAL_CALL getTypes() SAL_OVERRIDE
            { return WeakAggImplHelper_getTypes( cd::get() ); }
        virtual css::uno::Sequence< sal_Int8 > SAL_CALL getImplementationId() SAL_OVERRIDE
            { return ImplHelper_getImplementationId( cd::get() ); }
    };
    /** Implementation helper implementing interfaces css::lang::XTypeProvider and
        css::uno::XInterface inheriting from a BaseClass.
        All acquire() and release() calls are delegated to the BaseClass. Upon queryInterface(),
        if a demanded interface is not supported by this class directly, the request is
        delegated to the BaseClass.

        @attention
        The BaseClass has to be complete in a sense, that css::uno::XInterface
        and css::lang::XTypeProvider are implemented properly.  The
        BaseClass must have at least one ctor that can be called with six or
        fewer arguments, of which none is of non-const reference type.

        @derive
        Inherit from this class giving your additional interface(s) to be implemented as
        template argument(s). Your sub class defines method implementations for these interface(s).
    */
    template< class BaseClass, class Ifc1, class Ifc2 >
    class SAL_NO_VTABLE SAL_DLLPUBLIC_TEMPLATE ImplInheritanceHelper2
        : public BaseClass
        , public Ifc1, public Ifc2
    {
        struct cd : public rtl::StaticAggregate< class_data, ImplClassData2 < Ifc1, Ifc2, ImplInheritanceHelper2<BaseClass, Ifc1, Ifc2> > > {};
    protected:
        template< typename T1 >
        explicit ImplInheritanceHelper2(T1 const & arg1): BaseClass(arg1) {}
        template< typename T1, typename T2 >
        ImplInheritanceHelper2(T1 const & arg1, T2 const & arg2):
            BaseClass(arg1, arg2) {}
        template< typename T1, typename T2, typename T3 >
        ImplInheritanceHelper2(
            T1 const & arg1, T2 const & arg2, T3 const & arg3):
            BaseClass(arg1, arg2, arg3) {}
        template< typename T1, typename T2, typename T3, typename T4 >
        ImplInheritanceHelper2(
            T1 const & arg1, T2 const & arg2, T3 const & arg3, T4 const & arg4):
            BaseClass(arg1, arg2, arg3, arg4) {}
        template<
            typename T1, typename T2, typename T3, typename T4, typename T5 >
        ImplInheritanceHelper2(
            T1 const & arg1, T2 const & arg2, T3 const & arg3, T4 const & arg4,
            T5 const & arg5):
            BaseClass(arg1, arg2, arg3, arg4, arg5) {}
        template<
            typename T1, typename T2, typename T3, typename T4, typename T5,
            typename T6 >
        ImplInheritanceHelper2(
            T1 const & arg1, T2 const & arg2, T3 const & arg3, T4 const & arg4,
            T5 const & arg5, T6 const & arg6):
            BaseClass(arg1, arg2, arg3, arg4, arg5, arg6) {}
    public:
        ImplInheritanceHelper2() {}
        virtual css::uno::Any SAL_CALL queryInterface( css::uno::Type const & rType ) SAL_OVERRIDE
            {
                css::uno::Any aRet( ImplHelper_queryNoXInterface( rType, cd::get(), this ) );
                if (aRet.hasValue())
                    return aRet;
                return BaseClass::queryInterface( rType );
            }
        virtual void SAL_CALL acquire() SAL_NOEXCEPT SAL_OVERRIDE
            { BaseClass::acquire(); }
        virtual void SAL_CALL release() SAL_NOEXCEPT SAL_OVERRIDE
            { BaseClass::release(); }
        virtual css::uno::Sequence< css::uno::Type > SAL_CALL getTypes() SAL_OVERRIDE
            { return ImplInhHelper_getTypes( cd::get(), BaseClass::getTypes() ); }
        virtual css::uno::Sequence< sal_Int8 > SAL_CALL getImplementationId() SAL_OVERRIDE
            { return ImplHelper_getImplementationId( cd::get() ); }
    };
    /** Implementation helper implementing interfaces css::lang::XTypeProvider and
        css::uno::XInterface inheriting from a BaseClass.
        All acquire(),  release() and queryInterface() calls are delegated to the BaseClass.
        Upon queryAggregation(), if a demanded interface is not supported by this class directly,
        the request is delegated to the BaseClass.

        @attention
        The BaseClass has to be complete in a sense, that css::uno::XInterface,
        css::uno::XAggregation and css::lang::XTypeProvider
        are implemented properly.  The BaseClass must have at least one ctor
        that can be called with six or fewer arguments, of which none is of
        non-const reference type.

        @derive
        Inherit from this class giving your additional interface(s) to be implemented as
        template argument(s). Your sub class defines method implementations for these interface(s).
    */
    template< class BaseClass, class Ifc1, class Ifc2 >
    class SAL_NO_VTABLE SAL_DLLPUBLIC_TEMPLATE AggImplInheritanceHelper2
        : public BaseClass
        , public Ifc1, public Ifc2
    {
        struct cd : public rtl::StaticAggregate< class_data, ImplClassData2 < Ifc1, Ifc2, AggImplInheritanceHelper2<BaseClass, Ifc1, Ifc2> > > {};
    protected:
        template< typename T1 >
        explicit AggImplInheritanceHelper2(T1 const & arg1): BaseClass(arg1) {}
        template< typename T1, typename T2 >
        AggImplInheritanceHelper2(T1 const & arg1, T2 const & arg2):
            BaseClass(arg1, arg2) {}
        template< typename T1, typename T2, typename T3 >
        AggImplInheritanceHelper2(
            T1 const & arg1, T2 const & arg2, T3 const & arg3):
            BaseClass(arg1, arg2, arg3) {}
        template< typename T1, typename T2, typename T3, typename T4 >
        AggImplInheritanceHelper2(
            T1 const & arg1, T2 const & arg2, T3 const & arg3, T4 const & arg4):
            BaseClass(arg1, arg2, arg3, arg4) {}
        template<
            typename T1, typename T2, typename T3, typename T4, typename T5 >
        AggImplInheritanceHelper2(
            T1 const & arg1, T2 const & arg2, T3 const & arg3, T4 const & arg4,
            T5 const & arg5):
            BaseClass(arg1, arg2, arg3, arg4, arg5) {}
        template<
            typename T1, typename T2, typename T3, typename T4, typename T5,
            typename T6 >
        AggImplInheritanceHelper2(
            T1 const & arg1, T2 const & arg2, T3 const & arg3, T4 const & arg4,
            T5 const & arg5, T6 const & arg6):
            BaseClass(arg1, arg2, arg3, arg4, arg5, arg6) {}
    public:
        AggImplInheritanceHelper2() {}
        virtual css::uno::Any SAL_CALL queryInterface( css::uno::Type const & rType ) SAL_OVERRIDE
            { return BaseClass::queryInterface( rType ); }
        virtual css::uno::Any SAL_CALL queryAggregation( css::uno::Type const & rType ) SAL_OVERRIDE
            {
                css::uno::Any aRet( ImplHelper_queryNoXInterface( rType, cd::get(), this ) );
                if (aRet.hasValue())
                    return aRet;
                return BaseClass::queryAggregation( rType );
            }
        virtual void SAL_CALL acquire() SAL_NOEXCEPT SAL_OVERRIDE
            { BaseClass::acquire(); }
        virtual void SAL_CALL release() SAL_NOEXCEPT SAL_OVERRIDE
            { BaseClass::release(); }
        virtual css::uno::Sequence< css::uno::Type > SAL_CALL getTypes() SAL_OVERRIDE
            { return ImplInhHelper_getTypes( cd::get(), BaseClass::getTypes() ); }
        virtual css::uno::Sequence< sal_Int8 > SAL_CALL getImplementationId() SAL_OVERRIDE
            { return ImplHelper_getImplementationId( cd::get() ); }
    };
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
