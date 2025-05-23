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

#include <sal/config.h>

#include <string_view>

#include "dxfentrd.hxx"


//---------------- A Block (= Set of Entities) --------------------------


class DXFBlock : public DXFEntities {

public:

    DXFBlock * pSucc;
        // pointer to the next block in the list DXFBlocks::pFirst

    // properties of blocks; commented with group codes:
    OString m_sName;                      //  2
    OString m_sAlsoName;                  //  3
    tools::Long nFlags;                          // 70
    DXFVector aBasePoint;                 // 10,20,30
    OString m_sXRef;                      //  1

    DXFBlock();
    ~DXFBlock();

    void Read(DXFGroupReader & rDGR);
        // reads the block (including entities) from a dxf file
        // by rGDR until an ENDBLK, ENDSEC or EOF.
};


//---------------- A set of blocks -----------------------------------


class DXFBlocks {

    DXFBlock * pFirst;
        // list of blocks, READ ONLY!

public:

    DXFBlocks();
    ~DXFBlocks();

    void Read(DXFGroupReader & rDGR);
        // reads all block per rDGR until an ENDSEC or EOF.

    DXFBlock * Search(std::string_view rName) const;
        // looks for a block with the name, return NULL if not successful

    void Clear();
        // deletes all blocks

};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
