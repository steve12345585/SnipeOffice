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


#if !defined WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <stdio.h>
#include <objidl.h>
#include <stream_helper.hxx>

BufferStream::BufferStream(IStream *str) :
    stream(str)
{
    // These next few lines work around the "Seek pointer" bug found on Vista.
    char cBuf[20];
    unsigned long nCount;
    ULARGE_INTEGER nNewPosition;
    LARGE_INTEGER nMove;
    nMove.QuadPart = 0;
    stream->Seek( nMove, STREAM_SEEK_SET, &nNewPosition );
    stream->Read( cBuf, 20, &nCount );
}

BufferStream::~BufferStream()
{
}

unsigned long BufferStream::sread (unsigned char *buf, unsigned long size)
{
    unsigned long newsize;
    HRESULT hr;

    hr = stream->Read (buf, size, &newsize);
    if (hr == S_OK)
        return newsize;
    else
        return static_cast<unsigned long>(0);
}

long BufferStream::stell ()
{
    HRESULT hr;
    LARGE_INTEGER Move;
    ULARGE_INTEGER NewPosition;
    Move.QuadPart = 0;
    NewPosition.QuadPart = 0;

    hr = stream->Seek (Move, STREAM_SEEK_CUR, &NewPosition);
    if (hr == S_OK)
        return static_cast<long>(NewPosition.QuadPart);
    else
        return -1;
}

long BufferStream::sseek (long offset, int origin)
{
    HRESULT hr;
    LARGE_INTEGER Move;
    DWORD dwOrigin;
    Move.QuadPart = static_cast<__int64>(offset);

    switch (origin)
    {
    case SEEK_CUR:
        dwOrigin = STREAM_SEEK_CUR;
        break;
    case SEEK_END:
        dwOrigin = STREAM_SEEK_END;
        break;
    case SEEK_SET:
        dwOrigin = STREAM_SEEK_SET;
        break;
    default:
        return -1;
    }

    hr = stream->Seek (Move, dwOrigin, nullptr);
    if (hr == S_OK)
        return 0;
    else
        return -1;
}

FileStream::FileStream(const Filepath_char_t *filename) :
    file(nullptr)
{
    // fdo#67534: avoid locking to not interfere with soffice opening the file
    file = _wfsopen(filename, L"rb", _SH_DENYNO);
}

FileStream::~FileStream()
{
    if (file)
        fclose(file);
}

unsigned long FileStream::sread (unsigned char *buf, unsigned long size)
{
    if (file)
        return static_cast<unsigned long>(fread(buf, 1, size, file));
    return 0;
}

long FileStream::stell ()
{
    if (file)
        return ftell(file);
    return -1;
}

long FileStream::sseek (long offset, int origin)
{
    if (file)
        return fseek(file, offset, origin);
    return -1;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
