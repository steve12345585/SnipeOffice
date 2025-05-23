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

#include "unodialogabp.hxx"
#include "abspilot.hxx"
#include <comphelper/sequence.hxx>
#include <vcl/svapp.hxx>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/awt/XWindow.hpp>

#define PROPERTY_ID_DATASOURCENAME  3

namespace abp
{
    using namespace ::com::sun::star;
    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::ui::dialogs;

    OABSPilotUno::OABSPilotUno(const Reference< XComponentContext >& _rxORB)
        : OABSPilotUno_JBase(_rxORB)
    {
        registerProperty( u"DataSourceName"_ustr, PROPERTY_ID_DATASOURCENAME, PropertyAttribute::READONLY ,
            &m_sDataSourceName, cppu::UnoType<decltype(m_sDataSourceName)>::get() );
    }

    OUString SAL_CALL OABSPilotUno::getImplementationName()
    {
        return u"org.openoffice.comp.abp.OAddressBookSourcePilot"_ustr;
    }

    css::uno::Sequence<OUString> SAL_CALL OABSPilotUno::getSupportedServiceNames()
    {
        return { u"com.sun.star.ui.dialogs.AddressBookSourcePilot"_ustr };
    }

    Reference<XPropertySetInfo>  SAL_CALL OABSPilotUno::getPropertySetInfo()
    {
        Reference<XPropertySetInfo>  xInfo( createPropertySetInfo( getInfoHelper() ) );
        return xInfo;
    }


    ::cppu::IPropertyArrayHelper& OABSPilotUno::getInfoHelper()
    {
        return *getArrayHelper();
    }


    ::cppu::IPropertyArrayHelper* OABSPilotUno::createArrayHelper( ) const
    {
        Sequence< Property > aProps;
        describeProperties(aProps);
        return new ::cppu::OPropertyArrayHelper(aProps);
    }

    void SAL_CALL OABSPilotUno::initialize( const Sequence< Any >& aArguments )
    {
        Reference<awt::XWindow> xParentWindow;
        if (aArguments.getLength() == 1 && (aArguments[0] >>= xParentWindow) ) {
            Sequence< Any > aNewArgs{ Any(PropertyValue(
                u"ParentWindow"_ustr, 0, Any(xParentWindow), PropertyState_DIRECT_VALUE )) };
            OGenericUnoDialog::initialize(aNewArgs);
        } else {
            OGenericUnoDialog::initialize(aArguments);
        }
    }

    std::unique_ptr<weld::DialogController> OABSPilotUno::createDialog(const css::uno::Reference<css::awt::XWindow>& rParent)
    {
        return std::make_unique<OAddressBookSourcePilot>(Application::GetFrameWeld(rParent), m_aContext);
    }

    Any SAL_CALL OABSPilotUno::execute( const Sequence< NamedValue >& /*lArgs*/ )
    {
        // not interested in the context, not interested in the args
        // -> call the execute method of the XExecutableDialog
        static_cast< XExecutableDialog* >( this )->execute();

        // result interest not really ...
        // We show this dialog one times only!
        // User has one chance to accept it or not.
        // (or he can start it again by using wizard-menu!)
        // So we should deregister it on our general job execution service by using right protocol parameters.
        css::uno::Sequence< css::beans::NamedValue > lProtocol { { u"Deactivate"_ustr, css::uno::Any( true ) } };
        return Any( lProtocol );
    }

    void OABSPilotUno::executedDialog(sal_Int16 _nExecutionResult)
    {
        if ( _nExecutionResult == RET_OK )
        {
            const AddressSettings& aSettings = static_cast<OAddressBookSourcePilot*>(m_xDialog.get())->getSettings();
            m_sDataSourceName = aSettings.bRegisterDataSource ? aSettings.sRegisteredDataSourceName : aSettings.sDataSourceName;
        }
    }


}   // namespace abp

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
org_openoffice_comp_abp_OAddressBookSourcePilot(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new abp::OABSPilotUno(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
