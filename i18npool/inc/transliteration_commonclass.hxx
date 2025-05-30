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

#include <com/sun/star/i18n/XExtendedTransliteration.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <cppuhelper/implbase.hxx>
#include <rtl/ustring.hxx>

namespace i18npool {

class transliteration_commonclass : public cppu::WeakImplHelper<
                                                                  css::i18n::XExtendedTransliteration,
                                                                  css::lang::XServiceInfo
                                                                >
{
public:
        transliteration_commonclass();

        // Methods which are shared.
        void SAL_CALL
        loadModule( css::i18n::TransliterationModules modName, const css::lang::Locale& rLocale ) override;

        void SAL_CALL
        loadModuleNew( const css::uno::Sequence< css::i18n::TransliterationModulesNew >& modName, const css::lang::Locale& rLocale ) override;

        void SAL_CALL
        loadModuleByImplName( const OUString& implName, const css::lang::Locale& rLocale ) override;

        void SAL_CALL
        loadModulesByImplNames(const css::uno::Sequence< OUString >& modNamelist, const css::lang::Locale& rLocale) override;

        css::uno::Sequence< OUString > SAL_CALL
        getAvailableModules( const css::lang::Locale& rLocale, sal_Int16 sType ) override;

        // Methods which should be implemented in each transliteration module.
        virtual OUString SAL_CALL getName() override;

        virtual sal_Int16 SAL_CALL getType(  ) override = 0;

        virtual OUString SAL_CALL
        transliterate( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount, css::uno::Sequence< sal_Int32 >& offset ) override final
            { return transliterateImpl( inStr, startPos, nCount, &offset ); }

        virtual OUString SAL_CALL
        folding( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount, css::uno::Sequence< sal_Int32 >& offset) override final
            { return foldingImpl( inStr, startPos, nCount, &offset ); }

        // Methods in XExtendedTransliteration
        virtual OUString SAL_CALL
        transliterateString2String( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount ) override;
        virtual OUString SAL_CALL
        transliterateChar2String( sal_Unicode inChar) override;
        virtual sal_Unicode SAL_CALL
        transliterateChar2Char( sal_Unicode inChar ) override = 0;

        virtual sal_Bool SAL_CALL
        equals( const OUString& str1, sal_Int32 pos1, sal_Int32 nCount1, sal_Int32& nMatch1, const OUString& str2, sal_Int32 pos2, sal_Int32 nCount2, sal_Int32& nMatch2 ) override = 0;

        virtual css::uno::Sequence< OUString > SAL_CALL
        transliterateRange( const OUString& str1, const OUString& str2 ) override = 0;

        virtual sal_Int32 SAL_CALL
        compareSubstring( const OUString& s1, sal_Int32 off1, sal_Int32 len1, const OUString& s2, sal_Int32 off2, sal_Int32 len2) override;

        virtual sal_Int32 SAL_CALL
        compareString( const OUString& s1, const OUString& s2) override;

        //XServiceInfo
        virtual OUString SAL_CALL getImplementationName() override;
        virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
        virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;
protected:
        virtual OUString
        transliterateImpl( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount, css::uno::Sequence< sal_Int32 >* pOffset ) = 0;

        virtual OUString
        foldingImpl( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount, css::uno::Sequence< sal_Int32 >* pOffset ) = 0;

        css::lang::Locale   aLocale;
        const char*         transliterationName;
        const char*         implementationName;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
