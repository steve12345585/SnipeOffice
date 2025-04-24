/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SVX_COMMONSTYLEMANAGER_HXX
#define INCLUDED_SVX_COMMONSTYLEMANAGER_HXX

#include <sfx2/StyleManager.hxx>
#include <svx/svxdllapi.h>

class SfxObjectShell;

namespace svx
{

class SVXCORE_DLLPUBLIC CommonStyleManager final : public sfx2::StyleManager
{
public:
    CommonStyleManager(SfxObjectShell& rShell)
        : StyleManager(rShell)
    {}

    virtual std::unique_ptr<sfx2::StylePreviewRenderer> CreateStylePreviewRenderer(
                                            OutputDevice& rOutputDev, SfxStyleSheetBase* pStyle,
                                            tools::Long nMaxHeight) override;
};

} // end namespace svx

#endif // INCLUDED_SVX_COMMONSTYLEMANAGER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
