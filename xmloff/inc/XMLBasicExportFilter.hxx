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

#include <com/sun/star/xml/sax/XDocumentHandler.hpp>
#include <cppuhelper/implbase.hxx>


typedef ::cppu::WeakImplHelper<
    css::xml::sax::XDocumentHandler > XMLBasicExportFilter_BASE;

class XMLBasicExportFilter final : public XMLBasicExportFilter_BASE
{
private:
    css::uno::Reference< css::xml::sax::XDocumentHandler >    m_xHandler;

public:
    XMLBasicExportFilter(
        const css::uno::Reference< css::xml::sax::XDocumentHandler >& rxHandler );
    virtual ~XMLBasicExportFilter() override;

    // XDocumentHandler
    virtual void SAL_CALL startDocument() override;
    virtual void SAL_CALL endDocument() override;
    virtual void SAL_CALL startElement( const OUString& aName,
        const css::uno::Reference< css::xml::sax::XAttributeList >& xAttribs ) override;
    virtual void SAL_CALL endElement( const OUString& aName ) override;
    virtual void SAL_CALL characters( const OUString& aChars ) override;
    virtual void SAL_CALL ignorableWhitespace( const OUString& aWhitespaces ) override;
    virtual void SAL_CALL processingInstruction( const OUString& aTarget, const OUString& aData ) override;
    virtual void SAL_CALL setDocumentLocator( const css::uno::Reference< css::xml::sax::XLocator >& xLocator ) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
