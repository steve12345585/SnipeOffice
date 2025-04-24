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

#include <sfx2/shell.hxx>
#include <shellid.hxx>
#include <swmodule.hxx>
#include <unotools/caserotate.hxx>

class SwView;
class OutlinerView;

class SwAnnotationShell final : public SfxShell
{
    SwView&     m_rView;
    RotateTransliteration m_aRotateCase;

public:
    SFX_DECL_INTERFACE(SW_ANNOTATIONSHELL)

private:
    /// SfxInterface initializer.
    static void InitInterface_Impl();

public:
                SwAnnotationShell(SwView&);
    virtual     ~SwAnnotationShell() override;

    static void StateDisableItems(SfxItemSet &);
    void        Exec(SfxRequest &);

    void        GetState(SfxItemSet &);
    void        StateInsert(SfxItemSet &rSet);

    void        NoteExec(SfxRequest const &);
    void        GetNoteState(SfxItemSet &);

    void        ExecLingu(SfxRequest &rReq);
    void        GetLinguState(SfxItemSet &);

    void        ExecClpbrd(SfxRequest const &rReq);
    void        StateClpbrd(SfxItemSet &rSet);

    void        ExecTransliteration(SfxRequest const &);
    void        ExecRotateTransliteration(SfxRequest const &);

    void        ExecUndo(SfxRequest &rReq);
    void        StateUndo(SfxItemSet &rSet);

    static void StateStatusLine(SfxItemSet &rSet);

    void        InsertSymbol(SfxRequest& rReq);

    void        ExecSearch(SfxRequest&);
    void        StateSearch(SfxItemSet &);

    virtual SfxUndoManager*
                GetUndoManager() override;

    static SfxItemPool* GetAnnotationPool(SwView const & rV);

private:
    void        ExecPost(const SfxRequest& rReq, sal_uInt16 nEEWhich, SfxItemSet& rNewAttr, OutlinerView* pOLV );
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
