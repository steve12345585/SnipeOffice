/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#pragma once

#include <vcl/font/Feature.hxx>
#include <hb.h>
#include <i18nlangtag/languagetag.hxx>

#include <font/PhysicalFontFace.hxx>

namespace vcl::font
{
class FeatureCollector
{
private:
    const PhysicalFontFace* m_pFace;
    hb_face_t* m_pHbFace;
    std::vector<vcl::font::Feature>& m_rFontFeatures;
    const LanguageTag& m_rLanguageTag;

public:
    FeatureCollector(const PhysicalFontFace* pFace, std::vector<vcl::font::Feature>& rFontFeatures,
                     const LanguageTag& rLanguageTag)
        : m_pFace(pFace)
        , m_pHbFace(pFace->GetHbFace())
        , m_rFontFeatures(rFontFeatures)
        , m_rLanguageTag(rLanguageTag)
    {
    }

private:
    void collectForTable(hb_tag_t aTableTag);
    bool collectGraphite();

public:
    bool collect();
};

} // namespace vcl::font

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
