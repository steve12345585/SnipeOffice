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

#include <store/store.h>

#include <sal/types.h>
#include <rtl/string.hxx>
#include <rtl/ref.hxx>

#include "object.hxx"
#include "lockbyte.hxx"

#include "storbase.hxx"
#include "storpage.hxx"
#include "stordir.hxx"
#include "storlckb.hxx"

using rtl::Reference;

namespace store
{

namespace {

/** Template helper class as type safe Reference to store_handle_type.
 */
template<class store_handle_type>
class OStoreHandle : public rtl::Reference<store_handle_type>
{
public:
    explicit OStoreHandle (store_handle_type * pHandle)
        : rtl::Reference<store_handle_type> (pHandle)
    {}

    static store_handle_type * SAL_CALL query (void * pHandle)
    {
        return store::query (
            static_cast<OStoreObject*>(pHandle),
            static_cast<store_handle_type*>(nullptr));
    }
};

}

}

using namespace store;

storeError store_acquireHandle (
    storeHandle Handle
) noexcept
{
    OStoreObject *pHandle = static_cast<OStoreObject*>(Handle);
    if (!pHandle)
        return store_E_InvalidHandle;

    pHandle->acquire();
    return store_E_None;
}

storeError store_releaseHandle (
    storeHandle Handle
) noexcept
{
    OStoreObject *pHandle = static_cast<OStoreObject*>(Handle);
    if (!pHandle)
        return store_E_InvalidHandle;

    pHandle->release();
    return store_E_None;
}

storeError store_createMemoryFile (
    sal_uInt16       nPageSize,
    storeFileHandle *phFile
) noexcept
{
    if (!phFile)
        return store_E_InvalidParameter;
    *phFile = nullptr;

    Reference<ILockBytes> xLockBytes;

    storeError eErrCode = MemoryLockBytes_createInstance(xLockBytes);
    if (eErrCode != store_E_None)
        return eErrCode;
    OSL_ASSERT(xLockBytes.is());

    Reference<OStorePageManager> xManager (new OStorePageManager());

    eErrCode = xManager->initialize (
        &*xLockBytes, storeAccessMode::Create, nPageSize);
    if (eErrCode != store_E_None)
        return eErrCode;

    xManager->acquire();

    *phFile = xManager.get();
    return store_E_None;
}

storeError store_openFile (
    rtl_uString     *pFilename,
    storeAccessMode  eAccessMode,
    sal_uInt16       nPageSize,
    storeFileHandle *phFile
) noexcept
{
    if (phFile)
        *phFile = nullptr;

    if (!(pFilename && phFile))
        return store_E_InvalidParameter;

    Reference<ILockBytes> xLockBytes;

    storeError eErrCode = FileLockBytes_createInstance (xLockBytes, pFilename, eAccessMode);
    if (eErrCode != store_E_None)
        return eErrCode;
    OSL_ASSERT(xLockBytes.is());

    Reference<OStorePageManager> xManager (new OStorePageManager());
    eErrCode = xManager->initialize (
        &*xLockBytes, eAccessMode, nPageSize);
    if (eErrCode != store_E_None)
        return eErrCode;

    xManager->acquire();

    *phFile = xManager.get();
    return store_E_None;
}

/*
 * store_closeFile.
 */
storeError store_closeFile (
    storeFileHandle Handle
) noexcept
{
    OStorePageManager *pManager =
        OStoreHandle<OStorePageManager>::query (Handle);
    if (!pManager)
        return store_E_InvalidHandle;

    storeError eErrCode = pManager->close();
    pManager->release();
    return eErrCode;
}

storeError store_flushFile (
    storeFileHandle Handle
) noexcept
{
    OStoreHandle<OStorePageManager> xManager (
        OStoreHandle<OStorePageManager>::query (Handle));
    if (!xManager.is())
        return store_E_InvalidHandle;

    return xManager->flush();
}

storeError store_openDirectory (
    storeFileHandle       hFile,
    rtl_uString const    *pPath,
    rtl_uString const    *pName,
    storeAccessMode       eAccessMode,
    storeDirectoryHandle *phDirectory
) noexcept
{
    storeError eErrCode = store_E_None;
    if (phDirectory)
        *phDirectory = nullptr;

    OStoreHandle<OStorePageManager> xManager (
        OStoreHandle<OStorePageManager>::query (hFile));
    if (!xManager.is())
        return store_E_InvalidHandle;

    if (!(pPath && pName && phDirectory))
        return store_E_InvalidParameter;

    Reference<OStoreDirectory_Impl> xDirectory (new OStoreDirectory_Impl());

    OString aPath (pPath->buffer, pPath->length, RTL_TEXTENCODING_UTF8);
    OString aName (pName->buffer, pName->length, RTL_TEXTENCODING_UTF8);

    eErrCode = xDirectory->create (&*xManager, aPath.pData, aName.pData, eAccessMode);
    if (eErrCode != store_E_None)
        return eErrCode;

    xDirectory->acquire();

    *phDirectory = xDirectory.get();
    return store_E_None;
}

storeError store_findFirst (
    storeDirectoryHandle  Handle,
    storeFindData        *pFindData
) noexcept
{
    OStoreHandle<OStoreDirectory_Impl> xDirectory (
        OStoreHandle<OStoreDirectory_Impl>::query (Handle));
    if (!xDirectory.is())
        return store_E_InvalidHandle;

    if (!pFindData)
        return store_E_InvalidParameter;

    // Initialize FindData.
    memset (pFindData, 0, sizeof (storeFindData));

    // Find first.
    pFindData->m_nReserved = sal_uInt32(~0);
    return xDirectory->iterate (*pFindData);
}

storeError store_findNext (
    storeDirectoryHandle  Handle,
    storeFindData        *pFindData
) noexcept
{
    OStoreHandle<OStoreDirectory_Impl> xDirectory (
        OStoreHandle<OStoreDirectory_Impl>::query (Handle));
    if (!xDirectory.is())
        return store_E_InvalidHandle;

    if (!pFindData)
        return store_E_InvalidParameter;

    // Check FindData.
    if (!pFindData->m_nReserved)
        return store_E_NoMoreFiles;

    // Find next.
    pFindData->m_nReserved -= 1;
    return xDirectory->iterate (*pFindData);
}

storeError store_openStream (
    storeFileHandle    hFile,
    rtl_uString const *pPath,
    rtl_uString const *pName,
    storeAccessMode    eAccessMode,
    storeStreamHandle *phStream
) noexcept
{
    storeError eErrCode = store_E_None;
    if (phStream)
        *phStream = nullptr;

    OStoreHandle<OStorePageManager> xManager (
        OStoreHandle<OStorePageManager>::query (hFile));
    if (!xManager.is())
        return store_E_InvalidHandle;

    if (!(pPath && pName && phStream))
        return store_E_InvalidParameter;

    Reference<OStoreLockBytes> xLockBytes (new OStoreLockBytes());

    OString aPath (pPath->buffer, pPath->length, RTL_TEXTENCODING_UTF8);
    OString aName (pName->buffer, pName->length, RTL_TEXTENCODING_UTF8);

    eErrCode = xLockBytes->create (&*xManager, aPath.pData, aName.pData, eAccessMode);
    if (eErrCode != store_E_None)
        return eErrCode;

    xLockBytes->acquire();

    *phStream = xLockBytes.get();
    return store_E_None;
}

/*
 * store_readStream.
 */
storeError store_readStream (
    storeStreamHandle  Handle,
    sal_uInt32         nOffset,
    void              *pBuffer,
    sal_uInt32         nBytes,
    sal_uInt32        *pnDone
) noexcept
{
    OStoreHandle<OStoreLockBytes> xLockBytes (
        OStoreHandle<OStoreLockBytes>::query (Handle));
    if (!xLockBytes.is())
        return store_E_InvalidHandle;

    if (!(pBuffer && pnDone))
        return store_E_InvalidParameter;

    return xLockBytes->readAt (nOffset, pBuffer, nBytes, *pnDone);
}

storeError store_writeStream (
    storeStreamHandle  Handle,
    sal_uInt32         nOffset,
    const void        *pBuffer,
    sal_uInt32         nBytes,
    sal_uInt32        *pnDone
) noexcept
{
    OStoreHandle<OStoreLockBytes> xLockBytes (
        OStoreHandle<OStoreLockBytes>::query (Handle));
    if (!xLockBytes.is())
        return store_E_InvalidHandle;

    if (!(pBuffer && pnDone))
        return store_E_InvalidParameter;

    return xLockBytes->writeAt (nOffset, pBuffer, nBytes, *pnDone);
}

storeError store_remove (
    storeFileHandle Handle,
    rtl_uString const *pPath,
    rtl_uString const *pName
) noexcept
{
    storeError eErrCode = store_E_None;

    OStoreHandle<OStorePageManager> xManager (
        OStoreHandle<OStorePageManager>::query (Handle));
    if (!xManager.is())
        return store_E_InvalidHandle;

    if (!(pPath && pName))
        return store_E_InvalidParameter;

    // Setup page key.
    OString aPath (pPath->buffer, pPath->length, RTL_TEXTENCODING_UTF8);
    OString aName (pName->buffer, pName->length, RTL_TEXTENCODING_UTF8);
    OStorePageKey aKey;

    eErrCode = OStorePageManager::namei (aPath.pData, aName.pData, aKey);
    if (eErrCode != store_E_None)
        return eErrCode;

    // Remove.
    return xManager->remove (aKey);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
