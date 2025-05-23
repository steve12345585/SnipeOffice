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

#include <sdundogr.hxx>
#include <tools/long.hxx>

SdUndoGroup::~SdUndoGroup() = default;

bool SdUndoGroup::Merge(SfxUndoAction* pNextAction)
{
    bool bRet = false;

    if (auto pSdUndoAction = dynamic_cast<SdUndoAction*>(pNextAction))
    {
        SdUndoAction* pClone = pSdUndoAction->Clone();

        if (pClone)
        {
            AddAction(pClone);
            bRet = true;
        }
    }

    return bRet;
}

/**
 * Undo, reverse order of execution
 */
void SdUndoGroup::Undo()
{
    ::tools::Long nLast = aCtn.size();
    for (::tools::Long nAction = nLast - 1; nAction >= 0; nAction--)
    {
        aCtn[nAction]->Undo();
    }
}

void SdUndoGroup::Redo()
{
    size_t nLast = aCtn.size();
    for (size_t nAction = 0; nAction < nLast; nAction++)
    {
        aCtn[nAction]->Redo();
    }
}

void SdUndoGroup::AddAction(SdUndoAction* pAction) { aCtn.emplace_back(pAction); }

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
