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

#include <memory>

#include <prewin.h>
#include <postwin.h>
#include <objbase.h>
#include <strmif.h>
#include <Amvideo.h>
#include "interface.hxx"
#include <uuids.h>

#include "framegrabber.hxx"
#include "player.hxx"

#include <cppuhelper/supportsservice.hxx>
#include <osl/file.hxx>
#include <tools/stream.hxx>
#include <vcl/graph.hxx>
#include <vcl/dibtools.hxx>
#include <o3tl/char16_t2wchar_t.hxx>
#include <systools/win32/oleauto.hxx>

constexpr OUStringLiteral AVMEDIA_WIN_FRAMEGRABBER_IMPLEMENTATIONNAME = u"com.sun.star.comp.avmedia.FrameGrabber_DirectX";
constexpr OUString AVMEDIA_WIN_FRAMEGRABBER_SERVICENAME = u"com.sun.star.media.FrameGrabber_DirectX"_ustr;

using namespace ::com::sun::star;

namespace avmedia::win {


FrameGrabber::FrameGrabber()
    : sal::systools::CoInitializeGuard(COINIT_APARTMENTTHREADED, false,
                                       sal::systools::CoInitializeGuard::WhenFailed::NoThrow)
{
}


FrameGrabber::~FrameGrabber() = default;

namespace {

sal::systools::COMReference<IMediaDet> implCreateMediaDet( const OUString& rURL )
{
    sal::systools::COMReference<IMediaDet> pDet;

    if( SUCCEEDED(pDet.CoCreateInstance(CLSID_MediaDet, nullptr, CLSCTX_INPROC_SERVER)) )
    {
        OUString aLocalStr;

        if( osl::FileBase::getSystemPathFromFileURL( rURL, aLocalStr )
            == osl::FileBase::E_None )
        {
            if( !SUCCEEDED( pDet->put_Filename(sal::systools::BStr(aLocalStr)) ) )
                pDet.clear();
        }
    }

    return pDet;
}

}

bool FrameGrabber::create( const OUString& rURL )
{
    // just check if a MediaDet interface can be created with the given URL
    if (implCreateMediaDet(rURL))
        maURL = rURL;
    else
        maURL.clear();

    return !maURL.isEmpty();
}


uno::Reference< graphic::XGraphic > SAL_CALL FrameGrabber::grabFrame( double fMediaTime )
{
    uno::Reference< graphic::XGraphic > xRet;
    if (sal::systools::COMReference<IMediaDet> pDet = implCreateMediaDet(maURL))
    {
        double  fLength;
        long    nStreamCount;
        bool    bFound = false;

        if( SUCCEEDED( pDet->get_OutputStreams( &nStreamCount ) ) )
        {
            for( long n = 0; ( n < nStreamCount ) && !bFound; ++n )
            {
                GUID aMajorType;

                if( SUCCEEDED( pDet->put_CurrentStream( n ) )  &&
                    SUCCEEDED( pDet->get_StreamType( &aMajorType ) ) &&
                    ( aMajorType == MEDIATYPE_Video ) )
                {
                    bFound = true;
                }
            }
        }

        if( bFound &&
            ( S_OK == pDet->get_StreamLength( &fLength ) ) &&
            ( fLength > 0.0 ) && ( fMediaTime >= 0.0 ) && ( fMediaTime <= fLength ) )
        {
            AM_MEDIA_TYPE   aMediaType;
            LONG            nWidth = 0, nHeight = 0;
            long            nSize = 0;

            if( SUCCEEDED( pDet->get_StreamMediaType( &aMediaType ) ) )
            {
                if( ( aMediaType.formattype == FORMAT_VideoInfo ) &&
                    ( aMediaType.cbFormat >= sizeof( VIDEOINFOHEADER ) ) )
                {
                    VIDEOINFOHEADER* pVih = reinterpret_cast< VIDEOINFOHEADER* >( aMediaType.pbFormat );

                    nWidth = pVih->bmiHeader.biWidth;
                    nHeight = pVih->bmiHeader.biHeight;

                    if( nHeight < 0 )
                        nHeight *= -1;
                }

                if( aMediaType.cbFormat != 0 )
                {
                    ::CoTaskMemFree( aMediaType.pbFormat );
                    aMediaType.cbFormat = 0;
                    aMediaType.pbFormat = nullptr;
                }

                if( aMediaType.pUnk != nullptr )
                {
                    aMediaType.pUnk->Release();
                    aMediaType.pUnk = nullptr;
                }
            }

            if( ( nWidth > 0 ) && ( nHeight > 0 ) &&
                SUCCEEDED( pDet->GetBitmapBits( 0, &nSize, nullptr, nWidth, nHeight ) ) &&
                ( nSize > 0  ) )
            {
                auto pBuffer = std::make_unique<char[]>(nSize);

                try
                {
                    if( SUCCEEDED( pDet->GetBitmapBits( fMediaTime, nullptr, pBuffer.get(), nWidth, nHeight ) ) )
                    {
                        SvMemoryStream  aMemStm( pBuffer.get(), nSize, StreamMode::READ | StreamMode::WRITE );
                        Bitmap          aBmp;

                        if( ReadDIB(aBmp, aMemStm, false ) && !aBmp.IsEmpty() )
                        {
                            BitmapEx aBitmapEx(aBmp);
                            Graphic aGraphic(aBitmapEx);
                            xRet = aGraphic.GetXGraphic();
                        }
                    }
                }
                catch( ... )
                {
                }
            }
        }
    }

    return xRet;
}


OUString SAL_CALL FrameGrabber::getImplementationName(  )
{
    return AVMEDIA_WIN_FRAMEGRABBER_IMPLEMENTATIONNAME;
}


sal_Bool SAL_CALL FrameGrabber::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}


uno::Sequence< OUString > SAL_CALL FrameGrabber::getSupportedServiceNames(  )
{
    return { AVMEDIA_WIN_FRAMEGRABBER_SERVICENAME };
}

} // namespace avmedia::win


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
