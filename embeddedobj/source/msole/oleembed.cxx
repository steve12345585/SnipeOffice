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

#include <oleembobj.hxx>
#include <com/sun/star/embed/EmbedStates.hpp>
#include <com/sun/star/embed/EmbedVerbs.hpp>
#include <com/sun/star/embed/UnreachableStateException.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/EmbedUpdateModes.hpp>
#include <com/sun/star/embed/NeedsRunningStateException.hpp>
#include <com/sun/star/embed/StateChangeInProgressException.hpp>
#include <com/sun/star/embed/EmbedMisc.hpp>
#include <com/sun/star/embed/XEmbedObjectCreator.hpp>
#include <com/sun/star/io/TempFile.hpp>
#include <com/sun/star/io/XSeekable.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/lang/WrappedTargetRuntimeException.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/frame/XLoadable.hpp>
#include <com/sun/star/document/XStorageBasedDocument.hpp>
#include <com/sun/star/ucb/SimpleFileAccess.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/system/SystemShellExecute.hpp>
#include <com/sun/star/system/SystemShellExecuteFlags.hpp>

#include <cppuhelper/exc_hlp.hxx>
#include <comphelper/multicontainer2.hxx>
#include <comphelper/mimeconfighelper.hxx>
#include <comphelper/propertyvalue.hxx>
#include <sal/log.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <officecfg/Office/Common.hxx>

#include <targetstatecontrol.hxx>

#include "ownview.hxx"

#if defined(_WIN32)
#include "olecomponent.hxx"
#endif

using namespace ::com::sun::star;

#ifdef _WIN32

void OleEmbeddedObject::SwitchComponentToRunningState_Impl(osl::ResettableMutexGuard& guard)
{
    if ( !m_pOleComponent )
    {
        throw embed::UnreachableStateException();
    }
    try
    {
        m_pOleComponent->RunObject();
    }
    catch( const embed::UnreachableStateException& )
    {
        GetRidOfComponent(&guard);
        throw;
    }
    catch( const embed::WrongStateException& )
    {
        GetRidOfComponent(&guard);
        throw;
    }
}


uno::Sequence< sal_Int32 > OleEmbeddedObject::GetReachableStatesList_Impl(
                                                        const uno::Sequence< embed::VerbDescriptor >& aVerbList )
{
    uno::Sequence< sal_Int32 > aStates { embed::EmbedStates::LOADED, embed::EmbedStates::RUNNING };
    for ( embed::VerbDescriptor const & vd : aVerbList )
        if ( vd.VerbID == embed::EmbedVerbs::MS_OLEVERB_OPEN )
        {
            aStates.realloc(3);
            aStates.getArray()[2] = embed::EmbedStates::ACTIVE;
            break;
        }

    return aStates;
}


uno::Sequence< sal_Int32 > OleEmbeddedObject::GetIntermediateVerbsSequence_Impl( sal_Int32 nNewState )
{
    SAL_WARN_IF( m_nObjectState == embed::EmbedStates::LOADED, "embeddedobj.ole", "Loaded object is switched to running state without verbs using!" );

    // actually there will be only one verb
    if ( m_nObjectState == embed::EmbedStates::RUNNING && nNewState == embed::EmbedStates::ACTIVE )
    {
        return { embed::EmbedVerbs::MS_OLEVERB_OPEN };
    }

    return uno::Sequence< sal_Int32 >();
}
#endif

void OleEmbeddedObject::MoveListeners()
{
    if ( !m_pInterfaceContainer )
        return;

    // move state change listeners
    {
        comphelper::OInterfaceContainerHelper2* pStateChangeContainer =
            m_pInterfaceContainer->getContainer( cppu::UnoType<embed::XStateChangeListener>::get());
        if ( pStateChangeContainer != nullptr )
        {
            if ( m_xWrappedObject.is() )
            {
                comphelper::OInterfaceIteratorHelper2 pIterator( *pStateChangeContainer );
                while ( pIterator.hasMoreElements() )
                {
                    try
                    {
                        m_xWrappedObject->addStateChangeListener( static_cast<embed::XStateChangeListener*>(pIterator.next()) );
                    }
                    catch( const uno::RuntimeException& )
                    {
                        pIterator.remove();
                    }
                }
            }
        }
    }

    // move event listeners
    {
        comphelper::OInterfaceContainerHelper2* pEventContainer =
            m_pInterfaceContainer->getContainer( cppu::UnoType<document::XEventListener>::get());
        if ( pEventContainer != nullptr )
        {
            if ( m_xWrappedObject.is() )
            {
                comphelper::OInterfaceIteratorHelper2 pIterator( *pEventContainer );
                while ( pIterator.hasMoreElements() )
                {
                    try
                    {
                        m_xWrappedObject->addEventListener( static_cast<document::XEventListener*>(pIterator.next()) );
                    }
                    catch( const uno::RuntimeException& )
                    {
                        pIterator.remove();
                    }
                }
            }
        }
    }

    // move close listeners
    {
        comphelper::OInterfaceContainerHelper2* pCloseContainer =
            m_pInterfaceContainer->getContainer( cppu::UnoType<util::XCloseListener>::get());
        if ( pCloseContainer != nullptr )
        {
            if ( m_xWrappedObject.is() )
            {
                comphelper::OInterfaceIteratorHelper2 pIterator( *pCloseContainer );
                while ( pIterator.hasMoreElements() )
                {
                    try
                    {
                        m_xWrappedObject->addCloseListener( static_cast<util::XCloseListener*>(pIterator.next()) );
                    }
                    catch( const uno::RuntimeException& )
                    {
                        pIterator.remove();
                    }
                }
            }
        }
    }

    m_pInterfaceContainer.reset();
}


uno::Reference< embed::XStorage > OleEmbeddedObject::CreateTemporarySubstorage( OUString& o_aStorageName )
{
    uno::Reference< embed::XStorage > xResult;

    for ( sal_Int32 nInd = 0; nInd < 32000 && !xResult.is(); nInd++ )
    {
        OUString aName = OUString::number( nInd ) + "TMPSTOR" + m_aEntryName;
        if ( !m_xParentStorage->hasByName( aName ) )
        {
            xResult = m_xParentStorage->openStorageElement( aName, embed::ElementModes::READWRITE );
            o_aStorageName = aName;
        }
    }

    if ( !xResult.is() )
    {
        o_aStorageName.clear();
        throw uno::RuntimeException(u"Failed to create temporary storage for OLE embed object"_ustr);
    }

    return xResult;
}


OUString OleEmbeddedObject::MoveToTemporarySubstream()
{
    OUString aResult;
    for ( sal_Int32 nInd = 0; nInd < 32000 && aResult.isEmpty(); nInd++ )
    {
        OUString aName = OUString::number( nInd ) + "TMPSTREAM" + m_aEntryName;
        if ( !m_xParentStorage->hasByName( aName ) )
        {
            m_xParentStorage->renameElement( m_aEntryName, aName );
            aResult = aName;
        }
    }

    if ( aResult.isEmpty() )
        throw uno::RuntimeException(u"Failed to rename temporary storage for OLE embed object"_ustr);

    return aResult;
}


bool OleEmbeddedObject::TryToConvertToOOo( const uno::Reference< io::XStream >& xStream )
{
    bool bResult = false;

    OUString aStorageName;
    OUString aTmpStreamName;
    sal_Int32 nStep = 0;

    if ( m_pOleComponent || m_bReadOnly )
        return false;

    try
    {
        changeState( embed::EmbedStates::LOADED );

        // the stream must be seekable
        uno::Reference< io::XSeekable > xSeekable( xStream, uno::UNO_QUERY_THROW );
        xSeekable->seek( 0 );
        m_aFilterName = OwnView_Impl::GetFilterNameFromExtentionAndInStream( m_xContext, std::u16string_view(), xStream->getInputStream() );

        if ( !m_aFilterName.isEmpty()
          && ( m_aFilterName == "Calc MS Excel 2007 XML" || m_aFilterName == "Impress MS PowerPoint 2007 XML" || m_aFilterName == "MS Word 2007 XML"
              || m_aFilterName == "MS Excel 97 Vorlage/Template" || m_aFilterName == "MS Word 97 Vorlage" ) )
        {
            uno::Reference< container::XNameAccess > xFilterFactory(
                m_xContext->getServiceManager()->createInstanceWithContext(u"com.sun.star.document.FilterFactory"_ustr, m_xContext),
                uno::UNO_QUERY_THROW );

            OUString aDocServiceName;
            uno::Any aFilterAnyData = xFilterFactory->getByName( m_aFilterName );
            uno::Sequence< beans::PropertyValue > aFilterData;
            if ( aFilterAnyData >>= aFilterData )
            {
                for (beans::PropertyValue const& prop : aFilterData)
                    if ( prop.Name == "DocumentService" )
                        prop.Value >>= aDocServiceName;
            }

            if ( !aDocServiceName.isEmpty() )
            {
                // create the model
                uno::Sequence< uno::Any > aArguments{ uno::Any(
                    beans::NamedValue( u"EmbeddedObject"_ustr, uno::Any( true ))) };

                uno::Reference< util::XCloseable > xDocument( m_xContext->getServiceManager()->createInstanceWithArgumentsAndContext( aDocServiceName, aArguments, m_xContext ), uno::UNO_QUERY_THROW );
                uno::Reference< frame::XLoadable > xLoadable( xDocument, uno::UNO_QUERY_THROW );
                uno::Reference< document::XStorageBasedDocument > xStorDoc( xDocument, uno::UNO_QUERY_THROW );

                // let the model behave as embedded one
                uno::Reference< frame::XModel > xModel( xDocument, uno::UNO_QUERY_THROW );
                uno::Sequence< beans::PropertyValue > aSeq{ comphelper::makePropertyValue(
                    u"SetEmbedded"_ustr, true) };
                xModel->attachResource( OUString(), aSeq );

                // load the model from the stream
                uno::Sequence< beans::PropertyValue > aArgs{
                    comphelper::makePropertyValue(u"HierarchicalDocumentName"_ustr, m_aEntryName),
                    comphelper::makePropertyValue(u"ReadOnly"_ustr, true),
                    comphelper::makePropertyValue(u"FilterName"_ustr, m_aFilterName),
                    comphelper::makePropertyValue(u"URL"_ustr, u"private:stream"_ustr),
                    comphelper::makePropertyValue(u"InputStream"_ustr, xStream->getInputStream())
                };

                xSeekable->seek( 0 );
                xLoadable->load( aArgs );

                // the model is successfully loaded, create a new storage and store the model to the storage
                uno::Reference< embed::XStorage > xTmpStorage = CreateTemporarySubstorage( aStorageName );
                xStorDoc->storeToStorage( xTmpStorage, uno::Sequence< beans::PropertyValue >() );
                xDocument->close( true );
                uno::Reference< beans::XPropertySet > xStorProps( xTmpStorage, uno::UNO_QUERY_THROW );
                OUString aMediaType;
                xStorProps->getPropertyValue(u"MediaType"_ustr) >>= aMediaType;
                xTmpStorage->dispose();

                // look for the related embedded object factory
                ::comphelper::MimeConfigurationHelper aConfigHelper( m_xContext );
                OUString aEmbedFactory;
                if ( !aMediaType.isEmpty() )
                    aEmbedFactory = aConfigHelper.GetFactoryNameByMediaType( aMediaType );

                if ( aEmbedFactory.isEmpty() )
                    throw uno::RuntimeException(u"Failed to get OLE embedded object factory"_ustr);

                uno::Reference< uno::XInterface > xFact = m_xContext->getServiceManager()->createInstanceWithContext( aEmbedFactory, m_xContext );

                uno::Reference< embed::XEmbedObjectCreator > xEmbCreator( xFact, uno::UNO_QUERY_THROW );

                // now the object should be adjusted to become the wrapper
                nStep = 1;
                uno::Reference< lang::XComponent > xComp( m_xObjectStream, uno::UNO_QUERY_THROW );
                xComp->dispose();
                m_xObjectStream.clear();
                m_nObjectState = -1;

                nStep = 2;
                aTmpStreamName = MoveToTemporarySubstream();

                nStep = 3;
                m_xParentStorage->renameElement( aStorageName, m_aEntryName );

                nStep = 4;
                m_xWrappedObject.set( xEmbCreator->createInstanceInitFromEntry( m_xParentStorage, m_aEntryName, uno::Sequence< beans::PropertyValue >(), uno::Sequence< beans::PropertyValue >() ), uno::UNO_QUERY_THROW );

                // remember parent document name to show in the title bar
                m_xWrappedObject->setContainerName( m_aContainerName );

                bResult = true; // the change is no more revertable
                try
                {
                    m_xParentStorage->removeElement( aTmpStreamName );
                }
                catch( const uno::Exception& )
                {
                    // the success of the removing is not so important
                }
            }
        }
    }
    catch( const uno::Exception& )
    {
        // repair the object if necessary
        switch( nStep )
        {
            case 4:
            case 3:
            if ( !aTmpStreamName.isEmpty() && aTmpStreamName != m_aEntryName )
                try
                {
                    if ( m_xParentStorage->hasByName( m_aEntryName ) )
                        m_xParentStorage->removeElement( m_aEntryName );
                    m_xParentStorage->renameElement( aTmpStreamName, m_aEntryName );
                }
                catch ( const uno::Exception& ex )
                {
                    css::uno::Any anyEx = cppu::getCaughtException();
                    try {
                        close( true );
                    } catch( const uno::Exception& ) {}

                    m_xParentStorage->dispose(); // ??? the storage has information loss, it should be closed without committing!
                    throw css::lang::WrappedTargetRuntimeException( ex.Message,
                                    nullptr, anyEx ); // the repairing is not possible
                }
            [[fallthrough]];
            case 2:
                try
                {
                    m_xObjectStream = m_xParentStorage->openStreamElement( m_aEntryName, m_bReadOnly ? embed::ElementModes::READ : embed::ElementModes::READWRITE );
                    m_nObjectState = embed::EmbedStates::LOADED;
                }
                catch( const uno::Exception& ex )
                {
                    css::uno::Any anyEx = cppu::getCaughtException();
                    try {
                        close( true );
                    } catch( const uno::Exception& ) {}

                    throw css::lang::WrappedTargetRuntimeException( ex.Message,
                                    nullptr, anyEx ); // the repairing is not possible
                }
                [[fallthrough]];

            case 1:
            case 0:
                if ( !aStorageName.isEmpty() )
                    try {
                        m_xParentStorage->removeElement( aStorageName );
                    } catch( const uno::Exception& ) { SAL_WARN( "embeddedobj.ole", "Can not remove temporary storage!" ); }
                break;
        }
    }

    if ( bResult )
    {
        // the conversion was done successfully, now the additional initializations should happen

        MoveListeners();
        m_xWrappedObject->setClientSite( m_xClientSite );
        if ( m_xParent.is() )
        {
            uno::Reference< container::XChild > xChild( m_xWrappedObject, uno::UNO_QUERY );
            if ( xChild.is() )
                xChild->setParent( m_xParent );
        }

    }

    return bResult;
}


void SAL_CALL OleEmbeddedObject::changeState( sal_Int32 nNewState )
{
    if ( officecfg::Office::Common::Security::Scripting::DisableActiveContent::get()
         && nNewState != embed::EmbedStates::LOADED )
        throw embed::UnreachableStateException();
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->changeState( nNewState );
        return;
    }
    // end wrapping related part ====================

    ::osl::ResettableMutexGuard aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_nObjectState == -1 )
        throw embed::WrongStateException( u"The object has no persistence!"_ustr,
                                        static_cast< ::cppu::OWeakObject* >(this) );

    // in case the object is already in requested state
    if ( m_nObjectState == nNewState )
        return;

#ifdef _WIN32
    if ( m_pOleComponent )
    {
        if ( m_nTargetState != -1 )
        {
            // means that the object is currently trying to reach the target state
            throw embed::StateChangeInProgressException( OUString(),
                                                        uno::Reference< uno::XInterface >(),
                                                        m_nTargetState );
        }

        TargetStateControl_Impl aControl( m_nTargetState, nNewState );

        // TODO: additional verbs can be a problem, since nobody knows how the object
        //       will behave after activation

        sal_Int32 nOldState = m_nObjectState;
        StateChangeNotification_Impl( true, nOldState, nNewState, aGuard );

        try
        {
            if ( nNewState == embed::EmbedStates::LOADED )
            {
                // This means just closing of the current object
                // If component can not be closed the object stays in loaded state
                // and it holds reference to "incomplete" component
                // If the object is switched to running state later
                // the component will become "complete"

                // the loaded state must be set before, because of notifications!
                m_nObjectState = nNewState;

                {
                    VerbExecutionControllerGuard aVerbGuard( m_aVerbExecutionController );
                    ExecUnlocked([p = m_pOleComponent] { p->CloseObject(); }, aGuard);
                }

                StateChangeNotification_Impl( false, nOldState, m_nObjectState, aGuard );
            }
            else if ( nNewState == embed::EmbedStates::RUNNING || nNewState == embed::EmbedStates::ACTIVE )
            {
                if ( m_nObjectState == embed::EmbedStates::LOADED )
                {
                    // if the target object is in loaded state and a different state is specified
                    // as a new one the object first must be switched to running state.

                    // the component can exist already in nonrunning state
                    // it can be created during loading to detect type of object
                    CreateOleComponentAndLoad_Impl( m_pOleComponent );

                    SwitchComponentToRunningState_Impl(aGuard);
                    m_nObjectState = embed::EmbedStates::RUNNING;
                    StateChangeNotification_Impl( false, nOldState, m_nObjectState, aGuard );

                    if ( m_pOleComponent && m_bHasSizeToSet )
                    {
                        try {
                            ExecUnlocked([p = m_pOleComponent, s = m_aSizeToSet,
                                          a = m_nAspectToSet]() { p->SetExtent(s, a); },
                                         aGuard);
                            m_bHasSizeToSet = false;
                        }
                        catch( const uno::Exception& ) {}
                    }

                    if ( m_nObjectState == nNewState )
                        return;
                }

                // so now the object is either switched from Active to Running state or viceversa
                // the notification about object state change will be done asynchronously
                if ( m_nObjectState == embed::EmbedStates::RUNNING && nNewState == embed::EmbedStates::ACTIVE )
                {
                    // execute OPEN verb, if object does not reach active state it is an object's problem
                    ExecUnlocked([p = m_pOleComponent]()
                                 { p->ExecuteVerb(embed::EmbedVerbs::MS_OLEVERB_OPEN); },
                                 aGuard);

                    // some objects do not allow to set the size even in running state
                    if ( m_pOleComponent && m_bHasSizeToSet )
                    {
                        try {
                            ExecUnlocked([p = m_pOleComponent, s = m_aSizeToSet,
                                          a = m_nAspectToSet]() { p->SetExtent(s, a); },
                                         aGuard);
                            m_bHasSizeToSet = false;
                        }
                        catch( uno::Exception& ) {}
                    }

                    m_nObjectState = nNewState;
                }
                else if ( m_nObjectState == embed::EmbedStates::ACTIVE && nNewState == embed::EmbedStates::RUNNING )
                {
                    ExecUnlocked(
                        [p = m_pOleComponent]()
                        {
                            p->CloseObject();
                            p->RunObject(); // Should not fail, the object already was active
                        },
                        aGuard);
                    m_nObjectState = nNewState;
                }
                else
                {
                    throw embed::UnreachableStateException();
                }
            }
            else
                throw embed::UnreachableStateException();
        }
        catch( uno::Exception& )
        {
            StateChangeNotification_Impl( false, nOldState, m_nObjectState, aGuard );
            throw;
        }
    }
    else
#endif
    {
        throw embed::UnreachableStateException();
    }
}


uno::Sequence< sal_Int32 > SAL_CALL OleEmbeddedObject::getReachableStates()
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->getReachableStates();
    }
    // end wrapping related part ====================

    ::osl::ResettableMutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_nObjectState == -1 )
        throw embed::WrongStateException( u"The object has no persistence!"_ustr,
                                        static_cast< ::cppu::OWeakObject* >(this) );

#ifdef _WIN32
    if ( m_pOleComponent )
    {
        if ( m_nObjectState == embed::EmbedStates::LOADED )
        {
            // the list of supported verbs can be retrieved only when object is in running state
            throw embed::NeedsRunningStateException(); // TODO:
        }

        // the list of states can only be guessed based on standard verbs,
        // since there is no way to detect what additional verbs do
        // Pass m_pOleComponent to the lambda by copy, to make sure it doesn't depend on possible
        // destruction of 'this', while the lock is unset
        return GetReachableStatesList_Impl(
            ExecUnlocked([p = m_pOleComponent] { return p->GetVerbList(); }, aGuard));
    }
    else
#endif
    {
        return uno::Sequence< sal_Int32 >();
    }
}


sal_Int32 SAL_CALL OleEmbeddedObject::getCurrentState()
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->getCurrentState();
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_nObjectState == -1 )
        throw embed::WrongStateException( u"The object has no persistence!"_ustr,
                                        static_cast< ::cppu::OWeakObject* >(this) );

    // TODO: Shouldn't we ask object? ( I guess no )
    return m_nObjectState;
}

namespace
{
    bool lcl_CopyStream(const uno::Reference<io::XInputStream>& xIn, const uno::Reference<io::XOutputStream>& xOut, sal_Int32 nMaxCopy = SAL_MAX_INT32)
    {
        if (nMaxCopy <= 0)
            return false;

        const sal_Int32 nChunkSize = 4096;
        uno::Sequence< sal_Int8 > aData(nChunkSize);
        sal_Int32 nTotalRead = 0;
        sal_Int32 nRead;
        do
        {
            if (nTotalRead + aData.getLength() > nMaxCopy)
            {
                aData.realloc(nMaxCopy - nTotalRead);
            }
            nRead = xIn->readBytes(aData, aData.getLength());
            nTotalRead += nRead;
            xOut->writeBytes(aData);
        } while (nRead == nChunkSize && nTotalRead <= nMaxCopy);
        return nTotalRead != 0;
    }

    uno::Reference < io::XStream > lcl_GetExtractedStream( OUString& rUrl,
        const css::uno::Reference< css::uno::XComponentContext >& xContext,
        const css::uno::Reference< css::io::XStream >& xObjectStream )
    {
        uno::Reference <io::XTempFile> xNativeTempFile(
            io::TempFile::create(xContext),
            uno::UNO_SET_THROW);
        uno::Reference < io::XStream > xStream(xNativeTempFile);

        uno::Sequence< uno::Any > aArgs{ uno::Any(xObjectStream),
                                         uno::Any(true) }; // do not create copy
        uno::Reference< container::XNameContainer > xNameContainer(
            xContext->getServiceManager()->createInstanceWithArgumentsAndContext(
                u"com.sun.star.embed.OLESimpleStorage"_ustr,
                aArgs, xContext ), uno::UNO_QUERY_THROW );

        //various stream names that can contain the real document contents for
        //this object in a straightforward direct way
        static const std::u16string_view aStreamNames[] =
        {
            u"CONTENTS",
            u"Package",
            u"EmbeddedOdf",
            u"WordDocument",
            u"Workbook",
            u"PowerPoint Document"
        };

        bool bCopied = false;
        for (size_t i = 0; i < std::size(aStreamNames) && !bCopied; ++i)
        {
            uno::Reference<io::XStream> xEmbeddedFile;
            try
            {
                xNameContainer->getByName(OUString(aStreamNames[i])) >>= xEmbeddedFile;
            }
            catch (const container::NoSuchElementException&)
            {
                // ignore
            }
            bCopied = xEmbeddedFile.is() && lcl_CopyStream(xEmbeddedFile->getInputStream(), xStream->getOutputStream());
        }

        if (!bCopied)
        {
            uno::Reference< io::XStream > xOle10Native;
            try
            {
                xNameContainer->getByName(u"\1Ole10Native"_ustr) >>= xOle10Native;
            }
            catch (container::NoSuchElementException const&)
            {
                // ignore
            }
            if (xOle10Native.is())
            {
                const uno::Reference<io::XInputStream> xIn = xOle10Native->getInputStream();
                xIn->skipBytes(4); //size of the entire stream minus 4 bytes
                xIn->skipBytes(2); //word that represent the directory type
                uno::Sequence< sal_Int8 > aData(1);
                sal_Int32 nRead;
                do
                {
                    nRead = xIn->readBytes(aData, 1);
                } while (nRead == 1 && aData[0] != 0);  // file name plus extension of the attachment null terminated
                do
                {
                    nRead = xIn->readBytes(aData, 1);
                } while (nRead == 1 && aData[0] != 0);  // Fully Qualified File name with extension
                xIn->skipBytes(1); //single byte
                xIn->skipBytes(1); //single byte
                xIn->skipBytes(2); //Word that represent the directory type
                xIn->skipBytes(4); //len of string
                do
                {
                    nRead = xIn->readBytes(aData, 1);
                } while (nRead == 1 && aData[0] != 0);  // Actual string representing the file path
                uno::Sequence< sal_Int8 > aLenData(4);
                xIn->readBytes(aLenData, 4); //len of attachment
                sal_uInt32 nLen = static_cast<sal_uInt32>(
                                              (aLenData[0] & 0xFF) |
                                              ((aLenData[1] & 0xFF) <<  8) |
                                              ((aLenData[2] & 0xFF) << 16) |
                                              ((aLenData[3] & 0xFF) << 24));

                bCopied = lcl_CopyStream(xIn, xStream->getOutputStream(), nLen);
            }
        }

        uno::Reference< io::XSeekable > xSeekableStor(xObjectStream, uno::UNO_QUERY);
        if (xSeekableStor.is())
            xSeekableStor->seek(0);

        if (!bCopied)
            bCopied = lcl_CopyStream(xObjectStream->getInputStream(), xStream->getOutputStream());

        if (bCopied)
        {
            xNativeTempFile->setRemoveFile(false);
            rUrl = xNativeTempFile->getUri();

            xNativeTempFile.clear();

            uno::Reference < ucb::XSimpleFileAccess3 > xSimpleFileAccess(
                    ucb::SimpleFileAccess::create( xContext ) );

            xSimpleFileAccess->setReadOnly(rUrl, true);
        }
        else
        {
            xNativeTempFile->setRemoveFile(true);
        }

        return xStream;
    }

    //Dump the objects content to a tempfile, just the "CONTENTS" stream if
    //there is one for non-compound documents, otherwise the whole content.
    //On success a file is returned which must be removed by the caller
    OUString lcl_ExtractObject(const css::uno::Reference< css::uno::XComponentContext >& xContext,
        const css::uno::Reference< css::io::XStream >& xObjectStream)
    {
        OUString sUrl;

        // the solution is only active for Unix systems
#ifndef _WIN32
        lcl_GetExtractedStream(sUrl, xContext, xObjectStream);
#else
        (void) xContext;
        (void) xObjectStream;
#endif
        return sUrl;
    }

    uno::Reference < io::XStream > lcl_ExtractObjectStream( const css::uno::Reference< css::uno::XComponentContext >& xContext,
        const css::uno::Reference< css::io::XStream >& xObjectStream )
    {
        OUString sUrl;
        return lcl_GetExtractedStream( sUrl, xContext, xObjectStream );
    }
}


void SAL_CALL OleEmbeddedObject::doVerb( sal_Int32 nVerbID )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->doVerb(embed::EmbedVerbs::MS_OLEVERB_OPEN); // open content in the window not in-place
        return;
    }
    // end wrapping related part ====================

    ::osl::ResettableMutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_nObjectState == -1 )
        throw embed::WrongStateException( u"The object has no persistence!"_ustr,
                                        static_cast< ::cppu::OWeakObject* >(this) );

#ifdef _WIN32
    if ( m_pOleComponent )
    {
        sal_Int32 nOldState = m_nObjectState;

        // TODO/LATER detect target state here and do a notification
        // StateChangeNotification_Impl( sal_True, nOldState, nNewState );
        if ( m_nObjectState == embed::EmbedStates::LOADED )
        {
            // if the target object is in loaded state
            // it must be switched to running state to execute verb
            ExecUnlocked([this]() { changeState(embed::EmbedStates::RUNNING); }, aGuard);
        }

        try {
            if ( !m_pOleComponent )
                throw uno::RuntimeException("Null reference to OLE component");

            // ==== the STAMPIT related solution =============================
            m_aVerbExecutionController.StartControlExecution();

            ExecUnlocked(
                [nVerbID, p = m_pOleComponent, name = m_aContainerName]()
                {
                    p->ExecuteVerb(nVerbID);
                    p->SetHostName(name);
                },
                aGuard);

            // ==== the STAMPIT related solution =============================
            bool bModifiedOnExecution = m_aVerbExecutionController.EndControlExecution_WasModified();

            // this workaround is implemented for STAMPIT object
            // if object was modified during verb execution it is saved here
            if ( bModifiedOnExecution && m_pOleComponent->IsDirty() )
                SaveObject_Impl();

        }
        catch( uno::Exception& )
        {
            // ==== the STAMPIT related solution =============================
            m_aVerbExecutionController.EndControlExecution_WasModified();

            StateChangeNotification_Impl( false, nOldState, m_nObjectState, aGuard );
            throw;
        }

    }
    else
#endif
    {
        if ( nVerbID != -9 )
        {

            throw embed::UnreachableStateException();
        }

        // the workaround verb to show the object in case no server is available

        // if it is possible, the object will be converted to OOo format
        if ( !m_bTriedConversion )
        {
            m_bTriedConversion = true;
            if ( TryToConvertToOOo( m_xObjectStream ) )
            {
                aGuard.clear();
                changeState( embed::EmbedStates::ACTIVE );
                return;
            }
        }

        if ( !m_xOwnView.is() && m_xObjectStream.is() && m_aFilterName != "Text" )
        {
            try {
                uno::Reference< io::XSeekable > xSeekable( m_xObjectStream, uno::UNO_QUERY );
                if ( xSeekable.is() )
                    xSeekable->seek( 0 );

                m_xOwnView = new OwnView_Impl( m_xContext, m_xObjectStream->getInputStream() );
            }
            catch( uno::RuntimeException& )
            {
                throw;
            }
            catch (uno::Exception const&)
            {
                TOOLS_WARN_EXCEPTION("embeddedobj.ole", "OleEmbeddedObject::doVerb: -9 fallback path:");
            }
        }

        // it may be the OLE Storage, try to extract stream
        if ( !m_xOwnView.is() && m_xObjectStream.is() && m_aFilterName == "Text" )
        {
            uno::Reference< io::XStream > xStream = lcl_ExtractObjectStream( m_xContext, m_xObjectStream );

            if ( TryToConvertToOOo( xStream ) )
            {
                aGuard.clear();
                changeState( embed::EmbedStates::ACTIVE );
                return;
            }
        }

        if (!m_xOwnView.is() || !m_xOwnView->Open())
        {
            //Make a RO copy and see if the OS can find something to at
            //least display the content for us
            if (m_aTempDumpURL.isEmpty())
                m_aTempDumpURL = lcl_ExtractObject(m_xContext, m_xObjectStream);

            if (m_aTempDumpURL.isEmpty())
                throw embed::UnreachableStateException();

            uno::Reference< css::system::XSystemShellExecute > xSystemShellExecute(
                css::system::SystemShellExecute::create( m_xContext ) );
            xSystemShellExecute->execute(m_aTempDumpURL, OUString(), css::system::SystemShellExecuteFlags::URIS_ONLY);

        }

    }
}


uno::Sequence< embed::VerbDescriptor > SAL_CALL OleEmbeddedObject::getSupportedVerbs()
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->getSupportedVerbs();
    }
    // end wrapping related part ====================

    osl::ClearableMutexGuard aGuard(m_aMutex);
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_nObjectState == -1 )
        throw embed::WrongStateException( u"The object has no persistence!"_ustr,
                                        static_cast< ::cppu::OWeakObject* >(this) );
#ifdef _WIN32
    if ( m_pOleComponent )
    {
        // registry could be used in this case
        // if ( m_nObjectState == embed::EmbedStates::LOADED )
        // {
        //  // the list of supported verbs can be retrieved only when object is in running state
        //  throw embed::NeedsRunningStateException(); // TODO:
        // }

        aGuard.clear();
        return m_pOleComponent->GetVerbList();
    }
    else
#endif
    {
        // tdf#140079 Claim support for the OleEmbeddedObject::doVerb -9 fallback.
        // So in SfxViewFrame::GetState_Impl in case SID_OBJECT hasVerbs is not
        // empty, so that the doVerb attempt with -9 fallback is attempted
        uno::Sequence<embed::VerbDescriptor> aRet(1);
        aRet.getArray()[0].VerbID = -9;
        return aRet;
    }
}


void SAL_CALL OleEmbeddedObject::setClientSite(
                const uno::Reference< embed::XEmbeddedClient >& xClient )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->setClientSite( xClient );
        return;
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_xClientSite != xClient)
    {
        if ( m_nObjectState != embed::EmbedStates::LOADED && m_nObjectState != embed::EmbedStates::RUNNING )
            throw embed::WrongStateException(
                                    u"The client site can not be set currently!"_ustr,
                                    static_cast< ::cppu::OWeakObject* >(this) );

        m_xClientSite = xClient;
    }
}


uno::Reference< embed::XEmbeddedClient > SAL_CALL OleEmbeddedObject::getClientSite()
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->getClientSite();
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_nObjectState == -1 )
        throw embed::WrongStateException( u"The object has no persistence!"_ustr,
                                        static_cast< ::cppu::OWeakObject* >(this) );

    return m_xClientSite;
}


void SAL_CALL OleEmbeddedObject::update()
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->update();
        return;
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_nObjectState == -1 )
        throw embed::WrongStateException( u"The object has no persistence!"_ustr,
                                        static_cast< ::cppu::OWeakObject* >(this) );

    if ( m_nUpdateMode == embed::EmbedUpdateModes::EXPLICIT_UPDATE )
    {
        // TODO: update view representation
    }
    else
    {
        // the object must be up to date
        SAL_WARN_IF( m_nUpdateMode != embed::EmbedUpdateModes::ALWAYS_UPDATE, "embeddedobj.ole", "Unknown update mode!" );
    }
}


void SAL_CALL OleEmbeddedObject::setUpdateMode( sal_Int32 nMode )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->setUpdateMode( nMode );
        return;
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_nObjectState == -1 )
        throw embed::WrongStateException( u"The object has no persistence!"_ustr,
                                       static_cast< ::cppu::OWeakObject* >(this) );

    OSL_ENSURE( nMode == embed::EmbedUpdateModes::ALWAYS_UPDATE
                    || nMode == embed::EmbedUpdateModes::EXPLICIT_UPDATE,
                "Unknown update mode!" );
    m_nUpdateMode = nMode;
}


sal_Int64 SAL_CALL OleEmbeddedObject::getStatus( sal_Int64
    nAspect
)
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->getStatus( nAspect );
    }
    // end wrapping related part ====================

    osl::ResettableMutexGuard aGuard(m_aMutex);
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_nObjectState == -1 )
        throw embed::WrongStateException( u"The object must be in running state!"_ustr,
                                    static_cast< ::cppu::OWeakObject* >(this) );

    sal_Int64 nResult = 0;

#ifdef _WIN32
    if ( m_bGotStatus && m_nStatusAspect == nAspect )
        nResult = m_nStatus;
    else if ( m_pOleComponent )
    {
        m_nStatus = ExecUnlocked([p = m_pOleComponent, nAspect] { return p->GetMiscStatus(nAspect); }, aGuard);
        m_nStatusAspect = nAspect;
        m_bGotStatus = true;
        nResult = m_nStatus;
    }
#endif

    // this implementation needs size to be provided after object loading/creating to work in optimal way
    return ( nResult | embed::EmbedMisc::EMBED_NEEDSSIZEONLOAD );
}


void SAL_CALL OleEmbeddedObject::setContainerName( const OUString& sName )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->setContainerName( sName );
        return;
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    m_aContainerName = sName;
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
