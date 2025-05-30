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

#include <com/sun/star/uno/Reference.hxx>
#include <rtl/ustring.hxx>
#include <deque>
#include <string_view>
#include <vector>

namespace com::sun::star {
        namespace uno {
            class XComponentContext;
        }
        namespace xml::dom {
            class XDocument;
            class XNode;
        }
        namespace xml::xpath {
            class XXPathAPI;
        }
}

namespace dp_registry::backend {

class BackendDb
{
private:

    css::uno::Reference<css::xml::dom::XDocument> m_doc;
    css::uno::Reference<css::xml::xpath::XXPathAPI> m_xpathApi;

    BackendDb(BackendDb const &) = delete;
    BackendDb &  operator = (BackendDb const &) = delete;

protected:
    const css::uno::Reference<css::uno::XComponentContext> m_xContext;
    OUString m_urlDb;

protected:

    /* caller must make sure that only one thread accesses the function
     */
    css::uno::Reference<css::xml::dom::XDocument> const & getDocument();

    /* the namespace prefix is "reg" (without quotes)
     */
    css::uno::Reference<css::xml::xpath::XXPathAPI> const & getXPathAPI();
    void save();
    void removeElement(OUString const & sXPathExpression);

    css::uno::Reference<css::xml::dom::XNode> getKeyElement(
        std::u16string_view url);

    void writeSimpleList(
        std::deque< OUString> const & list,
        std::u16string_view sListTagName,
        std::u16string_view sMemberTagName,
        css::uno::Reference<css::xml::dom::XNode> const & xParent);

    void writeVectorOfPair(
        std::vector< std::pair< OUString, OUString > > const & vecPairs,
        std::u16string_view sVectorTagName,
        std::u16string_view sPairTagName,
        std::u16string_view sFirstTagName,
        std::u16string_view sSecondTagName,
        css::uno::Reference<css::xml::dom::XNode> const & xParent);

    void writeSimpleElement(
        std::u16string_view sElementName, OUString const & value,
        css::uno::Reference<css::xml::dom::XNode> const & xParent);

    css::uno::Reference<css::xml::dom::XNode> writeKeyElement(
        OUString const & url);

    OUString readSimpleElement(
        std::u16string_view sElementName,
        css::uno::Reference<css::xml::dom::XNode> const & xParent);

    std::vector< std::pair< OUString, OUString > >
    readVectorOfPair(
        css::uno::Reference<css::xml::dom::XNode> const & parent,
        std::u16string_view sListTagName,
        std::u16string_view sPairTagName,
        std::u16string_view sFirstTagName,
        std::u16string_view sSecondTagName);

    std::deque< OUString> readList(
        css::uno::Reference<css::xml::dom::XNode> const & parent,
        std::u16string_view sListTagName,
        std::u16string_view sMemberTagName);

    /* returns the values of one particularly child element of all key elements.
     */
    std::vector< OUString> getOneChildFromAllEntries(
        std::u16string_view sElementName);


    /*  returns the namespace which is to be written as xmlns attribute
        into the root element.
     */
    virtual OUString getDbNSName()=0;
    /* return the namespace prefix which is to be registered with the XPath API.

       The prefix can then be used in XPath expressions.
    */
    virtual OUString getNSPrefix()=0;
    /* returns the name of the root element without any namespace prefix.
     */
    virtual OUString getRootElementName()=0;
    /* returns the name of xml element for each entry
     */
    virtual OUString getKeyElementName()=0;

public:
    BackendDb(css::uno::Reference<css::uno::XComponentContext> const &  xContext,
              OUString const & url);
    virtual ~BackendDb() {};

    void removeEntry(std::u16string_view url);

    /* This is called to write the "revoked" attribute to the entry.
       This is done when XPackage::revokePackage is called.
    */
    void revokeEntry(std::u16string_view url);

    /* returns false if the entry does not exist yet.
     */
    bool activateEntry(std::u16string_view url);

    bool hasActiveEntry(std::u16string_view url);

};

class RegisteredDb: public BackendDb
{

public:
    RegisteredDb( css::uno::Reference<css::uno::XComponentContext> const &  xContext,
                  OUString const & url);


    void addEntry(OUString const & url);
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
