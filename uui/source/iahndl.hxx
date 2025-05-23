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

#include <rtl/ustring.hxx>

#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <com/sun/star/beans/Optional.hpp>
#include <com/sun/star/task/InteractionClassification.hpp>

#include <comphelper/errcode.hxx>
#include <unotools/resmgr.hxx>

#include <unordered_map>
#include <vector>

namespace com::sun::star {
    namespace awt {
        class XWindow;
    }
    namespace embed {
        class XStorage;
    }
    namespace lang {
        class XMultiServiceFactory;
    }
    namespace security {
        struct DocumentSignatureInformation;
    }
    namespace task {
        class XInteractionContinuation;
        class XInteractionHandler2;
        class XInteractionRequest;
    }
    namespace ucb {
        class NameClashResolveRequest;
    }
}

typedef std::unordered_map< OUString, OUString >    StringHashMap;

class UUIInteractionHelper
{
private:
    css::uno::Reference< css::uno::XComponentContext >      m_xContext;
    css::uno::Reference< css::awt::XWindow >                m_xWindowParam;
    OUString                                                m_aContextParam;
    StringHashMap                                           m_aTypedCustomHandlers;
    UUIInteractionHelper(UUIInteractionHelper const &) = delete;
    UUIInteractionHelper& operator =(UUIInteractionHelper const &) = delete;

public:
    explicit UUIInteractionHelper(
        css::uno::Reference< css::uno::XComponentContext > xContext);

    const css::uno::Reference<css::awt::XWindow> & GetParentWindow() const { return m_xWindowParam; }
    void SetParentWindow(const css::uno::Reference<css::awt::XWindow>& rWindow) { m_xWindowParam = rWindow; }

    void setContext(OUString const & context) { m_aContextParam = context; }

    ~UUIInteractionHelper();

    bool handleRequest( css::uno::Reference< css::task::XInteractionRequest > const & rRequest);

    css::beans::Optional< OUString >
    getStringFromRequest(
            css::uno::Reference< css::task::XInteractionRequest > const & rRequest);

    // Helper.
    static OUString
    replaceMessageWithArguments(
            const OUString& aMessage,
            std::vector< OUString > const & rArguments );

private:
    bool
    handleRequest_impl(
        css::uno::Reference< css::task::XInteractionRequest > const & rRequest,
        bool bObtainErrorStringOnly,
        bool & bHasErrorString,
        OUString & rErrorString);

    static void handlerequest(void* pHandleData, void* pInteractionHandler);

    css::beans::Optional< OUString >
    getStringFromRequest_impl(
        css::uno::Reference< css::task::XInteractionRequest > const & rRequest);

    static void getstringfromrequest(void* pHandleData, void* pInteractionHandler);

    const css::uno::Reference< css::awt::XWindow>&
    getParentXWindow() const;

    css::uno::Reference< css::task::XInteractionHandler2 >
    getInteractionHandler() const;

    bool    handleTypedHandlerImplementations(
                css::uno::Reference< css::task::XInteractionRequest > const &  rRequest
            );

    static bool
    isInformationalErrorMessageRequest(
        css::uno::Sequence<
            css::uno::Reference< css::task::XInteractionContinuation > > const &  rContinuations);

    bool
    handleInteractiveIOException(
        css::uno::Reference< css::task::XInteractionRequest > const & rRequest,
        bool bObtainErrorStringOnly,
        bool & bHasErrorString,
        OUString & rErrorString);

    bool
    handleAuthenticationRequest(
        css::uno::Reference< css::task::XInteractionRequest > const & rRequest);

    bool
    handleCertificateValidationRequest(
        css::uno::Reference< css::task::XInteractionRequest > const & rRequest);

    void
    handleNameClashResolveRequest(
        css::ucb::NameClashResolveRequest const & rRequest,
        css::uno::Sequence<
            css::uno::Reference< css::task::XInteractionContinuation > > const & rContinuations);

    bool
    handleMasterPasswordRequest(
        css::uno::Reference< css::task::XInteractionRequest > const & rRequest);

    bool
    handlePasswordRequest(
        css::uno::Reference< css::task::XInteractionRequest > const & rRequest);

    bool
    handleNoSuchFilterRequest(
        css::uno::Reference< css::task::XInteractionRequest > const & rRequest);

    bool
    handleFilterOptionsRequest(
        css::uno::Reference<
            css::task::XInteractionRequest > const & rRequest);

    void
    handleErrorHandlerRequest(
        css::task::InteractionClassification eClassification,
        ErrCode nErrorCode,
        std::vector< OUString > const & rArguments,
        css::uno::Sequence<
            css::uno::Reference< css::task::XInteractionContinuation > > const &  rContinuations,
        bool bObtainErrorStringOnly,
        bool & bHasErrorString,
        OUString & rErrorString);

    void
    handleGenericErrorRequest(
        ErrCodeMsg nErrorCode,
        css::uno::Sequence<
            css::uno::Reference< css::task::XInteractionContinuation > > const &  rContinuations,
        bool bObtainErrorStringOnly,
        bool & bHasErrorString,
        OUString & rErrorString);

    void
    handleMacroConfirmRequest(
        const OUString& aDocumentURL,
        const css::uno::Reference< css::embed::XStorage >& xZipStorage,
        const OUString& aDocumentVersion,
        const css::uno::Sequence< css::security::DocumentSignatureInformation >& aSignInfo,
        css::uno::Sequence<
            css::uno::Reference< css::task::XInteractionContinuation > > const &  rContinuations);

    void
    handleBrokenPackageRequest(
        std::vector< OUString > const & rArguments,
        css::uno::Sequence<
            css::uno::Reference< css::task::XInteractionContinuation > > const & rContinuations,
        bool bObtainErrorStringOnly,
        bool & bHasErrorString,
        OUString & rErrorString);

    bool handleLockedDocumentRequest(
        css::uno::Reference< css::task::XInteractionRequest > const & rRequest);

    bool handleChangedByOthersRequest(
        css::uno::Reference<
            css::task::XInteractionRequest > const & rRequest);

    bool handleLockFileProblemRequest(
        css::uno::Reference< css::task::XInteractionRequest > const & rRequest);

    bool handleReloadEditableRequest(
        css::uno::Reference<css::task::XInteractionRequest> const& rRequest);

    bool handleCustomRequest(
                const css::uno::Reference< css::task::XInteractionRequest >& i_rRequest,
                const OUString& i_rServiceName
            ) const;

    void
    handleAuthFallbackRequest(
            const OUString & instructions,
            const OUString & url,
            css::uno::Sequence< css::uno::Reference< css::task::XInteractionContinuation > > const & rContinuations );

    void
    handleLoadReadOnlyRequest(
        const OUString& sDocumentURL,
        css::uno::Sequence< css::uno::Reference< css::task::XInteractionContinuation > > const & rContinuations);
};

class ErrorResource
{
    const std::pair<TranslateId, ErrCode>* m_pStringArray;
    const std::locale& m_rResLocale;
public:
    explicit ErrorResource(const std::pair<TranslateId, ErrCode>* pStringArray, const std::locale& rResLocale)
        : m_pStringArray(pStringArray)
        , m_rResLocale(rResLocale)
    {
    }
    bool getString(ErrCode nErrorCode, OUString &rString) const;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
