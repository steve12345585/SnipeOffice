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
#ifndef INCLUDED_REPORTDESIGN_SOURCE_FILTER_XML_XMLHELPER_HXX
#define INCLUDED_REPORTDESIGN_SOURCE_FILTER_XML_XMLHELPER_HXX

#include <xmloff/xmlprmap.hxx>
#include <xmloff/contextid.hxx>
#include <xmloff/controlpropertyhdl.hxx>
#include <com/sun/star/beans/XPropertySet.hpp>

#include <memory>

#define CTF_RPT_NUMBERFORMAT                    (XML_DB_CTF_START + 1)


class SvXMLStylesContext;
class SvXMLTokenMap;
namespace rptxml
{
    class OPropertyHandlerFactory : public ::xmloff::OControlPropertyHandlerFactory
    {
        OPropertyHandlerFactory(const OPropertyHandlerFactory&) = delete;
        void operator =(const OPropertyHandlerFactory&) = delete;
    public:
        OPropertyHandlerFactory();
        virtual ~OPropertyHandlerFactory() override;

        virtual const XMLPropertyHandler* GetPropertyHandler(sal_Int32 _nType) const override;
    };

    class OXMLHelper
    {
    public:
        static rtl::Reference < XMLPropertySetMapper > GetCellStylePropertyMap(bool _bOldFormat, bool bForExport);

        static const SvXMLEnumMapEntry<sal_Int16>* GetReportPrintOptions();
        static const SvXMLEnumMapEntry<sal_Int16>* GetForceNewPageOptions();
        static const SvXMLEnumMapEntry<sal_Int16>* GetKeepTogetherOptions();
        static const SvXMLEnumMapEntry<sal_Int32>* GetCommandTypeOptions();
        static const SvXMLEnumMapEntry<sal_Int16>* GetImageScaleOptions();

        static const XMLPropertyMapEntry* GetTableStyleProps();
        static const XMLPropertyMapEntry* GetColumnStyleProps();

        static const XMLPropertyMapEntry* GetRowStyleProps();

        static void copyStyleElements(const bool _bOld,const OUString& _sStyleName,const SvXMLStylesContext* _pAutoStyles,const css::uno::Reference< css::beans::XPropertySet>& _xProp);
        static css::uno::Reference< css::beans::XPropertySet> createBorderPropertySet();

        static std::unique_ptr<SvXMLTokenMap> GetReportElemTokenMap();
        static std::unique_ptr<SvXMLTokenMap> GetSubDocumentElemTokenMap();

    };

} // rptxml

#endif // INCLUDED_REPORTDESIGN_SOURCE_FILTER_XML_XMLHELPER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
