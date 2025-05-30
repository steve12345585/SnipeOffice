/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <node.hxx>

/**
 Base class implementing writing of formulas to Word.
 */
class SmWordExportBase
{
public:
    explicit SmWordExportBase(const SmNode* pIn);
    virtual ~SmWordExportBase();

protected:
    void HandleNode(const SmNode* pNode, int nLevel);
    void HandleAllSubNodes(const SmNode* pNode, int nLevel);
    void HandleTable(const SmNode* pNode, int nLevel);
    virtual void HandleVerticalStack(const SmNode* pNode, int nLevel) = 0;
    virtual void HandleText(const SmNode* pNode, int nLevel) = 0;
    void HandleMath(const SmNode* pNode, int nLevel);
    virtual void HandleFractions(const SmNode* pNode, int nLevel, const char* type) = 0;
    void HandleUnaryOperation(const SmUnHorNode* pNode, int nLevel);
    void HandleBinaryOperation(const SmBinHorNode* pNode, int nLevel);
    virtual void HandleRoot(const SmRootNode* pNode, int nLevel) = 0;
    virtual void HandleAttribute(const SmAttributeNode* pNode, int nLevel) = 0;
    virtual void HandleOperator(const SmOperNode* pNode, int nLevel) = 0;
    void HandleSubSupScript(const SmSubSupNode* pNode, int nLevel);
    virtual void HandleSubSupScriptInternal(const SmSubSupNode* pNode, int nLevel, int flags) = 0;
    virtual void HandleMatrix(const SmMatrixNode* pNode, int nLevel) = 0;
    virtual void HandleBrace(const SmBraceNode* pNode, int nLevel) = 0;
    virtual void HandleVerticalBrace(const SmVerticalBraceNode* pNode, int nLevel) = 0;
    virtual void HandleBlank() = 0;
    const SmNode* GetTree() const { return m_pTree; }

private:
    const SmNode* const m_pTree;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
