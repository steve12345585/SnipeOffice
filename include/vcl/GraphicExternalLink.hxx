/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_VCL_GRAPHIC_EXTERNAL_LINK_HXX
#define INCLUDED_VCL_GRAPHIC_EXTERNAL_LINK_HXX

#include <vcl/dllapi.h>
#include <rtl/ustring.hxx>
#include <utility>

class VCL_DLLPUBLIC GraphicExternalLink
{
public:
    OUString msURL;

    GraphicExternalLink() {}

    GraphicExternalLink(OUString aURL)
        : msURL(std::move(aURL))
    {
    }
};

#endif // INCLUDED_VCL_GRAPHIC_EXTERNAL_LINK_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
