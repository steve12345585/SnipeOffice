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

#include <o3tl/test_info.hxx>
#include <osl/diagnose.h>
#include <comphelper/diagnose_ex.hxx>
#include <com/sun/star/datatransfer/clipboard/ClipboardEvent.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/weak.hxx>
#include <vcl/svapp.hxx>
#include <svdata.hxx>
#include <salinst.hxx>

#include <com/sun/star/datatransfer/clipboard/RenderingCapabilities.hpp>
#include "XNotifyingDataObject.hxx"

#include <systools/win32/comtools.hxx>
#include "DtObjFactory.hxx"
#include "APNDataObject.hxx"
#include "DOTransferable.hxx"
#include "WinClipboard.hxx"

#if !defined WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <ole2.h>
#include <objidl.h>

using namespace com::sun::star;

namespace
{
CWinClipboard* s_pCWinClipbImpl = nullptr;
std::mutex s_aClipboardSingletonMutex;

unsigned __stdcall releaseAsyncProc(void* p)
{
    static_cast<css::datatransfer::XTransferable*>(p)->release();
    return 0;
}

void releaseAsync(css::uno::Reference<css::datatransfer::XTransferable>& ref)
{
    if (!ref)
        return;
    auto pInterface = ref.get();
    pInterface->acquire();
    ref.clear(); // before starting the thread, to avoid race
    if (auto handle = _beginthreadex(nullptr, 0, releaseAsyncProc, pInterface, 0, nullptr))
        CloseHandle(reinterpret_cast<HANDLE>(handle));
}
}

/*XEventListener,*/
CWinClipboard::CWinClipboard(const uno::Reference<uno::XComponentContext>& rxContext,
                             const OUString& aClipboardName)
    : m_xContext(rxContext)
    , m_itsName(aClipboardName)
{
    // necessary to reassociate from
    // the static callback function
    {
        std::unique_lock aGuard(s_aClipboardSingletonMutex);
        s_pCWinClipbImpl = this;
    }

    registerClipboardViewer();
}

CWinClipboard::~CWinClipboard()
{
    assert(m_bDisposed);
    assert(!s_pCWinClipbImpl);
}

void CWinClipboard::disposing(std::unique_lock<std::mutex>& mutex)
{
    {
        std::unique_lock aGuard(s_aClipboardSingletonMutex);
        s_pCWinClipbImpl = nullptr;
    }

    unregisterClipboardViewer();

    WeakComponentImplHelper::disposing(mutex);
}

// XClipboard

CXNotifyingDataObject* CWinClipboard::getOwnClipContent() const
{
    assert(!m_pCurrentOwnClipContent || !m_pNewOwnClipContent); // Both can be null, or only one set
    return m_pCurrentOwnClipContent ? m_pCurrentOwnClipContent : m_pNewOwnClipContent;
}

// to avoid unnecessary traffic we check first if there is a clipboard
// content which was set via setContent, in this case we don't need
// to query the content from the clipboard, create a new wrapper object
// and so on, we simply return the original XTransferable instead of our
// DOTransferable

uno::Reference<datatransfer::XTransferable> SAL_CALL CWinClipboard::getContents()
{
    std::unique_lock aGuard(m_aMutex);
    return getContents_noLock();
}

css::uno::Reference<css::datatransfer::XTransferable> CWinClipboard::getContents_noLock()
{
    if (m_bDisposed)
        throw lang::DisposedException("object is already disposed",
                                      static_cast<XClipboardEx*>(this));

    assert(!getOwnClipContent() || !m_foreignContent); // Both can be null, or only one set

    // use the shortcut or create a transferable from
    // system clipboard
    if (auto pOwnClipContent = getOwnClipContent())
        return pOwnClipContent->m_XTransferable;

    // Content cached?
    if (m_foreignContent.is())
        return m_foreignContent;

    uno::Reference<datatransfer::XTransferable> rClipContent;

    // get the current format list from clipboard
    if (UINT nFormats; !GetUpdatedClipboardFormats(nullptr, 0, &nFormats)
                       && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        std::vector<UINT> aUINTFormats(nFormats);
        if (GetUpdatedClipboardFormats(aUINTFormats.data(), nFormats, &nFormats))
        {
            std::vector<sal_uInt32> aFormats(aUINTFormats.begin(), aUINTFormats.end());
            rClipContent = new CDOTransferable(m_xContext, this, aFormats);

            m_foreignContent = rClipContent;
        }
    }

    return rClipContent;
}

IDataObjectPtr CWinClipboard::getIDataObject()
{
    {
        std::unique_lock aGuard(m_aMutex);

        if (m_bDisposed)
            throw lang::DisposedException("object is already disposed",
                                          static_cast<XClipboardEx*>(this));
    }
    // get the current dataobject from clipboard
    IDataObjectPtr pIDataObject;
    HRESULT hr = m_MtaOleClipboard.getClipboard(&pIDataObject);

    if (SUCCEEDED(hr))
    {
        // create an apartment neutral dataobject and initialize it with a
        // com smart pointer to the IDataObject from clipboard
        pIDataObject = new CAPNDataObject(pIDataObject);
    }

    return pIDataObject;
}

void SAL_CALL CWinClipboard::setContents(
    const uno::Reference<datatransfer::XTransferable>& xTransferable,
    const uno::Reference<datatransfer::clipboard::XClipboardOwner>& xClipboardOwner)
{
    std::unique_lock aGuard(m_aMutex);

    if (m_bDisposed)
        throw lang::DisposedException("object is already disposed",
                                      static_cast<XClipboardEx*>(this));

    IDataObjectPtr pIDataObj;

    // The object must be destroyed only outside of the mutex lock, and in a different thread,
    // because it may call CWinClipboard::onReleaseDataObject, or try to lock solar mutex, in
    // another thread of this process synchronously
    releaseAsync(m_foreignContent); // clear m_foreignContent
    assert(!m_foreignContent.is());
    m_pCurrentOwnClipContent = nullptr;

    if (xTransferable.is())
    {
        // Store the new object's pointer to temporary m_pNewOwnClipContent, to be moved to
        // m_pCurrentOwnClipContent in handleClipboardContentChanged.
        m_pNewOwnClipContent = new CXNotifyingDataObject(
            CDTransObjFactory::createDataObjFromTransferable(m_xContext, xTransferable),
            xTransferable, xClipboardOwner, this);

        pIDataObj = IDataObjectPtr(m_pNewOwnClipContent);
    }
    else
    {
        m_pNewOwnClipContent = nullptr;
    }

    m_MtaOleClipboard.setClipboard(pIDataObj.get());
}

OUString SAL_CALL CWinClipboard::getName()
{
    std::unique_lock aGuard(m_aMutex);
    if (m_bDisposed)
        throw lang::DisposedException("object is already disposed",
                                      static_cast<XClipboardEx*>(this));

    return m_itsName;
}

// XFlushableClipboard

void SAL_CALL CWinClipboard::flushClipboard()
{
    std::unique_lock aGuard(m_aMutex);

    if (m_bDisposed)
        throw lang::DisposedException("object is already disposed",
                                      static_cast<XClipboardEx*>(this));

    // FlushClipboard does a callback and frees DataObject, which calls onReleaseDataObject and
    // locks mutex. FlushClipboard has to be synchron in order to prevent shutdown until all
    // clipboard-formats are rendered. The request is needed to prevent flushing if we are not
    // clipboard owner (it is not known what happens if we flush but aren't clipboard owner).
    // It may be possible to move the request to the clipboard STA thread by saving the
    // DataObject and call OleIsCurrentClipboard before flushing.

    if (getOwnClipContent())
    {
        aGuard.unlock();
        m_MtaOleClipboard.flushClipboard();
    }
}

// XClipboardEx

sal_Int8 SAL_CALL CWinClipboard::getRenderingCapabilities()
{
    if (m_bDisposed)
        throw lang::DisposedException("object is already disposed",
                                      static_cast<XClipboardEx*>(this));

    using namespace datatransfer::clipboard::RenderingCapabilities;
    return (Delayed | Persistent);
}

// XClipboardNotifier

void SAL_CALL CWinClipboard::addClipboardListener(
    const uno::Reference<datatransfer::clipboard::XClipboardListener>& listener)
{
    std::unique_lock aGuard(m_aMutex);
    if (m_bDisposed)
        throw lang::DisposedException("object is already disposed",
                                      static_cast<XClipboardEx*>(this));

    // check input parameter
    if (!listener.is())
        throw lang::IllegalArgumentException("empty reference", static_cast<XClipboardEx*>(this),
                                             1);

    maClipboardListeners.addInterface(aGuard, listener);
}

void SAL_CALL CWinClipboard::removeClipboardListener(
    const uno::Reference<datatransfer::clipboard::XClipboardListener>& listener)
{
    std::unique_lock aGuard(m_aMutex);
    if (m_bDisposed)
        throw lang::DisposedException("object is already disposed",
                                      static_cast<XClipboardEx*>(this));

    // check input parameter
    if (!listener.is())
        throw lang::IllegalArgumentException("empty reference", static_cast<XClipboardEx*>(this),
                                             1);

    maClipboardListeners.removeInterface(aGuard, listener);
}

void CWinClipboard::handleClipboardContentChanged()
{
    std::unique_lock aGuard(m_aMutex);
    if (m_bDisposed)
        return;

    // The object must be destroyed only outside of the mutex lock, and in a different thread,
    // because it may call CWinClipboard::onReleaseDataObject, or try to lock solar mutex, in
    // another thread of this process synchronously
    releaseAsync(m_foreignContent); // clear m_foreignContent
    assert(!m_foreignContent.is());
    // If new own content assignment is pending, do it; otherwise, clear it.
    // This makes sure that there will be no stuck clipboard content.
    m_pCurrentOwnClipContent = std::exchange(m_pNewOwnClipContent, nullptr);

    if (!maClipboardListeners.getLength(aGuard))
        return;

    try
    {
        uno::Reference<datatransfer::XTransferable> rXTransf(getContents_noLock());
        datatransfer::clipboard::ClipboardEvent aClipbEvent(static_cast<XClipboard*>(this),
                                                            rXTransf);
        maClipboardListeners.notifyEach(
            aGuard, &datatransfer::clipboard::XClipboardListener::changedContents, aClipbEvent);
        aGuard.unlock(); // for XTransferable dtor, that may delegate to another thread
    }
    catch (const lang::DisposedException&)
    {
        OSL_FAIL("Service Manager disposed");
        if (aGuard.owns_lock())
            aGuard.unlock();
        // no further clipboard changed notifications
        unregisterClipboardViewer();
    }
}

// XServiceInfo

OUString SAL_CALL CWinClipboard::getImplementationName()
{
    return "com.sun.star.datatransfer.clipboard.ClipboardW32";
}

sal_Bool SAL_CALL CWinClipboard::supportsService(const OUString& ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

uno::Sequence<OUString> SAL_CALL CWinClipboard::getSupportedServiceNames()
{
    return { "com.sun.star.datatransfer.clipboard.SystemClipboard" };
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
dtrans_CWinClipboard_get_implementation(css::uno::XComponentContext* context,
                                        css::uno::Sequence<css::uno::Any> const& args)
{
    // We run unit tests in parallel, which is a problem when touching a shared resource
    // like the system clipboard, so rather use the dummy GenericClipboard.
    static const bool bRunningUnitTest = o3tl::IsRunningUnitTest() || o3tl::IsRunningUITest();

    if (bRunningUnitTest)
    {
        SolarMutexGuard aGuard;
        auto xClipboard = ImplGetSVData()->mpDefInst->CreateClipboard(args);
        if (xClipboard.is())
            xClipboard->acquire();
        return xClipboard.get();
    }
    else
    {
        return cppu::acquire(new CWinClipboard(context, ""));
    }
}

void CWinClipboard::onReleaseDataObject(CXNotifyingDataObject& theCaller)
{
    theCaller.lostOwnership();

    // if the current caller is the one we currently hold, then set it to NULL
    // because an external source must be the clipboardowner now
    std::unique_lock aGuard(m_aMutex);

    if (getOwnClipContent() == &theCaller)
        m_pCurrentOwnClipContent = m_pNewOwnClipContent = nullptr;
}

void CWinClipboard::registerClipboardViewer()
{
    m_MtaOleClipboard.registerClipViewer(CWinClipboard::onClipboardContentChanged);
}

void CWinClipboard::unregisterClipboardViewer() { m_MtaOleClipboard.registerClipViewer(nullptr); }

void WINAPI CWinClipboard::onClipboardContentChanged()
{
    rtl::Reference<CWinClipboard> pWinClipboard;
    {
        // Only hold the mutex to obtain a safe reference to the impl, to avoid deadlock
        std::unique_lock aGuard(s_aClipboardSingletonMutex);
        pWinClipboard.set(s_pCWinClipbImpl);
    }

    if (pWinClipboard)
        pWinClipboard->handleClipboardContentChanged();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
