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

#include <querycontainer.hxx>
#include "query.hxx"
#include <strings.hxx>
#include <objectnameapproval.hxx>
#include <veto.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XContainer.hpp>
#include <com/sun/star/container/XContainerApproveBroadcaster.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <com/sun/star/lang/WrappedTargetRuntimeException.hpp>
#include <com/sun/star/sdbc/XConnection.hpp>
#include <com/sun/star/sdb/QueryDefinition.hpp>

#include <osl/diagnose.h>
#include <comphelper/uno3.hxx>
#include <comphelper/property.hxx>
#include <comphelper/types.hxx>
#include <cppuhelper/exc_hlp.hxx>

using namespace dbtools;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::ucb;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::util;
using namespace ::osl;
using namespace ::comphelper;
using namespace ::cppu;

namespace dbaccess
{

// OQueryContainer

OQueryContainer::OQueryContainer(
                  const Reference< XNameContainer >& _rxCommandDefinitions
                , const Reference< XConnection >& _rxConn
                , const Reference< XComponentContext >& _rxORB,
                ::dbtools::WarningsContainer* _pWarnings)
    :ODefinitionContainer(_rxORB,nullptr,std::make_shared<ODefinitionContainer_Impl>())
    ,m_pWarnings( _pWarnings )
    ,m_xCommandDefinitions(_rxCommandDefinitions)
    ,m_xConnection(_rxConn)
    ,m_eDoingCurrently(AggregateAction::NONE)
{
}

void OQueryContainer::init()
{
    Reference< XContainer > xContainer( m_xCommandDefinitions, UNO_QUERY_THROW );
    xContainer->addContainerListener( this );

    Reference< XContainerApproveBroadcaster > xContainerApprove( m_xCommandDefinitions, UNO_QUERY_THROW );
    xContainerApprove->addContainerApproveListener( this );

    // fill my structures
    ODefinitionContainer_Impl& rDefinitions( getDefinitions() );
    for (auto& definitionName : m_xCommandDefinitions->getElementNames())
    {
        rDefinitions.insert(definitionName, TContentPtr());
        m_aDocuments.push_back(m_aDocumentMap.emplace(definitionName, Documents::mapped_type()).first);
    }

    setElementApproval( std::make_shared<ObjectNameApproval>( m_xConnection, ObjectNameApproval::TypeQuery ) );
}

rtl::Reference<OQueryContainer> OQueryContainer::create(
                  const Reference< XNameContainer >& _rxCommandDefinitions
                , const Reference< XConnection >& _rxConn
                , const Reference< XComponentContext >& _rxORB,
                ::dbtools::WarningsContainer* _pWarnings)
{
    rtl::Reference c(
        new OQueryContainer(
            _rxCommandDefinitions, _rxConn, _rxORB, _pWarnings));
    c->init();
    return c;
}

OQueryContainer::~OQueryContainer()
{
    //  dispose();
        //  maybe we're already disposed, but this should be uncritical
}

IMPLEMENT_FORWARD_XINTERFACE2( OQueryContainer,ODefinitionContainer,OQueryContainer_Base)

IMPLEMENT_FORWARD_XTYPEPROVIDER2( OQueryContainer,ODefinitionContainer,OQueryContainer_Base)

void OQueryContainer::disposing()
{
    ODefinitionContainer::disposing();
    MutexGuard aGuard(m_aMutex);
    if ( !m_xCommandDefinitions.is() )
        // already disposed
        return;

    Reference< XContainer > xContainer( m_xCommandDefinitions, UNO_QUERY );
    xContainer->removeContainerListener( this );
    Reference< XContainerApproveBroadcaster > xContainerApprove( m_xCommandDefinitions, UNO_QUERY );
    xContainerApprove->removeContainerApproveListener( this );

    m_xCommandDefinitions   = nullptr;
    m_xConnection           = nullptr;
}

// XServiceInfo
OUString SAL_CALL OQueryContainer::getImplementationName()
    {
        return u"com.sun.star.sdb.dbaccess.OQueryContainer"_ustr;
    }
sal_Bool SAL_CALL OQueryContainer::supportsService(const OUString& _rServiceName)
    {
        const css::uno::Sequence< OUString > aSupported(getSupportedServiceNames());
        for (const OUString& s : aSupported)
            if (s == _rServiceName)
                return true;

        return false;
    }
css::uno::Sequence< OUString > SAL_CALL OQueryContainer::getSupportedServiceNames()
{
    return { SERVICE_SDBCX_CONTAINER, SERVICE_SDB_QUERIES };
}

// XDataDescriptorFactory
Reference< XPropertySet > SAL_CALL OQueryContainer::createDataDescriptor(  )
{
    return new OQueryDescriptor();
}

// XAppend
void SAL_CALL OQueryContainer::appendByDescriptor( const Reference< XPropertySet >& _rxDesc )
{
    ResettableMutexGuard aGuard(m_aMutex);
    if ( !m_xCommandDefinitions.is() )
        throw DisposedException( OUString(), *this );

    // first clone this object's CommandDefinition part
    Reference< css::sdb::XQueryDefinition > xCommandDefinitionPart = css::sdb::QueryDefinition::create(m_aContext);

    ::comphelper::copyProperties( _rxDesc, Reference<XPropertySet>(xCommandDefinitionPart, UNO_QUERY_THROW) );
    // TODO : the columns part of the descriptor has to be copied

    // create a wrapper for the object (*before* inserting into our command definition container)
    Reference< XContent > xNewObject( implCreateWrapper( Reference< XContent>( xCommandDefinitionPart, UNO_QUERY_THROW ) ) );

    OUString sNewObjectName;
    _rxDesc->getPropertyValue(PROPERTY_NAME) >>= sNewObjectName;

    try
    {
        notifyByName( aGuard, sNewObjectName, xNewObject, nullptr, E_INSERTED, ApproveListeners );
    }
    catch (const WrappedTargetException& e)
    {
        disposeComponent( xNewObject );
        disposeComponent( xCommandDefinitionPart );
        throw WrappedTargetRuntimeException(e.Message, e.Context, e.TargetException);
    }
    catch (const Exception&)
    {
        disposeComponent( xNewObject );
        disposeComponent( xCommandDefinitionPart );
        throw;
    }

    // insert the basic object into the definition container
    {
        m_eDoingCurrently = AggregateAction::Inserting;
        OAutoActionReset aAutoReset(*this);
        m_xCommandDefinitions->insertByName(sNewObjectName, Any(xCommandDefinitionPart));
    }

    implAppend( sNewObjectName, xNewObject );
    try
    {
        notifyByName( aGuard, sNewObjectName, xNewObject, nullptr, E_INSERTED, ContainerListemers );
    }
    catch (const WrappedTargetException& e)
    {
        throw WrappedTargetRuntimeException(e.Message, e.Context, e.TargetException);
    }
}

// XDrop
void SAL_CALL OQueryContainer::dropByName( const OUString& _rName )
{
    MutexGuard aGuard(m_aMutex);
    if ( !checkExistence(_rName) )
        throw NoSuchElementException(_rName,*this);

    if ( !m_xCommandDefinitions.is() )
        throw DisposedException( OUString(), *this );

    // now simply forward the remove request to the CommandDefinition container, we're a listener for the removal
    // and thus we do everything necessary in ::elementRemoved
    m_xCommandDefinitions->removeByName(_rName);
}

void SAL_CALL OQueryContainer::dropByIndex( sal_Int32 _nIndex )
{
    MutexGuard aGuard(m_aMutex);
    if ((_nIndex<0) || (_nIndex>getCount()))
        throw IndexOutOfBoundsException();

    if ( !m_xCommandDefinitions.is() )
        throw DisposedException( OUString(), *this );

    OUString sName;
    Reference<XPropertySet> xProp(Reference<XIndexAccess>(m_xCommandDefinitions,UNO_QUERY_THROW)->getByIndex(_nIndex),UNO_QUERY);
    if ( xProp.is() )
        xProp->getPropertyValue(PROPERTY_NAME) >>= sName;

    dropByName(sName);
}

void SAL_CALL OQueryContainer::elementInserted( const css::container::ContainerEvent& _rEvent )
{
    Reference< XContent > xNewElement;
    OUString sElementName;
    _rEvent.Accessor >>= sElementName;
    {
        MutexGuard aGuard(m_aMutex);
        if (AggregateAction::Inserting == m_eDoingCurrently)
            // nothing to do, we're inserting via an "appendByDescriptor"
            return;

        OSL_ENSURE(!sElementName.isEmpty(), "OQueryContainer::elementInserted : invalid name !");
        OSL_ENSURE(m_aDocumentMap.find(sElementName) == m_aDocumentMap.end(), "OQueryContainer::elementInserted         : oops... we're inconsistent with our master container !");
        if (sElementName.isEmpty() || hasByName(sElementName))
            return;

        // insert an own new element
        xNewElement = implCreateWrapper(sElementName);
    }
    insertByName(sElementName,Any(xNewElement));
}

void SAL_CALL OQueryContainer::elementRemoved( const css::container::ContainerEvent& _rEvent )
{
    OUString sAccessor;
    _rEvent.Accessor >>= sAccessor;
    {
        OSL_ENSURE(!sAccessor.isEmpty(), "OQueryContainer::elementRemoved : invalid name !");
        OSL_ENSURE(m_aDocumentMap.contains(sAccessor), "OQueryContainer::elementRemoved : oops... we're inconsistent with our master container !");
        if ( sAccessor.isEmpty() || !hasByName(sAccessor) )
            return;
    }
    removeByName(sAccessor);
}

void SAL_CALL OQueryContainer::elementReplaced( const css::container::ContainerEvent& _rEvent )
{
    Reference< XContent > xNewElement;
    OUString sAccessor;
    _rEvent.Accessor >>= sAccessor;

    {
        MutexGuard aGuard(m_aMutex);
        OSL_ENSURE(!sAccessor.isEmpty(), "OQueryContainer::elementReplaced : invalid name !");
        OSL_ENSURE(m_aDocumentMap.contains(sAccessor), "OQueryContainer::elementReplaced         : oops... we're inconsistent with our master container !");
        if (sAccessor.isEmpty() || !hasByName(sAccessor))
            return;

        xNewElement = implCreateWrapper(sAccessor);
    }

    replaceByName(sAccessor,Any(xNewElement));
}

Reference< XVeto > SAL_CALL OQueryContainer::approveInsertElement( const ContainerEvent& Event )
{
    OUString sName;
    OSL_VERIFY( Event.Accessor >>= sName );
    Reference< XContent > xElement( Event.Element, UNO_QUERY_THROW );

    rtl::Reference< Veto > xReturn;
    try
    {
        getElementApproval()->approveElement( sName );
    }
    catch( const Exception& )
    {
        xReturn = new Veto( ::cppu::getCaughtException() );
    }
    return xReturn;
}

Reference< XVeto > SAL_CALL OQueryContainer::approveReplaceElement( const ContainerEvent& /*Event*/ )
{
    return nullptr;
}

Reference< XVeto > SAL_CALL OQueryContainer::approveRemoveElement( const ContainerEvent& /*Event*/ )
{
    return nullptr;
}

void SAL_CALL OQueryContainer::disposing( const css::lang::EventObject& _rSource )
{
    if (_rSource.Source.get() == Reference< XInterface >(m_xCommandDefinitions, UNO_QUERY).get())
    {   // our "master container" (with the command definitions) is being disposed
        OSL_FAIL("OQueryContainer::disposing : nobody should dispose the CommandDefinition container before disposing my connection !");
        dispose();
    }
    else
    {
        Reference< XContent > xSource(_rSource.Source, UNO_QUERY);
        // it's one of our documents...
        for (auto const& document : m_aDocumentMap)
        {
            if ( xSource == document.second.get() )
            {
                m_xCommandDefinitions->removeByName(document.first);
                break;
            }
        }
        ODefinitionContainer::disposing(_rSource);
    }
}

OUString OQueryContainer::determineContentType() const
{
    return u"application/vnd.org.openoffice.DatabaseQueryContainer"_ustr;
}

Reference< XContent > OQueryContainer::implCreateWrapper(const OUString& _rName)
{
    Reference< XContent > xObject(m_xCommandDefinitions->getByName(_rName),UNO_QUERY);
    return implCreateWrapper(xObject);
}

Reference< XContent > OQueryContainer::implCreateWrapper(const Reference< XContent >& _rxCommandDesc)
{
    Reference<XNameContainer> xContainer(_rxCommandDesc,UNO_QUERY);
    rtl::Reference< OContentHelper > xReturn;
    if ( xContainer .is() )
    {
        xReturn = create( xContainer, m_xConnection, m_aContext, m_pWarnings ).
            get();
    }
    else
    {
        rtl::Reference<OQuery> pNewObject = new OQuery( Reference< XPropertySet >( _rxCommandDesc, UNO_QUERY ), m_xConnection, m_aContext );
        xReturn = pNewObject;

        pNewObject->setWarningsContainer( m_pWarnings );
//      pNewObject->getColumns();
        // Why? This is expensive. If you comment this in 'cause you really need it, be sure to run the
        // QueryInQuery test in dbaccess/qa/complex/dbaccess ...
    }

    return xReturn;
}

Reference< XContent > OQueryContainer::createObject( const OUString& _rName)
{
    return implCreateWrapper(_rName);
}

bool OQueryContainer::checkExistence(const OUString& _rName)
{
    bool bRet = false;
    if ( !m_bInPropertyChange )
    {
        bRet = m_xCommandDefinitions->hasByName(_rName);
        Documents::const_iterator aFind = m_aDocumentMap.find(_rName);
        if ( !bRet && aFind != m_aDocumentMap.end() )
        {
            m_aDocuments.erase( std::find(m_aDocuments.begin(),m_aDocuments.end(),aFind));
            m_aDocumentMap.erase(aFind);
        }
        else if ( bRet && aFind == m_aDocumentMap.end() )
        {
            implAppend(_rName,nullptr);
        }
    }
    return bRet;
}

sal_Bool SAL_CALL OQueryContainer::hasElements( )
{
    MutexGuard aGuard(m_aMutex);
    return m_xCommandDefinitions->hasElements();
}

sal_Int32 SAL_CALL OQueryContainer::getCount(  )
{
    MutexGuard aGuard(m_aMutex);
    return Reference<XIndexAccess>(m_xCommandDefinitions,UNO_QUERY_THROW)->getCount();
}

Sequence< OUString > SAL_CALL OQueryContainer::getElementNames(  )
{
    MutexGuard aGuard(m_aMutex);

    return m_xCommandDefinitions->getElementNames();
}

}   // namespace dbaccess

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
