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

#include <com/sun/star/document/XUndoAction.hpp>

#include <rtl/ref.hxx>
#include <rtl/ustring.hxx>
#include <comphelper/compbase.hxx>

#include <memory>

class SdrUndoAction;

namespace chart
{
class ChartModel;
class ChartModelClone;

namespace impl
{

typedef comphelper::WeakComponentImplHelper< css::document::XUndoAction > UndoElement_TBase;

class UndoElement final : public UndoElement_TBase
{
public:
    /** creates a new undo action

        @param i_actionString
            is the title of the Undo action
        @param i_documentModel
            is the actual document model which the undo actions operates on
        @param i_modelClone
            is the cloned model from before the changes, which the Undo action represents, have been applied.
            Upon <member>invoking</member>, the clone model is applied to the document model.
    */
    UndoElement( OUString  i_actionString,
                 rtl::Reference<::chart::ChartModel> i_documentModel,
                 std::shared_ptr< ChartModelClone > i_modelClone
               );
    virtual ~UndoElement() override;

    UndoElement(const UndoElement&) = delete;
    const UndoElement& operator=(const UndoElement&) = delete;

    // XUndoAction
    virtual OUString SAL_CALL getTitle() override;
    virtual void SAL_CALL undo(  ) override;
    virtual void SAL_CALL redo(  ) override;

    // WeakComponentImplHelper
    virtual void disposing(std::unique_lock<std::mutex>&) override;

private:
    void    impl_toggleModelState();

private:
    OUString                                      m_sActionString;
    rtl::Reference<::chart::ChartModel>           m_xDocumentModel;
    std::shared_ptr< ChartModelClone >            m_pModelClone;
};

typedef comphelper::WeakComponentImplHelper< css::document::XUndoAction > ShapeUndoElement_TBase;
class ShapeUndoElement final : public ShapeUndoElement_TBase
{
public:
    explicit ShapeUndoElement( std::unique_ptr<SdrUndoAction> xSdrUndoAction );
    virtual ~ShapeUndoElement() override;

    // XUndoAction
    virtual OUString SAL_CALL getTitle() override;
    virtual void SAL_CALL undo(  ) override;
    virtual void SAL_CALL redo(  ) override;

private:
    std::unique_ptr<SdrUndoAction> m_xAction;
};

} // namespace impl
} //  namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
