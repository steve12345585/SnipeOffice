/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sfx2/tabdlg.hxx>
#include <res_DataTableProperties.hxx>

namespace chart
{
/** Tab page for the data table properties */
class DataTableTabPage : public SfxTabPage
{
private:
    DataTablePropertiesResources m_aDataTablePropertiesResources;

public:
    DataTableTabPage(weld::Container* pPage, weld::DialogController* pController,
                     const SfxItemSet& rInAttrs);
    virtual ~DataTableTabPage() override;

    static std::unique_ptr<SfxTabPage>
    Create(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet* rInAttrs);

    virtual bool FillItemSet(SfxItemSet* rOutAttrs) override;
    virtual void Reset(const SfxItemSet* rInAttrs) override;
};

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
