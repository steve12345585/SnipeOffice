/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <documentimport.hxx>
#include <document.hxx>
#include <table.hxx>
#include <column.hxx>
#include <formulacell.hxx>
#include <docoptio.hxx>
#include <mtvelements.hxx>
#include <tokenarray.hxx>
#include <stringutil.hxx>
#include <compiler.hxx>
#include <paramisc.hxx>
#include <listenercontext.hxx>
#include <attarray.hxx>
#include <sharedformula.hxx>
#include <bcaslot.hxx>
#include <scopetools.hxx>
#include <numformat.hxx>

#include <o3tl/safeint.hxx>
#include <svl/sharedstringpool.hxx>
#include <svl/languageoptions.hxx>
#include <comphelper/configuration.hxx>
#include <unordered_map>

namespace {

struct ColAttr
{
    bool mbLatinNumFmtOnly;

    ColAttr() : mbLatinNumFmtOnly(false) {}
};

struct TabAttr
{
    std::vector<ColAttr> maCols;
};

}

struct ScDocumentImportImpl
{
    ScDocument& mrDoc;
    sc::StartListeningContext maListenCxt;
    std::vector<sc::TableColumnBlockPositionSet> maBlockPosSet;
    SvtScriptType mnDefaultScriptNumeric;
    bool mbFuzzing;
    std::vector<TabAttr> maTabAttrs;
    std::unordered_map<sal_uInt32, bool> maIsLatinScriptMap;

    explicit ScDocumentImportImpl(ScDocument& rDoc) :
        mrDoc(rDoc),
        maListenCxt(rDoc),
        mnDefaultScriptNumeric(SvtScriptType::UNKNOWN),
        mbFuzzing(comphelper::IsFuzzing())
    {}

    bool isValid( size_t nTab, size_t nCol )
    {
        return (nTab <= o3tl::make_unsigned(MAXTAB) && nCol <= o3tl::make_unsigned(mrDoc.MaxCol()));
    }

    ColAttr* getColAttr( size_t nTab, size_t nCol )
    {
        if (!isValid(nTab, nCol))
            return nullptr;

        if (nTab >= maTabAttrs.size())
            maTabAttrs.resize(nTab+1);

        TabAttr& rTab = maTabAttrs[nTab];
        if (nCol >= rTab.maCols.size())
            rTab.maCols.resize(nCol+1);

        return &rTab.maCols[nCol];
    }

    sc::ColumnBlockPosition* getBlockPosition( SCTAB nTab, SCCOL nCol )
    {
        if (!isValid(nTab, nCol))
            return nullptr;

        if (o3tl::make_unsigned(nTab) >= maBlockPosSet.size())
        {
            for (SCTAB i = maBlockPosSet.size(); i <= nTab; ++i)
                maBlockPosSet.emplace_back(mrDoc, i);
        }

        sc::TableColumnBlockPositionSet& rTab = maBlockPosSet[nTab];
        return rTab.getBlockPosition(nCol);
    }

    void invalidateBlockPositionSet(SCTAB nTab)
    {
        if (o3tl::make_unsigned(nTab) >= maBlockPosSet.size())
            return;

        sc::TableColumnBlockPositionSet& rTab = maBlockPosSet[nTab];
        rTab.invalidate();
    }

    void initForSheets()
    {
        size_t n = mrDoc.GetTableCount();
        for (size_t i = maBlockPosSet.size(); i < n; ++i)
            maBlockPosSet.emplace_back(mrDoc, i);

        if (maTabAttrs.size() < n)
            maTabAttrs.resize(n);
    }
};

ScDocumentImport::Attrs::Attrs() : mbLatinNumFmtOnly(false) {}

ScDocumentImport::Attrs::~Attrs() {}

ScDocumentImport::ScDocumentImport(ScDocument& rDoc) : mpImpl(new ScDocumentImportImpl(rDoc)) {}

ScDocumentImport::~ScDocumentImport()
{
}

ScDocument& ScDocumentImport::getDoc()
{
    return mpImpl->mrDoc;
}

const ScDocument& ScDocumentImport::getDoc() const
{
    return mpImpl->mrDoc;
}

void ScDocumentImport::initForSheets()
{
    mpImpl->initForSheets();
}

void ScDocumentImport::setDefaultNumericScript(SvtScriptType nScript)
{
    mpImpl->mnDefaultScriptNumeric = nScript;
}

void ScDocumentImport::setCellStyleToSheet(SCTAB nTab, const ScStyleSheet& rStyle)
{
    ScTable* pTab = mpImpl->mrDoc.FetchTable(nTab);
    if (!pTab)
        return;

    pTab->ApplyStyleArea(0, 0, getDoc().MaxCol(), getDoc().MaxRow(), rStyle);
}

SCTAB ScDocumentImport::getSheetIndex(const OUString& rName) const
{
    SCTAB nTab = -1;
    if (!mpImpl->mrDoc.GetTable(rName, nTab))
        return -1;

    return nTab;
}

SCTAB ScDocumentImport::getSheetCount() const
{
    return mpImpl->mrDoc.maTabs.size();
}

bool ScDocumentImport::appendSheet(const OUString& rName)
{
    SCTAB nTabCount = mpImpl->mrDoc.maTabs.size();
    if (!ValidTab(nTabCount))
        return false;

    mpImpl->mrDoc.maTabs.emplace_back(new ScTable(mpImpl->mrDoc, nTabCount, rName));
    return true;
}

void ScDocumentImport::setSheetName(SCTAB nTab, const OUString& rName)
{
    mpImpl->mrDoc.SetTabNameOnLoad(nTab, rName);
}

void ScDocumentImport::setOriginDate(sal_uInt16 nYear, sal_uInt16 nMonth, sal_uInt16 nDay)
{
    if (!mpImpl->mrDoc.pDocOptions)
        mpImpl->mrDoc.pDocOptions.reset( new ScDocOptions );

    mpImpl->mrDoc.pDocOptions->SetDate(nDay, nMonth, nYear);
}

void ScDocumentImport::invalidateBlockPositionSet(SCTAB nTab)
{
    mpImpl->invalidateBlockPositionSet(nTab);
}

void ScDocumentImport::setAutoInput(const ScAddress& rPos, const OUString& rStr, const ScSetStringParam* pStringParam)
{
    ScTable* pTab = mpImpl->mrDoc.FetchTable(rPos.Tab());
    if (!pTab)
        return;

    sc::ColumnBlockPosition* pBlockPos = mpImpl->getBlockPosition(rPos.Tab(), rPos.Col());

    if (!pBlockPos)
        return;

    // If ScSetStringParam was given, ScColumn::ParseString() shall take care
    // of checking. Ensure caller said so.
    assert(!pStringParam || pStringParam->mbCheckLinkFormula);

    ScCellValue aCell;
    pTab->aCol[rPos.Col()].ParseString(
        aCell, rPos.Row(), rPos.Tab(), rStr, mpImpl->mrDoc.GetAddressConvention(), pStringParam);

    sc::CellStoreType& rCells = pTab->aCol[rPos.Col()].maCells;
    switch (aCell.getType())
    {
        case CELLTYPE_STRING:
            // string is copied.
            pBlockPos->miCellPos = rCells.set(pBlockPos->miCellPos, rPos.Row(), *aCell.getSharedString());
        break;
        case CELLTYPE_EDIT:
            // Cell takes the ownership of the text object.
            pBlockPos->miCellPos = rCells.set(pBlockPos->miCellPos, rPos.Row(), aCell.releaseEditText());
        break;
        case CELLTYPE_VALUE:
            pBlockPos->miCellPos = rCells.set(pBlockPos->miCellPos, rPos.Row(), aCell.getDouble());
        break;
        case CELLTYPE_FORMULA:
            if (!pStringParam)
                mpImpl->mrDoc.CheckLinkFormulaNeedingCheck( *aCell.getFormula()->GetCode());
            // This formula cell instance is directly placed in the document without copying.
            pBlockPos->miCellPos = rCells.set(pBlockPos->miCellPos, rPos.Row(), aCell.releaseFormula());
        break;
        default:
            pBlockPos->miCellPos = rCells.set_empty(pBlockPos->miCellPos, rPos.Row(), rPos.Row());
    }
}

void ScDocumentImport::setNumericCell(const ScAddress& rPos, double fVal)
{
    ScTable* pTab = mpImpl->mrDoc.FetchTable(rPos.Tab());
    if (!pTab)
        return;

    sc::ColumnBlockPosition* pBlockPos = mpImpl->getBlockPosition(rPos.Tab(), rPos.Col());

    if (!pBlockPos)
        return;

    sc::CellStoreType& rCells = pTab->aCol[rPos.Col()].maCells;
    pBlockPos->miCellPos = rCells.set(pBlockPos->miCellPos, rPos.Row(), fVal);
}

void ScDocumentImport::setStringCell(const ScAddress& rPos, const OUString& rStr)
{
    ScTable* pTab = mpImpl->mrDoc.FetchTable(rPos.Tab());
    if (!pTab)
        return;

    sc::ColumnBlockPosition* pBlockPos = mpImpl->getBlockPosition(rPos.Tab(), rPos.Col());

    if (!pBlockPos)
        return;

    svl::SharedString aSS = mpImpl->mrDoc.GetSharedStringPool().intern(rStr);
    if (!aSS.getData())
        return;

    sc::CellStoreType& rCells = pTab->aCol[rPos.Col()].maCells;
    pBlockPos->miCellPos = rCells.set(pBlockPos->miCellPos, rPos.Row(), aSS);
}

void ScDocumentImport::setEditCell(const ScAddress& rPos, std::unique_ptr<EditTextObject> pEditText)
{
    ScTable* pTab = mpImpl->mrDoc.FetchTable(rPos.Tab());
    if (!pTab)
        return;

    sc::ColumnBlockPosition* pBlockPos = mpImpl->getBlockPosition(rPos.Tab(), rPos.Col());

    if (!pBlockPos)
        return;

    pEditText->NormalizeString(mpImpl->mrDoc.GetSharedStringPool());
    sc::CellStoreType& rCells = pTab->aCol[rPos.Col()].maCells;
    pBlockPos->miCellPos = rCells.set(pBlockPos->miCellPos, rPos.Row(), pEditText.release());
}

void ScDocumentImport::setFormulaCell(
    const ScAddress& rPos, const OUString& rFormula, formula::FormulaGrammar::Grammar eGrammar,
    const double* pResult )
{
    ScTable* pTab = mpImpl->mrDoc.FetchTable(rPos.Tab());
    if (!pTab)
        return;

    sc::ColumnBlockPosition* pBlockPos = mpImpl->getBlockPosition(rPos.Tab(), rPos.Col());

    if (!pBlockPos)
        return;

    std::unique_ptr<ScFormulaCell> pFC =
        std::make_unique<ScFormulaCell>(mpImpl->mrDoc, rPos, rFormula, eGrammar);

    mpImpl->mrDoc.CheckLinkFormulaNeedingCheck( *pFC->GetCode());

    if (pResult)
    {
        // Set cached result to this formula cell.
        pFC->SetResultDouble(*pResult);
    }

    sc::CellStoreType& rCells = pTab->aCol[rPos.Col()].maCells;
    pBlockPos->miCellPos =
        rCells.set(pBlockPos->miCellPos, rPos.Row(), pFC.release());
}

void ScDocumentImport::setFormulaCell(
    const ScAddress& rPos, const OUString& rFormula, formula::FormulaGrammar::Grammar eGrammar,
    const OUString& rResult )
{
    ScTable* pTab = mpImpl->mrDoc.FetchTable(rPos.Tab());
    if (!pTab)
        return;

    sc::ColumnBlockPosition* pBlockPos = mpImpl->getBlockPosition(rPos.Tab(), rPos.Col());

    if (!pBlockPos)
        return;

    std::unique_ptr<ScFormulaCell> pFC =
        std::make_unique<ScFormulaCell>(mpImpl->mrDoc, rPos, rFormula, eGrammar);

    mpImpl->mrDoc.CheckLinkFormulaNeedingCheck( *pFC->GetCode());

    // Set cached result to this formula cell.
    pFC->SetHybridString(mpImpl->mrDoc.GetSharedStringPool().intern(rResult));

    sc::CellStoreType& rCells = pTab->aCol[rPos.Col()].maCells;
    pBlockPos->miCellPos =
        rCells.set(pBlockPos->miCellPos, rPos.Row(), pFC.release());
}

void ScDocumentImport::setFormulaCell(const ScAddress& rPos, std::unique_ptr<ScTokenArray> pArray)
{
    ScTable* pTab = mpImpl->mrDoc.FetchTable(rPos.Tab());
    if (!pTab)
        return;

    sc::ColumnBlockPosition* pBlockPos = mpImpl->getBlockPosition(rPos.Tab(), rPos.Col());

    if (!pBlockPos)
        return;

    std::unique_ptr<ScFormulaCell> pFC =
        std::make_unique<ScFormulaCell>(mpImpl->mrDoc, rPos, std::move(pArray));

    mpImpl->mrDoc.CheckLinkFormulaNeedingCheck( *pFC->GetCode());

    sc::CellStoreType& rCells = pTab->aCol[rPos.Col()].maCells;
    pBlockPos->miCellPos =
        rCells.set(pBlockPos->miCellPos, rPos.Row(), pFC.release());
}

void ScDocumentImport::setFormulaCell(const ScAddress& rPos, ScFormulaCell* pCell)
{
    ScTable* pTab = mpImpl->mrDoc.FetchTable(rPos.Tab());
    if (!pTab)
        return;

    sc::ColumnBlockPosition* pBlockPos = mpImpl->getBlockPosition(rPos.Tab(), rPos.Col());

    if (!pBlockPos)
        return;

    if (pCell)
        mpImpl->mrDoc.CheckLinkFormulaNeedingCheck( *pCell->GetCode());

    sc::CellStoreType& rCells = pTab->aCol[rPos.Col()].maCells;

    sc::CellStoreType::position_type aPos = rCells.position(rPos.Row());
    if (aPos.first != rCells.end() && aPos.first->type == sc::element_type_formula)
    {
        ScFormulaCell* p = sc::formula_block::at(*aPos.first->data, aPos.second);
        sc::SharedFormulaUtil::unshareFormulaCell(aPos, *p);
    }

    pBlockPos->miCellPos =
        rCells.set(pBlockPos->miCellPos, rPos.Row(), pCell);
}

void ScDocumentImport::setMatrixCells(
    const ScRange& rRange, const ScTokenArray& rArray, formula::FormulaGrammar::Grammar eGram)
{
    const ScAddress& rBasePos = rRange.aStart;

    ScTable* pTab = mpImpl->mrDoc.FetchTable(rBasePos.Tab());
    if (!pTab)
        return;

    sc::ColumnBlockPosition* pBlockPos = mpImpl->getBlockPosition(rBasePos.Tab(), rBasePos.Col());

    if (!pBlockPos)
        return;

    if (comphelper::IsFuzzing()) //just too slow
        return;

    sc::CellStoreType& rCells = pTab->aCol[rBasePos.Col()].maCells;

    // Set the master cell.
    ScFormulaCell* pCell = new ScFormulaCell(mpImpl->mrDoc, rBasePos, rArray, eGram, ScMatrixMode::Formula);

    mpImpl->mrDoc.CheckLinkFormulaNeedingCheck( *pCell->GetCode());

    pBlockPos->miCellPos =
        rCells.set(pBlockPos->miCellPos, rBasePos.Row(), pCell);

    // Matrix formulas currently need re-calculation on import.
    pCell->SetMatColsRows(
        rRange.aEnd.Col()-rRange.aStart.Col()+1, rRange.aEnd.Row()-rRange.aStart.Row()+1);

    // Set the reference cells.
    ScSingleRefData aRefData;
    aRefData.InitFlags();
    aRefData.SetColRel(true);
    aRefData.SetRowRel(true);
    aRefData.SetTabRel(true);
    aRefData.SetAddress(mpImpl->mrDoc.GetSheetLimits(), rBasePos, rBasePos);

    ScTokenArray aArr(mpImpl->mrDoc); // consists only of one single reference token.
    formula::FormulaToken* t = aArr.AddMatrixSingleReference(aRefData);

    ScAddress aPos = rBasePos;
    for (SCROW nRow = rRange.aStart.Row()+1; nRow <= rRange.aEnd.Row(); ++nRow)
    {
        // Token array must be cloned so that each formula cell receives its own copy.
        aPos.SetRow(nRow);
        // Reference in each cell must point to the origin cell relative to the current cell.
        aRefData.SetAddress(mpImpl->mrDoc.GetSheetLimits(), rBasePos, aPos);
        *t->GetSingleRef() = aRefData;
        ScTokenArray aTokArr(aArr.CloneValue());
        pCell = new ScFormulaCell(mpImpl->mrDoc, aPos, aTokArr, eGram, ScMatrixMode::Reference);
        pBlockPos->miCellPos =
            rCells.set(pBlockPos->miCellPos, aPos.Row(), pCell);
    }

    for (SCCOL nCol = rRange.aStart.Col()+1; nCol <= rRange.aEnd.Col(); ++nCol)
    {
        pBlockPos = mpImpl->getBlockPosition(rBasePos.Tab(), nCol);
        if (!pBlockPos)
            return;

        sc::CellStoreType& rColCells = pTab->aCol[nCol].maCells;

        aPos.SetCol(nCol);
        for (SCROW nRow = rRange.aStart.Row(); nRow <= rRange.aEnd.Row(); ++nRow)
        {
            aPos.SetRow(nRow);
            aRefData.SetAddress(mpImpl->mrDoc.GetSheetLimits(), rBasePos, aPos);
            *t->GetSingleRef() = aRefData;
            ScTokenArray aTokArr(aArr.CloneValue());
            pCell = new ScFormulaCell(mpImpl->mrDoc, aPos, aTokArr, eGram, ScMatrixMode::Reference);
            pBlockPos->miCellPos =
                rColCells.set(pBlockPos->miCellPos, aPos.Row(), pCell);
        }
    }
}

void ScDocumentImport::setTableOpCells(const ScRange& rRange, const ScTabOpParam& rParam)
{
    SCTAB nTab = rRange.aStart.Tab();
    SCCOL nCol1 = rRange.aStart.Col();
    SCROW nRow1 = rRange.aStart.Row();
    SCCOL nCol2 = rRange.aEnd.Col();
    SCROW nRow2 = rRange.aEnd.Row();

    ScTable* pTab = mpImpl->mrDoc.FetchTable(nTab);
    if (!pTab)
        return;

    ScDocument& rDoc = mpImpl->mrDoc;
    ScRefAddress aRef;
    OUStringBuffer aFormulaBuf("="
        + ScCompiler::GetNativeSymbol(ocTableOp)
        + ScCompiler::GetNativeSymbol(ocOpen));

    OUString aSep = ScCompiler::GetNativeSymbol(ocSep);
    if (rParam.meMode == ScTabOpParam::Column) // column only
    {
        aRef.Set(rParam.aRefFormulaCell.GetAddress(), true, false, false);
        aFormulaBuf.append(aRef.GetRefString(rDoc, nTab)
            + aSep
            + rParam.aRefColCell.GetRefString(rDoc, nTab)
            + aSep);
        aRef.Set(nCol1, nRow1, nTab, false, true, true);
        aFormulaBuf.append(aRef.GetRefString(rDoc, nTab));
        nCol1++;
        nCol2 = std::min( nCol2, static_cast<SCCOL>(rParam.aRefFormulaEnd.Col() -
                    rParam.aRefFormulaCell.Col() + nCol1 + 1));
    }
    else if (rParam.meMode == ScTabOpParam::Row) // row only
    {
        aRef.Set(rParam.aRefFormulaCell.GetAddress(), false, true, false);
        aFormulaBuf.append(aRef.GetRefString(rDoc, nTab)
            + aSep
            + rParam.aRefRowCell.GetRefString(rDoc, nTab)
            + aSep);
        aRef.Set(nCol1, nRow1, nTab, true, false, true);
        aFormulaBuf.append(aRef.GetRefString(rDoc, nTab));
        ++nRow1;
        nRow2 = std::min(
            nRow2, rParam.aRefFormulaEnd.Row() - rParam.aRefFormulaCell.Row() + nRow1 + 1);
    }
    else // both
    {
        aFormulaBuf.append(rParam.aRefFormulaCell.GetRefString(rDoc, nTab)
            + aSep
            + rParam.aRefColCell.GetRefString(rDoc, nTab)
            + aSep);
        aRef.Set(nCol1, nRow1 + 1, nTab, false, true, true);
        aFormulaBuf.append(aRef.GetRefString(rDoc, nTab)
            + aSep
            + rParam.aRefRowCell.GetRefString(rDoc, nTab)
            + aSep);
        aRef.Set(nCol1 + 1, nRow1, nTab, true, false, true);
        aFormulaBuf.append(aRef.GetRefString(rDoc, nTab));
        ++nCol1;
        ++nRow1;
    }

    aFormulaBuf.append(ScCompiler::GetNativeSymbol(ocClose));

    ScFormulaCell aRefCell(
        rDoc, ScAddress(nCol1, nRow1, nTab), aFormulaBuf.makeStringAndClear(),
        formula::FormulaGrammar::GRAM_NATIVE, ScMatrixMode::NONE);

    for (SCCOL nCol = nCol1; nCol <= nCol2; ++nCol)
    {
        sc::ColumnBlockPosition* pBlockPos = mpImpl->getBlockPosition(nTab, nCol);

        if (!pBlockPos)
            // Something went horribly wrong.
            return;

        sc::CellStoreType& rColCells = pTab->aCol[nCol].maCells;

        for (SCROW nRow = nRow1; nRow <= nRow2; ++nRow)
        {
            ScAddress aPos(nCol, nRow, nTab);
            ScFormulaCell* pCell = new ScFormulaCell(aRefCell, rDoc, aPos);
            pBlockPos->miCellPos =
                rColCells.set(pBlockPos->miCellPos, nRow, pCell);
        }
    }
}

void ScDocumentImport::fillDownCells(const ScAddress& rPos, SCROW nFillSize)
{
    ScTable* pTab = mpImpl->mrDoc.FetchTable(rPos.Tab());
    if (!pTab)
        return;

    sc::ColumnBlockPosition* pBlockPos = mpImpl->getBlockPosition(rPos.Tab(), rPos.Col());

    if (!pBlockPos)
        return;

    sc::CellStoreType& rCells = pTab->aCol[rPos.Col()].maCells;
    ScRefCellValue aRefCell = pTab->aCol[rPos.Col()].GetCellValue(*pBlockPos, rPos.Row());

    switch (aRefCell.getType())
    {
        case CELLTYPE_VALUE:
        {
            std::vector<double> aCopied(nFillSize, aRefCell.getDouble());
            pBlockPos->miCellPos = rCells.set(
                pBlockPos->miCellPos, rPos.Row()+1, aCopied.begin(), aCopied.end());
            break;
        }
        case CELLTYPE_STRING:
        {
            std::vector<svl::SharedString> aCopied(nFillSize, *aRefCell.getSharedString());
            pBlockPos->miCellPos = rCells.set(
                pBlockPos->miCellPos, rPos.Row()+1, aCopied.begin(), aCopied.end());
            break;
        }
        default:
            break;
    }
}

void ScDocumentImport::setAttrEntries( SCTAB nTab, SCCOL nColStart, SCCOL nColEnd, Attrs&& rAttrs )
{
    ScTable* pTab = mpImpl->mrDoc.FetchTable(nTab);
    if (!pTab)
        return;

    for(SCCOL nCol = nColStart; nCol <= nColEnd; ++nCol )
    {
        ColAttr* pColAttr = mpImpl->getColAttr(nTab, nCol);
        if (pColAttr)
            pColAttr->mbLatinNumFmtOnly = rAttrs.mbLatinNumFmtOnly;
    }

    pTab->SetAttrEntries( nColStart, nColEnd, std::move( rAttrs.mvData ));
}

void ScDocumentImport::setRowsVisible(SCTAB nTab, SCROW nRowStart, SCROW nRowEnd, bool bVisible)
{
    if (!bVisible)
    {
        getDoc().ShowRows(nRowStart, nRowEnd, nTab, false);
        getDoc().SetDrawPageSize(nTab);
        getDoc().UpdatePageBreaks( nTab );
    }
    else
    {
        assert(false);
    }
}

void ScDocumentImport::setMergedCells(SCTAB nTab, SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2)
{
    ScTable* pTab = mpImpl->mrDoc.FetchTable(nTab);
    if (!pTab)
        return;

    pTab->SetMergedCells(nCol1, nRow1, nCol2, nRow2);
}

namespace {

class CellStoreInitializer
{
    // The pimpl pattern here is intentional.
    //
    // The problem with having the attributes in CellStoreInitializer
    // directly is that, as a functor, it might be copied around. In
    // that case miPos in _copied_ object points to maAttrs in the
    // original object, not in the copy. So later, deep in mdds, we end
    // up comparing iterators from different sequences.
    //
    // This could be solved by defining copy constructor and operator=,
    // but given the limited usage of the class, I think it is simpler
    // to let copies share the state.
    struct Impl
    {
        sc::CellTextAttrStoreType maAttrs;
        sc::CellTextAttrStoreType::iterator miPos;
        SvtScriptType mnScriptNumeric;

        explicit Impl(const ScSheetLimits& rSheetLimits, const SvtScriptType nScriptNumeric)
            : maAttrs(rSheetLimits.GetMaxRowCount()), miPos(maAttrs.begin()), mnScriptNumeric(nScriptNumeric)
        {}
    };

    ScDocumentImportImpl& mrDocImpl;
    SCTAB mnTab;
    SCCOL mnCol;

public:
    CellStoreInitializer( ScDocumentImportImpl& rDocImpl, SCTAB nTab, SCCOL nCol ) :
        mrDocImpl(rDocImpl),
        mnTab(nTab),
        mnCol(nCol),
        mpImpl(std::make_shared<Impl>(rDocImpl.mrDoc.GetSheetLimits(), mrDocImpl.mnDefaultScriptNumeric))
    {}

    std::shared_ptr<Impl> mpImpl;

    void operator() (const sc::CellStoreType::value_type& node)
    {
        if (node.type == sc::element_type_empty)
            return;

        // Fill with default values for non-empty cell segments.
        sc::CellTextAttr aDefault;
        switch (node.type)
        {
            case sc::element_type_numeric:
            {
                aDefault.mnScriptType = mpImpl->mnScriptNumeric;
                const ColAttr* p = mrDocImpl.getColAttr(mnTab, mnCol);
                if (p && p->mbLatinNumFmtOnly)
                    aDefault.mnScriptType = SvtScriptType::LATIN;
            }
            break;
            case sc::element_type_formula:
            {
                const ColAttr* p = mrDocImpl.getColAttr(mnTab, mnCol);
                if (p && p->mbLatinNumFmtOnly)
                {
                    // We can assume latin script type if the block only
                    // contains formula cells with numeric results.
                    ScFormulaCell** pp = &sc::formula_block::at(*node.data, 0);
                    ScFormulaCell** ppEnd = pp + node.size;
                    bool bNumResOnly = true;
                    for (; pp != ppEnd; ++pp)
                    {
                        const ScFormulaCell& rCell = **pp;
                        if (!rCell.IsValueNoError())
                        {
                            bNumResOnly = false;
                            break;
                        }
                    }

                    if (bNumResOnly)
                        aDefault.mnScriptType = SvtScriptType::LATIN;
                }
            }
            break;
            default:
                ;
        }

        std::vector<sc::CellTextAttr> aDefaults(node.size, aDefault);
        mpImpl->miPos = mpImpl->maAttrs.set(mpImpl->miPos, node.position, aDefaults.begin(), aDefaults.end());

        if (node.type != sc::element_type_formula)
            return;

        if (mrDocImpl.mbFuzzing) // skip listening when fuzzing
            return;

        // Have all formula cells start listening to the document.
        ScFormulaCell** pp = &sc::formula_block::at(*node.data, 0);
        ScFormulaCell** ppEnd = pp + node.size;
        for (; pp != ppEnd; ++pp)
        {
            ScFormulaCell& rFC = **pp;
            if (rFC.IsSharedTop())
            {
                // Register formula cells as a group.
                sc::SharedFormulaUtil::startListeningAsGroup(mrDocImpl.maListenCxt, pp);
                pp += rFC.GetSharedLength() - 1; // Move to the last one in the group.
            }
            else
                rFC.StartListeningTo(mrDocImpl.maListenCxt);
        }
    }

    void swap(sc::CellTextAttrStoreType& rAttrs)
    {
        mpImpl->maAttrs.swap(rAttrs);
    }
};

}

void ScDocumentImport::finalize()
{
    // Populate the text width and script type arrays in all columns. Also
    // activate all formula cells.
    for (auto& rxTab : mpImpl->mrDoc.maTabs)
    {
        if (!rxTab)
            continue;

        ScTable& rTab = *rxTab;
        SCCOL nNumCols = rTab.aCol.size();
        for (SCCOL nColIdx = 0; nColIdx < nNumCols; ++nColIdx)
            initColumn(rTab.aCol[nColIdx]);
    }

    mpImpl->mrDoc.finalizeOutlineImport();
}

void ScDocumentImport::initColumn(ScColumn& rCol)
{
    rCol.RegroupFormulaCells();

    CellStoreInitializer aFunc(*mpImpl, rCol.nTab, rCol.nCol);
    std::for_each(rCol.maCells.begin(), rCol.maCells.end(), aFunc);
    aFunc.swap(rCol.maCellTextAttrs);

    rCol.CellStorageModified();
}

namespace {

class CellStoreAfterImportBroadcaster
{
public:

    CellStoreAfterImportBroadcaster() {}

    void operator() (const sc::CellStoreType::value_type& node)
    {
        if (node.type == sc::element_type_formula)
        {
            // Broadcast all formula cells marked for recalc.
            ScFormulaCell** pp = &sc::formula_block::at(*node.data, 0);
            ScFormulaCell** ppEnd = pp + node.size;
            for (; pp != ppEnd; ++pp)
            {
                if ((*pp)->GetCode()->IsRecalcModeMustAfterImport())
                    (*pp)->SetDirty();
            }
        }
    }
};

}

void ScDocumentImport::broadcastRecalcAfterImport()
{
    sc::AutoCalcSwitch aACSwitch( mpImpl->mrDoc, false);
    ScBulkBroadcast aBulkBroadcast( mpImpl->mrDoc.GetBASM(), SfxHintId::ScDataChanged);

    for (auto& rxTab : mpImpl->mrDoc.maTabs)
    {
        if (!rxTab)
            continue;

        ScTable& rTab = *rxTab;
        SCCOL nNumCols = rTab.aCol.size();
        for (SCCOL nColIdx = 0; nColIdx < nNumCols; ++nColIdx)
            broadcastRecalcAfterImportColumn(rTab.aCol[nColIdx]);
    }
}

void ScDocumentImport::broadcastRecalcAfterImportColumn(ScColumn& rCol)
{
    CellStoreAfterImportBroadcaster aFunc;
    std::for_each(rCol.maCells.begin(), rCol.maCells.end(), aFunc);
}


bool ScDocumentImport::isLatinScript(const ScPatternAttr& rPatAttr)
{
    SvNumberFormatter* pFormatter = mpImpl->mrDoc.GetFormatTable();
    sal_uInt32 nKey = rPatAttr.GetNumberFormat(pFormatter);
    return isLatinScript(nKey);
}

bool ScDocumentImport::isLatinScript(sal_uInt32 nFormat)
{
    auto it = mpImpl->maIsLatinScriptMap.find(nFormat);
    if (it != mpImpl->maIsLatinScriptMap.end())
        return it->second;
    bool b = sc::NumFmtUtil::isLatinScript(nFormat, mpImpl->mrDoc);
    mpImpl->maIsLatinScriptMap.emplace(nFormat, b);
    return b;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
