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

#include <com/sun/star/awt/ColorStop.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <xmloff/xmlstyle.hxx>
#include <rtl/ustring.hxx>
#include <sal/types.h>

#include <vector>

// draw:gradient context

class XMLGradientStyleContext: public SvXMLStyleContext
{
private:
    css::uno::Any          maAny;
    OUString               maStrName;
    std::vector<css::awt::ColorStop> maColorStopVec;

public:

    XMLGradientStyleContext( SvXMLImport& rImport, sal_Int32 nElement,
                           const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList );
    virtual ~XMLGradientStyleContext() override;

    virtual css::uno::Reference<css::xml::sax::XFastContextHandler> SAL_CALL createFastChildContext(
        sal_Int32 nElement,
        const css::uno::Reference<css::xml::sax::XFastAttributeList>& AttrList) override;

    virtual void SAL_CALL endFastElement(sal_Int32 nElement) override;

    virtual bool IsTransient() const override;
};

// draw:hatch context

class XMLHatchStyleContext: public SvXMLStyleContext
{
private:
    css::uno::Any          maAny;
    OUString               maStrName;

public:

    XMLHatchStyleContext( SvXMLImport& rImport, sal_Int32 nElement,
                           const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList );
    virtual ~XMLHatchStyleContext() override;

    virtual void SAL_CALL endFastElement(sal_Int32 nElement) override;

    virtual bool IsTransient() const override;
};

// draw:fill-image context

class XMLBitmapStyleContext: public SvXMLStyleContext
{
private:
    css::uno::Any          maAny;
    OUString               maStrName;
    css::uno::Reference < css::io::XOutputStream > mxBase64Stream;

public:

    XMLBitmapStyleContext( SvXMLImport& rImport, sal_Int32 nElement,
                           const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList );
    virtual ~XMLBitmapStyleContext() override;

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext(
        sal_Int32 nElement, const css::uno::Reference< css::xml::sax::XFastAttributeList >& AttrList ) override;

    virtual void SAL_CALL endFastElement(sal_Int32 nElement) override;

    virtual bool IsTransient() const override;
};

// draw:transparency context

class XMLTransGradientStyleContext: public SvXMLStyleContext
{
private:
    css::uno::Any          maAny;
    OUString               maStrName;
    std::vector<css::awt::ColorStop> maColorStopVec; // Transparency is handled as color gray.

public:

    XMLTransGradientStyleContext( SvXMLImport& rImport, sal_Int32 nElement,
                           const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList );
    virtual ~XMLTransGradientStyleContext() override;

    virtual css::uno::Reference<css::xml::sax::XFastContextHandler> SAL_CALL createFastChildContext(
        sal_Int32 nElement,
        const css::uno::Reference<css::xml::sax::XFastAttributeList>& AttrList) override;

    virtual void SAL_CALL endFastElement(sal_Int32 nElement) override;

    virtual bool IsTransient() const override;
};

class XMLTransparencyStopContext: public SvXMLStyleContext
{
private:

public:

    XMLTransparencyStopContext(SvXMLImport& rImport, sal_Int32 nElement,
                           const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList,
                           std::vector<css::awt::ColorStop>& rColorStopVec);
    virtual ~XMLTransparencyStopContext() override;
};

// draw:marker context

class XMLMarkerStyleContext: public SvXMLStyleContext
{
private:
    css::uno::Any          maAny;
    OUString               maStrName;

public:

    XMLMarkerStyleContext( SvXMLImport& rImport, sal_Int32 nElement,
                           const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList );
    virtual ~XMLMarkerStyleContext() override;

    virtual void SAL_CALL endFastElement(sal_Int32 nElement) override;

    virtual bool IsTransient() const override;
};

// draw:marker context

class XMLDashStyleContext: public SvXMLStyleContext
{
private:
    css::uno::Any          maAny;
    OUString               maStrName;

public:

    XMLDashStyleContext( SvXMLImport& rImport, sal_Int32 nElement,
                           const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList );
    virtual ~XMLDashStyleContext() override;

    virtual void SAL_CALL endFastElement(sal_Int32 nElement) override;

    virtual bool IsTransient() const override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
