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

#include <rtl/ustring.hxx>
#include <utility>

namespace package_ucp {


#define PACKAGE_URL_SCHEME          "vnd.sun.star.pkg"
#define PACKAGE_ZIP_URL_SCHEME      "vnd.sun.star.zip"
#define PACKAGE_URL_SCHEME_LENGTH   16


class PackageUri
{
    mutable OUString m_aUri;
    mutable OUString m_aParentUri;
    mutable OUString m_aPackage;
    mutable OUString m_aPath;
    mutable OUString m_aName;
    mutable OUString m_aParam;
    mutable OUString m_aScheme;
    mutable bool            m_bValid;

private:
    void init() const;

public:
    explicit PackageUri( OUString aPackageUri )
    : m_aUri(std::move( aPackageUri )), m_bValid( false ) {}

    bool isValid() const
    { init(); return m_bValid; }

    const OUString & getUri() const
    { init(); return m_aUri; }

    void setUri( const OUString & rPackageUri )
    { m_aPath.clear(); m_aUri = rPackageUri; m_bValid = false; }

    const OUString & getParentUri() const
    { init(); return m_aParentUri; }

    const OUString & getPackage() const
    { init(); return m_aPackage; }

    const OUString & getPath() const
    { init(); return m_aPath; }

    const OUString & getName() const
    { init(); return m_aName; }

    const OUString & getParam() const
    { init(); return m_aParam; }

    const OUString & getScheme() const
    { init(); return m_aScheme; }

    inline bool isRootFolder() const;
};

inline bool PackageUri::isRootFolder() const
{
    init();
    return m_aPath == "/";
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
