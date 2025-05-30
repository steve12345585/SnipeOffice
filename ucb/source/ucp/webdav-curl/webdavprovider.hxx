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

#include <sal/config.h>

#include <memory>

#include <rtl/ref.hxx>
#include <com/sun/star/beans/Property.hpp>
#include "DAVSessionFactory.hxx"
#include <ucbhelper/providerhelper.hxx>
#include "PropertyMap.hxx"

namespace com::sun::star::lang {
class XSingleServiceFactory;
}

namespace http_dav_ucp {


// UNO service name for the provider. This name will be used by the UCB to
// create instances of the provider.
inline constexpr OUString WEBDAV_CONTENT_PROVIDER_SERVICE_NAME = u"com.sun.star.ucb.WebDAVContentProvider"_ustr;

// URL scheme. This is the scheme the provider will be able to create
// contents for. The UCB will select the provider ( i.e. in order to create
// contents ) according to this scheme.
#define VNDSUNSTARWEBDAV_URL_SCHEME  "vnd.sun.star.webdav"
#define VNDSUNSTARWEBDAVS_URL_SCHEME u"vnd.sun.star.webdavs"
#define HTTP_URL_SCHEME              u"http"
#define HTTPS_URL_SCHEME             u"https"
#define DAV_URL_SCHEME               u"dav"
#define DAVS_URL_SCHEME              u"davs"
#define WEBDAV_URL_SCHEME            u"webdav"
#define WEBDAVS_URL_SCHEME           u"webdavs"

inline constexpr OUString HTTP_CONTENT_TYPE = u"application/" HTTP_URL_SCHEME "-content"_ustr;

#define WEBDAV_CONTENT_TYPE    HTTP_CONTENT_TYPE
inline constexpr OUString WEBDAV_COLLECTION_TYPE = u"application/" VNDSUNSTARWEBDAV_URL_SCHEME "-collection"_ustr;


class ContentProvider : public ::ucbhelper::ContentProviderImplHelper
{
    rtl::Reference< DAVSessionFactory > m_xDAVSessionFactory;
    std::unique_ptr<PropertyMap> m_pProps;

public:
    explicit ContentProvider( const css::uno::Reference< css::uno::XComponentContext >& rContext );
    virtual ~ContentProvider() override;

    // XInterface
    virtual css::uno::Any SAL_CALL queryInterface( const css::uno::Type & rType ) override;
    virtual void SAL_CALL acquire()
        noexcept override;
    virtual void SAL_CALL release()
        noexcept override;

    // XTypeProvider
    virtual css::uno::Sequence< sal_Int8 > SAL_CALL getImplementationId() override;
    virtual css::uno::Sequence< css::uno::Type > SAL_CALL getTypes() override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XContentProvider
    virtual css::uno::Reference< css::ucb::XContent > SAL_CALL
    queryContent( const css::uno::Reference< css::ucb::XContentIdentifier >& Identifier ) override;


    // Non-interface methods.

    bool getProperty( const OUString & rPropName,
                      css::beans::Property & rProp );
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
