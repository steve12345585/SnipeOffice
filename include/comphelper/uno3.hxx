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

#ifndef INCLUDED_COMPHELPER_UNO3_HXX
#define INCLUDED_COMPHELPER_UNO3_HXX

#include <com/sun/star/uno/XAggregation.hpp>
#include <comphelper/sequence.hxx>


namespace comphelper
{
    /** used for declaring UNO3-Defaults, i.e. acquire/release
    */
    #define DECLARE_UNO3_DEFAULTS(classname, baseclass) \
        virtual void    SAL_CALL acquire() noexcept override { baseclass::acquire(); }    \
        virtual void    SAL_CALL release() noexcept override { baseclass::release(); }

    /** used for declaring UNO3-Defaults, i.e. acquire/release if you want to forward all queryInterfaces to the base class,
        (e.g. if you override queryAggregation)
    */
    #define DECLARE_UNO3_AGG_DEFAULTS(classname, baseclass) \
        virtual void            SAL_CALL acquire() noexcept override { baseclass::acquire(); } \
        virtual void            SAL_CALL release() noexcept override { baseclass::release(); }    \
        virtual css::uno::Any  SAL_CALL queryInterface(const css::uno::Type& _rType) override \
            { return baseclass::queryInterface(_rType); }

    /** Use this macro to forward XComponent methods to base class

        When using the ::cppu::WeakComponentImplHelper base classes to
        implement a UNO interface, a problem occurs when the interface
        itself already derives from XComponent (like e.g. awt::XWindow
        or awt::XControl): ::cppu::WeakComponentImplHelper is then
        still abstract. Using this macro in the most derived class
        definition provides overrides for the XComponent methods,
        forwarding them to the given baseclass.

        @param classname
        Name of the class this macro is issued within

        @param baseclass
        Name of the baseclass that should have the XInterface methods
        forwarded to - that's usually the WeakComponentImplHelperN base

        @param implhelper
        Name of the baseclass that should have the XComponent methods
        forwarded to - in the case of the WeakComponentImplHelper,
        that would be ::cppu::WeakComponentImplHelperBase
    */
    #define DECLARE_UNO3_XCOMPONENT_AGG_DEFAULTS(classname, baseclass, implhelper) \
        virtual void SAL_CALL acquire() noexcept override { baseclass::acquire(); }   \
        virtual void SAL_CALL release() noexcept override { baseclass::release(); }   \
        virtual css::uno::Any  SAL_CALL queryInterface(const css::uno::Type& _rType) override \
            { return baseclass::queryInterface(_rType); }                               \
        virtual void SAL_CALL dispose() override \
        {                                                                               \
            implhelper::dispose();                                                      \
        }                                                                               \
        virtual void SAL_CALL addEventListener(                                         \
            css::uno::Reference< css::lang::XEventListener > const & xListener ) override \
        {                                                                               \
            implhelper::addEventListener(xListener);                                        \
        }                                                                               \
        virtual void SAL_CALL removeEventListener(                                      \
            css::uno::Reference< css::lang::XEventListener > const & xListener ) override \
        {                                                                               \
            implhelper::removeEventListener(xListener);                                 \
        }

    //= deriving from multiple XInterface-derived classes

    //= forwarding/merging XInterface functionality

    #define DECLARE_XINTERFACE( )   \
        virtual css::uno::Any SAL_CALL queryInterface( const css::uno::Type& aType ) override; \
        virtual void SAL_CALL acquire() noexcept override; \
        virtual void SAL_CALL release() noexcept override;

    #define IMPLEMENT_FORWARD_REFCOUNT( classname, refcountbase ) \
        void SAL_CALL classname::acquire() noexcept { refcountbase::acquire(); } \
        void SAL_CALL classname::release() noexcept { refcountbase::release(); }

    #define IMPLEMENT_FORWARD_XINTERFACE2( classname, refcountbase, baseclass2 ) \
        IMPLEMENT_FORWARD_REFCOUNT( classname, refcountbase ) \
        css::uno::Any SAL_CALL classname::queryInterface( const css::uno::Type& _rType ) \
        { \
            css::uno::Any aReturn = refcountbase::queryInterface( _rType ); \
            if ( !aReturn.hasValue() ) \
                aReturn = baseclass2::queryInterface( _rType ); \
            return aReturn; \
        }

    #define IMPLEMENT_FORWARD_XINTERFACE3( classname, refcountbase, baseclass2, baseclass3 ) \
        IMPLEMENT_FORWARD_REFCOUNT( classname, refcountbase ) \
        css::uno::Any SAL_CALL classname::queryInterface( const css::uno::Type& _rType ) \
        { \
            css::uno::Any aReturn = refcountbase::queryInterface( _rType ); \
            if ( !aReturn.hasValue() ) \
            { \
                aReturn = baseclass2::queryInterface( _rType ); \
                if ( !aReturn.hasValue() ) \
                    aReturn = baseclass3::queryInterface( _rType ); \
            } \
            return aReturn; \
        }


    //= forwarding/merging XTypeProvider functionality

    #define DECLARE_XTYPEPROVIDER( )    \
        virtual css::uno::Sequence< css::uno::Type > SAL_CALL getTypes(  ) override; \
        virtual css::uno::Sequence< sal_Int8 > SAL_CALL getImplementationId(  ) override;

    #define IMPLEMENT_GET_IMPLEMENTATION_ID( classname ) \
        css::uno::Sequence< sal_Int8 > SAL_CALL classname::getImplementationId(  ) \
        { \
            return css::uno::Sequence<sal_Int8>(); \
        }

    #define IMPLEMENT_FORWARD_XTYPEPROVIDER2( classname, baseclass1, baseclass2 ) \
        css::uno::Sequence< css::uno::Type > SAL_CALL classname::getTypes(  ) \
        { \
            return ::comphelper::concatSequences( \
                baseclass1::getTypes(), \
                baseclass2::getTypes() \
            ); \
        } \
        \
        IMPLEMENT_GET_IMPLEMENTATION_ID( classname )

    /** ask for an iface of an aggregated object
        usage:<br/>
            Reference<XFoo> xFoo;<br/>
            if (query_aggregation(xAggregatedObject, xFoo))<br/>
                ...
    */
    template <class iface>
    bool query_aggregation(const css::uno::Reference< css::uno::XAggregation >& _rxAggregate, css::uno::Reference<iface>& _rxOut)
    {
        _rxOut.clear();
        if (_rxAggregate.is())
        {
            _rxAggregate->queryAggregation(cppu::UnoType<iface>::get())
                >>= _rxOut;
        }
        return _rxOut.is();
    }

    /** ask for an iface of an aggregated object
        usage:<br/>
            if (auto xFoo = query_aggregation<XFoo>(xAggregatedObject))<br/>
                ...
    */
    template <class iface>
    css::uno::Reference<iface> query_aggregation(const css::uno::Reference< css::uno::XAggregation >& _rxAggregate)
    {
        css::uno::Reference<iface> _rxOut;
        query_aggregation(_rxAggregate, _rxOut);
        return _rxOut;
    }
}   // namespace comphelper


#endif // INCLUDED_COMPHELPER_UNO3_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
