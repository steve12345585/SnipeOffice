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

#include <libxml/tree.h>

#include <sal/types.h>

#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/xml/dom/XDocumentType.hpp>
#include <com/sun/star/xml/dom/XNodeList.hpp>
#include <com/sun/star/xml/dom/XNamedNodeMap.hpp>

#include <cppuhelper/implbase.hxx>
#include <node.hxx>

namespace DOM
{
    typedef ::cppu::ImplInheritanceHelper< CNode, css::xml::dom::XDocumentType >
        CDocumentType_Base;

    class CDocumentType
        : public CDocumentType_Base
    {
    private:
        friend class CDocument;

        xmlDtdPtr m_aDtdPtr;

        CDocumentType(CDocument const& rDocument, ::osl::Mutex const& rMutex,
                xmlDtdPtr const pDtd);

    public:
        /**
        A NamedNodeMap containing the general entities, both external and
        internal, declared in the DTD.
        */
        virtual css::uno::Reference< css::xml::dom::XNamedNodeMap > SAL_CALL getEntities() override;

        /**
        The internal subset as a string, or null if there is none.
        */
        virtual OUString SAL_CALL getInternalSubset() override;

        /**
        The name of DTD; i.e., the name immediately following the DOCTYPE
        keyword.
        */
        virtual OUString SAL_CALL getName() override;

        /**
        A NamedNodeMap containing the notations declared in the DTD.
        */
        virtual css::uno::Reference< css::xml::dom::XNamedNodeMap > SAL_CALL getNotations() override;

        /**
        The public identifier of the external subset.
        */
        virtual OUString SAL_CALL getPublicId() override;

        /**
        The system identifier of the external subset.
        */
        virtual OUString SAL_CALL getSystemId() override;

        // ---- resolve uno inheritance problems...
        // overrides for XNode base
        virtual OUString SAL_CALL getNodeName() override;
        virtual OUString SAL_CALL getNodeValue() override;
    // --- delegation for XNode base.
    virtual css::uno::Reference< css::xml::dom::XNode > SAL_CALL appendChild(const css::uno::Reference< css::xml::dom::XNode >& newChild) override
    {
        return CNode::appendChild(newChild);
    }
    virtual css::uno::Reference< css::xml::dom::XNode > SAL_CALL cloneNode(sal_Bool deep) override
    {
        return CNode::cloneNode(deep);
    }
    virtual css::uno::Reference< css::xml::dom::XNamedNodeMap > SAL_CALL getAttributes() override
    {
        return CNode::getAttributes();
    }
    virtual css::uno::Reference< css::xml::dom::XNodeList > SAL_CALL getChildNodes() override
    {
        return CNode::getChildNodes();
    }
    virtual css::uno::Reference< css::xml::dom::XNode > SAL_CALL getFirstChild() override
    {
        return CNode::getFirstChild();
    }
    virtual css::uno::Reference< css::xml::dom::XNode > SAL_CALL getLastChild() override
    {
        return CNode::getLastChild();
    }
    virtual OUString SAL_CALL getLocalName() override
    {
        return CNode::getLocalName();
    }
    virtual OUString SAL_CALL getNamespaceURI() override
    {
        return CNode::getNamespaceURI();
    }
    virtual css::uno::Reference< css::xml::dom::XNode > SAL_CALL getNextSibling() override
    {
        return CNode::getNextSibling();
    }
    virtual css::xml::dom::NodeType SAL_CALL getNodeType() override
    {
        return CNode::getNodeType();
    }
    virtual css::uno::Reference< css::xml::dom::XDocument > SAL_CALL getOwnerDocument() override
    {
        return CNode::getOwnerDocument();
    }
    virtual css::uno::Reference< css::xml::dom::XNode > SAL_CALL getParentNode() override
    {
        return CNode::getParentNode();
    }
    virtual OUString SAL_CALL getPrefix() override
    {
        return CNode::getPrefix();
    }
    virtual css::uno::Reference< css::xml::dom::XNode > SAL_CALL getPreviousSibling() override
    {
        return CNode::getPreviousSibling();
    }
    virtual sal_Bool SAL_CALL hasAttributes() override
    {
        return CNode::hasAttributes();
    }
    virtual sal_Bool SAL_CALL hasChildNodes() override
    {
        return CNode::hasChildNodes();
    }
    virtual css::uno::Reference< css::xml::dom::XNode > SAL_CALL insertBefore(
            const css::uno::Reference< css::xml::dom::XNode >& newChild, const css::uno::Reference< css::xml::dom::XNode >& refChild) override
    {
        return CNode::insertBefore(newChild, refChild);
    }
    virtual sal_Bool SAL_CALL isSupported(const OUString& feature, const OUString& ver) override
    {
        return CNode::isSupported(feature, ver);
    }
    virtual void SAL_CALL normalize() override
    {
        CNode::normalize();
    }
    virtual css::uno::Reference< css::xml::dom::XNode > SAL_CALL removeChild(const css::uno::Reference< css::xml::dom::XNode >& oldChild) override
    {
        return CNode::removeChild(oldChild);
    }
    virtual css::uno::Reference< css::xml::dom::XNode > SAL_CALL replaceChild(
            const css::uno::Reference< css::xml::dom::XNode >& newChild, const css::uno::Reference< css::xml::dom::XNode >& oldChild) override
    {
        return CNode::replaceChild(newChild, oldChild);
    }
    virtual void SAL_CALL setNodeValue(const OUString& nodeValue) override
    {
        return CNode::setNodeValue(nodeValue);
    }
    virtual void SAL_CALL setPrefix(const OUString& prefix) override
    {
        return CNode::setPrefix(prefix);
    }

    };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
