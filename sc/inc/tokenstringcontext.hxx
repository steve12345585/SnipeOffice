/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "compiler.hxx"

#include <unordered_map>

class ScDocument;

namespace sc
{
/**
 * Context for creating string from an array of formula tokens, used in
 * ScTokenArray::CreateString().  You can re-use the same string context
 * between multiple CreateString() calls as long as the document content is
 * unmodified.
 */
struct SC_DLLPUBLIC TokenStringContext
{
    typedef std::unordered_map<sal_uInt16, OUString> IndexNameMapType;
    typedef std::unordered_map<size_t, std::vector<OUString>> IndexNamesMapType;
    typedef std::unordered_map<SCTAB, IndexNameMapType> TabIndexMapType;

    formula::FormulaGrammar::Grammar meGram;
    formula::FormulaCompiler::OpCodeMapPtr mxOpCodeMap;
    const ScCompiler::Convention* mpRefConv;
    OUString maErrRef;

    std::vector<OUString> maTabNames;
    IndexNameMapType maGlobalRangeNames;
    TabIndexMapType maSheetRangeNames;
    IndexNameMapType maNamedDBs;

    std::vector<OUString> maExternalFileNames;
    IndexNamesMapType maExternalCachedTabNames;

    TokenStringContext(const ScDocument& rDoc, formula::FormulaGrammar::Grammar eGram);
};

class SC_DLLPUBLIC CompileFormulaContext
{
    ScDocument& mrDoc;
    formula::FormulaGrammar::Grammar meGram;
    std::vector<OUString> maTabNames;
    // Used in FormulaCompiler::CreateStringFromToken(), set by ScXMLExport::WriteCell()
    std::optional<SvtSaveOptions::ODFSaneDefaultVersion> m_oODFSavingVersion;

    void updateTabNames();

public:
    CompileFormulaContext(ScDocument& rDoc);
    CompileFormulaContext(ScDocument& rDoc, formula::FormulaGrammar::Grammar eGram);

    formula::FormulaGrammar::Grammar getGrammar() const { return meGram; }
    void setGrammar(formula::FormulaGrammar::Grammar eGram);

    void setODFSavingVersion(SvtSaveOptions::ODFSaneDefaultVersion eDefaultVersion)
    {
        m_oODFSavingVersion = eDefaultVersion;
    }
    std::optional<SvtSaveOptions::ODFSaneDefaultVersion> getODFSavingVersion()
    {
        return m_oODFSavingVersion;
    }

    const std::vector<OUString>& getTabNames() const { return maTabNames; }

    ScDocument& getDoc() { return mrDoc; }
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
