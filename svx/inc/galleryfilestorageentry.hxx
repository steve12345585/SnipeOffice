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

#include <tools/urlobj.hxx>
#include <svx/galtheme.hxx>
#include "galleryfilestorage.hxx"
#include "gallerystoragelocations.hxx"

class GalleryObjectCollection;

class GalleryFileStorageEntry final
{
private:
    GalleryStorageLocations maGalleryStorageLocations;

public:
    GalleryFileStorageEntry();
    static void CreateUniqueURL(const INetURLObject& rBaseURL, INetURLObject& aURL);

    OUString ReadStrFromIni(std::u16string_view aKeyName) const;

    const INetURLObject& GetThmURL() const { return maGalleryStorageLocations.GetThmURL(); }
    const INetURLObject& GetSdgURL() const { return maGalleryStorageLocations.GetSdgURL(); }
    const INetURLObject& GetSdvURL() const { return maGalleryStorageLocations.GetSdvURL(); }
    const INetURLObject& GetStrURL() const { return maGalleryStorageLocations.GetStrURL(); }

    const GalleryStorageLocations& getGalleryStorageLocations() const
    {
        return maGalleryStorageLocations;
    }
    GalleryStorageLocations& getGalleryStorageLocations() { return maGalleryStorageLocations; }

    static GalleryThemeEntry* CreateThemeEntry(const INetURLObject& rURL, bool bReadOnly);

    void removeTheme();

    std::unique_ptr<GalleryTheme>& getCachedTheme(std::unique_ptr<GalleryTheme>& pNewTheme);

    void setStorageLocations(INetURLObject& rURL);

    std::unique_ptr<GalleryFileStorage>
    createGalleryStorageEngine(GalleryObjectCollection& mrGalleryObjectCollection, bool& bReadOnly);
};

SvStream& ReadGalleryTheme(SvStream& rIn, GalleryTheme& rTheme);

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
