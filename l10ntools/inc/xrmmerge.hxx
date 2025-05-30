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

#ifndef INCLUDED_L10NTOOLS_INC_XRMMERGE_HXX
#define INCLUDED_L10NTOOLS_INC_XRMMERGE_HXX

#include <memory>
#include <sal/config.h>

#include <fstream>
#include <string_view>

#include <rtl/string.hxx>

#include "export.hxx"
#include "po.hxx"

/// Parser for *.xrm and description.xml files
class XRMResParser
{
private:
    OString sGID;

    bool bError;
    bool bText;

    OString sCurrentOpenTag;
    OString sCurrentCloseTag;
    OString sCurrentText;
protected:
    static OString GetAttribute( const OString &rToken, std::string_view rAttribute );
    static void Error( const OString &rError );

    virtual void Output( const OString& rOutput )=0;
    virtual void WorkOnDesc(
        const OString &rOpenTag,
        OString &rText
    )=0;
    virtual void WorkOnText(
        const OString &rOpenTag,
        OString &rText
    )=0;
    virtual void EndOfText(
        const OString &rOpenTag,
        const OString &rCloseTag
    )=0;

    const OString& GetGID() const { return sGID; }

public:
    XRMResParser();
    virtual ~XRMResParser();

    void Execute( int nToken, char * pToken );

    void SetError() { bError = true; }
    bool GetError() const { return bError; }
};


/// Export strings from *.xrm and description.xml files
class XRMResExport final : public XRMResParser
{
private:
    std::unique_ptr<ResData> pResData;
    OString sPath;
    PoOfstream pOutputStream;

    void WorkOnDesc(
        const OString &rOpenTag,
        OString &rText
    ) override;
    void WorkOnText(
        const OString &rOpenTag,
        OString &rText
    ) override;
    void EndOfText(
        const OString &rOpenTag,
        const OString &rCloseTag
    ) override;
    void Output( const OString& rOutput ) override;

public:
    XRMResExport(
        const OString &rOutputFile,
        OString sFilePath
    );
    virtual ~XRMResExport() override;
};


/// Merge strings to *.xrm and description.xml files
class XRMResMerge final : public XRMResParser
{
private:
    std::unique_ptr<MergeDataFile> pMergeDataFile;
    OString sFilename;
    std::unique_ptr<ResData> pResData;
    std::ofstream pOutputStream;
    std::vector<OString> aLanguages;

    void WorkOnDesc(
        const OString &rOpenTag,
        OString &rText
    ) override;
    void WorkOnText(
        const OString &rOpenTag,
        OString &rText
    ) override;
    void EndOfText(
        const OString &rOpenTag,
        const OString &rCloseTag
    ) override;
    void Output( const OString& rOutput ) override;
public:
    XRMResMerge(
        const OString &rMergeSource,
        const OString &rOutputFile,
        OString sFilename
    );
    virtual ~XRMResMerge() override;
};

#endif // INCLUDED_L10NTOOLS_INC_XRMMERGE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
