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

#include <utility>
#include <xmloff/xmlexp.hxx>
#include <xmloff/xmltoken.hxx>

class SfxMedium;
class SmNode;
class SmVerticalBraceNode;
namespace com::sun::star
{
namespace io
{
class XOutputStream;
}
namespace beans
{
class XPropertySet;
}
}

class SmXMLExportWrapper
{
    css::uno::Reference<css::frame::XModel> xModel;
    bool bFlat; //set true for export to flat .mml, set false for
        //export to a .sxm (or whatever) package

private:
    // Use customized entities
    bool m_bUseHTMLMLEntities;

public:
    explicit SmXMLExportWrapper(css::uno::Reference<css::frame::XModel> xRef)
        : xModel(std::move(xRef))
        , bFlat(true)
        , m_bUseHTMLMLEntities(false)
    {
    }

    bool Export(SfxMedium& rMedium);
    void SetFlat(bool bIn) { bFlat = bIn; }

    bool IsUseHTMLMLEntities() const { return m_bUseHTMLMLEntities; }
    void SetUseHTMLMLEntities(bool bUseHTMLMLEntities)
    {
        m_bUseHTMLMLEntities = bUseHTMLMLEntities;
    }

    bool WriteThroughComponent(const css::uno::Reference<css::io::XOutputStream>& xOutputStream,
                               const css::uno::Reference<css::lang::XComponent>& xComponent,
                               css::uno::Reference<css::uno::XComponentContext> const& rxContext,
                               css::uno::Reference<css::beans::XPropertySet> const& rPropSet,
                               const char* pComponentName);

    bool WriteThroughComponent(const css::uno::Reference<css::embed::XStorage>& xStor,
                               const css::uno::Reference<css::lang::XComponent>& xComponent,
                               const char* pStreamName,
                               css::uno::Reference<css::uno::XComponentContext> const& rxContext,
                               css::uno::Reference<css::beans::XPropertySet> const& rPropSet,
                               const char* pComponentName);
};

class SmXMLExport final : public SvXMLExport
{
    const SmNode* pTree;
    OUString aText;
    bool bSuccess;

    void ExportNodes(const SmNode* pNode, int nLevel);
    void ExportTable(const SmNode* pNode, int nLevel);
    void ExportLine(const SmNode* pNode, int nLevel);
    void ExportExpression(const SmNode* pNode, int nLevel, bool bNoMrowContainer = false);
    void ExportText(const SmNode* pNode);
    void ExportMath(const SmNode* pNode);
    void ExportBinaryHorizontal(const SmNode* pNode, int nLevel);
    void ExportUnaryHorizontal(const SmNode* pNode, int nLevel);
    void ExportBrace(const SmNode* pNode, int nLevel);
    void ExportBinaryVertical(const SmNode* pNode, int nLevel);
    void ExportBinaryDiagonal(const SmNode* pNode, int nLevel);
    void ExportSubSupScript(const SmNode* pNode, int nLevel);
    void ExportRoot(const SmNode* pNode, int nLevel);
    void ExportOperator(const SmNode* pNode, int nLevel);
    void ExportAttributes(const SmNode* pNode, int nLevel);
    void ExportFont(const SmNode* pNode, int nLevel);
    void ExportVerticalBrace(const SmVerticalBraceNode* pNode, int nLevel);
    void ExportMatrix(const SmNode* pNode, int nLevel);
    void ExportBlank(const SmNode* pNode);

public:
    SmXMLExport(const css::uno::Reference<css::uno::XComponentContext>& rContext,
                OUString const& implementationName, SvXMLExportFlags nExportFlags);

    void ExportAutoStyles_() override {}
    void ExportMasterStyles_() override {}
    void ExportContent_() override;
    ErrCode exportDoc(enum ::xmloff::token::XMLTokenEnum eClass
                      = ::xmloff::token::XML_TOKEN_INVALID) override;

    virtual void GetViewSettings(css::uno::Sequence<css::beans::PropertyValue>& aProps) override;
    virtual void
    GetConfigurationSettings(css::uno::Sequence<css::beans::PropertyValue>& aProps) override;

    bool GetSuccess() const { return bSuccess; }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
