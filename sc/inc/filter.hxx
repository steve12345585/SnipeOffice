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

#include <rtl/textenc.h>
#include <rtl/ustring.hxx>
#include <comphelper/errcode.hxx>

#include "scdllapi.h"
#include <memory>

class SfxMedium;
class SvStream;

class ScAddress;
class ScDocument;
class ScRange;
class SvNumberFormatter;
class ScOrcusFilters;

// for import
enum EXCIMPFORMAT { EIF_AUTO, EIF_BIFF5, EIF_BIFF8, EIF_BIFF_LE4 };

// for export
enum ExportFormatExcel { ExpBiff5, ExpBiff8 };

// These are implemented inside the scfilt library and lazy loaded

class ScEEAbsImport {
  public:
    virtual ~ScEEAbsImport() {}
    virtual ErrCode Read( SvStream& rStream, const OUString& rBaseURL ) = 0;
    virtual ScRange GetRange() = 0;
    virtual void    WriteToDocument(
        bool bSizeColsRows = false, double nOutputFactor = 1.0,
        SvNumberFormatter* pFormatter = nullptr, bool bConvertDate = true,
        bool bConvertScientific = true ) = 0;
};

class SAL_DLLPUBLIC_RTTI ScFormatFilterPlugin {
  public:
    // various import filters
    virtual ErrCode ScImportLotus123( SfxMedium&, ScDocument&, rtl_TextEncoding eSrc ) = 0;
    virtual ErrCode ScImportQuattroPro(SvStream* pStream, ScDocument& rDoc) = 0;
    virtual ErrCode ScImportExcel( SfxMedium&, ScDocument*, const EXCIMPFORMAT ) = 0;
        // eFormat == EIF_AUTO  -> matching filter is used automatically
        // eFormat == EIF_BIFF5 -> only Biff5 stream is read successfully (in an Excel97 doc, too)
        // eFormat == EIF_BIFF8 -> only Biff8 stream is read successfully (only in Excel97 docs)
        // eFormat == EIF_BIFF_LE4 -> only non storage files _might_ be read successfully
    virtual ErrCode ScImportDif( SvStream&, ScDocument*, const ScAddress& rInsPos,
                 const rtl_TextEncoding eSrc ) = 0;
    virtual ErrCode ScImportRTF( SvStream&, const OUString& rBaseURL, ScDocument*, ScRange& rRange ) = 0;
    virtual ErrCode ScImportHTML( SvStream&, const OUString& rBaseURL, ScDocument*, ScRange& rRange, double nOutputFactor,
                                   bool bCalcWidthHeight, SvNumberFormatter* pFormatter, bool bConvertDate,
                                   bool bConvertScientific ) = 0;

    // various import helpers
    virtual std::unique_ptr<ScEEAbsImport> CreateRTFImport( ScDocument* pDoc, const ScRange& rRange ) = 0;
    virtual std::unique_ptr<ScEEAbsImport> CreateHTMLImport( ScDocument* pDocP, const OUString& rBaseURL, const ScRange& rRange ) = 0;
    virtual OUString       GetHTMLRangeNameList( ScDocument& rDoc, const OUString& rOrigName ) = 0;

    // various export filters
    virtual ErrCode ScExportExcel5( SfxMedium&, ScDocument*, ExportFormatExcel eFormat, rtl_TextEncoding eDest ) = 0;
    virtual void ScExportDif( SvStream&, ScDocument*, const ScAddress& rOutPos, const rtl_TextEncoding eDest ) = 0;
    virtual void ScExportDif( SvStream&, ScDocument*, const ScRange& rRange, const rtl_TextEncoding eDest ) = 0;
    virtual void ScExportHTML( SvStream&, const OUString& rBaseURL, ScDocument*, const ScRange& rRange, const rtl_TextEncoding eDest, bool bAll,
                  const OUString& rStreamPath, OUString& rNonConvertibleChars, const OUString& rFilterOptions ) = 0;
    virtual void ScExportRTF( SvStream&, ScDocument*, const ScRange& rRange, const rtl_TextEncoding eDest ) = 0;

    virtual ScOrcusFilters* GetOrcusFilters() = 0;

protected:
    ~ScFormatFilterPlugin() {}
};

// scfilt plugin symbol
extern "C" {
  SAL_DLLPUBLIC_EXPORT ScFormatFilterPlugin * ScFilterCreate();
}

class ScFormatFilter {
    public:
    SC_DLLPUBLIC static ScFormatFilterPlugin &Get();
};

struct LotusContext;

ErrCode ScImportLotus123old(LotusContext& rContext, SvStream&, rtl_TextEncoding eSrc);

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
