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

#include <cassert>

#include <com/sun/star/embed/EmbedUpdateModes.hpp>
#include <com/sun/star/embed/EmbedStates.hpp>
#include <com/sun/star/embed/WrongStateException.hpp>
#include <com/sun/star/lang/NoSupportException.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/lang/DisposedException.hpp>

#include <comphelper/multicontainer2.hxx>
#include <comphelper/sequenceashashmap.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/diagnose.h>

#include <oleembobj.hxx>
#include <utility>
#include "olepersist.hxx"

#include "ownview.hxx"

#include "olecomponent.hxx"

using namespace ::com::sun::star;


OleEmbeddedObject::OleEmbeddedObject( uno::Reference< uno::XComponentContext > xContext,
                                      const uno::Sequence< sal_Int8 >& aClassID,
                                      OUString aClassName )
: m_bReadOnly( false )
, m_bDisposed( false )
, m_nObjectState( -1 )
, m_nTargetState( -1 )
, m_nUpdateMode ( embed::EmbedUpdateModes::ALWAYS_UPDATE )
, m_xContext(std::move( xContext ))
, m_aClassID( aClassID )
, m_aClassName(std::move( aClassName ))
, m_bWaitSaveCompleted( false )
, m_bNewVisReplInStream( true )
, m_bStoreLoaded( false )
, m_bVisReplInitialized( false )
, m_bVisReplInStream( false )
, m_bStoreVisRepl( false )
, m_bIsLink( false )
, m_bHasCachedSize( false )
, m_nCachedAspect( 0 )
, m_bHasSizeToSet( false )
, m_nAspectToSet( 0 )
, m_bGotStatus( false )
, m_nStatus( 0 )
, m_nStatusAspect( 0 )
, m_bFromClipboard( false )
, m_bTriedConversion( false )
{
}


// In case of loading from persistent entry the classID of the object
// will be retrieved from the entry, during construction it is unknown
OleEmbeddedObject::OleEmbeddedObject( uno::Reference< uno::XComponentContext > xContext, bool bLink )
: m_bReadOnly( false )
, m_bDisposed( false )
, m_nObjectState( -1 )
, m_nTargetState( -1 )
, m_nUpdateMode( embed::EmbedUpdateModes::ALWAYS_UPDATE )
, m_xContext(std::move( xContext ))
, m_bWaitSaveCompleted( false )
, m_bNewVisReplInStream( true )
, m_bStoreLoaded( false )
, m_bVisReplInitialized( false )
, m_bVisReplInStream( false )
, m_bStoreVisRepl( false )
, m_bIsLink( bLink )
, m_bHasCachedSize( false )
, m_nCachedAspect( 0 )
, m_bHasSizeToSet( false )
, m_nAspectToSet( 0 )
, m_bGotStatus( false )
, m_nStatus( 0 )
, m_nStatusAspect( 0 )
, m_bFromClipboard( false )
, m_bTriedConversion( false )
{
}
#ifdef _WIN32

// this constructor let object be initialized from clipboard
OleEmbeddedObject::OleEmbeddedObject( const uno::Reference< uno::XComponentContext >& xContext )
: m_bReadOnly( false )
, m_bDisposed( false )
, m_nObjectState( -1 )
, m_nTargetState( -1 )
, m_nUpdateMode( embed::EmbedUpdateModes::ALWAYS_UPDATE )
, m_xContext( xContext )
, m_bWaitSaveCompleted( false )
, m_bNewVisReplInStream( true )
, m_bStoreLoaded( false )
, m_bVisReplInitialized( false )
, m_bVisReplInStream( false )
, m_bStoreVisRepl( false )
, m_bIsLink( false )
, m_bHasCachedSize( false )
, m_nCachedAspect( 0 )
, m_bHasSizeToSet( false )
, m_nAspectToSet( 0 )
, m_bGotStatus( false )
, m_nStatus( 0 )
, m_nStatusAspect( 0 )
, m_bFromClipboard( true )
, m_bTriedConversion( false )
{
}
#endif

OleEmbeddedObject::~OleEmbeddedObject()
{
    OSL_ENSURE( !m_pInterfaceContainer && !m_pOleComponent && !m_xObjectStream.is(),
                    "The object is not closed! DISASTER is possible!" );

    if ( m_pOleComponent || m_pInterfaceContainer || m_xObjectStream.is() )
    {
        // the component must be cleaned during closing
        osl_atomic_increment(&m_refCount); // to avoid crash
        try {
            Dispose();
        } catch( const uno::Exception& ) {}
    }

    if ( !m_aTempURL.isEmpty() )
           KillFile_Impl( m_aTempURL, m_xContext );

    if ( !m_aTempDumpURL.isEmpty() )
           KillFile_Impl( m_aTempDumpURL, m_xContext );
}


void OleEmbeddedObject::MakeEventListenerNotification_Impl( const OUString& aEventName,
                                                      osl::ResettableMutexGuard& guard )
{
    if ( !m_pInterfaceContainer )
        return;

    comphelper::OInterfaceContainerHelper2* pContainer =
        m_pInterfaceContainer->getContainer(
                                cppu::UnoType<document::XEventListener>::get());
    if ( pContainer == nullptr )
        return;

    auto proc = [&guard, aEvent = document::EventObject(getXWeak(), aEventName)](
                    const uno::Reference<document::XEventListener>& xListener)
    {
        try
        {
            osl::ResettableMutexGuardScopedReleaser area(guard);
            xListener->notifyEvent(aEvent);
        }
        catch (const lang::DisposedException&)
        {
            throw; // forEach handles this
        }
        catch (const uno::RuntimeException&)
        {
        }
    };
    pContainer->forEach<document::XEventListener>(proc);
}
#ifdef _WIN32

void OleEmbeddedObject::StateChangeNotification_Impl( bool bBeforeChange, sal_Int32 nOldState, sal_Int32 nNewState,
                                                      osl::ResettableMutexGuard& guard )
{
    if (!m_pInterfaceContainer)
        return;

    comphelper::OInterfaceContainerHelper2* pContainer = m_pInterfaceContainer->getContainer(
                        cppu::UnoType<embed::XStateChangeListener>::get());
    if (!pContainer)
        return;

    auto proc
        = [bBeforeChange, nOldState, nNewState, &guard, aSource = lang::EventObject(getXWeak())](
              const uno::Reference<embed::XStateChangeListener>& xListener)
    {
        try
        {
            osl::ResettableMutexGuardScopedReleaser area(guard);
            if (bBeforeChange)
                xListener->changingState(aSource, nOldState, nNewState);
            else
                xListener->stateChanged(aSource, nOldState, nNewState);
        }
        catch (const lang::DisposedException&)
        {
            throw; // forEach handles this
        }
        catch (const uno::Exception&)
        {
            // even if the listener complains ignore it for now
        }
    };
    pContainer->forEach<embed::XStateChangeListener>(proc);
}
#endif

void OleEmbeddedObject::GetRidOfComponent(osl::ResettableMutexGuard* guard)
{
#ifdef _WIN32
    if ( m_pOleComponent )
    {
        if ( m_nObjectState != -1 && m_nObjectState != embed::EmbedStates::LOADED )
            SaveObject_Impl();

        m_pOleComponent->removeCloseListener( m_xClosePreventer );
        // When releasing the guard below, avoid a case when two threads are doing the same;
        // store the reference on stack and clear m_pOleComponent in advance
        rtl::Reference<OleComponent> pOleComponent(std::move(m_pOleComponent));
        try
        {
            std::optional<osl::ResettableMutexGuardScopedReleaser> oReleaser;
            if (guard)
                oReleaser.emplace(*guard);
            pOleComponent->close(false);
        }
        catch( const uno::Exception& )
        {
            m_pOleComponent = std::move(pOleComponent);
            // TODO: there should be a special listener to wait for component closing
            //       and to notify object, may be object itself can be such a listener
            m_pOleComponent->addCloseListener( m_xClosePreventer );
            throw;
        }

        pOleComponent->disconnectEmbeddedObject();
    }
#else
    (void)guard;
#endif
}


void OleEmbeddedObject::Dispose(osl::ResettableMutexGuard* guard)
{
    if ( m_pInterfaceContainer )
    {
        lang::EventObject aSource( static_cast< ::cppu::OWeakObject* >( this ) );
        m_pInterfaceContainer->disposeAndClear( aSource );
        m_pInterfaceContainer.reset();
    }

    if ( m_xOwnView.is() )
    {
        m_xOwnView->Close();
        m_xOwnView.clear();
    }

    if ( m_pOleComponent )
        try {
            GetRidOfComponent(guard);
        } catch( const uno::Exception& )
        {
            m_bDisposed = true;
            throw; // TODO: there should be a special listener that will close object when
                    // component is finally closed
        }

    if ( m_xObjectStream.is() )
    {
        uno::Reference< lang::XComponent > xComp( m_xObjectStream, uno::UNO_QUERY );
        OSL_ENSURE( xComp.is(), "Storage stream doesn't support XComponent!" );

        if ( xComp.is() )
        {
            try {
                xComp->dispose();
            } catch( const uno::Exception& ) {}
        }
        m_xObjectStream.clear();
    }

    m_xParentStorage.clear();
    m_xClientSite.clear();
    m_xClosePreventer.clear();
    m_xNewCachedVisRepl.clear();
    m_xNewParentStorage.clear();
    m_xNewObjectStream.clear();
    m_xCachedVisualRepresentation.clear();
    m_xWrappedObject.clear();
    m_xParent.clear();
    m_pOleComponent.clear();

    m_bDisposed = true;
}


uno::Sequence< sal_Int8 > SAL_CALL OleEmbeddedObject::getClassID()
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->getClassID();
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    return m_aClassID;
}


OUString SAL_CALL OleEmbeddedObject::getClassName()
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->getClassName();
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    return m_aClassName;
}


void SAL_CALL OleEmbeddedObject::setClassInfo(
                const uno::Sequence< sal_Int8 >& aClassID, const OUString& aClassName )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->setClassInfo( aClassID, aClassName );
        return;
    }
    // end wrapping related part ====================

    // the object class info can not be changed explicitly
    throw lang::NoSupportException(); //TODO:
}


uno::Reference< util::XCloseable > SAL_CALL OleEmbeddedObject::getComponent()
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->getComponent();
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_nObjectState == -1 ) // || m_nObjectState == embed::EmbedStates::LOADED )
    {
        // the object is still not running
        throw uno::RuntimeException( u"The object is not loaded!"_ustr,
                                        static_cast< ::cppu::OWeakObject* >(this) );
    }

#if defined(_WIN32)
    if (m_pOleComponent.is())
    {
        return uno::Reference< util::XCloseable >( m_pOleComponent );
    }
#endif

    assert(!m_pOleComponent.is());
    // TODO/LATER: Is it correct???
    return uno::Reference< util::XCloseable >();
    // throw uno::RuntimeException(); // TODO
}


void SAL_CALL OleEmbeddedObject::addStateChangeListener( const uno::Reference< embed::XStateChangeListener >& xListener )
{
    // begin wrapping related part ====================
    if ( m_xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        m_xWrappedObject->addStateChangeListener( xListener );
        return;
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( !m_pInterfaceContainer )
        m_pInterfaceContainer.reset(new comphelper::OMultiTypeInterfaceContainerHelper2( m_aMutex ));

    m_pInterfaceContainer->addInterface( cppu::UnoType<embed::XStateChangeListener>::get(),
                                                        xListener );
}


void SAL_CALL OleEmbeddedObject::removeStateChangeListener(
                    const uno::Reference< embed::XStateChangeListener >& xListener )
{
    // begin wrapping related part ====================
    if ( m_xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        m_xWrappedObject->removeStateChangeListener( xListener );
        return;
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_pInterfaceContainer )
        m_pInterfaceContainer->removeInterface( cppu::UnoType<embed::XStateChangeListener>::get(),
                                                xListener );
}


void SAL_CALL OleEmbeddedObject::close( sal_Bool bDeliverOwnership )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->close( bDeliverOwnership );
        return;
    }
    // end wrapping related part ====================

    osl::ResettableMutexGuard aGuard(m_aMutex);
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    uno::Reference< uno::XInterface > xSelfHold( static_cast< ::cppu::OWeakObject* >( this ) );
    lang::EventObject aSource( static_cast< ::cppu::OWeakObject* >( this ) );

    if ( m_pInterfaceContainer )
    {
        comphelper::OInterfaceContainerHelper2* pContainer =
            m_pInterfaceContainer->getContainer( cppu::UnoType<util::XCloseListener>::get());
        if ( pContainer != nullptr )
        {
            comphelper::OInterfaceIteratorHelper2 pIterator(*pContainer);
            while (pIterator.hasMoreElements())
            {
                try
                {
                    static_cast<util::XCloseListener*>(pIterator.next())->queryClosing( aSource, bDeliverOwnership );
                }
                catch( const uno::RuntimeException& )
                {
                    pIterator.remove();
                }
            }
        }

        pContainer = m_pInterfaceContainer->getContainer(
                                    cppu::UnoType<util::XCloseListener>::get());
        if ( pContainer != nullptr )
        {
            comphelper::OInterfaceIteratorHelper2 pCloseIterator(*pContainer);
            while (pCloseIterator.hasMoreElements())
            {
                try
                {
                    static_cast<util::XCloseListener*>(pCloseIterator.next())->notifyClosing( aSource );
                }
                catch( const uno::RuntimeException& )
                {
                    pCloseIterator.remove();
                }
            }
        }
    }

    Dispose(&aGuard);
}


void SAL_CALL OleEmbeddedObject::addCloseListener( const uno::Reference< util::XCloseListener >& xListener )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->addCloseListener( xListener );
        return;
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( !m_pInterfaceContainer )
        m_pInterfaceContainer.reset(new comphelper::OMultiTypeInterfaceContainerHelper2( m_aMutex ));

    m_pInterfaceContainer->addInterface( cppu::UnoType<util::XCloseListener>::get(), xListener );
}


void SAL_CALL OleEmbeddedObject::removeCloseListener( const uno::Reference< util::XCloseListener >& xListener )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->removeCloseListener( xListener );
        return;
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_pInterfaceContainer )
        m_pInterfaceContainer->removeInterface( cppu::UnoType<util::XCloseListener>::get(),
                                                xListener );
}


void SAL_CALL OleEmbeddedObject::addEventListener( const uno::Reference< document::XEventListener >& xListener )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->addEventListener( xListener );
        return;
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( !m_pInterfaceContainer )
        m_pInterfaceContainer.reset(new comphelper::OMultiTypeInterfaceContainerHelper2( m_aMutex ));

    m_pInterfaceContainer->addInterface( cppu::UnoType<document::XEventListener>::get(), xListener );
}


void SAL_CALL OleEmbeddedObject::removeEventListener(
                const uno::Reference< document::XEventListener >& xListener )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->removeEventListener( xListener );
        return;
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_pInterfaceContainer )
        m_pInterfaceContainer->removeInterface( cppu::UnoType<document::XEventListener>::get(),
                                                xListener );
}

// XInplaceObject ( wrapper related implementation )

void SAL_CALL OleEmbeddedObject::setObjectRectangles( const awt::Rectangle& aPosRect,
                                                           const awt::Rectangle& aClipRect )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XInplaceObject > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->setObjectRectangles( aPosRect, aClipRect );
        return;
    }
    // end wrapping related part ====================

    throw embed::WrongStateException();
}


void SAL_CALL OleEmbeddedObject::enableModeless( sal_Bool bEnable )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XInplaceObject > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->enableModeless( bEnable );
        return;
    }
    // end wrapping related part ====================

    throw embed::WrongStateException();
}


void SAL_CALL OleEmbeddedObject::translateAccelerators(
                    const uno::Sequence< awt::KeyEvent >& aKeys )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XInplaceObject > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->translateAccelerators( aKeys );
        return;
    }
    // end wrapping related part ====================

}

// XChild

css::uno::Reference< css::uno::XInterface > SAL_CALL OleEmbeddedObject::getParent()
{
    // begin wrapping related part ====================
    uno::Reference< container::XChild > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->getParent();
    }
    // end wrapping related part ====================

    return m_xParent;
}


void SAL_CALL OleEmbeddedObject::setParent( const css::uno::Reference< css::uno::XInterface >& xParent )
{
    // begin wrapping related part ====================
    uno::Reference< container::XChild > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->setParent( xParent );
        return;
    }
    // end wrapping related part ====================

    m_xParent = xParent;
}

void OleEmbeddedObject::setStream(const css::uno::Reference<css::io::XStream>& xStream)
{
    m_xObjectStream = xStream;
}

css::uno::Reference<css::io::XStream> OleEmbeddedObject::getStream()
{
    return m_xObjectStream;
}

void OleEmbeddedObject::initialize(const uno::Sequence<uno::Any>& rArguments)
{
    if (!rArguments.hasElements())
        return;

    comphelper::SequenceAsHashMap aValues(rArguments[0]);
    auto it = aValues.find(u"StreamReadOnly"_ustr);
    if (it != aValues.end())
        it->second >>= m_bStreamReadOnly;
}

OUString SAL_CALL OleEmbeddedObject::getImplementationName()
{
    return u"com.sun.star.comp.embed.OleEmbeddedObject"_ustr;
}

sal_Bool SAL_CALL OleEmbeddedObject::supportsService(const OUString& ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

uno::Sequence<OUString> SAL_CALL OleEmbeddedObject::getSupportedServiceNames()
{
    return { u"com.sun.star.comp.embed.OleEmbeddedObject"_ustr };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
