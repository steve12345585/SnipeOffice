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

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include <tools/stream.hxx>
#include <map>

#include <mutex>
#include <osl/thread.h>
#include <sal/log.hxx>

#include <osl/file.hxx>
#include <osl/detail/file.h>

using namespace osl;

// InternalLock ----------------------------------------------------------------

namespace {

std::mutex& LockMutex()
{
    static std::mutex SINGLETON;
    return SINGLETON;
}

std::map<SvFileStream const *, osl::DirectoryItem> gLocks;

bool lockFile( const SvFileStream* pStream )
{
    osl::DirectoryItem aItem;
    if (osl::DirectoryItem::get( pStream->GetFileName(), aItem) != osl::FileBase::E_None )
    {
        SAL_INFO("tools.stream", "Failed to lookup stream for locking");
        return true;
    }

    osl::FileStatus aStatus( osl_FileStatus_Mask_Type );
    if ( aItem.getFileStatus( aStatus ) != osl::FileBase::E_None )
    {
        SAL_INFO("tools.stream", "Failed to stat stream for locking");
        return true;
    }
    if( aStatus.getFileType() == osl::FileStatus::Directory )
        return true;

    std::unique_lock aGuard( LockMutex() );
    for( const auto& [rLockStream, rLockItem] : gLocks )
    {
        if( aItem.isIdenticalTo( rLockItem ) )
        {
            StreamMode nLockMode = rLockStream->GetStreamMode();
            StreamMode nNewMode = pStream->GetStreamMode();
            bool bDenyByOptions = (nLockMode & StreamMode::SHARE_DENYALL) ||
                ( (nLockMode & StreamMode::SHARE_DENYWRITE) && (nNewMode & StreamMode::WRITE) ) ||
                ( (nLockMode & StreamMode::SHARE_DENYREAD) && (nNewMode & StreamMode::READ) );

            if( bDenyByOptions )
            {
                return false; // file is already locked
            }
        }
    }
    gLocks[pStream] = aItem;
    return true;
}

void unlockFile( SvFileStream const * pStream )
{
    std::unique_lock aGuard( LockMutex() );
    gLocks.erase(pStream);
}

}

static ErrCode GetSvError( int nErrno )
{
    static struct { int nErr; ErrCode sv; } const errArr[] =
    {
        { 0,            ERRCODE_NONE },
        { EACCES,       SVSTREAM_ACCESS_DENIED },
        { EBADF,        SVSTREAM_INVALID_HANDLE },
#if defined(NETBSD) || \
    defined(FREEBSD) || defined(MACOSX) || defined(OPENBSD) || \
    defined(__FreeBSD_kernel__) || defined(DRAGONFLY) || \
    defined(IOS) || defined(HAIKU)
        { EDEADLK,      SVSTREAM_LOCKING_VIOLATION },
#else
        { EDEADLOCK,    SVSTREAM_LOCKING_VIOLATION },
#endif
        { EINVAL,       SVSTREAM_INVALID_PARAMETER },
        { EMFILE,       SVSTREAM_TOO_MANY_OPEN_FILES },
        { ENFILE,       SVSTREAM_TOO_MANY_OPEN_FILES },
        { ENOENT,       SVSTREAM_FILE_NOT_FOUND },
        { EPERM,        SVSTREAM_ACCESS_DENIED },
        { EROFS,        SVSTREAM_ACCESS_DENIED },
        { EAGAIN,       SVSTREAM_LOCKING_VIOLATION },
        { EISDIR,       SVSTREAM_PATH_NOT_FOUND },
        { ELOOP,        SVSTREAM_PATH_NOT_FOUND },
#if !defined(NETBSD) && !defined (FREEBSD) && \
    !defined(MACOSX) && !defined(OPENBSD) && !defined(__FreeBSD_kernel__) && \
    !defined(DRAGONFLY)
        { EMULTIHOP,    SVSTREAM_PATH_NOT_FOUND },
        { ENOLINK,      SVSTREAM_PATH_NOT_FOUND },
#endif
        { ENOTDIR,      SVSTREAM_PATH_NOT_FOUND },
        { ETXTBSY,      SVSTREAM_ACCESS_DENIED  },
        { EEXIST,       SVSTREAM_CANNOT_MAKE    },
        { ENOSPC,       SVSTREAM_DISK_FULL      },
        { int(0xFFFF),  SVSTREAM_GENERALERROR }
    };

    ErrCode nRetVal = SVSTREAM_GENERALERROR; // default error
    int i=0;
    do
    {
        if ( errArr[i].nErr == nErrno )
        {
            nRetVal = errArr[i].sv;
            break;
        }
        ++i;
    }
    while( errArr[i].nErr != 0xFFFF );
    return nRetVal;
}

static ErrCode GetSvError( oslFileError nErrno )
{
    static struct { oslFileError nErr; ErrCode sv; } const errArr[] =
    {
        { osl_File_E_None,        ERRCODE_NONE },
        { osl_File_E_ACCES,       SVSTREAM_ACCESS_DENIED },
        { osl_File_E_BADF,        SVSTREAM_INVALID_HANDLE },
        { osl_File_E_DEADLK,      SVSTREAM_LOCKING_VIOLATION },
        { osl_File_E_INVAL,       SVSTREAM_INVALID_PARAMETER },
        { osl_File_E_MFILE,       SVSTREAM_TOO_MANY_OPEN_FILES },
        { osl_File_E_NFILE,       SVSTREAM_TOO_MANY_OPEN_FILES },
        { osl_File_E_NOENT,       SVSTREAM_FILE_NOT_FOUND },
        { osl_File_E_PERM,        SVSTREAM_ACCESS_DENIED },
        { osl_File_E_ROFS,        SVSTREAM_ACCESS_DENIED },
        { osl_File_E_AGAIN,       SVSTREAM_LOCKING_VIOLATION },
        { osl_File_E_ISDIR,       SVSTREAM_PATH_NOT_FOUND },
        { osl_File_E_LOOP,        SVSTREAM_PATH_NOT_FOUND },
        { osl_File_E_MULTIHOP,    SVSTREAM_PATH_NOT_FOUND },
        { osl_File_E_NOLINK,      SVSTREAM_PATH_NOT_FOUND },
        { osl_File_E_NOTDIR,      SVSTREAM_PATH_NOT_FOUND },
        { osl_File_E_EXIST,       SVSTREAM_CANNOT_MAKE    },
        { osl_File_E_NOSPC,       SVSTREAM_DISK_FULL      },
        { oslFileError(0xFFFF),   SVSTREAM_GENERALERROR }
    };

    ErrCode nRetVal = SVSTREAM_GENERALERROR; // default error
    int i=0;
    do
    {
        if ( errArr[i].nErr == nErrno )
        {
            nRetVal = errArr[i].sv;
            break;
        }
        ++i;
    }
    while( errArr[i].nErr != oslFileError(0xFFFF) );
    return nRetVal;
}

SvFileStream::SvFileStream( const OUString& rFileName, StreamMode nOpenMode )
{
    bIsOpen             = false;
    m_isWritable        = false;

    SetBufferSize( 1024 );
    // convert URL to SystemPath, if necessary
    OUString aSystemFileName;
    if( FileBase::getSystemPathFromFileURL( rFileName , aSystemFileName )
        != FileBase::E_None )
    {
        aSystemFileName = rFileName;
    }
    Open( aSystemFileName, nOpenMode );
}

SvFileStream::SvFileStream()
{
    bIsOpen             = false;
    m_isWritable        = false;
    SetBufferSize( 1024 );
}

SvFileStream::~SvFileStream()
{
    Close();
}

std::size_t SvFileStream::GetData( void* pData, std::size_t nSize )
{
    SAL_INFO("tools", OString::number(static_cast<sal_Int64>(nSize)) << " Bytes from " << aFilename);

    sal_uInt64 nRead = 0;
    if ( IsOpen() )
    {
        oslFileError rc = osl_readFile(mxFileHandle,pData,static_cast<sal_uInt64>(nSize),&nRead);
        if ( rc != osl_File_E_None )
        {
            SetError( ::GetSvError( rc ));
            return -1;
        }
    }
    return static_cast<std::size_t>(nRead);
}

std::size_t SvFileStream::PutData( const void* pData, std::size_t nSize )
{
    SAL_INFO("tools", OString::number(static_cast<sal_Int64>(nSize)) << " Bytes to " << aFilename);

    sal_uInt64 nWrite = 0;
    if ( IsOpen() )
    {
        oslFileError rc = osl_writeFile(mxFileHandle,pData,static_cast<sal_uInt64>(nSize),&nWrite);
        if ( rc != osl_File_E_None )
        {
            SetError( ::GetSvError( rc ) );
            return -1;
        }
        else if( !nWrite )
        {
            SetError( SVSTREAM_DISK_FULL );
            return -1;
        }
    }
    return static_cast<std::size_t>(nWrite);
}

sal_uInt64 SvFileStream::SeekPos(sal_uInt64 const nPos)
{
    // check if a truncated STREAM_SEEK_TO_END was passed
    assert(nPos != sal_uInt64(sal_uInt32(STREAM_SEEK_TO_END)));
    if ( IsOpen() )
    {
        oslFileError rc;
        sal_uInt64 nNewPos;
        if ( nPos != STREAM_SEEK_TO_END )
            rc = osl_setFilePos( mxFileHandle, osl_Pos_Absolut, nPos );
        else
            rc = osl_setFilePos( mxFileHandle, osl_Pos_End, 0 );

        if ( rc != osl_File_E_None )
        {
            SetError( SVSTREAM_SEEK_ERROR );
            return 0;
        }
        if ( nPos != STREAM_SEEK_TO_END )
            return nPos;
        osl_getFilePos( mxFileHandle, &nNewPos );
        return nNewPos;
    }
    SetError( SVSTREAM_GENERALERROR );
    return 0;
}

void SvFileStream::FlushData()
{
    auto rc = osl_syncFile(mxFileHandle);
    if (rc != osl_File_E_None)
        SetError( ::GetSvError( rc ));
 }

bool SvFileStream::LockFile()
{
    int nLockMode = 0;

    if ( ! IsOpen() )
        return false;

    if (m_eStreamMode & StreamMode::SHARE_DENYALL)
    {
        if (m_isWritable)
            nLockMode = F_WRLCK;
        else
            nLockMode = F_RDLCK;
    }

    if (m_eStreamMode & StreamMode::SHARE_DENYREAD)
    {
        if (m_isWritable)
            nLockMode = F_WRLCK;
        else
        {
            SetError(SVSTREAM_LOCKING_VIOLATION);
            return false;
        }
    }

    if (m_eStreamMode & StreamMode::SHARE_DENYWRITE)
    {
        if (m_isWritable)
            nLockMode = F_WRLCK;
        else
            nLockMode = F_RDLCK;
    }

    if (!nLockMode)
        return true;

    if( !lockFile( this ) )
    {
        SAL_WARN("tools.stream", "InternalLock on " << aFilename << "failed");
        return false;
    }

    return true;
}

void SvFileStream::UnlockFile()
{
    if ( ! IsOpen() )
        return;

    unlockFile( this );
}

void SvFileStream::Open( const OUString& rFilename, StreamMode nOpenMode )
{
    sal_uInt32 uFlags;
    oslFileHandle nHandleTmp;

    Close();
    errno = 0;
    m_eStreamMode = nOpenMode;
    m_eStreamMode &= ~StreamMode::TRUNC; // don't truncate on reopen

    aFilename = rFilename;

    SAL_INFO("tools", aFilename);

    OUString aFileURL;
    osl::DirectoryItem aItem;
    osl::FileStatus aStatus( osl_FileStatus_Mask_Type | osl_FileStatus_Mask_LinkTargetURL );

    // FIXME: we really need to switch to a pure URL model ...
    if ( osl::File::getFileURLFromSystemPath( aFilename, aFileURL ) != osl::FileBase::E_None )
        aFileURL = aFilename;

    // don't both stat()ing a temporary file, unnecessary
    bool bStatValid = true;
    if (!(nOpenMode & StreamMode::TEMPORARY))
    {
        bStatValid = ( osl::DirectoryItem::get( aFileURL, aItem) == osl::FileBase::E_None &&
                        aItem.getFileStatus( aStatus ) == osl::FileBase::E_None );

        // SvFileStream can't open a directory
        if( bStatValid && aStatus.getFileType() == osl::FileStatus::Directory )
        {
            SetError( ::GetSvError( EISDIR ) );
            return;
        }
    }

    if ( !( nOpenMode & StreamMode::WRITE ) )
        uFlags = osl_File_OpenFlag_Read;
    else if ( !( nOpenMode & StreamMode::READ ) )
        uFlags = osl_File_OpenFlag_Write;
    else
        uFlags = osl_File_OpenFlag_Read | osl_File_OpenFlag_Write;

    // Fix (MDA, 18.01.95): Don't open with O_CREAT upon RD_ONLY
    // Important for Read-Only-Filesystems (e.g,  CDROM)
    if ( (!( nOpenMode & StreamMode::NOCREATE )) && ( uFlags != osl_File_OpenFlag_Read ) )
        uFlags |= osl_File_OpenFlag_Create;
    if ( nOpenMode & StreamMode::TRUNC )
        uFlags |= osl_File_OpenFlag_Trunc;

    uFlags |= osl_File_OpenFlag_NoExcl | osl_File_OpenFlag_NoLock;

    if ( nOpenMode & StreamMode::WRITE)
    {
        if ( nOpenMode & StreamMode::COPY_ON_SYMLINK )
        {
            if ( bStatValid && aStatus.getFileType() == osl::FileStatus::Link &&
                 aStatus.getLinkTargetURL().getLength() > 0 )
            {
                // delete the symbolic link, and replace it with the contents of the link
                if (osl::File::remove( aFileURL ) == osl::FileBase::E_None )
                {
                    File::copy( aStatus.getLinkTargetURL(), aFileURL );
                    SAL_INFO("tools.stream",
                        "Removing link and replacing with file contents (" <<
                        aStatus.getLinkTargetURL() << ") -> (" << aFileURL << ").");
                }
            }
        }
    }

    oslFileError rc = osl_openFile( aFileURL.pData, &nHandleTmp, uFlags );
    if ( rc != osl_File_E_None )
    {
        if ( uFlags & osl_File_OpenFlag_Write )
        {
            // Change to read-only
            uFlags &= ~osl_File_OpenFlag_Write;
            rc = osl_openFile( aFileURL.pData, &nHandleTmp, uFlags );
        }
    }
    if ( rc == osl_File_E_None )
    {
        mxFileHandle = nHandleTmp;
        bIsOpen = true;
        if ( uFlags & osl_File_OpenFlag_Write )
            m_isWritable = true;

        if ( !LockFile() ) // whole file
        {
            osl_closeFile( nHandleTmp );
            bIsOpen = false;
            m_isWritable = false;
            mxFileHandle = nullptr;
        }
    }
    else
        SetError( ::GetSvError( rc ) );
}

void SvFileStream::Close()
{
    UnlockFile();

    if ( IsOpen() )
    {
        SAL_INFO("tools", "Closing " << aFilename);
        FlushBuffer();
        osl_closeFile( mxFileHandle );
        mxFileHandle = nullptr;
    }

    bIsOpen     = false;
    m_isWritable = false;
    SvStream::ClearBuffer();
    SvStream::ClearError();
}

/// set filepointer to beginning of file
void SvFileStream::ResetError()
{
    SvStream::ClearError();
}

void SvFileStream::SetSize (sal_uInt64 const nSize)
{
    if (IsOpen())
    {
        oslFileError rc = osl_setFileSize( mxFileHandle, nSize );
        if (rc != osl_File_E_None )
        {
            SetError ( ::GetSvError( rc ));
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
