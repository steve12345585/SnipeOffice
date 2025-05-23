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

#include <com/sun/star/i18n/XCharacterClassification.hpp>
#include <cppuhelper/implbase.hxx>
#include <utility>
#include <vector>
#include <optional>
#include <com/sun/star/lang/XServiceInfo.hpp>

namespace com::sun::star::uno { class XComponentContext; }

namespace i18npool {

class CharacterClassificationImpl final : public cppu::WeakImplHelper
<
    css::i18n::XCharacterClassification,
    css::lang::XServiceInfo
>
{
public:

    CharacterClassificationImpl( const css::uno::Reference < css::uno::XComponentContext >& rxContext );
    virtual ~CharacterClassificationImpl() override;

    virtual OUString SAL_CALL toUpper( const OUString& Text,
        sal_Int32 nPos, sal_Int32 nCount, const css::lang::Locale& rLocale ) override;
    virtual OUString SAL_CALL toLower( const OUString& Text,
        sal_Int32 nPos, sal_Int32 nCount, const css::lang::Locale& rLocale ) override;
    virtual OUString SAL_CALL toTitle( const OUString& Text, sal_Int32 nPos,
        sal_Int32 nCount, const css::lang::Locale& rLocale ) override;
    virtual sal_Int16 SAL_CALL getType( const OUString& Text, sal_Int32 nPos ) override;
    virtual sal_Int16 SAL_CALL getCharacterDirection( const OUString& Text, sal_Int32 nPos ) override;
    virtual sal_Int16 SAL_CALL getScript( const OUString& Text, sal_Int32 nPos ) override;
    virtual sal_Int32 SAL_CALL getCharacterType( const OUString& text, sal_Int32 nPos,
        const css::lang::Locale& rLocale ) override;
    virtual sal_Int32 SAL_CALL getStringType( const OUString& text, sal_Int32 nPos,
        sal_Int32 nCount, const css::lang::Locale& rLocale ) override;
    virtual css::i18n::ParseResult SAL_CALL parseAnyToken( const OUString& Text, sal_Int32 nPos,
        const css::lang::Locale& rLocale, sal_Int32 nStartCharFlags,
        const OUString& userDefinedCharactersStart, sal_Int32 nContCharFlags,
        const OUString& userDefinedCharactersCont ) override;
    virtual css::i18n::ParseResult SAL_CALL parsePredefinedToken( sal_Int32 nTokenType,
        const OUString& Text, sal_Int32 nPos, const css::lang::Locale& rLocale,
        sal_Int32 nStartCharFlags, const OUString& userDefinedCharactersStart,
        sal_Int32 nContCharFlags, const OUString& userDefinedCharactersCont ) override;

    //XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

private:
    struct lookupTableItem {
        lookupTableItem(css::lang::Locale _aLocale, OUString _aName,
                        css::uno::Reference < XCharacterClassification > _xCI) :
            aLocale(std::move(_aLocale)), aName(std::move(_aName)), xCI(std::move(_xCI)) {};
        css::lang::Locale aLocale;
        OUString aName;
        css::uno::Reference < XCharacterClassification > xCI;
        bool equals(const css::lang::Locale& rLocale) const {
            return aLocale.Language == rLocale.Language &&
                aLocale.Country == rLocale.Country &&
                aLocale.Variant == rLocale.Variant;
        };
    };
    std::vector<lookupTableItem> lookupTable;
    std::optional<lookupTableItem> cachedItem;

    css::uno::Reference < css::uno::XComponentContext > m_xContext;
    css::uno::Reference < XCharacterClassification > xUCI;

    /// @throws css::uno::RuntimeException
    css::uno::Reference < XCharacterClassification > const & getLocaleSpecificCharacterClassification(const css::lang::Locale& rLocale);
    bool createLocaleSpecificCharacterClassification(const OUString& serviceName, const css::lang::Locale& rLocale);

};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
