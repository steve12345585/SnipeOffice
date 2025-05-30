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

#include <utility>
#include <vbahelper/vbadocumentsbase.hxx>

#include <unotools/mediadescriptor.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <com/sun/star/container/XEnumerationAccess.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/FrameSearchFlag.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <com/sun/star/document/MacroExecMode.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <tools/urlobj.hxx>
#include <o3tl/safeint.hxx>
#include <osl/file.hxx>
#include <unordered_map>

#include <vbahelper/vbadocumentbase.hxx>
#include <ooo/vba/XApplicationBase.hpp>

using namespace ::ooo::vba;
using namespace ::com::sun::star;

typedef  std::unordered_map< OUString,
                             sal_Int32 > NameIndexHash;

typedef std::vector < uno::Reference< frame::XModel > > Documents;

// #FIXME clearly this is a candidate for some sort of helper base class as
// this is a copy of SelectedSheetsEnum ( vbawindow.cxx )

namespace {

class DocumentsEnumImpl : public ::cppu::WeakImplHelper< container::XEnumeration >
{
    uno::Reference< uno::XComponentContext > m_xContext;
    Documents m_documents;
    Documents::const_iterator m_it;

public:
    /// @throws uno::RuntimeException
    DocumentsEnumImpl( uno::Reference< uno::XComponentContext > xContext, Documents&& docs ) :  m_xContext(std::move( xContext )), m_documents( std::move(docs) )
    {
        m_it = m_documents.begin();
    }
    /// @throws uno::RuntimeException
    explicit DocumentsEnumImpl( uno::Reference< uno::XComponentContext > xContext ) :  m_xContext(std::move( xContext ))
    {
        uno::Reference< frame::XDesktop2 > xDesktop = frame::Desktop::create( m_xContext );
        uno::Reference< container::XEnumeration > xComponents = xDesktop->getComponents()->createEnumeration();
        while( xComponents->hasMoreElements() )
        {
            uno::Reference< frame::XModel > xNext( xComponents->nextElement(), uno::UNO_QUERY );
            if ( xNext.is() )
                m_documents.push_back( xNext );
        }
        m_it = m_documents.begin();
    }
    // XEnumeration
    virtual sal_Bool SAL_CALL hasMoreElements(  ) override
    {
        return m_it != m_documents.end();
    }

    virtual uno::Any SAL_CALL nextElement(  ) override
    {
        if ( !hasMoreElements() )
        {
            throw container::NoSuchElementException();
        }
        return css::uno::Any( *(m_it++) );
    }
};

}

// #FIXME clearly this is also a candidate for some sort of helper base class as
// a very similar one is used in vbawindow ( SelectedSheetsEnumAccess )
// Maybe a template base class that does all of the operations on the hashmap
// and vector only, and the sub-class does everything else
// => ctor, createEnumeration & factory method need be defined ( to be called
// by getByIndex, getByName )
typedef ::cppu::WeakImplHelper< container::XEnumerationAccess
    , css::container::XIndexAccess
    , css::container::XNameAccess
    > DocumentsAccessImpl_BASE;

namespace {

class DocumentsAccessImpl : public DocumentsAccessImpl_BASE
{
    uno::Reference< uno::XComponentContext > m_xContext;
    Documents m_documents;
    NameIndexHash namesToIndices;
public:
    /// @throws uno::RuntimeException
    DocumentsAccessImpl( uno::Reference< uno::XComponentContext > xContext, VbaDocumentsBase::DOCUMENT_TYPE eDocType ) :m_xContext(std::move( xContext ))
    {
        uno::Reference< container::XEnumeration > xEnum = new DocumentsEnumImpl( m_xContext );
        sal_Int32 nIndex=0;
        while( xEnum->hasMoreElements() )
        {
            uno::Reference< lang::XServiceInfo > xServiceInfo( xEnum->nextElement(), uno::UNO_QUERY );
            if ( xServiceInfo.is()
                && (  ( xServiceInfo->supportsService( u"com.sun.star.sheet.SpreadsheetDocument"_ustr ) && eDocType == VbaDocumentsBase::EXCEL_DOCUMENT )
                || ( xServiceInfo->supportsService( u"com.sun.star.text.TextDocument"_ustr ) && eDocType == VbaDocumentsBase::WORD_DOCUMENT ) ) )
            {
                uno::Reference< frame::XModel > xModel( xServiceInfo, uno::UNO_QUERY_THROW ); // that the spreadsheetdocument is a xmodel is a given
                m_documents.push_back( xModel );
                OUString sName = VbaDocumentBase::getNameFromModel( xModel );
                namesToIndices[ sName ] = nIndex++;
            }
        }

    }

    //XEnumerationAccess
    virtual uno::Reference< container::XEnumeration > SAL_CALL createEnumeration(  ) override
    {
        return new DocumentsEnumImpl( m_xContext, std::vector(m_documents) );
    }
    // XIndexAccess
    virtual ::sal_Int32 SAL_CALL getCount(  ) override
    {
        return m_documents.size();
    }
    virtual uno::Any SAL_CALL getByIndex( ::sal_Int32 Index ) override
    {
        if ( Index < 0
            || o3tl::make_unsigned(Index) >= m_documents.size() )
            throw lang::IndexOutOfBoundsException();
        return css::uno::Any( m_documents[ Index ] ); // returns xspreadsheetdoc
    }

    //XElementAccess
    virtual uno::Type SAL_CALL getElementType(  ) override
    {
        return cppu::UnoType<frame::XModel>::get();
    }

    virtual sal_Bool SAL_CALL hasElements(  ) override
    {
        return (!m_documents.empty());
    }

    //XNameAccess
    virtual uno::Any SAL_CALL getByName( const OUString& aName ) override
    {
        NameIndexHash::const_iterator it = namesToIndices.find( aName );
        if ( it == namesToIndices.end() )
            throw container::NoSuchElementException();
        return css::uno::Any( m_documents[ it->second ] );

    }

    virtual uno::Sequence< OUString > SAL_CALL getElementNames(  ) override
    {
        return comphelper::mapKeysToSequence( namesToIndices );
    }

    virtual sal_Bool SAL_CALL hasByName( const OUString& aName ) override
    {
        NameIndexHash::const_iterator it = namesToIndices.find( aName );
        return (it != namesToIndices.end());
    }

};

}

VbaDocumentsBase::VbaDocumentsBase( const uno::Reference< XHelperInterface >& xParent, const uno::Reference< css::uno::XComponentContext >& xContext, DOCUMENT_TYPE eDocType ) : VbaDocumentsBase_BASE( xParent, xContext, uno::Reference< container::XIndexAccess >( new DocumentsAccessImpl( xContext, eDocType ) ) ), meDocType( eDocType )
{
}

namespace {

void lclSetupComponent( const uno::Reference< lang::XComponent >& rxComponent, bool bScreenUpdating, bool bInteractive )
{
    if( !bScreenUpdating ) try
    {
        uno::Reference< frame::XModel >( rxComponent, uno::UNO_QUERY_THROW )->lockControllers();
    }
    catch( uno::Exception& )
    {
    }

    if( bInteractive )
        return;
    try
    {
        uno::Reference< frame::XModel > xModel( rxComponent, uno::UNO_QUERY );
        if (!xModel)
            return;
        uno::Reference< frame::XController > xController( xModel->getCurrentController() );
        if (!xController)
            return;
        uno::Reference< frame::XFrame > xFrame( xController->getFrame() );
        if (!xFrame)
            return;
        uno::Reference< awt::XWindow > xWindow( xFrame->getContainerWindow() );
        if (!xWindow)
            return;
        xWindow->setEnable( false );
    }
    catch( uno::Exception& )
    {
    }
}

} // namespace

uno::Any VbaDocumentsBase::createDocument()
{
    // #163808# determine state of Application.ScreenUpdating and Application.Interactive symbols (before new document is opened)
    uno::Reference< XApplicationBase > xApplication( Application(), uno::UNO_QUERY );
    bool bScreenUpdating = !xApplication.is() || xApplication->getScreenUpdating();
    bool bInteractive = true;

    try
    {
        bInteractive = !xApplication.is() || xApplication->getInteractive();
    }
    catch( const uno::Exception& )
    {
    }

    uno::Reference< frame::XDesktop2 > xLoader = frame::Desktop::create(mxContext);
    OUString sURL;
    if( meDocType == WORD_DOCUMENT )
        sURL = "private:factory/swriter";
    else if( meDocType == EXCEL_DOCUMENT )
        sURL = "private:factory/scalc";
    else
        throw uno::RuntimeException( u"Not implemented"_ustr );

    // prepare the media descriptor
    utl::MediaDescriptor aMediaDesc;
    aMediaDesc[ utl::MediaDescriptor::PROP_MACROEXECUTIONMODE ] <<= document::MacroExecMode::USE_CONFIG;
    aMediaDesc.setComponentDataEntry( u"ApplyFormDesignMode"_ustr , uno::Any( false ) );

    // create the new document
    uno::Reference< lang::XComponent > xComponent = xLoader->loadComponentFromURL(
                                       sURL , u"_blank"_ustr, 0,
                                       aMediaDesc.getAsConstPropertyValueList() );

    // #163808# lock document controllers and container window if specified by application
    lclSetupComponent( xComponent, bScreenUpdating, bInteractive );

    return uno::Any( xComponent );
}

// #TODO# #FIXME# can any of the unused params below be used?
uno::Any VbaDocumentsBase::openDocument( const OUString& rFileName, const uno::Any& ReadOnly, const uno::Sequence< beans::PropertyValue >& rProps )
{
    // #163808# determine state of Application.ScreenUpdating and Application.Interactive symbols (before new document is opened)
    uno::Reference< XApplicationBase > xApplication( Application(), uno::UNO_QUERY );
    bool bScreenUpdating = !xApplication.is() || xApplication->getScreenUpdating();
    bool bInteractive = true;

    try
    {
        bInteractive = !xApplication.is() || xApplication->getInteractive();
    }
    catch( const uno::Exception& )
    {
    }

    // we need to detect if this is a URL, if not then assume it's a file path
    OUString aURL;
    INetURLObject aObj;
    aObj.SetURL( rFileName );
    bool bIsURL = aObj.GetProtocol() != INetProtocol::NotValid;
    if ( bIsURL )
        aURL = rFileName;
    else
        osl::FileBase::getFileURLFromSystemPath( rFileName, aURL );
    uno::Reference< frame::XDesktop2 > xDesktop = frame::Desktop::create( mxContext );

    uno::Sequence< beans::PropertyValue > sProps( rProps );
    sProps.realloc( sProps.getLength() + 1 );
    auto pProps = sProps.getArray();
    pProps[ sProps.getLength() - 1 ].Name = "MacroExecutionMode";
    pProps[ sProps.getLength() - 1 ].Value <<= document::MacroExecMode::ALWAYS_EXECUTE_NO_WARN;

    if ( ReadOnly.hasValue()  )
    {
        bool bIsReadOnly = false;
        ReadOnly >>= bIsReadOnly;
        if ( bIsReadOnly )
        {
            sProps.realloc( sProps.getLength() + 1 );
            pProps = sProps.getArray();
            pProps[ sProps.getLength() - 1 ].Name = "ReadOnly";
            pProps[ sProps.getLength() - 1 ].Value <<= true;
        }
    }

    uno::Reference< lang::XComponent > xComponent = xDesktop->loadComponentFromURL( aURL,
        u"_default"_ustr ,
        frame::FrameSearchFlag::CREATE,
        sProps);

    // #163808# lock document controllers and container window if specified by application
    lclSetupComponent( xComponent, bScreenUpdating, bInteractive );

    return uno::Any( xComponent );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
