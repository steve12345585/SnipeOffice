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

#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XNamed.hpp>
#include <com/sun/star/util/XRefreshable.hpp>
#include <com/sun/star/text/XDocumentIndexMark.hpp>
#include <com/sun/star/text/XDocumentIndex.hpp>

#include <cppuhelper/implbase.hxx>

#include <sfx2/Metadatable.hxx>

#include <toxe.hxx>
#include <unobaseclass.hxx>
#include "unosection.hxx"
#include <swdllapi.h>

class SwDoc;
class SwTOXBaseSection;
class SwTOXMark;
class SwTOXType;

typedef ::cppu::ImplInheritanceHelper
<   SwXSection
,   css::util::XRefreshable
,   css::text::XDocumentIndex
> SwXDocumentIndex_Base;

class SW_DLLPUBLIC SwXDocumentIndex final
    : public SwXDocumentIndex_Base
{

private:

    class StyleAccess_Impl;
    class TokenAccess_Impl;

    class Impl;
    ::sw::UnoImplPtr<Impl> m_pImpl;

    virtual ~SwXDocumentIndex() override;

    SwXDocumentIndex(SwTOXBaseSection &, SwDoc &);

    /// descriptor
    SwXDocumentIndex(const TOXTypes eToxType, SwDoc& rDoc);

public:

    static rtl::Reference<SwXDocumentIndex>
        CreateXDocumentIndex(SwDoc & rDoc, SwTOXBaseSection * pSection,
                TOXTypes eTypes = TOX_INDEX);

    // MetadatableMixin
    virtual ::sfx2::Metadatable* GetCoreObject() override;
    virtual css::uno::Reference< css::frame::XModel >
        GetModel() override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(
            const OUString& rServiceName) override;
    virtual css::uno::Sequence< OUString > SAL_CALL
        getSupportedServiceNames() override;

    // XComponent
    virtual void SAL_CALL dispose() override;
    virtual void SAL_CALL addEventListener(
            const css::uno::Reference< css::lang::XEventListener > & xListener) override;
    virtual void SAL_CALL removeEventListener(
            const css::uno::Reference< css::lang::XEventListener > & xListener) override;

    // XPropertySet
    virtual css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL
        getPropertySetInfo() override;
    virtual void SAL_CALL setPropertyValue(
            const OUString& rPropertyName,
            const css::uno::Any& rValue) override;
    virtual css::uno::Any SAL_CALL getPropertyValue(
            const OUString& rPropertyName) override;
    virtual void SAL_CALL addPropertyChangeListener(
            const OUString& rPropertyName,
            const css::uno::Reference< css::beans::XPropertyChangeListener >& xListener) override;
    virtual void SAL_CALL removePropertyChangeListener(
            const OUString& rPropertyName,
            const css::uno::Reference< css::beans::XPropertyChangeListener >& xListener) override;
    virtual void SAL_CALL addVetoableChangeListener(
            const OUString& rPropertyName,
            const css::uno::Reference< css::beans::XVetoableChangeListener >& xListener) override;
    virtual void SAL_CALL removeVetoableChangeListener(
            const OUString& rPropertyName,
            const css::uno::Reference< css::beans::XVetoableChangeListener >& xListener) override;

    // XNamed
    virtual OUString SAL_CALL getName() override;
    virtual void SAL_CALL setName(const OUString& rName) override;

    // XRefreshable
    virtual void SAL_CALL refresh() override;
    virtual void SAL_CALL addRefreshListener(
            const css::uno::Reference< css::util::XRefreshListener>& xListener) override;
    virtual void SAL_CALL removeRefreshListener(
            const css::uno::Reference< css::util::XRefreshListener>& xListener) override;

    // XTextContent
    virtual void SAL_CALL attach(
            const css::uno::Reference< css::text::XTextRange > & xTextRange) override;
    virtual css::uno::Reference< css::text::XTextRange > SAL_CALL getAnchor() override;

    // XDocumentIndex
    virtual OUString SAL_CALL getServiceName() override;
    virtual void SAL_CALL update() override;

};

typedef ::cppu::WeakImplHelper
<   css::lang::XServiceInfo
,   css::beans::XPropertySet
,   css::text::XDocumentIndexMark
> SwXDocumentIndexMark_Base;

class SwXDocumentIndexMark final
    : public SwXDocumentIndexMark_Base
{

private:

    class Impl;
    ::sw::UnoImplPtr<Impl> m_pImpl;

    virtual ~SwXDocumentIndexMark() override;

    SwXDocumentIndexMark(SwDoc & rDoc,
                const SwTOXType & rType, const SwTOXMark & rMark);

    /// descriptor
    SwXDocumentIndexMark(const TOXTypes eToxType);

public:

    static rtl::Reference<SwXDocumentIndexMark>
        CreateXDocumentIndexMark(SwDoc & rDoc,
            SwTOXMark * pMark, TOXTypes eType = TOX_INDEX);

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(
            const OUString& rServiceName) override;
    virtual css::uno::Sequence< OUString > SAL_CALL
        getSupportedServiceNames() override;

    // XComponent
    virtual void SAL_CALL dispose() override;
    virtual void SAL_CALL addEventListener(
            const css::uno::Reference< css::lang::XEventListener > & xListener) override;
    virtual void SAL_CALL removeEventListener(
            const css::uno::Reference< css::lang::XEventListener > & xListener) override;

    // XPropertySet
    virtual css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL
        getPropertySetInfo() override;
    virtual void SAL_CALL setPropertyValue(
            const OUString& rPropertyName,
            const css::uno::Any& rValue) override;
    virtual css::uno::Any SAL_CALL getPropertyValue(
            const OUString& rPropertyName) override;
    virtual void SAL_CALL addPropertyChangeListener(
            const OUString& rPropertyName,
            const css::uno::Reference< css::beans::XPropertyChangeListener >& xListener) override;
    virtual void SAL_CALL removePropertyChangeListener(
            const OUString& rPropertyName,
            const css::uno::Reference<css::beans::XPropertyChangeListener >& xListener) override;
    virtual void SAL_CALL addVetoableChangeListener(
            const OUString& rPropertyName,
            const css::uno::Reference< css::beans::XVetoableChangeListener >& xListener) override;
    virtual void SAL_CALL removeVetoableChangeListener(
            const OUString& rPropertyName,
            const css::uno::Reference< css::beans::XVetoableChangeListener >& xListener) override;

    // XTextContent
    virtual void SAL_CALL attach(
            const css::uno::Reference< css::text::XTextRange > & xTextRange) override;
    virtual css::uno::Reference< css::text::XTextRange > SAL_CALL getAnchor() override;

    // XDocumentIndexMark
    virtual OUString SAL_CALL getMarkEntry() override;
    virtual void SAL_CALL setMarkEntry(const OUString& rIndexEntry) override;

    // called when the associated SwTOXMark is deleted
    void OnSwTOXMarkDeleted();

};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
