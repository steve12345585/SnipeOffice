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

#ifndef INCLUDED_SVX_SOURCE_INC_XFM_ADDCONDITION_HXX
#define INCLUDED_SVX_SOURCE_INC_XFM_ADDCONDITION_HXX

#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/xforms/XModel.hpp>
#include <svtools/genericunodialog.hxx>
#include <comphelper/proparrhlp.hxx>


namespace svxform
{

    typedef ::svt::OGenericUnoDialog OAddConditionDialogBase;
    class OAddConditionDialog final
            :public OAddConditionDialogBase
            ,public ::comphelper::OPropertyArrayUsageHelper< OAddConditionDialog >
    {
    public:
        static css::uno::Reference< css::uno::XInterface >
            Create( const css::uno::Reference< css::lang::XMultiServiceFactory >& );

    private:
        OAddConditionDialog( const css::uno::Reference< css::uno::XComponentContext >& _rxORB );

        // XTypeProvider
        virtual css::uno::Sequence<sal_Int8> SAL_CALL getImplementationId(  ) override;

        // XServiceInfo
        virtual OUString SAL_CALL getImplementationName() override;
        virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

        // XPropertySet
        virtual css::uno::Reference<css::beans::XPropertySetInfo>  SAL_CALL getPropertySetInfo() override;
        virtual ::cppu::IPropertyArrayHelper& SAL_CALL getInfoHelper() override;

        // OPropertyArrayUsageHelper
        virtual ::cppu::IPropertyArrayHelper* createArrayHelper( ) const override;

        // OGenericUnoDialog overridables
        virtual std::unique_ptr<weld::DialogController> createDialog(const css::uno::Reference<css::awt::XWindow>& rParent) override;
        virtual void executedDialog(sal_Int16 _nExecutionResult) override;

        css::uno::Reference< css::beans::XPropertySet >
                                m_xBinding;
        OUString                m_sFacetName;
        OUString                m_sConditionValue;
        css::uno::Reference< css::xforms::XModel >
                                m_xWorkModel;
    };


}


#endif // INCLUDED_SVX_SOURCE_INC_XFM_ADDCONDITION_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
