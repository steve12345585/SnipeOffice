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

#include <rtl/ustring.hxx>
#include <xmloff/xmlimp.hxx>
#include <xmloff/prstylei.hxx>
#include <xmloff/xmlimppr.hxx>

namespace dbaxml
{
    class ODBFilter;
    class OTableStylesContext;

    class OTableStyleContext : public XMLPropStyleContext
    {
        OUString             m_sDataStyleName;
        OUString             sPageStyle;
        OTableStylesContext*        pStyles;
        sal_Int32                   m_nNumberFormat;

        ODBFilter& GetOwnImport();

    protected:

        virtual void SetAttribute( sal_Int32 nElement,
                                const OUString& rValue ) override;

    public:


        OTableStyleContext( ODBFilter& rImport,
                OTableStylesContext& rStyles, XmlStyleFamily nFamily );

        virtual ~OTableStyleContext() override;

        virtual void FillPropertySet(const css::uno::Reference<
                    css::beans::XPropertySet > & rPropSet ) override;

        virtual void SetDefaults() override;

        void AddProperty(sal_Int16 nContextID, const css::uno::Any& aValue);
    };

    class OTableStylesContext : public SvXMLStylesContext
    {
        sal_Int32 m_nNumberFormatIndex;
        sal_Int32 m_nMasterPageNameIndex;
        bool bAutoStyles : 1;

        mutable std::unique_ptr < SvXMLImportPropertyMapper > m_xTableImpPropMapper;
        mutable std::unique_ptr < SvXMLImportPropertyMapper > m_xColumnImpPropMapper;
        mutable std::unique_ptr < SvXMLImportPropertyMapper > m_xCellImpPropMapper;

        ODBFilter& GetOwnImport();

    protected:

        // Create a style context.
        using SvXMLStylesContext::CreateStyleStyleChildContext;
        virtual SvXMLStyleContext *CreateStyleStyleChildContext(
                XmlStyleFamily nFamily,
                sal_Int32 nElement,
                const css::uno::Reference< css::xml::sax::XFastAttributeList > & xAttrList ) override;

    public:


        OTableStylesContext( SvXMLImport& rImport, bool bAutoStyles );
        virtual ~OTableStylesContext() override;

        virtual void SAL_CALL endFastElement(sal_Int32 nElement) override;

        virtual SvXMLImportPropertyMapper* GetImportPropertyMapper(
                            XmlStyleFamily nFamily ) const override;
        virtual OUString GetServiceName( XmlStyleFamily nFamily ) const override;

        sal_Int32 GetIndex(const sal_Int16 nContextID);
    };
} // dbaxml

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
