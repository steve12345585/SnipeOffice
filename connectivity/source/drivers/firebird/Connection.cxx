/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
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

#include "Blob.hxx"
#include "Catalog.hxx"
#include "Clob.hxx"
#include "Connection.hxx"
#include "DatabaseMetaData.hxx"
#include "PreparedStatement.hxx"
#include "Statement.hxx"
#include "Util.hxx"

#include <stdexcept>

#include <com/sun/star/document/XDocumentEventBroadcaster.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/io/XStream.hpp>
#include <com/sun/star/lang/WrappedTargetRuntimeException.hpp>
#include <com/sun/star/sdbc/SQLException.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/sdbc/TransactionIsolation.hpp>
#include <com/sun/star/ucb/SimpleFileAccess.hpp>
#include <com/sun/star/ucb/XSimpleFileAccess2.hpp>

#include <connectivity/dbexception.hxx>
#include <strings.hrc>
#include <resource/sharedresources.hxx>

#include <comphelper/processfactory.hxx>
#include <comphelper/servicehelper.hxx>
#include <comphelper/storagehelper.hxx>
#include <cppuhelper/exc_hlp.hxx>
#include <unotools/tempfile.hxx>

#include <osl/file.hxx>
#include <rtl/strbuf.hxx>
#include <sal/log.hxx>

using namespace connectivity::firebird;
using namespace connectivity;

using namespace ::osl;

using namespace ::com::sun::star;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::document;
using namespace ::com::sun::star::embed;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::sdbcx;
using namespace ::com::sun::star::uno;

/**
 * Location within the .odb that an embedded .fdb will be stored.
 * Only relevant for embedded dbs.
 */
constexpr OUString our_sFDBLocation( u"firebird.fdb"_ustr );
/**
 * Older version of LO may store the database in a .fdb file
 */
constexpr OUString our_sFBKLocation( u"firebird.fbk"_ustr );

Connection::Connection()
    : Connection_BASE(m_aMutex)
    , m_bIsEmbedded(false)
    , m_bIsFile(false)
    , m_bIsAutoCommit(true)
    , m_bIsReadOnly(false)
    , m_aTransactionIsolation(TransactionIsolation::READ_COMMITTED)
#if SAL_TYPES_SIZEOFPOINTER == 8
    , m_aDBHandle(0)
    , m_aTransactionHandle(0)
#else
    , m_aDBHandle(nullptr)
    , m_aTransactionHandle(nullptr)
#endif
    , m_xCatalog(nullptr)
    , m_xMetaData(nullptr)
{
}

Connection::~Connection()
{
    if(!isClosed())
        close();
}

namespace {

struct ConnectionGuard
{
    oslInterlockedCount& m_refCount;
    explicit ConnectionGuard(oslInterlockedCount& refCount)
        : m_refCount(refCount)
    {
        osl_atomic_increment(&m_refCount);
    }
    ~ConnectionGuard()
    {
        osl_atomic_decrement(&m_refCount);
    }
};

}

void Connection::construct(const OUString& url, const Sequence< PropertyValue >& info)
{
    ConnectionGuard aGuard(m_refCount);

    try
    {
        m_sConnectionURL = url;

        bool bIsNewDatabase = false;
        // the database may be stored as an
        // fdb file in older versions
        bool bIsFdbStored = false;
        if (url == "sdbc:embedded:firebird")
        {
            m_bIsEmbedded = true;

            const PropertyValue* pIter = info.getConstArray();
            const PropertyValue* pEnd = pIter + info.getLength();

            for (;pIter != pEnd; ++pIter)
            {
                if ( pIter->Name == "Storage" )
                {
                    m_xEmbeddedStorage.set(pIter->Value,UNO_QUERY);
                }
                else if ( pIter->Name == "Document" )
                {
                    pIter->Value >>= m_xParentDocument;
                }
            }

            if ( !m_xEmbeddedStorage.is() )
            {
                ::connectivity::SharedResources aResources;
                const OUString sMessage = aResources.getResourceString(STR_NO_STORAGE);
                ::dbtools::throwGenericSQLException(sMessage ,*this);
            }

            bIsNewDatabase = !m_xEmbeddedStorage->hasElements();

            m_pDatabaseFileDir.reset(new ::utl::TempFileNamed(nullptr, true));
            m_pDatabaseFileDir->EnableKillingFile();
            m_sFirebirdURL = m_pDatabaseFileDir->GetFileName() + "/firebird.fdb";
            m_sFBKPath = m_pDatabaseFileDir->GetFileName() + "/firebird.fbk";

            SAL_INFO("connectivity.firebird", "Temporary .fdb location:  " << m_sFirebirdURL);

            if (!bIsNewDatabase)
            {
                if (m_xEmbeddedStorage->hasByName(our_sFBKLocation) &&
                    m_xEmbeddedStorage->isStreamElement(our_sFBKLocation))
                {
                    SAL_INFO("connectivity.firebird", "Extracting* .fbk from .odb" );
                    loadDatabaseFile(our_sFBKLocation, m_sFBKPath);
                }
                else if(m_xEmbeddedStorage->hasByName(our_sFDBLocation) &&
                        m_xEmbeddedStorage->isStreamElement(our_sFDBLocation))
                {
                    SAL_INFO("connectivity.firebird", "Found .fdb instead of .fbk");
                    bIsFdbStored = true;
                    loadDatabaseFile(our_sFDBLocation, m_sFirebirdURL);
                }
                else
                {
                    // There might be files which are not firebird databases.
                    // This is not a problem.
                    bIsNewDatabase = true;
                }
            }
            // TODO: Get DB properties from XML

        }
        // External file AND/OR remote connection
        else if (url.startsWith("sdbc:firebird:"))
        {
            m_sFirebirdURL = url.copy(strlen("sdbc:firebird:"));
            if (m_sFirebirdURL.startsWith("file://"))
            {
                m_bIsFile = true;
                uno::Reference< ucb::XSimpleFileAccess > xFileAccess =
                    ucb::SimpleFileAccess::create(comphelper::getProcessComponentContext());
                if (!xFileAccess->exists(m_sFirebirdURL))
                    bIsNewDatabase = true;

                osl::FileBase::getSystemPathFromFileURL(m_sFirebirdURL, m_sFirebirdURL);
            }
        }

        std::string dpbBuffer;
        {
            OString userName;
            OString userPassword;

            dpbBuffer.push_back(isc_dpb_version1);
            dpbBuffer.push_back(isc_dpb_sql_dialect);
            dpbBuffer.push_back(1); // 1 byte long
            dpbBuffer.push_back(SQL_DIALECT_CURRENT);

            // set UTF8 as default character set of the database
            const char sCharset[] = "UTF8";
            dpbBuffer.push_back(isc_dpb_set_db_charset);
            dpbBuffer.push_back(sizeof(sCharset) - 1);
            dpbBuffer.append(sCharset);
            // set UTF8 as default character set of the connection
            dpbBuffer.push_back(isc_dpb_lc_ctype);
            dpbBuffer.push_back(sizeof(sCharset) - 1);
            dpbBuffer.append(sCharset);

            // Do any more dpbBuffer additions here

            if (m_bIsEmbedded || m_bIsFile)
            {
                userName = "sysdba"_ostr;
                userPassword = "masterkey"_ostr;
            }
            else
            {
                for (const auto& rIter : info)
                {
                    if (rIter.Name == "user")
                    {
                        if (OUString value; rIter.Value >>= value)
                            userName = OUStringToOString(value, RTL_TEXTENCODING_UTF8);
                    }
                    else if (rIter.Name == "password")
                    {
                        if (OUString value; rIter.Value >>= value)
                            userPassword = OUStringToOString(value, RTL_TEXTENCODING_UTF8);
                    }
                }
            }

            if (!userName.isEmpty())
            {
                const sal_Int32 nMaxUsername = 255; //max size
                int nUsernameLength = std::min(userName.getLength(), nMaxUsername);
                dpbBuffer.push_back(isc_dpb_user_name);
                dpbBuffer.push_back(nUsernameLength);
                dpbBuffer.append(userName.getStr(), nUsernameLength);
            }

            if (!userPassword.isEmpty())
            {
                const sal_Int32 nMaxPassword = 255; //max size
                int nPasswordLength = std::min(userPassword.getLength(), nMaxPassword);
                dpbBuffer.push_back(isc_dpb_password);
                dpbBuffer.push_back(nPasswordLength);
                dpbBuffer.append(userPassword.getStr(), nPasswordLength);
            }
        }

        // use isc_dpb_utf8_filename to identify encoding of filenames
        dpbBuffer.push_back(isc_dpb_utf8_filename);
        dpbBuffer.push_back(0); // no filename here, it is passed to functions directly

        ISC_STATUS_ARRAY status;            /* status vector */
        ISC_STATUS aErr;
        const OString sFirebirdURL = OUStringToOString(m_sFirebirdURL, RTL_TEXTENCODING_UTF8);
        if (bIsNewDatabase)
        {
            aErr = isc_create_database(status,
                                       sFirebirdURL.getLength(),
                                       sFirebirdURL.getStr(),
                                       &m_aDBHandle,
                                       dpbBuffer.size(),
                                       dpbBuffer.c_str(),
                                       0);
            if (aErr)
            {
                evaluateStatusVector(status, u"isc_create_database", *this);
            }
        }
        else
        {
            if (m_bIsEmbedded && !bIsFdbStored) // We need to restore the .fbk first
            {
                runBackupService(isc_action_svc_restore);
            }

            aErr = isc_attach_database(status,
                                       sFirebirdURL.getLength(),
                                       sFirebirdURL.getStr(),
                                       &m_aDBHandle,
                                       dpbBuffer.size(),
                                       dpbBuffer.c_str());
            if (aErr)
            {
                evaluateStatusVector(status, u"isc_attach_database", *this);
            }
        }

        if (m_bIsEmbedded) // Add DocumentEventListener to save the .fdb as needed
        {
            // We need to attach as a document listener in order to be able to store
            // the temporary db back into the .odb when saving
            uno::Reference<XDocumentEventBroadcaster> xBroadcaster(m_xParentDocument, UNO_QUERY);

            if (xBroadcaster.is())
                xBroadcaster->addDocumentEventListener(this);
            else
                assert(false);
        }
    }
    catch (const Exception&)
    {
        throw;
    }
    catch (const std::exception&)
    {
        throw;
    }
    catch (...) // const Firebird::Exception& firebird throws this, but doesn't install the fb_exception.h that declares it

    {
        throw std::runtime_error("Generic Firebird::Exception");
    }
}

//----- XServiceInfo ---------------------------------------------------------
IMPLEMENT_SERVICE_INFO(Connection, u"com.sun.star.sdbc.drivers.firebird.Connection"_ustr,
                                                    u"com.sun.star.sdbc.Connection"_ustr)

Reference< XBlob> Connection::createBlob(ISC_QUAD const * pBlobId)
{
    MutexGuard aGuard(m_aMutex);
    checkDisposed(Connection_BASE::rBHelper.bDisposed);

    Reference< XBlob > xReturn = new Blob(&m_aDBHandle,
                                          &m_aTransactionHandle,
                                          *pBlobId);

    m_aStatements.emplace_back(xReturn);
    return xReturn;
}

Reference< XClob> Connection::createClob(ISC_QUAD const * pBlobId)
{
    MutexGuard aGuard(m_aMutex);
    checkDisposed(Connection_BASE::rBHelper.bDisposed);

    Reference< XClob > xReturn = new Clob(&m_aDBHandle,
                                          &m_aTransactionHandle,
                                          *pBlobId);

    m_aStatements.emplace_back(xReturn);
    return xReturn;
}

//----- XUnoTunnel ----------------------------------------------------------
// virtual
sal_Int64 SAL_CALL Connection::getSomething(const css::uno::Sequence<sal_Int8>& rId)
{
    return comphelper::getSomethingImpl(rId, this);
}

// static
const css::uno::Sequence<sal_Int8> & Connection::getUnoTunnelId()
{
    static const comphelper::UnoIdInit implId;
    return implId.getSeq();
}

//----- XConnection ----------------------------------------------------------
Reference< XStatement > SAL_CALL Connection::createStatement( )
{
    MutexGuard aGuard( m_aMutex );
    checkDisposed(Connection_BASE::rBHelper.bDisposed);

    // the pre
    if(m_aTypeInfo.empty())
        buildTypeInfo();

    // create a statement
    // the statement can only be executed once
    Reference< XStatement > xReturn = new OStatement(this);
    m_aStatements.emplace_back(xReturn);
    return xReturn;
}

Reference< XPreparedStatement > SAL_CALL Connection::prepareStatement(
            const OUString& _sSql)
{
    SAL_INFO("connectivity.firebird", "prepareStatement() "
             "called with sql: " << _sSql);
    MutexGuard aGuard(m_aMutex);
    checkDisposed(Connection_BASE::rBHelper.bDisposed);

    if(m_aTypeInfo.empty())
        buildTypeInfo();

    Reference< XPreparedStatement > xReturn = new OPreparedStatement(this, _sSql);
    m_aStatements.emplace_back(xReturn);

    return xReturn;
}

Reference< XPreparedStatement > SAL_CALL Connection::prepareCall(
                const OUString& _sSql )
{
    SAL_INFO("connectivity.firebird", "prepareCall(). "
             "_sSql: " << _sSql);

    MutexGuard aGuard( m_aMutex );
    checkDisposed(Connection_BASE::rBHelper.bDisposed);

    // OUString sSqlStatement (transformPreparedStatement( _sSql ));

    // not implemented yet :-) a task to do
    return nullptr;
}

OUString SAL_CALL Connection::nativeSQL( const OUString& _sSql )
{
    // We do not need to adapt the SQL for Firebird atm.
    return _sSql;
}

void SAL_CALL Connection::setAutoCommit( sal_Bool autoCommit )
{
    MutexGuard aGuard( m_aMutex );
    checkDisposed(Connection_BASE::rBHelper.bDisposed);

    m_bIsAutoCommit = autoCommit;

    if (m_aTransactionHandle)
    {
        setupTransaction();
    }
}

sal_Bool SAL_CALL Connection::getAutoCommit()
{
    MutexGuard aGuard( m_aMutex );
    checkDisposed(Connection_BASE::rBHelper.bDisposed);

    return m_bIsAutoCommit;
}

void Connection::setupTransaction()
{
    MutexGuard aGuard( m_aMutex );
    ISC_STATUS status_vector[20];

    // TODO: is this sensible? If we have changed parameters then transaction
    // is lost...
    if (m_aTransactionHandle)
    {
        disposeStatements();
        isc_rollback_transaction(status_vector, &m_aTransactionHandle);
    }

    char aTransactionIsolation = 0;
    switch (m_aTransactionIsolation)
    {
        // TODO: confirm that these are correct.
        case TransactionIsolation::READ_UNCOMMITTED:
            aTransactionIsolation = isc_tpb_concurrency;
            break;
        case TransactionIsolation::READ_COMMITTED:
            aTransactionIsolation = isc_tpb_read_committed;
            break;
        case TransactionIsolation::REPEATABLE_READ:
        case TransactionIsolation::SERIALIZABLE:
            aTransactionIsolation = isc_tpb_consistency;
            break;
        default:
            assert( false ); // We must have a valid TransactionIsolation.
    }

    // You cannot pass an empty tpb parameter so we have to do some pointer
    // arithmetic to avoid problems. (i.e. aTPB[x] = 0 is invalid)
    char aTPB[5];
    char* pTPB = aTPB;

    *pTPB++ = isc_tpb_version3;
    if (m_bIsAutoCommit)
        *pTPB++ = isc_tpb_autocommit;
    *pTPB++ = (!m_bIsReadOnly ? isc_tpb_write : isc_tpb_read);
    *pTPB++ = aTransactionIsolation;
    *pTPB++ = isc_tpb_wait;

    isc_start_transaction(status_vector,
                          &m_aTransactionHandle,
                          1,
                          &m_aDBHandle,
                          pTPB - aTPB, // bytes used in TPB
                          aTPB);

    evaluateStatusVector(status_vector,
                         u"isc_start_transaction",
                         *this);
}

isc_tr_handle& Connection::getTransaction()
{
    MutexGuard aGuard( m_aMutex );
    if (!m_aTransactionHandle)
    {
        setupTransaction();
    }
    return m_aTransactionHandle;
}

void SAL_CALL Connection::commit()
{
    MutexGuard aGuard( m_aMutex );
    checkDisposed(Connection_BASE::rBHelper.bDisposed);

    ISC_STATUS status_vector[20];

    if (!m_bIsAutoCommit && m_aTransactionHandle)
    {
        disposeStatements();
        isc_commit_transaction(status_vector, &m_aTransactionHandle);
        evaluateStatusVector(status_vector,
                             u"isc_commit_transaction",
                             *this);
    }
}

void Connection::loadDatabaseFile(const OUString& srcLocation, const OUString& tmpLocation)
{
    Reference< XStream > xDBStream(m_xEmbeddedStorage->openStreamElement(srcLocation,
            ElementModes::READ));

    uno::Reference< ucb::XSimpleFileAccess2 > xFileAccess =
        ucb::SimpleFileAccess::create( comphelper::getProcessComponentContext() );
    if ( !xFileAccess.is() )
    {
        ::connectivity::SharedResources aResources;
        // TODO FIXME: this does _not_ look like the right error message
        const OUString sMessage = aResources.getResourceString(STR_ERROR_NEW_VERSION);
        ::dbtools::throwGenericSQLException(sMessage ,*this);
    }
    xFileAccess->writeFile(tmpLocation,xDBStream->getInputStream());
}

isc_svc_handle Connection::attachServiceManager()
{
    ISC_STATUS_ARRAY aStatusVector;
#if SAL_TYPES_SIZEOFPOINTER == 8
    isc_svc_handle aServiceHandle = 0;
#else
    isc_svc_handle aServiceHandle = nullptr;
#endif

    char aSPBBuffer[256];
    char* pSPB = aSPBBuffer;
    *pSPB++ = isc_spb_version;
    *pSPB++ = isc_spb_current_version;
    *pSPB++ = isc_spb_user_name;
    OUString sUserName(u"SYSDBA"_ustr);
    char aLength = static_cast<char>(sUserName.getLength());
    *pSPB++ = aLength;
    strncpy(pSPB,
            OUStringToOString(sUserName,
                              RTL_TEXTENCODING_UTF8).getStr(),
            aLength);
    pSPB += aLength;
    // TODO: do we need ", isc_dpb_trusted_auth, 1, 1" -- probably not but ...
    if (isc_service_attach(aStatusVector,
                            0, // Denotes null-terminated string next
                            "service_mgr",
                            &aServiceHandle,
                            pSPB - aSPBBuffer,
                            aSPBBuffer))
    {
        evaluateStatusVector(aStatusVector,
                             u"isc_service_attach",
                             *this);
    }

    return aServiceHandle;
}

void Connection::detachServiceManager(isc_svc_handle aServiceHandle)
{
    ISC_STATUS_ARRAY aStatusVector;
    if (isc_service_detach(aStatusVector,
                            &aServiceHandle))
    {
        evaluateStatusVector(aStatusVector,
                             u"isc_service_detach",
                             *this);
    }
}

void Connection::runBackupService(const short nAction)
{
    assert(nAction == isc_action_svc_backup
           || nAction == isc_action_svc_restore);

    ISC_STATUS_ARRAY aStatusVector;

    // convert paths to 8-Bit strings
    OString sFDBPath = OUStringToOString(m_sFirebirdURL, RTL_TEXTENCODING_UTF8);
    OString sFBKPath = OUStringToOString(m_sFBKPath, RTL_TEXTENCODING_UTF8);


    sal_uInt16 nFDBLength = sFDBPath.getLength();
    sal_uInt16 nFBKLength = sFBKPath.getLength();
    OStringBuffer aRequest( // byte array
        OStringChar(static_cast<char>(nAction))
        + OStringChar(char(isc_spb_dbname)) // .fdb
        + OStringChar(static_cast<char>(nFDBLength & 0xFF)) // least significant byte first
        + OStringChar(static_cast<char>((nFDBLength >> 8) & 0xFF))
        + sFDBPath
        + OStringChar(char(isc_spb_bkp_file)) // .fbk
        + OStringChar(static_cast<char>(nFBKLength & 0xFF))
        + OStringChar(static_cast<char>((nFBKLength >> 8) & 0xFF))
        + sFBKPath);

    if (nAction == isc_action_svc_restore)
    {
        aRequest.append(char(isc_spb_options)); // 4-Byte bitmask
        char sOptions[4];
        char * pOptions = sOptions;
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4310) // cast truncates data
#endif
        ADD_SPB_NUMERIC(pOptions, isc_spb_res_create);
#ifdef _WIN32
#pragma warning(pop)
#endif
        aRequest.append(sOptions, 4);
    }

    isc_svc_handle aServiceHandle;
    aServiceHandle = attachServiceManager();

    if (isc_service_start(aStatusVector,
                            &aServiceHandle,
                            nullptr,
                            aRequest.getLength(),
                            aRequest.getStr()))
    {
        evaluateStatusVector(aStatusVector, u"isc_service_start", *this);
    }

    char aInfoSPB = isc_info_svc_line;
    char aResults[256];

    // query blocks until success or error
    if(isc_service_query(aStatusVector,
                      &aServiceHandle,
                      nullptr, // Reserved null
                      0,nullptr, // "send" spb -- size and spb -- not needed?
                      1,
                      &aInfoSPB,
                      sizeof(aResults),
                      aResults))
    {
        evaluateStatusVector(aStatusVector, u"isc_service_query", *this);
    }

    detachServiceManager(aServiceHandle);
}


void SAL_CALL Connection::rollback()
{
    MutexGuard aGuard( m_aMutex );
    checkDisposed(Connection_BASE::rBHelper.bDisposed);

    ISC_STATUS status_vector[20];

    if (!m_bIsAutoCommit && m_aTransactionHandle)
    {
        isc_rollback_transaction(status_vector, &m_aTransactionHandle);
    }
}

sal_Bool SAL_CALL Connection::isClosed(  )
{
    MutexGuard aGuard( m_aMutex );

    // just simple -> we are close when we are disposed that means someone called dispose(); (XComponent)
    return Connection_BASE::rBHelper.bDisposed;
}

Reference< XDatabaseMetaData > SAL_CALL Connection::getMetaData(  )
{
    MutexGuard aGuard( m_aMutex );
    checkDisposed(Connection_BASE::rBHelper.bDisposed);

    // here we have to create the class with biggest interface
    // The answer is 42 :-)
    rtl::Reference< ODatabaseMetaData > xMetaData = m_xMetaData.get();
    if(!xMetaData.is())
    {
        xMetaData = new ODatabaseMetaData(this); // need the connection because it can return it
        m_xMetaData = xMetaData.get();
    }

    return xMetaData;
}

void SAL_CALL Connection::setReadOnly(sal_Bool readOnly)
{
    MutexGuard aGuard( m_aMutex );
    checkDisposed(Connection_BASE::rBHelper.bDisposed);

    m_bIsReadOnly = readOnly;
    setupTransaction();
}

sal_Bool SAL_CALL Connection::isReadOnly()
{
    MutexGuard aGuard( m_aMutex );
    checkDisposed(Connection_BASE::rBHelper.bDisposed);

    return m_bIsReadOnly;
}

void SAL_CALL Connection::setCatalog(const OUString& /*catalog*/)
{
    ::dbtools::throwFunctionNotSupportedSQLException(u"setCatalog"_ustr, *this);
}

OUString SAL_CALL Connection::getCatalog()
{
    ::dbtools::throwFunctionNotSupportedSQLException(u"getCatalog"_ustr, *this);
}

void SAL_CALL Connection::setTransactionIsolation( sal_Int32 level )
{
    MutexGuard aGuard( m_aMutex );
    checkDisposed(Connection_BASE::rBHelper.bDisposed);

    m_aTransactionIsolation = level;
    setupTransaction();
}

sal_Int32 SAL_CALL Connection::getTransactionIsolation(  )
{
    MutexGuard aGuard( m_aMutex );
    checkDisposed(Connection_BASE::rBHelper.bDisposed);

    return m_aTransactionIsolation;
}

Reference< XNameAccess > SAL_CALL Connection::getTypeMap()
{
    ::dbtools::throwFeatureNotImplementedSQLException( u"XConnection::getTypeMap"_ustr, *this );
}

void SAL_CALL Connection::setTypeMap(const Reference< XNameAccess >&)
{
    ::dbtools::throwFeatureNotImplementedSQLException( u"XConnection::setTypeMap"_ustr, *this );
}

//----- XCloseable -----------------------------------------------------------
void SAL_CALL Connection::close(  )
{
    // we just dispose us
    {
        MutexGuard aGuard( m_aMutex );
        checkDisposed(Connection_BASE::rBHelper.bDisposed);

    }
    dispose();
}

// XWarningsSupplier
Any SAL_CALL Connection::getWarnings(  )
{
    // when you collected some warnings -> return it
    return Any();
}

void SAL_CALL Connection::clearWarnings(  )
{
    // you should clear your collected warnings here
}

// XDocumentEventListener
void SAL_CALL Connection::documentEventOccured( const DocumentEvent& Event )
{
    MutexGuard aGuard(m_aMutex);

    if (!m_bIsEmbedded)
        return;

    if (Event.EventName != "OnSave" && Event.EventName != "OnSaveAs")
        return;

    commit(); // Commit and close transaction
    if ( !(m_bIsEmbedded && m_xEmbeddedStorage.is()) )
        return;

    storeDatabase();
}

// XEventListener
void SAL_CALL Connection::disposing(const EventObject& /*rSource*/)
{
    MutexGuard aGuard( m_aMutex );

    m_xEmbeddedStorage.clear();
}

void Connection::buildTypeInfo()
{
    MutexGuard aGuard( m_aMutex );

    Reference< XResultSet> xRs = getMetaData ()->getTypeInfo ();
    Reference< XRow> xRow(xRs,UNO_QUERY);
    // Information for a single SQL type

    // Loop on the result set until we reach end of file

    while (xRs->next ())
    {
        OTypeInfo aInfo;
        aInfo.aTypeName         = xRow->getString   (1);
        aInfo.nType             = xRow->getShort    (2);
        aInfo.nPrecision        = xRow->getInt      (3);
        // aLiteralPrefix    = xRow->getString   (4);
        // aLiteralSuffix    = xRow->getString   (5);
        // aCreateParams     = xRow->getString   (6);
        // bNullable         = xRow->getBoolean  (7);
        // bCaseSensitive    = xRow->getBoolean  (8);
        // nSearchType       = xRow->getShort    (9);
        // bUnsigned         = xRow->getBoolean  (10);
        // bCurrency         = xRow->getBoolean  (11);
        // bAutoIncrement    = xRow->getBoolean  (12);
        aInfo.aLocalTypeName    = xRow->getString   (13);
        // nMinimumScale     = xRow->getShort    (14);
        aInfo.nMaximumScale     = xRow->getShort    (15);
        // nNumPrecRadix     = (sal_Int16)xRow->getInt(18);


        // Now that we have the type info, save it
        // in the Hashtable if we don't already have an
        // entry for this SQL type.

        m_aTypeInfo.push_back(aInfo);
    }

    SAL_INFO("connectivity.firebird", "buildTypeInfo(). "
             "Type info built.");

    // Close the result set/statement.

    Reference< XCloseable> xClose(xRs,UNO_QUERY);
    xClose->close();

    SAL_INFO("connectivity.firebird", "buildTypeInfo(). "
             "Closed.");
}

void Connection::disposing()
{
    MutexGuard aGuard(m_aMutex);

    disposeStatements();

    m_xMetaData.clear();

    ISC_STATUS_ARRAY status;            /* status vector */
    if (m_aTransactionHandle)
    {
        // TODO: confirm whether we need to ask the user here.
        isc_rollback_transaction(status, &m_aTransactionHandle);
    }

    if (m_aDBHandle)
    {
        if (isc_detach_database(status, &m_aDBHandle))
        {
            evaluateStatusVector(status, u"isc_detach_database", *this);
        }
    }

    storeDatabase();

    cppu::WeakComponentImplHelperBase::disposing();

    m_pDatabaseFileDir.reset();
}

void Connection::storeDatabase()
{
    MutexGuard aGuard(m_aMutex);
    if (m_bIsEmbedded && m_xEmbeddedStorage.is())
    {
        SAL_INFO("connectivity.firebird", "Writing .fbk from running db");
        try
        {
            runBackupService(isc_action_svc_backup);
        }
        catch (const SQLException& e)
        {
            auto a = cppu::getCaughtException();
            throw WrappedTargetRuntimeException(e.Message, e.Context, a);
        }
        Reference<XStream> xDBStream(
            m_xEmbeddedStorage->openStreamElement(our_sFBKLocation, ElementModes::WRITE));
        using namespace ::comphelper;
        const Reference<XComponentContext>& xContext = comphelper::getProcessComponentContext();
        Reference<XInputStream> xInputStream;
        if (!xContext.is())
            return;
        xInputStream = OStorageHelper::GetInputStreamFromURL(m_sFBKPath, xContext);
        if (xInputStream.is())
            OStorageHelper::CopyInputToOutput(xInputStream, xDBStream->getOutputStream());
    }
}


void Connection::disposeStatements()
{
    MutexGuard aGuard(m_aMutex);
    for (auto const& statement : m_aStatements)
    {
        Reference< XComponent > xComp(statement.get(), UNO_QUERY);
        if (xComp.is())
            xComp->dispose();
    }
    m_aStatements.clear();
}

uno::Reference< XTablesSupplier > Connection::createCatalog()
{
    MutexGuard aGuard(m_aMutex);

    // m_xCatalog is a weak reference. Reuse it if it still exists.
    rtl::Reference< Catalog > xCatalog = m_xCatalog.get();
    if (!xCatalog.is())
    {
        xCatalog = new Catalog(this);
        m_xCatalog = xCatalog.get();
    }
    return xCatalog;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
