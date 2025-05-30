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

#include "ChartModelClone.hxx"

#include <rtl/ustring.hxx>

#include <memory>

namespace com::sun::star::document { class XUndoManager; }

namespace chart
{

/** A guard which does nothing, unless you explicitly call commitAction. In particular, in its destructor, it
    does neither auto-commit nor auto-rollback the model changes.
 */
class UndoGuard
{
public:
    explicit UndoGuard(
        OUString i_undoMessage,
        const css::uno::Reference< css::document::XUndoManager > & i_undoManager,
        const ModelFacet i_facet = E_MODEL
    );
    ~UndoGuard();

    void    commit();
    void    rollback();

protected:
    bool    isActionPosted() const { return m_bActionPosted; }

private:
    void    discardSnapshot();

private:
    rtl::Reference<::chart::ChartModel>                       m_xChartModel;
    const css::uno::Reference< css::document::XUndoManager >  m_xUndoManager;

    std::shared_ptr< ChartModelClone >  m_pDocumentSnapshot;
    OUString                           m_aUndoString;
    bool                                    m_bActionPosted;
};

/** A guard which, in its destructor, restores the model state it found in the constructor. If
    <member>commitAction</member> is called inbetween, the restoration is not performed.
 */
class UndoLiveUpdateGuard : public UndoGuard
{
public:
    explicit UndoLiveUpdateGuard(
        const OUString& i_undoMessage,
        const css::uno::Reference< css::document::XUndoManager > & i_undoManager
    );
    ~UndoLiveUpdateGuard();
};

/** Same as UndoLiveUpdateGuard but with additional storage of the chart's data.
    Only use this if the data has internal data.
 */
class UndoLiveUpdateGuardWithData :
        public UndoGuard
{
public:
    explicit UndoLiveUpdateGuardWithData(
        const OUString& i_undoMessage,
        const css::uno::Reference< css::document::XUndoManager > & i_undoManager
    );
    ~UndoLiveUpdateGuardWithData();
};

class UndoGuardWithSelection : public UndoGuard
{
public:
    explicit UndoGuardWithSelection(
        const OUString& i_undoMessage,
        const css::uno::Reference< css::document::XUndoManager > & i_undoManager
    );
    virtual ~UndoGuardWithSelection();
};

class HiddenUndoContext
{
public:
    explicit HiddenUndoContext(
        const css::uno::Reference< css::document::XUndoManager > & i_undoManager
    );
    ~HiddenUndoContext();

private:
    css::uno::Reference< css::document::XUndoManager >    m_xUndoManager;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
