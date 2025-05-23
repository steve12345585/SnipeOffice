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

#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/drawing/ConnectorType.hpp>
#include <com/sun/star/drawing/CircleKind.hpp>
#include <xmloff/xmlnume.hxx>
#include <xmloff/maptype.hxx>
#include <xmloff/xmlement.hxx>
#include <xmloff/prhdlfac.hxx>
#include <xmloff/xmlprmap.hxx>
#include <xmloff/xmlexppr.hxx>

// entry list for graphic properties

extern const XMLPropertyMapEntry aXMLSDProperties[];

// entry list for presentation page properties

extern const XMLPropertyMapEntry aXMLSDPresPageProps[];

// enum maps for attributes

extern SvXMLEnumMapEntry<css::drawing::ConnectorType> const aXML_ConnectionKind_EnumMap[];
extern SvXMLEnumMapEntry<css::drawing::CircleKind> const aXML_CircleKind_EnumMap[];

/** contains the attribute to property mapping for a drawing layer table */
extern const XMLPropertyMapEntry aXMLTableShapeAttributes[];

// factory for own graphic properties

class SvXMLExport;
class SvXMLImport;

class XMLSdPropHdlFactory : public XMLPropertyHandlerFactory
{
private:
    css::uno::Reference< css::frame::XModel > mxModel;
    SvXMLExport* mpExport;
    SvXMLImport* mpImport;

public:
    XMLSdPropHdlFactory( css::uno::Reference< css::frame::XModel > xModel, SvXMLExport& rExport );
    XMLSdPropHdlFactory( css::uno::Reference< css::frame::XModel > xModel, SvXMLImport& rImport );
    virtual ~XMLSdPropHdlFactory() override;
    virtual const XMLPropertyHandler* GetPropertyHandler( sal_Int32 nType ) const override;
};

class XMLShapePropertySetMapper : public XMLPropertySetMapper
{
public:
    XMLShapePropertySetMapper(const rtl::Reference< XMLPropertyHandlerFactory >& rFactoryRef, bool bForExport);
    virtual ~XMLShapePropertySetMapper() override;
};

class XMLShapeExportPropertyMapper : public SvXMLExportPropertyMapper
{
private:
    SvxXMLNumRuleExport maNumRuleExp;
    bool mbIsInAutoStyles;

protected:
    virtual void ContextFilter(
        bool bEnableFoFontFamily,
        ::std::vector< XMLPropertyState >& rProperties,
        const css::uno::Reference< css::beans::XPropertySet >& rPropSet ) const override;
public:
    XMLShapeExportPropertyMapper( const rtl::Reference< XMLPropertySetMapper >& rMapper, SvXMLExport& rExport );
    virtual ~XMLShapeExportPropertyMapper() override;

    virtual void        handleElementItem(
                            SvXMLExport& rExport,
                            const XMLPropertyState& rProperty,
                            SvXmlExportFlags nFlags,
                            const ::std::vector< XMLPropertyState >* pProperties,
                            sal_uInt32 nIdx
                            ) const override;

    void SetAutoStyles( bool bIsInAutoStyles ) { mbIsInAutoStyles = bIsInAutoStyles; }

    virtual void handleSpecialItem(
            comphelper::AttributeList& rAttrList,
            const XMLPropertyState& rProperty,
            const SvXMLUnitConverter& rUnitConverter,
            const SvXMLNamespaceMap& rNamespaceMap,
            const ::std::vector< XMLPropertyState > *pProperties,
            sal_uInt32 nIdx ) const override;
};

class XMLPageExportPropertyMapper : public SvXMLExportPropertyMapper
{
private:
    SvXMLExport& mrExport;

protected:
    virtual void ContextFilter(
        bool bEnableFoFontFamily,
        ::std::vector< XMLPropertyState >& rProperties,
        const css::uno::Reference< css::beans::XPropertySet >& rPropSet ) const override;
public:
    XMLPageExportPropertyMapper( const rtl::Reference< XMLPropertySetMapper >& rMapper, SvXMLExport& rExport );
    virtual ~XMLPageExportPropertyMapper() override;

    virtual void        handleElementItem(
                            SvXMLExport& rExport,
                            const XMLPropertyState& rProperty,
                            SvXmlExportFlags nFlags,
                            const ::std::vector< XMLPropertyState >* pProperties,
                            sal_uInt32 nIdx
                            ) const override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
