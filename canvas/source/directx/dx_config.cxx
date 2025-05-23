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
#include <sal/log.hxx>

#include <basegfx/vector/b2ivector.hxx>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <comphelper/anytostring.hxx>
#include <cppuhelper/exc_hlp.hxx>
#include <osl/diagnose.h>
#include <comphelper/diagnose_ex.hxx>

#include "dx_config.hxx"

using namespace com::sun::star;

namespace dxcanvas
{
    DXCanvasItem::DXCanvasItem() :
        ConfigItem(
            "Office.Canvas/DXCanvas",
            ConfigItemMode::NONE ),
        maValues(),
        maMaxTextureSize(),
        mbDenylistCurrentDevice(false),
        mbValuesDirty(false)
    {
        try
        {
            uno::Sequence< uno::Any > aProps( GetProperties( { "DeviceDenylist" } ));
            uno::Sequence< sal_Int32 > aValues;

            if (aProps.hasElements() &&
                (aProps[0] >>= aValues) )
            {
                const sal_Int32* pValues = aValues.getConstArray();
                const sal_Int32 nNumEntries( aValues.getLength()*sizeof(sal_Int32)/sizeof(DeviceInfo) );
                for( sal_Int32 i=0; i<nNumEntries; ++i )
                {
                    DeviceInfo aInfo;
                    aInfo.nVendorId         = *pValues++;
                    aInfo.nDeviceId         = *pValues++;
                    aInfo.nDeviceSubSysId   = *pValues++;
                    aInfo.nDeviceRevision   = *pValues++;
                    aInfo.nDriverId         = *pValues++;
                    aInfo.nDriverVersion    = *pValues++;
                    aInfo.nDriverSubVersion = *pValues++;
                    aInfo.nDriverBuildId    = *pValues++;
                    maValues.insert(aInfo);
                }
            }

            aProps = GetProperties( { "DenylistCurrentDevice" } );
            if( aProps.getLength() > 0 )
                aProps[0] >>= mbDenylistCurrentDevice;

            aProps = GetProperties( { "MaxTextureSize" } );
            if( aProps.getLength() > 0 )
                maMaxTextureSize = aProps[0].get<sal_Int32>();
            else
                maMaxTextureSize.reset();
        }
        catch( const uno::Exception& )
        {
            TOOLS_WARN_EXCEPTION( "canvas", "" );
        }
    }

    DXCanvasItem::~DXCanvasItem()
    {
        if( !mbValuesDirty )
            return;

        try
        {
            uno::Sequence< sal_Int32 > aValues( sizeof(DeviceInfo)/sizeof(sal_Int32)*maValues.size() );

            sal_Int32* pValues = aValues.getArray();
            for( const auto& rValueSet : maValues )
            {
                const DeviceInfo& rInfo( rValueSet );
                *pValues++ = rInfo.nVendorId;
                *pValues++ = rInfo.nDeviceId;
                *pValues++ = rInfo.nDeviceSubSysId;
                *pValues++ = rInfo.nDeviceRevision;
                *pValues++ = rInfo.nDriverId;
                *pValues++ = rInfo.nDriverVersion;
                *pValues++ = rInfo.nDriverSubVersion;
                *pValues++ = rInfo.nDriverBuildId;
            }

            PutProperties({"DeviceDenylist"}, {css::uno::Any(aValues)});
        }
        catch( const uno::Exception& )
        {
            TOOLS_WARN_EXCEPTION( "canvas", "" );
        }
    }

    void DXCanvasItem::Notify( const css::uno::Sequence<OUString>& ) {}
    void DXCanvasItem::ImplCommit() {}

    bool DXCanvasItem::isDeviceUsable( const DeviceInfo& rDeviceInfo ) const
    {
        return maValues.find(rDeviceInfo) == maValues.end();
    }

    bool DXCanvasItem::isDenylistCurrentDevice() const
    {
        return mbDenylistCurrentDevice;
    }

    void DXCanvasItem::denylistDevice( const DeviceInfo& rDeviceInfo )
    {
        mbValuesDirty = true;
        maValues.insert(rDeviceInfo);
    }

    void DXCanvasItem::adaptMaxTextureSize( basegfx::B2IVector& io_maxTextureSize ) const
    {
        if( maMaxTextureSize )
        {
            io_maxTextureSize.setX(
                std::min( *maMaxTextureSize,
                          io_maxTextureSize.getX() ));
            io_maxTextureSize.setY(
                std::min( *maMaxTextureSize,
                          io_maxTextureSize.getY() ));
        }
    }

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
