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

#include <memory>
#include <vcl/weld.hxx>
#include <svl/itemset.hxx>

#include "res_ErrorBar.hxx"

namespace chart
{
class ChartView;

class InsertErrorBarsDialog final : public weld::GenericDialogController
{
public:
    InsertErrorBarsDialog(weld::Window* pParent, const SfxItemSet& rMyAttrs,
                          const rtl::Reference<::chart::ChartModel>& xChartDocument,
                          ErrorBarResources::tErrorBarType eType);

    void SetAxisMinorStepWidthForErrorBarDecimals(double fMinorStepWidth);

    static double
    getAxisMinorStepWidthForErrorBarDecimals(const rtl::Reference<::chart::ChartModel>& xChartModel,
                                             const rtl::Reference<::chart::ChartView>& xChartView,
                                             std::u16string_view rSelectedObjectCID);

    void FillItemSet(SfxItemSet& rOutAttrs);

private:
    std::unique_ptr<ErrorBarResources> m_apErrorBarResources;
};

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
