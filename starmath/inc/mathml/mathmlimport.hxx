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
#include <xmloff/xmlimp.hxx>
#include <comphelper/errcode.hxx>

#include <deque>

class SmNode;
class SfxMedium;
namespace com::sun::star
{
namespace beans
{
class XPropertySet;
}
}
class SmModel;

typedef std::deque<std::unique_ptr<SmNode>> SmNodeStack;

class SmXMLImportWrapper
{
    rtl::Reference<SmModel> m_xModel;

private:
    // Use customized entities
    bool m_bUseHTMLMLEntities;

public:
    explicit SmXMLImportWrapper(rtl::Reference<SmModel> xRef)
        : m_xModel(std::move(xRef))
        , m_bUseHTMLMLEntities(false)
    {
    }

    ErrCode Import(SfxMedium& rMedium);
    void useHTMLMLEntities(bool bUseHTMLMLEntities) { m_bUseHTMLMLEntities = bUseHTMLMLEntities; }

    static ErrCode
    ReadThroughComponent(const css::uno::Reference<css::io::XInputStream>& xInputStream,
                         const css::uno::Reference<css::lang::XComponent>& xModelComponent,
                         css::uno::Reference<css::uno::XComponentContext> const& rxContext,
                         css::uno::Reference<css::beans::XPropertySet> const& rPropSet,
                         const char* pFilterName, bool bEncrypted, bool bUseHTMLMLEntities);

    static ErrCode
    ReadThroughComponent(const css::uno::Reference<css::embed::XStorage>& xStorage,
                         const css::uno::Reference<css::lang::XComponent>& xModelComponent,
                         const char* pStreamName,
                         css::uno::Reference<css::uno::XComponentContext> const& rxContext,
                         css::uno::Reference<css::beans::XPropertySet> const& rPropSet,
                         const char* pFilterName, bool bUseHTMLMLEntities);
};

class SmXMLImport final : public SvXMLImport
{
    SmNodeStack aNodeStack;
    bool bSuccess;
    int nParseDepth;
    OUString aText;
    sal_Int16 mnSmSyntaxVersion;

public:
    SmXMLImport(const css::uno::Reference<css::uno::XComponentContext>& rContext,
                OUString const& implementationName, SvXMLImportFlags nImportFlags);
    virtual ~SmXMLImport() noexcept override;

    void SAL_CALL endDocument() override;

    SvXMLImportContext* CreateFastContext(
        sal_Int32 nElement,
        const css::uno::Reference<css::xml::sax::XFastAttributeList>& xAttrList) override;

    SmNodeStack& GetNodeStack() { return aNodeStack; }

    bool GetSuccess() const { return bSuccess; }
    [[nodiscard]] const OUString& GetText() const { return aText; }
    void SetText(const OUString& rStr) { aText = rStr; }

    virtual void
    SetViewSettings(const css::uno::Sequence<css::beans::PropertyValue>& aViewProps) override;
    virtual void SetConfigurationSettings(
        const css::uno::Sequence<css::beans::PropertyValue>& aViewProps) override;

    void IncParseDepth() { ++nParseDepth; }
    bool TooDeep() const { return nParseDepth >= 2048; }
    void DecParseDepth() { --nParseDepth; }
    void SetSmSyntaxVersion(sal_Int16 nSmSyntaxVersion) { mnSmSyntaxVersion = nSmSyntaxVersion; }
    sal_Int16 GetSmSyntaxVersion() const { return mnSmSyntaxVersion; }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
