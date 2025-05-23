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

#include "precompile.h"

#include <o3tl/char16_t2wchar_t.hxx>
#include <unotools/tempfile.hxx>

#include <string.h>

#include "hwplib.h"
#include "hwpfile.h"
#include "htags.h"

bool HyperText::Read(HWPFile& hwpf)
{
    size_t nRead = hwpf.ReadBlock(filename, 256);
    nRead += hwpf.Read2b(bookmark, 16);
    nRead += hwpf.ReadBlock(macro, 325);
    if (hwpf.Read1b(type))
        ++nRead;
    else
        type = 0;
    nRead += hwpf.ReadBlock(reserve, 3);
    if( type == 2 )
    {
        for( int i = 1; i < 256; i++)
        {
            filename[i-1] = filename[i];
            if( filename[i] == 0 )
                break;
        }
    }
    return nRead == 617;
}

EmPicture::EmPicture(size_t tsize)
    : size(tsize >= 32 ? tsize - 32 : 0)
    , data(size, 0)
{
}

EmPicture::~EmPicture()
{
};

bool EmPicture::Read(HWPFile & hwpf)
{
    if (size == 0)
        return false;
    hwpf.ReadBlock(name, 16);
    hwpf.ReadBlock(type, 16);
    name[0] = 'H';
    name[1] = 'W';
    name[2] = 'P';
    return hwpf.ReadBlock(data.data(), size) != 0;
}


OlePicture::OlePicture(int tsize)
    : signature(0)
#ifdef _WIN32
    , pis(nullptr)
#endif
{
    size = tsize - 4;
    if (size <= 0)
        return;
};

#ifdef _WIN32
OlePicture::~OlePicture()
{
     if( pis )
          pis->Release();
};
#endif

#define FILESTG_SIGNATURE_NORMAL 0xF8995568

void OlePicture::Read(HWPFile & hwpf)
{
    if (size <= 0)
        return;

// We process only FILESTG_SIGNATURE_NORMAL.
    hwpf.Read4b(signature);
    if (signature != FILESTG_SIGNATURE_NORMAL)
        return;
#ifdef _WIN32
    char *data = new char[size];
    if (hwpf.ReadBlock(data,size) == 0)
    {
          delete [] data;
          return;
    }

    utl::TempFileNamed aTempFile;
    aTempFile.EnableKillingFile();

    SvFileStream aOutputStream(aTempFile.GetURL(), StreamMode::WRITE);
    aOutputStream.WriteBytes(data, size);
    delete [] data;
    aOutputStream.Close();
    if( StgOpenStorage(o3tl::toW(aTempFile.GetFileName().getStr()), nullptr,
                    STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_TRANSACTED,
                    nullptr, 0, &pis) != S_OK ) {
         pis = nullptr;
         return;
    }
#else
    hwpf.SkipBlock(size);
#endif
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
