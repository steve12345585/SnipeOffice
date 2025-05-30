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

#include <oox/dump/dffdumper.hxx>
#include <utility>

#ifdef DBG_UTIL

namespace oox::dump {

namespace {

const sal_uInt16 DFF_ID_BSE                 = 0xF007;   /// BLIP store entry.
const sal_uInt16 DFF_ID_BSTORECONTAINER     = 0xF001;   /// BLIP store container.
const sal_uInt16 DFF_ID_CHILDANCHOR         = 0xF00F;   /// Child anchor (in groups).
const sal_uInt16 DFF_ID_CLIENTANCHOR        = 0xF010;   /// Client anchor.
const sal_uInt16 DFF_ID_DG                  = 0xF008;   /// Drawing.
const sal_uInt16 DFF_ID_DGG                 = 0xF006;   /// Drawing group.
const sal_uInt16 DFF_ID_OPT                 = 0xF00B;   /// Property set.
const sal_uInt16 DFF_ID_OPT2                = 0xF121;   /// Secondary property set.
const sal_uInt16 DFF_ID_OPT3                = 0xF122;   /// Ternary property set.
const sal_uInt16 DFF_ID_SP                  = 0xF00A;   /// Shape.
const sal_uInt16 DFF_ID_SPGR                = 0xF009;   /// Shape group.
const sal_uInt16 DFF_ID_SPLITMENUCOLORS     = 0xF11E;   /// Current toolbar colors.

const sal_uInt16 DFF_OPT_IDMASK             = 0x3FFF;
const sal_uInt16 DFF_OPT_COMPLEX            = 0x8000;

} // namespace

bool DffStreamObject::implReadRecordHeader( BinaryInputStream& rBaseStrm, sal_Int64& ornRecId, sal_Int64& ornRecSize )
{
    mnInstVer = rBaseStrm.readuInt16();
    ornRecId = rBaseStrm.readuInt16();
    mnRealSize = rBaseStrm.readInt32();
    ornRecSize = isContainer() ? 0 : mnRealSize;
    return !rBaseStrm.isEof();
}

void DffStreamObject::implWriteExtHeader()
{
    const char* pcListName = "DFF-RECORD-INST";
    switch( getRecId() )
    {
        case DFF_ID_BSE:                pcListName = "DFFBSE-RECORD-INST";          break;  // BLIP type
        case DFF_ID_BSTORECONTAINER:    pcListName = "DFFBSTORECONT-RECORD-INST";   break;  // BLIP count
        case DFF_ID_DG:                 pcListName = "DFFDG-RECORD-INST";           break;  // drawing ID
        case DFF_ID_OPT:                pcListName = "DFFOPT-RECORD-INST";          break;  // property count
        case DFF_ID_SP:                 pcListName = "DFFSP-RECORD-INST";           break;  // shape type
        case DFF_ID_SPLITMENUCOLORS:    pcListName = "DFFSPLITMENUC-RECORD-INST";   break;  // number of colors
    }
    MultiItemsGuard aMultiGuard( mxOut );
    writeHexItem( "instance", mnInstVer, pcListName );
    if( isContainer() ) writeDecItem( "container-size", mnRealSize );
}

void DffStreamObject::implDumpRecordBody()
{
    switch( getRecId() )
    {
        case DFF_ID_BSE:
            dumpDec< sal_uInt8 >( "win-type", "DFFBSE-TYPE" );
            dumpDec< sal_uInt8 >( "mac-type", "DFFBSE-TYPE" );
            dumpGuid( "guid" );
            dumpDec< sal_uInt16 >( "tag" );
            dumpDec< sal_uInt32 >( "blip-size" );
            dumpDec< sal_uInt32 >( "blip-refcount" );
            dumpDec< sal_uInt32 >( "blip-streampos" );
            dumpDec< sal_uInt8 >( "blip-usage", "DFFBSE-USAGE" );
            dumpDec< sal_uInt8 >( "blip-name-len" );
            dumpUnused( 2 );
        break;

        case DFF_ID_CHILDANCHOR:
            dumpDec< sal_uInt32 >( "left" );
            dumpDec< sal_uInt32 >( "top" );
            dumpDec< sal_uInt32 >( "right" );
            dumpDec< sal_uInt32 >( "bottom" );
        break;

        case DFF_ID_CLIENTANCHOR:
        break;

        case DFF_ID_DG:
            dumpDec< sal_uInt32 >( "shape-count" );
            dumpHex< sal_uInt32 >( "max-shape-id", "CONV-DEC" );
        break;

        case DFF_ID_DGG:
        {
            dumpHex< sal_uInt32 >( "max-shape-id", "CONV-DEC" );
            sal_uInt32 nClusters = dumpDec< sal_uInt32 >( "id-cluster-count" );
            dumpDec< sal_uInt32 >( "shape-count" );
            dumpDec< sal_uInt32 >( "drawing-count" );
            mxOut->resetItemIndex( 1 );
            TableGuard aTabGuard( mxOut, 15, 16 );
            for( sal_uInt32 nCluster = 1; !mxStrm->isEof() && (nCluster < nClusters); ++nCluster )
            {
                MultiItemsGuard aMultiGuard( mxOut );
                writeEmptyItem( "#cluster" );
                dumpDec< sal_uInt32 >( "drawing-id" );
                dumpHex< sal_uInt32 >( "next-free-id", "CONV-DEC" );
            }
        }
        break;

        case DFF_ID_OPT:
        case DFF_ID_OPT2:
        case DFF_ID_OPT3:
            dumpDffOpt();
        break;

        case DFF_ID_SP:
            dumpHex< sal_uInt32 >( "shape-id", "CONV-DEC" );
            dumpHex< sal_uInt32 >( "shape-flags", "DFFSP-FLAGS" );
        break;

        case DFF_ID_SPGR:
            dumpDec< sal_uInt32 >( "left" );
            dumpDec< sal_uInt32 >( "top" );
            dumpDec< sal_uInt32 >( "right" );
            dumpDec< sal_uInt32 >( "bottom" );
        break;

        case DFF_ID_SPLITMENUCOLORS:
            dumpDffSimpleColor( "fill-color" );
            dumpDffSimpleColor( "line-color" );
            dumpDffSimpleColor( "shadow-color" );
            dumpDffSimpleColor( "3d-color" );
        break;
    }
}

sal_uInt32 DffStreamObject::dumpDffSimpleColor( const String& rName )
{
    return dumpHex< sal_uInt32 >( rName, "DFF-SIMPLE-COLOR" );
}

namespace {

enum class PropType { Binary };

struct PropInfo
{
    OUString            maName;
    PropType            meType;
    sal_uInt16          mnId;
    sal_uInt32          mnSize;
    explicit     PropInfo( OUString aName, PropType eType, sal_uInt16 nId, sal_uInt32 nSize ) :
                            maName(std::move( aName )), meType( eType ), mnId( nId ), mnSize( nSize ) {}
};

typedef ::std::vector< PropInfo > PropInfoVector;

} // namespace

void DffStreamObject::dumpDffOpt()
{
    sal_uInt16 nPropCount = getInst();
    PropInfoVector aPropInfos;
    mxOut->resetItemIndex();
    for( sal_uInt16 nPropIdx = 0; !mxStrm->isEof() && (nPropIdx < nPropCount); ++nPropIdx )
    {
        sal_uInt16 nPropId = dumpDffOptPropHeader();
        sal_uInt16 nBaseId = nPropId & DFF_OPT_IDMASK;
        sal_uInt32 nValue = mxStrm->readuInt32();

        IndentGuard aIndent( mxOut );
        if( getFlag( nPropId, DFF_OPT_COMPLEX ) )
        {
            writeHexItem( "complex-size", nValue, "CONV-DEC" );
            aPropInfos.emplace_back( String( "property-data" ), PropType::Binary, nBaseId, nValue );
        }
        else
        {
            writeHexItem( "value", nValue );
        }
    }

    mxOut->resetItemIndex();
    for (auto const& propInfo : aPropInfos)
    {
        if (mxStrm->isEof())
            break;
        mxOut->startMultiItems();
        writeEmptyItem( "#complex-data" );
        writeHexItem( "id", propInfo.mnId, "DFFOPT-PROPERTY-NAMES" );
        mxOut->endMultiItems();
        IndentGuard aIndent( mxOut );
        switch(propInfo.meType)
        {
            case PropType::Binary:
                dumpBinary( propInfo.maName, propInfo.mnSize );
            break;
            default: assert(false && "missing implementation?");
        }
    }
}

sal_uInt16 DffStreamObject::dumpDffOptPropHeader()
{
    MultiItemsGuard aMultiGuard( mxOut );
    TableGuard aTabGuard( mxOut, 11 );
    writeEmptyItem( "#prop" );
    return dumpHex< sal_uInt16 >( "id", "DFFOPT-PROPERTY-ID" );
}

} // namespace oox::dump

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
