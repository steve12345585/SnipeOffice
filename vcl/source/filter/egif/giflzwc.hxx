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

#include <vcl/mapmod.hxx>


class   GIFImageDataOutputStream;
struct  GIFLZWCTreeNode;


class GIFLZWCompressor
{
private:

    std::unique_ptr<GIFImageDataOutputStream> pIDOS;
    std::unique_ptr<GIFLZWCTreeNode[]> pTable;
    GIFLZWCTreeNode*            pPrefix;
    sal_uInt16                  nDataSize;
    sal_uInt16                  nClearCode;
    sal_uInt16                  nEOICode;
    sal_uInt16                  nTableSize;
    sal_uInt16                  nCodeSize;

public:

                                GIFLZWCompressor();
                                ~GIFLZWCompressor();

    void                        StartCompression( SvStream& rGIF, sal_uInt16 nPixelSize );
    void                        Compress(sal_uInt8* pSrc, sal_uInt32 nSize);
    void                        EndCompression();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
