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

#include <xmloff/xmlexp.hxx>

class SwXMLTextBlocks;

class SwXMLBlockListExport final : public SvXMLExport
{
private:
    SwXMLTextBlocks &m_rBlockList;

public:
    SwXMLBlockListExport(
        const css::uno::Reference< css::uno::XComponentContext >& rContext,
        SwXMLTextBlocks & rBlocks,
        const OUString &rFileName,
        css::uno::Reference< css::xml::sax::XDocumentHandler> const &rHandler);

    ErrCode exportDoc( enum ::xmloff::token::XMLTokenEnum eClass = ::xmloff::token::XML_TOKEN_INVALID ) override;
    void ExportAutoStyles_() override {}
    void ExportMasterStyles_ () override {}
    void ExportContent_() override {}
};

class SwXMLTextBlockExport final : public SvXMLExport
{
private:
    SwXMLTextBlocks &m_rBlockList;

public:
    SwXMLTextBlockExport(
        const css::uno::Reference< css::uno::XComponentContext >& rContext,
        SwXMLTextBlocks & rBlocks,
        const OUString &rFileName,
        css::uno::Reference< css::xml::sax::XDocumentHandler> const &rHandler);

    ErrCode exportDoc(enum ::xmloff::token::XMLTokenEnum /*eClass*/) override { return ERRCODE_NONE; }
    void exportDoc(std::u16string_view rText);
    void ExportAutoStyles_() override {}
    void ExportMasterStyles_ () override {}
    void ExportContent_() override {}
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
