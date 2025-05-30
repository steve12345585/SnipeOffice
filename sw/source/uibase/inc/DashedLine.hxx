/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef INCLUDED_SW_SOURCE_UIBASE_INC_DASHEDLINE_HXX
#define INCLUDED_SW_SOURCE_UIBASE_INC_DASHEDLINE_HXX

#include <vcl/ctrl.hxx>
#include <viewopt.hxx>

/** Class for displaying a dashed line in the Writer GUI.
  */
class SwDashedLine : public Control
{
    const Color& (SwViewOption::*m_pColorFn)() const;

public:
    SwDashedLine(vcl::Window* pParent, const Color& (SwViewOption::*pColorFn)() const);
    virtual ~SwDashedLine() override;

    virtual void Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle& rRect) override;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
