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

#include <svx/svdview.hxx>

namespace basctl
{

class DlgEditor;


// DlgEdView


class DlgEdView : public SdrView
{
private:
    DlgEditor& rDlgEditor;

public:

    DlgEdView(
        SdrModel& rSdrModel,
        OutputDevice& rOut,
        DlgEditor& rEditor);

    virtual ~DlgEdView() override;

    virtual void MarkListHasChanged() override;
    virtual void MakeVisible( const tools::Rectangle& rRect, vcl::Window& rWin ) override;

protected:
    /// override to handle HitTest for some objects specially
    using SdrView::CheckSingleSdrObjectHit;
    virtual SdrObject* CheckSingleSdrObjectHit(const Point& rPnt, sal_uInt16 nTol, SdrObject* pObj, SdrPageView* pPV, SdrSearchOptions nOptions, const SdrLayerIDSet* pMVisLay) const override;
};

} // namespace basctl

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
