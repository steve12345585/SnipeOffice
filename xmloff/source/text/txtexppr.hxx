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


#include <xmloff/xmlexppr.hxx>
#include "txtdrope.hxx"
#include <xmltabe.hxx>
#include <XMLTextColumnsExport.hxx>
#include <XMLBackgroundImageExport.hxx>
#include <xmloff/XMLComplexColorExport.hxx>

class SvXMLExport;
class XMLTextExportPropertySetMapper: public SvXMLExportPropertyMapper
{
    SvXMLExport& rExport;

    OUString sDropCharStyle;
    bool bDropWholeWord;

    void ContextFontFilter(
                bool bEnableFoFontFamily,
                XMLPropertyState *pFontNameState,
                XMLPropertyState *pFontFamilyNameState,
                XMLPropertyState *pFontStyleNameState,
                XMLPropertyState *pFontFamilyState,
                XMLPropertyState *pFontPitchState,
                XMLPropertyState *pFontCharsetState ) const;
    static void ContextFontHeightFilter(
                XMLPropertyState* pCharHeightState,
                XMLPropertyState* pCharPropHeightState,
                XMLPropertyState* pCharDiffHeightState );

private:
//  SvXMLUnitConverter& mrUnitConverter;
//  const Reference< xml::sax::XDocumentHandler > & mrHandler;
    XMLTextDropCapExport maDropCapExport;
    SvxXMLTabStopExport maTabStopExport;
    XMLTextColumnsExport maTextColumnsExport;
    XMLComplexColorExport maComplexColorExport;
    XMLBackgroundImageExport maBackgroundImageExport;

    /** Application-specific filter. By default do nothing. */
    virtual void ContextFilter(
            bool bEnableFoFontFamily,
            ::std::vector< XMLPropertyState >& rProperties,
            const css::uno::Reference< css::beans::XPropertySet >& rPropSet ) const override;
    const SvXMLExport& GetExport() const { return rExport; }

public:

    XMLTextExportPropertySetMapper(
            const rtl::Reference< XMLPropertySetMapper >& rMapper,
            SvXMLExport& rExt );
    virtual ~XMLTextExportPropertySetMapper() override;

    virtual void handleElementItem(
        SvXMLExport& rExport,
        const XMLPropertyState& rProperty,
        SvXmlExportFlags nFlags,
        const ::std::vector< XMLPropertyState > *pProperties,
        sal_uInt32 nIdx ) const override;

    virtual void handleSpecialItem(
        comphelper::AttributeList& rAttrList,
        const XMLPropertyState& rProperty,
        const SvXMLUnitConverter& rUnitConverter,
        const SvXMLNamespaceMap& rNamespaceMap,
        const ::std::vector< XMLPropertyState > *pProperties,
        sal_uInt32 nIdx ) const override;
};


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
