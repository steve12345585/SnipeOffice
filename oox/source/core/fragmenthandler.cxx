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

#include <oox/core/fragmenthandler.hxx>

#include <oox/core/xmlfilterbase.hxx>
#include <utility>

namespace oox::core {

using namespace ::com::sun::star::io;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::xml::sax;

FragmentBaseData::FragmentBaseData( XmlFilterBase& rFilter, OUString aFragmentPath, RelationsRef xRelations ) :
    mrFilter( rFilter ),
    maFragmentPath(std::move( aFragmentPath )),
    mxRelations(std::move( xRelations ))
{
}

FragmentHandler::FragmentHandler( XmlFilterBase& rFilter, const OUString& rFragmentPath ) :
    FragmentHandler_BASE( std::make_shared<FragmentBaseData>( rFilter, rFragmentPath, rFilter.importRelations( rFragmentPath ) ) )
{
}

FragmentHandler::FragmentHandler( XmlFilterBase& rFilter, const OUString& rFragmentPath, RelationsRef xRelations ) :
    FragmentHandler_BASE( std::make_shared<FragmentBaseData>( rFilter, rFragmentPath, xRelations ) )
{
}

FragmentHandler::~FragmentHandler()
{
}

// com.sun.star.xml.sax.XFastDocumentHandler interface ------------------------

void FragmentHandler::startDocument()
{
}

void FragmentHandler::endDocument()
{
}

void FragmentHandler::processingInstruction( const OUString& /*rTarget*/, const OUString& /*rData*/ )
{
}

void FragmentHandler::setDocumentLocator( const Reference< XLocator >& rxLocator )
{
    implSetLocator( rxLocator );
}

// com.sun.star.xml.sax.XFastContextHandler interface -------------------------

void FragmentHandler::startFastElement( sal_Int32, const Reference< XFastAttributeList >& )
{
}

void FragmentHandler::startUnknownElement( const OUString&, const OUString&, const Reference< XFastAttributeList >& )
{
}

void FragmentHandler::endFastElement( sal_Int32 )
{
}

void FragmentHandler::endUnknownElement( const OUString&, const OUString& )
{
}

Reference< XFastContextHandler > FragmentHandler::createFastChildContext( sal_Int32, const Reference< XFastAttributeList >& )
{
    return nullptr;
}

Reference< XFastContextHandler > FragmentHandler::createUnknownChildContext( const OUString&, const OUString&, const Reference< XFastAttributeList >& )
{
    return nullptr;
}

void FragmentHandler::characters( const OUString& )
{
}

// XML stream handling --------------------------------------------------------

Reference< XInputStream > FragmentHandler::openFragmentStream() const
{
    return getFilter().openInputStream( getFragmentPath() );
}

// binary records -------------------------------------------------------------

const RecordInfo* FragmentHandler::getRecordInfos() const
{
    // default: no support for binary records
    return nullptr;
}

} // namespace oox::core

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
