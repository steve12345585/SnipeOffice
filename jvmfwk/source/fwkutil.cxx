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


#if defined(_WIN32)
#if !defined WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <algorithm>
#endif

#include <osl/module.hxx>
#include <rtl/ustring.hxx>
#include <osl/file.hxx>
#include <sal/log.hxx>

#include "framework.hxx"
#include <fwkutil.hxx>
#include <memory>

using namespace osl;


namespace jfw
{

/** provides a bootstrap class which already knows the values from the
    jvmfkwrc file.
*/
const rtl::Bootstrap* Bootstrap()
{
    static const rtl::Bootstrap* SINGLETON = []()
        {
            OUString sIni = getLibraryLocation() +
#ifdef MACOSX
                // For some reason the jvmfwk3rc file is traditionally in
                // LIBO_URE_ETC_FOLDER
                "/../" LIBO_URE_ETC_FOLDER
#endif
                SAL_CONFIGFILE("/jvmfwk3");
            ::rtl::Bootstrap *  bootstrap = new ::rtl::Bootstrap(sIni);
            SAL_INFO("jfw.level2", "Using configuration file " << sIni);
            return bootstrap;
        }();
    return SINGLETON;
};

osl::Mutex& FwkMutex()
{
    static osl::Mutex SINGLETON;
    return SINGLETON;
}


rtl::ByteSequence encodeBase16(const rtl::ByteSequence& rawData)
{
    static const char EncodingTable[] =
        {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    sal_Int32 lenRaw = rawData.getLength();
    std::unique_ptr<char[]> pBuf(new char[lenRaw * 2]);
    const sal_Int8* arRaw = rawData.getConstArray();

    char* pCurBuf = pBuf.get();
    for (int i = 0; i < lenRaw; i++)
    {
        unsigned char curChar = arRaw[i];
        curChar >>= 4;

        *pCurBuf = EncodingTable[curChar];
        pCurBuf++;

        curChar = arRaw[i];
        curChar &= 0x0F;

        *pCurBuf = EncodingTable[curChar];
        pCurBuf++;
    }

    rtl::ByteSequence ret(reinterpret_cast<sal_Int8*>(pBuf.get()), lenRaw * 2);
    return ret;
}

rtl::ByteSequence decodeBase16(const rtl::ByteSequence& data)
{
    static const char decodingTable[] =
        {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    sal_Int32 lenData = data.getLength();
    sal_Int32 lenBuf = lenData / 2; //always divisible by two
    std::unique_ptr<unsigned char[]> pBuf(new unsigned char[lenBuf]);
    const sal_Int8* pData = data.getConstArray();
    for (sal_Int32 i = 0; i < lenBuf; i++)
    {
        sal_Int8 curChar = *pData++;
        //find the index of the first 4bits
        //  TODO  What happens if text is not valid Hex characters?
        unsigned char nibble = 0;
        for (unsigned char j = 0; j < 16; j++)
        {
            if (curChar == decodingTable[j])
            {
                nibble = j;
                break;
            }
        }
        nibble <<= 4;
        curChar = *pData++;
        //find the index for the next 4bits
        for (unsigned char j = 0; j < 16; j++)
        {
            if (curChar == decodingTable[j])
            {
                nibble |= j;
                break;
            }
        }
        pBuf[i] = nibble;
    }
    rtl::ByteSequence ret(reinterpret_cast<sal_Int8*>(pBuf.get()), lenBuf );
    return ret;
}

OUString getDirFromFile(std::u16string_view usFilePath)
{
    size_t index = usFilePath.rfind('/');
    return OUString(usFilePath.substr(0, index));
}

OUString getLibraryLocation()
{
    OUString libraryFileUrl;

    if (!osl::Module::getUrlFromAddress(
            reinterpret_cast< oslGenericFunction >(getLibraryLocation),
            libraryFileUrl))
        throw FrameworkException(JFW_E_ERROR,
                    "[Java framework] Error in function getLibraryLocation (fwkutil.cxx)."_ostr);

    return getDirFromFile(libraryFileUrl);
}

jfw::FileStatus checkFileURL(const OUString & sURL)
{
    jfw::FileStatus ret = jfw::FILE_OK;
    DirectoryItem item;
    File::RC rc_item = DirectoryItem::get(sURL, item);
    if (File::E_None == rc_item)
    {
        osl::FileStatus status(osl_FileStatus_Mask_Validate);

        File::RC rc_stat = item.getFileStatus(status);
        if (File::E_None == rc_stat)
        {
            ret = FILE_OK;
        }
        else if (File::E_NOENT == rc_stat)
        {
            ret = FILE_DOES_NOT_EXIST;
        }
        else
        {
            ret = FILE_INVALID;
        }
    }
    else if (File::E_NOENT == rc_item)
    {
        ret = FILE_DOES_NOT_EXIST;
    }
    else
    {
        ret = FILE_INVALID;
    }
    return ret;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
