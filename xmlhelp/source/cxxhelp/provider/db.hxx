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
#pragma once

#include <com/sun/star/ucb/XSimpleFileAccess3.hpp>
#include <comphelper/fileurl.hxx>
#include <osl/diagnose.h>
#include <rtl/string.hxx>
#include <memory>
#include <unordered_map>

namespace helpdatafileproxy {

    class HDFData
    {
        friend class        Hdf;

        int                     m_nSize;
        std::unique_ptr<char[]> m_pBuffer;

        void copyToBuffer( const char* pSrcData, int nSize );

    public:
        HDFData() : m_nSize( 0 ) {}

        int getSize() const
            { return m_nSize; }
        const char* getData() const
            { return m_pBuffer.get(); }
    };

    typedef std::unordered_map< OString,std::pair<int,int> >   StringToValPosMap;
    typedef std::unordered_map< OString,OString >     StringToDataMap;

    class Hdf
    {
        OUString       m_aFileURL;
        std::unique_ptr<StringToDataMap>   m_pStringToDataMap;
        std::unique_ptr<StringToValPosMap> m_pStringToValPosMap;
        css::uno::Reference< css::ucb::XSimpleFileAccess3 >
                            m_xSFA;

        css::uno::Sequence< sal_Int8 >
                            m_aItData;
        int                 m_nItRead;
        int                 m_iItPos;

        static bool implReadLenAndData(
            const char* pData, char const * end, int& riPos, HDFData& rValue );

    public:
        //HDFHelp must get a fileURL which can then directly be used by simple file access.
        //SimpleFileAccess requires file URLs as arguments. Passing file path may work but fails
        //for example when using long file paths on Windows, which start with "\\?\"
        Hdf( OUString aFileURL,
             css::uno::Reference< css::ucb::XSimpleFileAccess3 > xSFA )
                : m_aFileURL( std::move(aFileURL) )
                , m_xSFA( std::move(xSFA) )
                , m_nItRead( -1 )
                , m_iItPos( -1 )
        {
            OSL_ASSERT(comphelper::isFileUrl(m_aFileURL));
        }
        ~Hdf();

        void createHashMap( bool bOptimizeForPerformance );
        void releaseHashMap();

        bool getValueForKey( const OString& rKey, HDFData& rValue );

        bool startIteration();
        bool getNextKeyAndValue( HDFData& rKey, HDFData& rValue );
        void stopIteration();
        Hdf(const Hdf&) = delete;
        void operator=(const Hdf&) = delete;
    };

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
