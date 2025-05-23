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

#include "xpathobject.hxx"

#include <string.h>

#include <utility>

#include "../dom/document.hxx"
#include "nodelist.hxx"

using namespace css::uno;
using namespace css::xml::dom;
using namespace css::xml::xpath;

namespace XPath
{
    static XPathObjectType lcl_GetType(xmlXPathObjectPtr const pXPathObj)
    {
        switch (pXPathObj->type)
        {
            case XPATH_UNDEFINED:
                return XPathObjectType_XPATH_UNDEFINED;
            case XPATH_NODESET:
                return XPathObjectType_XPATH_NODESET;
            case XPATH_BOOLEAN:
                return XPathObjectType_XPATH_BOOLEAN;
            case XPATH_NUMBER:
                return XPathObjectType_XPATH_NUMBER;
            case XPATH_STRING:
                return XPathObjectType_XPATH_STRING;
#if LIBXML_VERSION < 21000 || defined(LIBXML_XPTR_LOCS_ENABLED)
            case XPATH_POINT:
                return XPathObjectType_XPATH_POINT;
            case XPATH_RANGE:
                return XPathObjectType_XPATH_RANGE;
            case XPATH_LOCATIONSET:
                return XPathObjectType_XPATH_LOCATIONSET;
#endif
            case XPATH_USERS:
                return XPathObjectType_XPATH_USERS;
            case XPATH_XSLT_TREE:
                return XPathObjectType_XPATH_XSLT_TREE;
            default:
                return XPathObjectType_XPATH_UNDEFINED;
        }
    }

    CXPathObject::CXPathObject(
            ::rtl::Reference<DOM::CDocument> pDocument,
            ::osl::Mutex & rMutex,
            std::shared_ptr<xmlXPathObject> const& pXPathObj)
        : m_pDocument(std::move(pDocument))
        , m_rMutex(rMutex)
        , m_pXPathObj(pXPathObj)
        , m_XPathObjectType(lcl_GetType(pXPathObj.get()))
    {
    }

    /**
        get object type
    */
    XPathObjectType CXPathObject::getObjectType()
    {
        return m_XPathObjectType;
    }

    /**
        get the nodes from a nodelist type object
    */
    Reference< XNodeList > SAL_CALL
    CXPathObject::getNodeList()
    {
        ::osl::MutexGuard const g(m_rMutex);

        Reference< XNodeList > const xRet(
            new CNodeList(m_pDocument, m_rMutex, m_pXPathObj));
        return xRet;
    }

     /**
        get value of a boolean object
     */
    sal_Bool SAL_CALL CXPathObject::getBoolean()
    {
        ::osl::MutexGuard const g(m_rMutex);

        return xmlXPathCastToBoolean(m_pXPathObj.get()) != 0;
    }

    /**
        get number as byte
    */
    sal_Int8 SAL_CALL CXPathObject::getByte()
    {
        ::osl::MutexGuard const g(m_rMutex);

        return static_cast<sal_Int8>(xmlXPathCastToNumber(m_pXPathObj.get()));
    }

    /**
        get number as short
    */
    sal_Int16 SAL_CALL CXPathObject::getShort()
    {
        ::osl::MutexGuard const g(m_rMutex);

        return static_cast<sal_Int16>(xmlXPathCastToNumber(m_pXPathObj.get()));
    }

    /**
        get number as long
    */
    sal_Int32 SAL_CALL CXPathObject::getLong()
    {
        ::osl::MutexGuard const g(m_rMutex);

        return static_cast<sal_Int32>(xmlXPathCastToNumber(m_pXPathObj.get()));
    }

    /**
        get number as hyper
    */
    sal_Int64 SAL_CALL CXPathObject::getHyper()
    {
        ::osl::MutexGuard const g(m_rMutex);

        return static_cast<sal_Int64>(xmlXPathCastToNumber(m_pXPathObj.get()));
    }

    /**
        get number as float
    */
    float SAL_CALL CXPathObject::getFloat()
    {
        ::osl::MutexGuard const g(m_rMutex);

        return static_cast<float>(xmlXPathCastToNumber(m_pXPathObj.get()));
    }

    /**
        get number as double
    */
    double SAL_CALL CXPathObject::getDouble()
    {
        ::osl::MutexGuard const g(m_rMutex);

        return  xmlXPathCastToNumber(m_pXPathObj.get());
    }

    /**
        get string value
    */
    OUString SAL_CALL CXPathObject::getString()
    {
        ::osl::MutexGuard const g(m_rMutex);

        std::shared_ptr<xmlChar const> str(
            xmlXPathCastToString(m_pXPathObj.get()), xmlFree);
        char const*const pS(reinterpret_cast<char const*>(str.get()));
        return OUString(pS, strlen(pS), RTL_TEXTENCODING_UTF8);
    }

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
