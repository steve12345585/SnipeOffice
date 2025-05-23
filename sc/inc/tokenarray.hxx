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

#include <formula/token.hxx>
#include <rtl/ref.hxx>
#include "document.hxx"
#include "scdllapi.h"
#include "types.hxx"
#include "calcmacros.hxx"
#include "address.hxx"
#include "global.hxx"
#include <formula/tokenarray.hxx>

namespace sc {

struct RefUpdateContext;
struct RefUpdateInsertTabContext;
struct RefUpdateDeleteTabContext;
struct RefUpdateMoveTabContext;
struct RefUpdateResult;
struct TokenStringContext;
class ColRowReorderMapType;

}

struct ScRawToken;
struct ScSingleRefData;
struct ScComplexRefData;

class SAL_WARN_UNUSED SAL_DLLPUBLIC_RTTI ScTokenArray final : public formula::FormulaTokenArray
{
    friend class ScCompiler;

    bool ImplGetReference( ScRange& rRange, const ScAddress& rPos, bool bValidOnly ) const;

    // hold a reference to the limits because sometimes our lifetime exceeds the lifetime of the associated ScDocument
    rtl::Reference<ScSheetLimits> mxSheetLimits;
    size_t mnHashValue;
    ScFormulaVectorState meVectorState : 4; // Only 4 bits
    bool mbOpenCLEnabled : 1;
    bool mbThreadingEnabled : 1;

    void CheckForThreading( const formula::FormulaToken& r );

public:
    SC_DLLPUBLIC ScTokenArray(const ScDocument& rDoc);
    ScTokenArray(ScSheetLimits&);
    /** Assignment with incrementing references of FormulaToken entries
        (not copied!) */
    ScTokenArray( const ScTokenArray& ) = default;
    ScTokenArray( ScTokenArray&& ) = default;
    SC_DLLPUBLIC virtual ~ScTokenArray() override;

    bool EqualTokens( const ScTokenArray* pArr2 ) const;

    SC_DLLPUBLIC virtual void Clear() override;
    SC_DLLPUBLIC std::unique_ptr<ScTokenArray> Clone() const;    /// True copy!
    SC_DLLPUBLIC ScTokenArray CloneValue() const;    /// True copy!

    SC_DLLPUBLIC void GenHash();
    size_t GetHash() const { return mnHashValue;}

    ScFormulaVectorState GetVectorState() const { return meVectorState;}
    void ResetVectorState();
    bool IsFormulaVectorDisabled() const;

    /**
     * If the array contains at least one relative row reference or named
     * expression, it's variant. Otherwise invariant.
     */
    bool IsInvariant() const;

    /// Exactly and only one range (valid or deleted)
    SC_DLLPUBLIC bool IsReference( ScRange& rRange, const ScAddress& rPos ) const;
    /// Exactly and only one valid range (no #REF!s)
    SC_DLLPUBLIC bool IsValidReference( ScRange& rRange, const ScAddress& rPos ) const;

                            /** Determines the extent of direct adjacent
                                references. Only use with real functions, e.g.
                                GetOuterFuncOpCode() == ocSum ! */
    bool                    GetAdjacentExtendOfOuterFuncRefs(
                                SCCOLROW& nExtend,
                                const ScAddress& rPos, ScDirection );

    formula::FormulaToken* AddRawToken( const ScRawToken& );
    SC_DLLPUBLIC virtual bool AddFormulaToken(
        const css::sheet::FormulaToken& rToken,
        svl::SharedStringPool& rSPool,
        formula::ExternalReferenceHelper* _pRef) override;
    SC_DLLPUBLIC virtual void CheckToken( const formula::FormulaToken& r ) override;
    SC_DLLPUBLIC virtual formula::FormulaToken* AddOpCode( OpCode eCode ) override;
    /** ScSingleRefToken with ocPush. */
    SC_DLLPUBLIC formula::FormulaToken* AddSingleReference( const ScSingleRefData& rRef );
    /** ScSingleRefOpToken with ocMatRef. */
    formula::FormulaToken* AddMatrixSingleReference( const ScSingleRefData& rRef );
    SC_DLLPUBLIC formula::FormulaToken* AddDoubleReference( const ScComplexRefData& rRef );
    SC_DLLPUBLIC void      AddRangeName( sal_uInt16 n, sal_Int16 nSheet );
    formula::FormulaToken* AddDBRange( sal_uInt16 n );
    SC_DLLPUBLIC formula::FormulaToken* AddExternalName( sal_uInt16 nFileId, const svl::SharedString& rName );
    SC_DLLPUBLIC void AddExternalSingleReference( sal_uInt16 nFileId, const svl::SharedString& rTabName, const ScSingleRefData& rRef );
    SC_DLLPUBLIC formula::FormulaToken* AddExternalDoubleReference( sal_uInt16 nFileId, const svl::SharedString& rTabName, const ScComplexRefData& rRef );
    SC_DLLPUBLIC formula::FormulaToken* AddMatrix( const ScMatrixRef& p );
    /** ScSingleRefOpToken with ocColRowName. */
    SC_DLLPUBLIC formula::FormulaToken* AddColRowName( const ScSingleRefData& rRef );
    SC_DLLPUBLIC virtual formula::FormulaToken* MergeArray( ) override;

    /** Merge very last SingleRef+ocRange+SingleRef combination into DoubleRef
        and adjust pCode array, or do nothing if conditions not met. */
    void MergeRangeReference( const ScAddress & rPos );

    /// Assign XML string placeholder to the array
    void AssignXMLString( const OUString &rText, const OUString &rFormulaNmsp );

    /** Assignment with incrementing references of FormulaToken entries
        (not copied!) */
    ScTokenArray& operator=( const ScTokenArray& );
    ScTokenArray& operator=( ScTokenArray&& );

    /**
     * Make all absolute references external references pointing to the old document
     *
     * @param rOldDoc old document
     * @param rNewDoc new document
     * @param rPos position of the cell to determine if the reference is in the copied area
     * @param bRangeName set for range names, range names have special handling for absolute sheet ref + relative col/row ref
     */
    void ReadjustAbsolute3DReferences( const ScDocument& rOldDoc, ScDocument& rNewDoc, const ScAddress& rPos, bool bRangeName = false );

    /**
     * Make all absolute references pointing to the copied range if the range is copied too
     * @param bCheckCopyArea should reference pointing into the copy area be adjusted independently from being absolute, should be true only for copy&paste between documents
     */
    void AdjustAbsoluteRefs( const ScDocument& rOldDoc, const ScAddress& rOldPos, const ScAddress& rNewPos, bool bCheckCopyArea );

    /** When copying a sheet-local named expression, move sheet references that
        point to the originating sheet to point to the new sheet instead.
     */
    void AdjustSheetLocalNameReferences( SCTAB nOldTab, SCTAB nNewTab );

    /** Returns true if the sheet nTab is referenced in code. Relative sheet
        references are evaluated using nPosTab.
     */
    bool ReferencesSheet( SCTAB nTab, SCTAB nPosTab ) const;

    /**
     * Adjust all references in response to shifting of cells during cell
     * insertion and deletion.
     *
     * @param rCxt context that stores details of shifted region.
     * @param rOldPos old cell position prior to shifting.
     */
    sc::RefUpdateResult AdjustReferenceOnShift( const sc::RefUpdateContext& rCxt, const ScAddress& rOldPos );

    sc::RefUpdateResult AdjustReferenceOnMove(
        const sc::RefUpdateContext& rCxt, const ScAddress& rOldPos, const ScAddress& rNewPos );

    /**
     * Move reference positions in response to column reordering.  A range
     * reference gets moved only when the whole range fits in a single column.
     *
     * @param rPos position of this formula cell
     * @param nTab sheet where columns are reordered.
     * @param nRow1 top row of reordered range.
     * @param nRow2 bottom row of reordered range.
     * @param rColMap old-to-new column mapping.
     */
    void MoveReferenceColReorder(
        const ScAddress& rPos, SCTAB nTab, SCROW nRow1, SCROW nRow2,
        const sc::ColRowReorderMapType& rColMap );

    void MoveReferenceRowReorder(
        const ScAddress& rPos, SCTAB nTab, SCCOL nCol1, SCCOL nCol2,
        const sc::ColRowReorderMapType& rRowMap );

    /**
     * Adjust all references in named expression. In named expression, we only
     * update absolute positions, and leave relative positions intact.
     *
     * @param rCxt context that stores details of shifted region
     *
     * @return update result.
     */
    sc::RefUpdateResult AdjustReferenceInName( const sc::RefUpdateContext& rCxt, const ScAddress& rPos );

    sc::RefUpdateResult AdjustReferenceInMovedName( const sc::RefUpdateContext& rCxt, const ScAddress& rPos );

    /**
     * Adjust all references on sheet deletion.
     *
     * @param nDelPos position of sheet being deleted.
     * @param nSheets number of sheets to delete.
     * @param rOldPos position of formula cell prior to the deletion.
     *
     * @return true if at least one reference has changed its sheet reference.
     */
    sc::RefUpdateResult AdjustReferenceOnDeletedTab( const sc::RefUpdateDeleteTabContext& rCxt, const ScAddress& rOldPos );

    sc::RefUpdateResult AdjustReferenceOnInsertedTab( const sc::RefUpdateInsertTabContext& rCxt, const ScAddress& rOldPos );

    sc::RefUpdateResult AdjustReferenceOnMovedTab( const sc::RefUpdateMoveTabContext& rCxt, const ScAddress& rOldPos );

    /**
     * Adjust all internal references on base position change.
     */
    void AdjustReferenceOnMovedOrigin( const ScAddress& rOldPos, const ScAddress& rNewPos );

    /**
     * Adjust all internal references on base position change if they point to
     * a sheet other than the one of rOldPos.
     */
    void AdjustReferenceOnMovedOriginIfOtherSheet( const ScAddress& rOldPos, const ScAddress& rNewPos );

    /**
     * Adjust internal range references on base position change to justify /
     * put in order the relative references.
     */
    void AdjustReferenceOnCopy( const ScAddress& rNewPos );

    /**
     * Clear sheet deleted flag from internal reference tokens if the sheet
     * index falls within specified range.  Note that when a reference is on a
     * sheet that's been deleted, its referenced sheet index retains the
     * original index of the deleted sheet.
     *
     * @param rPos position of formula cell
     * @param nStartTab index of first sheet, inclusive.
     * @param nEndTab index of last sheet, inclusive.
     */
    void ClearTabDeleted( const ScAddress& rPos, SCTAB nStartTab, SCTAB nEndTab );

    void CheckRelativeReferenceBounds(
        const sc::RefUpdateContext& rCxt, const ScAddress& rPos, SCROW nGroupLen, std::vector<SCROW>& rBounds ) const;

    void CheckRelativeReferenceBounds(
        const ScAddress& rPos, SCROW nGroupLen, const ScRange& rRange, std::vector<SCROW>& rBounds ) const;

    void CheckExpandReferenceBounds(
        const sc::RefUpdateContext& rCxt, const ScAddress& rPos, SCROW nGroupLen, std::vector<SCROW>& rBounds ) const;

    /**
     * Create a string representation of formula token array without modifying
     * the internal state of the token array.
     */
    SC_DLLPUBLIC OUString CreateString( sc::TokenStringContext& rCxt, const ScAddress& rPos ) const;

    SC_DLLPUBLIC void WrapReference( const ScAddress& rPos, SCCOL nMaxCol, SCROW nMaxRow );

    sal_Int32 GetWeight() const;

    bool IsEnabledForOpenCL() const { return mbOpenCLEnabled; }
    bool IsEnabledForThreading() const { return mbThreadingEnabled; }

#if DEBUG_FORMULA_COMPILER
    void Dump() const;
#endif
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
