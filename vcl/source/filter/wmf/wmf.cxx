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

#include "emfwr.hxx"
#include "wmfwr.hxx"
#include <vcl/wmf.hxx>
#include <vcl/gdimetafiletools.hxx>
#include <vcl/graph.hxx>

using namespace com::sun::star;

bool ReadWindowMetafile( SvStream& rStream, GDIMetaFile& rMTF )
{
    // tdf#111484 Use new method to import Metafile. Take current StreamPos
    // into account (used by SwWW8ImplReader::ReadGrafFile and by
    // SwWw6ReadMetaStream, so do *not* ignore. OTOH XclImpDrawing::ReadWmf
    // is nice enough to copy to an own MemStream to avoid that indirect
    // parameter passing...)
    const sal_uInt64 nStreamStart(rStream.Tell());
    const sal_uInt64 nStreamEnd(rStream.TellEnd());

    if (nStreamStart >= nStreamEnd)
    {
        return false;
    }

    // Read binary data to mem array
    const sal_uInt64 nStreamLength(nStreamEnd - nStreamStart);
    BinaryDataContainer aDataContainer(rStream, nStreamLength);
    rStream.Seek(nStreamStart);

    if (rStream.good())
    {
        // Throw into VectorGraphicData to get the import. Do not care
        // too much for type, this will be checked there. Also no path
        // needed, it is a temporary object
        auto aVectorGraphicDataPtr =
            std::make_shared<VectorGraphicData>(aDataContainer, VectorGraphicDataType::Emf);

        // create a Graphic and grep Metafile from it
        const Graphic aGraphic(aVectorGraphicDataPtr);

        // get the Metafile from it, done
        rMTF = aGraphic.GetGDIMetaFile();
        return true;
    }

    return rStream.good();
}

bool ConvertGDIMetaFileToWMF( const GDIMetaFile & rMTF, SvStream & rTargetStream,
                              FilterConfigItem const * pConfigItem, bool bPlaceable)
{
    WMFWriter aWMFWriter;
    GDIMetaFile aGdiMetaFile(rMTF);

    if(usesClipActions(aGdiMetaFile))
    {
        // #i121267# It is necessary to prepare the metafile since the export does *not* support
        // clip regions. This tooling method clips the geometry content of the metafile internally
        // against its own clip regions, so that the export is safe to ignore clip regions
        clipMetafileContentAgainstOwnRegions(aGdiMetaFile);
    }

    bool bRet = aWMFWriter.WriteWMF(aGdiMetaFile, rTargetStream, pConfigItem, bPlaceable);
    return bRet;
}

bool ConvertGraphicToWMF(const Graphic& rGraphic, SvStream& rTargetStream,
                         FilterConfigItem const* pConfigItem, bool bPlaceable)
{
    GfxLink aLink = rGraphic.GetGfxLink();
    if (aLink.GetType() == GfxLinkType::NativeWmf && aLink.GetData() && aLink.GetDataSize())
    {
        if(!aLink.IsEMF()) // If WMF, just write directly.
            return rTargetStream.WriteBytes(aLink.GetData(), aLink.GetDataSize()) == aLink.GetDataSize();

        // This may be an EMF+ file. In EmfReader::ReadEnhWMF() we normally drop non-EMF commands
        // when reading EMF+, so converting that to WMF is better done by re-parsing with EMF+ disabled.
        auto & rDataContainer = aLink.getDataContainer();
        auto aVectorGraphicData
            = std::make_shared<VectorGraphicData>(rDataContainer, VectorGraphicDataType::Emf);
        aVectorGraphicData->setEnableEMFPlus(false);
        Graphic aGraphic(aVectorGraphicData);
        bool bRet = ConvertGDIMetaFileToWMF(aGraphic.GetGDIMetaFile(), rTargetStream, pConfigItem,
                                            bPlaceable);
        return bRet;
    }

    bool bRet = ConvertGDIMetaFileToWMF(rGraphic.GetGDIMetaFile(), rTargetStream, pConfigItem,
                                        bPlaceable);
    return bRet;
}

bool ConvertGDIMetaFileToEMF(const GDIMetaFile & rMTF, SvStream & rTargetStream)
{
    EMFWriter aEMFWriter(rTargetStream);
    GDIMetaFile aGdiMetaFile(rMTF);

    if(usesClipActions(aGdiMetaFile))
    {
        // #i121267# It is necessary to prepare the metafile since the export does *not* support
        // clip regions. This tooling method clips the geometry content of the metafile internally
        // against its own clip regions, so that the export is safe to ignore clip regions
        clipMetafileContentAgainstOwnRegions(aGdiMetaFile);
    }

    return aEMFWriter.WriteEMF(aGdiMetaFile);
}

bool WriteWindowMetafileBits( SvStream& rStream, const GDIMetaFile& rMTF )
{
    return WMFWriter().WriteWMF( rMTF, rStream, nullptr, false );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
