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

#include <com/sun/star/i18n/XNativeNumberSupplier2.hpp>
#include <com/sun/star/i18n/NativeNumberXmlAttributes.hpp>
#include <cppuhelper/implbase.hxx>
#include <com/sun/star/lang/XServiceInfo.hpp>

namespace com::sun::star::i18n { class XCharacterClassification; }

namespace i18npool {

//      ----------------------------------------------------
//      class NativeNumberSupplierService
//      ----------------------------------------------------
class NativeNumberSupplierService final : public cppu::WeakImplHelper
<
        css::i18n::XNativeNumberSupplier2,
        css::lang::XServiceInfo
>
{
public:
        NativeNumberSupplierService() {}

        // Methods
        virtual OUString SAL_CALL getNativeNumberString( const OUString& aNumberString,
                const css::lang::Locale& aLocale, sal_Int16 nNativeNumberMode ) override;

        virtual sal_Bool SAL_CALL isValidNatNum( const css::lang::Locale& rLocale,
                sal_Int16 nNativeNumberMode ) override { return isValidNatNumImpl(rLocale, nNativeNumberMode); }

        virtual css::i18n::NativeNumberXmlAttributes SAL_CALL convertToXmlAttributes(
                const css::lang::Locale& aLocale, sal_Int16 nNativeNumberMode ) override;

        virtual sal_Int16 SAL_CALL convertFromXmlAttributes(
                const css::i18n::NativeNumberXmlAttributes& aAttr ) override;

        // XNativeNumberSupplier2
        virtual OUString SAL_CALL getNativeNumberStringParams(
            const OUString& rNumberString, const css::lang::Locale& rLocale,
            sal_Int16 nNativeNumberMode, const OUString& rNativeNumberParams) override;

        //XServiceInfo
        virtual OUString SAL_CALL getImplementationName() override;
        virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
        virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

        // following methods are not for XNativeNumberSupplier, they are for calling from transliterations
        /// @throws css::uno::RuntimeException
        OUString getNativeNumberString(const OUString& rNumberString,
                                       const css::lang::Locale& rLocale,
                                       sal_Int16 nNativeNumberMode,
                                       css::uno::Sequence<sal_Int32>* pOffset,
                                       std::u16string_view rNativeNumberParams = std::u16string_view());
        /// @throws css::uno::RuntimeException
        static sal_Unicode getNativeNumberChar( const sal_Unicode inChar,
                const css::lang::Locale& aLocale, sal_Int16 nNativeNumberMode ) ;

private:
        static bool isValidNatNumImpl( const css::lang::Locale& aLocale,
                sal_Int16 nNativeNumberMode );
        css::lang::Locale aLocale;
        mutable css::uno::Reference< css::i18n::XCharacterClassification > xCharClass;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
