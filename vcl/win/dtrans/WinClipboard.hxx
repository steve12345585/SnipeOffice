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

#pragma once

#include <rtl/ustring.hxx>
#include <sal/types.h>
#include <comphelper/compbase.hxx>
#include <comphelper/interfacecontainer4.hxx>
#include <com/sun/star/datatransfer/XTransferable.hpp>
#include <com/sun/star/datatransfer/clipboard/XClipboardEx.hpp>
#include <com/sun/star/datatransfer/clipboard/XClipboardOwner.hpp>
#include <com/sun/star/datatransfer/clipboard/XClipboardListener.hpp>
#include <com/sun/star/datatransfer/clipboard/XClipboardNotifier.hpp>
#include <com/sun/star/datatransfer/clipboard/XSystemClipboard.hpp>
#include <com/sun/star/datatransfer/clipboard/XFlushableClipboard.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <osl/conditn.hxx>
#include <systools/win32/comtools.hxx>

#include "MtaOleClipb.hxx"
#include "XNotifyingDataObject.hxx"

// implements the XClipboard[Ex] ... interfaces
// for the clipboard viewer mechanism we need a static callback function
// and a static member to reassociate from this static function to the
// class instance
// watch out: we are using only one static member variable and not a list
// because we assume to be instantiated only once
// this will be assured by a OneInstanceFactory of the service and not
// by this class!

class CWinClipboard final
    : public comphelper::WeakComponentImplHelper<css::datatransfer::clipboard::XSystemClipboard,
                                                 css::datatransfer::clipboard::XFlushableClipboard,
                                                 css::lang::XServiceInfo>
{
    friend CXNotifyingDataObject::~CXNotifyingDataObject();

    css::uno::Reference<css::uno::XComponentContext> m_xContext;
    const OUString m_itsName;
    CMtaOleClipboard m_MtaOleClipboard;
    CXNotifyingDataObject* m_pNewOwnClipContent = nullptr; // until onClipboardContentChanged
    CXNotifyingDataObject* m_pCurrentOwnClipContent = nullptr;
    css::uno::Reference<css::datatransfer::XTransferable> m_foreignContent;
    comphelper::OInterfaceContainerHelper4<css::datatransfer::clipboard::XClipboardListener>
        maClipboardListeners;

    CXNotifyingDataObject* getOwnClipContent() const;

    void handleClipboardContentChanged();
    void onReleaseDataObject(CXNotifyingDataObject& theCaller);

    void registerClipboardViewer();
    void unregisterClipboardViewer();

    static void WINAPI onClipboardContentChanged();

    css::uno::Reference<css::datatransfer::XTransferable> getContents_noLock();

public:
    CWinClipboard(const css::uno::Reference<css::uno::XComponentContext>& rxContext,
                  const OUString& aClipboardName);
    virtual ~CWinClipboard() override;

    // XClipboard
    virtual css::uno::Reference<css::datatransfer::XTransferable> SAL_CALL getContents() override;
    virtual void SAL_CALL setContents(
        const css::uno::Reference<css::datatransfer::XTransferable>& xTransferable,
        const css::uno::Reference<css::datatransfer::clipboard::XClipboardOwner>& xClipboardOwner)
        override;
    virtual OUString SAL_CALL getName() override;

    // XFlushableClipboard
    virtual void SAL_CALL flushClipboard() override;

    // XClipboardEx
    virtual sal_Int8 SAL_CALL getRenderingCapabilities() override;

    // XClipboardNotifier
    virtual void SAL_CALL addClipboardListener(
        const css::uno::Reference<css::datatransfer::clipboard::XClipboardListener>& listener)
        override;
    virtual void SAL_CALL removeClipboardListener(
        const css::uno::Reference<css::datatransfer::clipboard::XClipboardListener>& listener)
        override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    virtual css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;

    IDataObjectPtr getIDataObject();

    virtual void disposing(std::unique_lock<std::mutex>&) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
