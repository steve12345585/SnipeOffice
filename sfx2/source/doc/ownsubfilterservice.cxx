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

#include <com/sun/star/frame/DoubleInitializationException.hpp>
#include <com/sun/star/document/XFilter.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/io/XStream.hpp>

#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <sfx2/objsh.hxx>

using namespace css;

namespace {

class OwnSubFilterService : public cppu::WeakImplHelper < document::XFilter
                                                        ,lang::XServiceInfo >
{
    uno::Reference< frame::XModel > m_xModel;
    uno::Reference< io::XStream > m_xStream;
    SfxObjectShell* m_pObjectShell;

public:
    /// @throws css::uno::Exception
    /// @throws css::uno::RuntimeException
    explicit OwnSubFilterService(const css::uno::Sequence< css::uno::Any >& aArguments);

    // XFilter
    virtual sal_Bool SAL_CALL filter( const uno::Sequence< beans::PropertyValue >& aDescriptor ) override;
    virtual void SAL_CALL cancel() override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName(  ) override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual uno::Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;
};

OwnSubFilterService::OwnSubFilterService(const css::uno::Sequence< css::uno::Any >& aArguments)
    : m_pObjectShell( nullptr )
{
    if ( aArguments.getLength() != 2 )
        throw lang::IllegalArgumentException();

    if ( m_pObjectShell )
        throw frame::DoubleInitializationException();

    if ( ( aArguments[1] >>= m_xStream ) && m_xStream.is()
      && ( aArguments[0] >>= m_xModel ) && m_xModel.is() )
    {
        m_pObjectShell = SfxObjectShell::GetShellFromComponent(m_xModel);
    }

    if ( !m_pObjectShell )
        throw lang::IllegalArgumentException();
}

sal_Bool SAL_CALL OwnSubFilterService::filter( const uno::Sequence< beans::PropertyValue >& aDescriptor )
{
    if ( !m_pObjectShell )
        throw uno::RuntimeException();

    return m_pObjectShell->ImportFromGeneratedStream_Impl( m_xStream, aDescriptor );
}

void SAL_CALL OwnSubFilterService::cancel()
{
    // not implemented
}

OUString SAL_CALL OwnSubFilterService::getImplementationName()
{
    return u"com.sun.star.comp.document.OwnSubFilter"_ustr;
}

sal_Bool SAL_CALL OwnSubFilterService::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

uno::Sequence< OUString > SAL_CALL OwnSubFilterService::getSupportedServiceNames()
{
    return { u"com.sun.star.document.OwnSubFilter"_ustr, u"com.sun.star.comp.document.OwnSubFilter"_ustr };
}

}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
com_sun_star_comp_document_OwnSubFilter_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &arguments)
{
    return cppu::acquire(new OwnSubFilterService(arguments));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
