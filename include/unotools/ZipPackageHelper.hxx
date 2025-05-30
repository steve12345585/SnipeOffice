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
#ifndef INCLUDED_UNOTOOLS_ZIPPACKAGEHELPER_HXX
#define INCLUDED_UNOTOOLS_ZIPPACKAGEHELPER_HXX

#include <unotools/unotoolsdllapi.h>

#include <com/sun/star/uno/XInterface.hpp>

namespace com::sun::star::container { class XHierarchicalNameAccess; }
namespace com::sun::star::lang { class XSingleServiceFactory; }
namespace com::sun::star::uno { class XComponentContext; }

namespace utl {

class UNOTOOLS_DLLPUBLIC ZipPackageHelper
{
public:
    ZipPackageHelper( const css::uno::Reference< css::uno::XComponentContext >& rxContext,
        const OUString& sPackageURL);

    void savePackage();

    /// @throws css::uno::Exception
    void addFile( css::uno::Reference< css::uno::XInterface > const & xRootFolder,
                  const OUString& rSourceFile );

    /// @throws css::uno::Exception
    css::uno::Reference< css::uno::XInterface > addFolder( css::uno::Reference< css::uno::XInterface > const & xRootFolder,
                                                           const OUString& rName );

    void addFolderWithContent( css::uno::Reference< css::uno::XInterface > const & xRootFolder,
                               const OUString& rDirURL );

    css::uno::Reference< css::uno::XInterface >& getRootFolder();

private:
    css::uno::Reference< css::uno::XComponentContext > mxContext;
    css::uno::Reference< css::container::XHierarchicalNameAccess > mxHNameAccess;
    css::uno::Reference< css::lang::XSingleServiceFactory > mxFactory;
    css::uno::Reference< css::uno::XInterface > mxRootFolder;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
