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

#include <rtl/ustring.hxx>
#include <unotools/ucbstreamhelper.hxx>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/task/InteractionHandler.hpp>
#include <com/sun/star/ucb/ContentCreationException.hpp>
#include <com/sun/star/ucb/CommandAbortedException.hpp>
#include <com/sun/star/ucb/InsertCommandArgument.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>

#include <comphelper/simplefileaccessinteraction.hxx>
#include <ucbhelper/content.hxx>
#include <unotools/streamwrap.hxx>
#include "ucblockbytes.hxx"

namespace com::sun::star::ucb { class XCommandEnvironment; }

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::ucb;
using namespace ::com::sun::star::task;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::beans;

namespace utl
{

static std::unique_ptr<SvStream> lcl_CreateStream( const OUString& rFileName, StreamMode eOpenMode,
                                   const Reference < XInteractionHandler >& xInteractionHandler,
                                   bool bEnsureFileExists )
{
    std::unique_ptr<SvStream> pStream;
    UcbLockBytesRef xLockBytes;
    if ( eOpenMode & StreamMode::WRITE )
    {
        bool bTruncate = bool( eOpenMode & StreamMode::TRUNC );
        if ( bTruncate )
        {
            try
            {
                // truncate is implemented with deleting the original file
                ::ucbhelper::Content aCnt(
                    rFileName, Reference < XCommandEnvironment >(),
                    comphelper::getProcessComponentContext() );
                aCnt.executeCommand( u"delete"_ustr, css::uno::Any( true ) );
            }

            catch ( const CommandAbortedException& )
            {
                // couldn't truncate/delete
            }
            catch ( const ContentCreationException& )
            {
            }
            catch ( const Exception& )
            {
            }
        }

        if ( bEnsureFileExists || bTruncate )
        {
            try
            {
                // make sure that the desired file exists before trying to open
                SvMemoryStream aStream(0,0);
                rtl::Reference<::utl::OInputStreamWrapper> xInput = new ::utl::OInputStreamWrapper( aStream );

                ::ucbhelper::Content aContent(
                    rFileName, Reference < XCommandEnvironment >(),
                    comphelper::getProcessComponentContext() );
                InsertCommandArgument aInsertArg;
                aInsertArg.Data = xInput;

                aInsertArg.ReplaceExisting = false;
                Any aCmdArg;
                aCmdArg <<= aInsertArg;
                aContent.executeCommand( u"insert"_ustr, aCmdArg );
            }

            // it is NOT an error when the stream already exists and no truncation was desired
            catch ( const CommandAbortedException& )
            {
                // currently never an error is detected !
            }
            catch ( const ContentCreationException& )
            {
            }
            catch ( const Exception& )
            {
            }
        }
    }

    try
    {
        // create LockBytes using UCB
        ::ucbhelper::Content aContent(
            rFileName, Reference < XCommandEnvironment >(),
            comphelper::getProcessComponentContext() );
        xLockBytes = UcbLockBytes::CreateLockBytes( aContent.get(), Sequence < PropertyValue >(),
                                                    eOpenMode, xInteractionHandler );
        if ( xLockBytes.is() )
        {
            pStream.reset( new SvStream( xLockBytes.get() ) );
            pStream->SetBufferSize( 4096 );
            pStream->SetError( xLockBytes->GetError() );
        }
    }
    catch ( const CommandAbortedException& )
    {
    }
    catch ( const ContentCreationException& )
    {
    }
    catch ( const Exception& )
    {
    }

    return pStream;
}

std::unique_ptr<SvStream>
UcbStreamHelper::CreateStream(const OUString& rFileName, StreamMode eOpenMode,
                              const css::uno::Reference<css::awt::XWindow>& xParentWin,
                              bool bUseSimpleFileAccessInteraction)
{
    // related tdf#99312
    // create a specialized interaction handler to manages Web certificates and Web credentials when needed
    Reference< XInteractionHandler > xIH(
        css::task::InteractionHandler::createWithParent(comphelper::getProcessComponentContext(), xParentWin));

    if (!bUseSimpleFileAccessInteraction)
        return lcl_CreateStream(rFileName, eOpenMode, xIH, true /* bEnsureFileExists */);

    Reference<XInteractionHandler> xIHScoped(new comphelper::SimpleFileAccessInteraction(xIH));

    return lcl_CreateStream( rFileName, eOpenMode, xIHScoped, true /* bEnsureFileExists */ );
}

std::unique_ptr<SvStream>
UcbStreamHelper::CreateStream(const OUString& rFileName, StreamMode eOpenMode, bool bFileExists,
                              const css::uno::Reference<css::awt::XWindow> & xParentWin,
                              bool bUseSimpleFileAccessInteraction)
{
    // related tdf#99312
    // create a specialized interaction handler to manages Web certificates and Web credentials when needed
    Reference< XInteractionHandler > xIH(
        css::task::InteractionHandler::createWithParent(comphelper::getProcessComponentContext(), xParentWin));

    if (!bUseSimpleFileAccessInteraction)
        return lcl_CreateStream(rFileName, eOpenMode, xIH, !bFileExists);

    Reference<XInteractionHandler> xIHScoped(new comphelper::SimpleFileAccessInteraction(xIH));
    return lcl_CreateStream( rFileName, eOpenMode, xIHScoped,!bFileExists );
}


std::unique_ptr<SvStream> UcbStreamHelper::CreateStream( const Reference < XInputStream >& xStream )
{
    std::unique_ptr<SvStream> pStream;
    UcbLockBytesRef xLockBytes = UcbLockBytes::CreateInputLockBytes( xStream );
    if ( xLockBytes.is() )
    {
        pStream.reset( new SvStream( xLockBytes.get() ) );
        pStream->SetBufferSize( 4096 );
        pStream->SetError( xLockBytes->GetError() );
    }

    return pStream;
}

std::unique_ptr<SvStream> UcbStreamHelper::CreateStream( const Reference < XStream >& xStream )
{
    std::unique_ptr<SvStream> pStream;
    if ( xStream->getOutputStream().is() )
    {
        UcbLockBytesRef xLockBytes = UcbLockBytes::CreateLockBytes( xStream );
        if ( xLockBytes.is() )
        {
            pStream.reset( new SvStream( xLockBytes.get() ) );
            pStream->SetBufferSize( 4096 );
            pStream->SetError( xLockBytes->GetError() );
        }
    }
    else
        return CreateStream( xStream->getInputStream() );

    return pStream;
}

std::unique_ptr<SvStream> UcbStreamHelper::CreateStream( const Reference < XInputStream >& xStream, bool bCloseStream )
{
    std::unique_ptr<SvStream> pStream;
    UcbLockBytesRef xLockBytes = UcbLockBytes::CreateInputLockBytes( xStream );
    if ( xLockBytes.is() )
    {
        if ( !bCloseStream )
            xLockBytes->setDontClose();

        pStream.reset( new SvStream( xLockBytes.get() ) );
        pStream->SetBufferSize( 4096 );
        pStream->SetError( xLockBytes->GetError() );
    }

    return pStream;
};

std::unique_ptr<SvStream> UcbStreamHelper::CreateStream( const Reference < XStream >& xStream, bool bCloseStream )
{
    std::unique_ptr<SvStream> pStream;
    if ( xStream->getOutputStream().is() )
    {
        UcbLockBytesRef xLockBytes = UcbLockBytes::CreateLockBytes( xStream );
        if ( xLockBytes.is() )
        {
            if ( !bCloseStream )
                xLockBytes->setDontClose();

            pStream.reset( new SvStream( xLockBytes.get() ) );
            pStream->SetBufferSize( 4096 );
            pStream->SetError( xLockBytes->GetError() );
        }
    }
    else
        return CreateStream( xStream->getInputStream(), bCloseStream );

    return pStream;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
