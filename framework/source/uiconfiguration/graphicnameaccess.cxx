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

#include <uiconfiguration/graphicnameaccess.hxx>

#include <comphelper/sequence.hxx>

using namespace ::com::sun::star;

namespace framework
{

GraphicNameAccess::GraphicNameAccess()
{
}

GraphicNameAccess::~GraphicNameAccess()
{
}

void GraphicNameAccess::addElement( const OUString& rName, const uno::Reference< graphic::XGraphic >& rElement )
{
    m_aNameToElementMap.emplace( rName, rElement );
}

// XNameAccess
uno::Any SAL_CALL GraphicNameAccess::getByName( const OUString& aName )
{
    NameGraphicHashMap::const_iterator pIter = m_aNameToElementMap.find( aName );
    if ( pIter == m_aNameToElementMap.end() )
        throw container::NoSuchElementException();
    return uno::Any( pIter->second );
}

uno::Sequence< OUString > SAL_CALL GraphicNameAccess::getElementNames()
{
    if ( !m_aSeq.hasElements() )
    {
        m_aSeq = comphelper::mapKeysToSequence(m_aNameToElementMap);
    }

    return m_aSeq;
}

sal_Bool SAL_CALL GraphicNameAccess::hasByName( const OUString& aName )
{
    NameGraphicHashMap::const_iterator pIter = m_aNameToElementMap.find( aName );
    return ( pIter != m_aNameToElementMap.end() );
}

// XElementAccess
sal_Bool SAL_CALL GraphicNameAccess::hasElements()
{
    return ( !m_aNameToElementMap.empty() );
}

uno::Type SAL_CALL GraphicNameAccess::getElementType()
{
    return cppu::UnoType<graphic::XGraphic>::get();
}

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
