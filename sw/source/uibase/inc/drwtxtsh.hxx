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
#ifndef INCLUDED_SW_SOURCE_UIBASE_INC_DRWTXTSH_HXX
#define INCLUDED_SW_SOURCE_UIBASE_INC_DRWTXTSH_HXX

#include <sfx2/shell.hxx>
#include <shellid.hxx>
#include <unotools/caserotate.hxx>

class SdrView;
class SwView;
class SwWrtShell;

/// SfxShell subclass that is used while interacting with the editeng-based text of a shape.
class SwDrawTextShell final : public SfxShell
{
    SwView      &m_rView;
    RotateTransliteration m_aRotateCase;

    SdrView     *m_pSdrView;

    void        SetAttrToMarked(const SfxItemSet& rAttr);
    void        InsertSymbol(SfxRequest& rReq);
    bool    IsTextEdit() const;

public:
    SFX_DECL_INTERFACE(SW_DRWTXTSHELL)

private:
    /// SfxInterface initializer.
    static void InitInterface_Impl();

public:
    SwView     &GetView() { return m_rView; }
    SwWrtShell &GetShell();

                SwDrawTextShell(SwView &rView);
    virtual     ~SwDrawTextShell() override;

    virtual SfxUndoManager*
                GetUndoManager() override;

    static void StateDisableItems(SfxItemSet &);

    void        Execute(SfxRequest &);
    void        ExecDraw(SfxRequest &);
    void        GetStatePropPanelAttr(SfxItemSet &);
    void        GetState(SfxItemSet &);
    void        GetDrawTextCtrlState(SfxItemSet&);

    void        ExecFontWork(SfxRequest const & rReq);
    void        StateFontWork(SfxItemSet& rSet);
    void        ExecFormText(SfxRequest const & rReq);
    void        GetFormTextState(SfxItemSet& rSet);
    void        ExecDrawLingu(SfxRequest const &rReq);
    void        ExecUndo(SfxRequest &rReq);
    void        StateUndo(SfxItemSet &rSet);
    void        ExecClpbrd(SfxRequest const &rReq);
    void        StateClpbrd(SfxItemSet &rSet);
    void        StateInsert(SfxItemSet &rSet);
    void        ExecTransliteration(SfxRequest const &);
    void        ExecRotateTransliteration(SfxRequest const &);

    void        Init();

private:
    void        ExecutePost(const SfxRequest& rReq, sal_uInt16 nEEWhich, SfxItemSet& rNewAttr,
                    const OutlinerView* pOLV, bool bRestoreSelection, const ESelection& rOldSelection);
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
