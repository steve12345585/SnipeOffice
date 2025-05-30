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

#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XOutputStream.hpp>

#include <string_view>
#include <vector>


extern OUString string_encode( const OUString & rText );
extern OUString string_decode( const OUString & rText );

bool copyStreams( const css::uno::Reference< css::io::XInputStream >& xIS, const css::uno::Reference< css::io::XOutputStream >& xOS );
bool createDirectory( std::u16string_view rURL );


class filter_info_impl
{
public:
    OUString   maFilterName;
    OUString   maType;
    OUString   maDocumentService;
    OUString   maInterfaceName;
    OUString   maComment;
    OUString   maExtension;
    OUString   maExportXSLT;
    OUString   maImportXSLT;
    OUString   maImportTemplate;
    OUString   maDocType;
    OUString   maImportService;
    OUString   maExportService;

    sal_Int32       maFlags;
    sal_Int32       maFileFormatVersion;
    sal_Int32       mnDocumentIconID;

    bool        mbReadonly;

    bool        mbNeedsXSLT2;

    filter_info_impl();
    bool operator==( const filter_info_impl& ) const;

    css::uno::Sequence< OUString > getFilterUserData() const;
};


struct application_info_impl
{
    OUString   maDocumentService;
    OUString   maDocumentUIName;
    OUString   maXMLImporter;
    OUString   maXMLExporter;

    application_info_impl(const OUString& rDocumentService, const OUString& rUINameRes, const OUString& rXMLImporter, const OUString& rXMLExporter);
};


extern std::vector< application_info_impl > const & getApplicationInfos();
extern OUString getApplicationUIName( std::u16string_view rServiceName );
extern const application_info_impl* getApplicationInfo( std::u16string_view rServiceName );

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
