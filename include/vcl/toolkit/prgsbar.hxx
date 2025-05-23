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

#if !defined(VCL_DLLIMPLEMENTATION) && !defined(TOOLKIT_DLLIMPLEMENTATION) && !defined(VCL_INTERNALS)
#error "don't use this in new code"
#endif

#include <config_options.h>
#include <vcl/dllapi.h>
#include <vcl/window.hxx>

/*************************************************************************
 *
 * class ProgressBar
 *
 * this class is used to display the ProgressBar
 *
 * -----------------------------------------------------------------------
 *
 * WinBits
 *
 * WB_BORDER           border around the window
 * WB_3DLOOK           3D representation
 *
 * -----------------------------------------------------------------------
 *
 * Methods
 *
 * Use SetValue() to set a percentage between 0 and 100. A value larger
 * than 100 will cause the last rectangle to start flashing
 *
 ************************************************************************/


class UNLESS_MERGELIBS(VCL_DLLPUBLIC) ProgressBar final : public vcl::Window
{
public:
    enum class BarStyle
    {
        Progress,
        Level,
    };

private:
    Point               maPos;
    tools::Long                mnPrgsWidth;
    tools::Long                mnPrgsHeight;
    sal_uInt16          mnPercent;
    sal_uInt16          mnPercentCount;
    bool                mbCalcNew;
    BarStyle            meBarStyle;

    using Window::ImplInit;
    SAL_DLLPRIVATE void             ImplInit();
    SAL_DLLPRIVATE void             ImplInitSettings( bool bFont, bool bForeground, bool bBackground );
    SAL_DLLPRIVATE void ImplDrawProgress(vcl::RenderContext& rRenderContext, sal_uInt16 nNewPerc);

protected:
    virtual void        DumpAsPropertyTree(tools::JsonWriter&) override;

public:
                        ProgressBar( vcl::Window* pParent, WinBits nWinBits, BarStyle eBarStyle );

    virtual void        Paint( vcl::RenderContext& rRenderContext, const tools::Rectangle& rRect ) override;
    virtual void        Resize() override;
    virtual void        StateChanged( StateChangedType nStateChange ) override;
    virtual void        DataChanged( const DataChangedEvent& rDCEvt ) override;
    virtual Size        GetOptimalSize() const override;

    void                SetValue( sal_uInt16 nNewPercent );
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
