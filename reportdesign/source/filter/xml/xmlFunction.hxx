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
#ifndef INCLUDED_REPORTDESIGN_SOURCE_FILTER_XML_XMLFUNCTION_HXX
#define INCLUDED_REPORTDESIGN_SOURCE_FILTER_XML_XMLFUNCTION_HXX

#include <xmloff/xmlictxt.hxx>
#include <com/sun/star/report/XFunctionsSupplier.hpp>
#include <com/sun/star/report/XFunctions.hpp>


namespace rptxml
{
    class ORptFilter;
    class OXMLFunction final : public SvXMLImportContext
    {
        css::uno::Reference< css::report::XFunctions >    m_xFunctions;
        css::uno::Reference< css::report::XFunction >     m_xFunction;
        bool                                              m_bAddToReport;

        ORptFilter& GetOwnImport();

        OXMLFunction(const OXMLFunction&);
        OXMLFunction& operator =(const OXMLFunction&);
    public:

        OXMLFunction( ORptFilter& rImport
                    ,const css::uno::Reference< css::xml::sax::XFastAttributeList > & xAttrList
                    ,const css::uno::Reference< css::report::XFunctionsSupplier >&    _xFunctions
                    ,bool _bAddToReport = false
                    );
        virtual ~OXMLFunction() override;

        virtual void SAL_CALL endFastElement(sal_Int32 nElement) override;
    };

} // namespace rptxml


#endif // RPT_XMLFunction_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
