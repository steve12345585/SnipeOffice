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

#include <xmloff/xmlictxt.hxx>
#include <com/sun/star/uno/Reference.h>


namespace com::sun::star {
    namespace xml::sax { class XAttributeList; }
    namespace beans { class XPropertySet; }
}

/**
 * Import index body.
 *
 * This class basically delegates all CreateChildContext() calls to
 * the text import and doesn't do much else.
 */
class XMLIndexBodyContext : public SvXMLImportContext
{
    bool bHasContent;

public:


    XMLIndexBodyContext( SvXMLImport& rImport );

    virtual ~XMLIndexBodyContext() override;

    /// return whether any content elements were encountered
    inline bool HasContent() const;

protected:

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext(
        sal_Int32 nElement,
        const css::uno::Reference< css::xml::sax::XFastAttributeList >& AttrList ) override;
};

inline bool XMLIndexBodyContext::HasContent() const
{
    return bHasContent;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
