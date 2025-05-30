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

#ifndef INCLUDED_SW_SOURCE_FILTER_XML_XMLITEM_HXX
#define INCLUDED_SW_SOURCE_FILTER_XML_XMLITEM_HXX

#include <com/sun/star/xml/sax/XFastAttributeList.hpp>
#include <svl/itemset.hxx>
#include <xmloff/xmlictxt.hxx>

class SfxItemSet;
class SvXMLImportItemMapper;
class SvXMLUnitConverter;
struct SvXMLItemMapEntry;
class SwXMLBrushItemImportContext;

class SwXMLItemSetContext final : public SvXMLImportContext
{
    SfxItemSet                  &m_rItemSet;
    const SvXMLImportItemMapper &m_rIMapper;
    const SvXMLUnitConverter    &m_rUnitConv;
    rtl::Reference<SwXMLBrushItemImportContext> m_xBackground;

public:

    SwXMLItemSetContext( SvXMLImport& rImport, sal_Int32 nElement,
                         const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList,
                         SfxItemSet&  rItemSet,
                         SvXMLImportItemMapper& rIMap,
                         const SvXMLUnitConverter& rUnitConv );

    virtual ~SwXMLItemSetContext() override;

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext(
        sal_Int32 nElement, const css::uno::Reference< css::xml::sax::XFastAttributeList >& AttrList ) override;

private:
    // This method is called from this instance implementation of
    // createFastChildContext if the element matches an entry in the
    // SvXMLImportItemMapper with the mid flag MID_SW_FLAG_ELEMENT_ITEM_IMPORT
    SvXMLImportContextRef createFastChildContext( sal_Int32 nElement,
                                   const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList,
                                   const SvXMLItemMapEntry& rEntry );
};

#endif // INCLUDED_SW_SOURCE_FILTER_XML_XMLITEM_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
