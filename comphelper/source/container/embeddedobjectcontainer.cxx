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

#include <com/sun/star/container/XChild.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/embed/EmbeddedObjectCreator.hpp>
#include <com/sun/star/embed/WrongStateException.hpp>
#include <com/sun/star/embed/XEmbeddedObject.hpp>
#include <com/sun/star/embed/XEmbedPersist.hpp>
#include <com/sun/star/embed/XLinkageSupport.hpp>
#include <com/sun/star/embed/XTransactedObject.hpp>
#include <com/sun/star/embed/XOptimizedStorage.hpp>
#include <com/sun/star/embed/EntryInitModes.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/util/XCloseable.hpp>
#include <com/sun/star/util/XModifiable.hpp>
#include <com/sun/star/embed/EmbedStates.hpp>
#include <com/sun/star/beans/XPropertySetInfo.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/embed/Aspects.hpp>
#include <com/sun/star/embed/EmbedMisc.hpp>

#include <comphelper/classids.hxx>
#include <comphelper/mimeconfighelper.hxx>
#include <comphelper/seqstream.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/storagehelper.hxx>
#include <comphelper/embeddedobjectcontainer.hxx>
#include <comphelper/sequence.hxx>
#include <comphelper/propertysequence.hxx>
#include <comphelper/propertyvalue.hxx>
#include <cppuhelper/weakref.hxx>
#include <sal/log.hxx>
#include <rtl/ref.hxx>

#include <officecfg/Office/Common.hxx>

#include <algorithm>
#include <unordered_map>


using namespace ::com::sun::star;

namespace comphelper {

typedef std::unordered_map<OUString, uno::Reference<embed::XEmbeddedObject>> EmbeddedObjectContainerNameMap;
struct EmbedImpl
{
    // TODO/LATER: remove objects from temp. Container storage when object is disposed
    EmbeddedObjectContainerNameMap maNameToObjectMap;
    // to speed up lookup by Reference
    std::unordered_map<uno::Reference<embed::XEmbeddedObject>, OUString> maObjectToNameMap;
    uno::Reference < embed::XStorage > mxStorage;
    EmbeddedObjectContainer* mpTempObjectContainer;
    uno::Reference < embed::XStorage > mxImageStorage;
    uno::WeakReference < uno::XInterface > m_xModel;

    bool mbOwnsStorage : 1;
    bool mbUserAllowsLinkUpdate : 1;

    const uno::Reference < embed::XStorage >& GetReplacements();
};

const uno::Reference < embed::XStorage >& EmbedImpl::GetReplacements()
{
    if ( !mxImageStorage.is() )
    {
        try
        {
            mxImageStorage = mxStorage->openStorageElement(
                u"ObjectReplacements"_ustr, embed::ElementModes::READWRITE );
        }
        catch (const uno::Exception&)
        {
            mxImageStorage = mxStorage->openStorageElement(
                u"ObjectReplacements"_ustr, embed::ElementModes::READ );
        }
    }

    if ( !mxImageStorage.is() )
        throw io::IOException(u"No ObjectReplacements"_ustr);

    return mxImageStorage;
}

EmbeddedObjectContainer::EmbeddedObjectContainer()
    :     pImpl(new EmbedImpl)
{
    pImpl->mxStorage = ::comphelper::OStorageHelper::GetTemporaryStorage();
    pImpl->mbOwnsStorage = true;
    pImpl->mbUserAllowsLinkUpdate = true;
    pImpl->mpTempObjectContainer = nullptr;
}

EmbeddedObjectContainer::EmbeddedObjectContainer( const uno::Reference < embed::XStorage >& rStor )
    :     pImpl(new EmbedImpl)
{
    pImpl->mxStorage = rStor;
    pImpl->mbOwnsStorage = false;
    pImpl->mbUserAllowsLinkUpdate = true;
    pImpl->mpTempObjectContainer = nullptr;
}

EmbeddedObjectContainer::EmbeddedObjectContainer( const uno::Reference < embed::XStorage >& rStor, const uno::Reference < uno::XInterface >& xModel )
    :     pImpl(new EmbedImpl)
{
    pImpl->mxStorage = rStor;
    pImpl->mbOwnsStorage = false;
    pImpl->mbUserAllowsLinkUpdate = true;
    pImpl->mpTempObjectContainer = nullptr;
    pImpl->m_xModel = xModel;
}

void EmbeddedObjectContainer::SwitchPersistence( const uno::Reference < embed::XStorage >& rStor )
{
    ReleaseImageSubStorage();

    if ( pImpl->mbOwnsStorage )
        pImpl->mxStorage->dispose();

    pImpl->mxStorage = rStor;
    pImpl->mbOwnsStorage = false;
}

bool EmbeddedObjectContainer::CommitImageSubStorage()
{
    if ( !pImpl->mxImageStorage )
        return true;

    try
    {
        bool bReadOnlyMode = true;
        uno::Reference < beans::XPropertySet > xSet(pImpl->mxImageStorage,uno::UNO_QUERY);
        if ( xSet.is() )
        {
            // get the open mode from the parent storage
            sal_Int32 nMode = 0;
            uno::Any aAny = xSet->getPropertyValue(u"OpenMode"_ustr);
            if ( aAny >>= nMode )
                bReadOnlyMode = !(nMode & embed::ElementModes::WRITE );
        } // if ( xSet.is() )
        if ( !bReadOnlyMode )
        {
            uno::Reference< embed::XTransactedObject > xTransact( pImpl->mxImageStorage, uno::UNO_QUERY );
            if (xTransact)
                xTransact->commit();
        }
    }
    catch (const uno::Exception&)
    {
        return false;
    }

    return true;
}

void EmbeddedObjectContainer::ReleaseImageSubStorage()
{
    CommitImageSubStorage();

    if ( pImpl->mxImageStorage.is() )
    {
        try
        {
            pImpl->mxImageStorage->dispose();
            pImpl->mxImageStorage.clear();
        }
        catch (const uno::Exception&)
        {
            SAL_WARN( "comphelper.container", "Problems releasing image substorage!" );
        }
    }
}

EmbeddedObjectContainer::~EmbeddedObjectContainer()
{
    ReleaseImageSubStorage();

    if ( pImpl->mbOwnsStorage )
        pImpl->mxStorage->dispose();

    delete pImpl->mpTempObjectContainer;
}

void EmbeddedObjectContainer::CloseEmbeddedObjects()
{
    for( const auto& rObj : pImpl->maNameToObjectMap )
    {
        uno::Reference < embed::XEmbeddedObject > const & xClose = rObj.second;
        if( xClose.is() )
        {
            try
            {
                xClose->close( true );
            }
            catch (const uno::Exception&)
            {
            }
        }
    }
}

OUString EmbeddedObjectContainer::CreateUniqueObjectName()
{
    OUString aStr;
    sal_Int32 i=1;
    do
    {
        aStr = "Object " + OUString::number( i++ );
    }
    while( HasEmbeddedObject( aStr ) );
    // TODO/LATER: should we consider deleted objects?

    return aStr;
}

uno::Sequence < OUString > EmbeddedObjectContainer::GetObjectNames() const
{
    return comphelper::mapKeysToSequence(pImpl->maNameToObjectMap);
}

bool EmbeddedObjectContainer::HasEmbeddedObjects() const
{
    return !pImpl->maNameToObjectMap.empty();
}

bool EmbeddedObjectContainer::HasEmbeddedObject( const OUString& rName )
{
    if (pImpl->maNameToObjectMap.contains(rName))
        return true;
    if (!pImpl->mxStorage.is())
        return false;
    return pImpl->mxStorage->hasByName(rName);
}

bool EmbeddedObjectContainer::HasEmbeddedObject( const uno::Reference < embed::XEmbeddedObject >& xObj ) const
{
    return pImpl->maObjectToNameMap.contains(xObj);
}

bool EmbeddedObjectContainer::HasInstantiatedEmbeddedObject( const OUString& rName )
{
    // allows to detect whether the object was already instantiated
    // currently the filter instantiate it on loading, so this method allows
    // to avoid objects pointing to the same persistence
    return pImpl->maNameToObjectMap.contains(rName);
}

OUString EmbeddedObjectContainer::GetEmbeddedObjectName( const css::uno::Reference < css::embed::XEmbeddedObject >& xObj ) const
{
    auto it = pImpl->maObjectToNameMap.find(xObj);
    if (it == pImpl->maObjectToNameMap.end())
    {
        SAL_WARN( "comphelper.container", "Unknown object!" );
        return OUString();
    }
    return it->second;
}

uno::Reference< embed::XEmbeddedObject>
EmbeddedObjectContainer::GetEmbeddedObject(
        const OUString& rName, OUString const*const pBaseURL)
{
    SAL_WARN_IF( rName.isEmpty(), "comphelper.container", "Empty object name!");

    uno::Reference < embed::XEmbeddedObject > xObj;
    auto aIt = pImpl->maNameToObjectMap.find( rName );

#if OSL_DEBUG_LEVEL > 1
    uno::Reference < container::XNameAccess > xAccess( pImpl->mxStorage, uno::UNO_QUERY );
    uno::Sequence< OUString> aSeq = xAccess->getElementNames();
    const OUString* pIter = aSeq.getConstArray();
    const OUString* pEnd   = pIter + aSeq.getLength();
    for(;pIter != pEnd;++pIter)
    {
        (void)*pIter;
    }
    OSL_ENSURE( aIt != pImpl->maNameToObjectMap.end() || xAccess->hasByName(rName), "Could not return object!" );
#endif

    // check if object was already created
    if ( aIt != pImpl->maNameToObjectMap.end() )
        xObj = (*aIt).second;
    else
        xObj = Get_Impl(rName, uno::Reference<embed::XEmbeddedObject>(), pBaseURL);

    return xObj;
}

uno::Reference<embed::XEmbeddedObject> EmbeddedObjectContainer::Get_Impl(
        const OUString& rName,
        const uno::Reference<embed::XEmbeddedObject>& xCopy,
        OUString const*const pBaseURL)
{
    uno::Reference < embed::XEmbeddedObject > xObj;
    try
    {
        // create the object from the storage
        uno::Reference < beans::XPropertySet > xSet( pImpl->mxStorage, uno::UNO_QUERY );
        bool bReadOnlyMode = true;
        if ( xSet.is() )
        {
            // get the open mode from the parent storage
            sal_Int32 nMode = 0;
            uno::Any aAny = xSet->getPropertyValue(u"OpenMode"_ustr);
            if ( aAny >>= nMode )
                bReadOnlyMode = !(nMode & embed::ElementModes::WRITE );
        }

        // object was not added until now - should happen only by calling this method from "inside"
        //TODO/LATER: it would be good to detect an error when an object should be created already, but isn't (not an "inside" call)
        uno::Reference < embed::XEmbeddedObjectCreator > xFactory = embed::EmbeddedObjectCreator::create( ::comphelper::getProcessComponentContext() );
        uno::Sequence< beans::PropertyValue > aObjDescr(1 + (xCopy.is() ? 1 : 0) + (pBaseURL ? 1 : 0));
        auto itObjDescr = aObjDescr.getArray();
        itObjDescr->Name = "Parent";
        itObjDescr->Value <<= pImpl->m_xModel.get();
        if (pBaseURL)
        {
            ++itObjDescr;
            itObjDescr->Name = "DefaultParentBaseURL";
            itObjDescr->Value <<= *pBaseURL;
        }
        if ( xCopy.is() )
        {
            ++itObjDescr;
            itObjDescr->Name = "CloneFrom";
            itObjDescr->Value <<= xCopy;
        }

        uno::Sequence< beans::PropertyValue > aMediaDescr{ comphelper::makePropertyValue(
            u"ReadOnly"_ustr, bReadOnlyMode) };
        xObj.set( xFactory->createInstanceInitFromEntry(
                pImpl->mxStorage, rName,
                aMediaDescr, aObjDescr ), uno::UNO_QUERY );

        // insert object into my list
        AddEmbeddedObject( xObj, rName );
    }
    catch (uno::Exception const& e)
    {
        SAL_WARN("comphelper.container", "EmbeddedObjectContainer::Get_Impl: exception caught: " << e);
    }

    return xObj;
}

uno::Reference < embed::XEmbeddedObject > EmbeddedObjectContainer::CreateEmbeddedObject( const uno::Sequence < sal_Int8 >& rClassId,
            const uno::Sequence < beans::PropertyValue >& rArgs, OUString& rNewName, OUString const* pBaseURL )
{
    if ( rNewName.isEmpty() )
        rNewName = CreateUniqueObjectName();

    SAL_WARN_IF( HasEmbeddedObject(rNewName), "comphelper.container", "Object to create already exists!");

    // create object from classid by inserting it into storage
    uno::Reference < embed::XEmbeddedObject > xObj;
    try
    {
        uno::Reference < embed::XEmbeddedObjectCreator > xFactory = embed::EmbeddedObjectCreator::create( ::comphelper::getProcessComponentContext() );

        const size_t nExtraArgs = pBaseURL ? 2 : 1;
        uno::Sequence< beans::PropertyValue > aObjDescr( rArgs.getLength() + nExtraArgs );
        auto pObjDescr = aObjDescr.getArray();
        pObjDescr[0].Name = "Parent";
        pObjDescr[0].Value <<= pImpl->m_xModel.get();
        if (pBaseURL)
        {
            pObjDescr[1].Name = "DefaultParentBaseURL";
            pObjDescr[1].Value <<= *pBaseURL;
        }
        std::copy( rArgs.begin(), rArgs.end(), pObjDescr + nExtraArgs );
        xObj.set( xFactory->createInstanceInitNew(
                    rClassId, OUString(), pImpl->mxStorage, rNewName,
                    aObjDescr ), uno::UNO_QUERY );

        AddEmbeddedObject( xObj, rNewName );

        OSL_ENSURE( !xObj.is() || xObj->getCurrentState() != embed::EmbedStates::LOADED,
                    "A freshly create object should be running always!" );
    }
    catch (uno::Exception const& e)
    {
        SAL_WARN("comphelper.container", "EmbeddedObjectContainer::CreateEmbeddedObject: exception caught: " << e);
    }

    return xObj;
}

uno::Reference < embed::XEmbeddedObject > EmbeddedObjectContainer::CreateEmbeddedObject( const uno::Sequence < sal_Int8 >& rClassId, OUString& rNewName, OUString const* pBaseURL )
{
    return CreateEmbeddedObject( rClassId, uno::Sequence < beans::PropertyValue >(), rNewName, pBaseURL );
}

void EmbeddedObjectContainer::AddEmbeddedObject( const css::uno::Reference < css::embed::XEmbeddedObject >& xObj, const OUString& rName )
{
#if OSL_DEBUG_LEVEL > 1
    SAL_WARN_IF( rName.isEmpty(), "comphelper.container", "Added object doesn't have a name!");
    uno::Reference < container::XNameAccess > xAccess( pImpl->mxStorage, uno::UNO_QUERY );
    uno::Reference < embed::XEmbedPersist > xEmb( xObj, uno::UNO_QUERY );
    uno::Reference < embed::XLinkageSupport > xLink( xEmb, uno::UNO_QUERY );
    // if the object has a persistence and the object is not a link than it must have persistence entry in the storage
    OSL_ENSURE( !( xEmb.is() && ( !xLink.is() || !xLink->isLink() ) ) || xAccess->hasByName(rName),
                    "Added element not in storage!" );
#endif

    // remember object - it needs to be in storage already
    auto aIt = pImpl->maNameToObjectMap.find( rName );
    OSL_ENSURE( aIt == pImpl->maNameToObjectMap.end(), "Element already inserted!" );
    pImpl->maNameToObjectMap[ rName ] = xObj;
    pImpl->maObjectToNameMap[ xObj ] = rName;
    uno::Reference < container::XChild > xChild( xObj, uno::UNO_QUERY );
    if ( xChild.is() && xChild->getParent() != pImpl->m_xModel.get() )
        xChild->setParent( pImpl->m_xModel.get() );

    // look for object in temporary container
    if ( !pImpl->mpTempObjectContainer )
        return;

    auto& rObjectContainer = pImpl->mpTempObjectContainer->pImpl->maNameToObjectMap;
    auto aIter = std::find_if(rObjectContainer.begin(), rObjectContainer.end(),
        [&xObj](const EmbeddedObjectContainerNameMap::value_type& rEntry) { return rEntry.second == xObj; });
    if (aIter == rObjectContainer.end())
        return;

    // copy replacement image from temporary container (if there is any)
    OUString aTempName = aIter->first;
    OUString aMediaType;
    uno::Reference < io::XInputStream > xStream = pImpl->mpTempObjectContainer->GetGraphicStream( xObj, &aMediaType );
    if ( xStream.is() )
    {
        InsertGraphicStream( xStream, rName, aMediaType );
        xStream = nullptr;
        pImpl->mpTempObjectContainer->RemoveGraphicStream( aTempName );
    }

    // remove object from storage of temporary container
    uno::Reference < embed::XEmbedPersist > xPersist( xObj, uno::UNO_QUERY );
    if ( xPersist.is() )
    {
        try
        {
            pImpl->mpTempObjectContainer->pImpl->mxStorage->removeElement( aTempName );
        }
        catch (const uno::Exception&)
        {
        }
    }

    // temp. container needs to forget the object
    pImpl->mpTempObjectContainer->pImpl->maObjectToNameMap.erase( aIter->second );
    pImpl->mpTempObjectContainer->pImpl->maNameToObjectMap.erase( aIter );
}

bool EmbeddedObjectContainer::StoreEmbeddedObject(
    const uno::Reference < embed::XEmbeddedObject >& xObj, OUString& rName, bool bCopy,
    const OUString& rSrcShellID, const OUString& rDestShellID )
{
    uno::Reference < embed::XEmbedPersist > xPersist( xObj, uno::UNO_QUERY );
    if ( rName.isEmpty() )
        rName = CreateUniqueObjectName();

#if OSL_DEBUG_LEVEL > 1
    uno::Reference < container::XNameAccess > xAccess( pImpl->mxStorage, uno::UNO_QUERY );
    OSL_ENSURE( !xPersist.is() || !xAccess->hasByName(rName), "Inserting element already present in storage!" );
    OSL_ENSURE( xPersist.is() || xObj->getCurrentState() == embed::EmbedStates::RUNNING, "Non persistent object inserted!");
#endif

    // insert objects' storage into the container storage (if object has one)
    try
    {
        if ( xPersist.is() )
        {
            uno::Sequence < beans::PropertyValue > aSeq;
            auto aObjArgs(::comphelper::InitPropertySequence({
                { "SourceShellID", uno::Any(rSrcShellID) },
                { "DestinationShellID", uno::Any(rDestShellID) }
            }));
            if ( bCopy )
            {
                xPersist->storeToEntry(pImpl->mxStorage, rName, aSeq, aObjArgs);
            }
            else
            {
                //TODO/LATER: possible optimization, don't store immediately
                //xPersist->setPersistentEntry( pImpl->mxStorage, rName, embed::EntryInitModes::ENTRY_NO_INIT, aSeq, aSeq );
                xPersist->storeAsEntry( pImpl->mxStorage, rName, aSeq, aObjArgs );
                xPersist->saveCompleted( true );
            }
        }
    }
    catch (uno::Exception const& e)
    {
        SAL_WARN("comphelper.container", "EmbeddedObjectContainer::StoreEmbeddedObject: exception caught: " << e);
        // TODO/LATER: better error recovery should keep storage intact
        return false;
    }

    return true;
}
bool EmbeddedObjectContainer::InsertEmbeddedObject( const uno::Reference < embed::XEmbeddedObject >& xObj, OUString& rName,
        OUString const* pTargetShellID )
{
    // store it into the container storage
    OUString sTargetShellID;
    if (pTargetShellID)
        sTargetShellID = *pTargetShellID;
    if (StoreEmbeddedObject(xObj, rName, false, OUString(), sTargetShellID))
    {
        // remember object
        AddEmbeddedObject( xObj, rName );
        return true;
    }
    else
        return false;
}

uno::Reference < embed::XEmbeddedObject > EmbeddedObjectContainer::InsertEmbeddedObject( const uno::Reference < io::XInputStream >& xStm, OUString& rNewName )
{
    if ( rNewName.isEmpty() )
        rNewName = CreateUniqueObjectName();

    // store it into the container storage
    bool bIsStorage = false;
    try
    {
        // first try storage persistence
        uno::Reference < embed::XStorage > xStore = ::comphelper::OStorageHelper::GetStorageFromInputStream( xStm );

        // storage was created from stream successfully
        bIsStorage = true;

        uno::Reference < embed::XStorage > xNewStore = pImpl->mxStorage->openStorageElement( rNewName, embed::ElementModes::READWRITE );
        xStore->copyToStorage( xNewStore );
    }
    catch (const uno::Exception&)
    {
        if ( bIsStorage )
            // it is storage persistence, but opening of new substorage or copying to it failed
            return uno::Reference < embed::XEmbeddedObject >();

        // stream didn't contain a storage, now try stream persistence
        try
        {
            uno::Reference < io::XStream > xNewStream = pImpl->mxStorage->openStreamElement( rNewName, embed::ElementModes::READWRITE );
            ::comphelper::OStorageHelper::CopyInputToOutput( xStm, xNewStream->getOutputStream() );

            // No mediatype is provided so the default for OLE objects value is used
            // it is correct so for now, but what if somebody introduces a new stream based embedded object?
            // Probably introducing of such an object must be restricted ( a storage must be used! ).
            uno::Reference< beans::XPropertySet > xProps( xNewStream, uno::UNO_QUERY_THROW );
            xProps->setPropertyValue(u"MediaType"_ustr,
                    uno::Any( u"application/vnd.sun.star.oleobject"_ustr ) );
        }
        catch (uno::Exception const& e)
        {
            // complete disaster!
            SAL_WARN("comphelper.container", "EmbeddedObjectContainer::InsertEmbeddedObject: exception caught: " << e);
            return uno::Reference < embed::XEmbeddedObject >();
        }
    }

    // stream was copied into the container storage in either way, now try to open something form it
    uno::Reference < embed::XEmbeddedObject > xRet = GetEmbeddedObject( rNewName );
    try
    {
        if ( !xRet.is() )
            // no object could be created, so withdraw insertion
            pImpl->mxStorage->removeElement( rNewName );
    }
    catch (const uno::Exception&)
    {
    }

    return xRet;
}

uno::Reference < embed::XEmbeddedObject > EmbeddedObjectContainer::InsertEmbeddedObject( const css::uno::Sequence < css::beans::PropertyValue >& aMedium, OUString& rNewName, OUString const* pBaseURL )
{
    if ( rNewName.isEmpty() )
        rNewName = CreateUniqueObjectName();

    uno::Reference < embed::XEmbeddedObject > xObj;
    try
    {
        uno::Reference < embed::XEmbeddedObjectCreator > xFactory = embed::EmbeddedObjectCreator::create( ::comphelper::getProcessComponentContext() );
        uno::Sequence< beans::PropertyValue > aObjDescr(pBaseURL ? 2 : 1);
        auto pObjDescr = aObjDescr.getArray();
        pObjDescr[0].Name = "Parent";
        pObjDescr[0].Value <<= pImpl->m_xModel.get();
        if (pBaseURL)
        {
            pObjDescr[1].Name = "DefaultParentBaseURL";
            pObjDescr[1].Value <<= *pBaseURL;
        }
        xObj.set( xFactory->createInstanceInitFromMediaDescriptor(
                pImpl->mxStorage, rNewName, aMedium, aObjDescr ), uno::UNO_QUERY );
        uno::Reference < embed::XEmbedPersist > xPersist( xObj, uno::UNO_QUERY );

        OSL_ENSURE( !xObj.is() || xObj->getCurrentState() != embed::EmbedStates::LOADED,
                    "A freshly create object should be running always!" );

        // possible optimization: store later!
        if ( xPersist.is())
            xPersist->storeOwn();

        AddEmbeddedObject( xObj, rNewName );
    }
    catch (const uno::Exception&)
    {
    }

    return xObj;
}

uno::Reference < embed::XEmbeddedObject > EmbeddedObjectContainer::InsertEmbeddedLink( const css::uno::Sequence < css::beans::PropertyValue >& aMedium, OUString& rNewName )
{
    if ( rNewName.isEmpty() )
        rNewName = CreateUniqueObjectName();

    uno::Reference < embed::XEmbeddedObject > xObj;
    try
    {
        uno::Reference < embed::XEmbeddedObjectCreator > xFactory = embed::EmbeddedObjectCreator::create(::comphelper::getProcessComponentContext());
        uno::Sequence< beans::PropertyValue > aObjDescr{ comphelper::makePropertyValue(
            u"Parent"_ustr, pImpl->m_xModel.get()) };
        xObj.set( xFactory->createInstanceLink( pImpl->mxStorage, rNewName, aMedium, aObjDescr ), uno::UNO_QUERY );

        uno::Reference < embed::XEmbedPersist > xPersist( xObj, uno::UNO_QUERY );

        OSL_ENSURE( !xObj.is() || xObj->getCurrentState() != embed::EmbedStates::LOADED,
                    "A freshly create object should be running always!" );

        // possible optimization: store later!
        if ( xPersist.is())
            xPersist->storeOwn();

        AddEmbeddedObject( xObj, rNewName );
    }
    catch (uno::Exception const& e)
    {
        SAL_WARN("comphelper.container", "EmbeddedObjectContainer::InsertEmbeddedLink: "
                "exception caught: " << e);
    }

    return xObj;
}

bool EmbeddedObjectContainer::TryToCopyGraphReplacement( EmbeddedObjectContainer& rSrc,
                                                            const OUString& aOrigName,
                                                            const OUString& aTargetName )
{
    bool bResult = false;

    if ( ( &rSrc != this || aOrigName != aTargetName ) && !aOrigName.isEmpty() && !aTargetName.isEmpty() )
    {
        OUString aMediaType;
        uno::Reference < io::XInputStream > xGrStream = rSrc.GetGraphicStream( aOrigName, &aMediaType );
        if ( xGrStream.is() )
            bResult = InsertGraphicStream( xGrStream, aTargetName, aMediaType );
    }

    return bResult;
}

uno::Reference < embed::XEmbeddedObject > EmbeddedObjectContainer::CopyAndGetEmbeddedObject(
    EmbeddedObjectContainer& rSrc, const uno::Reference <embed::XEmbeddedObject>& xObj, OUString& rName,
    const OUString& rSrcShellID, const OUString& rDestShellID )
{
    uno::Reference< embed::XEmbeddedObject > xResult;

    // TODO/LATER: For now only objects that implement XEmbedPersist have a replacement image, it might change in future
    // do an incompatible change so that object name is provided in all the move and copy methods
    OUString aOrigName;
    try
    {
        uno::Reference < embed::XEmbedPersist > xPersist( xObj, uno::UNO_QUERY );
        if (xPersist)
            aOrigName = xPersist->getEntryName();
    }
    catch (const uno::Exception&)
    {
    }

    if ( rName.isEmpty() )
        rName = CreateUniqueObjectName();

    // objects without persistence are not really stored by the method
    if (xObj.is() && StoreEmbeddedObject(xObj, rName, true, rSrcShellID, rDestShellID))
    {
        SAL_INFO_IF(rDestShellID.isEmpty(), "comphelper.container",
            "SfxObjectShell with no base URL?"); // every shell has a base URL, except the clipboard SwDocShell
        xResult = Get_Impl(rName, xObj, &rDestShellID);
        if ( !xResult.is() )
        {
            // this is a case when object has no real persistence
            // in such cases a new object should be explicitly created and initialized with the data of the old one
            try
            {
                uno::Reference< embed::XLinkageSupport > xOrigLinkage( xObj, uno::UNO_QUERY );
                if ( xOrigLinkage.is() && xOrigLinkage->isLink() )
                {
                    // this is an OOo link, it has no persistence
                    OUString aURL = xOrigLinkage->getLinkURL();
                    if ( aURL.isEmpty() )
                        throw uno::RuntimeException("URL of the linked object is empty");

                    // create new linked object from the URL the link is based on
                    uno::Reference < embed::XEmbeddedObjectCreator > xCreator =
                        embed::EmbeddedObjectCreator::create( ::comphelper::getProcessComponentContext() );

                    uno::Sequence< beans::PropertyValue > aMediaDescr{ comphelper::makePropertyValue(
                        u"URL"_ustr, aURL) };
                    uno::Sequence< beans::PropertyValue > aObjDescr{ comphelper::makePropertyValue(
                        u"Parent"_ustr, pImpl->m_xModel.get()) };
                    xResult.set(xCreator->createInstanceLink(
                                    pImpl->mxStorage,
                                    rName,
                                    aMediaDescr,
                                    aObjDescr ),
                                uno::UNO_QUERY_THROW );
                }
                else
                {
                    // the component is required for copying of this object
                    if ( xObj->getCurrentState() == embed::EmbedStates::LOADED )
                        xObj->changeState( embed::EmbedStates::RUNNING );

                    // this must be an object based on properties, otherwise we can not copy it currently
                    uno::Reference< beans::XPropertySet > xOrigProps( xObj->getComponent(), uno::UNO_QUERY_THROW );

                    // use object class ID to create a new one and transfer all the properties
                    uno::Reference < embed::XEmbeddedObjectCreator > xCreator =
                        embed::EmbeddedObjectCreator::create( ::comphelper::getProcessComponentContext() );

                    uno::Sequence< beans::PropertyValue > aObjDescr{ comphelper::makePropertyValue(
                        u"Parent"_ustr, pImpl->m_xModel.get()) };
                    xResult.set(xCreator->createInstanceInitNew(
                                    xObj->getClassID(),
                                    xObj->getClassName(),
                                    pImpl->mxStorage,
                                    rName,
                                    aObjDescr ),
                                uno::UNO_QUERY_THROW );

                    if ( xResult->getCurrentState() == embed::EmbedStates::LOADED )
                        xResult->changeState( embed::EmbedStates::RUNNING );

                    uno::Reference< beans::XPropertySet > xTargetProps( xResult->getComponent(), uno::UNO_QUERY_THROW );

                    // copy all the properties from xOrigProps to xTargetProps
                    uno::Reference< beans::XPropertySetInfo > xOrigInfo = xOrigProps->getPropertySetInfo();
                    if ( !xOrigInfo.is() )
                        throw uno::RuntimeException("Object has no properties");

                    const uno::Sequence< beans::Property > aPropertiesList = xOrigInfo->getProperties();
                    for ( const auto & p : aPropertiesList )
                    {
                        try
                        {
                            xTargetProps->setPropertyValue(
                                p.Name,
                                xOrigProps->getPropertyValue( p.Name ) );
                        }
                        catch (const beans::PropertyVetoException&)
                        {
                            // impossibility to copy readonly property is not treated as an error for now
                            // but the assertion is helpful to detect such scenarios and review them
                            SAL_WARN( "comphelper.container", "Could not copy readonly property!" );
                        }
                    }
                }

                if ( xResult.is() )
                    AddEmbeddedObject( xResult, rName );
            }
            catch (const uno::Exception&)
            {
                if ( xResult.is() )
                {
                    try
                    {
                        xResult->close( true );
                    }
                    catch (const uno::Exception&)
                    {
                    }
                    xResult.clear();
                }
            }
        }
    }

    SAL_WARN_IF( !xResult.is(), "comphelper.container", "Can not copy embedded object that has no persistence!" );

    if ( xResult.is() )
    {
        // the object is successfully copied, try to copy graphical replacement
        if ( !aOrigName.isEmpty() )
            TryToCopyGraphReplacement( rSrc, aOrigName, rName );

        // the object might need the size to be set
        try
        {
            if ( xResult->getStatus( embed::Aspects::MSOLE_CONTENT ) & embed::EmbedMisc::EMBED_NEEDSSIZEONLOAD )
                xResult->setVisualAreaSize( embed::Aspects::MSOLE_CONTENT,
                                            xObj->getVisualAreaSize( embed::Aspects::MSOLE_CONTENT ) );
        }
        catch (const uno::Exception&)
        {
        }
    }

    return xResult;
}

// #i119941, bKeepToTempStorage: use to specify whether store the removed object to temporary storage+
void EmbeddedObjectContainer::RemoveEmbeddedObject( const OUString& rName, bool bKeepToTempStorage )
{
    uno::Reference < embed::XEmbeddedObject > xObj = GetEmbeddedObject( rName );
    if ( xObj.is() )
        RemoveEmbeddedObject( xObj, bKeepToTempStorage );
}

bool EmbeddedObjectContainer::MoveEmbeddedObject( const OUString& rName, EmbeddedObjectContainer& rCnt )
{
    // find object entry
    auto aIt2 = rCnt.pImpl->maNameToObjectMap.find( rName );
    OSL_ENSURE( aIt2 == rCnt.pImpl->maNameToObjectMap.end(), "Object does already exist in target container!" );

    if ( aIt2 != rCnt.pImpl->maNameToObjectMap.end() )
        return false;

    uno::Reference < embed::XEmbeddedObject > xObj;
    auto aIt = pImpl->maNameToObjectMap.find( rName );
    if ( aIt != pImpl->maNameToObjectMap.end() )
    {
        xObj = (*aIt).second;
        try
        {
            if ( xObj.is() )
            {
                // move object
                OUString aName( rName );
                rCnt.InsertEmbeddedObject( xObj, aName );
                pImpl->maObjectToNameMap.erase( aIt->second );
                pImpl->maNameToObjectMap.erase( aIt );
                uno::Reference < embed::XEmbedPersist > xPersist( xObj, uno::UNO_QUERY );
                if ( xPersist.is() )
                    pImpl->mxStorage->removeElement( rName );
            }
            else
            {
                // copy storages; object *must* have persistence!
                uno::Reference < embed::XStorage > xOld = pImpl->mxStorage->openStorageElement( rName, embed::ElementModes::READ );
                uno::Reference < embed::XStorage > xNew = rCnt.pImpl->mxStorage->openStorageElement( rName, embed::ElementModes::READWRITE );
                xOld->copyToStorage( xNew );
            }

            rCnt.TryToCopyGraphReplacement( *this, rName, rName );
            // RemoveGraphicStream( rName );

            return true;
        }
        catch (const uno::Exception&)
        {
            SAL_WARN( "comphelper.container", "Could not move object!");
            return false;
        }

    }
    else
        SAL_WARN( "comphelper.container", "Unknown object!");
    return false;
}

// #i119941, bKeepToTempStorage: use to specify whether store the removed object to temporary storage+
bool EmbeddedObjectContainer::RemoveEmbeddedObject( const uno::Reference < embed::XEmbeddedObject >& xObj, bool bKeepToTempStorage )
{
    uno::Reference < embed::XEmbedPersist > xPersist( xObj, uno::UNO_QUERY );
    OUString aName;
    if ( xPersist.is() )
        aName = xPersist->getEntryName();

#if OSL_DEBUG_LEVEL > 1
    uno::Reference < container::XNameAccess > xAccess( pImpl->mxStorage, uno::UNO_QUERY );
    uno::Reference < embed::XLinkageSupport > xLink( xPersist, uno::UNO_QUERY );
    sal_Bool bIsNotEmbedded = !xPersist.is() || ( xLink.is() && xLink->isLink() );

    // if the object has a persistence and the object is not a link than it must have persistence entry in the storage
    OSL_ENSURE( bIsNotEmbedded || xAccess->hasByName(aName), "Removing element not present in storage!" );
#endif

    // somebody still needs the object, so we must assign a temporary persistence
    try
    {
        if ( xPersist.is() && bKeepToTempStorage ) // #i119941
        {

            if ( !pImpl->mpTempObjectContainer )
            {
                pImpl->mpTempObjectContainer = new EmbeddedObjectContainer();
                try
                {
                    // TODO/LATER: in future probably the temporary container will have two storages ( of two formats )
                    //             the media type will be provided with object insertion
                    OUString aOrigStorMediaType;
                    uno::Reference< beans::XPropertySet > xStorProps( pImpl->mxStorage, uno::UNO_QUERY_THROW );
                    static constexpr OUString s_sMediaType(u"MediaType"_ustr);
                    xStorProps->getPropertyValue( s_sMediaType ) >>= aOrigStorMediaType;

                    SAL_WARN_IF( aOrigStorMediaType.isEmpty(), "comphelper.container", "No valuable media type in the storage!" );

                    uno::Reference< beans::XPropertySet > xTargetStorProps(
                                                                pImpl->mpTempObjectContainer->pImpl->mxStorage,
                                                                uno::UNO_QUERY_THROW );
                    xTargetStorProps->setPropertyValue( s_sMediaType,uno::Any( aOrigStorMediaType ) );
                }
                catch (const uno::Exception&)
                {
                    SAL_WARN( "comphelper.container", "Can not set the new media type to a storage!" );
                }
            }

            OUString aTempName, aMediaType;
            /* Do not create a new name for a removed object, in the pImpl->mpTempObjectContainer,
            because the original m_aEntryName of xObj will be overwritten by InsertEmbeddedObject(),
            so uno::Reference < embed::XEmbeddedObject >& xObj will misbehave in
            EmbeddedObjectContainer::StoreAsChildren and SfxObjectShell::SaveCompletedChildren
            and will throw an exception because of objects with the same names! */
            if( !pImpl->mpTempObjectContainer->HasEmbeddedObject(aName) )
                aTempName = aName;

            pImpl->mpTempObjectContainer->InsertEmbeddedObject( xObj, aTempName );

            uno::Reference < io::XInputStream > xStream = GetGraphicStream( xObj, &aMediaType );
            if ( xStream.is() )
                pImpl->mpTempObjectContainer->InsertGraphicStream( xStream, aTempName, aMediaType );

            // object is stored, so at least it can be set to loaded state
            xObj->changeState( embed::EmbedStates::LOADED );
        }
        else
            // objects without persistence need to stay in running state if they shall not be closed
            xObj->changeState( embed::EmbedStates::RUNNING );
    }
    catch (const uno::Exception&)
    {
        return false;
    }

    auto aIter = std::find_if(pImpl->maNameToObjectMap.begin(), pImpl->maNameToObjectMap.end(),
        [&xObj](const EmbeddedObjectContainerNameMap::value_type& rEntry) { return rEntry.second == xObj; });
    if (aIter != pImpl->maNameToObjectMap.end())
    {
        pImpl->maObjectToNameMap.erase( aIter->second );
        pImpl->maNameToObjectMap.erase( aIter );
        uno::Reference < container::XChild > xChild( xObj, uno::UNO_QUERY );
        if ( xChild.is() )
            xChild->setParent( uno::Reference < uno::XInterface >() );
    }
    else
        SAL_WARN( "comphelper.container", "Object not found for removal!" );

    if ( !xPersist || !bKeepToTempStorage )  // #i119941#
        return true;

    // remove replacement image (if there is one)
    RemoveGraphicStream( aName );

    // now it's time to remove the storage from the container storage
    try
    {
#if OSL_DEBUG_LEVEL > 1
        // if the object has a persistence and the object is not a link than it must have persistence entry in storage
        OSL_ENSURE( bIsNotEmbedded || pImpl->mxStorage->hasByName( aName ), "The object has no persistence entry in the storage!" );
#endif
        if ( xPersist.is() && pImpl->mxStorage->hasByName( aName ) )
            pImpl->mxStorage->removeElement( aName );
    }
    catch (const uno::Exception&)
    {
        SAL_WARN( "comphelper.container", "Failed to remove object from storage!" );
        return false;
    }

    return true;
}

void EmbeddedObjectContainer::CloseEmbeddedObject( const uno::Reference < embed::XEmbeddedObject >& xObj )
{
    // disconnect the object from the container and close it if possible

    auto aIter = std::find_if(pImpl->maNameToObjectMap.begin(), pImpl->maNameToObjectMap.end(),
        [&xObj](const EmbeddedObjectContainerNameMap::value_type& rEntry) { return rEntry.second == xObj; });
    if (aIter == pImpl->maNameToObjectMap.end())
        return;

    pImpl->maObjectToNameMap.erase( aIter->second );
    pImpl->maNameToObjectMap.erase( aIter );

    try
    {
        xObj->close( true );
    }
    catch (const uno::Exception&)
    {
        // it is no problem if the object is already closed
        // TODO/LATER: what if the object can not be closed?
    }
}

uno::Reference < io::XInputStream > EmbeddedObjectContainer::GetGraphicStream( const OUString& aName, OUString* pMediaType )
{
    uno::Reference < io::XInputStream > xStream;

    SAL_WARN_IF( aName.isEmpty(), "comphelper.container", "Retrieving graphic for unknown object!" );
    if ( !aName.isEmpty() )
    {
        try
        {
            uno::Reference < embed::XStorage > xReplacements = pImpl->GetReplacements();
            uno::Reference < io::XStream > xGraphicStream = xReplacements->openStreamElement( aName, embed::ElementModes::READ );
            xStream = xGraphicStream->getInputStream();
            if ( pMediaType )
            {
                uno::Reference < beans::XPropertySet > xSet( xStream, uno::UNO_QUERY );
                if ( xSet.is() )
                {
                    uno::Any aAny = xSet->getPropertyValue(u"MediaType"_ustr);
                    aAny >>= *pMediaType;
                }
            }
        }
        catch (uno::Exception const& e)
        {
            SAL_INFO("comphelper.container",
                "EmbeddedObjectContainer::GetGraphicStream(): " << e);
        }
    }

    return xStream;
}

uno::Reference < io::XInputStream > EmbeddedObjectContainer::GetGraphicStream( const css::uno::Reference < css::embed::XEmbeddedObject >& xObj, OUString* pMediaType )
{
    // try to load it from the container storage
    return GetGraphicStream( GetEmbeddedObjectName( xObj ), pMediaType );
}

bool EmbeddedObjectContainer::InsertGraphicStream( const css::uno::Reference < css::io::XInputStream >& rStream, const OUString& rObjectName, const OUString& rMediaType )
{
    try
    {
        uno::Reference < embed::XStorage > xReplacements = pImpl->GetReplacements();

        // store it into the subfolder
        uno::Reference < io::XOutputStream > xOutStream;
        uno::Reference < io::XStream > xGraphicStream = xReplacements->openStreamElement( rObjectName,
                embed::ElementModes::READWRITE | embed::ElementModes::TRUNCATE );
        xOutStream = xGraphicStream->getOutputStream();
        ::comphelper::OStorageHelper::CopyInputToOutput( rStream, xOutStream );
        xOutStream->flush();

        uno::Reference< beans::XPropertySet > xPropSet( xGraphicStream, uno::UNO_QUERY );
        if (xPropSet)
        {
            xPropSet->setPropertyValue(u"UseCommonStoragePasswordEncryption"_ustr,
                                        uno::Any( true ) );
            xPropSet->setPropertyValue(u"MediaType"_ustr, uno::Any(rMediaType) );

            xPropSet->setPropertyValue(u"Compressed"_ustr,
                                        uno::Any( true ) );
        }
    }
    catch (const uno::Exception&)
    {
        return false;
    }

    return true;
}

bool EmbeddedObjectContainer::InsertGraphicStreamDirectly( const css::uno::Reference < css::io::XInputStream >& rStream, const OUString& rObjectName, const OUString& rMediaType )
{
    try
    {
        uno::Reference < embed::XStorage > xReplacement = pImpl->GetReplacements();
        uno::Reference < embed::XOptimizedStorage > xOptRepl( xReplacement, uno::UNO_QUERY_THROW );

        // store it into the subfolder
        uno::Sequence< beans::PropertyValue > aProps{
            comphelper::makePropertyValue(u"MediaType"_ustr, rMediaType),
            comphelper::makePropertyValue(u"UseCommonStoragePasswordEncryption"_ustr, true),
            comphelper::makePropertyValue(u"Compressed"_ustr, true)
        };

        if ( xReplacement->hasByName( rObjectName ) )
            xReplacement->removeElement( rObjectName );

        xOptRepl->insertStreamElementDirect( rObjectName, rStream, aProps );
    }
    catch (const uno::Exception&)
    {
        return false;
    }

    return true;
}


void EmbeddedObjectContainer::RemoveGraphicStream( const OUString& rObjectName )
{
    try
    {
        uno::Reference < embed::XStorage > xReplacements = pImpl->GetReplacements();
        xReplacements->removeElement( rObjectName );
    }
    catch (const uno::Exception&)
    {
    }
}
namespace {
    void InsertStreamIntoPicturesStorage_Impl( const uno::Reference< embed::XStorage >& xDocStor,
                                            const uno::Reference< io::XInputStream >& xInStream,
                                            const OUString& aStreamName )
    {
        OSL_ENSURE( !aStreamName.isEmpty() && xInStream.is() && xDocStor.is(), "Misuse of the method!" );

        try
        {
            uno::Reference< embed::XStorage > xPictures = xDocStor->openStorageElement(
                                        u"Pictures"_ustr,
                                        embed::ElementModes::READWRITE );
            uno::Reference< io::XStream > xObjReplStr = xPictures->openStreamElement(
                                        aStreamName,
                                        embed::ElementModes::READWRITE | embed::ElementModes::TRUNCATE );
            uno::Reference< io::XOutputStream > xOutStream(
                                        xObjReplStr->getInputStream(), uno::UNO_QUERY_THROW );

            ::comphelper::OStorageHelper::CopyInputToOutput( xInStream, xOutStream );
            xOutStream->closeOutput();

            uno::Reference< embed::XTransactedObject > xTransact( xPictures, uno::UNO_QUERY );
            if ( xTransact.is() )
                xTransact->commit();
        }
        catch (const uno::Exception&)
        {
            SAL_WARN( "comphelper.container", "The images storage is not available!" );
        }
    }

}

static bool AlwaysStoreReplacementImages(const uno::Reference<embed::XEmbeddedObject>& xObj)
{
    const auto clsid = xObj->getClassID();
    if (clsid == MimeConfigurationHelper::GetSequenceClassID(SO3_SCH_CLASSID)
        || clsid == MimeConfigurationHelper::GetSequenceClassID(SO3_SM_CLASSID))
        return false;
    return true;
}

bool EmbeddedObjectContainer::StoreAsChildren(bool _bOasisFormat,bool _bCreateEmbedded, bool _bAutoSaveEvent,
                                              const uno::Reference < embed::XStorage >& _xStorage)
{
    bool bResult = false;
    try
    {
        comphelper::EmbeddedObjectContainer aCnt( _xStorage );
        for (auto& name : GetObjectNames())
        {
            uno::Reference<embed::XEmbeddedObject> xObj = GetEmbeddedObject(name);
            SAL_WARN_IF( !xObj.is(), "comphelper.container", "An empty entry in the embedded objects list!" );
            if ( xObj.is() )
            {
                bool bSwitchBackToLoaded = false;
                uno::Reference< embed::XLinkageSupport > xLink( xObj, uno::UNO_QUERY );

                uno::Reference < io::XInputStream > xStream;
                OUString aMediaType;
                if (officecfg::Office::Common::Save::Graphic::AddReplacementImages::get()
                    || AlwaysStoreReplacementImages(xObj))
                {
                    sal_Int32 nCurState = xObj->getCurrentState();
                    if (nCurState == embed::EmbedStates::LOADED
                        || nCurState == embed::EmbedStates::RUNNING)
                    {
                        // means that the object is not active
                        // copy replacement image from old to new container
                        xStream = GetGraphicStream(xObj, &aMediaType);
                    }

                    if (!xStream.is() && getUserAllowsLinkUpdate())
                    {
                        // the image must be regenerated
                        // TODO/LATER: another aspect could be used
                        if (xObj->getCurrentState() == embed::EmbedStates::LOADED)
                            bSwitchBackToLoaded = true;

                        xStream = GetGraphicReplacementStream(embed::Aspects::MSOLE_CONTENT, xObj,
                                                              &aMediaType);
                    }
                }

                if ( _bOasisFormat || (xLink.is() && xLink->isLink()) )
                {
                    if ( xStream.is() )
                    {
                        if ( _bOasisFormat )
                        {
                            // if it is an embedded object or the optimized inserting fails the normal inserting should be done
                            if ( _bCreateEmbedded
                                || !aCnt.InsertGraphicStreamDirectly(xStream, name, aMediaType))
                                aCnt.InsertGraphicStream(xStream, name, aMediaType);
                        }
                        else
                        {
                            // it is a linked object exported into SO7 format
                            InsertStreamIntoPicturesStorage_Impl(_xStorage, xStream, name);
                        }
                    }
                }

                uno::Reference< embed::XEmbedPersist > xPersist( xObj, uno::UNO_QUERY );
                if ( xPersist.is() )
                {
                    uno::Sequence< beans::PropertyValue > aArgs( _bOasisFormat ? 3 : 4 );
                    auto pArgs = aArgs.getArray();
                    pArgs[0].Name = "StoreVisualReplacement";
                    pArgs[0].Value <<= !_bOasisFormat;

                    // if it is an embedded object or the optimized inserting fails the normal inserting should be done
                    pArgs[1].Name = "CanTryOptimization";
                    pArgs[1].Value <<= !_bCreateEmbedded;

                    pArgs[2].Name = "AutoSaveEvent";
                    pArgs[2].Value <<= _bAutoSaveEvent;

                    if ( !_bOasisFormat )
                    {
                        // if object has no cached replacement it will use this one
                        pArgs[3].Name = "VisualReplacement";
                        pArgs[3].Value <<= xStream;
                    }

                    try
                    {
                        xPersist->storeAsEntry( _xStorage, xPersist->getEntryName(), uno::Sequence< beans::PropertyValue >(), aArgs );
                    }
                    catch (const embed::WrongStateException&)
                    {
                        SAL_WARN("comphelper.container", "failed to store '" << name << "'");
                    }
                }

                if ( bSwitchBackToLoaded )
                    // switch back to loaded state; that way we have a minimum cache confusion
                    xObj->changeState( embed::EmbedStates::LOADED );
            }
        }

        bResult = aCnt.CommitImageSubStorage();

    }
    catch (const uno::Exception& e)
    {
        // TODO/LATER: error handling
        bResult = false;
        SAL_WARN("comphelper.container", "failed. Message: " << e);
    }

    // the old SO6 format does not store graphical replacements
    if ( !_bOasisFormat && bResult )
    {
        try
        {
            // the substorage still can not be locked by the embedded object container
            OUString aObjReplElement( u"ObjectReplacements"_ustr );
            if ( _xStorage->hasByName( aObjReplElement ) && _xStorage->isStorageElement( aObjReplElement ) )
                _xStorage->removeElement( aObjReplElement );
        }
        catch (const uno::Exception&)
        {
            // TODO/LATER: error handling;
            bResult = false;
        }
    }
    return bResult;
}

bool EmbeddedObjectContainer::StoreChildren(bool _bOasisFormat,bool _bObjectsOnly)
{
    bool bResult = true;
    for (auto& name : GetObjectNames())
    {
        try
        {
            uno::Reference<embed::XEmbeddedObject> xObj = GetEmbeddedObject(name);
            SAL_WARN_IF( !xObj.is(), "comphelper.container", "An empty entry in the embedded objects list!" );
            if ( xObj.is() )
            {
                sal_Int32 nCurState = xObj->getCurrentState();
                if ( _bOasisFormat && nCurState != embed::EmbedStates::LOADED && nCurState != embed::EmbedStates::RUNNING )
                {
                    // means that the object is active
                    // the image must be regenerated
                    OUString aMediaType;

                    // TODO/LATER: another aspect could be used
                    uno::Reference < io::XInputStream > xStream =
                                GetGraphicReplacementStream(
                                                            embed::Aspects::MSOLE_CONTENT,
                                                            xObj,
                                                            &aMediaType );
                    if ( xStream.is() )
                    {
                        if (!InsertGraphicStreamDirectly(xStream, name, aMediaType))
                            InsertGraphicStream(xStream, name, aMediaType);
                    }
                }

                // TODO/LATER: currently the object by default does not cache replacement image
                // that means that if somebody loads SO7 document and store its objects using
                // this method the images might be lost.
                // Currently this method is only used on storing to alien formats, that means
                // that SO7 documents storing does not use it, and all other filters are
                // based on OASIS format. But if it changes the method must be fixed. The fix
                // must be done only on demand since it can affect performance.

                uno::Reference< embed::XEmbedPersist > xPersist( xObj, uno::UNO_QUERY );
                if ( xPersist.is() )
                {
                    try
                    {
                        //TODO/LATER: only storing if changed!
                        //xPersist->storeOwn(); //commented, i120168

                        // begin:all charts will be persisted as xml format on disk when saving, which is time consuming.
                        // '_bObjectsOnly' mean we are storing to alien formats.
                        //  'isStorageElement' mean current object is NOT a MS OLE format. (may also include in future), i120168
                        if (_bObjectsOnly && (nCurState == embed::EmbedStates::LOADED || nCurState == embed::EmbedStates::RUNNING)
                            && (pImpl->mxStorage->isStorageElement(name)))
                        {
                            uno::Reference< util::XModifiable > xModifiable( xObj->getComponent(), uno::UNO_QUERY );
                            if ( xModifiable.is() && xModifiable->isModified())
                            {
                                xPersist->storeOwn();
                            }
                            else
                            {
                                //do nothing. Embedded model is not modified, no need to persist.
                            }
                        }
                        else //the embedded object is in active status, always store back it.
                        {
                            xPersist->storeOwn();
                        }
                        //end i120168
                    }
                    catch (const uno::Exception&)
                    {
                        // TODO/LATER: error handling
                        bResult = false;
                        break;
                    }
                }

                if ( !_bOasisFormat && !_bObjectsOnly )
                {
                    // copy replacement images for linked objects
                    try
                    {
                        uno::Reference< embed::XLinkageSupport > xLink( xObj, uno::UNO_QUERY );
                        if ( xLink.is() && xLink->isLink() )
                        {
                            OUString aMediaType;
                            uno::Reference < io::XInputStream > xInStream = GetGraphicStream( xObj, &aMediaType );
                            if ( xInStream.is() )
                                InsertStreamIntoPicturesStorage_Impl( pImpl->mxStorage, xInStream, name );
                        }
                    }
                    catch (const uno::Exception&)
                    {
                    }
                }
            }
        }
        catch (const uno::Exception&)
        {
            // TODO/LATER: error handling
        }
    }

    if ( bResult && _bOasisFormat )
        bResult = CommitImageSubStorage();

    if ( bResult && !_bObjectsOnly )
    {
        try
        {
            ReleaseImageSubStorage();
            OUString aObjReplElement( u"ObjectReplacements"_ustr );
            if ( !_bOasisFormat && pImpl->mxStorage->hasByName( aObjReplElement ) && pImpl->mxStorage->isStorageElement( aObjReplElement ) )
                pImpl->mxStorage->removeElement( aObjReplElement );
        }
        catch (const uno::Exception&)
        {
            // TODO/LATER: error handling
            bResult = false;
        }
    }
    return bResult;
}

uno::Reference< io::XInputStream > EmbeddedObjectContainer::GetGraphicReplacementStream(
                                                                sal_Int64 nViewAspect,
                                                                const uno::Reference< embed::XEmbeddedObject >& xObj,
                                                                OUString* pMediaType )
{
    if ( !xObj.is() )
        return nullptr;

    rtl::Reference< ::comphelper::SequenceInputStream > xInStream;
    try
    {
        // retrieving of the visual representation can switch object to running state
        embed::VisualRepresentation aRep = xObj->getPreferredVisualRepresentation( nViewAspect );
        if ( pMediaType )
            *pMediaType = aRep.Flavor.MimeType;

        uno::Sequence < sal_Int8 > aSeq;
        aRep.Data >>= aSeq;
        xInStream = new ::comphelper::SequenceInputStream( aSeq );
    }
    catch (const uno::Exception&)
    {
    }

    return xInStream;
}

bool EmbeddedObjectContainer::SetPersistentEntries(const uno::Reference< embed::XStorage >& _xStorage,bool _bClearModifiedFlag)
{
    bool bError = false;
    for (auto& name : GetObjectNames())
    {
        uno::Reference<embed::XEmbeddedObject> xObj = GetEmbeddedObject(name);
        SAL_WARN_IF( !xObj.is(), "comphelper.container", "An empty entry in the embedded objects list!" );
        if ( xObj.is() )
        {
            uno::Reference< embed::XEmbedPersist > xPersist( xObj, uno::UNO_QUERY );
            if ( xPersist.is() )
            {
                try
                {
                    xPersist->setPersistentEntry( _xStorage,
                                                name,
                                                embed::EntryInitModes::NO_INIT,
                                                uno::Sequence< beans::PropertyValue >(),
                                                uno::Sequence< beans::PropertyValue >() );

                }
                catch (const uno::Exception&)
                {
                    // TODO/LATER: error handling
                    bError = true;
                    break;
                }
            }
            if ( _bClearModifiedFlag )
            {
                // if this method is used as part of SaveCompleted the object must stay unmodified after execution
                try
                {
                    uno::Reference< util::XModifiable > xModif( xObj->getComponent(), uno::UNO_QUERY );
                    if ( xModif && xModif->isModified() )
                        xModif->setModified( false );
                }
                catch (const uno::Exception&)
                {
                }
            }
        }
    }
    return bError;
}

bool EmbeddedObjectContainer::getUserAllowsLinkUpdate() const
{
    if (officecfg::Office::Common::Security::Scripting::DisableActiveContent::get())
        return false;
    return pImpl->mbUserAllowsLinkUpdate;
}

void EmbeddedObjectContainer::setUserAllowsLinkUpdate(bool bNew)
{
    if(pImpl->mbUserAllowsLinkUpdate != bNew)
    {
        pImpl->mbUserAllowsLinkUpdate = bNew;
    }
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
