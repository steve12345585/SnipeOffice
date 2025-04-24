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

#include <tools/long.hxx>
#include "tblenum.hxx"
#include "swdllapi.h"
#include <unotools/resmgr.hxx>
#include "viewsh.hxx"

#include <string_view>

class SwRect;
class Size;
class SwViewShell;
class SwDocShell;
class ReferenceMarkerName;

extern void ScrollMDI(SwViewShell const & rVwSh, const SwRect &, sal_uInt16 nRangeX, sal_uInt16 nRangeY
    , ScrollSizeMode eScrollSizeMode = ScrollSizeMode::ScrollSizeDefault);
extern bool IsScrollMDI(SwViewShell const & rVwSh, const SwRect &);
extern void SizeNotify(SwViewShell const & rVwSh, const Size &);

// Update of status bar during an action.
extern void PageNumNotify(SwViewShell const & rVwSh);

enum FlyMode { FLY_DRAG_START, FLY_DRAG, FLY_DRAG_END };
extern void FrameNotify( SwViewShell* pVwSh, FlyMode eMode = FLY_DRAG );

SW_DLLPUBLIC void StartProgress(TranslateId pMessId, tools::Long nStartVal, tools::Long nEndVal, SwDocShell *pDocSh = nullptr);
SW_DLLPUBLIC void EndProgress      ( SwDocShell const *pDocSh );
SW_DLLPUBLIC void SetProgressState  ( tools::Long nPosition, SwDocShell const *pDocShell );
void RescheduleProgress( SwDocShell const *pDocShell );

void RepaintPagePreview( SwViewShell const * pVwSh, const SwRect& rRect );

// Read ChgMode for tables from configuration.
TableChgMode GetTableChgDefaultMode();

bool JumpToSwMark( SwViewShell const * pVwSh, const ReferenceMarkerName& rMark );

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
