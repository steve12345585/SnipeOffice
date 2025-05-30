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

#ifndef INCLUDED_SHELL_INC_INTERNAL_PROPSHEETS_HXX
#define INCLUDED_SHELL_INC_INTERNAL_PROPSHEETS_HXX

#include "metainforeader.hxx"

#include <shlobj.h>
#include <string>
#include <memory>
#include "filepath.hxx"

class CPropertySheet : public IShellExtInit, public IShellPropSheetExt
{
public:
    CPropertySheet(LONG RefCnt = 1);

    virtual ~CPropertySheet();


    // IUnknown methods


    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID riid,
            void __RPC_FAR *__RPC_FAR *ppvObject) override;

    virtual ULONG STDMETHODCALLTYPE AddRef() override;

    virtual ULONG STDMETHODCALLTYPE Release() override;


    // IShellExtInit


    virtual HRESULT STDMETHODCALLTYPE Initialize(
        LPCITEMIDLIST pidlFolder, IDataObject * lpdobj, HKEY hkeyProgID) override;


    // IShellPropSheetExt


    virtual HRESULT STDMETHODCALLTYPE AddPages(LPFNSVADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam) override;

    virtual HRESULT STDMETHODCALLTYPE ReplacePage(
        EXPPS uPageID, LPFNSVADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam) override;

private:
    // Windows callback functions
    static UINT CALLBACK PropPageSummaryCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
    static bool CALLBACK PropPageSummaryProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK PropPageStatisticsProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);


    // Helper functions
    void InitPropPageSummary(HWND hwnd, LPPROPSHEETPAGE lppsp);
    void InitPropPageStatistics(HWND hwnd, LPPROPSHEETPAGE lppsp);

private:
    LONG m_RefCnt;
    std::wstring m_FileName;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
