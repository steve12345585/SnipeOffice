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

#include "MNSINIParser.hxx"
#include <com/sun/star/io/IOException.hpp>
#include <osl/file.h>
#include <rtl/byteseq.hxx>
#include <sal/log.hxx>
#include <o3tl/string_view.hxx>

IniParser::IniParser(OUString const & rIniName)
{
    OUString iniUrl;
    if (osl_File_E_None != osl_getFileURLFromSystemPath(rIniName.pData, &iniUrl.pData))
        return;


    oslFileHandle handle=nullptr;
    oslFileError fileError = osl_File_E_INVAL;
    try{
        if (!iniUrl.isEmpty())
            fileError = osl_openFile(iniUrl.pData, &handle, osl_File_OpenFlag_Read);
    }
    catch(const css::io::IOException&)
    {
        SAL_WARN("connectivity.mozab", "couldn't open file: " << iniUrl );
    }

    if (osl_File_E_None == fileError)
    {
        rtl::ByteSequence seq;
        sal_uInt64 nSize = 0;

        osl_getFileSize(handle, &nSize);
        OUString sectionName(  u"no name section"_ustr );
        while (true)
        {
            sal_uInt64 nPos;
            if (osl_File_E_None != osl_getFilePos(handle, &nPos) || nPos >= nSize)
                break;
            if (osl_File_E_None != osl_readLine(handle, reinterpret_cast<sal_Sequence **>(&seq)))
                break;
            OString line(reinterpret_cast<const char *>(seq.getConstArray()), seq.getLength() );
            sal_Int32 nIndex = line.indexOf('=');
            if (nIndex >= 1)
            {
                ini_Section *aSection = &mAllSection[sectionName];
                struct ini_NameValue nameValue;
                nameValue.sName = OStringToOUString(
                    o3tl::trim(line.subView(0,nIndex)), RTL_TEXTENCODING_ASCII_US );
                nameValue.sValue = OStringToOUString(
                    o3tl::trim(line.subView(nIndex+1)), RTL_TEXTENCODING_UTF8 );

                aSection->vVector.push_back(nameValue);

            }
            else
            {
                sal_Int32 nIndexStart = line.indexOf('[');
                sal_Int32 nIndexEnd = line.indexOf(']');
                if ( nIndexEnd > nIndexStart && nIndexStart >=0)
                {
                    sectionName =  OStringToOUString(
                        o3tl::trim(line.subView(nIndexStart + 1,nIndexEnd - nIndexStart -1)), RTL_TEXTENCODING_ASCII_US );
                    if (sectionName.isEmpty())
                        sectionName = "no name section";
                }
            }
        }
        osl_closeFile(handle);
    }
    else
    {
        SAL_INFO("connectivity.mozab", "couldn't open file: " << iniUrl );
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
