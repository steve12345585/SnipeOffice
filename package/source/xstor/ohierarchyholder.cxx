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

#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/XHierarchicalStorageAccess2.hpp>
#include <com/sun/star/embed/XTransactedObject.hpp>
#include <com/sun/star/embed/XTransactionBroadcaster.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/WrappedTargetRuntimeException.hpp>
#include <cppuhelper/exc_hlp.hxx>
#include <o3tl/string_view.hxx>

#include "ohierarchyholder.hxx"
#include "xstorage.hxx"

using namespace ::com::sun::star;

// OHierarchyHolder_Impl

uno::Reference< embed::XExtendedStorageStream > OHierarchyHolder_Impl::GetStreamHierarchically( sal_Int32 nStorageMode, std::vector<OUString>& aListPath, sal_Int32 nStreamMode, const ::comphelper::SequenceAsHashMap& aEncryptionData )
{
    if ( !( nStorageMode & embed::ElementModes::WRITE ) && ( nStreamMode & embed::ElementModes::WRITE ) )
        throw io::IOException(u"invalid storage/stream mode combo"_ustr);

    uno::Reference< embed::XExtendedStorageStream > xResult =
        m_xChild->GetStreamHierarchically( nStorageMode, aListPath, nStreamMode, aEncryptionData );
    if ( !xResult.is() )
        throw uno::RuntimeException();

    return xResult;
}

void OHierarchyHolder_Impl::RemoveStreamHierarchically( std::vector<OUString>& aListPath )
{
    m_xChild->RemoveStreamHierarchically( aListPath );
}

// static
std::vector<OUString> OHierarchyHolder_Impl::GetListPathFromString( std::u16string_view aPath )
{
    std::vector<OUString> aResult;
    sal_Int32 nIndex = 0;
    do
    {
        OUString aName( o3tl::getToken(aPath, 0, '/', nIndex ) );
        if ( aName.isEmpty() )
            throw lang::IllegalArgumentException();

        aResult.push_back( aName );
    }
    while ( nIndex >= 0 );

    return aResult;
}

// OHierarchyElement_Impl

uno::Reference< embed::XExtendedStorageStream > OHierarchyElement_Impl::GetStreamHierarchically( sal_Int32 nStorageMode, std::vector<OUString>& aListPath, sal_Int32 nStreamMode, const ::comphelper::SequenceAsHashMap& aEncryptionData )
{
    std::unique_lock aGuard( m_aMutex );

    if ( !( nStorageMode & embed::ElementModes::WRITE ) && ( nStreamMode & embed::ElementModes::WRITE ) )
        throw io::IOException(u"invalid storage/stream mode combo"_ustr);

    if ( aListPath.empty() )
        throw uno::RuntimeException();

    OUString aNextName = *(aListPath.begin());
    aListPath.erase( aListPath.begin() );

    uno::Reference< embed::XExtendedStorageStream > xResult;

    rtl::Reference< OStorage > xOwnStor = m_xOwnStorage.is() ? m_xOwnStorage
                : m_xWeakOwnStorage.get();
    if (!xOwnStor)
        throw uno::RuntimeException(u"no own storage"_ustr);

    if ( aListPath.empty() )
    {
        if ( aEncryptionData.empty() )
            xResult = xOwnStor->openStreamElementByHierarchicalName( aNextName, nStreamMode );
        else
            xResult = xOwnStor->openEncryptedStreamByHierarchicalName( aNextName, nStreamMode, aEncryptionData.getAsConstNamedValueList() );

        uno::Reference< embed::XTransactedObject > xTransact( xResult, uno::UNO_QUERY );
        if ( xTransact.is() )
        {
            // the existence of the transacted object means that the stream is opened for writing also
            // so the whole chain must be committed
            uno::Reference< embed::XTransactionBroadcaster > xTrBroadcast( xTransact, uno::UNO_QUERY_THROW );
            xTrBroadcast->addTransactionListener( static_cast< embed::XTransactionListener* >( this ) );
        }
        else
        {
            uno::Reference< lang::XComponent > xStreamComp( xResult, uno::UNO_QUERY_THROW );
            xStreamComp->addEventListener( static_cast< lang::XEventListener* >( this ) );
        }

        m_aOpenStreams.emplace_back( xResult );
    }
    else
    {
        bool bNewElement = false;
        ::rtl::Reference< OHierarchyElement_Impl > aElement;
        OHierarchyElementList_Impl::iterator aIter = m_aChildren.find( aNextName );
        if ( aIter != m_aChildren.end() )
            aElement = aIter->second;

        if ( !aElement.is() )
        {
            bNewElement = true;
            rtl::Reference< OStorage > xChildStorage = xOwnStor->openStorageElement2( aNextName, nStorageMode );
            if ( !xChildStorage.is() )
                throw uno::RuntimeException();

            aElement = new OHierarchyElement_Impl( xChildStorage );
        }

        xResult = aElement->GetStreamHierarchically( nStorageMode, aListPath, nStreamMode, aEncryptionData );
        if ( !xResult.is() )
            throw uno::RuntimeException();

        if ( bNewElement )
        {
            m_aChildren[aNextName] = aElement;
            aElement->SetParent( this );
        }
    }

    // the subelement was opened successfully, remember the storage to let it be locked
    m_xOwnStorage = std::move(xOwnStor);

    return xResult;
}

void OHierarchyElement_Impl::RemoveStreamHierarchically( std::vector<OUString>& aListPath )
{
    std::unique_lock aGuard( m_aMutex );

    if ( aListPath.empty() )
        throw uno::RuntimeException();

    OUString aNextName = *(aListPath.begin());
    aListPath.erase( aListPath.begin() );

    rtl::Reference< OStorage > xOwnStor = m_xOwnStorage.is() ? m_xOwnStorage
                : m_xWeakOwnStorage.get();
    if (!xOwnStor)
        throw uno::RuntimeException(u"no own storage"_ustr);

    if ( aListPath.empty() )
    {
        xOwnStor->removeElement( aNextName );
    }
    else
    {
        ::rtl::Reference< OHierarchyElement_Impl > aElement;
        OHierarchyElementList_Impl::iterator aIter = m_aChildren.find( aNextName );
        if ( aIter != m_aChildren.end() )
            aElement = aIter->second;

        if ( !aElement.is() )
        {
            rtl::Reference< OStorage > xChildStorage = xOwnStor->openStorageElement2( aNextName,
                                                                                            embed::ElementModes::READWRITE );
            if ( !xChildStorage.is() )
                throw uno::RuntimeException();

            aElement = new OHierarchyElement_Impl( xChildStorage );
        }

        aElement->RemoveStreamHierarchically( aListPath );
    }

    xOwnStor->commit();

    TestForClosing();
}

void OHierarchyElement_Impl::Commit()
{
    ::rtl::Reference< OHierarchyElement_Impl > xKeepAlive( this );
    ::rtl::Reference< OHierarchyElement_Impl > aParent;
    rtl::Reference<OStorage> xOwnStor;

    {
        std::unique_lock aGuard( m_aMutex );
        aParent = m_rParent;
        xOwnStor = m_xOwnStorage;
    }

    if ( xOwnStor.is() )
    {
        xOwnStor->commit();
        if ( aParent.is() )
            aParent->Commit();
    }
}

void OHierarchyElement_Impl::TestForClosing()
{
    ::rtl::Reference< OHierarchyElement_Impl > xKeepAlive( this );
    {
        std::unique_lock aGuard( m_aMutex );

        if ( m_aOpenStreams.empty() && m_aChildren.empty() )
        {
            if ( m_rParent.is() )
            {
                // only the root storage should not be disposed, other storages can be disposed
                if ( m_xOwnStorage.is() )
                {
                    try
                    {
                        m_xOwnStorage->dispose();
                    }
                    catch( uno::Exception& )
                    {}
                }

                m_rParent->RemoveElement( this );
            }

            m_xOwnStorage.clear();
        }
    }
}

void SAL_CALL OHierarchyElement_Impl::disposing( const lang::EventObject& Source )
{
    try
    {
        {
            std::unique_lock aGuard(m_aMutex);
            uno::Reference< embed::XExtendedStorageStream > xStream(Source.Source, uno::UNO_QUERY);

            std::erase_if(m_aOpenStreams,
                [&xStream](const OWeakStorRefVector_Impl::value_type& rxStorage) {
                return !rxStorage.get().is() || rxStorage.get() == xStream; });
        }

        TestForClosing();
    }
    catch( uno::Exception& ex )
    {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetRuntimeException( ex.Message,
                        nullptr, anyEx ); // no exception must happen here, usually an exception means disaster
    }
}

void OHierarchyElement_Impl::RemoveElement( const ::rtl::Reference< OHierarchyElement_Impl >& aRef )
{
    {
        std::unique_lock aGuard( m_aMutex );
        OHierarchyElementList_Impl::iterator aIter = m_aChildren.begin();
        while (aIter != m_aChildren.end())
        {
            if (aIter->second == aRef )
                aIter = m_aChildren.erase(aIter);
            else
                ++aIter;
        }
    }

    TestForClosing();
}

// XTransactionListener
void SAL_CALL OHierarchyElement_Impl::preCommit( const css::lang::EventObject& /*aEvent*/ )
{
}

void SAL_CALL OHierarchyElement_Impl::commited( const css::lang::EventObject& /*aEvent*/ )
{
    try
    {
        Commit();
    }
    catch( const uno::Exception& )
    {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetRuntimeException(
                            u"Can not commit storage sequence!"_ustr,
                            uno::Reference< uno::XInterface >(),
                            anyEx );
    }
}

void SAL_CALL OHierarchyElement_Impl::preRevert( const css::lang::EventObject& /*aEvent*/ )
{
}

void SAL_CALL OHierarchyElement_Impl::reverted( const css::lang::EventObject& /*aEvent*/ )
{
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
