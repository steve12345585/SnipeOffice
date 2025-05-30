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

#include <com/sun/star/text/XTextDocument.hpp>
#include <com/sun/star/container/XEnumerationAccess.hpp>

#include <tools/urlobj.hxx>
#include <rtl/ref.hxx>

#include "vbadocument.hxx"
#include "vbadocuments.hxx"

#include <osl/file.hxx>
#include <utility>
#include <unotxdoc.hxx>

using namespace ::ooo::vba;
using namespace ::com::sun::star;

static uno::Any
getDocument( uno::Reference< uno::XComponentContext > const & xContext, const rtl::Reference< SwXTextDocument > &xDoc, const uno::Any& aApplication )
{
    // FIXME: fine as long as SwVbaDocument is stateless ...
    if( !xDoc.is() )
        return uno::Any();

    rtl::Reference<SwVbaDocument> pWb = new SwVbaDocument(  uno::Reference< XHelperInterface >( aApplication, uno::UNO_QUERY_THROW ), xContext, xDoc );
    return uno::Any( uno::Reference< word::XDocument > (pWb) );
}

namespace {

class DocumentEnumImpl : public EnumerationHelperImpl
{
    uno::Any m_aApplication;
public:
    /// @throws uno::RuntimeException
    DocumentEnumImpl( const uno::Reference< XHelperInterface >& xParent, const uno::Reference< uno::XComponentContext >& xContext, const uno::Reference< container::XEnumeration >& xEnumeration, uno::Any  aApplication ) : EnumerationHelperImpl( xParent, xContext, xEnumeration ), m_aApplication(std::move( aApplication )) {}

    virtual uno::Any SAL_CALL nextElement(  ) override
    {
        uno::Reference< text::XTextDocument > xDoc( m_xEnumeration->nextElement(), uno::UNO_QUERY_THROW );
        return getDocument( m_xContext, dynamic_cast<SwXTextDocument*>(xDoc.get()), m_aApplication );
    }
};

}

SwVbaDocuments::SwVbaDocuments( const uno::Reference< XHelperInterface >& xParent, const uno::Reference< uno::XComponentContext >& xContext ) : SwVbaDocuments_BASE( xParent, xContext, VbaDocumentsBase::WORD_DOCUMENT )
{
}
// XEnumerationAccess
uno::Type
SwVbaDocuments::getElementType()
{
    return cppu::UnoType<word::XDocument>::get();
}
uno::Reference< container::XEnumeration >
SwVbaDocuments::createEnumeration()
{
    // #FIXME it's possible the DocumentEnumImpl here doesn't reflect
    // the state of this object (although it should) would be
    // safer to create an enumeration based on this objects state
    // rather than one effectively based of the desktop component
    uno::Reference< container::XEnumerationAccess > xEnumerationAccess( m_xIndexAccess, uno::UNO_QUERY_THROW );
    return new DocumentEnumImpl( mxParent, mxContext, xEnumerationAccess->createEnumeration(), Application() );
}

uno::Any
SwVbaDocuments::createCollectionObject( const uno::Any& aSource )
{
    uno::Reference< text::XTextDocument > xDoc( aSource, uno::UNO_QUERY_THROW );
    return getDocument( mxContext, dynamic_cast<SwXTextDocument*>(xDoc.get()), Application() );
}

uno::Any SAL_CALL
SwVbaDocuments::Add( const uno::Any& Template, const uno::Any& /*NewTemplate*/, const uno::Any& /*DocumentType*/, const uno::Any& /*Visible*/ )
{
    OUString sFileName;
    if( Template.hasValue() && ( Template >>= sFileName ) )
    {
        return  Open( sFileName, uno::Any(), uno::Any(), uno::Any(), uno::Any(), uno::Any(), uno::Any(), uno::Any(), uno::Any(), uno::Any(), uno::Any(), uno::Any(), uno::Any(), uno::Any(), uno::Any(), uno::Any());
    }
    uno::Reference <text::XTextDocument> xTextDoc( createDocument() , uno::UNO_QUERY_THROW );
    return getDocument( mxContext, dynamic_cast<SwXTextDocument*>(xTextDoc.get()), Application() );
}

// #TODO# #FIXME# can any of the unused params below be used?
// #TODO# #FIXME# surely we should actually close the document here
void SAL_CALL
SwVbaDocuments::Close( const uno::Any& /*SaveChanges*/, const uno::Any& /*OriginalFormat*/, const uno::Any& /*RouteDocument*/ )
{
}

// #TODO# #FIXME# can any of the unused params below be used?
uno::Any SAL_CALL
SwVbaDocuments::Open( const OUString& Filename, const uno::Any& /*ConfirmConversions*/, const uno::Any& ReadOnly, const uno::Any& /*AddToRecentFiles*/, const uno::Any& /*PasswordDocument*/, const uno::Any& /*PasswordTemplate*/, const uno::Any& /*Revert*/, const uno::Any& /*WritePasswordDocument*/, const uno::Any& /*WritePasswordTemplate*/, const uno::Any& /*Format*/, const uno::Any& /*Encoding*/, const uno::Any& /*Visible*/, const uno::Any& /*OpenAndRepair*/, const uno::Any& /*DocumentDirection*/, const uno::Any& /*NoEncodingDialog*/, const uno::Any& /*XMLTransform*/ )
{
    SAL_INFO("sw.vba", "Documents.Open(Filename:=" << Filename << ",ReadOnly:=" << ReadOnly << ")");

    // we need to detect if this is a URL, if not then assume it's a file path
    OUString aURL;
    INetURLObject aObj;
    aObj.SetURL( Filename );
    bool bIsURL = aObj.GetProtocol() != INetProtocol::NotValid;
    if ( bIsURL )
        aURL = Filename;
    else
        osl::FileBase::getFileURLFromSystemPath( Filename, aURL );

    uno::Reference <text::XTextDocument> xSpreadDoc( openDocument( Filename, ReadOnly, {}), uno::UNO_QUERY_THROW );
    uno::Any aRet = getDocument( mxContext, dynamic_cast<SwXTextDocument*>(xSpreadDoc.get()), Application() );
    uno::Reference< word::XDocument > xDocument( aRet, uno::UNO_QUERY );
    if ( xDocument.is() )
        xDocument->Activate();
    return aRet;
}

uno::Any SAL_CALL
SwVbaDocuments::OpenNoRepairDialog( const OUString& Filename, const uno::Any& ConfirmConversions, const uno::Any& ReadOnly, const uno::Any& AddToRecentFiles, const uno::Any& PasswordDocument, const uno::Any& PasswordTemplate, const uno::Any& Revert, const uno::Any& WritePasswordDocument, const uno::Any& WritePasswordTemplate, const uno::Any& Format, const uno::Any& Encoding, const uno::Any& Visible, const uno::Any& OpenAndRepair, const uno::Any& DocumentDirection, const uno::Any& NoEncodingDialog, const uno::Any& XMLTransform )
{
    return Open( Filename, ConfirmConversions, ReadOnly, AddToRecentFiles, PasswordDocument, PasswordTemplate, Revert, WritePasswordDocument, WritePasswordTemplate, Format, Encoding, Visible, OpenAndRepair, DocumentDirection, NoEncodingDialog, XMLTransform );
}

uno::Any SAL_CALL
SwVbaDocuments::OpenOld( const OUString& FileName, const uno::Any& ConfirmConversions, const uno::Any& ReadOnly, const uno::Any& AddToRecentFiles, const uno::Any& PasswordDocument, const uno::Any& PasswordTemplate, const uno::Any& Revert, const uno::Any& WritePasswordDocument, const uno::Any& WritePasswordTemplate, const uno::Any& Format )
{
    return Open( FileName, ConfirmConversions, ReadOnly, AddToRecentFiles, PasswordDocument, PasswordTemplate, Revert, WritePasswordDocument, WritePasswordTemplate, Format, uno::Any(), uno::Any(), uno::Any(), uno::Any(), uno::Any(), uno::Any() );
}

OUString
SwVbaDocuments::getServiceImplName()
{
    return u"SwVbaDocuments"_ustr;
}

uno::Sequence<OUString>
SwVbaDocuments::getServiceNames()
{
    static uno::Sequence< OUString > const sNames
    {
        u"ooo.vba.word.Documents"_ustr
    };
    return sNames;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
