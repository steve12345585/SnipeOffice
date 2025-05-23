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

#include <unotools/confignode.hxx>
#include <unotools/configpaths.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <osl/diagnose.h>
#include <sal/log.hxx>
#include <com/sun/star/configuration/theDefaultProvider.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/util/XChangesBatch.hpp>
#include <com/sun/star/util/XStringEscape.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/container/XNamed.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/container/XHierarchicalNameAccess.hpp>
#include <comphelper/namedvaluecollection.hxx>

namespace utl
{

    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::lang;
    using namespace ::com::sun::star::util;
    using namespace ::com::sun::star::container;
    using namespace ::com::sun::star::configuration;

    //= OConfigurationNode

    OConfigurationNode::OConfigurationNode(const Reference< XInterface >& _rxNode )
        :m_bEscapeNames(false)
    {
        OSL_ENSURE(_rxNode.is(), "OConfigurationNode::OConfigurationNode: invalid node interface!");
        if (_rxNode.is())
        {
            // collect all interfaces necessary
            m_xHierarchyAccess.set(_rxNode, UNO_QUERY);
            m_xDirectAccess.set(_rxNode, UNO_QUERY);

            // reset _all_ interfaces if _one_ of them is not supported
            if (!m_xHierarchyAccess.is() || !m_xDirectAccess.is())
            {
                m_xHierarchyAccess = nullptr;
                m_xDirectAccess = nullptr;
            }

            // now for the non-critical interfaces
            m_xReplaceAccess.set(_rxNode, UNO_QUERY);
            m_xContainerAccess.set(_rxNode, UNO_QUERY);
        }

        Reference< XComponent > xConfigNodeComp(m_xDirectAccess, UNO_QUERY);
        if (xConfigNodeComp.is())
            startComponentListening(xConfigNodeComp);

        if (isValid())
            m_bEscapeNames = isSetNode() && Reference< XStringEscape >::query(m_xDirectAccess).is();
    }

    OConfigurationNode::OConfigurationNode(const OConfigurationNode& _rSource)
        : OEventListenerAdapter()
        , m_xHierarchyAccess(_rSource.m_xHierarchyAccess)
        , m_xDirectAccess(_rSource.m_xDirectAccess)
        , m_xReplaceAccess(_rSource.m_xReplaceAccess)
        , m_xContainerAccess(_rSource.m_xContainerAccess)
        , m_bEscapeNames(_rSource.m_bEscapeNames)
    {
        Reference< XComponent > xConfigNodeComp(m_xDirectAccess, UNO_QUERY);
        if (xConfigNodeComp.is())
            startComponentListening(xConfigNodeComp);
    }

    OConfigurationNode::OConfigurationNode(OConfigurationNode&& _rSource)
        : OEventListenerAdapter()
        , m_xHierarchyAccess(std::move(_rSource.m_xHierarchyAccess))
        , m_xDirectAccess(std::move(_rSource.m_xDirectAccess))
        , m_xReplaceAccess(std::move(_rSource.m_xReplaceAccess))
        , m_xContainerAccess(std::move(_rSource.m_xContainerAccess))
        , m_bEscapeNames(std::move(_rSource.m_bEscapeNames))
    {
        Reference< XComponent > xConfigNodeComp(m_xDirectAccess, UNO_QUERY);
        if (xConfigNodeComp.is())
            startComponentListening(xConfigNodeComp);
    }

    OConfigurationNode& OConfigurationNode::operator=(const OConfigurationNode& _rSource)
    {
        stopAllComponentListening();

        m_xHierarchyAccess = _rSource.m_xHierarchyAccess;
        m_xDirectAccess = _rSource.m_xDirectAccess;
        m_xContainerAccess = _rSource.m_xContainerAccess;
        m_xReplaceAccess = _rSource.m_xReplaceAccess;
        m_bEscapeNames = _rSource.m_bEscapeNames;

        Reference< XComponent > xConfigNodeComp(m_xDirectAccess, UNO_QUERY);
        if (xConfigNodeComp.is())
            startComponentListening(xConfigNodeComp);

        return *this;
    }

    OConfigurationNode& OConfigurationNode::operator=(OConfigurationNode&& _rSource)
    {
        stopAllComponentListening();

        m_xHierarchyAccess = std::move(_rSource.m_xHierarchyAccess);
        m_xDirectAccess = std::move(_rSource.m_xDirectAccess);
        m_xContainerAccess = std::move(_rSource.m_xContainerAccess);
        m_xReplaceAccess = std::move(_rSource.m_xReplaceAccess);
        m_bEscapeNames = std::move(_rSource.m_bEscapeNames);

        Reference< XComponent > xConfigNodeComp(m_xDirectAccess, UNO_QUERY);
        if (xConfigNodeComp.is())
            startComponentListening(xConfigNodeComp);

        return *this;
    }

    void OConfigurationNode::_disposing( const EventObject& _rSource )
    {
        Reference< XComponent > xDisposingSource(_rSource.Source, UNO_QUERY);
        Reference< XComponent > xConfigNodeComp(m_xDirectAccess, UNO_QUERY);
        if (xDisposingSource.get() == xConfigNodeComp.get())
            clear();
    }

    OUString OConfigurationNode::getLocalName() const
    {
        OUString sLocalName;
        try
        {
            Reference< XNamed > xNamed( m_xDirectAccess, UNO_QUERY_THROW );
            sLocalName = xNamed->getName();
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("unotools");
        }
        return sLocalName;
    }

    OUString OConfigurationNode::normalizeName(const OUString& _rName, NAMEORIGIN _eOrigin) const
    {
        OUString sName(_rName);
        if (m_bEscapeNames)
        {
            Reference< XStringEscape > xEscaper(m_xDirectAccess, UNO_QUERY);
            if (xEscaper.is() && !sName.isEmpty())
            {
                try
                {
                    if (NO_CALLER == _eOrigin)
                        sName = xEscaper->escapeString(sName);
                    else
                        sName = xEscaper->unescapeString(sName);
                }
                catch(Exception&)
                {
                    DBG_UNHANDLED_EXCEPTION("unotools");
                }
            }
        }
        return sName;
    }

    Sequence< OUString > OConfigurationNode::getNodeNames() const noexcept
    {
        OSL_ENSURE(m_xDirectAccess.is(), "OConfigurationNode::getNodeNames: object is invalid!");
        Sequence< OUString > aReturn;
        if (m_xDirectAccess.is())
        {
            try
            {
                aReturn = m_xDirectAccess->getElementNames();
                // normalize the names
                std::transform(std::cbegin(aReturn), std::cend(aReturn), aReturn.getArray(),
                    [this](const OUString& rName) -> OUString { return normalizeName(rName, NO_CONFIGURATION); });
            }
            catch(Exception&)
            {
                TOOLS_WARN_EXCEPTION( "unotools", "OConfigurationNode::getNodeNames");
            }
        }

        return aReturn;
    }

    bool OConfigurationNode::removeNode(const OUString& _rName) const noexcept
    {
        OSL_ENSURE(m_xContainerAccess.is(), "OConfigurationNode::removeNode: object is invalid!");
        if (m_xContainerAccess.is())
        {
            try
            {
                OUString sName = normalizeName(_rName, NO_CALLER);
                m_xContainerAccess->removeByName(sName);
                return true;
            }
            catch (NoSuchElementException&)
            {
                SAL_WARN( "unotools", "OConfigurationNode::removeNode: there is no element named: " << _rName );
            }
            catch(Exception&)
            {
                TOOLS_WARN_EXCEPTION( "unotools", "OConfigurationNode::removeNode");
            }
        }
        return false;
    }

    OConfigurationNode OConfigurationNode::insertNode(const OUString& _rName,const Reference< XInterface >& _xNode) const noexcept
    {
        if(_xNode.is())
        {
            try
            {
                OUString sName = normalizeName(_rName, NO_CALLER);
                m_xContainerAccess->insertByName(sName, Any(_xNode));
                // if we're here, all was ok ...
                return OConfigurationNode( _xNode );
            }
            catch(const Exception&)
            {
                DBG_UNHANDLED_EXCEPTION("unotools");
            }

            // dispose the child if it has already been created, but could not be inserted
            Reference< XComponent > xChildComp(_xNode, UNO_QUERY);
            if (xChildComp.is())
                try { xChildComp->dispose(); } catch(Exception&) { }
        }

        return OConfigurationNode();
    }

    OConfigurationNode OConfigurationNode::createNode(const OUString& _rName) const noexcept
    {
        Reference< XSingleServiceFactory > xChildFactory(m_xContainerAccess, UNO_QUERY);
        OSL_ENSURE(xChildFactory.is(), "OConfigurationNode::createNode: object is invalid or read-only!");

        if (xChildFactory.is()) // implies m_xContainerAccess.is()
        {
            Reference< XInterface > xNewChild;
            try
            {
                xNewChild = xChildFactory->createInstance();
            }
            catch(const Exception&)
            {
                DBG_UNHANDLED_EXCEPTION("unotools");
            }
            return insertNode(_rName,xNewChild);
        }

        return OConfigurationNode();
    }

    OConfigurationNode OConfigurationNode::openNode(const OUString& _rPath) const noexcept
    {
        OSL_ENSURE(m_xDirectAccess.is(), "OConfigurationNode::openNode: object is invalid!");
        OSL_ENSURE(m_xHierarchyAccess.is(), "OConfigurationNode::openNode: object is invalid!");
        try
        {
            OUString sNormalized = normalizeName(_rPath, NO_CALLER);

            Reference< XInterface > xNode;
            if (m_xDirectAccess.is() && m_xDirectAccess->hasByName(sNormalized))
            {
                xNode.set(
                    m_xDirectAccess->getByName(sNormalized), css::uno::UNO_QUERY);
                if (!xNode.is())
                    OSL_FAIL("OConfigurationNode::openNode: could not open the node!");
            }
            else if (m_xHierarchyAccess.is())
            {
                xNode.set(
                    m_xHierarchyAccess->getByHierarchicalName(_rPath),
                    css::uno::UNO_QUERY);
                if (!xNode.is())
                    OSL_FAIL("OConfigurationNode::openNode: could not open the node!");
            }
            if (xNode.is())
                return OConfigurationNode( xNode );
        }
        catch(const NoSuchElementException&)
        {
            SAL_WARN( "unotools", "OConfigurationNode::openNode: there is no element named " << _rPath );
        }
        catch(Exception&)
        {
            TOOLS_WARN_EXCEPTION( "unotools", "OConfigurationNode::openNode: caught an exception while retrieving the node!");
        }
        return OConfigurationNode();
    }

    bool OConfigurationNode::isSetNode() const
    {
        bool bIsSet = false;
        Reference< XServiceInfo > xSI(m_xHierarchyAccess, UNO_QUERY);
        if (xSI.is())
        {
            try { bIsSet = xSI->supportsService(u"com.sun.star.configuration.SetAccess"_ustr); }
            catch(Exception&) { }
        }
        return bIsSet;
    }

    bool OConfigurationNode::hasByHierarchicalName( const OUString& _rName ) const noexcept
    {
        OSL_ENSURE( m_xHierarchyAccess.is(), "OConfigurationNode::hasByHierarchicalName: no hierarchy access!" );
        try
        {
            if ( m_xHierarchyAccess.is() )
            {
                OUString sName = normalizeName( _rName, NO_CALLER );
                return m_xHierarchyAccess->hasByHierarchicalName( sName );
            }
        }
        catch(Exception&)
        {
        }
        return false;
    }

    bool OConfigurationNode::hasByName(const OUString& _rName) const noexcept
    {
        OSL_ENSURE(m_xDirectAccess.is(), "OConfigurationNode::hasByName: object is invalid!");
        try
        {
            OUString sName = normalizeName(_rName, NO_CALLER);
            if (m_xDirectAccess.is())
                return m_xDirectAccess->hasByName(sName);
        }
        catch(Exception&)
        {
        }
        return false;
    }

    bool OConfigurationNode::setNodeValue(const OUString& _rPath, const Any& _rValue) const noexcept
    {
        bool bResult = false;

        OSL_ENSURE(m_xReplaceAccess.is(), "OConfigurationNode::setNodeValue: object is invalid!");
        if (m_xReplaceAccess.is())
        {
            try
            {
                // check if _rPath is a level-1 path
                OUString sNormalizedName = normalizeName(_rPath, NO_CALLER);
                if (m_xReplaceAccess->hasByName(sNormalizedName))
                {
                    m_xReplaceAccess->replaceByName(sNormalizedName, _rValue);
                    bResult = true;
                }

                // check if the name refers to an indirect descendant
                else if (m_xHierarchyAccess.is() && m_xHierarchyAccess->hasByHierarchicalName(_rPath))
                {
                    OSL_ASSERT(!_rPath.isEmpty());

                    OUString sParentPath, sLocalName;

                    if ( splitLastFromConfigurationPath(_rPath, sParentPath, sLocalName) )
                    {
                        OConfigurationNode aParentAccess = openNode(sParentPath);
                        if (aParentAccess.isValid())
                            bResult = aParentAccess.setNodeValue(sLocalName, _rValue);
                    }
                    else
                    {
                        m_xReplaceAccess->replaceByName(sLocalName, _rValue);
                        bResult = true;
                    }
                }

            }
            catch(Exception&)
            {
                TOOLS_WARN_EXCEPTION( "unotools", "OConfigurationNode::setNodeValue: could not replace the value");
            }

        }
        return bResult;
    }

    Any OConfigurationNode::getNodeValue(const OUString& _rPath) const noexcept
    {
        OSL_ENSURE(m_xDirectAccess.is(), "OConfigurationNode::hasByName: object is invalid!");
        OSL_ENSURE(m_xHierarchyAccess.is(), "OConfigurationNode::hasByName: object is invalid!");
        Any aReturn;
        try
        {
            OUString sNormalizedPath = normalizeName(_rPath, NO_CALLER);
            if (m_xDirectAccess.is() && m_xDirectAccess->hasByName(sNormalizedPath) )
            {
                aReturn = m_xDirectAccess->getByName(sNormalizedPath);
            }
            else if (m_xHierarchyAccess.is())
            {
                aReturn = m_xHierarchyAccess->getByHierarchicalName(_rPath);
            }
        }
        catch(const NoSuchElementException&)
        {
            DBG_UNHANDLED_EXCEPTION("unotools");
        }
        return aReturn;
    }

    void OConfigurationNode::clear() noexcept
    {
        m_xHierarchyAccess.clear();
        m_xDirectAccess.clear();
        m_xReplaceAccess.clear();
        m_xContainerAccess.clear();
    }

    //= helper

    namespace
    {

        Reference< XMultiServiceFactory > lcl_getConfigProvider( const Reference<XComponentContext> & i_rContext )
        {
            try
            {
                Reference< XMultiServiceFactory > xProvider = theDefaultProvider::get( i_rContext );
                return xProvider;
            }
            catch ( const Exception& )
            {
                DBG_UNHANDLED_EXCEPTION("unotools");
            }
            return nullptr;
        }

        Reference< XInterface > lcl_createConfigurationRoot( const Reference< XMultiServiceFactory >& i_rxConfigProvider,
            const OUString& i_rNodePath, const bool i_bUpdatable, const sal_Int32 i_nDepth )
        {
            ENSURE_OR_RETURN( i_rxConfigProvider.is(), "invalid provider", nullptr );
            try
            {
                ::comphelper::NamedValueCollection aArgs;
                aArgs.put( u"nodepath"_ustr, i_rNodePath );
                aArgs.put( u"depth"_ustr, i_nDepth );

                OUString sAccessService( i_bUpdatable ?
                                u"com.sun.star.configuration.ConfigurationUpdateAccess"_ustr :
                                u"com.sun.star.configuration.ConfigurationAccess"_ustr);

                Reference< XInterface > xRoot(
                    i_rxConfigProvider->createInstanceWithArguments( sAccessService, aArgs.getWrappedPropertyValues() ),
                    UNO_SET_THROW
                );
                return xRoot;
            }
            catch ( const Exception& )
            {
                DBG_UNHANDLED_EXCEPTION("unotools");
            }
            return nullptr;
        }
    }

    OConfigurationTreeRoot::OConfigurationTreeRoot( const Reference< XInterface >& _rxRootNode )
        :OConfigurationNode( _rxRootNode )
        ,m_xCommitter( _rxRootNode, UNO_QUERY )
    {
    }

    OConfigurationTreeRoot::OConfigurationTreeRoot( const Reference<XComponentContext> & i_rContext, const OUString& i_rNodePath, const bool i_bUpdatable )
        :OConfigurationNode( lcl_createConfigurationRoot( lcl_getConfigProvider( i_rContext ),
                                i_rNodePath, i_bUpdatable, -1 ) )
        ,m_xCommitter()
    {
        if ( i_bUpdatable )
        {
            m_xCommitter.set( getUNONode(), UNO_QUERY );
            OSL_ENSURE( m_xCommitter.is(), "OConfigurationTreeRoot::OConfigurationTreeRoot: could not create an updatable node!" );
        }
    }

    void OConfigurationTreeRoot::clear() noexcept
    {
        OConfigurationNode::clear();
        m_xCommitter.clear();
    }

    bool OConfigurationTreeRoot::commit() const noexcept
    {
        OSL_ENSURE(isValid(), "OConfigurationTreeRoot::commit: object is invalid!");
        if (!isValid())
            return false;
        OSL_ENSURE(m_xCommitter.is(), "OConfigurationTreeRoot::commit: I'm a readonly node!");
        if (!m_xCommitter.is())
            return false;

        try
        {
            m_xCommitter->commitChanges();
            return true;
        }
        catch(const Exception&)
        {
            DBG_UNHANDLED_EXCEPTION("unotools");
        }
        return false;
    }

    OConfigurationTreeRoot OConfigurationTreeRoot::createWithProvider(const Reference< XMultiServiceFactory >& _rxConfProvider, const OUString& _rPath, sal_Int32 _nDepth, CREATION_MODE _eMode)
    {
        Reference< XInterface > xRoot( lcl_createConfigurationRoot(
            _rxConfProvider, _rPath, _eMode != CM_READONLY, _nDepth ) );
        if ( xRoot.is() )
            return OConfigurationTreeRoot( xRoot );
        return OConfigurationTreeRoot();
    }

    OConfigurationTreeRoot OConfigurationTreeRoot::createWithComponentContext( const Reference< XComponentContext >& _rxContext, const OUString& _rPath, sal_Int32 _nDepth, CREATION_MODE _eMode )
    {
        return createWithProvider( lcl_getConfigProvider( _rxContext ), _rPath, _nDepth, _eMode );
    }

    OConfigurationTreeRoot OConfigurationTreeRoot::tryCreateWithComponentContext( const Reference< XComponentContext >& rxContext,
        const OUString& _rPath, sal_Int32 _nDepth , CREATION_MODE _eMode )
    {
        OSL_ENSURE( rxContext.is(), "OConfigurationTreeRoot::tryCreateWithComponentContext: invalid XComponentContext!" );
        try
        {
            Reference< XMultiServiceFactory > xConfigFactory = theDefaultProvider::get( rxContext );
            return createWithProvider( xConfigFactory, _rPath, _nDepth, _eMode );
        }
        catch(const Exception&)
        {
            // silence this, 'cause the contract of this method states "no assertions"
        }
        return OConfigurationTreeRoot();
    }

}   // namespace utl

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
