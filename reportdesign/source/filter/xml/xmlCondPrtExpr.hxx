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
#ifndef INCLUDED_REPORTDESIGN_SOURCE_FILTER_XML_XMLCONDPRTEXPR_HXX
#define INCLUDED_REPORTDESIGN_SOURCE_FILTER_XML_XMLCONDPRTEXPR_HXX

#include <xmloff/xmlictxt.hxx>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <rtl/ustrbuf.hxx>

namespace rptxml
{
    class ORptFilter;
    class OXMLCondPrtExpr : public SvXMLImportContext
    {
        css::uno::Reference< css::beans::XPropertySet >   m_xComponent;
        OUStringBuffer m_aCharBuffer;

        OXMLCondPrtExpr(const OXMLCondPrtExpr&) = delete;
        void operator =(const OXMLCondPrtExpr&) = delete;
    public:

        OXMLCondPrtExpr( ORptFilter& _rImport
                    ,const css::uno::Reference< css::xml::sax::XFastAttributeList > & _xAttrList
                    ,const css::uno::Reference< css::beans::XPropertySet >& _xComponent);
        virtual ~OXMLCondPrtExpr() override;

        virtual void SAL_CALL characters( const OUString& rChars ) override;
        virtual void SAL_CALL endFastElement( sal_Int32 nElement ) override;
    };

} // namespace rptxml


#endif // INCLUDED_REPORTDESIGN_SOURCE_FILTER_XML_XMLCONDPRTEXPR_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
