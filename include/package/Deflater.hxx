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

#ifndef INCLUDED_PACKAGE_DEFLATER_HXX
#define INCLUDED_PACKAGE_DEFLATER_HXX

#include <config_options.h>
#include <com/sun/star/uno/Sequence.hxx>
#include <package/packagedllapi.hxx>
#include <memory>

struct z_stream_s;

namespace ZipUtils {

class UNLESS_MERGELIBS(DLLPUBLIC_PACKAGE) Deflater final
{
    typedef struct z_stream_s z_stream;

    css::uno::Sequence< sal_Int8 > sInBuffer;
    bool                    bFinish;
    bool                    bFinished;
    sal_Int64               nOffset, nLength;
    // zlib total_in / total_out may be stored in 32bit, so they can overflow in case of 4gb files
    sal_uInt64              nTotalOut64, nTotalIn64; // save the overflowed value here.
    std::unique_ptr<z_stream> pStream;

    void init (sal_Int32 nLevel, bool bNowrap);
    sal_Int32 doDeflateBytes (css::uno::Sequence < sal_Int8 > &rBuffer, sal_Int32 nNewOffset, sal_Int32 nNewLength);

public:
    ~Deflater();
    Deflater(sal_Int32 nSetLevel, bool bNowrap);
    void setInputSegment( const css::uno::Sequence< sal_Int8 >& rBuffer );
    bool needsInput() const;
    void finish(  );
    bool finished() const { return bFinished;}
    sal_Int32 doDeflateSegment( css::uno::Sequence< sal_Int8 >& rBuffer, sal_Int32 nNewLength );
    sal_Int64 getTotalIn() const;
    sal_Int64 getTotalOut() const;
    void reset(  );
    void end(  );
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
