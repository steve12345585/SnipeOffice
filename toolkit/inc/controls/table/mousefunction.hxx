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

#include "tabletypes.hxx"

#include <salhelper/simplereferenceobject.hxx>

class MouseEvent;

namespace svt::table
{
class ITableControl;

//= FunctionResult

enum FunctionResult
{
    ActivateFunction,
    ContinueFunction,
    DeactivateFunction,

    SkipFunction
};

//= MouseFunction

class MouseFunction : public ::salhelper::SimpleReferenceObject
{
public:
    MouseFunction() {}
    MouseFunction(const MouseFunction&) = delete;
    MouseFunction& operator=(const MouseFunction&) = delete;
    virtual FunctionResult handleMouseMove(ITableControl& i_tableControl, MouseEvent const& i_event)
        = 0;
    virtual FunctionResult handleMouseDown(ITableControl& i_tableControl, MouseEvent const& i_event)
        = 0;
    virtual FunctionResult handleMouseUp(ITableControl& i_tableControl, MouseEvent const& i_event)
        = 0;

protected:
    virtual ~MouseFunction() override {}
};

//= ColumnResize

class ColumnResize final : public MouseFunction
{
public:
    ColumnResize()
        : m_nResizingColumn(COL_INVALID)
    {
    }

public:
    // MouseFunction
    virtual FunctionResult handleMouseMove(ITableControl& i_tableControl,
                                           MouseEvent const& i_event) override;
    virtual FunctionResult handleMouseDown(ITableControl& i_tableControl,
                                           MouseEvent const& i_event) override;
    virtual FunctionResult handleMouseUp(ITableControl& i_tableControl,
                                         MouseEvent const& i_event) override;

private:
    ColPos m_nResizingColumn;
};

//= RowSelection

class RowSelection final : public MouseFunction
{
public:
    RowSelection()
        : m_bActive(false)
    {
    }

public:
    // MouseFunction
    virtual FunctionResult handleMouseMove(ITableControl& i_tableControl,
                                           MouseEvent const& i_event) override;
    virtual FunctionResult handleMouseDown(ITableControl& i_tableControl,
                                           MouseEvent const& i_event) override;
    virtual FunctionResult handleMouseUp(ITableControl& i_tableControl,
                                         MouseEvent const& i_event) override;

private:
    bool m_bActive;
};

//= ColumnSortHandler

class ColumnSortHandler final : public MouseFunction
{
public:
    ColumnSortHandler()
        : m_nActiveColumn(COL_INVALID)
    {
    }

public:
    // MouseFunction
    virtual FunctionResult handleMouseMove(ITableControl& i_tableControl,
                                           MouseEvent const& i_event) override;
    virtual FunctionResult handleMouseDown(ITableControl& i_tableControl,
                                           MouseEvent const& i_event) override;
    virtual FunctionResult handleMouseUp(ITableControl& i_tableControl,
                                         MouseEvent const& i_event) override;

private:
    ColPos m_nActiveColumn;
};

} // namespace svt::table

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
