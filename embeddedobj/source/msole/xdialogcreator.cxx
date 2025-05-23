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

#include <com/sun/star/embed/EmbeddedObjectCreator.hpp>
#include <com/sun/star/embed/XEmbeddedObject.hpp>
#include <com/sun/star/embed/EntryInitModes.hpp>
#include <com/sun/star/embed/OLEEmbeddedObjectFactory.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/datatransfer/DataFlavor.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/ucb/CommandAbortedException.hpp>
#include <com/sun/star/task/XStatusIndicatorFactory.hpp>

#include <osl/thread.h>
#include <osl/file.hxx>
#include <osl/module.hxx>
#include <comphelper/classids.hxx>

#include "platform.h"
#include <comphelper/mimeconfighelper.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/propertyvalue.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/weak.hxx>
#include <comphelper/sequenceashashmap.hxx>

#include "xdialogcreator.hxx"
#include <oleembobj.hxx>


#ifdef _WIN32

#include <oledlg.h>
#include <o3tl/char16_t2wchar_t.hxx>
#include <vcl/winscheduler.hxx>

namespace {

class InitializedOleGuard
{
public:
    InitializedOleGuard()
    {
        if ( !SUCCEEDED( OleInitialize( nullptr ) ) )
            throw css::uno::RuntimeException();
    }

    ~InitializedOleGuard()
    {
        OleUninitialize();
    }
};

}

extern "C" {
typedef UINT STDAPICALLTYPE OleUIInsertObjectA_Type(LPOLEUIINSERTOBJECTA);
}

#endif


using namespace ::com::sun::star;
using namespace ::comphelper;

static uno::Sequence< sal_Int8 > GetRelatedInternalID_Impl( const uno::Sequence< sal_Int8 >& aClassID )
{
    // Writer
    if ( MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SW_OLE_EMBED_CLASSID_60 ) )
      || MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SW_OLE_EMBED_CLASSID_8 ) ) )
        return MimeConfigurationHelper::GetSequenceClassID( SO3_SW_CLASSID_60 );

    // Calc
    if ( MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SC_OLE_EMBED_CLASSID_60 ) )
      || MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SC_OLE_EMBED_CLASSID_8 ) ) )
        return MimeConfigurationHelper::GetSequenceClassID( SO3_SC_CLASSID_60 );

    // Impress
    if ( MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SIMPRESS_OLE_EMBED_CLASSID_60 ) )
      || MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SIMPRESS_OLE_EMBED_CLASSID_8 ) ) )
        return MimeConfigurationHelper::GetSequenceClassID( SO3_SIMPRESS_CLASSID_60 );

    // Draw
    if ( MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SDRAW_OLE_EMBED_CLASSID_60 ) )
      || MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SDRAW_OLE_EMBED_CLASSID_8 ) ) )
        return MimeConfigurationHelper::GetSequenceClassID( SO3_SDRAW_CLASSID_60 );

    // Chart
    if ( MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SCH_OLE_EMBED_CLASSID_60 ) )
      || MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SCH_OLE_EMBED_CLASSID_8 ) ) )
        return MimeConfigurationHelper::GetSequenceClassID( SO3_SCH_CLASSID_60 );

    // Math
    if ( MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SM_OLE_EMBED_CLASSID_60 ) )
      || MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SM_OLE_EMBED_CLASSID_8 ) ) )
        return MimeConfigurationHelper::GetSequenceClassID( SO3_SM_CLASSID_60 );

    return aClassID;
}


embed::InsertedObjectInfo SAL_CALL MSOLEDialogObjectCreator::createInstanceByDialog(
            const uno::Reference< embed::XStorage >& xStorage,
            const OUString& sEntName,
            const uno::Sequence< beans::PropertyValue >& aInObjArgs )
{
    embed::InsertedObjectInfo aObjectInfo;
    uno::Sequence< beans::PropertyValue > aObjArgs( aInObjArgs );

#ifdef _WIN32

    if ( !xStorage.is() )
        throw lang::IllegalArgumentException( "No parent storage is provided!",
                                            static_cast< ::cppu::OWeakObject* >(this),
                                            1 );

    if ( !sEntName.getLength() )
        throw lang::IllegalArgumentException( "Empty element name is provided!",
                                            static_cast< ::cppu::OWeakObject* >(this),
                                            2 );

    InitializedOleGuard aGuard;

    OLEUIINSERTOBJECTW io{ sizeof(io) };
    io.hWndOwner = GetActiveWindow();

    wchar_t szFile[MAX_PATH];
    szFile[0] = 0;
    io.lpszFile = szFile;
    io.cchFile = std::size(szFile);

    io.dwFlags = IOF_SELECTCREATENEW | IOF_DISABLELINK;

    // Disable any event loop shortcuts by enabling a real timer.
    // This way the native windows dialog won't block our own processing.
    WinScheduler::SetForceRealTimer();

    UINT uTemp = OleUIInsertObjectW(&io);

    if ( OLEUI_OK != uTemp )
        throw ucb::CommandAbortedException();

    if (io.dwFlags & IOF_SELECTCREATENEW)
    {
        uno::Reference< embed::XEmbeddedObjectCreator > xEmbCreator = embed::EmbeddedObjectCreator::create( m_xContext );

        uno::Sequence< sal_Int8 > aClassID = MimeConfigurationHelper::GetSequenceClassID( io.clsid.Data1,
                                                                                          io.clsid.Data2,
                                                                                          io.clsid.Data3,
                                                                                          io.clsid.Data4[0],
                                                                                          io.clsid.Data4[1],
                                                                                          io.clsid.Data4[2],
                                                                                          io.clsid.Data4[3],
                                                                                          io.clsid.Data4[4],
                                                                                          io.clsid.Data4[5],
                                                                                          io.clsid.Data4[6],
                                                                                          io.clsid.Data4[7] );

        aClassID = GetRelatedInternalID_Impl( aClassID );

        //TODO: retrieve ClassName
        aObjectInfo.Object.set( xEmbCreator->createInstanceInitNew( aClassID, OUString(), xStorage, sEntName, aObjArgs ),
                                uno::UNO_QUERY );
    }
    else
    {
        OUString aFileName(o3tl::toU(szFile));
        OUString aFileURL;
        if ( osl::FileBase::getFileURLFromSystemPath( aFileName, aFileURL ) != osl::FileBase::E_None )
            throw uno::RuntimeException();

        uno::Sequence< beans::PropertyValue > aMediaDescr{ comphelper::makePropertyValue("URL",
                                                                                         aFileURL) };

        // TODO: use config helper for type detection
        uno::Reference< embed::XEmbeddedObjectCreator > xEmbCreator;
        ::comphelper::MimeConfigurationHelper aHelper( m_xContext );

        if ( aHelper.AddFilterNameCheckOwnFile( aMediaDescr ) )
            xEmbCreator = embed::EmbeddedObjectCreator::create( m_xContext );
        else
            xEmbCreator = embed::OLEEmbeddedObjectFactory::create( m_xContext );

        if ( !xEmbCreator.is() )
            throw uno::RuntimeException();

        uno::Reference<task::XStatusIndicator> xProgress;
        OUString aProgressText;
        comphelper::SequenceAsHashMap aMap(aInObjArgs);
        auto it = aMap.find("StatusIndicator");
        if (it != aMap.end())
        {
            it->second >>= xProgress;
        }
        it = aMap.find("StatusIndicatorText");
        if (it != aMap.end())
        {
            it->second >>= aProgressText;
        }
        if (xProgress.is())
        {
            xProgress->start(aProgressText, 100);
        }

        aObjectInfo.Object.set( xEmbCreator->createInstanceInitFromMediaDescriptor( xStorage, sEntName, aMediaDescr, aObjArgs ),
                                uno::UNO_QUERY );

        if (xProgress.is())
        {
            xProgress->end();
        }
    }

    if ( ( io.dwFlags & IOF_CHECKDISPLAYASICON) && io.hMetaPict != nullptr )
    {
        METAFILEPICT* pMF = static_cast<METAFILEPICT*>(GlobalLock( io.hMetaPict ));
        if ( pMF )
        {
            sal_uInt32 nBufSize = GetMetaFileBitsEx( pMF->hMF, 0, nullptr );
            uno::Sequence< sal_Int8 > aMetafile( nBufSize + 22 );
            sal_Int8* pBuf = aMetafile.getArray();
            *reinterpret_cast<long*>( pBuf ) = 0x9ac6cdd7L;
            *reinterpret_cast<short*>( pBuf+6 ) = SHORT(0);
            *reinterpret_cast<short*>( pBuf+8 ) = SHORT(0);
            *reinterpret_cast<short*>( pBuf+10 ) = static_cast<SHORT>(pMF->xExt);
            *reinterpret_cast<short*>( pBuf+12 ) = static_cast<SHORT>(pMF->yExt);
            *reinterpret_cast<short*>( pBuf+14 ) = USHORT(2540);

            if ( nBufSize && nBufSize == GetMetaFileBitsEx( pMF->hMF, nBufSize, pBuf+22 ) )
            {
                datatransfer::DataFlavor aFlavor(
                    "application/x-openoffice-wmf;windows_formatname=\"Image WMF\"",
                    "Image WMF",
                    cppu::UnoType<uno::Sequence< sal_Int8 >>::get() );

                aObjectInfo.Options = { { "Icon", css::uno::Any(aMetafile) },
                                        { "IconFormat", css::uno::Any(aFlavor) } };
            }

            GlobalUnlock( io.hMetaPict );
        }
    }

    OSL_ENSURE( aObjectInfo.Object.is(), "No object was created!" );
    if ( !aObjectInfo.Object.is() )
        throw uno::RuntimeException();

    return aObjectInfo;
#else
    throw lang::NoSupportException(); // TODO:
#endif
}


embed::InsertedObjectInfo SAL_CALL MSOLEDialogObjectCreator::createInstanceInitFromClipboard(
                const uno::Reference< embed::XStorage >& xStorage,
                const OUString& sEntryName,
                const uno::Sequence< beans::PropertyValue >& aObjectArgs )
{
    embed::InsertedObjectInfo aObjectInfo;

#ifdef _WIN32
    if ( !xStorage.is() )
        throw lang::IllegalArgumentException( "No parent storage is provided!",
                                            static_cast< ::cppu::OWeakObject* >(this),
                                            1 );

    if ( !sEntryName.getLength() )
        throw lang::IllegalArgumentException( "Empty element name is provided!",
                                            static_cast< ::cppu::OWeakObject* >(this),
                                            2 );

    uno::Reference< embed::XEmbeddedObject > xResult(
                    new OleEmbeddedObject( m_xContext ) );

    uno::Reference< embed::XEmbedPersist > xPersist( xResult, uno::UNO_QUERY_THROW );
    xPersist->setPersistentEntry( xStorage,
                                    sEntryName,
                                    embed::EntryInitModes::DEFAULT_INIT,
                                    uno::Sequence< beans::PropertyValue >(),
                                    aObjectArgs );

    aObjectInfo.Object = xResult;

    // TODO/LATER: in case of iconify object the icon should be stored in aObjectInfo

    OSL_ENSURE( aObjectInfo.Object.is(), "No object was created!" );
    if ( !aObjectInfo.Object.is() )
        throw uno::RuntimeException();

    return aObjectInfo;
#else
    throw lang::NoSupportException(); // TODO:
#endif
}


OUString SAL_CALL MSOLEDialogObjectCreator::getImplementationName()
{
    return "com.sun.star.comp.embed.MSOLEObjectSystemCreator";
}


sal_Bool SAL_CALL MSOLEDialogObjectCreator::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}


uno::Sequence< OUString > SAL_CALL MSOLEDialogObjectCreator::getSupportedServiceNames()
{
    return { "com.sun.star.embed.MSOLEObjectSystemCreator",
             "com.sun.star.comp.embed.MSOLEObjectSystemCreator" };
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
embeddedobj_MSOLEDialogObjectCreator_get_implementation(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new MSOLEDialogObjectCreator(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
