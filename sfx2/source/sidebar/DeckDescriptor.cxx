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

#include <sidebar/DeckDescriptor.hxx>

namespace sfx2::sidebar {

DeckDescriptor::DeckDescriptor()
    : mbIsEnabled(true),
      mnOrderIndex(10000), // Default value as defined in Sidebar.xcs
      mbExperimental(false)
{
}

DeckDescriptor::DeckDescriptor (const DeckDescriptor& rOther)
    : msTitle(rOther.msTitle),
      msId(rOther.msId),
      msIconURL(rOther.msIconURL),
      msHighContrastIconURL(rOther.msHighContrastIconURL),
      msTitleBarIconURL(rOther.msTitleBarIconURL),
      msHighContrastTitleBarIconURL(rOther.msHighContrastTitleBarIconURL),
      msHelpText(rOther.msHelpText),
      msHelpId(rOther.msHelpId),
      maContextList(rOther.maContextList),
      mbIsEnabled(rOther.mbIsEnabled),
      mnOrderIndex(rOther.mnOrderIndex),
      mbExperimental(rOther.mbExperimental),
      mbHiddenInViewerMode(rOther.mbHiddenInViewerMode),
      mpDeck(rOther.mpDeck)
{
}

DeckDescriptor::~DeckDescriptor()
{
}

} // end of namespace sfx2::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
