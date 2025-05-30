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

#include <sal/log.hxx>
#include <utility>
#include <vcl/unohelp2.hxx>
#include <sot/exchange.hxx>
#include <sot/formats.hxx>
#include <vcl/svapp.hxx>
#include <com/sun/star/datatransfer/UnsupportedFlavorException.hpp>
#include <com/sun/star/datatransfer/clipboard/XClipboard.hpp>
#include <com/sun/star/datatransfer/clipboard/XFlushableClipboard.hpp>
#include <cppuhelper/queryinterface.hxx>
#include <boost/property_tree/json_parser.hpp>
#include <comphelper/lok.hxx>
#include <LibreOfficeKit/LibreOfficeKitEnums.h>

using namespace ::com::sun::star;

namespace vcl::unohelper {

    TextDataObject::TextDataObject( OUString aText ) : maText(std::move( aText ))
    {
    }

    TextDataObject::~TextDataObject()
    {
    }

    void TextDataObject::CopyStringTo( const OUString& rContent,
        const uno::Reference< datatransfer::clipboard::XClipboard >& rxClipboard,
        const vcl::ILibreOfficeKitNotifier* pNotifier)
    {
        SAL_WARN_IF( !rxClipboard.is(), "vcl", "TextDataObject::CopyStringTo: invalid clipboard!" );
        if ( !rxClipboard.is() )
            return;

        rtl::Reference<TextDataObject> pDataObj = new TextDataObject( rContent );

        SolarMutexReleaser aReleaser;
        try
        {
            rxClipboard->setContents( pDataObj, nullptr );

            uno::Reference< datatransfer::clipboard::XFlushableClipboard > xFlushableClipboard( rxClipboard, uno::UNO_QUERY );
            if( xFlushableClipboard.is() )
                xFlushableClipboard->flushClipboard();

            if (pNotifier != nullptr && comphelper::LibreOfficeKit::isActive())
            {
                boost::property_tree::ptree aTree;
                aTree.put("content", rContent);
                aTree.put("mimeType", "text/plain");
                std::stringstream aStream;
                boost::property_tree::write_json(aStream, aTree);
                pNotifier->libreOfficeKitViewCallback(LOK_CALLBACK_CLIPBOARD_CHANGED, OString(aStream.str()));
            }
        }
        catch( const uno::Exception& )
        {
        }
    }

    // css::uno::XInterface
    uno::Any TextDataObject::queryInterface( const uno::Type & rType )
    {
        uno::Any aRet = ::cppu::queryInterface( rType, static_cast< datatransfer::XTransferable* >(this) );
        return (aRet.hasValue() ? aRet : OWeakObject::queryInterface( rType ));
    }

    // css::datatransfer::XTransferable
    uno::Any TextDataObject::getTransferData( const datatransfer::DataFlavor& rFlavor )
    {
        SotClipboardFormatId nT = SotExchange::GetFormat( rFlavor );
        if ( nT != SotClipboardFormatId::STRING )
        {
            throw datatransfer::UnsupportedFlavorException();
        }
        return uno::Any(maText);
    }

    uno::Sequence< datatransfer::DataFlavor > TextDataObject::getTransferDataFlavors(  )
    {
        uno::Sequence< datatransfer::DataFlavor > aDataFlavors(1);
        SotExchange::GetFormatDataFlavor( SotClipboardFormatId::STRING, aDataFlavors.getArray()[0] );
        return aDataFlavors;
    }

    sal_Bool TextDataObject::isDataFlavorSupported( const datatransfer::DataFlavor& rFlavor )
    {
        SotClipboardFormatId nT = SotExchange::GetFormat( rFlavor );
        return ( nT == SotClipboardFormatId::STRING );
    }

}  // namespace vcl::unohelper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
