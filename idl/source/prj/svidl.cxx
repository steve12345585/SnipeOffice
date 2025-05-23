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
#include <database.hxx>
#include <command.hxx>
#include <rtl/ustring.hxx>
#include <osl/file.hxx>
#include <memory>

#define BR 0x8000
static bool FileMove_Impl( const OUString & rFile1, const OUString & rFile2, bool bMoveAlways )
{
    //printf( "Move from %s to %s\n", rFile2.GetStr(), rFile1.GetStr() );
    size_t nC1 = 0;
    size_t nC2 = 1;
    if( !bMoveAlways )
    {
        SvFileStream aOutStm1( rFile1, StreamMode::STD_READ );
        SvFileStream aOutStm2( rFile2, StreamMode::STD_READ );
        if( aOutStm1.GetError() == ERRCODE_NONE )
        {
            std::unique_ptr<sal_uInt8[]> pBuf1(new sal_uInt8[ BR ]);
            std::unique_ptr<sal_uInt8[]> pBuf2(new sal_uInt8[ BR ]);
            nC1 = aOutStm1.ReadBytes(pBuf1.get(), BR);
            nC2 = aOutStm2.ReadBytes(pBuf2.get(), BR);
            while( nC1 == nC2 )
            {
                if( memcmp( pBuf1.get(), pBuf2.get(), nC1 ) )
                {
                    nC1++;
                    break;
                }
                else
                {
                    if( 0x8000 != nC1 )
                        break;
                    nC1 = aOutStm1.ReadBytes(pBuf1.get(), BR);
                    nC2 = aOutStm2.ReadBytes(pBuf2.get(), BR);
                }
            }
        }
    }
    OUString fileURL2;
    osl::FileBase::getFileURLFromSystemPath( rFile2, fileURL2 );
    if( nC1 != nC2 )
    {// something has changed
        OUString fileURL1;
        osl::FileBase::getFileURLFromSystemPath( rFile1, fileURL1 );
        // move file
        if( osl::FileBase::E_None != osl::File::move( fileURL2, fileURL1 ) )
        {
            // delete both files
            osl::File::remove( fileURL1 );
            osl::File::remove( fileURL2 );
            return false;
        }
        return true;
    }
    return osl::FileBase::E_None == osl::File::remove( fileURL2 );
}

//This function gets a system path to a file [fname], creates a temp file in
//the same folder as [fname] and returns the system path of the temp file.
static OUString tempFileHelper(std::u16string_view fname)
{
    OUString aTmpFile;

    size_t delimIndex = fname.rfind( '/' );
    if( delimIndex > 0 && delimIndex != std::u16string_view::npos)
    {
        OUString aTmpDir( fname.substr( 0,  delimIndex ) );
        osl::FileBase::getFileURLFromSystemPath( aTmpDir, aTmpDir );
        osl::FileBase::createTempFile( &aTmpDir, nullptr, &aTmpFile );
        osl::FileBase::getSystemPathFromFileURL( aTmpFile, aTmpFile );
    }
    else
    {
        OString aStr = "invalid filename: " +
            OUStringToOString(fname, RTL_TEXTENCODING_UTF8);
        fprintf(stderr, "%s\n", aStr.getStr());
    }
    return aTmpFile;
}

int main ( int argc, char ** argv)
{
    OUString aTmpSlotMapFile;
    OUString aTmpDepFile;

    SvCommand aCommand( argc, argv );

    if( aCommand.nVerbosity != 0 )
        printf( "StarView Interface Definition Language (IDL) Compiler 3.0\n" );

    Init();
    std::unique_ptr<SvIdlWorkingBase> pDataBase( new SvIdlWorkingBase(aCommand));

    int nExit = 0;
    if( !aCommand.aExportFile.isEmpty() )
    {
        osl::DirectoryItem aDI;
        osl::FileStatus fileStatus( osl_FileStatus_Mask_FileName );
        (void)osl::DirectoryItem::get( aCommand.aExportFile, aDI );
        (void)aDI.getFileStatus(fileStatus);
        pDataBase->SetExportFile( fileStatus.getFileName() );
    }

    if( ReadIdl( pDataBase.get(), aCommand ) )
    {
        if( nExit == 0 && !aCommand.aSlotMapFile.isEmpty() )
        {
            aTmpSlotMapFile = tempFileHelper(aCommand.aSlotMapFile);
            SvFileStream aOutStm( aTmpSlotMapFile, StreamMode::READWRITE | StreamMode::TRUNC );
            if( !pDataBase->WriteSfx( aOutStm ) )
            {
                nExit = -1;
                OString aStr = "cannot write slotmap file: " +
                    OUStringToOString(aCommand.aSlotMapFile, RTL_TEXTENCODING_UTF8);
                fprintf(stderr, "%s\n", aStr.getStr());
            }
        }
        if (nExit == 0 && !aCommand.m_DepFile.isEmpty())
        {
            aTmpDepFile = tempFileHelper(aCommand.m_DepFile);
            SvFileStream aOutStm( aTmpDepFile, StreamMode::READWRITE | StreamMode::TRUNC );
            pDataBase->WriteDepFile(aOutStm, aCommand.aTargetFile);
            if( aOutStm.GetError() != ERRCODE_NONE )
            {
                nExit = -1;
                fprintf( stderr, "cannot write dependency file: %s\n",
                        OUStringToOString( aCommand.m_DepFile,
                            RTL_TEXTENCODING_UTF8 ).getStr() );
            }
        }
    }
    else
        nExit = -1;

    if( nExit == 0 )
    {
        bool bErr = false;
        bool bDoMove = aCommand.aTargetFile.isEmpty();
        OUString aErrFile, aErrFile2;
        if (!aCommand.aSlotMapFile.isEmpty())
        {
            bErr = !FileMove_Impl( aCommand.aSlotMapFile, aTmpSlotMapFile, bDoMove );
            if( bErr ) {
                aErrFile = aCommand.aSlotMapFile;
                aErrFile2 = aTmpSlotMapFile;
            }
        }
        if (!bErr && !aCommand.m_DepFile.isEmpty())
        {
            bErr |= !FileMove_Impl( aCommand.m_DepFile, aTmpDepFile, bDoMove );
            if (bErr) {
                aErrFile = aCommand.m_DepFile;
                aErrFile2 = aTmpDepFile;
            }
        }

        if( bErr )
        {
            nExit = -1;
            OString aStr = "cannot move file from: " +
                OUStringToOString(aErrFile2, RTL_TEXTENCODING_UTF8) +
                "\n              to file: " +
                OUStringToOString(aErrFile, RTL_TEXTENCODING_UTF8);
            fprintf( stderr, "%s\n", aStr.getStr() );
        }
        else
        {
            if( !aCommand.aTargetFile.isEmpty() )
            {
                // stamp file, because idl passed through correctly
                SvFileStream aOutStm( aCommand.aTargetFile,
                                StreamMode::READWRITE | StreamMode::TRUNC );
            }
        }
    }

    if( nExit != 0 )
    {
        if( !aCommand.aSlotMapFile.isEmpty() )
        {
            osl::FileBase::getSystemPathFromFileURL( aTmpSlotMapFile, aTmpSlotMapFile );
            osl::File::remove( aTmpSlotMapFile );
        }
    }

    if( nExit != 0 )
        fprintf( stderr, "svidl terminated with errors\n" );
    return nExit;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
