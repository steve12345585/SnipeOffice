/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/container/XContainerListener.hpp>

#include <rtl/ref.hxx>
#include <rtl/ustring.hxx>
#include "scdllapi.h"

#include <memory>
#include <unordered_map>

class ScDocument;
class ScFormulaCell;
class ScUserMacroDepTracker;
class VBAProjectListener;

class ScMacroManager
{
public:
    explicit ScMacroManager(ScDocument& rDoc);
    ~ScMacroManager();

    SC_DLLPUBLIC void InitUserFuncData();
    SC_DLLPUBLIC void SetUserFuncVolatile(const OUString& sName, bool isVolatile);
    SC_DLLPUBLIC bool GetUserFuncVolatile(const OUString& sName);

    void AddDependentCell(const OUString& aModuleName, ScFormulaCell* pCell);
    void RemoveDependentCell(const ScFormulaCell* pCell);
    void BroadcastModuleUpdate(const OUString& aModuleName);

private:
    typedef std::unordered_map<OUString, bool> NameBoolMap;
    NameBoolMap mhFuncToVolatile;
    rtl::Reference<VBAProjectListener> mxContainerListener;

    ::std::unique_ptr<ScUserMacroDepTracker> mpDepTracker;
    ScDocument& mrDoc;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
