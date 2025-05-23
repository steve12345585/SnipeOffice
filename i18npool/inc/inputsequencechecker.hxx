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
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/i18n/XExtendedInputSequenceChecker.hpp>

#include <utility>
#include <vector>
#include <optional>

namespace com::sun::star::uno { class XComponentContext; }

namespace i18npool {



class InputSequenceCheckerImpl : public cppu::WeakImplHelper
<
    css::i18n::XExtendedInputSequenceChecker,
    css::lang::XServiceInfo
>
{
public:
    InputSequenceCheckerImpl( const css::uno::Reference < css::uno::XComponentContext >& rxContext );
    InputSequenceCheckerImpl(const char *pServiceName);
    virtual ~InputSequenceCheckerImpl() override;

    virtual sal_Bool SAL_CALL checkInputSequence(const OUString& Text, sal_Int32 nStartPos,
        sal_Unicode inputChar, sal_Int16 inputCheckMode) override;

    virtual sal_Int32 SAL_CALL correctInputSequence(OUString& Text, sal_Int32 nStartPos,
        sal_Unicode inputChar, sal_Int16 inputCheckMode) override;

    //XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

private:
    const char *serviceName;

    struct lookupTableItem {
        lookupTableItem(const char* rLanguage, css::uno::Reference < css::i18n::XExtendedInputSequenceChecker > _xISC) :
            aLanguage(rLanguage), xISC(std::move(_xISC)) {}
        const char* aLanguage;
        css::uno::Reference < css::i18n::XExtendedInputSequenceChecker > xISC;
    };
    std::vector<lookupTableItem> lookupTable;
    std::optional<lookupTableItem> cachedItem;

    css::uno::Reference < css::uno::XComponentContext > m_xContext;

    /// @throws css::uno::RuntimeException
    css::uno::Reference< css::i18n::XExtendedInputSequenceChecker >& getInputSequenceChecker(char const * rLanguage);
    static char* getLanguageByScripType(sal_Unicode cChar, sal_Unicode nChar);
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
