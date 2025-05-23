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

#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/beans/Property.hpp>
#include <rtl/ustring.hxx>

#include <set>
#include <utility>


namespace pcr
{


    OUString GetUIHeadlineName(sal_Int16 _nClassId, const css::uno::Any& _rUnoObject);
    sal_Int16 classifyComponent( const css::uno::Reference< css::uno::XInterface >& _rxComponent );


    struct FindPropertyByHandle
    {
    private:
        sal_Int32 m_nId;

    public:
        explicit FindPropertyByHandle( sal_Int32 _nId ) : m_nId ( _nId ) { }
        bool operator()( const css::beans::Property& _rProp ) const
        {
            return m_nId == _rProp.Handle;
        }
    };


    struct FindPropertyByName
    {
    private:
        OUString m_sName;

    public:
        explicit FindPropertyByName( OUString _aName ) : m_sName(std::move( _aName )) { }
        bool operator()( const css::beans::Property& _rProp ) const
        {
            return m_sName == _rProp.Name;
        }
    };


    struct PropertyLessByName
    {
        bool operator() (const css::beans::Property& _rLhs, const css::beans::Property& _rRhs) const
        {
            return _rLhs.Name < _rRhs.Name;
        }
    };


    struct TypeLessByName
    {
        bool operator() (const css::uno::Type& _rLhs, const css::uno::Type& _rRhs) const
        {
            return _rLhs.getTypeName() < _rRhs.getTypeName();
        }
    };


    typedef std::set< css::beans::Property, PropertyLessByName > PropertyBag;


} // namespace pcr


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
