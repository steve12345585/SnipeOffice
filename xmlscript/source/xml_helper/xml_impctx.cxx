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

#include <xml_import.hxx>

#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <com/sun/star/container/NoSuchElementException.hpp>
#include <com/sun/star/xml/input/XAttributes.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <sal/log.hxx>
#include <rtl/ref.hxx>

#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;

namespace xmlscript
{

const sal_Int32 UID_UNKNOWN = -1;

typedef std::unordered_map< OUString, sal_Int32 > t_OUString2LongMap;

namespace {

struct PrefixEntry
{
    ::std::vector< sal_Int32 > m_Uids;

    PrefixEntry()
        { m_Uids.reserve( 4 ); }
};

}

typedef std::unordered_map< OUString, PrefixEntry > t_OUString2PrefixMap;

namespace {

struct ElementEntry
{
    Reference< xml::input::XElement > m_xElement;
    ::std::vector< OUString > m_prefixes;

    ElementEntry()
        { m_prefixes.reserve( 2 ); }
};

class ExtendedAttributes;

class MGuard
{
    std::mutex * m_pMutex;
public:
    explicit MGuard( std::optional<std::mutex> & oMutex )
        : m_pMutex( oMutex ? &*oMutex : nullptr )
        { if (m_pMutex) m_pMutex->lock(); }
    ~MGuard() noexcept
        { if (m_pMutex) m_pMutex->unlock(); }
};

class DocumentHandlerImpl :
    public ::cppu::WeakImplHelper< xml::sax::XDocumentHandler,
                                    xml::input::XNamespaceMapping,
                                    lang::XInitialization,
                                    css::lang::XServiceInfo >
{
    friend class ExtendedAttributes;

    Reference< xml::input::XRoot > m_xRoot;

    t_OUString2LongMap m_URI2Uid;
    sal_Int32 m_uid_count;

    sal_Int32 m_nLastURI_lookup;
    OUString m_aLastURI_lookup;

    t_OUString2PrefixMap m_prefixes;
    sal_Int32 m_nLastPrefix_lookup;
    OUString m_aLastPrefix_lookup;

    std::vector< ElementEntry > m_elements;
    sal_Int32 m_nSkipElements;

    mutable std::optional<std::mutex> m_oMutex;

    inline Reference< xml::input::XElement > getCurrentElement() const;

    inline sal_Int32 getUidByURI( OUString const & rURI );
    inline sal_Int32 getUidByPrefix( OUString const & rPrefix );

    inline void pushPrefix(
        OUString const & rPrefix, OUString const & rURI );
    inline void popPrefix( OUString const & rPrefix );

    inline void getElementName(
        OUString const & rQName, sal_Int32 * pUid, OUString * pLocalName );

public:
    DocumentHandlerImpl(
        Reference< xml::input::XRoot > const & xRoot,
        bool bSingleThreadedUse );

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(
        OUString const & servicename ) override;
    virtual Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XInitialization
    virtual void SAL_CALL initialize(
        Sequence< Any > const & arguments ) override;

    // XDocumentHandler
    virtual void SAL_CALL startDocument() override;
    virtual void SAL_CALL endDocument() override;
    virtual void SAL_CALL startElement(
        OUString const & rQElementName,
        Reference< xml::sax::XAttributeList > const & xAttribs ) override;
    virtual void SAL_CALL endElement(
        OUString const & rQElementName ) override;
    virtual void SAL_CALL characters(
        OUString const & rChars ) override;
    virtual void SAL_CALL ignorableWhitespace(
        OUString const & rWhitespaces ) override;
    virtual void SAL_CALL processingInstruction(
        OUString const & rTarget, OUString const & rData ) override;
    virtual void SAL_CALL setDocumentLocator(
        Reference< xml::sax::XLocator > const & xLocator ) override;

    // XNamespaceMapping
    virtual sal_Int32 SAL_CALL getUidByUri( OUString const & Uri ) override;
    virtual OUString SAL_CALL getUriByUid( sal_Int32 Uid ) override;
};

}

constexpr OUString g_sXMLNS_PREFIX_UNKNOWN( u"<<< unknown prefix >>>"_ustr );
constexpr OUString g_sXMLNS( u"xmlns"_ustr );


DocumentHandlerImpl::DocumentHandlerImpl(
    Reference< xml::input::XRoot > const & xRoot,
    bool bSingleThreadedUse )
    : m_xRoot( xRoot ),
      m_uid_count( 0 ),
      m_nLastURI_lookup( UID_UNKNOWN ),
      m_aLastURI_lookup( u"<<< unknown URI >>>"_ustr ),
      m_nLastPrefix_lookup( UID_UNKNOWN ),
      m_aLastPrefix_lookup( u"<<< unknown URI >>>"_ustr ),
      m_nSkipElements( 0 )
{
    m_elements.reserve( 10 );

    if (! bSingleThreadedUse)
        m_oMutex.emplace();
}

inline Reference< xml::input::XElement >
DocumentHandlerImpl::getCurrentElement() const
{
    MGuard aGuard( m_oMutex );
    if (m_elements.empty())
        return Reference< xml::input::XElement >();
    else
        return m_elements.back().m_xElement;
}

inline sal_Int32 DocumentHandlerImpl::getUidByURI( OUString const & rURI )
{
    MGuard guard( m_oMutex );
    if (m_nLastURI_lookup == UID_UNKNOWN || m_aLastURI_lookup != rURI)
    {
        t_OUString2LongMap::const_iterator iFind( m_URI2Uid.find( rURI ) );
        if (iFind != m_URI2Uid.end()) // id found
        {
            m_nLastURI_lookup = iFind->second;
            m_aLastURI_lookup = rURI;
        }
        else
        {
            m_nLastURI_lookup = m_uid_count;
            ++m_uid_count;
            m_URI2Uid[ rURI ] = m_nLastURI_lookup;
            m_aLastURI_lookup = rURI;
        }
    }
    return m_nLastURI_lookup;
}

inline sal_Int32 DocumentHandlerImpl::getUidByPrefix(
    OUString const & rPrefix )
{
    // commonly the last added prefix is used often for several tags...
    // good guess
    if (m_nLastPrefix_lookup == UID_UNKNOWN || m_aLastPrefix_lookup != rPrefix)
    {
        t_OUString2PrefixMap::const_iterator iFind(
            m_prefixes.find( rPrefix ) );
        if (iFind != m_prefixes.end())
        {
            const PrefixEntry & rPrefixEntry = iFind->second;
            SAL_WARN_IF( rPrefixEntry.m_Uids.empty(), "xmlscript.xmlhelper", "rPrefixEntry.m_Uids is empty" );
            m_nLastPrefix_lookup = rPrefixEntry.m_Uids.back();
            m_aLastPrefix_lookup = rPrefix;
        }
        else
        {
            m_nLastPrefix_lookup = UID_UNKNOWN;
            m_aLastPrefix_lookup = g_sXMLNS_PREFIX_UNKNOWN;
        }
    }
    return m_nLastPrefix_lookup;
}

inline void DocumentHandlerImpl::pushPrefix(
    OUString const & rPrefix, OUString const & rURI )
{
    // lookup id for URI
    sal_Int32 nUid = getUidByURI( rURI );

    // mark prefix with id
    t_OUString2PrefixMap::iterator iFind( m_prefixes.find( rPrefix ) );
    if (iFind == m_prefixes.end()) // unused prefix
    {
        PrefixEntry aEntry;
        aEntry.m_Uids.push_back( nUid ); // latest id for prefix
        m_prefixes[rPrefix] = std::move(aEntry);
    }
    else
    {
        PrefixEntry& rEntry = iFind->second;
        SAL_WARN_IF(rEntry.m_Uids.empty(), "xmlscript.xmlhelper", "pEntry->m_Uids is empty");
        rEntry.m_Uids.push_back(nUid);
    }

    m_aLastPrefix_lookup = rPrefix;
    m_nLastPrefix_lookup = nUid;
}

inline void DocumentHandlerImpl::popPrefix(
    OUString const & rPrefix )
{
    t_OUString2PrefixMap::iterator iFind( m_prefixes.find( rPrefix ) );
    if (iFind != m_prefixes.end()) // unused prefix
    {
        PrefixEntry& rEntry = iFind->second;
        rEntry.m_Uids.pop_back(); // pop last id for prefix
        if (rEntry.m_Uids.empty()) // erase prefix key
        {
            m_prefixes.erase(iFind);
        }
    }

    m_nLastPrefix_lookup = UID_UNKNOWN;
    m_aLastPrefix_lookup = g_sXMLNS_PREFIX_UNKNOWN;
}

inline void DocumentHandlerImpl::getElementName(
    OUString const & rQName, sal_Int32 * pUid, OUString * pLocalName )
{
    sal_Int32 nColonPos = rQName.indexOf( ':' );
    *pLocalName = (nColonPos >= 0 ? rQName.copy( nColonPos +1 ) : rQName);
    *pUid = getUidByPrefix(
        nColonPos >= 0 ? rQName.copy( 0, nColonPos ) : OUString() );
}

namespace {

class ExtendedAttributes :
    public ::cppu::WeakImplHelper< xml::input::XAttributes >
{
    sal_Int32 const m_nAttributes;
    std::unique_ptr<sal_Int32[]> m_pUids;
    std::unique_ptr<OUString[]>  m_pLocalNames;
    std::unique_ptr<OUString[]>  m_pQNames;
    std::unique_ptr<OUString[]>  m_pValues;

public:
    inline ExtendedAttributes(
        sal_Int32 nAttributes,
        std::unique_ptr<sal_Int32[]> pUids,
        std::unique_ptr<OUString[]> pLocalNames,
        std::unique_ptr<OUString[]> pQNames,
        Reference< xml::sax::XAttributeList > const & xAttributeList );

    // XAttributes
    virtual sal_Int32 SAL_CALL getLength() override;
    virtual sal_Int32 SAL_CALL getIndexByQName(
        OUString const & rQName ) override;
    virtual sal_Int32 SAL_CALL getIndexByUidName(
        sal_Int32 nUid, OUString const & rLocalName ) override;
    virtual OUString SAL_CALL getQNameByIndex(
        sal_Int32 nIndex ) override;
    virtual sal_Int32 SAL_CALL getUidByIndex(
        sal_Int32 nIndex ) override;
    virtual OUString SAL_CALL getLocalNameByIndex(
        sal_Int32 nIndex ) override;
    virtual OUString SAL_CALL getValueByIndex(
        sal_Int32 nIndex ) override;
    virtual OUString SAL_CALL getValueByUidName(
        sal_Int32 nUid, OUString const & rLocalName ) override;
    virtual OUString SAL_CALL getTypeByIndex(
        sal_Int32 nIndex ) override;
};

}

inline ExtendedAttributes::ExtendedAttributes(
    sal_Int32 nAttributes,
    std::unique_ptr<sal_Int32[]> pUids,
    std::unique_ptr<OUString[]> pLocalNames, std::unique_ptr<OUString[]> pQNames,
    Reference< xml::sax::XAttributeList > const & xAttributeList )
    : m_nAttributes( nAttributes )
    , m_pUids( std::move(pUids) )
    , m_pLocalNames( std::move(pLocalNames) )
    , m_pQNames( std::move(pQNames) )
    , m_pValues( new OUString[ nAttributes ] )
{
    for ( sal_Int32 nPos = 0; nPos < nAttributes; ++nPos )
    {
        m_pValues[ nPos ] = xAttributeList->getValueByIndex( nPos );
    }
}

// XServiceInfo

OUString DocumentHandlerImpl::getImplementationName()
{
    return u"com.sun.star.comp.xml.input.SaxDocumentHandler"_ustr;
}

sal_Bool DocumentHandlerImpl::supportsService( OUString const & servicename )
{
    return cppu::supportsService(this, servicename);
}

Sequence< OUString > DocumentHandlerImpl::getSupportedServiceNames()
{
    return { u"com.sun.star.xml.input.SaxDocumentHandler"_ustr };
}

// XInitialization

void DocumentHandlerImpl::initialize(
    Sequence< Any > const & arguments )
{
    MGuard guard( m_oMutex );
    Reference< xml::input::XRoot > xRoot;
    if (arguments.getLength() != 1 ||
        !(arguments[ 0 ] >>= xRoot) ||
        !xRoot.is())
    {
        throw RuntimeException( u"missing root instance!"_ustr );
    }
    m_xRoot = std::move(xRoot);
}

// XNamespaceMapping

sal_Int32 DocumentHandlerImpl::getUidByUri( OUString const & Uri )
{
    sal_Int32 uid = getUidByURI( Uri );
    SAL_WARN_IF( uid == UID_UNKNOWN, "xmlscript.xmlhelper", "uid UNKNOWN");
    return uid;
}

OUString DocumentHandlerImpl::getUriByUid( sal_Int32 Uid )
{
    MGuard guard( m_oMutex );
    for (const auto& rURIUid : m_URI2Uid)
    {
        if (rURIUid.second == Uid)
            return rURIUid.first;
    }
    throw container::NoSuchElementException( u"no such xmlns uid!"_ustr , getXWeak() );
}

// XDocumentHandler

void DocumentHandlerImpl::startDocument()
{
    m_xRoot->startDocument( static_cast< xml::input::XNamespaceMapping * >( this ) );
}

void DocumentHandlerImpl::endDocument()
{
    m_xRoot->endDocument();
}

void DocumentHandlerImpl::startElement(
    OUString const & rQElementName,
    Reference< xml::sax::XAttributeList > const & xAttribs )
{
    Reference< xml::input::XElement > xCurrentElement;
    rtl::Reference< ExtendedAttributes > xAttributes;
    sal_Int32 nUid;
    OUString aLocalName;
    ElementEntry elementEntry;

    { // guard start:
    MGuard aGuard( m_oMutex );
    // currently skipping elements and waiting for end tags?
    if (m_nSkipElements > 0)
    {
        ++m_nSkipElements; // wait for another end tag
        SAL_INFO("xmlscript.xmlhelper", " no context given on createChildElement() => ignoring element \"" << rQElementName << "\" ...");
        return;
    }

    sal_Int16 nAttribs = xAttribs->getLength();

    // save all namespace ids
    std::unique_ptr<sal_Int32[]> pUids(new sal_Int32[ nAttribs ]);
    std::unique_ptr<OUString[]> pPrefixes(new OUString[ nAttribs ]);
    std::unique_ptr<OUString[]> pLocalNames(new OUString[ nAttribs ]);
    std::unique_ptr<OUString[]> pQNames(new OUString[ nAttribs ]);

    // first recognize all xmlns attributes
    sal_Int16 nPos;
    for ( nPos = 0; nPos < nAttribs; ++nPos )
    {
        // mark attribute to be collected further
        // on with attribute's uid and current prefix
        pUids[ nPos ] = 0; // modified

        pQNames[ nPos ] = xAttribs->getNameByIndex( nPos );
        OUString const & rQAttributeName = pQNames[ nPos ];

        if (rQAttributeName.startsWith( g_sXMLNS ))
        {
            if (rQAttributeName.getLength() == 5) // set default namespace
            {
                OUString aDefNamespacePrefix;
                pushPrefix(
                    aDefNamespacePrefix,
                    xAttribs->getValueByIndex( nPos ) );
                elementEntry.m_prefixes.push_back( aDefNamespacePrefix );
                pUids[ nPos ]          = UID_UNKNOWN;
                pPrefixes[ nPos ]      = g_sXMLNS;
                pLocalNames[ nPos ]    = aDefNamespacePrefix;
            }
            else if (':' == rQAttributeName[ 5 ]) // set prefix
            {
                OUString aPrefix( rQAttributeName.copy( 6 ) );
                pushPrefix( aPrefix, xAttribs->getValueByIndex( nPos ) );
                elementEntry.m_prefixes.push_back( aPrefix );
                pUids[ nPos ]          = UID_UNKNOWN;
                pPrefixes[ nPos ]      = g_sXMLNS;
                pLocalNames[ nPos ]    = aPrefix;
            }
            // else just a name starting with xmlns, but no prefix
        }
    }

    // now read out attribute prefixes (all namespace prefixes have been set)
    for ( nPos = 0; nPos < nAttribs; ++nPos )
    {
        if (pUids[ nPos ] >= 0) // no xmlns: attribute
        {
            OUString const & rQAttributeName = pQNames[ nPos ];
            SAL_WARN_IF(rQAttributeName.startsWith( "xmlns:" ), "xmlscript.xmlhelper", "### unexpected xmlns!" );

            // collect attribute's uid and current prefix
            sal_Int32 nColonPos = rQAttributeName.indexOf( ':' );
            if (nColonPos >= 0)
            {
                pPrefixes[ nPos ] = rQAttributeName.copy( 0, nColonPos );
                pLocalNames[ nPos ] = rQAttributeName.copy( nColonPos +1 );
            }
            else
            {
                pPrefixes[ nPos ].clear();
                pLocalNames[ nPos ] = rQAttributeName;
                // leave local names unmodified
            }
            pUids[ nPos ] = getUidByPrefix( pPrefixes[ nPos ] );
        }
    }
    pPrefixes.reset();
    // ownership of arrays belongs to attribute list
    xAttributes = new ExtendedAttributes(nAttribs, std::move(pUids), std::move(pLocalNames),
                                         std::move(pQNames), xAttribs);

    getElementName( rQElementName, &nUid, &aLocalName );

    // create new child context and append to list
    if (! m_elements.empty())
        xCurrentElement = m_elements.back().m_xElement;
    } // :guard end

    if (xCurrentElement.is())
    {
        elementEntry.m_xElement =
            xCurrentElement->startChildElement( nUid, aLocalName, xAttributes );
    }
    else
    {
        elementEntry.m_xElement =
            m_xRoot->startRootElement( nUid, aLocalName, xAttributes );
    }

    {
    MGuard aGuard( m_oMutex );
    if (elementEntry.m_xElement.is())
    {
        m_elements.push_back( std::move(elementEntry) );
    }
    else
    {
        ++m_nSkipElements;

        // pop prefixes
        for (sal_Int32 nPos = elementEntry.m_prefixes.size(); nPos--;)
            popPrefix(elementEntry.m_prefixes[nPos]);

        SAL_INFO("xmlscript.xmlhelper", " no context given on createChildElement() => ignoring element \"" << rQElementName << "\" ...");
    }
    }
}

void DocumentHandlerImpl::endElement(
    OUString const & rQElementName )
{
    Reference< xml::input::XElement > xCurrentElement;
    {
        MGuard aGuard( m_oMutex );
        if (m_nSkipElements)
        {
            --m_nSkipElements;
            SAL_INFO("xmlscript.xmlhelper", "### received endElement() for \"" << rQElementName << "\".");
            return;
        }

        // popping context
        SAL_WARN_IF( m_elements.empty(), "xmlscript.xmlhelper", "m_elements is empty" );
        ElementEntry& rEntry = m_elements.back();
        xCurrentElement = rEntry.m_xElement;

#if OSL_DEBUG_LEVEL > 0
        sal_Int32 nUid;
        OUString aLocalName;
        getElementName( rQElementName, &nUid, &aLocalName );
        SAL_WARN_IF( xCurrentElement->getLocalName() != aLocalName, "xmlscript.xmlhelper", "xCurrentElement->getLocalName() != aLocalName" );
        SAL_WARN_IF( xCurrentElement->getUid() != nUid, "xmlscript.xmlhelper", "xCurrentElement->getUid() != nUid" );
#endif

        // pop prefixes
        for ( sal_Int32 nPos = rEntry.m_prefixes.size(); nPos--; )
        {
            popPrefix( rEntry.m_prefixes[ nPos ] );
        }
        m_elements.pop_back();
    }
    xCurrentElement->endElement();
}

void DocumentHandlerImpl::characters( OUString const & rChars )
{
    Reference< xml::input::XElement > xCurrentElement( getCurrentElement() );
    if (xCurrentElement.is())
        xCurrentElement->characters( rChars );
}

void DocumentHandlerImpl::ignorableWhitespace(
    OUString const & rWhitespaces )
{
    Reference< xml::input::XElement > xCurrentElement( getCurrentElement() );
    if (xCurrentElement.is())
        xCurrentElement->ignorableWhitespace( rWhitespaces );
}

void DocumentHandlerImpl::processingInstruction(
    OUString const & rTarget, OUString const & rData )
{
    Reference< xml::input::XElement > xCurrentElement( getCurrentElement() );
    if (xCurrentElement.is())
        xCurrentElement->processingInstruction( rTarget, rData );
    else
        m_xRoot->processingInstruction( rTarget, rData );
}

void DocumentHandlerImpl::setDocumentLocator(
    Reference< xml::sax::XLocator > const & xLocator )
{
    m_xRoot->setDocumentLocator( xLocator );
}

// XAttributes

sal_Int32 ExtendedAttributes::getIndexByQName( OUString const & rQName )
{
    for ( sal_Int32 nPos = m_nAttributes; nPos--; )
    {
        if (m_pQNames[ nPos ] == rQName)
        {
            return nPos;
        }
    }
    return -1;
}

sal_Int32 ExtendedAttributes::getLength()
{
    return m_nAttributes;
}

OUString ExtendedAttributes::getLocalNameByIndex( sal_Int32 nIndex )
{
    if (nIndex < m_nAttributes)
        return m_pLocalNames[ nIndex ];
    else
        return OUString();
}

OUString ExtendedAttributes::getQNameByIndex( sal_Int32 nIndex )
{
    if (nIndex < m_nAttributes)
        return m_pQNames[ nIndex ];
    else
        return OUString();
}

OUString ExtendedAttributes::getTypeByIndex( sal_Int32 nIndex )
{
    SAL_WARN_IF( nIndex >= m_nAttributes , "xmlscript.xmlhelper", "nIndex is bigger then m_nAttributes");
    return OUString(); // unsupported
}

OUString ExtendedAttributes::getValueByIndex( sal_Int32 nIndex )
{
    if (nIndex < m_nAttributes)
        return m_pValues[ nIndex ];
    else
        return OUString();
}

sal_Int32 ExtendedAttributes::getIndexByUidName(
    sal_Int32 nUid, OUString const & rLocalName )
{
    for ( sal_Int32 nPos = m_nAttributes; nPos--; )
    {
        if (m_pUids[ nPos ] == nUid && m_pLocalNames[ nPos ] == rLocalName)
        {
            return nPos;
        }
    }
    return -1;
}

sal_Int32 ExtendedAttributes::getUidByIndex( sal_Int32 nIndex )
{
    if (nIndex < m_nAttributes)
        return m_pUids[ nIndex ];
    else
        return -1;
}

OUString ExtendedAttributes::getValueByUidName(
    sal_Int32 nUid, OUString const & rLocalName )
{
    for ( sal_Int32 nPos = m_nAttributes; nPos--; )
    {
        if (m_pUids[ nPos ] == nUid && m_pLocalNames[ nPos ] == rLocalName)
        {
            return m_pValues[ nPos ];
        }
    }
    return OUString();
}

Reference< xml::sax::XDocumentHandler > createDocumentHandler(
    Reference< xml::input::XRoot > const & xRoot )
{
    SAL_WARN_IF( !xRoot.is(), "xmlscript.xmlhelper", "xRoot is NULL" );
    if (xRoot.is())
    {
        return new DocumentHandlerImpl(xRoot, true /* mt use */);
    }
    return Reference< xml::sax::XDocumentHandler >();
}

}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_comp_xml_input_SaxDocumentHandler_get_implementation(
    css::uno::XComponentContext* , css::uno::Sequence<css::uno::Any> const& )
{
    return cppu::acquire(new xmlscript::DocumentHandlerImpl({}, false /* mt use */));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
