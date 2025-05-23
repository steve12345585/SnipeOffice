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

#ifndef INCLUDED_OOX_CORE_FILTERBASE_HXX
#define INCLUDED_OOX_CORE_FILTERBASE_HXX

#include <memory>

#include <com/sun/star/document/XExporter.hpp>
#include <com/sun/star/document/XFilter.hpp>
#include <com/sun/star/document/XImporter.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <oox/dllapi.h>
#include <oox/helper/binarystreambase.hxx>
#include <oox/helper/storagebase.hxx>
#include <rtl/ustring.hxx>
#include <sal/types.h>

namespace com::sun::star {
    namespace beans { struct PropertyValue; }
    namespace frame { class XFrame; }
    namespace frame { class XModel; }
    namespace io { class XInputStream; }
    namespace io { class XOutputStream; }
    namespace io { class XStream; }
    namespace lang { class XMultiServiceFactory; }
    namespace task { class XStatusIndicator; }
    namespace uno { class XComponentContext; }
}

namespace comphelper {
    class SequenceAsHashMap;
}
namespace utl {
    class MediaDescriptor;
}

namespace oox {
    class GraphicHelper;
    class ModelObjectHelper;
}

namespace oox::ole {
    class OleObjectHelper;
    class VbaProject;
}

namespace oox::core {

enum OoxmlVersion
{
    /** There are currently 5 editions of ECMA-376, latest is from 2021.
      * 1st edition allegedly corresponds to Word 2007
      * 2nd edition allegedly corresponds to ISO 29500:2008
      * it's unclear what changed in later editions; there is:
        Annex M.  Differences Between ECMA-376:2016 and ECMA-376:2006
        but that's relative to 1st edition.
    */
    ECMA_376_1ST_EDITION,
    ISOIEC_29500_2008
};

struct FilterBaseImpl;

typedef ::cppu::WeakImplHelper<
        css::lang::XServiceInfo,
        css::lang::XInitialization,
        css::document::XImporter,
        css::document::XExporter,
        css::document::XFilter >
    FilterBase_BASE;

class OOX_DLLPUBLIC FilterBase : public FilterBase_BASE
{
public:
    /// @throws css::uno::RuntimeException
    explicit            FilterBase(
                            const css::uno::Reference< css::uno::XComponentContext >& rxContext );

    virtual             ~FilterBase() override;

    /** Returns true, if filter is an import filter. */
    bool                isImportFilter() const;
    /** Returns true, if filter is an export filter. */
    bool                isExportFilter() const;

    OoxmlVersion getVersion() const;

    /** Derived classes implement import of the entire document. */
    virtual bool        importDocument() = 0;

    /** Derived classes implement export of the entire document. */
    virtual bool        exportDocument() = 0;


    /** Returns the component context passed in the filter constructor (always existing). */
    const css::uno::Reference< css::uno::XComponentContext >&
                        getComponentContext() const;

    /** Returns the document model (always existing). */
    const css::uno::Reference< css::frame::XModel >&
                        getModel() const;

    /** Returns the service factory provided by the document model (always existing). */
    const css::uno::Reference< css::lang::XMultiServiceFactory >&
                        getModelFactory() const;

    /** Returns the frame that will contain the document model (may be null). */
    const css::uno::Reference< css::frame::XFrame >&
                        getTargetFrame() const;

    /** Returns the status indicator (may be null). */
    const css::uno::Reference< css::task::XStatusIndicator >&
                        getStatusIndicator() const;

    /** Returns the FilterData */
    ::comphelper::SequenceAsHashMap& getFilterData() const;

    /** Returns the media descriptor. */
    utl::MediaDescriptor& getMediaDescriptor() const;

    /** Returns the URL of the imported or exported file. */
    const OUString& getFileUrl() const;

    /** Returns an absolute URL for the passed relative or absolute URL. */
    OUString     getAbsoluteUrl( const OUString& rUrl ) const;

    /** Returns the base storage of the imported/exported file. */
    StorageRef const & getStorage() const;

    /** Opens and returns the specified input stream from the base storage.

        @param rStreamName
            The name of the embedded storage stream. The name may contain
            slashes to open streams from embedded substorages. If base stream
            access has been enabled in the storage, the base stream can be
            accessed by passing an empty string as stream name.
     */
    css::uno::Reference< css::io::XInputStream >
                        openInputStream( const OUString& rStreamName ) const;

    /** Opens and returns the specified output stream from the base storage.

        @param rStreamName
            The name of the embedded storage stream. The name may contain
            slashes to open streams from embedded substorages. If base stream
            access has been enabled in the storage, the base stream can be
            accessed by passing an empty string as stream name.
     */
    css::uno::Reference< css::io::XOutputStream >
                        openOutputStream( const OUString& rStreamName ) const;

    /** Commits changes to base storage (and substorages) */
    void                commitStorage() const;

    // helpers ----------------------------------------------------------------

    /** Returns a helper for the handling of graphics and graphic objects. */
    GraphicHelper&      getGraphicHelper() const;

    /** Returns a helper with containers for various named drawing objects for
        the imported document. */
    ModelObjectHelper&  getModelObjectHelper() const;

    ModelObjectHelper& getModelObjectHelperForModel(
        const css::uno::Reference<css::lang::XMultiServiceFactory>& xFactory) const;

    /** Returns a helper for the handling of OLE objects. */
    ::oox::ole::OleObjectHelper& getOleObjectHelper() const;

    /** Returns the VBA project manager. */
    ::oox::ole::VbaProject& getVbaProject() const;

    /** Imports the raw binary data from the specified stream.
        @return  True, if the data could be imported from the stream. */
    bool                importBinaryData( StreamDataSequence & orDataSeq, const OUString& rStreamName );

    // com.sun.star.lang.XServiceInfo interface -------------------------------

    virtual sal_Bool SAL_CALL
                        supportsService( const OUString& rServiceName ) override;

    virtual css::uno::Sequence< OUString > SAL_CALL
                        getSupportedServiceNames() override;

    // com.sun.star.lang.XInitialization interface ----------------------------

    /** Receives user defined arguments.

        @param rArgs
            the sequence of arguments passed to the filter. The implementation
            expects one or two arguments. The first argument shall be the
            com.sun.star.lang.XMultiServiceFactory interface of the global
            service factory. The optional second argument may contain a
            sequence of com.sun.star.beans.NamedValue objects. The different
            filter implementations may support different arguments.
     */
    virtual void SAL_CALL initialize(
                            const css::uno::Sequence< css::uno::Any >& rArgs ) override;

    // com.sun.star.document.XImporter interface ------------------------------

    virtual void SAL_CALL setTargetDocument(
                            const css::uno::Reference< css::lang::XComponent >& rxDocument ) override;

    // com.sun.star.document.XExporter interface ------------------------------

    virtual void SAL_CALL setSourceDocument(
                            const css::uno::Reference< css::lang::XComponent >& rxDocument ) override;

    // com.sun.star.document.XFilter interface --------------------------------

    virtual sal_Bool SAL_CALL filter(
                            const css::uno::Sequence< css::beans::PropertyValue >& rMediaDescSeq ) override;

    virtual void SAL_CALL cancel() override;

    bool exportVBA() const;

    bool isExportTemplate() const;

protected:
    virtual css::uno::Reference< css::io::XInputStream >
                        implGetInputStream( utl::MediaDescriptor& rMediaDesc ) const;
    virtual css::uno::Reference< css::io::XStream >
                        implGetOutputStream( utl::MediaDescriptor& rMediaDesc ) const;

    virtual bool        implFinalizeExport( utl::MediaDescriptor& rMediaDescriptor );

    css::uno::Reference< css::io::XStream > const &
                        getMainDocumentStream( ) const;

private:
    void                setMediaDescriptor(
                            const css::uno::Sequence< css::beans::PropertyValue >& rMediaDescSeq );

    /** Derived classes may create a specialized graphic helper, e.g. for
        resolving palette colors. */
    virtual GraphicHelper* implCreateGraphicHelper() const;

    /** Derived classes create a VBA project manager object. */
    virtual ::oox::ole::VbaProject* implCreateVbaProject() const = 0;

    virtual StorageRef  implCreateStorage(
                            const css::uno::Reference< css::io::XInputStream >& rxInStream ) const = 0;
    virtual StorageRef  implCreateStorage(
                            const css::uno::Reference< css::io::XStream >& rxOutStream ) const = 0;

private:
    std::unique_ptr< FilterBaseImpl > mxImpl;
};

} // namespace oox::core

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
