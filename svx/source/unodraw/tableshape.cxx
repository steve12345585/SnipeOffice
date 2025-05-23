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

#include <UnoGraphicExporter.hxx>
#include "shapeimpl.hxx"
#include <svx/unodraw/SvxTableShape.hxx>
#include <svx/unoshprp.hxx>
#include <svx/svdotable.hxx>
#include <svx/svdpool.hxx>


using namespace sdr::table;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::container;

SvxTableShape::SvxTableShape(SdrObject* pObj)
:   SvxShape( pObj, getSvxMapProvider().GetMap(SVXMAP_TABLE), getSvxMapProvider().GetPropertySet(SVXMAP_TABLE, SdrObject::GetGlobalDrawObjectItemPool()) )
{
    SetShapeType( u"com.sun.star.drawing.TableShape"_ustr );
}

SvxTableShape::~SvxTableShape() noexcept
{
}

bool SvxTableShape::setPropertyValueImpl(
    const OUString& rName,
    const SfxItemPropertyMapEntry* pProperty,
    const css::uno::Any& rValue )
{
    switch( pProperty->nWID )
    {
    case OWN_ATTR_TABLETEMPLATE:
    {
        Reference< XIndexAccess > xTemplate;

        if( !(rValue >>= xTemplate) )
            throw IllegalArgumentException();

        if( HasSdrObject() )
            static_cast< sdr::table::SdrTableObj* >( GetSdrObject() )->setTableStyle(xTemplate);

        return true;
    }
    case OWN_ATTR_TABLETEMPLATE_FIRSTROW:
    case OWN_ATTR_TABLETEMPLATE_LASTROW:
    case OWN_ATTR_TABLETEMPLATE_FIRSTCOLUMN:
    case OWN_ATTR_TABLETEMPLATE_LASTCOLUMN:
    case OWN_ATTR_TABLETEMPLATE_BANDINGROWS:
    case OWN_ATTR_TABLETEMPLATE_BANDINGCOLUMNS:
    {
        if( HasSdrObject() )
        {
            TableStyleSettings aSettings( static_cast< sdr::table::SdrTableObj* >( GetSdrObject() )->getTableStyleSettings() );

            switch( pProperty->nWID )
            {
            case OWN_ATTR_TABLETEMPLATE_FIRSTROW:           rValue >>= aSettings.mbUseFirstRow; break;
            case OWN_ATTR_TABLETEMPLATE_LASTROW:            rValue >>= aSettings.mbUseLastRow; break;
            case OWN_ATTR_TABLETEMPLATE_FIRSTCOLUMN:        rValue >>= aSettings.mbUseFirstColumn; break;
            case OWN_ATTR_TABLETEMPLATE_LASTCOLUMN:         rValue >>= aSettings.mbUseLastColumn; break;
            case OWN_ATTR_TABLETEMPLATE_BANDINGROWS:        rValue >>= aSettings.mbUseRowBanding; break;
            case OWN_ATTR_TABLETEMPLATE_BANDINGCOLUMNS:    rValue >>= aSettings.mbUseColumnBanding; break;
            }

            static_cast< sdr::table::SdrTableObj* >( GetSdrObject() )->setTableStyleSettings(aSettings);
        }

        return true;
    }
    default:
    {
        return SvxShape::setPropertyValueImpl( rName, pProperty, rValue );
    }
    }
}

bool SvxTableShape::getPropertyValueImpl(
    const OUString& rName,
    const SfxItemPropertyMapEntry* pProperty,
    css::uno::Any& rValue )
{
    switch( pProperty->nWID )
    {
    case OWN_ATTR_OLEMODEL:
    {
        if( HasSdrObject() )
        {
            rValue <<= static_cast< sdr::table::SdrTableObj* >( GetSdrObject() )->getTable();
        }
        return true;
    }
    case OWN_ATTR_TABLETEMPLATE:
    {
        if( HasSdrObject() )
        {
            rValue <<= static_cast< sdr::table::SdrTableObj* >( GetSdrObject() )->getTableStyle();
        }
        return true;
    }
    case OWN_ATTR_REPLACEMENT_GRAPHIC:
    {
        if( HasSdrObject() )
        {
            Graphic aGraphic( SvxGetGraphicForShape( *GetSdrObject() ) );
            rValue <<= aGraphic.GetXGraphic();
        }
        return true;
    }
    case OWN_ATTR_TABLETEMPLATE_FIRSTROW:
    case OWN_ATTR_TABLETEMPLATE_LASTROW:
    case OWN_ATTR_TABLETEMPLATE_FIRSTCOLUMN:
    case OWN_ATTR_TABLETEMPLATE_LASTCOLUMN:
    case OWN_ATTR_TABLETEMPLATE_BANDINGROWS:
    case OWN_ATTR_TABLETEMPLATE_BANDINGCOLUMNS:
    {
        if( HasSdrObject() )
        {
            TableStyleSettings aSettings( static_cast< sdr::table::SdrTableObj* >( GetSdrObject() )->getTableStyleSettings() );

            switch( pProperty->nWID )
            {
            case OWN_ATTR_TABLETEMPLATE_FIRSTROW:           rValue <<= aSettings.mbUseFirstRow; break;
            case OWN_ATTR_TABLETEMPLATE_LASTROW:            rValue <<= aSettings.mbUseLastRow; break;
            case OWN_ATTR_TABLETEMPLATE_FIRSTCOLUMN:        rValue <<= aSettings.mbUseFirstColumn; break;
            case OWN_ATTR_TABLETEMPLATE_LASTCOLUMN:         rValue <<= aSettings.mbUseLastColumn; break;
            case OWN_ATTR_TABLETEMPLATE_BANDINGROWS:        rValue <<= aSettings.mbUseRowBanding; break;
            case OWN_ATTR_TABLETEMPLATE_BANDINGCOLUMNS:    rValue <<= aSettings.mbUseColumnBanding; break;
            }
        }

        return true;
    }
    default:
    {
        return SvxShape::getPropertyValueImpl( rName, pProperty, rValue );
    }
    }
}

void SvxTableShape::lock()
{
    SvxShape::lock();
    if( HasSdrObject() )
        static_cast< sdr::table::SdrTableObj* >( GetSdrObject() )->uno_lock();
}

void SvxTableShape::unlock()
{
    if( HasSdrObject() )
        static_cast< sdr::table::SdrTableObj* >( GetSdrObject() )->uno_unlock();
    SvxShape::unlock();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
