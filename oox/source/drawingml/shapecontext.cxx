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

#include <com/sun/star/xml/sax/FastToken.hpp>
#include <com/sun/star/drawing/LineStyle.hpp>
#include <com/sun/star/beans/XMultiPropertySet.hpp>

#include <oox/helper/attributelist.hxx>
#include <oox/drawingml/shapecontext.hxx>
#include <drawingml/shapepropertiescontext.hxx>
#include <drawingml/shapestylecontext.hxx>
#include <oox/drawingml/drawingmltypes.hxx>
#include <drawingml/textbodycontext.hxx>
#include <drawingml/textbodypropertiescontext.hxx>
#include "hyperlinkcontext.hxx"
#include <oox/token/namespaces.hxx>
#include <oox/token/tokens.hxx>
#include <sal/log.hxx>
#include <drawingml/transform2dcontext.hxx>
#include <utility>

using namespace oox::core;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;

namespace oox::drawingml {

// CT_Shape
ShapeContext::ShapeContext( ContextHandler2Helper const & rParent, ShapePtr pMasterShapePtr, ShapePtr pShapePtr )
: ContextHandler2( rParent )
, mpMasterShapePtr(std::move( pMasterShapePtr ))
, mpShapePtr(std::move( pShapePtr ))
{
    if( mpMasterShapePtr && mpShapePtr )
        mpMasterShapePtr->addChild( mpShapePtr );
}

ShapeContext::~ShapeContext()
{
}

ContextHandlerRef ShapeContext::onCreateContext( sal_Int32 aElementToken, const AttributeList& rAttribs )
{
    switch( getBaseToken( aElementToken ) )
    {
    // nvSpPr CT_ShapeNonVisual begin
//  case XML_drElemPr:
//      break;
    case XML_extLst:
    case XML_ext:
        break;
    case XML_decorative:
        {
            mpShapePtr->setDecorative(rAttribs.getBool(XML_val, false));
        }
        break;
    case XML_cNvPr:
    {
        mpShapePtr->setHidden( rAttribs.getBool( XML_hidden, false ) );
        mpShapePtr->setId( rAttribs.getStringDefaulted( XML_id ) );
        mpShapePtr->setName( rAttribs.getStringDefaulted( XML_name ) );
        mpShapePtr->setDescription( rAttribs.getStringDefaulted( XML_descr ) );
        mpShapePtr->setTitle(rAttribs.getStringDefaulted(XML_title));
        break;
    }
    case XML_hlinkMouseOver:
    case XML_hlinkClick:
        return new HyperLinkContext( *this, rAttribs,  getShape()->getShapeProperties() );
    case XML_ph:
        mpShapePtr->setSubType( rAttribs.getToken( XML_type, XML_obj ) );
        if( rAttribs.hasAttribute( XML_idx ) )
            mpShapePtr->setSubTypeIndex( rAttribs.getInteger( XML_idx, 0 ) );
        break;
    // nvSpPr CT_ShapeNonVisual end

    case XML_spPr:
        return new ShapePropertiesContext( *this, *mpShapePtr );

    case XML_style:
        return new ShapeStyleContext( *this, *mpShapePtr );

    case XML_txBody:
    case XML_txbxContent:
    {
        if (!mpShapePtr->getTextBody())
            mpShapePtr->setTextBody( std::make_shared<TextBody>() );
        return new TextBodyContext( *this, mpShapePtr );
    }
    case XML_txXfrm: // diagram shape. [MS-ODRAWXML]
    {
        const TextBodyPtr& rShapePtr = mpShapePtr->getTextBody();
        if (rShapePtr)
            return new oox::drawingml::Transform2DContext( *this, rAttribs, *mpShapePtr, true );
    }
        break;
    case XML_cNvSpPr:
        break;
    case XML_spLocks:
        break;
    case XML_bodyPr:
        if (!mpShapePtr->getTextBody())
            mpShapePtr->setTextBody( std::make_shared<TextBody>() );
        return new TextBodyPropertiesContext( *this, rAttribs, mpShapePtr );
    case XML_txbx:
        break;
    case XML_cNvPicPr:
        break;
    case XML_nvPicPr:
    case XML_picLocks:
        break;
    case XML_relIds:
        break;
    case XML_nvSpPr:
        break;
    default:
        SAL_INFO("oox", "ShapeContext::onCreateContext: unhandled element: " << getBaseToken(aElementToken));
        break;
    }

    return this;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
