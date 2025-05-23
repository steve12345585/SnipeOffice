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

#include "clipboardmanager.hxx"
#include <com/sun/star/container/ElementExistException.hpp>
#include <com/sun/star/container/NoSuchElementException.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <cppuhelper/supportsservice.hxx>
#include <comphelper/sequence.hxx>
#include <rtl/ref.hxx>

using namespace com::sun::star::container;
using namespace com::sun::star::datatransfer;
using namespace com::sun::star::datatransfer::clipboard;
using namespace com::sun::star::lang;
using namespace com::sun::star::uno;
using namespace cppu;
using namespace osl;

using ::dtrans::ClipboardManager;

static std::mutex g_InstanceGuard;
static rtl::Reference<ClipboardManager> g_Instance;
static bool g_Disposed = false;


ClipboardManager::ClipboardManager():
    m_aDefaultName(OUString("default"))
{
}

ClipboardManager::~ClipboardManager()
{
}

OUString SAL_CALL ClipboardManager::getImplementationName(  )
{
    return "com.sun.star.comp.datatransfer.ClipboardManager";
}

sal_Bool SAL_CALL ClipboardManager::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

Sequence< OUString > SAL_CALL ClipboardManager::getSupportedServiceNames(  )
{
    return { "com.sun.star.datatransfer.clipboard.ClipboardManager" };
}

Reference< XClipboard > SAL_CALL ClipboardManager::getClipboard( const OUString& aName )
{
    std::unique_lock aGuard(m_aMutex);

    // object is disposed already
    if (m_bDisposed)
        throw DisposedException("object is disposed.",
                                static_cast < XClipboardManager * > (this));

    ClipboardMap::iterator iter =
        m_aClipboardMap.find(aName.getLength() ? aName : m_aDefaultName);

    if (iter != m_aClipboardMap.end())
        return iter->second;

    throw NoSuchElementException(aName, static_cast < XClipboardManager * > (this));
}

void SAL_CALL ClipboardManager::addClipboard( const Reference< XClipboard >& xClipboard )
{
    OSL_ASSERT(xClipboard.is());

    // check parameter
    if (!xClipboard.is())
        throw IllegalArgumentException("empty reference",
                                       static_cast < XClipboardManager * > (this), 1);

    // the name "default" is reserved for internal use
    OUString aName = xClipboard->getName();
    if ( m_aDefaultName == aName )
        throw IllegalArgumentException("name reserved",
                                       static_cast < XClipboardManager * > (this), 1);

    // try to add new clipboard to the list
    std::unique_lock aGuard(m_aMutex);
    if (!m_bDisposed)
    {
        std::pair< const OUString, Reference< XClipboard > > value (
            aName.getLength() ? aName : m_aDefaultName,
            xClipboard );

        std::pair< ClipboardMap::iterator, bool > p = m_aClipboardMap.insert(value);
        aGuard.unlock();

        // insert failed, element must exist already
        if (!p.second)
            throw ElementExistException(aName, static_cast < XClipboardManager * > (this));

        // request disposing notifications
        Reference< XComponent > xComponent(xClipboard, UNO_QUERY);
        if (xComponent.is())
            xComponent->addEventListener(static_cast < XEventListener * > (this));
    }
}

void SAL_CALL ClipboardManager::removeClipboard( const OUString& aName )
{
    std::unique_lock aGuard(m_aMutex);
    if (!m_bDisposed)
        m_aClipboardMap.erase(aName.getLength() ? aName : m_aDefaultName );
}

Sequence< OUString > SAL_CALL ClipboardManager::listClipboardNames()
{
    std::unique_lock aGuard(m_aMutex);

    if (m_bDisposed)
        throw DisposedException("object is disposed.",
                                static_cast < XClipboardManager * > (this));

    return comphelper::mapKeysToSequence(m_aClipboardMap);
}

void ClipboardManager::disposing(std::unique_lock<std::mutex>& rGuard)
{
    rGuard.unlock();

    {
        std::unique_lock aGuard(g_InstanceGuard);
        g_Instance.clear();
        g_Disposed = true;
    }

    // removeClipboard is still allowed here,  so make a copy of the
    // list (to ensure integrity) and clear the original.
    rGuard.lock();
    ClipboardMap aCopy;
    std::swap(aCopy, m_aClipboardMap);
    rGuard.unlock();

    // dispose all clipboards still in list
    for (auto const& elem : aCopy)
    {
        Reference< XComponent > xComponent(elem.second, UNO_QUERY);
        if (xComponent.is())
        {
            try
            {
                xComponent->removeEventListener(static_cast < XEventListener * > (this));
                xComponent->dispose();
            }
            catch (const Exception&)
            {
                // exceptions can be safely ignored here.
            }
        }
    }
}

void SAL_CALL  ClipboardManager::disposing( const EventObject& event )
{
    Reference < XClipboard > xClipboard(event.Source, UNO_QUERY);

    if (xClipboard.is())
        removeClipboard(xClipboard->getName());
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
dtrans_ClipboardManager_get_implementation(
    css::uno::XComponentContext* , css::uno::Sequence<css::uno::Any> const&)
{
    std::unique_lock aGuard(g_InstanceGuard);
    if (g_Disposed)
        return nullptr;
    if (!g_Instance)
        g_Instance.set(new ClipboardManager());
    return cppu::acquire(g_Instance.get());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
