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

#include <toolkit/controls/unocontrols.hxx>
#include <toolkit/controls/unocontrolmodel.hxx>

namespace com::sun::star::util { class XNumberFormatter; }


namespace toolkit
{


    // = UnoControlFormattedFieldModel

    class UnoControlFormattedFieldModel final : public UnoControlModel
    {
    public:
        UnoControlFormattedFieldModel( const css::uno::Reference< css::uno::XComponentContext >& rxContext );
        UnoControlFormattedFieldModel( const UnoControlFormattedFieldModel& rModel )
            : UnoControlModel(rModel)
            , m_bRevokedAsClient(false)
            , m_bSettingValueAndText(false)
        {
        }

        rtl::Reference<UnoControlModel> Clone() const override { return new UnoControlFormattedFieldModel( *this ); }

        // css::io::XPersistObject
        OUString SAL_CALL getServiceName() override;

        // css::beans::XMultiPropertySet
        css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL getPropertySetInfo(  ) override;


        // css::lang::XServiceInfo
        OUString SAL_CALL getImplementationName() override;

        css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;

    private:
        virtual ~UnoControlFormattedFieldModel() override;

        // XComponent
        void SAL_CALL dispose() override;

        // XPropertySet
        void SAL_CALL setPropertyValues( const css::uno::Sequence< OUString >& PropertyNames, const css::uno::Sequence< css::uno::Any >& Values ) override;

        // UnoControlModel
        virtual void ImplNormalizePropertySequence(
                        const sal_Int32                 _nCount,        /// the number of entries in the arrays
                        sal_Int32*                      _pHandles,      /// the handles of the properties to set
                        css::uno::Any*     _pValues,       /// the values of the properties to set
                        sal_Int32*                      _pValidHandles  /// pointer to the valid handles, allowed to be adjusted
                    )   const override;
        void    impl_updateTextFromValue_nothrow(std::unique_lock<std::mutex>& rGuard);
        void    impl_updateCachedFormatter_nothrow(std::unique_lock<std::mutex>& rGuard);
        void    impl_updateCachedFormatKey_nothrow(std::unique_lock<std::mutex>& rGuard);

        css::uno::Any      ImplGetDefaultValue( sal_uInt16 nPropId ) const override;
        ::cppu::IPropertyArrayHelper& getInfoHelper() override;
        bool convertFastPropertyValue(
                    std::unique_lock<std::mutex>& rGuard,
                    css::uno::Any& rConvertedValue,
                    css::uno::Any& rOldValue,
                    sal_Int32 nPropId,
                    const css::uno::Any& rValue
                ) override;

        void setFastPropertyValue_NoBroadcast(
                    std::unique_lock<std::mutex>& rGuard,
                    sal_Int32 nHandle,
                    const css::uno::Any& rValue
                ) override;

        css::uno::Any      m_aCachedFormat;
        bool               m_bRevokedAsClient;
        bool               m_bSettingValueAndText;
        css::uno::Reference< css::util::XNumberFormatter >
                           m_xCachedFormatter;
    };


    // = UnoFormattedFieldControl

    class UnoFormattedFieldControl final : public UnoSpinFieldControl
    {
    public:
                            UnoFormattedFieldControl();
        OUString     GetComponentServiceName() const override;

        // css::awt::XTextListener
        void SAL_CALL textChanged( const css::awt::TextEvent& rEvent ) override;

        // css::lang::XServiceInfo
        OUString SAL_CALL getImplementationName() override;

        css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;
    };


}   // namespace toolkit


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
