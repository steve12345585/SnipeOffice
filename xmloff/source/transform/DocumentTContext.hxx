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

#include "TransformerContext.hxx"

class XMLDocumentTransformerContext : public XMLTransformerContext
{
public:
    // A contexts constructor does anything that is required if an element
    // starts. Namespace processing has been done already.
    // Note that virtual methods cannot be used inside constructors. Use
    // StartElement instead if this is required.
    XMLDocumentTransformerContext(XMLTransformerBase& rTransformer, const OUString& rQName);

    // StartElement is called after a context has been constructed and
    // before an elements context is parsed. It may be used for actions that
    // require virtual methods. The default is to do nothing.
    virtual void
    StartElement(const css::uno::Reference<css::xml::sax::XAttributeList>& xAttrList) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
