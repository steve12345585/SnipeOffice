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
#include <com/sun/star/text/XTextContent.hpp>
#include <com/sun/star/text/XTextField.hpp>
#include <com/sun/star/text/XFormField.hpp>

#include <cppuhelper/implbase.hxx>

#include <svl/listener.hxx>
#include <sfx2/Metadatable.hxx>

#include <unobaseclass.hxx>
#include <IDocumentMarkAccess.hxx>

class SwDoc;
class SwXTextRange;

typedef ::cppu::ImplInheritanceHelper
<   ::sfx2::MetadatableMixin
,   css::lang::XServiceInfo
,   css::beans::XPropertySet
,   css::container::XNamed
,   css::text::XTextContent
> SwXBookmark_Base;

/// UNO API wrapper around an internal sw::mark::MarkBase.
class SAL_DLLPUBLIC_RTTI SwXBookmark
    : public SwXBookmark_Base
{

private:

    class Impl;
    ::sw::UnoImplPtr<Impl> m_pImpl;

protected:
    /// @throws css::lang::IllegalArgumentException
    /// @throws css::uno::RuntimeException
    void attachToRangeEx(
            const css::uno::Reference< css::text::XTextRange > & xTextRange,
            IDocumentMarkAccess::MarkType eType,
            bool isFieldmarkSeparatorAtStart = false);
    /// @throws css::lang::IllegalArgumentException
    /// @throws css::uno::RuntimeException
    virtual void attachToRange(
            const css::uno::Reference< css::text::XTextRange > & xTextRange);

    ::sw::mark::MarkBase* GetBookmark() const;

    IDocumentMarkAccess* GetIDocumentMarkAccess();

    SwDoc * GetDoc();

    void registerInMark( SwXBookmark& rXMark, ::sw::mark::MarkBase* const pMarkBase );

    virtual ~SwXBookmark() override;

    SwXBookmark(SwDoc *const pDoc);

    /// descriptor
    SwXBookmark();

public:

    static rtl::Reference<SwXBookmark>
        CreateXBookmark(SwDoc & rDoc, ::sw::mark::MarkBase * pBookmark);

    /// @return IMark for this, but only if it lives in pDoc
    static ::sw::mark::MarkBase const* GetBookmarkInDoc(SwDoc const*const pDoc,
            const css::uno::Reference<css::uno::XInterface> & xUT);

    // MetadatableMixin
    virtual ::sfx2::Metadatable* GetCoreObject() override;
    virtual css::uno::Reference< css::frame::XModel > GetModel() override;

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
            const css::uno::Reference<
                css::beans::XVetoableChangeListener >& xListener) override;

    // XNamed
    virtual OUString SAL_CALL getName() override;
    SW_DLLPUBLIC virtual void SAL_CALL setName(const OUString& rName) override;

    // XTextContent
    virtual void SAL_CALL attach(
            const css::uno::Reference< css::text::XTextRange > & xTextRange) override;
    virtual css::uno::Reference< css::text::XTextRange > SAL_CALL getAnchor() override;

};

class SwXFieldmarkParameters final
    : public ::cppu::WeakImplHelper< css::container::XNameContainer>
    , public SvtListener
{
    private:
        ::sw::mark::Fieldmark* m_pFieldmark;
        /// @throws css::uno::RuntimeException
        ::sw::mark::Fieldmark::parameter_map_t* getCoreParameters();
    public:
        SwXFieldmarkParameters(::sw::mark::Fieldmark* const pFieldmark)
            : m_pFieldmark(pFieldmark)
        {
            StartListening(pFieldmark->GetNotifier());
        }

        // XNameContainer
        virtual void SAL_CALL insertByName( const OUString& aName, const css::uno::Any& aElement ) override;
        virtual void SAL_CALL removeByName( const OUString& Name ) override;
        // XNameReplace
        virtual void SAL_CALL replaceByName( const OUString& aName, const css::uno::Any& aElement ) override;
        // XNameAccess
        virtual css::uno::Any SAL_CALL getByName( const OUString& aName ) override;
        virtual css::uno::Sequence< OUString > SAL_CALL getElementNames(  ) override;
        virtual sal_Bool SAL_CALL hasByName( const OUString& aName ) override;
        // XElementAccess
        virtual css::uno::Type SAL_CALL getElementType(  ) override;
        virtual sal_Bool SAL_CALL hasElements(  ) override;

        virtual void Notify( const SfxHint& rHint ) override;
};

typedef cppu::ImplInheritanceHelper< SwXBookmark,
        css::text::XFormField,
        css::text::XTextField
    > SwXFieldmark_Base;

/// UNO wrapper around an sw::mark::Fieldmark.
class SW_DLLPUBLIC SwXFieldmark final
    : public SwXFieldmark_Base
{
    ::sw::mark::CheckboxFieldmark* getCheckboxFieldmark();
    bool const m_bReplacementObject;
    bool m_isFieldmarkSeparatorAtStart = false;

    rtl::Reference<SwXTextRange>
        GetCommand(::sw::mark::Fieldmark const& rMark);
    rtl::Reference<SwXTextRange>
        GetResult(::sw::mark::Fieldmark const& rMark);

    SwXFieldmark(bool isReplacementObject, SwDoc* pDoc);

    // workaround MSVC compiler
    SwXFieldmark(const SwXFieldmark&) = delete;
    SwXFieldmark(SwXFieldmark&&) = delete;

public:
    static rtl::Reference<SwXFieldmark>
        CreateXFieldmark(SwDoc & rDoc, ::sw::mark::MarkBase * pMark,
                bool isReplacementObject = false);

    virtual void attachToRange(
            const css::uno::Reference<css::text::XTextRange > & xTextRange) override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual css::uno::Sequence<OUString> SAL_CALL
        getSupportedServiceNames() override;

    // XPropertySet
    virtual css::uno::Reference<css::beans::XPropertySetInfo> SAL_CALL
        getPropertySetInfo() override;
    virtual void SAL_CALL setPropertyValue(
            const OUString& rPropertyName,
            const css::uno::Any& rValue) override;
    virtual css::uno::Any SAL_CALL getPropertyValue(
            const OUString& rPropertyName) override;

    // XComponent
    virtual void SAL_CALL dispose() override;
    virtual void SAL_CALL addEventListener(
            const css::uno::Reference<css::lang::XEventListener> & xListener) override;
    virtual void SAL_CALL removeEventListener(
            const css::uno::Reference<css::lang::XEventListener> & xListener) override;

    // XTextContent
    virtual void SAL_CALL attach(
            const css::uno::Reference<css::text::XTextRange> & xTextRange) override;
    virtual css::uno::Reference<css::text::XTextRange> SAL_CALL getAnchor() override;

    // XTextField
    virtual OUString SAL_CALL getPresentation(sal_Bool bShowCommand) override;

    // XFormField
    virtual OUString SAL_CALL getFieldType() override;
    virtual void SAL_CALL setFieldType(const OUString& description) override;
    virtual css::uno::Reference<css::container::XNameContainer> SAL_CALL getParameters() override;

};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
