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

#include <gallerystoragelocations.hxx>
#include <svx/galmisc.hxx>

INetURLObject GalleryStorageLocations::ImplGetURLIgnoreCase(const INetURLObject& rURL)
{
    INetURLObject aURL(rURL);

    // check original file name
    if (!FileExists(aURL))
    {
        // check upper case file name
        aURL.setName(aURL.getName().toAsciiUpperCase());

        if (!FileExists(aURL))
        {
            // check lower case file name
            aURL.setName(aURL.getName().toAsciiLowerCase());
        }
    }

    return aURL;
}

void GalleryStorageLocations::SetThmExtension(INetURLObject& aURL)
{
    aURL.setExtension(u"thm");
    maThmURL = ImplGetURLIgnoreCase(aURL);
}

void GalleryStorageLocations::SetSdgExtension(INetURLObject& aURL)
{
    aURL.setExtension(u"sdg");
    maSdgURL = ImplGetURLIgnoreCase(aURL);
}

void GalleryStorageLocations::SetSdvExtension(INetURLObject& aURL)
{
    aURL.setExtension(u"sdv");
    maSdvURL = ImplGetURLIgnoreCase(aURL);
}

void GalleryStorageLocations::SetStrExtension(INetURLObject& aURL)
{
    aURL.setExtension(u"str");
    maStrURL = ImplGetURLIgnoreCase(aURL);
}

void GalleryStorageLocations::SetStorageLocations(INetURLObject& rURL)
{
    SetThmExtension(rURL);
    SetSdgExtension(rURL);
    SetSdvExtension(rURL);
    SetStrExtension(rURL);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
