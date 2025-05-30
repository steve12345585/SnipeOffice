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

#include <GeneralUndo.hxx>
#include <tools/multisel.hxx>

#include <vector>

#include <com/sun/star/uno/Any.h>
#include <TypeInfo.hxx>
#include <vcl/vclptr.hxx>

namespace dbaui
{
    class OTableRowView;
    class OTableRow;
    class OTableDesignUndoAct : public OCommentUndoAction
    {
    protected:
        VclPtr<OTableRowView> m_pTabDgnCtrl;

        virtual void    Undo() override;
        virtual void    Redo() override;
    public:
        OTableDesignUndoAct(OTableRowView* pOwner, TranslateId pCommentID);
        virtual ~OTableDesignUndoAct() override;
    };

    class OTableEditorCtrl;
    class OTableEditorUndoAct : public OTableDesignUndoAct
    {
    protected:
        VclPtr<OTableEditorCtrl> pTabEdCtrl;

    public:
        OTableEditorUndoAct(OTableEditorCtrl* pOwner, TranslateId pCommentID);
        virtual ~OTableEditorUndoAct() override;
    };

    class OTableDesignCellUndoAct final : public OTableDesignUndoAct
    {
        sal_uInt16     m_nCol;
        sal_Int32      m_nRow;
        css::uno::Any  m_sOldText;
        css::uno::Any  m_sNewText;

        virtual void    Undo() override;
        virtual void    Redo() override;
    public:
        OTableDesignCellUndoAct( OTableRowView* pOwner, sal_Int32 nRowID, sal_uInt16 nColumn );
        virtual ~OTableDesignCellUndoAct() override;
    };

    class OTableEditorTypeSelUndoAct final : public OTableEditorUndoAct
    {
        sal_uInt16          m_nCol;
        sal_Int32       m_nRow;
        TOTypeInfoSP    m_pOldType;
        TOTypeInfoSP    m_pNewType;

        virtual void    Undo() override;
        virtual void    Redo() override;
    public:
        OTableEditorTypeSelUndoAct( OTableEditorCtrl* pOwner, sal_Int32 nRowID, sal_uInt16 nColumn, TOTypeInfoSP _pOldType );
        virtual ~OTableEditorTypeSelUndoAct() override;
    };

    class OTableEditorDelUndoAct final : public OTableEditorUndoAct
    {
        std::vector< std::shared_ptr<OTableRow> > m_aDeletedRows;

        virtual void    Undo() override;
        virtual void    Redo() override;
    public:
        explicit OTableEditorDelUndoAct( OTableEditorCtrl* pOwner );
        virtual ~OTableEditorDelUndoAct() override;
    };

    class OTableEditorInsUndoAct final : public OTableEditorUndoAct
    {
        std::vector< std::shared_ptr<OTableRow> > m_vInsertedRows;
        tools::Long                        m_nInsPos;

        virtual void    Undo() override;
        virtual void    Redo() override;
    public:
        OTableEditorInsUndoAct( OTableEditorCtrl* pOwner,
                                tools::Long nInsertPosition,
                                std::vector<  std::shared_ptr<OTableRow> >&& _vInsertedRows);
        virtual ~OTableEditorInsUndoAct() override;
    };

    class OTableEditorInsNewUndoAct final : public OTableEditorUndoAct
    {
        sal_Int32 m_nInsPos;
        sal_Int32 m_nInsRows;

        virtual void    Undo() override;
        virtual void    Redo() override;
    public:
        OTableEditorInsNewUndoAct( OTableEditorCtrl* pOwner, sal_Int32 nInsertPosition, sal_Int32 nInsertedRows );
        virtual ~OTableEditorInsNewUndoAct() override;
    };

    class OPrimKeyUndoAct final : public OTableEditorUndoAct
    {
        MultiSelection      m_aDelKeys,
                            m_aInsKeys;
        VclPtr<OTableEditorCtrl> m_pEditorCtrl;

        virtual void    Undo() override;
        virtual void    Redo() override;
    public:
        OPrimKeyUndoAct( OTableEditorCtrl* pOwner, const MultiSelection& aDeletedKeys, const MultiSelection& aInsertedKeys );
        virtual ~OPrimKeyUndoAct() override;
    };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
