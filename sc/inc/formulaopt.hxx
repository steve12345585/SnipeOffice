/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <map>
#include <svl/poolitem.hxx>
#include <unotools/configitem.hxx>
#include <formula/grammar.hxx>
#include "scdllapi.h"
#include "calcconfig.hxx"

class SC_DLLPUBLIC ScFormulaOptions
{
private:
    bool bUseEnglishFuncName;     // use English function name even if the locale is not English.
    formula::FormulaGrammar::Grammar eFormulaGrammar;  // formula grammar used to switch different formula syntax
    ScCalcConfig aCalcConfig;
    bool mbWriteCalcConfig;

    OUString aFormulaSepArg;
    OUString aFormulaSepArrayRow;
    OUString aFormulaSepArrayCol;

    ScRecalcOptions meOOXMLRecalc;
    ScRecalcOptions meODFRecalc;
    ScRecalcOptions meReCalcOptiRowHeights;

public:
    ScFormulaOptions();

    void SetDefaults();

    void SetFormulaSyntax( ::formula::FormulaGrammar::Grammar eGram ) { eFormulaGrammar = eGram; }
    ::formula::FormulaGrammar::Grammar GetFormulaSyntax() const { return eFormulaGrammar; }

    ScCalcConfig& GetCalcConfig() { return aCalcConfig; }
    const ScCalcConfig& GetCalcConfig() const { return aCalcConfig; }
    void SetCalcConfig(const ScCalcConfig& rConfig) { aCalcConfig = rConfig; }

    void SetUseEnglishFuncName( bool bVal ) { bUseEnglishFuncName = bVal; }
    bool GetUseEnglishFuncName() const { return bUseEnglishFuncName; }

    void SetWriteCalcConfig( bool bVal ) { mbWriteCalcConfig = bVal; }
    bool GetWriteCalcConfig() const { return mbWriteCalcConfig; }

    void SetFormulaSepArg(const OUString& rSep) { aFormulaSepArg = rSep; }
    const OUString& GetFormulaSepArg() const { return aFormulaSepArg; }

    void SetFormulaSepArrayRow(const OUString& rSep) { aFormulaSepArrayRow = rSep; }
    const OUString& GetFormulaSepArrayRow() const { return aFormulaSepArrayRow; }

    void SetFormulaSepArrayCol(const OUString& rSep) { aFormulaSepArrayCol = rSep; }
    const OUString& GetFormulaSepArrayCol() const { return aFormulaSepArrayCol; }

    void SetOOXMLRecalcOptions( ScRecalcOptions eOpt ) { meOOXMLRecalc = eOpt; }
    ScRecalcOptions GetOOXMLRecalcOptions() const { return meOOXMLRecalc; }

    void SetODFRecalcOptions( ScRecalcOptions eOpt ) { meODFRecalc = eOpt; }
    ScRecalcOptions GetODFRecalcOptions() const { return meODFRecalc; }

    void SetReCalcOptiRowHeights( ScRecalcOptions eOpt ) { meReCalcOptiRowHeights = eOpt; }
    ScRecalcOptions GetReCalcOptiRowHeights() const { return meReCalcOptiRowHeights; }

    void ResetFormulaSeparators();

    static void GetDefaultFormulaSeparators(OUString& rSepArg, OUString& rSepArrayCol, OUString& rSepArrayRow);

    bool               operator== ( const ScFormulaOptions& rOpt ) const;
    bool               operator!= ( const ScFormulaOptions& rOpt ) const;
};

// item for the dialog / options page

class SC_DLLPUBLIC ScTpFormulaItem final : public SfxPoolItem
{
public:
    DECLARE_ITEM_TYPE_FUNCTION(ScTpFormulaItem)
    ScTpFormulaItem( ScFormulaOptions aOpt );
    virtual ~ScTpFormulaItem() override;

    ScTpFormulaItem(ScTpFormulaItem const &) = default;
    ScTpFormulaItem(ScTpFormulaItem &&) = default;
    ScTpFormulaItem & operator =(ScTpFormulaItem const &) = delete; // due to SfxPoolItem
    ScTpFormulaItem & operator =(ScTpFormulaItem &&) = delete; // due to SfxPoolItem

    virtual bool            operator==( const SfxPoolItem& ) const override;
    virtual ScTpFormulaItem* Clone( SfxItemPool *pPool = nullptr ) const override;

    const ScFormulaOptions& GetFormulaOptions() const { return theOptions; }

private:
    ScFormulaOptions theOptions;
};

// config item

class ScFormulaCfg final : public ScFormulaOptions, public utl::ConfigItem
{
    typedef std::map<OUString,sal_uInt16> PropsToIds;
    static css::uno::Sequence<OUString> GetPropertyNames();
    static ScFormulaCfg::PropsToIds GetPropNamesToId();
    void UpdateFromProperties( const css::uno::Sequence<OUString>& rNames );

    virtual void ImplCommit() override;

public:
    ScFormulaCfg();

    void SetOptions( const ScFormulaOptions& rNew );

    virtual void Notify( const css::uno::Sequence< OUString >& aPropertyNames ) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
