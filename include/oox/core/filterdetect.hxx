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

#ifndef INCLUDED_OOX_CORE_FILTERDETECT_HXX
#define INCLUDED_OOX_CORE_FILTERDETECT_HXX

#include <vector>

#include <com/sun/star/document/XExtendedFilterDetection.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/xml/sax/XFastDocumentHandler.hpp>
#include <cppuhelper/implbase.hxx>
#include <oox/dllapi.h>
#include <rtl/ustring.hxx>
#include <sal/types.h>

namespace com::sun::star {
    namespace io { class XInputStream; }
    namespace uno { class XComponentContext; }
}

namespace utl { class MediaDescriptor; }

namespace oox { class AttributeList; }

namespace oox::core {

enum class OOXMLVariant {
    ECMA_Transitional,
    ISO_Transitional,
    ISO_Strict
};


/** Document handler specifically designed for detecting OOXML file formats.

    It takes a reference to the filter string object via its constructor, and
    puts the name of the detected filter to it, if it successfully finds one.
 */
class FilterDetectDocHandler final : public ::cppu::WeakImplHelper< css::xml::sax::XFastDocumentHandler >
{
public:
    explicit            FilterDetectDocHandler( const css::uno::Reference< css::uno::XComponentContext >& rxContext, OUString& rFilter, OUString aFileName );
    virtual             ~FilterDetectDocHandler() override;

    // XFastDocumentHandler
    virtual void SAL_CALL startDocument() override;
    virtual void SAL_CALL endDocument() override;
    virtual void SAL_CALL processingInstruction( const OUString& rTarget, const OUString& rData ) override;
    virtual void SAL_CALL setDocumentLocator( const css::uno::Reference< css::xml::sax::XLocator >& xLocator ) override;

    // XFastContextHandler
    virtual void SAL_CALL startFastElement( sal_Int32 nElement, const css::uno::Reference< css::xml::sax::XFastAttributeList >& Attribs ) override;
    virtual void SAL_CALL startUnknownElement( const OUString& Namespace, const OUString& Name, const css::uno::Reference< css::xml::sax::XFastAttributeList >& Attribs ) override;
    virtual void SAL_CALL endFastElement( sal_Int32 Element ) override;
    virtual void SAL_CALL endUnknownElement( const OUString& Namespace, const OUString& Name ) override;
    virtual css::uno::Reference< XFastContextHandler > SAL_CALL createFastChildContext( sal_Int32 Element, const css::uno::Reference< css::xml::sax::XFastAttributeList >& Attribs ) override;
    virtual css::uno::Reference< XFastContextHandler > SAL_CALL createUnknownChildContext( const OUString& Namespace, const OUString& Name, const css::uno::Reference< css::xml::sax::XFastAttributeList >& Attribs ) override;
    virtual void SAL_CALL characters( const OUString& aChars ) override;

private:
    void parseSettings(const AttributeList& rAttribs);
    void                parseRelationship( const AttributeList& rAttribs );

    OUString            getFilterNameFromContentType( std::u16string_view rContentType, std::u16string_view rFileName );
    void                parseContentTypesDefault( const AttributeList& rAttribs );
    void                parseContentTypesOverride( const AttributeList& rAttribs );

private:
    typedef ::std::vector< sal_Int32 > ContextVector;

    OUString&           mrFilterName;
    OUString            maFileName;
    ContextVector       maContextStack;
    OUString            maTargetPath;
    OOXMLVariant        maOOXMLVariant;
    css::uno::Reference< css::uno::XComponentContext > mxContext;
};


class OOX_DLLPUBLIC FilterDetect final : public ::cppu::WeakImplHelper<css::document::XExtendedFilterDetection, css::lang::XServiceInfo>
{
public:
    /// @throws css::uno::RuntimeException
    explicit            FilterDetect( const css::uno::Reference< css::uno::XComponentContext >& rxContext );
    virtual             ~FilterDetect() override;

    /** Tries to extract an unencrypted ZIP package from the passed media
        descriptor.

        First, this function checks if the input stream provided by the media
        descriptor property 'InputStream' contains a ZIP package. If yes, this
        stream is returned.

        Second, this function checks if the 'ComponentData' property exists and
        contains a sequence of com.sun.star.beans.NamedValue. If yes, a named
        value is searched with the name 'DecryptedPackage' and a value of type
        com.sun.star.io.XStream. If the input stream provided by this XStream
        contains a ZIP package, this input stream is returned.

        Third, this function checks if the input stream of the media descriptor
        contains an OLE package. If yes, it checks the existence of the streams
        'EncryptionInfo' and 'EncryptedPackage' and tries to decrypt the package
        into a temporary file. This may include requesting a password from the
        media descriptor property 'Password' or from the user, using the
        interaction handler provided by the descriptor. On success, and if the
        decrypted package is a ZIP package, the XStream of the temporary file
        is stored in the property 'ComponentData' of the media descriptor and
        its input stream is returned.
     */
    css::uno::Reference< css::io::XInputStream >
                        extractUnencryptedPackage( utl::MediaDescriptor& rMediaDesc ) const;

    // com.sun.star.lang.XServiceInfo interface -------------------------------

    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& rServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // com.sun.star.document.XExtendedFilterDetection interface ---------------

    /** Detects MS Office 2007 file types and supports package decryption.

        The following file types are detected:
        - MS Word 2007 XML Document (*.docx, *.docm)
        - MS Word 2007 XML Template (*.dotx, *.dotm)
        - MS Excel 2007 XML Document (*.xlsx, *.xlsm)
        - MS Excel 2007 BIFF12 Document (*.xlsb)
        - MS Excel 2007 XML Template (*.xltx, *.xltm)
        - MS Powerpoint 2007 XML Document (*.pptx, *.pptm)
        - MS Powerpoint 2007 XML Template (*.potx, *.potm)

        If the package is encrypted, the detection tries to decrypt it into a
        temporary file. The user may be asked for a password. The XStream
        interface of the temporary file will be stored in the 'ComponentData'
        property of the passed media descriptor.
     */
    virtual OUString SAL_CALL
                        detect( css::uno::Sequence< css::beans::PropertyValue >& rMediaDescSeq ) override;

private:
    css::uno::Reference< css::uno::XComponentContext > mxContext;
};


} // namespace oox::core

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
