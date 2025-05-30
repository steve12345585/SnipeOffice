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

#include <svl/itemset.hxx>

#include <sfx2/frmdescr.hxx>
#include <sfx2/app.hxx>
#include <memory>

SfxFrameDescriptor::SfxFrameDescriptor() :
    aMargin( -1, -1 ),
    eScroll( ScrollingMode::Auto ),
    bHasBorder( true ),
    bHasBorderSet( false )
{
}

SfxFrameDescriptor::~SfxFrameDescriptor()
{
}

SfxItemSet* SfxFrameDescriptor::GetArgs()
{
    if( !m_pArgs )
        m_pArgs.reset( new SfxAllItemSet( SfxGetpApp()->GetPool() ) );
    return m_pArgs.get();
}

void SfxFrameDescriptor::SetURL( std::u16string_view rURL )
{
    aURL = INetURLObject(rURL);
    SetActualURL();
}

void SfxFrameDescriptor::SetActualURL()
{
    if ( m_pArgs )
        m_pArgs->ClearItem();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
