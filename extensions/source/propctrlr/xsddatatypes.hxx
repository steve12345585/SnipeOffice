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

#include <com/sun/star/uno/Reference.hxx>
#include <rtl/ref.hxx>
#include <salhelper/simplereferenceobject.hxx>

namespace com::sun::star {
    namespace xsd {
        class XDataType;
    }
    namespace beans {
        class XPropertySetInfo;
    }
}


namespace pcr
{


    //= XSDDataType

    class XSDDataType : public salhelper::SimpleReferenceObject
    {
    private:
        css::uno::Reference< css::xsd::XDataType >
                            m_xDataType;
        css::uno::Reference< css::beans::XPropertySetInfo >
                            m_xFacetInfo;

    public:
        explicit XSDDataType(
            const css::uno::Reference< css::xsd::XDataType >& _rxDataType
        );

        /// retrieves the underlying UNO component
        const css::uno::Reference< css::xsd::XDataType >&
                getUnoDataType() const { return m_xDataType; }

        /// classifies the data typ
        sal_Int16           classify() const;

        // attribute access
        OUString            getName() const;
        bool                isBasicType() const;

        /// determines whether a given facet exists at the type
        bool                hasFacet( const OUString& _rFacetName ) const;
        /// retrieves a facet value
        css::uno::Any       getFacet( const OUString& _rFacetName );
        /// sets a facet value
        void                setFacet( const OUString& _rFacetName, const css::uno::Any& _rFacetValue );

        /** copies as much facets (values, respectively) from a give data type instance
        */
        void                copyFacetsFrom( const ::rtl::Reference< XSDDataType >& _pSourceType );

    protected:
        virtual ~XSDDataType() override;

    private:
        XSDDataType( const XSDDataType& ) = delete;
        XSDDataType& operator=( const XSDDataType& ) = delete;
    };


} // namespace pcr


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
