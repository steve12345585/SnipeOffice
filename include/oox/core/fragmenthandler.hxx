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

#ifndef INCLUDED_OOX_CORE_FRAGMENTHANDLER_HXX
#define INCLUDED_OOX_CORE_FRAGMENTHANDLER_HXX

#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/xml/sax/XFastDocumentHandler.hpp>
#include <cppuhelper/implbase.hxx>
#include <oox/core/contexthandler.hxx>
#include <oox/core/relations.hxx>
#include <oox/dllapi.h>
#include <rtl/ref.hxx>
#include <rtl/ustring.hxx>
#include <sal/types.h>

namespace com::sun::star {
    namespace io { class XInputStream; }
    namespace xml::sax { class XFastContextHandler; }
    namespace xml::sax { class XLocator; }
}

namespace oox::core {

class XmlFilterBase;

/** Base data of a fragment.

    This data is stored in a separate struct to make it accessible in every
    child context handler of the fragment.
 */
struct FragmentBaseData
{
    XmlFilterBase&      mrFilter;
    const OUString      maFragmentPath;
    css::uno::Reference< css::xml::sax::XLocator >
                        mxLocator;
    RelationsRef        mxRelations;

    explicit            FragmentBaseData(
                            XmlFilterBase& rFilter,
                            OUString aFragmentPath,
                            RelationsRef xRelations );
};


/** Describes record identifiers used to create contexts in a binary stream.

    If a record is used to start a new context, usually the record identifier
    increased by 1 is used to mark the end of this context, e.g. the Excel
    record SHEETDATA == 0x0091 starts the <sheetData> context, and the record
    SHEETDATA_END == 0x0092 ends this context. But some records are used to
    start a new context, though there is no identifier to end this context,
    e.g. the ROW or EXTROW records. These record identifiers can be marked by
    setting the mnEndRecId member of this struct to -1.
 */
struct RecordInfo
{
    sal_Int32           mnStartRecId;       ///< Record identifier for context start.
    sal_Int32           mnEndRecId;         ///< Record identifier for context end, -1 = no record.
};


typedef ::cppu::ImplInheritanceHelper< ContextHandler, css::xml::sax::XFastDocumentHandler > FragmentHandler_BASE;

class OOX_DLLPUBLIC FragmentHandler : public FragmentHandler_BASE
{
public:
    explicit            FragmentHandler( XmlFilterBase& rFilter, const OUString& rFragmentPath );
    virtual             ~FragmentHandler() override;

    FragmentHandler(FragmentHandler const &) = default;
    FragmentHandler(FragmentHandler &&) = default;
    FragmentHandler & operator =(FragmentHandler const &) = delete; // due to ContextHandler
    FragmentHandler & operator =(FragmentHandler &&) = delete; // due to ContextHandler

    /** Returns the com.sun.star.xml.sax.XFastContextHandler interface of this context. */
    css::uno::Reference< css::xml::sax::XFastContextHandler >
                        getFastContextHandler() { return static_cast< ContextHandler* >( this ); }

    // com.sun.star.xml.sax.XFastDocumentHandler interface --------------------

    virtual void SAL_CALL startDocument() override;
    virtual void SAL_CALL endDocument() override;
    virtual void SAL_CALL processingInstruction( const OUString& rTarget, const OUString& rData ) override;
    virtual void SAL_CALL setDocumentLocator( const css::uno::Reference< css::xml::sax::XLocator >& rxLocator ) override;

    // com.sun.star.xml.sax.XFastContextHandler interface ---------------------

    virtual void SAL_CALL startFastElement( ::sal_Int32 Element, const css::uno::Reference< css::xml::sax::XFastAttributeList >& Attribs ) override;
    virtual void SAL_CALL startUnknownElement( const OUString& Namespace, const OUString& Name, const css::uno::Reference< css::xml::sax::XFastAttributeList >& Attribs ) override;
    virtual void SAL_CALL endFastElement( ::sal_Int32 Element ) override;
    virtual void SAL_CALL endUnknownElement( const OUString& Namespace, const OUString& Name ) override;
    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext( ::sal_Int32 Element, const css::uno::Reference< css::xml::sax::XFastAttributeList >& Attribs ) override;
    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createUnknownChildContext( const OUString& Namespace, const OUString& Name, const css::uno::Reference< css::xml::sax::XFastAttributeList >& Attribs ) override;
    virtual void SAL_CALL characters( const OUString& aChars ) override;

    // XML stream handling ----------------------------------------------------

    /** Opens the fragment stream referred by the own fragment path. Derived
        classes may provide specialized stream implementations. */
    virtual css::uno::Reference< css::io::XInputStream >
                        openFragmentStream() const;

    // binary records ---------------------------------------------------------

    virtual const RecordInfo* getRecordInfos() const;

protected:
    explicit            FragmentHandler( XmlFilterBase& rFilter, const OUString& rFragmentPath, RelationsRef xRelations );
};

typedef ::rtl::Reference< FragmentHandler > FragmentHandlerRef;


} // namespace oox::core

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
