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

#include <toolboxconfiguration.hxx>

#include <com/sun/star/xml/sax/XDocumentHandler.hpp>

#include <rtl/ustring.hxx>
#include <rtl/ref.hxx>
#include <comphelper/attributelist.hxx>
#include <cppuhelper/implbase.hxx>

#include <unordered_map>

namespace framework{

// Hash code function for using in all hash maps of follow implementation.

// workaround for incremental linking bugs in MSVC2015
class SAL_DLLPUBLIC_TEMPLATE OReadToolBoxDocumentHandler_Base : public cppu::WeakImplHelper< css::xml::sax::XDocumentHandler > {};

class OReadToolBoxDocumentHandler final : public OReadToolBoxDocumentHandler_Base
{
    public:
        enum ToolBox_XML_Entry
        {
            TB_ELEMENT_TOOLBAR,
            TB_ELEMENT_TOOLBARITEM,
            TB_ELEMENT_TOOLBARSPACE,
            TB_ELEMENT_TOOLBARBREAK,
            TB_ELEMENT_TOOLBARSEPARATOR,
            TB_ATTRIBUTE_TEXT,
            TB_ATTRIBUTE_URL,
            TB_ATTRIBUTE_VISIBLE,
            TB_ATTRIBUTE_STYLE,
            TB_ATTRIBUTE_UINAME,
            TB_XML_ENTRY_COUNT
        };

        enum ToolBox_XML_Namespace
        {
            TB_NS_TOOLBAR,
            TB_NS_XLINK
        };

        OReadToolBoxDocumentHandler( const css::uno::Reference< css::container::XIndexContainer >& rItemContainer );
        virtual ~OReadToolBoxDocumentHandler() override;

        // XDocumentHandler
        virtual void SAL_CALL startDocument() override;

        virtual void SAL_CALL endDocument() override;

        virtual void SAL_CALL startElement(
            const OUString& aName,
            const css::uno::Reference< css::xml::sax::XAttributeList > &xAttribs) override;

        virtual void SAL_CALL endElement(const OUString& aName) override;

        virtual void SAL_CALL characters(const OUString& aChars) override;

        virtual void SAL_CALL ignorableWhitespace(const OUString& aWhitespaces) override;

        virtual void SAL_CALL processingInstruction(const OUString& aTarget,
                                                    const OUString& aData) override;

        virtual void SAL_CALL setDocumentLocator(
            const css::uno::Reference< css::xml::sax::XLocator > &xLocator) override;

    private:
        OUString getErrorLineString();

        class ToolBoxHashMap : public std::unordered_map<OUString,
                                                         ToolBox_XML_Entry>
        {
        };

        bool                                                      m_bToolBarStartFound : 1;
        bool                                                      m_bToolBarItemStartFound : 1;
        bool                                                      m_bToolBarSpaceStartFound : 1;
        bool                                                      m_bToolBarBreakStartFound : 1;
        bool                                                      m_bToolBarSeparatorStartFound : 1;
        ToolBoxHashMap                                            m_aToolBoxMap;
        css::uno::Reference< css::container::XIndexContainer >    m_rItemContainer;
        css::uno::Reference< css::xml::sax::XLocator >            m_xLocator;

        OUString                                                  m_aType;
        OUString                                                  m_aLabel;
        OUString                                                  m_aStyle;
        OUString                                                  m_aIsVisible;
        OUString                                                  m_aCommandURL;
};

class OWriteToolBoxDocumentHandler final
{
    public:
            OWriteToolBoxDocumentHandler(
                const css::uno::Reference< css::container::XIndexAccess >& rItemAccess,
                css::uno::Reference< css::xml::sax::XDocumentHandler > const & rDocumentHandler );
            ~OWriteToolBoxDocumentHandler();

        /// @throws css::xml::sax::SAXException
        /// @throws css::uno::RuntimeException
        void WriteToolBoxDocument();

    private:
        /// @throws css::xml::sax::SAXException
        /// @throws css::uno::RuntimeException
        void WriteToolBoxItem( const OUString& aCommandURL, const OUString& aLabel, sal_Int16 nStyle, bool bVisible );

        /// @throws css::xml::sax::SAXException
        /// @throws css::uno::RuntimeException
        void WriteToolBoxSpace();

        /// @throws css::xml::sax::SAXException
        /// @throws css::uno::RuntimeException
        void WriteToolBoxBreak();

        /// @throws css::xml::sax::SAXException
        /// @throws css::uno::RuntimeException
        void WriteToolBoxSeparator();

        css::uno::Reference< css::xml::sax::XDocumentHandler > m_xWriteDocumentHandler;
        rtl::Reference< ::comphelper::AttributeList >          m_xEmptyList;
        css::uno::Reference< css::container::XIndexAccess >    m_rItemAccess;
        OUString                                               m_aXMLToolbarNS;
        OUString                                               m_aXMLXlinkNS;
        OUString                                               m_aAttributeURL;
};

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
