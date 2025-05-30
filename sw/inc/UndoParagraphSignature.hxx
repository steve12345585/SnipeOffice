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

#include "undobj.hxx"
#include <rtl/ustring.hxx>

#include <com/sun/star/uno/Reference.hxx>

class SwDoc;

namespace com::sun::star::text
{
class XTextContent;
}
namespace com::sun::star::text
{
class XTextField;
}

/// Undo/Redo Paragraph Signature.
class SwUndoParagraphSigning final : public SwUndo
{
private:
    SwDoc& m_rDoc;
    css::uno::Reference<css::text::XTextField> m_xField;
    css::uno::Reference<css::text::XTextContent> m_xParent;
    OUString m_signature;
    OUString m_usage;
    OUString m_display;
    const bool m_bRemove;

public:
    SwUndoParagraphSigning(SwDoc& rDoc, css::uno::Reference<css::text::XTextField> xField,
                           css::uno::Reference<css::text::XTextContent> xParent,
                           const bool bRemove);

    virtual void UndoImpl(::sw::UndoRedoContext&) override;
    virtual void RedoImpl(::sw::UndoRedoContext&) override;
    virtual void RepeatImpl(::sw::RepeatContext&) override;

private:
    void Insert();
    void Remove();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
