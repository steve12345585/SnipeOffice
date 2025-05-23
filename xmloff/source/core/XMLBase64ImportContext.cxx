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

#include <com/sun/star/io/XOutputStream.hpp>

#include <comphelper/base64.hxx>
#include <o3tl/string_view.hxx>

#include <xmloff/xmlimp.hxx>
#include <xmloff/XMLBase64ImportContext.hxx>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::io;


XMLBase64ImportContext::XMLBase64ImportContext(
        SvXMLImport& rImport,
        const Reference< XOutputStream >& rOut ) :
    SvXMLImportContext( rImport ),
    m_xOut( rOut )
{
}

XMLBase64ImportContext::~XMLBase64ImportContext()
{
}

void XMLBase64ImportContext::endFastElement(sal_Int32 )
{
    std::u16string_view sChars = o3tl::trim(maCharBuffer);
    if( !sChars.empty() )
    {
        Sequence< sal_Int8 > aBuffer( (sChars.size() / 4) * 3 );
        ::comphelper::Base64::decodeSomeChars( aBuffer, sChars );
        m_xOut->writeBytes( aBuffer );
    }
    maCharBuffer.setLength(0);
    m_xOut->closeOutput();
}

void XMLBase64ImportContext::characters( const OUString& rChars )
{
    maCharBuffer.append(rChars);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
