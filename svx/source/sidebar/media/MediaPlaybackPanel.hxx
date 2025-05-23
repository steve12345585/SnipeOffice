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

#include <sfx2/sidebar/PanelLayout.hxx>
#include <avmedia/mediaitem.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/sidebar/ControllerItem.hxx>
#include <avmedia/MediaControlBase.hxx>
#include <vcl/idle.hxx>

using namespace css;
using namespace ::com::sun::star::frame;

namespace svx::sidebar {

/** This panel provides media playback control in document
*/
class MediaPlaybackPanel
    : public PanelLayout,
    public ::sfx2::sidebar::ControllerItem::ItemUpdateReceiverInterface,
    public ::avmedia::MediaControlBase
{
public:
    MediaPlaybackPanel (
        weld::Widget* pParent,
        SfxBindings* pBindings);
    static std::unique_ptr<PanelLayout> Create(
        weld::Widget* pParent,
        SfxBindings* pBindings);
    virtual ~MediaPlaybackPanel() override;

protected:
    virtual void UpdateToolBoxes(const avmedia::MediaItem& rMediaItem) override;

private:
    std::unique_ptr< ::avmedia::MediaItem > mpMediaItem;
    ::sfx2::sidebar::ControllerItem         maMediaController;
    Idle            maIdle;
    SfxBindings*    mpBindings;
    void Initialize();
    void Update();
    virtual void NotifyItemUpdate( const sal_uInt16 nSID,
                                    const SfxItemState eState,
                                    const SfxPoolItem* pState) override;

    virtual void GetControlState(
        const sal_uInt16 /*nSId*/,
        boost::property_tree::ptree& /*rState*/) override {};

    DECL_LINK(PlayToolBoxSelectHdl, const OUString&, void);
    DECL_LINK(VolumeSlideHdl, weld::Scale&, void);
    DECL_LINK(SeekHdl, weld::Scale&, void);

    DECL_LINK(TimeoutHdl, Timer*, void);
};


} // end of namespace svx::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
