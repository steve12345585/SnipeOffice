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

#include <cppuhelper/implbase.hxx>
#include <com/sun/star/util/XTextSearch2.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>

#include <map>
#include <memory>
#include <mutex>

#include <unicode/regex.h>
#include <unicode/unistr.h>
#include <unicode/uversion.h>

namespace com::sun::star::i18n { class XBreakIterator; }
namespace com::sun::star::i18n { class XCharacterClassification; }
namespace com::sun::star::i18n { class XExtendedTransliteration; }
namespace com::sun::star::uno { class XComponentContext; }


class WLevDistance;
typedef ::std::map< sal_Unicode, sal_Int32 > TextSearchJumpTable;

class TextSearch: public cppu::WeakImplHelper
<
    css::util::XTextSearch2,
    css::lang::XServiceInfo
>
{
    std::mutex m_aMutex;
    css::uno::Reference < css::uno::XComponentContext > m_xContext;

    css::util::SearchOptions2 aSrchPara;
    OUString sSrchStr;
    OUString sSrchStr2;

    mutable css::uno::Reference< css::i18n::XCharacterClassification > xCharClass;

    css::uno::Reference< css::i18n::XExtendedTransliteration > xTranslit;
    css::uno::Reference< css::i18n::XExtendedTransliteration > xTranslit2;

    // define a function pointer for the different search methods
    typedef css::util::SearchResult
        (TextSearch::*FnSrch)( std::unique_lock<std::mutex>& rGuard, const OUString& searchStr,
                                sal_Int32 startPos, sal_Int32 endPos );

    FnSrch fnForward;
    FnSrch fnBackward;

    // to fix UX regression, U+0027 matches also U+2019 in non-regex search
    bool bSearchApostrophe;

    // Members and methods for the normal (Boyer-Moore) search
    std::unique_ptr<TextSearchJumpTable> pJumpTable;
    std::unique_ptr<TextSearchJumpTable> pJumpTable2;
    bool bIsForwardTab;
    bool bUsePrimarySrchStr;
    void MakeForwardTab();
    void MakeForwardTab2();
    void MakeBackwardTab();
    void MakeBackwardTab2();
    sal_Int32 GetDiff( const sal_Unicode ) const;
    /// @throws css::uno::RuntimeException
    css::util::SearchResult
        NSrchFrwrd( std::unique_lock<std::mutex>& rGuard, const OUString& searchStr,
                                sal_Int32 startPos, sal_Int32 endPos );
    /// @throws css::uno::RuntimeException
    css::util::SearchResult
        NSrchBkwrd( std::unique_lock<std::mutex>& rGuard, const OUString& searchStr,
                                sal_Int32 startPos, sal_Int32 endPos );

    // Members and methods for the regular expression search
    std::unique_ptr<icu::RegexMatcher> pRegexMatcher;
    /// @throws css::uno::RuntimeException
    css::util::SearchResult
        RESrchFrwrd( std::unique_lock<std::mutex>& rGuard, const OUString& searchStr,
                                sal_Int32 startPos, sal_Int32 endPos );
    /// @throws css::uno::RuntimeException
    css::util::SearchResult
        RESrchBkwrd( std::unique_lock<std::mutex>& rGuard, const OUString& searchStr,
                                sal_Int32 startPos, sal_Int32 endPos );
    void RESrchPrepare( const css::util::SearchOptions2&);

    // Members and methods for the "Weight Levenshtein-Distance" search
    int nLimit;
    std::unique_ptr<WLevDistance> pWLD;
    css::uno::Reference < css::i18n::XBreakIterator > xBreak;
    /// @throws css::uno::RuntimeException
    css::util::SearchResult
        ApproxSrchFrwrd( std::unique_lock<std::mutex>& rGuard, const OUString& searchStr,
                                sal_Int32 startPos, sal_Int32 endPos );
    /// @throws css::uno::RuntimeException
    css::util::SearchResult
        ApproxSrchBkwrd( std::unique_lock<std::mutex>& rGuard, const OUString& searchStr,
                                sal_Int32 startPos, sal_Int32 endPos );

    // Members and methods for the wildcard search
    OUString    maWildcardReversePattern;
    OUString    maWildcardReversePattern2;
    sal_uInt32  mcWildcardEscapeChar;
    bool        mbWildcardAllowSubstring;
    /// @throws css::uno::RuntimeException
    css::util::SearchResult
        WildcardSrchFrwrd( std::unique_lock<std::mutex>& rGuard, const OUString& searchStr,
                                sal_Int32 startPos, sal_Int32 endPos );
    /// @throws css::uno::RuntimeException
    css::util::SearchResult
        WildcardSrchBkwrd( std::unique_lock<std::mutex>& rGuard, const OUString& searchStr,
                                sal_Int32 startPos, sal_Int32 endPos );

    bool IsDelimiter( const OUString& rStr, sal_Int32 nPos ) const;

public:
    explicit TextSearch(
        const css::uno::Reference < css::uno::XComponentContext >& rxContext );

    virtual ~TextSearch() override;

    // XTextSearch
    virtual void SAL_CALL
        setOptions( const css::util::SearchOptions& options ) override;
    virtual css::util::SearchResult SAL_CALL
        searchForward( const OUString& searchStr,
                        sal_Int32 startPos, sal_Int32 endPos ) override;
    virtual css::util::SearchResult SAL_CALL
        searchBackward( const OUString& searchStr,
                        sal_Int32 startPos, sal_Int32 endPos ) override;

    // XTextSearch2
    virtual void SAL_CALL
        setOptions2( const css::util::SearchOptions2& options ) override;

    //XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
