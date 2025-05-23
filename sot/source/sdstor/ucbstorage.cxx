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

#include <com/sun/star/io/NotConnectedException.hpp>
#include <com/sun/star/io/BufferSizeExceededException.hpp>
#include <com/sun/star/uno/RuntimeException.hpp>
#include <ucbhelper/content.hxx>
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/ucb/NameClash.hpp>
#include <unotools/tempfile.hxx>
#include <unotools/ucbstreamhelper.hxx>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/ucb/InsertCommandArgument.hpp>
#include <com/sun/star/ucb/ResultSetException.hpp>
#include <com/sun/star/uno/Sequence.h>
#include <com/sun/star/sdbc/XResultSet.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/ucb/CommandAbortedException.hpp>
#include <com/sun/star/datatransfer/DataFlavor.hpp>
#include <com/sun/star/ucb/ContentInfo.hpp>
#include <com/sun/star/ucb/ContentInfoAttribute.hpp>
#include <com/sun/star/beans/Property.hpp>
#include <com/sun/star/packages/manifest/ManifestWriter.hpp>
#include <com/sun/star/packages/manifest/ManifestReader.hpp>
#include <com/sun/star/ucb/InteractiveIOException.hpp>
#include <com/sun/star/ucb/ContentCreationException.hpp>

#include <memory>
#include <optional>
#include <o3tl/safeint.hxx>
#include <osl/diagnose.h>
#include <osl/file.hxx>
#include <sal/log.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <tools/ref.hxx>
#include <tools/debug.hxx>
#include <unotools/streamwrap.hxx>
#include <unotools/ucbhelper.hxx>
#include <tools/urlobj.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/propertyvalue.hxx>
#include <cppuhelper/implbase.hxx>
#include <ucbhelper/commandenvironment.hxx>

#include <sot/stg.hxx>
#include <sot/storinfo.hxx>
#include <sot/exchange.hxx>
#include <sot/formats.hxx>
#include <comphelper/classids.hxx>

#include <mutex>
#include <utility>
#include <vector>

namespace com::sun::star::ucb { class XCommandEnvironment; }

using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::ucb;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::sdbc;
using namespace ::ucbhelper;

#if OSL_DEBUG_LEVEL > 0
static int nOpenFiles=0;
static int nOpenStreams=0;
#endif

typedef ::cppu::WeakImplHelper < XInputStream, XSeekable > FileInputStreamWrapper_Base;

namespace {

class FileStreamWrapper_Impl : public FileInputStreamWrapper_Base, public comphelper::ByteReader
{
protected:
    std::mutex    m_aMutex;
    OUString        m_aURL;
    std::unique_ptr<SvStream> m_pSvStream;

public:
    explicit FileStreamWrapper_Impl(OUString aName);
    virtual ~FileStreamWrapper_Impl() override;

    virtual void SAL_CALL seek( sal_Int64 _nLocation ) override;
    virtual sal_Int64 SAL_CALL getPosition(  ) override;
    virtual sal_Int64 SAL_CALL getLength(  ) override;
    virtual sal_Int32 SAL_CALL readBytes( Sequence< sal_Int8 >& aData, sal_Int32 nBytesToRead) override;
    virtual sal_Int32 SAL_CALL readSomeBytes( Sequence< sal_Int8 >& aData, sal_Int32 nMaxBytesToRead) override;
    virtual void      SAL_CALL skipBytes(sal_Int32 nBytesToSkip) override;
    virtual sal_Int32 SAL_CALL available() override;
    virtual void      SAL_CALL closeInput() override;

    virtual sal_Int32 readSomeBytes(sal_Int8* aData, sal_Int32 nBytesToRead) override;

protected:
    void checkConnected();
    void checkError();
};

}

FileStreamWrapper_Impl::FileStreamWrapper_Impl( OUString aName )
    : m_aURL(std::move( aName ))
{
    // if no URL is provided the stream is empty
}


FileStreamWrapper_Impl::~FileStreamWrapper_Impl()
{
    if ( m_pSvStream )
    {
        m_pSvStream.reset();
#if OSL_DEBUG_LEVEL > 0
        --nOpenFiles;
#endif
    }

    if (!m_aURL.isEmpty())
        osl::File::remove(m_aURL);
}


sal_Int32 SAL_CALL FileStreamWrapper_Impl::readBytes(Sequence< sal_Int8 >& aData, sal_Int32 nBytesToRead)
{
    if ( m_aURL.isEmpty() )
    {
        aData.realloc( 0 );
        return 0;
    }

    checkConnected();

    if (nBytesToRead < 0)
        throw BufferSizeExceededException(OUString(), getXWeak());

    std::scoped_lock aGuard( m_aMutex );

    if (aData.getLength() < nBytesToRead)
        aData.realloc(nBytesToRead);

    sal_uInt32 nRead = m_pSvStream->ReadBytes(static_cast<void*>(aData.getArray()), nBytesToRead);
    checkError();

    // if read characters < MaxLength, adjust sequence
    if (nRead < o3tl::make_unsigned(aData.getLength()))
        aData.realloc( nRead );

    return nRead;
}

sal_Int32 FileStreamWrapper_Impl::readSomeBytes(sal_Int8* aData, sal_Int32 nBytesToRead)
{
    if ( m_aURL.isEmpty() )
        return 0;

    checkConnected();

    if (nBytesToRead < 0)
        throw BufferSizeExceededException(OUString(), getXWeak());

    std::scoped_lock aGuard( m_aMutex );

    sal_uInt32 nRead = m_pSvStream->ReadBytes(static_cast<void*>(aData), nBytesToRead);
    checkError();

    return nRead;
}

sal_Int32 SAL_CALL FileStreamWrapper_Impl::readSomeBytes(Sequence< sal_Int8 >& aData, sal_Int32 nMaxBytesToRead)
{
    if ( m_aURL.isEmpty() )
    {
        aData.realloc( 0 );
        return 0;
    }

    checkError();

    if (nMaxBytesToRead < 0)
        throw BufferSizeExceededException(OUString(), getXWeak());

    if (m_pSvStream->eof())
    {
        aData.realloc(0);
        return 0;
    }
    else
        return readBytes(aData, nMaxBytesToRead);
}


void SAL_CALL FileStreamWrapper_Impl::skipBytes(sal_Int32 nBytesToSkip)
{
    if ( m_aURL.isEmpty() )
        return;

    std::scoped_lock aGuard( m_aMutex );
    checkError();

    m_pSvStream->SeekRel(nBytesToSkip);
    checkError();
}


sal_Int32 SAL_CALL FileStreamWrapper_Impl::available()
{
    if ( m_aURL.isEmpty() )
        return 0;

    std::scoped_lock aGuard( m_aMutex );
    checkConnected();

    sal_Int64 nAvailable = m_pSvStream->remainingSize();
    checkError();

    return std::min<sal_Int64>(SAL_MAX_INT32, nAvailable);
}


void SAL_CALL FileStreamWrapper_Impl::closeInput()
{
    if ( m_aURL.isEmpty() )
        return;

    std::scoped_lock aGuard( m_aMutex );
    checkConnected();
    m_pSvStream.reset();
#if OSL_DEBUG_LEVEL > 0
    --nOpenFiles;
#endif
    osl::File::remove(m_aURL);
    m_aURL.clear();
}


void SAL_CALL FileStreamWrapper_Impl::seek( sal_Int64 _nLocation )
{
    if ( m_aURL.isEmpty() )
        return;

    std::scoped_lock aGuard( m_aMutex );
    checkConnected();

    m_pSvStream->Seek(static_cast<sal_uInt32>(_nLocation));
    checkError();
}


sal_Int64 SAL_CALL FileStreamWrapper_Impl::getPosition(  )
{
    if ( m_aURL.isEmpty() )
        return 0;

    std::scoped_lock aGuard( m_aMutex );
    checkConnected();

    sal_uInt64 nPos = m_pSvStream->Tell();
    checkError();
    return nPos;
}


sal_Int64 SAL_CALL FileStreamWrapper_Impl::getLength(  )
{
    if ( m_aURL.isEmpty() )
        return 0;

    std::scoped_lock aGuard( m_aMutex );
    checkConnected();

    checkError();

    sal_Int64 nEndPos = m_pSvStream->TellEnd();

    return nEndPos;
}


void FileStreamWrapper_Impl::checkConnected()
{
    if ( m_aURL.isEmpty() )
        throw NotConnectedException(OUString(), getXWeak());
    if ( !m_pSvStream )
    {
        m_pSvStream = ::utl::UcbStreamHelper::CreateStream( m_aURL, StreamMode::STD_READ );
#if OSL_DEBUG_LEVEL > 0
        ++nOpenFiles;
#endif
    }
}


void FileStreamWrapper_Impl::checkError()
{
    checkConnected();

    if (m_pSvStream->SvStream::GetError() != ERRCODE_NONE)
        // TODO: really evaluate the error
        throw NotConnectedException(OUString(), getXWeak());
}


#define COMMIT_RESULT_FAILURE           0
#define COMMIT_RESULT_NOTHING_TO_DO     1
#define COMMIT_RESULT_SUCCESS           2

static SotClipboardFormatId GetFormatId_Impl( const SvGlobalName& aName )
{
    if ( aName == SvGlobalName( SO3_SW_CLASSID_60 ) )
        return SotClipboardFormatId::STARWRITER_60;
    if ( aName == SvGlobalName( SO3_SWWEB_CLASSID_60 ) )
        return SotClipboardFormatId::STARWRITERWEB_60;
    if ( aName == SvGlobalName( SO3_SWGLOB_CLASSID_60 ) )
        return SotClipboardFormatId::STARWRITERGLOB_60;
    if ( aName == SvGlobalName( SO3_SDRAW_CLASSID_60 ) )
        return SotClipboardFormatId::STARDRAW_60;
    if ( aName == SvGlobalName( SO3_SIMPRESS_CLASSID_60 ) )
        return SotClipboardFormatId::STARIMPRESS_60;
    if ( aName == SvGlobalName( SO3_SC_CLASSID_60 ) )
        return SotClipboardFormatId::STARCALC_60;
    if ( aName == SvGlobalName( SO3_SCH_CLASSID_60 ) )
        return SotClipboardFormatId::STARCHART_60;
    if ( aName == SvGlobalName( SO3_SM_CLASSID_60 ) )
        return SotClipboardFormatId::STARMATH_60;
    if ( aName == SvGlobalName( SO3_OUT_CLASSID ) ||
         aName == SvGlobalName( SO3_APPLET_CLASSID ) ||
         aName == SvGlobalName( SO3_PLUGIN_CLASSID ) ||
         aName == SvGlobalName( SO3_IFRAME_CLASSID ) )
        // allowed, but not supported
        return SotClipboardFormatId::NONE;
    else
    {
        OSL_FAIL( "Unknown UCB storage format!" );
        return SotClipboardFormatId::NONE;
    }
}


static SvGlobalName GetClassId_Impl( SotClipboardFormatId nFormat )
{
    switch ( nFormat )
    {
        case SotClipboardFormatId::STARWRITER_8 :
        case SotClipboardFormatId::STARWRITER_8_TEMPLATE :
            return SvGlobalName( SO3_SW_CLASSID_60 );
        case SotClipboardFormatId::STARWRITERWEB_8 :
            return SvGlobalName( SO3_SWWEB_CLASSID_60 );
        case SotClipboardFormatId::STARWRITERGLOB_8 :
        case SotClipboardFormatId::STARWRITERGLOB_8_TEMPLATE :
            return SvGlobalName( SO3_SWGLOB_CLASSID_60 );
        case SotClipboardFormatId::STARDRAW_8 :
        case SotClipboardFormatId::STARDRAW_8_TEMPLATE :
            return SvGlobalName( SO3_SDRAW_CLASSID_60 );
        case SotClipboardFormatId::STARIMPRESS_8 :
        case SotClipboardFormatId::STARIMPRESS_8_TEMPLATE :
            return SvGlobalName( SO3_SIMPRESS_CLASSID_60 );
        case SotClipboardFormatId::STARCALC_8 :
        case SotClipboardFormatId::STARCALC_8_TEMPLATE :
            return SvGlobalName( SO3_SC_CLASSID_60 );
        case SotClipboardFormatId::STARCHART_8 :
        case SotClipboardFormatId::STARCHART_8_TEMPLATE :
            return SvGlobalName( SO3_SCH_CLASSID_60 );
        case SotClipboardFormatId::STARMATH_8 :
        case SotClipboardFormatId::STARMATH_8_TEMPLATE :
            return SvGlobalName( SO3_SM_CLASSID_60 );
        case SotClipboardFormatId::STARWRITER_60 :
            return SvGlobalName( SO3_SW_CLASSID_60 );
        case SotClipboardFormatId::STARWRITERWEB_60 :
            return SvGlobalName( SO3_SWWEB_CLASSID_60 );
        case SotClipboardFormatId::STARWRITERGLOB_60 :
            return SvGlobalName( SO3_SWGLOB_CLASSID_60 );
        case SotClipboardFormatId::STARDRAW_60 :
            return SvGlobalName( SO3_SDRAW_CLASSID_60 );
        case SotClipboardFormatId::STARIMPRESS_60 :
            return SvGlobalName( SO3_SIMPRESS_CLASSID_60 );
        case SotClipboardFormatId::STARCALC_60 :
            return SvGlobalName( SO3_SC_CLASSID_60 );
        case SotClipboardFormatId::STARCHART_60 :
            return SvGlobalName( SO3_SCH_CLASSID_60 );
        case SotClipboardFormatId::STARMATH_60 :
            return SvGlobalName( SO3_SM_CLASSID_60 );
        default :
            return SvGlobalName();
    }
}

// All storage and streams are refcounted internally; outside of this classes they are only accessible through a handle
// class, that uses the refcounted object as impl-class.

class UCBStorageStream_Impl : public SvRefBase, public SvStream
{
                                virtual ~UCBStorageStream_Impl() override;
public:

    virtual std::size_t         GetData(void* pData, std::size_t nSize) override;
    virtual std::size_t         PutData(const void* pData, std::size_t nSize) override;
    virtual sal_uInt64          SeekPos( sal_uInt64 nPos ) override;
    virtual void                SetSize( sal_uInt64 nSize ) override;
    virtual void                FlushData() override;
    virtual void                ResetError() override;

    UCBStorageStream*           m_pAntiImpl;    // only valid if an external reference exists

    OUString                    m_aOriginalName;// the original name before accessing the stream
    OUString                    m_aName;        // the actual name ( changed with a Rename command at the parent )
    OUString                    m_aURL;         // the full path name to create the content
    OUString                    m_aContentType;
    OUString                    m_aOriginalContentType;
    OString                     m_aKey;
    ::ucbhelper::Content*       m_pContent;     // the content that provides the data
    Reference<XInputStream>     m_rSource;      // the stream covering the original data of the content
    std::unique_ptr<SvStream>   m_pStream;      // the stream worked on; for readonly streams it is the original stream of the content
                                                // for read/write streams it's a copy into a temporary file
    OUString                    m_aTempURL;     // URL of this temporary stream
    ErrCode                     m_nError;
    StreamMode                  m_nMode;        // open mode ( read/write/trunc/nocreate/sharing )
    bool                        m_bSourceRead;  // Source still contains useful information
    bool                        m_bModified;    // only modified streams will be sent to the original content
    bool                        m_bCommited;    // sending the streams is coordinated by the root storage of the package
    bool                        m_bDirect;      // the storage and its streams are opened in direct mode; for UCBStorages
                                                // this means that the root storage does an autocommit when its external
                                                // reference is destroyed
    bool                        m_bIsOLEStorage;// an OLEStorage on a UCBStorageStream makes this an Autocommit-stream

                                UCBStorageStream_Impl( const OUString&, StreamMode, UCBStorageStream*, bool,
                                                       bool bRepair, Reference< XProgressHandler > const & xProgress );

    void                        Free();
    bool                        Init();
    bool                        Clear();
    sal_Int16                   Commit();       // if modified and committed: transfer an XInputStream to the content
    void                        Revert();       // discard all changes
    BaseStorage*                CreateStorage();// create an OLE Storage on the UCBStorageStream
    sal_uInt64                  GetSize();

    sal_uInt64                  ReadSourceWriteTemporary( sal_uInt64 aLength ); // read aLength from source and copy to temporary,
                                                                           // no seeking is produced
    void                        ReadSourceWriteTemporary();                // read source till the end and copy to temporary,

    void                        CopySourceToTemporary();                // same as ReadSourceWriteToTemporary()
                                                                        // but the writing is done at the end of temporary
                                                                        // pointer position is not changed
    using SvStream::SetError;
    void                        SetError( ErrCode nError );
    void                        PrepareCachedForReopen( StreamMode nMode );
};

typedef tools::SvRef<UCBStorageStream_Impl> UCBStorageStream_ImplRef;

struct UCBStorageElement_Impl;
typedef std::vector<std::unique_ptr<UCBStorageElement_Impl>> UCBStorageElementList_Impl;

class UCBStorage_Impl : public SvRefBase
{
                                virtual ~UCBStorage_Impl() override;
public:
    UCBStorage*                 m_pAntiImpl;    // only valid if external references exists

    OUString                    m_aName;        // the actual name ( changed with a Rename command at the parent )
    OUString                    m_aURL;         // the full path name to create the content
    OUString                    m_aContentType;
    OUString                    m_aOriginalContentType;
    std::optional<::ucbhelper::Content> m_oContent;     // the content that provides the storage elements
    std::unique_ptr<::utl::TempFileNamed> m_pTempFile;    // temporary file, only for storages on stream
    SvStream*                   m_pSource;      // original stream, only for storages on a stream
    ErrCode                     m_nError;
    StreamMode                  m_nMode;        // open mode ( read/write/trunc/nocreate/sharing )
    bool                        m_bCommited;    // sending the streams is coordinated by the root storage of the package
    bool                        m_bDirect;      // the storage and its streams are opened in direct mode; for UCBStorages
                                                // this means that the root storage does an autocommit when its external
                                                // reference is destroyed
    bool                        m_bIsRoot;      // marks this storage as root storages that manages all commits and reverts
    bool                        m_bIsLinked;
    bool                        m_bListCreated;
    SotClipboardFormatId        m_nFormat;
    OUString                    m_aUserTypeName;
    SvGlobalName                m_aClassId;

    UCBStorageElementList_Impl  m_aChildrenList;

    bool                        m_bRepairPackage;
    Reference< XProgressHandler > m_xProgressHandler;

                                UCBStorage_Impl( const ::ucbhelper::Content&, const OUString&, StreamMode, UCBStorage*, bool,
                                                 bool, bool = false, Reference< XProgressHandler > const & = Reference< XProgressHandler >() );
                                UCBStorage_Impl( const OUString&, StreamMode, UCBStorage*, bool, bool,
                                                 bool, Reference< XProgressHandler > const & );
                                UCBStorage_Impl( SvStream&, UCBStorage*, bool );
    void                        Init();
    sal_Int16                   Commit();
    void                        Revert();
    bool                        Insert( ::ucbhelper::Content *pContent );
    UCBStorage_Impl*            OpenStorage( UCBStorageElement_Impl* pElement, StreamMode nMode, bool bDirect );
    void                        OpenStream( UCBStorageElement_Impl*, StreamMode, bool );
    void                        SetProps( const Sequence < Sequence < PropertyValue > >& rSequence, const OUString& );
    void                        GetProps( sal_Int32&, Sequence < Sequence < PropertyValue > >& rSequence, const OUString& );
    sal_Int32                   GetObjectCount();
    void                        ReadContent();
    void                        CreateContent();
    ::ucbhelper::Content*       GetContent()
                                {
                                    if ( !m_oContent )
                                        CreateContent();
                                    return m_oContent ? &*m_oContent : nullptr;
                                }
    UCBStorageElementList_Impl& GetChildrenList()
                                {
                                    const ErrCode nError = m_nError;
                                    ReadContent();
                                    if ( m_nMode & StreamMode::WRITE )
                                    {
                                        m_nError = nError;
                                        if ( m_pAntiImpl )
                                        {
                                            m_pAntiImpl->ResetError();
                                            m_pAntiImpl->SetError( nError );
                                        }
                                    }
                                    return m_aChildrenList;
                                }

    void                        SetError( ErrCode nError );
};

typedef tools::SvRef<UCBStorage_Impl> UCBStorage_ImplRef;

// this struct contains all necessary information on an element inside a UCBStorage
struct UCBStorageElement_Impl
{
    OUString                    m_aName;        // the actual URL relative to the root "folder"
    OUString                    m_aOriginalName;// the original name in the content
    sal_uInt64                  m_nSize;
    bool                        m_bIsFolder;    // Only true when it is a UCBStorage !
    bool                        m_bIsStorage;   // Also true when it is an OLEStorage !
    bool                        m_bIsRemoved;   // element will be removed on commit
    bool                        m_bIsInserted;  // element will be removed on revert
    UCBStorage_ImplRef          m_xStorage;     // reference to the "real" storage
    UCBStorageStream_ImplRef    m_xStream;      // reference to the "real" stream

                                UCBStorageElement_Impl( const OUString& rName,
                                                        bool bIsFolder = false, sal_uInt64 nSize = 0 )
                                    : m_aName( rName )
                                    , m_aOriginalName( rName )
                                    , m_nSize( nSize )
                                    , m_bIsFolder( bIsFolder )
                                    , m_bIsStorage( bIsFolder )
                                    , m_bIsRemoved( false )
                                    , m_bIsInserted( false )
                                {
                                }

    ::ucbhelper::Content*       GetContent();
    bool                        IsModified() const;
    const OUString &            GetContentType() const;
    void                        SetContentType( const OUString& );
    const OUString &            GetOriginalContentType() const;
    bool                        IsLoaded() const
                                { return m_xStream.is() || m_xStorage.is(); }
};

::ucbhelper::Content* UCBStorageElement_Impl::GetContent()
{
    if ( m_xStream.is() )
        return m_xStream->m_pContent;
    else if ( m_xStorage.is() )
        return m_xStorage->GetContent();
    else
        return nullptr;
}

const OUString & UCBStorageElement_Impl::GetContentType() const
{
    if ( m_xStream.is() )
        return m_xStream->m_aContentType;
    else if ( m_xStorage.is() )
        return m_xStorage->m_aContentType;
    else
    {
        OSL_FAIL("Element not loaded!");
        return EMPTY_OUSTRING;
    }
}

void UCBStorageElement_Impl::SetContentType( const OUString& rType )
{
    if ( m_xStream.is() ) {
        m_xStream->m_aContentType = m_xStream->m_aOriginalContentType = rType;
    }
    else if ( m_xStorage.is() ) {
        m_xStorage->m_aContentType = m_xStorage->m_aOriginalContentType = rType;
    }
    else {
        OSL_FAIL("Element not loaded!");
    }
}

const OUString & UCBStorageElement_Impl::GetOriginalContentType() const
{
    if ( m_xStream.is() )
        return m_xStream->m_aOriginalContentType;
    else if ( m_xStorage.is() )
        return m_xStorage->m_aOriginalContentType;
    else
        return EMPTY_OUSTRING;
}

bool UCBStorageElement_Impl::IsModified() const
{
    bool bModified = m_bIsRemoved || m_bIsInserted || m_aName != m_aOriginalName;
    if ( bModified )
    {
        if ( m_xStream.is() )
            bModified = m_xStream->m_aContentType != m_xStream->m_aOriginalContentType;
        else if ( m_xStorage.is() )
            bModified = m_xStorage->m_aContentType != m_xStorage->m_aOriginalContentType;
    }

    return bModified;
}

UCBStorageStream_Impl::UCBStorageStream_Impl( const OUString& rName, StreamMode nMode, UCBStorageStream* pStream, bool bDirect, bool bRepair, Reference< XProgressHandler > const & xProgress  )
    : m_pAntiImpl( pStream )
    , m_aURL( rName )
    , m_pContent( nullptr )
    , m_nError( ERRCODE_NONE )
    , m_nMode( nMode )
    , m_bSourceRead( !( nMode & StreamMode::TRUNC ) )
    , m_bModified( false )
    , m_bCommited( false )
    , m_bDirect( bDirect )
    , m_bIsOLEStorage( false )
{
    // name is last segment in URL
    INetURLObject aObj( rName );
    m_aName = m_aOriginalName = aObj.GetLastName();
    try
    {
        // create the content
        rtl::Reference< ::ucbhelper::CommandEnvironment > xComEnv;

        OUString aTemp( rName );

        if ( bRepair )
        {
            xComEnv = new ::ucbhelper::CommandEnvironment( Reference< css::task::XInteractionHandler >(), xProgress );
            aTemp += "?repairpackage";
        }

        m_pContent = new ::ucbhelper::Content( aTemp, xComEnv, comphelper::getProcessComponentContext() );
    }
    catch (const ContentCreationException&)
    {
        // content could not be created
        SetError( SVSTREAM_CANNOT_MAKE );
    }
    catch (const RuntimeException&)
    {
        // any other error - not specified
        SetError( ERRCODE_IO_GENERAL );
    }
}

UCBStorageStream_Impl::~UCBStorageStream_Impl()
{
    if( m_rSource.is() )
        m_rSource.clear();

    m_pStream.reset();

    if (!m_aTempURL.isEmpty())
        osl::File::remove(m_aTempURL);

    delete m_pContent;
}


bool UCBStorageStream_Impl::Init()
{
    if( !m_pStream )
    {
        // no temporary stream was created
        // create one

        if ( m_aTempURL.isEmpty() )
            m_aTempURL = ::utl::CreateTempURL();

        m_pStream = ::utl::UcbStreamHelper::CreateStream( m_aTempURL, StreamMode::STD_READWRITE, true /* bFileExists */ );
#if OSL_DEBUG_LEVEL > 0
        ++nOpenFiles;
#endif

        if( !m_pStream )
        {
            OSL_FAIL( "Suspicious temporary stream creation!" );
            SetError( SVSTREAM_CANNOT_MAKE );
            return false;
        }

        SetError( m_pStream->GetError() );
    }

    if( m_bSourceRead && !m_rSource.is() )
    {
        // source file contain useful information and is not opened
        // open it from the point of noncopied data

        try
        {
            m_rSource = m_pContent->openStream();
        }
        catch (const Exception&)
        {
            // usually means that stream could not be opened
        }

        if( m_rSource.is() )
        {
            m_pStream->Seek( STREAM_SEEK_TO_END );

            try
            {
                m_rSource->skipBytes( m_pStream->Tell() );
            }
            catch (const BufferSizeExceededException&)
            {
                // the temporary stream already contain all the data
                m_bSourceRead = false;
            }
            catch (const Exception&)
            {
                // something is really wrong
                m_bSourceRead = false;
                OSL_FAIL( "Can not operate original stream!" );
                SetError( SVSTREAM_CANNOT_MAKE );
            }

            m_pStream->Seek( 0 );
        }
        else
        {
            // if the new file is edited then no source exist
            m_bSourceRead = false;
                //SetError( SVSTREAM_CANNOT_MAKE );
        }
    }

    DBG_ASSERT( m_rSource.is() || !m_bSourceRead, "Unreadable source stream!" );

    return true;
}

void UCBStorageStream_Impl::ReadSourceWriteTemporary()
{
    // read source stream till the end and copy all the data to
    // the current position of the temporary stream

    if( m_bSourceRead )
    {
        Sequence<sal_Int8> aData(32000);

        try
        {
            sal_Int32 aReaded;
            do
            {
                aReaded = m_rSource->readBytes( aData, 32000 );
                m_pStream->WriteBytes(aData.getConstArray(), aReaded);
            } while( aReaded == 32000 );
        }
        catch (const Exception &)
        {
            TOOLS_WARN_EXCEPTION("sot", "");
        }
    }

    m_bSourceRead = false;
}

sal_uInt64 UCBStorageStream_Impl::ReadSourceWriteTemporary(sal_uInt64 aLength)
{
    // read aLength byte from the source stream and copy them to the current
    // position of the temporary stream

    sal_uInt64 aResult = 0;

    if( m_bSourceRead )
    {
        Sequence<sal_Int8> aData(32000);

        try
        {

            sal_Int32 aReaded = 32000;

            for (sal_uInt64 nInd = 0; nInd < aLength && aReaded == 32000 ; nInd += 32000)
            {
                sal_Int32 aToCopy = std::min<sal_Int32>( aLength - nInd, 32000 );
                aReaded = m_rSource->readBytes( aData, aToCopy );
                aResult += m_pStream->WriteBytes(aData.getConstArray(), aReaded);
            }

            if( aResult < aLength )
                m_bSourceRead = false;
        }
        catch( const Exception & )
        {
            TOOLS_WARN_EXCEPTION("sot", "");
        }
    }

    return aResult;
}

void UCBStorageStream_Impl::CopySourceToTemporary()
{
    // current position of the temporary stream is not changed
    if( m_bSourceRead )
    {
        sal_uInt64 aPos = m_pStream->Tell();
        m_pStream->Seek( STREAM_SEEK_TO_END );
        ReadSourceWriteTemporary();
        m_pStream->Seek( aPos );
    }
}

// UCBStorageStream_Impl must have a SvStream interface, because it then can be used as underlying stream
// of an OLEStorage; so every write access caused by storage operations marks the UCBStorageStream as modified
std::size_t UCBStorageStream_Impl::GetData(void* pData, std::size_t const nSize)
{
    std::size_t aResult = 0;

    if( !Init() )
        return 0;


    // read data that is in temporary stream
    aResult = m_pStream->ReadBytes( pData, nSize );
    if( m_bSourceRead && aResult < nSize )
    {
        // read the tail of the data from original stream
        // copy this tail to the temporary stream

        std::size_t aToRead = nSize - aResult;
        pData = static_cast<void*>( static_cast<char*>(pData) + aResult );

        try
        {
            Sequence<sal_Int8> aData( aToRead );
            std::size_t aReaded = m_rSource->readBytes( aData, aToRead );
            aResult += m_pStream->WriteBytes(static_cast<const void*>(aData.getConstArray()), aReaded);
            memcpy( pData, aData.getArray(), aReaded );
        }
        catch (const Exception &)
        {
            TOOLS_WARN_EXCEPTION("sot", "");
        }

        if( aResult < nSize )
            m_bSourceRead = false;
    }

    return aResult;
}

std::size_t UCBStorageStream_Impl::PutData(const void* pData, std::size_t const nSize)
{
    if ( !(m_nMode & StreamMode::WRITE) )
    {
        SetError( ERRCODE_IO_ACCESSDENIED );
        return 0; // ?mav?
    }

    if( !nSize || !Init() )
        return 0;

    std::size_t aResult = m_pStream->WriteBytes( pData, nSize );

    m_bModified = aResult > 0;

    return aResult;

}

sal_uInt64 UCBStorageStream_Impl::SeekPos(sal_uInt64 const nPos)
{
    // check if a truncated STREAM_SEEK_TO_END was passed
    assert(nPos != SAL_MAX_UINT32);

    if( !Init() )
        return 0;

    sal_uInt64 aResult;

    if( nPos == STREAM_SEEK_TO_END )
    {
        m_pStream->Seek( STREAM_SEEK_TO_END );
        ReadSourceWriteTemporary();
        aResult = m_pStream->Tell();
    }
    else
    {
        // the problem is that even if nPos is larger the length
        // of the stream, the stream pointer will be moved to this position
        // so we have to check if temporary stream does not contain required position

        if( m_pStream->Tell() > nPos
            || m_pStream->Seek( STREAM_SEEK_TO_END ) > nPos )
        {
            // no copying is required
            aResult = m_pStream->Seek( nPos );
        }
        else
        {
            // the temp stream pointer points to the end now
            aResult = m_pStream->Tell();

            if( aResult < nPos )
            {
                if( m_bSourceRead )
                {
                    aResult += ReadSourceWriteTemporary( nPos - aResult );
                    if( aResult < nPos )
                        m_bSourceRead = false;

                    DBG_ASSERT( aResult == m_pStream->Tell(), "Error in stream arithmetic!\n" );
                }

                if( (m_nMode & StreamMode::WRITE) && !m_bSourceRead && aResult < nPos )
                {
                    // it means that all the Source stream was copied already
                    // but the required position still was not reached
                    // for writable streams it should be done
                    m_pStream->SetStreamSize( nPos );
                    aResult = m_pStream->Seek( STREAM_SEEK_TO_END );
                    DBG_ASSERT( aResult == nPos, "Error in stream arithmetic!\n" );
                }
            }
        }
    }

    return aResult;
}

void  UCBStorageStream_Impl::SetSize(sal_uInt64 const nSize)
{
    if ( !(m_nMode & StreamMode::WRITE) )
    {
        SetError( ERRCODE_IO_ACCESSDENIED );
        return;
    }

    if( !Init() )
        return;

    m_bModified = true;

    if( m_bSourceRead )
    {
        sal_uInt64 const aPos = m_pStream->Tell();
        m_pStream->Seek( STREAM_SEEK_TO_END );
        if( m_pStream->Tell() < nSize )
            ReadSourceWriteTemporary( nSize - m_pStream->Tell() );
        m_pStream->Seek( aPos );
    }

    m_pStream->SetStreamSize( nSize );
    m_bSourceRead = false;
}

void  UCBStorageStream_Impl::FlushData()
{
    if( m_pStream )
    {
        CopySourceToTemporary();
        m_pStream->Flush();
    }

    m_bCommited = true;
}

void UCBStorageStream_Impl::SetError( ErrCode nErr )
{
    if ( !m_nError )
    {
        m_nError = nErr;
        SvStream::SetError( nErr );
        if ( m_pAntiImpl ) m_pAntiImpl->SetError( nErr );
    }
}

void  UCBStorageStream_Impl::ResetError()
{
    m_nError = ERRCODE_NONE;
    SvStream::ResetError();
    if ( m_pAntiImpl )
        m_pAntiImpl->ResetError();
}

sal_uInt64 UCBStorageStream_Impl::GetSize()
{
    if( !Init() )
        return 0;

    sal_uInt64 nPos = m_pStream->Tell();
    m_pStream->Seek( STREAM_SEEK_TO_END );
    ReadSourceWriteTemporary();
    sal_uInt64 nRet = m_pStream->Tell();
    m_pStream->Seek( nPos );

    return nRet;
}

BaseStorage* UCBStorageStream_Impl::CreateStorage()
{
    // create an OLEStorage on a SvStream ( = this )
    // it gets the root attribute because otherwise it would probably not write before my root is committed
    UCBStorageStream* pNewStorageStream = new UCBStorageStream( this );
    Storage *pStorage = new Storage( *pNewStorageStream, m_bDirect );

    // GetError() call clears error code for OLE storages, must be changed in future
    const ErrCode nTmpErr = pStorage->GetError();
    pStorage->SetError( nTmpErr );

    m_bIsOLEStorage = !nTmpErr;
    return static_cast< BaseStorage* > ( pStorage );
}

sal_Int16 UCBStorageStream_Impl::Commit()
{
    // send stream to the original content
    // the  parent storage is responsible for the correct handling of deleted contents
    if ( m_bCommited || m_bIsOLEStorage || m_bDirect )
    {
        // modified streams with OLEStorages on it have autocommit; it is assumed that the OLEStorage
        // was committed as well ( if not opened in direct mode )

        if ( m_bModified )
        {
            try
            {
                CopySourceToTemporary();

                // release all stream handles
                Free();

                // the temporary file does not exist only for truncated streams
                DBG_ASSERT( !m_aTempURL.isEmpty() || ( m_nMode & StreamMode::TRUNC ), "No temporary file to read from!");
                if ( m_aTempURL.isEmpty() && !( m_nMode & StreamMode::TRUNC ) )
                    throw RuntimeException();

                InsertCommandArgument aArg;
                // create wrapper to stream that is only used while reading inside package component
                aArg.Data.set(new FileStreamWrapper_Impl(m_aTempURL));
                aArg.ReplaceExisting = true;
                m_pContent->executeCommand( u"insert"_ustr, Any(aArg) );

                // wrapper now controls lifetime of temporary file
                m_aTempURL.clear();

                INetURLObject aObj( m_aURL );
                aObj.setName( m_aName );
                m_aURL = aObj.GetMainURL( INetURLObject::DecodeMechanism::NONE );
                m_bModified = false;
                m_bSourceRead = true;
            }
            catch (const CommandAbortedException&)
            {
                // any command wasn't executed successfully - not specified
                SetError( ERRCODE_IO_GENERAL );
                return COMMIT_RESULT_FAILURE;
            }
            catch (const RuntimeException&)
            {
                // any other error - not specified
                SetError( ERRCODE_IO_GENERAL );
                return COMMIT_RESULT_FAILURE;
            }
            catch (const Exception&)
            {
                // any other error - not specified
                SetError( ERRCODE_IO_GENERAL );
                return COMMIT_RESULT_FAILURE;
            }

            m_bCommited = false;
            return COMMIT_RESULT_SUCCESS;
        }
    }

    return COMMIT_RESULT_NOTHING_TO_DO;
}

void UCBStorageStream_Impl::Revert()
{
    // if an OLEStorage is created on this stream, no "revert" is necessary because OLEStorages do nothing on "Revert" !
    if ( m_bCommited )
    {
        OSL_FAIL("Revert while commit is in progress!" );
        return;                   //  ???
    }

    Free();
    if ( !m_aTempURL.isEmpty() )
    {
        osl::File::remove(m_aTempURL);
        m_aTempURL.clear();
    }

    m_bSourceRead = false;
    try
    {
        m_rSource = m_pContent->openStream();
        if( m_rSource.is() )
        {
            if ( m_pAntiImpl && ( m_nMode & StreamMode::TRUNC ) )
                // stream is in use and should be truncated
                m_bSourceRead = false;
            else
            {
                m_nMode &= ~StreamMode::TRUNC;
                m_bSourceRead = true;
            }
        }
        else
            SetError( SVSTREAM_CANNOT_MAKE );
    }
    catch (const ContentCreationException&)
    {
        SetError( ERRCODE_IO_GENERAL );
    }
    catch (const RuntimeException&)
    {
        SetError( ERRCODE_IO_GENERAL );
    }
    catch (const Exception&)
    {
    }

    m_bModified = false;
    m_aName = m_aOriginalName;
    m_aContentType = m_aOriginalContentType;
}

bool UCBStorageStream_Impl::Clear()
{
    bool bRet = ( m_pAntiImpl == nullptr );
    DBG_ASSERT( bRet, "Removing used stream!" );
    if( bRet )
    {
        Free();
    }

    return bRet;
}

void UCBStorageStream_Impl::Free()
{
#if OSL_DEBUG_LEVEL > 0
    if ( m_pStream )
    {
        if ( !m_aTempURL.isEmpty() )
            --nOpenFiles;
        else
            --nOpenStreams;
    }
#endif

    m_rSource.clear();
    m_pStream.reset();
}

void UCBStorageStream_Impl::PrepareCachedForReopen( StreamMode nMode )
{
    bool isWritable = bool( m_nMode & StreamMode::WRITE );
    if ( isWritable )
    {
        // once stream was writable, never reset to readonly
        nMode |= StreamMode::WRITE;
    }

    m_nMode = nMode;
    Free();

    if ( nMode & StreamMode::TRUNC )
    {
        m_bSourceRead = false; // usually it should be 0 already but just in case...

        if ( !m_aTempURL.isEmpty() )
        {
            osl::File::remove(m_aTempURL);
            m_aTempURL.clear();
        }
    }
}

UCBStorageStream::UCBStorageStream( const OUString& rName, StreamMode nMode, bool bDirect, bool bRepair, Reference< XProgressHandler > const & xProgress )
{
    // pImp must be initialized in the body, because otherwise the vtable of the stream is not initialized
    // to class UCBStorageStream !
    pImp = new UCBStorageStream_Impl( rName, nMode, this, bDirect, bRepair, xProgress );
    pImp->AddFirstRef();             // use direct refcounting because in header file only a pointer should be used
    StorageBase::m_nMode = pImp->m_nMode;
}

UCBStorageStream::UCBStorageStream( UCBStorageStream_Impl *pImpl )
    : pImp( pImpl )
{
    pImp->AddFirstRef();             // use direct refcounting because in header file only a pointer should be used
    pImp->m_pAntiImpl = this;
    SetError( pImp->m_nError );
    StorageBase::m_nMode = pImp->m_nMode;
}

UCBStorageStream::~UCBStorageStream()
{
    if ( pImp->m_nMode & StreamMode::WRITE )
        pImp->Flush();
    pImp->m_pAntiImpl = nullptr;
    pImp->Free();
    pImp->ReleaseRef();
}

sal_Int32 UCBStorageStream::Read( void * pData, sal_Int32 nSize )
{
    //return pImp->m_pStream->Read( pData, nSize );
    return pImp->GetData( pData, nSize );
}

sal_Int32 UCBStorageStream::Write( const void* pData, sal_Int32 nSize )
{
    return pImp->PutData( pData, nSize );
}

sal_uInt64 UCBStorageStream::Seek( sal_uInt64 nPos )
{
    //return pImp->m_pStream->Seek( nPos );
    return pImp->Seek( nPos );
}

sal_uInt64 UCBStorageStream::Tell()
{
    if( !pImp->Init() )
        return 0;
    return pImp->m_pStream->Tell();
}

void UCBStorageStream::Flush()
{
    // streams are never really transacted, so flush also means commit !
    Commit();
}

bool UCBStorageStream::SetSize( sal_uInt64 nNewSize )
{
    pImp->SetSize( nNewSize );
    return !pImp->GetError();
}

bool UCBStorageStream::Validate( bool bWrite ) const
{
    return ( !bWrite || ( pImp->m_nMode & StreamMode::WRITE ) );
}

bool UCBStorageStream::ValidateMode( StreamMode m ) const
{
    // ???
    if( m == ( StreamMode::READ | StreamMode::TRUNC ) )  // from stg.cxx
        return true;
    if( ( m & StreamMode::READWRITE) == StreamMode::READ )
    {
        // only SHARE_DENYWRITE or SHARE_DENYALL allowed
        if( ( m & StreamMode::SHARE_DENYWRITE )
         || ( m & StreamMode::SHARE_DENYALL ) )
            return true;
    }
    else
    {
        // only SHARE_DENYALL allowed
        // storages open in r/o mode are OK, since only
        // the commit may fail
        if( m & StreamMode::SHARE_DENYALL )
            return true;
    }

    return true;
}

SvStream* UCBStorageStream::GetModifySvStream()
{
    return static_cast<SvStream*>(pImp);
}

bool  UCBStorageStream::Equals( const BaseStorageStream& rStream ) const
{
    // ???
    return static_cast<BaseStorageStream const *>(this) == &rStream;
}

bool UCBStorageStream::Commit()
{
    // mark this stream for sending it on root commit
    pImp->FlushData();
    return true;
}

void UCBStorageStream::CopyTo( BaseStorageStream* pDestStm )
{
    if( !pImp->Init() )
        return;

    UCBStorageStream* pStg =  dynamic_cast<UCBStorageStream*>( pDestStm );
    if ( pStg )
        pStg->pImp->m_aContentType = pImp->m_aContentType;

    pDestStm->SetSize( 0 );
    Seek( STREAM_SEEK_TO_END );
    sal_Int32 n = Tell();
    if( n < 0 )
        return;

    if( !pDestStm->SetSize( n ) || !n )
        return;

    std::unique_ptr<sal_uInt8[]> p(new sal_uInt8[ 4096 ]);
    Seek( 0 );
    pDestStm->Seek( 0 );
    while( n )
    {
        sal_Int32 nn = n;
        if( nn > 4096 )
            nn = 4096;
        if( Read( p.get(), nn ) != nn )
            break;
        if( pDestStm->Write( p.get(), nn ) != nn )
            break;
        n -= nn;
    }
}

bool UCBStorageStream::SetProperty( const OUString& rName, const css::uno::Any& rValue )
{
    if ( rName == "Title")
        return false;

    if ( rName == "MediaType")
    {
        OUString aTmp;
        rValue >>= aTmp;
        pImp->m_aContentType = aTmp;
    }

    try
    {
        if ( pImp->m_pContent )
        {
            pImp->m_pContent->setPropertyValue( rName, rValue );
            return true;
        }
    }
    catch (const Exception&)
    {
    }

    return false;
}

sal_uInt64 UCBStorageStream::GetSize() const
{
    return pImp->GetSize();
}

UCBStorage::UCBStorage( SvStream& rStrm, bool bDirect )
{
    // pImp must be initialized in the body, because otherwise the vtable of the stream is not initialized
    // to class UCBStorage !
    pImp = new UCBStorage_Impl( rStrm, this, bDirect );

    pImp->AddFirstRef();
    pImp->Init();
    StorageBase::m_nMode = pImp->m_nMode;
}

UCBStorage::UCBStorage( const ::ucbhelper::Content& rContent, const OUString& rName, StreamMode nMode, bool bDirect, bool bIsRoot )
{
    // pImp must be initialized in the body, because otherwise the vtable of the stream is not initialized
    // to class UCBStorage !
    pImp = new UCBStorage_Impl( rContent, rName, nMode, this, bDirect, bIsRoot );
    pImp->AddFirstRef();
    pImp->Init();
    StorageBase::m_nMode = pImp->m_nMode;
}

UCBStorage::UCBStorage( const OUString& rName, StreamMode nMode, bool bDirect, bool bIsRoot, bool bIsRepair, Reference< XProgressHandler > const & xProgressHandler )
{
    // pImp must be initialized in the body, because otherwise the vtable of the stream is not initialized
    // to class UCBStorage !
    pImp = new UCBStorage_Impl( rName, nMode, this, bDirect, bIsRoot, bIsRepair, xProgressHandler );
    pImp->AddFirstRef();
    pImp->Init();
    StorageBase::m_nMode = pImp->m_nMode;
}

UCBStorage::UCBStorage( const OUString& rName, StreamMode nMode, bool bDirect, bool bIsRoot )
{
    // pImp must be initialized in the body, because otherwise the vtable of the stream is not initialized
    // to class UCBStorage !
    pImp = new UCBStorage_Impl( rName, nMode, this, bDirect, bIsRoot, false, Reference< XProgressHandler >() );
    pImp->AddFirstRef();
    pImp->Init();
    StorageBase::m_nMode = pImp->m_nMode;
}

UCBStorage::UCBStorage( UCBStorage_Impl *pImpl )
    : pImp( pImpl )
{
    pImp->m_pAntiImpl = this;
    SetError( pImp->m_nError );
    pImp->AddFirstRef();             // use direct refcounting because in header file only a pointer should be used
    StorageBase::m_nMode = pImp->m_nMode;
}

UCBStorage::~UCBStorage()
{
    if ( pImp->m_bIsRoot && pImp->m_bDirect && ( !pImp->m_pTempFile || pImp->m_pSource ) )
        // DirectMode is simulated with an AutoCommit
        Commit();

    pImp->m_pAntiImpl = nullptr;
    pImp->ReleaseRef();
}

UCBStorage_Impl::UCBStorage_Impl( const ::ucbhelper::Content& rContent, const OUString& rName, StreamMode nMode, UCBStorage* pStorage, bool bDirect, bool bIsRoot, bool bIsRepair, Reference< XProgressHandler > const & xProgressHandler  )
    : m_pAntiImpl( pStorage )
    , m_oContent( rContent )
    , m_pSource( nullptr )
    //, m_pStream( NULL )
    , m_nError( ERRCODE_NONE )
    , m_nMode( nMode )
    , m_bCommited( false )
    , m_bDirect( bDirect )
    , m_bIsRoot( bIsRoot )
    , m_bIsLinked( true )
    , m_bListCreated( false )
    , m_nFormat( SotClipboardFormatId::NONE )
    , m_bRepairPackage( bIsRepair )
    , m_xProgressHandler( xProgressHandler )
{
    OUString aName( rName );
    if( aName.isEmpty() )
    {
        // no name given = use temporary name!
        DBG_ASSERT( m_bIsRoot, "SubStorage must have a name!" );
        m_pTempFile.reset(new ::utl::TempFileNamed);
        m_pTempFile->EnableKillingFile();
        m_aName = aName = m_pTempFile->GetURL();
    }

    m_aURL = rName;
}

UCBStorage_Impl::UCBStorage_Impl( const OUString& rName, StreamMode nMode, UCBStorage* pStorage, bool bDirect, bool bIsRoot, bool bIsRepair, Reference< XProgressHandler > const & xProgressHandler )
    : m_pAntiImpl( pStorage )
    , m_pSource( nullptr )
    //, m_pStream( NULL )
    , m_nError( ERRCODE_NONE )
    , m_nMode( nMode )
    , m_bCommited( false )
    , m_bDirect( bDirect )
    , m_bIsRoot( bIsRoot )
    , m_bIsLinked( false )
    , m_bListCreated( false )
    , m_nFormat( SotClipboardFormatId::NONE )
    , m_bRepairPackage( bIsRepair )
    , m_xProgressHandler( xProgressHandler )
{
    OUString aName( rName );
    if( aName.isEmpty() )
    {
        // no name given = use temporary name!
        DBG_ASSERT( m_bIsRoot, "SubStorage must have a name!" );
        m_pTempFile.reset(new ::utl::TempFileNamed);
        m_pTempFile->EnableKillingFile();
        m_aName = aName = m_pTempFile->GetURL();
    }

    if ( m_bIsRoot )
    {
        // create the special package URL for the package content
        m_aURL = "vnd.sun.star.pkg://" +
            INetURLObject::encode( aName, INetURLObject::PART_AUTHORITY, INetURLObject::EncodeMechanism::All );

        if ( m_nMode & StreamMode::WRITE )
        {
            // the root storage opens the package, so make sure that there is any
            ::utl::UcbStreamHelper::CreateStream( aName, StreamMode::STD_READWRITE, m_pTempFile != nullptr /* bFileExists */ );
        }
    }
    else
    {
        // substorages are opened like streams: the URL is a "child URL" of the root package URL
        m_aURL = rName;
        if ( !m_aURL.startsWith( "vnd.sun.star.pkg://") )
            m_bIsLinked = true;
    }
}

UCBStorage_Impl::UCBStorage_Impl( SvStream& rStream, UCBStorage* pStorage, bool bDirect )
    : m_pAntiImpl( pStorage )
    , m_pTempFile( new ::utl::TempFileNamed )
    , m_pSource( &rStream )
    , m_nError( ERRCODE_NONE )
    , m_bCommited( false )
    , m_bDirect( bDirect )
    , m_bIsRoot( true )
    , m_bIsLinked( false )
    , m_bListCreated( false )
    , m_nFormat( SotClipboardFormatId::NONE )
    , m_bRepairPackage( false )
{
    // opening in direct mode is too fuzzy because the data is transferred to the stream in the Commit() call,
    // which will be called in the storages' dtor
    m_pTempFile->EnableKillingFile();
    DBG_ASSERT( !bDirect, "Storage on a stream must not be opened in direct mode!" );

    // UCBStorages work on a content, so a temporary file for a content must be created, even if the stream is only
    // accessed readonly
    // the root storage opens the package; create the special package URL for the package content
    m_aURL = "vnd.sun.star.pkg://" +
        INetURLObject::encode( m_pTempFile->GetURL(), INetURLObject::PART_AUTHORITY, INetURLObject::EncodeMechanism::All );

    // copy data into the temporary file
    std::unique_ptr<SvStream> pStream(::utl::UcbStreamHelper::CreateStream( m_pTempFile->GetURL(), StreamMode::STD_READWRITE, true /* bFileExists */ ));
    if ( pStream )
    {
        rStream.Seek(0);
        rStream.ReadStream( *pStream );
        pStream->Flush();
        pStream.reset();
    }

    // close stream and let content access the file
    m_pSource->Seek(0);

    // check opening mode
    m_nMode = StreamMode::READ;
    if( rStream.IsWritable() )
        m_nMode = StreamMode::READ | StreamMode::WRITE;
}

void UCBStorage_Impl::Init()
{
    // name is last segment in URL
    INetURLObject aObj( m_aURL );
    if ( m_aName.isEmpty() )
        // if the name was not already set to a temp name
        m_aName = aObj.GetLastName();

    if ( !m_oContent )
        CreateContent();

    if ( m_oContent )
    {
        if ( m_bIsLinked )
        {
            if( m_bIsRoot )
            {
                ReadContent();
                if ( m_nError == ERRCODE_NONE )
                {
                    // read the manifest.xml file
                    aObj.Append( u"META-INF" );
                    aObj.Append( u"manifest.xml" );

                    // create input stream
                    std::unique_ptr<SvStream> pStream(::utl::UcbStreamHelper::CreateStream( aObj.GetMainURL( INetURLObject::DecodeMechanism::NONE ), StreamMode::STD_READ ));
                    // no stream means no manifest.xml
                    if ( pStream )
                    {
                        if ( !pStream->GetError() )
                        {
                            rtl::Reference<::utl::OInputStreamWrapper> pHelper = new ::utl::OInputStreamWrapper( *pStream );

                            // create a manifest reader object that will read in the manifest from the stream
                            Reference < css::packages::manifest::XManifestReader > xReader =
                                css::packages::manifest::ManifestReader::create(
                                    ::comphelper::getProcessComponentContext() ) ;
                            Sequence < Sequence < PropertyValue > > aProps = xReader->readManifestSequence( pHelper );

                            // cleanup
                            xReader = nullptr;
                            pHelper = nullptr;
                            SetProps( aProps, OUString() );
                        }
                    }
                }
            }
            else
                ReadContent();
        }
        else
        {
            // get the manifest information from the package
            try {
                Any aAny = m_oContent->getPropertyValue(u"MediaType"_ustr);
                OUString aTmp;
                if ( ( aAny >>= aTmp ) && !aTmp.isEmpty() )
                    m_aContentType = m_aOriginalContentType = aTmp;
            }
            catch (const Exception&)
            {
                SAL_WARN( "sot",
                          "getPropertyValue has thrown an exception! Please let developers know the scenario!" );
            }
        }
    }

    if ( m_aContentType.isEmpty() )
        return;

    // get the clipboard format using the content type
    css::datatransfer::DataFlavor aDataFlavor;
    aDataFlavor.MimeType = m_aContentType;
    m_nFormat = SotExchange::GetFormat( aDataFlavor );

    // get the ClassId using the clipboard format ( internal table )
    m_aClassId = GetClassId_Impl( m_nFormat );

    // get human presentable name using the clipboard format
    SotExchange::GetFormatDataFlavor( m_nFormat, aDataFlavor );
    m_aUserTypeName = aDataFlavor.HumanPresentableName;

    if( m_oContent && !m_bIsLinked && m_aClassId != SvGlobalName() )
        ReadContent();
}

void UCBStorage_Impl::CreateContent()
{
    try
    {
        // create content; where to put StreamMode ?! ( already done when opening the file of the package ? )
        rtl::Reference< ::ucbhelper::CommandEnvironment > xComEnv;

        OUString aTemp( m_aURL );

        if ( m_bRepairPackage )
        {
            xComEnv = new ::ucbhelper::CommandEnvironment( Reference< css::task::XInteractionHandler >(),
                                                     m_xProgressHandler );
            aTemp += "?repairpackage";
        }

        m_oContent.emplace( aTemp, xComEnv, comphelper::getProcessComponentContext() );
    }
    catch (const ContentCreationException&)
    {
        // content could not be created
        SetError( SVSTREAM_CANNOT_MAKE );
    }
    catch (const RuntimeException&)
    {
        // any other error - not specified
        SetError( SVSTREAM_CANNOT_MAKE );
    }
}

void UCBStorage_Impl::ReadContent()
{
    if ( m_bListCreated )
        return;

    m_bListCreated = true;

    try
    {
        GetContent();
        if ( !m_oContent )
            return;

        // create cursor for access to children
        Reference< XResultSet > xResultSet = m_oContent->createCursor( { u"Title"_ustr, u"IsFolder"_ustr, u"MediaType"_ustr, u"Size"_ustr }, ::ucbhelper::INCLUDE_FOLDERS_AND_DOCUMENTS );
        Reference< XRow > xRow( xResultSet, UNO_QUERY );
        if ( xResultSet.is() )
        {
            while ( xResultSet->next() )
            {
                // insert all into the children list
                OUString aTitle( xRow->getString(1) );
                if ( m_bIsLinked )
                {
                    // unpacked storages have to deal with the meta-inf folder by themselves
                    if ( aTitle == "META-INF" )
                        continue;
                }

                bool bIsFolder( xRow->getBoolean(2) );
                sal_Int64 nSize = xRow->getLong(4);
                UCBStorageElement_Impl* pElement = new UCBStorageElement_Impl( aTitle, bIsFolder, nSize );
                m_aChildrenList.emplace_back( pElement );

                bool bIsOfficeDocument = m_bIsLinked || ( m_aClassId != SvGlobalName() );
                if ( bIsFolder )
                {
                    if ( m_bIsLinked )
                        OpenStorage( pElement, m_nMode, m_bDirect );
                    if ( pElement->m_xStorage.is() )
                        pElement->m_xStorage->Init();
                }
                else if ( bIsOfficeDocument )
                {
                    // streams can be external OLE objects, so they are now folders, but storages!
                    OUString aName( m_aURL + "/" + xRow->getString(1));

                    rtl::Reference< ::ucbhelper::CommandEnvironment > xComEnv;
                    if ( m_bRepairPackage )
                    {
                        xComEnv = new ::ucbhelper::CommandEnvironment( Reference< css::task::XInteractionHandler >(),
                                                                m_xProgressHandler );
                        aName += "?repairpackage";
                    }

                    ::ucbhelper::Content aContent( aName, xComEnv, comphelper::getProcessComponentContext() );

                    OUString aMediaType;
                    Any aAny = aContent.getPropertyValue(u"MediaType"_ustr);
                    if ( ( aAny >>= aMediaType ) && ( aMediaType == "application/vnd.sun.star.oleobject" ) )
                        pElement->m_bIsStorage = true;
                    else if ( aMediaType.isEmpty() )
                    {
                        // older files didn't have that special content type, so they must be detected
                        OpenStream( pElement, StreamMode::STD_READ, m_bDirect );
                        if ( Storage::IsStorageFile( pElement->m_xStream.get() ) )
                            pElement->m_bIsStorage = true;
                        else
                            pElement->m_xStream->Free();
                    }
                }
            }
        }
    }
    catch (const InteractiveIOException& r)
    {
        if ( r.Code != IOErrorCode_NOT_EXISTING )
            SetError( ERRCODE_IO_GENERAL );
    }
    catch (const CommandAbortedException&)
    {
        // any command wasn't executed successfully - not specified
        if ( !( m_nMode & StreamMode::WRITE ) )
            // if the folder was just inserted and not already committed, this is not an error!
            SetError( ERRCODE_IO_GENERAL );
    }
    catch (const RuntimeException&)
    {
        // any other error - not specified
        SetError( ERRCODE_IO_GENERAL );
    }
    catch (const ResultSetException&)
    {
        // means that the package file is broken
        SetError( ERRCODE_IO_BROKENPACKAGE );
    }
    catch (const SQLException&)
    {
        // means that the file can be broken
        SetError( ERRCODE_IO_WRONGFORMAT );
    }
    catch (const Exception&)
    {
        // any other error - not specified
        SetError( ERRCODE_IO_GENERAL );
    }
}

void UCBStorage_Impl::SetError( ErrCode nError )
{
    if ( !m_nError )
    {
        m_nError = nError;
        if ( m_pAntiImpl ) m_pAntiImpl->SetError( nError );
    }
}

sal_Int32 UCBStorage_Impl::GetObjectCount()
{
    sal_Int32 nCount = m_aChildrenList.size();
    for (auto& pElement : m_aChildrenList)
    {
        DBG_ASSERT( !pElement->m_bIsFolder || pElement->m_xStorage.is(), "Storage should be open!" );
        if ( pElement->m_bIsFolder && pElement->m_xStorage.is() )
            nCount += pElement->m_xStorage->GetObjectCount();
    }

    return nCount;
}

static OUString Find_Impl( const Sequence < Sequence < PropertyValue > >& rSequence, std::u16string_view rPath )
{
    bool bFound = false;
    for ( const Sequence < PropertyValue >& rMyProps : rSequence )
    {
        OUString aType;

        for ( const PropertyValue& rAny : rMyProps )
        {
            if ( rAny.Name == "FullPath" )
            {
                OUString aTmp;
                if ( ( rAny.Value >>= aTmp ) && aTmp == rPath )
                    bFound = true;
                if ( !aType.isEmpty() )
                    break;
            }
            else if ( rAny.Name == "MediaType" )
            {
                if ( ( rAny.Value >>= aType ) && !aType.isEmpty() && bFound )
                    break;
            }
        }

        if ( bFound )
            return aType;
    }

    return OUString();
}

void UCBStorage_Impl::SetProps( const Sequence < Sequence < PropertyValue > >& rSequence, const OUString& rPath )
{
    OUString aPath( rPath );
    if ( !m_bIsRoot )
        aPath += m_aName;
    aPath += "/";

    m_aContentType = m_aOriginalContentType = Find_Impl( rSequence, aPath );

    if ( m_bIsRoot )
        // the "FullPath" of a child always starts without '/'
        aPath.clear();

    for (auto& pElement : m_aChildrenList)
    {
        DBG_ASSERT( !pElement->m_bIsFolder || pElement->m_xStorage.is(), "Storage should be open!" );
        if ( pElement->m_bIsFolder && pElement->m_xStorage.is() )
            pElement->m_xStorage->SetProps( rSequence, aPath );
        else
        {
            OUString aElementPath = aPath + pElement->m_aName;
            pElement->SetContentType( Find_Impl( rSequence, aElementPath ) );
        }
    }

    if ( m_aContentType.isEmpty() )
        return;

    // get the clipboard format using the content type
    css::datatransfer::DataFlavor aDataFlavor;
    aDataFlavor.MimeType = m_aContentType;
    m_nFormat = SotExchange::GetFormat( aDataFlavor );

    // get the ClassId using the clipboard format ( internal table )
    m_aClassId = GetClassId_Impl( m_nFormat );

    // get human presentable name using the clipboard format
    SotExchange::GetFormatDataFlavor( m_nFormat, aDataFlavor );
    m_aUserTypeName = aDataFlavor.HumanPresentableName;
}

void UCBStorage_Impl::GetProps( sal_Int32& nProps, Sequence < Sequence < PropertyValue > >& rSequence, const OUString& rPath )
{
    auto pSequence = rSequence.getArray();

    // first my own properties
    // first property is the "FullPath" name
    // it's '/' for the root storage and m_aName for each element, followed by a '/' if it's a folder
    OUString aPath( rPath );
    if ( !m_bIsRoot )
        aPath += m_aName;
    aPath += "/";
    Sequence < PropertyValue > aProps{ comphelper::makePropertyValue(u"MediaType"_ustr, m_aContentType),
                                       comphelper::makePropertyValue(u"FullPath"_ustr, aPath) };
    pSequence[nProps++] = aProps;

    if ( m_bIsRoot )
        // the "FullPath" of a child always starts without '/'
        aPath.clear();

    // now the properties of my elements
    for (auto& pElement : m_aChildrenList)
    {
        DBG_ASSERT( !pElement->m_bIsFolder || pElement->m_xStorage.is(), "Storage should be open!" );
        if ( pElement->m_bIsFolder && pElement->m_xStorage.is() )
            // storages add there properties by themselves ( see above )
            pElement->m_xStorage->GetProps( nProps, rSequence, aPath );
        else
        {
            // properties of streams
            OUString aElementPath = aPath + pElement->m_aName;
            aProps = { comphelper::makePropertyValue(u"MediaType"_ustr, pElement->GetContentType()),
                       comphelper::makePropertyValue(u"FullPath"_ustr, aElementPath) };
            pSequence[ nProps++ ] = aProps;
        }
    }
}

UCBStorage_Impl::~UCBStorage_Impl()
{
    m_aChildrenList.clear();

    m_oContent.reset();
    m_pTempFile.reset();
}

bool UCBStorage_Impl::Insert( ::ucbhelper::Content *pContent )
{
    // a new substorage is inserted into a UCBStorage ( given by the parameter pContent )
    // it must be inserted with a title and a type
    bool bRet = false;

    try
    {
        const Sequence< ContentInfo > aInfo = pContent->queryCreatableContentsInfo();
        if ( !aInfo.hasElements() )
            return false;

        for ( const ContentInfo & rCurr : aInfo )
        {
            // Simply look for the first KIND_FOLDER...
            if ( rCurr.Attributes & ContentInfoAttribute::KIND_FOLDER )
            {
                // Make sure the only required bootstrap property is "Title",
                const Sequence< Property > & rProps = rCurr.Properties;
                if ( rProps.getLength() != 1 )
                    continue;

                if ( rProps[ 0 ].Name != "Title" )
                    continue;

                Content aNewFolder;
                if ( !pContent->insertNewContent( rCurr.Type, { u"Title"_ustr }, { Any(m_aName) }, aNewFolder ) )
                    continue;

                // remove old content, create an "empty" new one and initialize it with the new inserted
                m_oContent.emplace( aNewFolder );
                bRet = true;
            }
        }
    }
    catch (const CommandAbortedException&)
    {
        // any command wasn't executed successfully - not specified
        SetError( ERRCODE_IO_GENERAL );
    }
    catch (const RuntimeException&)
    {
        // any other error - not specified
        SetError( ERRCODE_IO_GENERAL );
    }
    catch (const Exception&)
    {
        // any other error - not specified
        SetError( ERRCODE_IO_GENERAL );
    }

    return bRet;
}

sal_Int16 UCBStorage_Impl::Commit()
{
    // send all changes to the package
    sal_Int16 nRet = COMMIT_RESULT_NOTHING_TO_DO;

    // there is nothing to do if the storage has been opened readonly or if it was opened in transacted mode and no
    // commit command has been sent
    if ( ( m_nMode & StreamMode::WRITE ) && ( m_bCommited || m_bDirect ) )
    {
        try
        {
            // all errors will be caught in the "catch" statement outside the loop
            for ( size_t i = 0; i < m_aChildrenList.size() && nRet; ++i )
            {
                auto& pElement = m_aChildrenList[ i ];
                ::ucbhelper::Content* pContent = pElement->GetContent();
                std::unique_ptr< ::ucbhelper::Content > xDeleteContent;
                if ( !pContent && pElement->IsModified() )
                {
                    // if the element has never been opened, no content has been created until now
                    OUString aName = m_aURL + "/" + pElement->m_aOriginalName;
                    pContent = new ::ucbhelper::Content( aName, Reference< css::ucb::XCommandEnvironment >(), comphelper::getProcessComponentContext() );
                    xDeleteContent.reset(pContent);  // delete it later on exit scope
                }

                if ( pElement->m_bIsRemoved )
                {
                    // was it inserted, then removed (so there would be nothing to do!)
                    if ( !pElement->m_bIsInserted )
                    {
                        // first remove all open stream handles
                        if (pContent && (!pElement->m_xStream.is() || pElement->m_xStream->Clear()))
                        {
                            pContent->executeCommand( u"delete"_ustr, Any( true ) );
                            nRet = COMMIT_RESULT_SUCCESS;
                        }
                        else
                            // couldn't release stream because there are external references to it
                            nRet = COMMIT_RESULT_FAILURE;
                    }
                }
                else
                {
                    sal_Int16 nLocalRet = COMMIT_RESULT_NOTHING_TO_DO;
                    if ( pElement->m_xStorage.is() )
                    {
                        // element is a storage
                        // do a commit in the following cases:
                        //  - if storage is already inserted, and changed
                        //  - storage is not in a package
                        //  - it's a new storage, try to insert and commit if successful inserted
                        if ( !pElement->m_bIsInserted || m_bIsLinked
                             || pElement->m_xStorage->Insert( m_oContent ? &*m_oContent : nullptr ) )
                        {
                            nLocalRet = pElement->m_xStorage->Commit();
                            pContent = pElement->GetContent();
                        }
                    }
                    else if ( pElement->m_xStream.is() )
                    {
                        // element is a stream
                        nLocalRet = pElement->m_xStream->Commit();
                        if ( pElement->m_xStream->m_bIsOLEStorage )
                        {
                            // OLE storage should be stored encrypted, if the storage uses encryption
                            pElement->m_xStream->m_aContentType = "application/vnd.sun.star.oleobject";
                            Any aValue;
                            aValue <<= true;
                            pElement->m_xStream->m_pContent->setPropertyValue(u"Encrypted"_ustr, aValue );
                        }

                        pContent = pElement->GetContent();
                    }

                    if (pContent && pElement->m_aName != pElement->m_aOriginalName)
                    {
                        // name ( title ) of the element was changed
                        nLocalRet = COMMIT_RESULT_SUCCESS;
                        pContent->setPropertyValue(u"Title"_ustr, Any(pElement->m_aName) );
                    }

                    if (pContent && pElement->IsLoaded() && pElement->GetContentType() != pElement->GetOriginalContentType())
                    {
                        // mediatype of the element was changed
                        nLocalRet = COMMIT_RESULT_SUCCESS;
                        pContent->setPropertyValue(u"MediaType"_ustr, Any(pElement->GetContentType()) );
                    }

                    if ( nLocalRet != COMMIT_RESULT_NOTHING_TO_DO )
                        nRet = nLocalRet;
                }

                if ( nRet == COMMIT_RESULT_FAILURE )
                    break;
            }
        }
        catch (const ContentCreationException&)
        {
            // content could not be created
            SetError( ERRCODE_IO_NOTEXISTS );
            return COMMIT_RESULT_FAILURE;
        }
        catch (const CommandAbortedException&)
        {
            // any command wasn't executed successfully - not specified
            SetError( ERRCODE_IO_GENERAL );
            return COMMIT_RESULT_FAILURE;
        }
        catch (const RuntimeException&)
        {
            // any other error - not specified
            SetError( ERRCODE_IO_GENERAL );
            return COMMIT_RESULT_FAILURE;
        }
        catch (const Exception&)
        {
            // any other error - not specified
            SetError( ERRCODE_IO_GENERAL );
            return COMMIT_RESULT_FAILURE;
        }

        if ( m_bIsRoot && m_oContent )
        {
            // the root storage must flush the root package content
            if ( nRet == COMMIT_RESULT_SUCCESS )
            {
                try
                {
                    // commit the media type to the JAR file
                    // clipboard format and ClassId will be retrieved from the media type when the file is loaded again
                    Any aType;
                    aType <<= m_aContentType;
                    m_oContent->setPropertyValue(u"MediaType"_ustr, aType );

                    if (  m_bIsLinked )
                    {
                        // write a manifest file
                        // first create a subfolder "META-inf"
                        Content aNewSubFolder;
                        bool bRet = ::utl::UCBContentHelper::MakeFolder( *m_oContent, u"META-INF"_ustr, aNewSubFolder );
                        if ( bRet )
                        {
                            // create a stream to write the manifest file - use a temp file
                            OUString aURL( aNewSubFolder.getURL() );
                            std::optional< ::utl::TempFileNamed > pTempFile(&aURL );

                            // get the stream from the temp file and create an output stream wrapper
                            SvStream* pStream = pTempFile->GetStream( StreamMode::STD_READWRITE );
                            rtl::Reference<::utl::OOutputStreamWrapper> xOutputStream = new ::utl::OOutputStreamWrapper( *pStream );

                            // create a manifest writer object that will fill the stream
                            Reference < css::packages::manifest::XManifestWriter > xWriter =
                                css::packages::manifest::ManifestWriter::create(
                                    ::comphelper::getProcessComponentContext() );
                            sal_Int32 nCount = GetObjectCount() + 1;
                            Sequence < Sequence < PropertyValue > > aProps( nCount );
                            sal_Int32 nProps = 0;
                            GetProps( nProps, aProps, OUString() );
                            xWriter->writeManifestSequence( xOutputStream, aProps );

                            // move the stream to its desired location
                            Content aSource( pTempFile->GetURL(), Reference < XCommandEnvironment >(), comphelper::getProcessComponentContext() );
                            xWriter = nullptr;
                            xOutputStream = nullptr;
                            pTempFile.reset();
                            aNewSubFolder.transferContent( aSource, InsertOperation::Move, u"manifest.xml"_ustr, NameClash::OVERWRITE );
                        }
                    }
                    else
                    {
#if OSL_DEBUG_LEVEL > 0
                        SAL_INFO("sot", "Files: " << nOpenFiles);
                        SAL_INFO("sot", "Streams: " << nOpenStreams);
#endif
                        // force writing
                        Any aAny;
                        m_oContent->executeCommand( u"flush"_ustr, aAny );
                        if ( m_pSource != nullptr )
                        {
                            std::unique_ptr<SvStream> pStream(::utl::UcbStreamHelper::CreateStream( m_pTempFile->GetURL(), StreamMode::STD_READ ));
                            m_pSource->SetStreamSize(0);
                            // m_pSource->Seek(0);
                            pStream->ReadStream( *m_pSource );
                            pStream.reset();
                            m_pSource->Seek(0);
                        }
                    }
                }
                catch (const CommandAbortedException&)
                {
                    // how to tell the content : forget all changes ?!
                    // or should we assume that the content does it by itself because he threw an exception ?!
                    // any command wasn't executed successfully - not specified
                    SetError( ERRCODE_IO_GENERAL );
                    return COMMIT_RESULT_FAILURE;
                }
                catch (const RuntimeException&)
                {
                    // how to tell the content : forget all changes ?!
                    // or should we assume that the content does it by itself because he threw an exception ?!
                    // any other error - not specified
                    SetError( ERRCODE_IO_GENERAL );
                    return COMMIT_RESULT_FAILURE;
                }
                catch (const InteractiveIOException& r)
                {
                    if ( r.Code == IOErrorCode_ACCESS_DENIED || r.Code == IOErrorCode_LOCKING_VIOLATION )
                        SetError( ERRCODE_IO_ACCESSDENIED );
                    else if ( r.Code == IOErrorCode_NOT_EXISTING )
                        SetError( ERRCODE_IO_NOTEXISTS );
                    else if ( r.Code == IOErrorCode_CANT_READ )
                        SetError( ERRCODE_IO_CANTREAD );
                    else if ( r.Code == IOErrorCode_CANT_WRITE )
                        SetError( ERRCODE_IO_CANTWRITE );
                    else
                        SetError( ERRCODE_IO_GENERAL );

                    return COMMIT_RESULT_FAILURE;
                }
                catch (const Exception&)
                {
                    // how to tell the content : forget all changes ?!
                    // or should we assume that the content does it by itself because he threw an exception ?!
                    // any other error - not specified
                    SetError( ERRCODE_IO_GENERAL );
                    return COMMIT_RESULT_FAILURE;
                }
            }
            else if ( nRet != COMMIT_RESULT_NOTHING_TO_DO )
            {
                // how to tell the content : forget all changes ?! Should we ?!
                SetError( ERRCODE_IO_GENERAL );
                return nRet;
            }

            // after successful root commit all elements names and types are adjusted and all removed elements
            // are also removed from the lists
            for ( size_t i = 0; i < m_aChildrenList.size(); )
            {
                auto& pInnerElement = m_aChildrenList[ i ];
                if ( pInnerElement->m_bIsRemoved )
                    m_aChildrenList.erase( m_aChildrenList.begin() + i );
                else
                {
                    pInnerElement->m_aOriginalName = pInnerElement->m_aName;
                    pInnerElement->m_bIsInserted = false;
                    ++i;
                }
            }
        }

        m_bCommited = false;
    }

    return nRet;
}

void UCBStorage_Impl::Revert()
{
    for ( size_t i = 0; i < m_aChildrenList.size(); )
    {
        auto& pElement = m_aChildrenList[ i ];
        pElement->m_bIsRemoved = false;
        if ( pElement->m_bIsInserted )
            m_aChildrenList.erase( m_aChildrenList.begin() + i );
        else
        {
            if ( pElement->m_xStream.is() )
            {
                pElement->m_xStream->m_bCommited = false;
                pElement->m_xStream->Revert();
            }
            else if ( pElement->m_xStorage.is() )
            {
                pElement->m_xStorage->m_bCommited = false;
                pElement->m_xStorage->Revert();
            }

            pElement->m_aName = pElement->m_aOriginalName;
            pElement->m_bIsRemoved = false;
            ++i;
        }
    }
}

const OUString& UCBStorage::GetName() const
{
    return pImp->m_aName; // pImp->m_aURL ?!
}

bool UCBStorage::IsRoot() const
{
    return pImp->m_bIsRoot;
}

void UCBStorage::SetDirty()
{
}

void UCBStorage::SetClass( const SvGlobalName & rClass, SotClipboardFormatId nOriginalClipFormat, const OUString & rUserTypeName )
{
    pImp->m_aClassId = rClass;
    pImp->m_nFormat = nOriginalClipFormat;
    pImp->m_aUserTypeName = rUserTypeName;

    // in UCB storages only the content type will be stored, all other information can be reconstructed
    // ( see the UCBStorage_Impl::Init() method )
    css::datatransfer::DataFlavor aDataFlavor;
    SotExchange::GetFormatDataFlavor( pImp->m_nFormat, aDataFlavor );
    pImp->m_aContentType = aDataFlavor.MimeType;
}

void UCBStorage::SetClassId( const ClsId& rClsId )
{
    pImp->m_aClassId = SvGlobalName( rClsId );
    if ( pImp->m_aClassId == SvGlobalName() )
        return;

    // in OLE storages the clipboard format and the user name will be transferred when a storage is copied because both are
    // stored in one the substreams
    // UCB storages store the content type information as content type in the manifest file and so this information must be
    // kept up to date, and also the other type information that is hold only at runtime because it can be reconstructed from
    // the content type
    pImp->m_nFormat = GetFormatId_Impl( pImp->m_aClassId );
    if ( pImp->m_nFormat != SotClipboardFormatId::NONE )
    {
        css::datatransfer::DataFlavor aDataFlavor;
        SotExchange::GetFormatDataFlavor( pImp->m_nFormat, aDataFlavor );
        pImp->m_aUserTypeName = aDataFlavor.HumanPresentableName;
        pImp->m_aContentType = aDataFlavor.MimeType;
    }
}

const ClsId& UCBStorage::GetClassId() const
{
    return pImp->m_aClassId.GetCLSID();
}

SvGlobalName UCBStorage::GetClassName()
{
    return  pImp->m_aClassId;
}

SotClipboardFormatId UCBStorage::GetFormat()
{
    return pImp->m_nFormat;
}

OUString UCBStorage::GetUserName()
{
    OSL_FAIL("UserName is not implemented in UCB storages!" );
    return pImp->m_aUserTypeName;
}

void UCBStorage::FillInfoList( SvStorageInfoList* pList ) const
{
    // put information in childrenlist into StorageInfoList
    for (auto& pElement : pImp->GetChildrenList())
    {
        if ( !pElement->m_bIsRemoved )
        {
            // problem: what about the size of a substorage ?!
            sal_uInt64 nSize = pElement->m_nSize;
            if ( pElement->m_xStream.is() )
                nSize = pElement->m_xStream->GetSize();
            SvStorageInfo aInfo( pElement->m_aName, nSize, pElement->m_bIsStorage );
            pList->push_back( aInfo );
        }
    }
}

bool UCBStorage::CopyStorageElement_Impl( UCBStorageElement_Impl const & rElement, BaseStorage* pDest, const OUString& rNew ) const
{
    // insert stream or storage into the list or stream of the destination storage
    // not into the content, this will be done on commit !
    // be aware of name changes !
    if ( !rElement.m_bIsStorage )
    {
        // copy the streams data
        // the destination stream must not be open
        tools::SvRef<BaseStorageStream> pOtherStream(pDest->OpenStream( rNew, StreamMode::WRITE | StreamMode::SHARE_DENYALL, pImp->m_bDirect ));
        BaseStorageStream* pStream = nullptr;
        bool bDeleteStream = false;

        // if stream is already open, it is allowed to copy it, so be aware of this
        if ( rElement.m_xStream.is() )
            pStream = rElement.m_xStream->m_pAntiImpl;
        if ( !pStream )
        {
            pStream = const_cast< UCBStorage* >(this)->OpenStream( rElement.m_aName, StreamMode::STD_READ, pImp->m_bDirect );
            bDeleteStream = true;
        }

        pStream->CopyTo( pOtherStream.get() );
        SetError( pStream->GetError() );
        if( pOtherStream->GetError() )
            pDest->SetError( pOtherStream->GetError() );
        else
            pOtherStream->Commit();

        if ( bDeleteStream )
            delete pStream;
    }
    else
    {
        // copy the storage content
        // the destination storage must not be open
        BaseStorage* pStorage = nullptr;

        // if stream is already open, it is allowed to copy it, so be aware of this
        bool bDeleteStorage = false;
        if ( rElement.m_xStorage.is() )
            pStorage = rElement.m_xStorage->m_pAntiImpl;
        if ( !pStorage )
        {
            pStorage = const_cast<UCBStorage*>(this)->OpenStorage( rElement.m_aName, pImp->m_nMode, pImp->m_bDirect );
            bDeleteStorage = true;
        }

        UCBStorage* pUCBDest =  dynamic_cast<UCBStorage*>( pDest );
        UCBStorage* pUCBCopy =  dynamic_cast<UCBStorage*>( pStorage );

        bool bOpenUCBStorage = pUCBDest && pUCBCopy;
        tools::SvRef<BaseStorage> pOtherStorage(bOpenUCBStorage ?
                pDest->OpenUCBStorage( rNew, StreamMode::WRITE | StreamMode::SHARE_DENYALL, pImp->m_bDirect ) :
                pDest->OpenOLEStorage( rNew, StreamMode::WRITE | StreamMode::SHARE_DENYALL, pImp->m_bDirect ));

        // For UCB storages, the class id and the format id may differ,
        // do passing the class id is not sufficient.
        if( bOpenUCBStorage )
            pOtherStorage->SetClass( pStorage->GetClassName(),
                                     pStorage->GetFormat(),
                                     pUCBCopy->pImp->m_aUserTypeName );
        else
            pOtherStorage->SetClassId( pStorage->GetClassId() );
        pStorage->CopyTo( *pOtherStorage );
        SetError( pStorage->GetError() );
        if( pOtherStorage->GetError() )
            pDest->SetError( pOtherStorage->GetError() );
        else
            pOtherStorage->Commit();

        if ( bDeleteStorage )
            delete pStorage;
    }

    return Good() && pDest->Good();
}

UCBStorageElement_Impl* UCBStorage::FindElement_Impl( std::u16string_view rName ) const
{
    DBG_ASSERT( !rName.empty(), "Name is empty!" );
    for (const auto& pElement : pImp->GetChildrenList())
    {
        if ( pElement->m_aName == rName && !pElement->m_bIsRemoved )
            return pElement.get();
    }
    return nullptr;
}

bool UCBStorage::CopyTo( BaseStorage& rDestStg ) const
{
    DBG_ASSERT( &rDestStg != static_cast<BaseStorage const *>(this), "Self-Copying is not possible!" );
    if ( &rDestStg == static_cast<BaseStorage const *>(this) )
        return false;

    // perhaps it's also a problem if one storage is a parent of the other ?!
    // or if not: could be optimized ?!

    // For UCB storages, the class id and the format id may differ,
    // do passing the class id is not sufficient.
    if( dynamic_cast<const UCBStorage *>(&rDestStg) != nullptr )
        rDestStg.SetClass( pImp->m_aClassId, pImp->m_nFormat,
                           pImp->m_aUserTypeName );
    else
        rDestStg.SetClassId( GetClassId() );
    rDestStg.SetDirty();

    bool bRet = true;
    for ( size_t i = 0; i < pImp->GetChildrenList().size() && bRet; ++i )
    {
        auto& pElement = pImp->GetChildrenList()[ i ];
        if ( !pElement->m_bIsRemoved )
            bRet = CopyStorageElement_Impl( *pElement, &rDestStg, pElement->m_aName );
    }

    if( !bRet )
        SetError( rDestStg.GetError() );
    return Good() && rDestStg.Good();
}

bool UCBStorage::CopyTo( const OUString& rElemName, BaseStorage* pDest, const OUString& rNew )
{
    if( rElemName.isEmpty() )
        return false;

    if ( pDest == static_cast<BaseStorage*>(this) )
    {
        // can't double an element
        return false;
    }
    else
    {
        // for copying no optimization is useful, because in every case the stream data must be copied
        UCBStorageElement_Impl* pElement = FindElement_Impl( rElemName );
        if ( pElement )
            return CopyStorageElement_Impl( *pElement, pDest, rNew );
        else
        {
            SetError( SVSTREAM_FILE_NOT_FOUND );
            return false;
        }
    }
}

bool UCBStorage::Commit()
{
    // mark this storage for sending it on root commit
    pImp->m_bCommited = true;
    if ( pImp->m_bIsRoot )
        // the root storage coordinates committing by sending a Commit command to its content
        return ( pImp->Commit() != COMMIT_RESULT_FAILURE );
    else
        return true;
}

bool UCBStorage::Revert()
{
    pImp->Revert();
    return true;
}

BaseStorageStream* UCBStorage::OpenStream( const OUString& rEleName, StreamMode nMode, bool bDirect )
{
    if( rEleName.isEmpty() )
        return nullptr;

    // try to find the storage element
    UCBStorageElement_Impl *pElement = FindElement_Impl( rEleName );
    if ( !pElement )
    {
        // element does not exist, check if creation is allowed
        if( nMode & StreamMode::NOCREATE )
        {
            SetError( ( nMode & StreamMode::WRITE ) ? SVSTREAM_CANNOT_MAKE : SVSTREAM_FILE_NOT_FOUND );
            OUString aName = pImp->m_aURL + "/" + rEleName;
            UCBStorageStream* pStream = new UCBStorageStream( aName, nMode, bDirect, pImp->m_bRepairPackage, pImp->m_xProgressHandler );
            pStream->SetError( GetError() );
            pStream->pImp->m_aName = rEleName;
            return pStream;
        }
        else
        {
            // create a new UCBStorageElement and insert it into the list
            pElement = new UCBStorageElement_Impl( rEleName );
            pElement->m_bIsInserted = true;
            pImp->m_aChildrenList.emplace_back( pElement );
        }
    }

    if ( !pElement->m_bIsFolder )
    {
        // check if stream is already created
        if ( pElement->m_xStream.is() )
        {
            // stream has already been created; if it has no external reference, it may be opened another time
            if ( pElement->m_xStream->m_pAntiImpl )
            {
                OSL_FAIL("Stream is already open!" );
                SetError( SVSTREAM_ACCESS_DENIED );  // ???
                return nullptr;
            }
            else
            {
                // check if stream is opened with the same keyword as before
                // if not, generate a new stream because it could be encrypted vs. decrypted!
                if ( pElement->m_xStream->m_aKey.isEmpty() )
                {
                    pElement->m_xStream->PrepareCachedForReopen( nMode );

                    return new UCBStorageStream( pElement->m_xStream.get() );
                }
            }
        }

        // stream is opened the first time
        pImp->OpenStream( pElement, nMode, bDirect );

        // if name has been changed before creating the stream: set name!
        pElement->m_xStream->m_aName = rEleName;
        return new UCBStorageStream( pElement->m_xStream.get() );
    }

    return nullptr;
}

void UCBStorage_Impl::OpenStream( UCBStorageElement_Impl* pElement, StreamMode nMode, bool bDirect )
{
    OUString aName = m_aURL + "/" +pElement->m_aOriginalName;
    pElement->m_xStream = new UCBStorageStream_Impl( aName, nMode, nullptr, bDirect, m_bRepairPackage, m_xProgressHandler );
}

BaseStorage* UCBStorage::OpenUCBStorage( const OUString& rEleName, StreamMode nMode, bool bDirect )
{
    if( rEleName.isEmpty() )
        return nullptr;

    return OpenStorage_Impl( rEleName, nMode, bDirect, true );
}

BaseStorage* UCBStorage::OpenOLEStorage( const OUString& rEleName, StreamMode nMode, bool bDirect )
{
    if( rEleName.isEmpty() )
        return nullptr;

    return OpenStorage_Impl( rEleName, nMode, bDirect, false );
}

BaseStorage* UCBStorage::OpenStorage( const OUString& rEleName, StreamMode nMode, bool bDirect )
{
    if( rEleName.isEmpty() )
        return nullptr;

    return OpenStorage_Impl( rEleName, nMode, bDirect, true );
}

BaseStorage* UCBStorage::OpenStorage_Impl( const OUString& rEleName, StreamMode nMode, bool bDirect, bool bForceUCBStorage )
{
    // try to find the storage element
    UCBStorageElement_Impl *pElement = FindElement_Impl( rEleName );
    if ( !pElement )
    {
        // element does not exist, check if creation is allowed
        if( nMode & StreamMode::NOCREATE )
        {
            SetError( ( nMode & StreamMode::WRITE ) ? SVSTREAM_CANNOT_MAKE : SVSTREAM_FILE_NOT_FOUND );
            OUString aName = pImp->m_aURL + "/" + rEleName;  //  ???
            UCBStorage *pStorage = new UCBStorage( aName, nMode, bDirect, false, pImp->m_bRepairPackage, pImp->m_xProgressHandler );
            pStorage->pImp->m_bIsRoot = false;
            pStorage->pImp->m_bListCreated = true; // the storage is pretty new, nothing to read
            pStorage->SetError( GetError() );
            return pStorage;
        }

        // create a new UCBStorageElement and insert it into the list
        // problem: perhaps an OLEStorage should be created ?!
        // Because nothing is known about the element that should be created, an external parameter is needed !
        pElement = new UCBStorageElement_Impl( rEleName );
        pElement->m_bIsInserted = true;
        pImp->m_aChildrenList.emplace_back( pElement );
    }

    if ( !pElement->m_bIsFolder && ( pElement->m_bIsStorage || !bForceUCBStorage ) )
    {
        // create OLE storages on a stream ( see ctor of SotStorage )
        // Such a storage will be created on a UCBStorageStream; it will write into the stream
        // if it is opened in direct mode or when it is committed. In this case the stream will be
        // modified and then it MUST be treated as committed.
        if ( !pElement->m_xStream.is() )
        {
            BaseStorageStream* pStr = OpenStream( rEleName, nMode, bDirect );
            UCBStorageStream* pStream =  dynamic_cast<UCBStorageStream*>( pStr );
            if ( !pStream )
            {
                SetError( ( nMode & StreamMode::WRITE ) ? SVSTREAM_CANNOT_MAKE : SVSTREAM_FILE_NOT_FOUND );
                return nullptr;
            }

            pElement->m_xStream = pStream->pImp;
            delete pStream;
        }

        pElement->m_xStream->PrepareCachedForReopen( nMode );
        bool bInited = pElement->m_xStream->Init();
        if (!bInited)
        {
            SetError( ( nMode & StreamMode::WRITE ) ? SVSTREAM_CANNOT_MAKE : SVSTREAM_FILE_NOT_FOUND );
            return nullptr;
        }

        pElement->m_bIsStorage = true;
        return pElement->m_xStream->CreateStorage();  // can only be created in transacted mode
    }
    else if ( pElement->m_xStorage.is() )
    {
        // storage has already been opened; if it has no external reference, it may be opened another time
        if ( pElement->m_xStorage->m_pAntiImpl )
        {
            OSL_FAIL("Storage is already open!" );
            SetError( SVSTREAM_ACCESS_DENIED );  // ???
        }
        else
        {
            bool bIsWritable = bool( pElement->m_xStorage->m_nMode & StreamMode::WRITE );
            if ( !bIsWritable && ( nMode & StreamMode::WRITE ) )
            {
                OUString aName = pImp->m_aURL + "/" + pElement->m_aOriginalName;
                UCBStorage* pStorage = new UCBStorage( aName, nMode, bDirect, false, pImp->m_bRepairPackage, pImp->m_xProgressHandler );
                pElement->m_xStorage = pStorage->pImp;
                return pStorage;
            }
            else
            {
                return new UCBStorage( pElement->m_xStorage.get() );
            }
        }
    }
    else if ( !pElement->m_xStream.is() )
    {
        // storage is opened the first time
        bool bIsWritable = bool(pImp->m_nMode & StreamMode::WRITE);
        if ( pImp->m_bIsLinked && pImp->m_bIsRoot && bIsWritable )
        {
            // make sure that the root storage object has been created before substorages will be created
            INetURLObject aFolderObj( pImp->m_aURL );
            aFolderObj.removeSegment();

            Content aFolder( aFolderObj.GetMainURL( INetURLObject::DecodeMechanism::NONE ), Reference < XCommandEnvironment >(), comphelper::getProcessComponentContext() );
            pImp->m_oContent.emplace();
            bool bRet = ::utl::UCBContentHelper::MakeFolder( aFolder, pImp->m_aName, *pImp->m_oContent );
            if ( !bRet )
            {
                SetError( SVSTREAM_CANNOT_MAKE );
                return nullptr;
            }
        }

        UCBStorage_Impl* pStor = pImp->OpenStorage( pElement, nMode, bDirect );
        if ( pStor )
        {
            if ( pElement->m_bIsInserted )
                pStor->m_bListCreated = true; // the storage is pretty new, nothing to read

            return new UCBStorage( pStor );
        }
    }

    return nullptr;
}

UCBStorage_Impl* UCBStorage_Impl::OpenStorage( UCBStorageElement_Impl* pElement, StreamMode nMode, bool bDirect )
{
    UCBStorage_Impl* pRet = nullptr;
    OUString aName = m_aURL + "/" + pElement->m_aOriginalName;  //  ???

    pElement->m_bIsStorage = pElement->m_bIsFolder = true;

    if ( m_bIsLinked && !::utl::UCBContentHelper::Exists( aName ) )
    {
        Content aNewFolder;
        bool bRet = ::utl::UCBContentHelper::MakeFolder( *m_oContent, pElement->m_aOriginalName, aNewFolder );
        if ( bRet )
            pRet = new UCBStorage_Impl( aNewFolder, aName, nMode, nullptr, bDirect, false, m_bRepairPackage, m_xProgressHandler );
    }
    else
    {
        pRet = new UCBStorage_Impl( aName, nMode, nullptr, bDirect, false, m_bRepairPackage, m_xProgressHandler );
    }

    if ( pRet )
    {
        pRet->m_bIsLinked = m_bIsLinked;
        pRet->m_bIsRoot = false;

        // if name has been changed before creating the stream: set name!
        pRet->m_aName = pElement->m_aOriginalName;
        pElement->m_xStorage = pRet;
    }

    if ( pRet )
        pRet->Init();

    return pRet;
}

bool UCBStorage::IsStorage( const OUString& rEleName ) const
{
    if( rEleName.isEmpty() )
        return false;

    const UCBStorageElement_Impl *pElement = FindElement_Impl( rEleName );
    return ( pElement && pElement->m_bIsStorage );
}

bool UCBStorage::IsStream( const OUString& rEleName ) const
{
    if( rEleName.isEmpty() )
        return false;

    const UCBStorageElement_Impl *pElement = FindElement_Impl( rEleName );
    return ( pElement && !pElement->m_bIsStorage );
}

bool UCBStorage::IsContained( const OUString & rEleName ) const
{
    if( rEleName.isEmpty() )
        return false;
    const UCBStorageElement_Impl *pElement = FindElement_Impl( rEleName );
    return ( pElement != nullptr );
}

void UCBStorage::Remove( const OUString& rEleName )
{
    if( rEleName.isEmpty() )
        return;

    UCBStorageElement_Impl *pElement = FindElement_Impl( rEleName );
    if ( pElement )
    {
        pElement->m_bIsRemoved = true;
    }
    else
        SetError( SVSTREAM_FILE_NOT_FOUND );
}

bool UCBStorage::ValidateFAT()
{
    // ???
    return true;
}

bool UCBStorage::Validate( bool  bWrite ) const
{
    // ???
    return ( !bWrite || ( pImp->m_nMode & StreamMode::WRITE ) );
}

bool UCBStorage::ValidateMode( StreamMode m ) const
{
    // ???
    if( m == ( StreamMode::READ | StreamMode::TRUNC ) )  // from stg.cxx
        return true;
    // only SHARE_DENYALL allowed
    // storages open in r/o mode are OK, since only
    // the commit may fail
    if( m & StreamMode::SHARE_DENYALL )
        return true;

    return true;
}

bool UCBStorage::Equals( const BaseStorage& rStorage ) const
{
    // ???
    return static_cast<BaseStorage const *>(this) == &rStorage;
}

bool UCBStorage::IsStorageFile( SvStream* pFile )
{
    if ( !pFile )
        return false;

    sal_uInt64 nPos = pFile->Tell();
    if ( pFile->TellEnd() < 4 )
        return false;

    pFile->Seek(0);
    sal_uInt32 nBytes(0);
    pFile->ReadUInt32( nBytes );

    // search for the magic bytes
    bool bRet = ( nBytes == 0x04034b50 );
    if ( !bRet )
    {
        // disk spanned file have an additional header in front of the usual one
        bRet = ( nBytes == 0x08074b50 );
        if ( bRet )
        {
            nBytes = 0;
            pFile->ReadUInt32( nBytes );
            bRet = ( nBytes == 0x04034b50 );
        }
    }

    pFile->Seek( nPos );
    return bRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
