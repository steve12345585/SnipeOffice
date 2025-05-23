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

#include <dmapper/resourcemodel.hxx>

#include "OOXMLFastContextHandler.hxx"
#include "OOXMLFastContextHandlerTheme.hxx"

namespace writerfilter::ooxml {

enum class ResourceType {
    NoResource,
    Table,
    Stream,
    List,
    Integer,
    Properties,
    Hex,
    HexColor,
    String,
    Shape,
    Boolean,
    Value,
    XNote,
    TextTableCell,
    TextTableRow,
    TextTable,
    PropertyTable,
    Math,
    Any,
    EmuMeasure,
    TwipsMeasure_asSigned,
    TwipsMeasure_asZero,
    HpsMeasure,
    MeasurementOrPercent,
    CommentEx,
    Theme,
};

struct AttributeInfo
{
    Token_t m_nToken;
    ResourceType m_nResource;
    Id m_nRef;
};

class OOXMLFactory_ns : public virtual SvRefBase {
public:
    typedef tools::SvRef<OOXMLFactory_ns> Pointer_t;

    virtual void startAction(OOXMLFastContextHandler * pHandler);
    virtual void charactersAction(OOXMLFastContextHandler * pHandler, const OUString & rString);
    virtual void endAction(OOXMLFastContextHandler * pHandler);
    virtual void attributeAction(OOXMLFastContextHandler * pHandler, Token_t nToken, const OOXMLValue& pValue);

protected:
    virtual ~OOXMLFactory_ns() override;

public:
    virtual bool getListValue(Id nId, std::string_view aValue, sal_uInt32& rOutValue) = 0;
    virtual Id getResourceId(Id nDefine, sal_Int32 nToken) = 0;
    virtual const AttributeInfo* getAttributeInfoArray(Id nId) = 0;
    virtual bool getElementId(Id nDefine, Id nId, ResourceType& rOutResource, Id& rOutElement) = 0;
};

class OOXMLFactory
{
public:

    static css::uno::Reference< css::xml::sax::XFastContextHandler> createFastChildContext(OOXMLFastContextHandler * pHandler, Token_t Element);

    static css::uno::Reference< css::xml::sax::XFastContextHandler> createFastChildContextFromStart(OOXMLFastContextHandler * pHandler, Token_t Element);

    static void attributes(OOXMLFastContextHandler * pHandler, const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs);

    static void characters(OOXMLFastContextHandler * pHandler, const OUString & rString);

    static void startAction(OOXMLFastContextHandler * pHandler);
    static void endAction(OOXMLFastContextHandler * pHandler);

private:
    OOXMLFactory() = delete;
    static OOXMLFactory_ns::Pointer_t getFactoryForNamespace(Id id);

    static css::uno::Reference< css::xml::sax::XFastContextHandler> createFastChildContextFromFactory(OOXMLFastContextHandler * pHandler,
            const OOXMLFactory_ns::Pointer_t& pFactory, Token_t Element);
};

}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
