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

#include <sal/config.h>

#include <string_view>

#include <uielement/addonstoolbarwrapper.hxx>

#include <com/sun/star/frame/ModuleManager.hpp>
#include <com/sun/star/frame/XModuleManager2.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/ui/XUIElementFactory.hpp>

#include <comphelper/propertyvalue.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <vcl/svapp.hxx>

using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::frame;
using namespace com::sun::star::beans;
using namespace ::com::sun::star::ui;
using namespace framework;

namespace {

class AddonsToolBarFactory :  public ::cppu::WeakImplHelper< css::lang::XServiceInfo ,
                                                              css::ui::XUIElementFactory >
{
public:
    explicit AddonsToolBarFactory( const css::uno::Reference< css::uno::XComponentContext >& xContext );

    virtual OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.framework.AddonsToolBarFactory"_ustr;
    }

    virtual sal_Bool SAL_CALL supportsService(OUString const & ServiceName) override
    {
        return cppu::supportsService(this, ServiceName);
    }

    virtual css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override
    {
        return {u"com.sun.star.ui.ToolBarFactory"_ustr};
    }

    // XUIElementFactory
    virtual css::uno::Reference< css::ui::XUIElement > SAL_CALL createUIElement( const OUString& ResourceURL, const css::uno::Sequence< css::beans::PropertyValue >& Args ) override;

    bool hasButtonsInContext( const css::uno::Sequence< css::uno::Sequence< css::beans::PropertyValue > >& rPropSeq,
                                  const css::uno::Reference< css::frame::XFrame >& rFrame );

private:
    css::uno::Reference< css::uno::XComponentContext >     m_xContext;
    css::uno::Reference< css::frame::XModuleManager2 >     m_xModuleManager;
};

AddonsToolBarFactory::AddonsToolBarFactory(
    const css::uno::Reference< css::uno::XComponentContext >& xContext ) :
    m_xContext( xContext )
    , m_xModuleManager( ModuleManager::create( xContext ) )
{
}

bool IsCorrectContext( std::u16string_view rModuleIdentifier, std::u16string_view aContextList )
{
    if ( aContextList.empty() )
        return true;

    if ( !rModuleIdentifier.empty() )
    {
        return aContextList.find( rModuleIdentifier ) != std::u16string_view::npos;
    }

    return false;
}

bool AddonsToolBarFactory::hasButtonsInContext(
    const Sequence< Sequence< PropertyValue > >& rPropSeqSeq,
    const Reference< XFrame >& rFrame )
{
    OUString aModuleIdentifier;
    try
    {
        aModuleIdentifier = m_xModuleManager->identify( rFrame );
    }
    catch ( const RuntimeException& )
    {
        throw;
    }
    catch ( const Exception& )
    {
    }

    // Check before we create a toolbar that we have at least one button in
    // the current frame context.
    for ( Sequence<PropertyValue> const & props : rPropSeqSeq )
    {
        bool    bIsButton( true );
        bool    bIsCorrectContext( false );
        sal_uInt32  nPropChecked( 0 );

        for ( PropertyValue const & prop : props )
        {
            if ( prop.Name == "Context" )
            {
                OUString aContextList;
                if ( prop.Value >>= aContextList )
                    bIsCorrectContext = IsCorrectContext( aModuleIdentifier, aContextList );
                nPropChecked++;
            }
            else if ( prop.Name == "URL" )
            {
                OUString aURL;
                prop.Value >>= aURL;
                bIsButton = aURL != "private:separator";
                nPropChecked++;
            }

            if ( nPropChecked == 2 )
                break;
        }

        if ( bIsButton && bIsCorrectContext )
            return true;
    }

    return false;
}

// XUIElementFactory
Reference< XUIElement > SAL_CALL AddonsToolBarFactory::createUIElement(
    const OUString& ResourceURL,
    const Sequence< PropertyValue >& Args )
{
    SolarMutexGuard g;

    Sequence< Sequence< PropertyValue > >   aConfigData;
    Reference< XFrame >                     xFrame;
    OUString                           aResourceURL( ResourceURL );

    for ( PropertyValue const & arg : Args )
    {
        if ( arg.Name == "ConfigurationData" )
            arg.Value >>= aConfigData;
        else if ( arg.Name == "Frame" )
            arg.Value >>= xFrame;
        else if ( arg.Name == "ResourceURL" )
            arg.Value >>= aResourceURL;
    }

    if ( !aResourceURL.startsWith("private:resource/toolbar/addon_") )
        throw IllegalArgumentException();

    // Identify frame and determine module identifier to look for context based buttons
    rtl::Reference< AddonsToolBarWrapper > xToolBar;
    if ( xFrame.is() &&
         aConfigData.hasElements() &&
         hasButtonsInContext( aConfigData, xFrame ))
    {
        Sequence< Any > aPropSeq{ Any(comphelper::makePropertyValue(u"Frame"_ustr, xFrame)),
                                  Any(comphelper::makePropertyValue(u"ConfigurationData"_ustr,
                                                                    aConfigData)),
                                  Any(comphelper::makePropertyValue(u"ResourceURL"_ustr, aResourceURL)) };

        SolarMutexGuard aGuard;
        rtl::Reference<AddonsToolBarWrapper> pToolBarWrapper = new AddonsToolBarWrapper( m_xContext );
        xToolBar = pToolBarWrapper;
        pToolBarWrapper->initialize( aPropSeq );
    }

    return xToolBar;
}

}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
com_sun_star_comp_framework_AddonsToolBarFactory_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new AddonsToolBarFactory(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
