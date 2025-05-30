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

#include <sot/storage.hxx>
#include <xmloff/xmlictxt.hxx>
#include <xmloff/xmlimp.hxx>
#include <editeng/svxacorr.hxx>

class SvXMLAutoCorrectImport : public SvXMLImport
{
protected:

    // This method is called after the namespace map has been updated, but
    // before a context for the current element has been pushed.
    virtual SvXMLImportContext *CreateFastContext( sal_Int32 Element,
        const css::uno::Reference< css::xml::sax::XFastAttributeList > & xAttrList ) override;

public:
    SvxAutocorrWordList     *pAutocorr_List;
    SvxAutoCorrect          &rAutoCorrect;
    css::uno::Reference < css::embed::XStorage > xStorage;

    SvXMLAutoCorrectImport(
        const css::uno::Reference< css::uno::XComponentContext > & xContext,
        SvxAutocorrWordList *pNewAutocorr_List,
        SvxAutoCorrect &rNewAutoCorrect,
        css::uno::Reference < css::embed::XStorage > xNewStorage);

    virtual ~SvXMLAutoCorrectImport() noexcept override;
};

class SvXMLWordListContext : public SvXMLImportContext
{
private:
    SvXMLAutoCorrectImport & rLocalRef;
public:
    SvXMLWordListContext ( SvXMLAutoCorrectImport& rImport );

    virtual css::uno::Reference<XFastContextHandler> SAL_CALL createFastChildContext( sal_Int32 Element,
        const css::uno::Reference< css::xml::sax::XFastAttributeList > & xAttrList ) override;

    virtual ~SvXMLWordListContext() override;
};

class SvXMLWordContext : public SvXMLImportContext
{
public:
    SvXMLWordContext ( SvXMLAutoCorrectImport& rImport,
        const css::uno::Reference< css::xml::sax::XFastAttributeList > & xAttrList );

    virtual ~SvXMLWordContext() override;
};


class SvXMLExceptionListImport : public SvXMLImport
{
protected:

    // This method is called after the namespace map has been updated, but
    // before a context for the current element has been pushed.
    virtual SvXMLImportContext *CreateFastContext( sal_Int32 Element, const css::uno::Reference<
            css::xml::sax::XFastAttributeList > & xAttrList ) override;
public:
    SvStringsISortDtor  &rList;

    SvXMLExceptionListImport(
        const css::uno::Reference< css::uno::XComponentContext > & xContext,
        SvStringsISortDtor & rNewList );

    virtual ~SvXMLExceptionListImport() noexcept override;
};

class SvXMLExceptionListContext : public SvXMLImportContext
{
private:
    SvXMLExceptionListImport & rLocalRef;
public:
    SvXMLExceptionListContext ( SvXMLExceptionListImport& rImport );

    virtual css::uno::Reference<XFastContextHandler> SAL_CALL createFastChildContext( sal_Int32 Element,
        const css::uno::Reference< css::xml::sax::XFastAttributeList > & xAttrList ) override;

    virtual ~SvXMLExceptionListContext() override;
};

class SvXMLExceptionContext : public SvXMLImportContext
{
public:
    SvXMLExceptionContext ( SvXMLExceptionListImport& rImport,
        const css::uno::Reference< css::xml::sax::XFastAttributeList > & xAttrList );

    virtual ~SvXMLExceptionContext() override;
};


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
