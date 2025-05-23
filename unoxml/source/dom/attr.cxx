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

#include "attr.hxx"

#include <string.h>

#include <memory>
#include <libxml/entities.h>

#include <osl/diagnose.h>
#include <sal/log.hxx>

#include <com/sun/star/xml/dom/events/XMutationEvent.hpp>

#include "document.hxx"

using namespace css::uno;
using namespace css::xml::dom;
using namespace css::xml::dom::events;

namespace DOM
{
    CAttr::CAttr(CDocument const& rDocument, ::osl::Mutex const& rMutex,
            xmlAttrPtr const pAttr)
        : CAttr_Base(rDocument, rMutex,
                NodeType_ATTRIBUTE_NODE, reinterpret_cast<xmlNodePtr>(pAttr))
        , m_aAttrPtr(pAttr)
    {
    }

    xmlNsPtr CAttr::GetNamespace(xmlNodePtr const pNode)
    {
        if (!m_oNamespace)
        {
            return nullptr;
        }
        xmlChar const*const pUri(reinterpret_cast<xmlChar const*>(
                m_oNamespace->first.getStr()));
        xmlChar const*const pPrefix(reinterpret_cast<xmlChar const*>(
                m_oNamespace->second.getStr()));
        xmlNsPtr pNs = xmlSearchNs(pNode->doc, pNode, pPrefix);
        if (pNs && (0 != xmlStrcmp(pNs->href, pUri))) {
            return pNs;
        }
        pNs = xmlNewNs(pNode, pUri, pPrefix);
        if (pNs) {
            return pNs;
        }
        pNs = xmlSearchNsByHref(pNode->doc, pNode, pUri);
        // if (!pNs) hmm... now what? throw?
        if (!pNs) {
            SAL_WARN("unoxml", "CAttr: cannot create namespace");
        }
        return pNs;
    }

    bool CAttr::IsChildTypeAllowed(NodeType const nodeType, NodeType const*const)
    {
        switch (nodeType) {
            case NodeType_TEXT_NODE:
            case NodeType_ENTITY_REFERENCE_NODE:
                return true;
            default:
                return false;
        }
    }

    OUString SAL_CALL CAttr::getNodeName()
    {
        return getName();
    }
    OUString SAL_CALL CAttr::getNodeValue()
    {
        return getValue();
    }
    OUString SAL_CALL CAttr::getLocalName()
    {
        return getName();
    }


    /**
    Returns the name of this attribute.
    */
    OUString SAL_CALL CAttr::getName()
    {
        ::osl::MutexGuard const g(m_rMutex);

        if ((nullptr == m_aNodePtr) || (nullptr == m_aAttrPtr)) {
            return OUString();
        }
        OUString const aName(reinterpret_cast<char const *>(m_aAttrPtr->name),
                strlen(reinterpret_cast<char const *>(m_aAttrPtr->name)), RTL_TEXTENCODING_UTF8);
        return aName;
    }

    /**
    The Element node this attribute is attached to or null if this
    attribute is not in use.
    */
    Reference< XElement > SAL_CALL CAttr::getOwnerElement()
    {
        ::osl::MutexGuard const g(m_rMutex);

        if ((nullptr == m_aNodePtr) || (nullptr == m_aAttrPtr)) {
            return nullptr;
        }
        if (nullptr == m_aAttrPtr->parent) {
            return nullptr;
        }
        Reference< XElement > const xRet(
            static_cast< XNode* >(GetOwnerDocument().GetCNode(
                    m_aAttrPtr->parent).get()),
            UNO_QUERY_THROW);
        return xRet;
    }

    /**
    If this attribute was explicitly given a value in the original
    document, this is true; otherwise, it is false.
    */
    sal_Bool SAL_CALL CAttr::getSpecified()
    {
        // FIXME if this DOM implementation supported DTDs it would need
        // to check that this attribute is not default or something
        return true;
    }

    /**
    On retrieval, the value of the attribute is returned as a string.
    */
    OUString SAL_CALL CAttr::getValue()
    {
        ::osl::MutexGuard const g(m_rMutex);

        if ((nullptr == m_aNodePtr) || (nullptr == m_aAttrPtr)) {
            return OUString();
        }
        if (nullptr == m_aAttrPtr->children) {
            return OUString();
        }
        char const*const pContent(reinterpret_cast<char const*>(m_aAttrPtr->children->content));
        return OUString(pContent, strlen(pContent), RTL_TEXTENCODING_UTF8);
    }

    /**
    Sets the value of the attribute from a string.
    */
    void SAL_CALL CAttr::setValue(const OUString& value)
    {
        ::osl::ClearableMutexGuard guard(m_rMutex);

        if ((nullptr == m_aNodePtr) || (nullptr == m_aAttrPtr)) {
            return;
        }

        // remember old value (for mutation event)
        OUString sOldValue = getValue();

        OString o1 = OUStringToOString(value, RTL_TEXTENCODING_UTF8);
        xmlChar const * pValue = reinterpret_cast<xmlChar const *>(o1.getStr());
        // this does not work if the attribute was created anew
        // xmlNodePtr pNode = m_aAttrPtr->parent;
        // xmlSetProp(pNode, m_aAttrPtr->name, pValue);
        std::shared_ptr<xmlChar const> const buffer(
                xmlEncodeEntitiesReentrant(m_aAttrPtr->doc, pValue), xmlFree);
        xmlFreeNodeList(m_aAttrPtr->children);
        m_aAttrPtr->children =
            xmlStringGetNodeList(m_aAttrPtr->doc, buffer.get());
        xmlNodePtr tmp = m_aAttrPtr->children;
        while (tmp != nullptr) {
            tmp->parent = m_aNodePtr;
            tmp->doc = m_aAttrPtr->doc;
            if (tmp->next == nullptr)
                m_aNodePtr->last = tmp;
            tmp = tmp->next;
        }

        // dispatch DOM events to signal change in attribute value
        // dispatch DomAttrModified + DOMSubtreeModified
        OUString sEventName( u"DOMAttrModified"_ustr );
        Reference< XDocumentEvent > docevent(getOwnerDocument(), UNO_QUERY);
        Reference< XMutationEvent > event(docevent->createEvent(sEventName),UNO_QUERY);
        event->initMutationEvent(
                sEventName, true, false,
                Reference<XNode>( static_cast<XAttr*>( this ) ),
                sOldValue, value, getName(), AttrChangeType_MODIFICATION );

        guard.clear(); // release mutex before calling event handlers

        dispatchEvent(event);
        dispatchSubtreeModified();
    }

    void SAL_CALL CAttr::setPrefix(const OUString& prefix)
    {
        ::osl::MutexGuard const g(m_rMutex);

        if (!m_aNodePtr) { return; }

        if (m_oNamespace)
        {
            OSL_ASSERT(!m_aNodePtr->parent);
            m_oNamespace->second =
                OUStringToOString(prefix, RTL_TEXTENCODING_UTF8);
        }
        else
        {
            CNode::setPrefix(prefix);
        }
    }

    OUString SAL_CALL CAttr::getPrefix()
    {
        ::osl::MutexGuard const g(m_rMutex);

        if (!m_aNodePtr) { return OUString(); }

        if (m_oNamespace)
        {
            OSL_ASSERT(!m_aNodePtr->parent);
            OUString const ret(OStringToOUString(
                        m_oNamespace->second, RTL_TEXTENCODING_UTF8));
            return ret;
        }
        else
        {
            return CNode::getPrefix();
        }
    }

    OUString SAL_CALL CAttr::getNamespaceURI()
    {
        ::osl::MutexGuard const g(m_rMutex);

        if (!m_aNodePtr) { return OUString(); }

        if (m_oNamespace)
        {
            OSL_ASSERT(!m_aNodePtr->parent);
            OUString const ret(OStringToOUString(
                        m_oNamespace->first, RTL_TEXTENCODING_UTF8));
            return ret;
        }
        else
        {
            return CNode::getNamespaceURI();
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
