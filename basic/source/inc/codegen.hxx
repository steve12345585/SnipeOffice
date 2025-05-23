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

#include "opcodes.hxx"
#include "buffer.hxx"

class SbiParser;
class SbModule;

class SbiCodeGen {
    SbiParser* pParser;         // for error messages, line, column etc.
    SbModule& rMod;
    SbiBuffer aCode;
    short  nLine, nCol;         // for stmnt command
    short  nForLevel;           // #29955
    bool bStmnt;            // true: statement-opcode is pending

public:
    SbiCodeGen(SbModule&, SbiParser*);
    SbiParser* GetParser() { return pParser; }
    SbModule& GetModule() { return rMod; }
    sal_uInt32 Gen( SbiOpcode );
    sal_uInt32 Gen( SbiOpcode, sal_uInt32 );
    sal_uInt32 Gen( SbiOpcode, sal_uInt32, sal_uInt32 );
    void Patch( sal_uInt32 o, sal_uInt32 v ){ aCode.Patch( o, v ); }
    void BackChain( sal_uInt32 off )    { aCode.Chain( off );  }
    void Statement();
    void GenStmnt();            // create statement-opcode maybe
    sal_uInt32 GetPC() const;
    sal_uInt32 GetOffset() const { return GetPC() + 1; }
    void Save();

    // #29955 service for-loop-level
    void IncForLevel() { nForLevel++; }
    void DecForLevel() { nForLevel--; }

    static sal_uInt32 calcNewOffSet( sal_uInt8 const * pCode, sal_uInt16 nOffset );
    static sal_uInt16 calcLegacyOffSet( sal_uInt8 const * pCode, sal_uInt32 nOffset );

};

template < class T, class S >
class PCodeBuffConvertor
{
    T m_nSize;
    const sal_uInt8* m_pStart;
    std::vector<sal_uInt8> m_aCnvtdBuf;

    PCodeBuffConvertor(const PCodeBuffConvertor& ) = delete;
    PCodeBuffConvertor& operator = ( const PCodeBuffConvertor& ) = delete;
public:
    PCodeBuffConvertor(const sal_uInt8* pCode, T nSize)
        : m_nSize(nSize)
        , m_pStart(pCode)
    {
        convert();
    }
    void convert();
    // pass ownership
    std::vector<sal_uInt8>&& GetBuffer() { return std::move(m_aCnvtdBuf); }
};

// #111897 PARAM_INFO flags start at 0x00010000 to not
// conflict with DefaultId in SbxParamInfo::nUserData
#define PARAM_INFO_PARAMARRAY       0x0010000
#define PARAM_INFO_WITHBRACKETS     0x0020000

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
