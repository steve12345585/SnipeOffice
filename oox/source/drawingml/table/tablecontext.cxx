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

#include <oox/helper/attributelist.hxx>
#include <drawingml/colorchoicecontext.hxx>
#include <drawingml/guidcontext.hxx>
#include <drawingml/table/tablecontext.hxx>
#include <drawingml/table/tableproperties.hxx>
#include <drawingml/table/tablestylecontext.hxx>
#include <drawingml/table/tablerowcontext.hxx>
#include <drawingml/effectpropertiescontext.hxx>
#include <oox/token/namespaces.hxx>
#include <oox/token/tokens.hxx>

using namespace ::oox::core;
using namespace ::com::sun::star;

namespace oox::drawingml::table {

TableContext::TableContext( ContextHandler2Helper const & rParent, const ShapePtr& pShapePtr )
: ShapeContext( rParent, ShapePtr(), pShapePtr )
, mrTableProperties( *pShapePtr->getTableProperties() )
{
    pShapePtr->setTableType();
}

TableContext::~TableContext()
{
}

ContextHandlerRef
TableContext::onCreateContext( ::sal_Int32 aElementToken, const AttributeList& rAttribs )
{
    switch( aElementToken )
    {
    case A_TOKEN( tblPr ):              // CT_TableProperties
        {
            mrTableProperties.setFirstRow( rAttribs.getBool( XML_firstRow, false ) );
            mrTableProperties.setFirstCol( rAttribs.getBool( XML_firstCol, false ) );
            mrTableProperties.setLastRow( rAttribs.getBool( XML_lastRow, false ) );
            mrTableProperties.setLastCol( rAttribs.getBool( XML_lastCol, false ) );
            mrTableProperties.setBandRow( rAttribs.getBool( XML_bandRow, false ) );
            mrTableProperties.setBandCol( rAttribs.getBool( XML_bandCol, false ) );
        }
        break;
    case A_TOKEN(solidFill):
        return new ColorContext(*this, mrTableProperties.getBgColor());
    case A_TOKEN( tableStyle ):         // CT_TableStyle
        {
            std::shared_ptr< TableStyle >& rTableStyle = mrTableProperties.getTableStyle();
            rTableStyle = std::make_shared<TableStyle>();
            return new TableStyleContext( *this, rAttribs, *rTableStyle );
        }
    case A_TOKEN( effectLst ):  // CT_EffectList
        {
            return new EffectPropertiesContext(*this, mpShapePtr->getEffectProperties());
        }
    case A_TOKEN( tableStyleId ):       // ST_Guid
        return new oox::drawingml::GuidContext( *this, mrTableProperties.getStyleId() );

    case A_TOKEN( tblGrid ):            // CT_TableGrid
        break;
    case A_TOKEN( gridCol ):            // CT_TableCol
        {
            std::vector< sal_Int32 >& rvTableGrid( mrTableProperties.getTableGrid() );
            rvTableGrid.push_back( rAttribs.getInteger( XML_w, 0 ) );
        }
        break;
    case A_TOKEN( tr ):                 // CT_TableRow
        {
            std::vector< TableRow >& rvTableRows( mrTableProperties.getTableRows() );
            rvTableRows.emplace_back();
            return new TableRowContext( *this, rAttribs, rvTableRows.back() );
        }
    }

    return this;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
