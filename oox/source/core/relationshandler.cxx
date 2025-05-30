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

#include <sal/config.h>

#include <string_view>

#include <oox/core/relationshandler.hxx>

#include <sal/log.hxx>
#include <oox/helper/attributelist.hxx>
#include <oox/token/namespaces.hxx>
#include <oox/token/tokens.hxx>

namespace oox::core {

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::xml::sax;

namespace {

/*  Build path to relations file from passed fragment path, e.g.:
        'path/path/file.xml'    -> 'path/path/_rels/file.xml.rels'
        'file.xml'              -> '_rels/file.xml.rels'
        ''                      -> '_rels/.rels'
 */
OUString lclGetRelationsPath( std::u16string_view rFragmentPath )
{
    size_t nPathLen = rFragmentPath.rfind( '/' );
    if (nPathLen == std::u16string_view::npos)
        nPathLen = 0;
    else
        ++nPathLen;
    return OUString::Concat(rFragmentPath.substr(0, nPathLen)) +    // file path including slash
        "_rels/" +                                // additional '_rels/' path
        rFragmentPath.substr(nPathLen) +  // file name after path
        ".rels";                                 // '.rels' suffix
}

} // namespace

RelationsFragment::RelationsFragment( XmlFilterBase& rFilter, const RelationsRef& xRelations ) :
    FragmentHandler( rFilter, lclGetRelationsPath( xRelations->getFragmentPath() ), xRelations ),
    mxRelations( xRelations )
{
}

Reference< XFastContextHandler > RelationsFragment::createFastChildContext(
        sal_Int32 nElement, const Reference< XFastAttributeList >& rxAttribs )
{
    Reference< XFastContextHandler > xRet;
    AttributeList aAttribs( rxAttribs );
    switch( nElement )
    {
        case PR_TOKEN( Relationship ):
        {
            Relation aRelation;
            aRelation.maId     = aAttribs.getStringDefaulted( XML_Id);
            aRelation.maType   = aAttribs.getStringDefaulted( XML_Type);
            aRelation.maTarget = aAttribs.getStringDefaulted( XML_Target);
            if( !aRelation.maId.isEmpty() && !aRelation.maType.isEmpty() && !aRelation.maTarget.isEmpty() )
            {
                sal_Int32 nTargetMode = aAttribs.getToken( XML_TargetMode, XML_Internal );
                SAL_WARN_IF( (nTargetMode != XML_Internal) && (nTargetMode != XML_External), "oox",
                    "RelationsFragment::createFastChildContext - unexpected target mode, assuming external" );
                aRelation.mbExternal = nTargetMode != XML_Internal;

                SAL_WARN_IF( mxRelations->count( aRelation.maId ) != 0, "oox",
                    "RelationsFragment::createFastChildContext - relation identifier exists already" );
                mxRelations->emplace( aRelation.maId, aRelation );
            }
        }
        break;
        case PR_TOKEN( Relationships ):
            xRet = getFastContextHandler();
        break;
    }
    return xRet;
}

} // namespace oox::core

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
