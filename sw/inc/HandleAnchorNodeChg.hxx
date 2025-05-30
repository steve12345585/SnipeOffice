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

#include "pam.hxx"
#include <wrtsh.hxx>

class SwFlyFrameFormat;
class SwFormatAnchor;
class SwFlyFrame;

// helper class to track change of anchor node of at-paragraph respectively
// at-character anchored fly frames
// if such a change happens, it has to be checked, if the count of the anchor
// frames also change. if yes, a re-creation of the fly frames is needed:
// - deletion of existing fly frames before the intrinsic anchor node changes
// - creation of new fly frames after the intrinsic anchor node change.
class SwHandleAnchorNodeChg
{
public:
    /** checks, if re-creation of fly frames for an anchor node change at the
        given fly frame format is necessary, and performs the first part.

        @param _rFlyFrameFormat
        reference to the fly frame format instance, which is handled.

        @param _rNewAnchorFormat
        new anchor attribute, which will be applied at the given fly frame format

        @param _pKeepThisFlyFrame
        optional parameter - pointer to a fly frame of the given fly frame format,
        which isn't deleted, if re-creation of fly frames is necessary.
    */
    SwHandleAnchorNodeChg(SwFlyFrameFormat& _rFlyFrameFormat,
                          const SwFormatAnchor& _rNewAnchorFormat,
                          SwFlyFrame const* _pKeepThisFlyFrame = nullptr);

    /** calls <SwFlyFrameFormat::MakeFrames>, if re-creation of fly frames is necessary. */
    ~SwHandleAnchorNodeChg();

private:
    // fly frame format, which is tracked for an anchor node change.
    SwFlyFrameFormat& mrFlyFrameFormat;
    // internal flag, which indicates that the certain anchor node change occurs
    // and that re-creation of fly frames is necessary.
    bool mbAnchorNodeChanged;

    /// If the fly frame has a comment, this points to the old comment anchor.
    std::optional<SwPosition> moCommentAnchor;

    SwWrtShell* mpWrtShell;

    void ImplDestroy();

    SwHandleAnchorNodeChg(const SwHandleAnchorNodeChg&) = delete;
    void operator=(const SwHandleAnchorNodeChg) = delete;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
