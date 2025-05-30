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

#include "global.hxx"
#include "address.hxx"
#include <formula/grammar.hxx>
#include "scdllapi.h"
#include "calcmacros.hxx"

#include <memory>
#include <map>
#include <vector>

class ScDocument;
class ScTokenArray;

namespace sc {

struct RefUpdateContext;
struct RefUpdateInsertTabContext;
struct RefUpdateDeleteTabContext;
struct RefUpdateMoveTabContext;
class CompileFormulaContext;

}

class ScRangeData
{
public:
    enum class Type //specialization to typed_flags outside of class
    {
        Name       = 0x0000,
        Database   = 0x0001,
        Criteria   = 0x0002,
        PrintArea  = 0x0004,
        ColHeader  = 0x0008,
        RowHeader  = 0x0010,
        AbsArea    = 0x0020,
        RefArea    = 0x0040,
        AbsPos     = 0x0080,
        Hidden     = 0x0100
    };

    enum class IsNameValidType
    {
        NAME_VALID,
        NAME_INVALID_CELL_REF,
        NAME_INVALID_BAD_STRING
    };

private:
    OUString        aName;
    OUString        aUpperName; // #i62977# for faster searching (aName is never modified after ctor)
    OUString        maNewName;  ///< used for formulas after changing names in the dialog
    std::unique_ptr<ScTokenArray>
                    pCode;
    ScAddress       aPos;
    Type            eType;
    ScDocument&     rDoc;
    formula::FormulaGrammar::Grammar    eTempGrammar;   // needed for unresolved XML compiles
    sal_uInt16      nIndex;
    bool            bModified;          // is set/cleared by UpdateReference

    void CompileRangeData( const OUString& rSymbol, bool bSetError );
    void InitCode();
public:

    SC_DLLPUBLIC                ScRangeData( ScDocument& rDoc,
                                 const OUString& rName,
                                 const OUString& rSymbol,
                                 const ScAddress& rAdr = ScAddress(),
                                 Type nType = Type::Name,
                                 const formula::FormulaGrammar::Grammar eGrammar = formula::FormulaGrammar::GRAM_DEFAULT );
    SC_DLLPUBLIC                ScRangeData( ScDocument& rDoc,
                                 const OUString& rName,
                                 const ScTokenArray& rArr,
                                 const ScAddress& rAdr = ScAddress(),
                                 Type nType = Type::Name );
    SC_DLLPUBLIC                ScRangeData( ScDocument& rDoc,
                                 const OUString& rName,
                                 const ScAddress& rTarget );
                                // rTarget is ABSPOS jump label

    /* Exact copy, not recompiled, no other index (!), nothing... except if
     * pDocument or pPos are passed, those values are assigned instead of the
     * copies. */
    ScRangeData( const ScRangeData& rScRangeData, ScDocument* pDocument = nullptr, const ScAddress* pPos = nullptr );

    SC_DLLPUBLIC ~ScRangeData();

    bool            operator== (const ScRangeData& rData) const;

    void            GetName( OUString& rName ) const  { rName = maNewName.isEmpty() ? aName : maNewName; }
    const OUString&   GetName() const           { return maNewName.isEmpty() ? aName : maNewName; }
    const OUString&   GetUpperName() const      { return aUpperName; }
    const ScAddress&  GetPos() const                  { return aPos; }
    // The index has to be unique. If index=0 a new index value is assigned.
    void            SetIndex( sal_uInt16 nInd )         { nIndex = nInd; }
    sal_uInt16      GetIndex() const                { return nIndex; }
    /// Does not change the name, but sets maNewName for formula update after dialog.
    void            SetNewName( const OUString& rNewName )  { maNewName = rNewName; }
    ScTokenArray*   GetCode()                       { return pCode.get(); }
    SC_DLLPUBLIC void   SetCode( const ScTokenArray& );
    const ScTokenArray* GetCode() const             { return pCode.get(); }
    SC_DLLPUBLIC FormulaError GetErrCode() const;
    bool            HasReferences() const;
    void            AddType( Type nType );
    Type            GetType() const                 { return eType; }
    bool            HasType( Type nType ) const;
    SC_DLLPUBLIC sal_uInt32 GetUnoType() const;
    SC_DLLPUBLIC OUString GetSymbol( const formula::FormulaGrammar::Grammar eGrammar = formula::FormulaGrammar::GRAM_DEFAULT ) const;
    SC_DLLPUBLIC OUString GetSymbol( const ScAddress& rPos, const formula::FormulaGrammar::Grammar eGrammar = formula::FormulaGrammar::GRAM_DEFAULT ) const;
    void            UpdateSymbol( OUStringBuffer& rBuffer, const ScAddress& );

    /**
     * @param nLocalTab sheet index where this name belongs, or -1 for global
     *                  name.
     */
    void UpdateReference( sc::RefUpdateContext& rCxt, SCTAB nLocalTab );
    bool            IsModified() const              { return bModified; }

    SC_DLLPUBLIC void           GuessPosition();

    void            UpdateTranspose( const ScRange& rSource, const ScAddress& rDest );
    void            UpdateGrow( const ScRange& rArea, SCCOL nGrowX, SCROW nGrowY );

    SC_DLLPUBLIC bool           IsReference( ScRange& rRef ) const;
    bool                        IsReference( ScRange& rRef, const ScAddress& rPos ) const;
    SC_DLLPUBLIC bool           IsValidReference( ScRange& rRef ) const;
    bool                        IsRangeAtBlock( const ScRange& ) const;

    void UpdateInsertTab( sc::RefUpdateInsertTabContext& rCxt, SCTAB nLocalTab );
    void UpdateDeleteTab( sc::RefUpdateDeleteTabContext& rCxt, SCTAB nLocalTab );
    void UpdateMoveTab( sc::RefUpdateMoveTabContext& rCxt, SCTAB nLocalTab );

    void            ValidateTabRefs();

    static void     MakeValidName( const ScDocument& rDoc, OUString& rName );

    SC_DLLPUBLIC static IsNameValidType     IsNameValid( const OUString& rName, const ScDocument& rDoc );

    bool HasPossibleAddressConflict() const;

    void CompileUnresolvedXML( sc::CompileFormulaContext& rCxt );

#if DEBUG_FORMULA_COMPILER
    void Dump() const;
#endif
};
namespace o3tl
{
    template<> struct typed_flags<ScRangeData::Type> : is_typed_flags<ScRangeData::Type, 0x1ff> {};
}


inline void ScRangeData::AddType( Type nType )
{
    eType = eType|nType;
}

inline bool ScRangeData::HasType( Type nType ) const
{
    return ( ( eType & nType ) == nType );
}

extern "C" int ScRangeData_QsortNameCompare( const void*, const void* );

class ScRangeName
{
private:
    typedef std::vector<ScRangeData*> IndexDataType;
    typedef ::std::map<OUString, std::unique_ptr<ScRangeData>> DataType;
    DataType m_Data;
    IndexDataType maIndexToData;
    // Use for optimization, true if any of the contained names resolves
    // as a valid cell address (e.g. 'day1' with 16k columns).
    mutable bool mHasPossibleAddressConflict : 1;
    mutable bool mHasPossibleAddressConflictDirty : 1;

    void checkHasPossibleAddressConflict() const;

public:
    /// Map that stores non-managed pointers to ScRangeName instances.
    typedef ::std::map<SCTAB, const ScRangeName*> TabNameCopyMap;

    typedef DataType::const_iterator const_iterator;
    typedef DataType::iterator iterator;

    ScRangeName();
    SC_DLLPUBLIC ScRangeName(const ScRangeName& r);

    SC_DLLPUBLIC const ScRangeData* findByRange(const ScRange& rRange) const;
    SC_DLLPUBLIC ScRangeData* findByUpperName(const OUString& rName);
    SC_DLLPUBLIC const ScRangeData* findByUpperName(const OUString& rName) const;
    SC_DLLPUBLIC ScRangeData* findByIndex(sal_uInt16 i) const;
    void UpdateReference( sc::RefUpdateContext& rCxt, SCTAB nLocalTab = -1 );
    void UpdateInsertTab( sc::RefUpdateInsertTabContext& rCxt, SCTAB nLocalTab = -1 );
    void UpdateDeleteTab( sc::RefUpdateDeleteTabContext& rCxt, SCTAB nLocalTab = -1 );
    void UpdateMoveTab( sc::RefUpdateMoveTabContext& rCxt, SCTAB nLocalTab = -1 );
    void UpdateTranspose(const ScRange& rSource, const ScAddress& rDest);
    void UpdateGrow(const ScRange& rArea, SCCOL nGrowX, SCROW nGrowY);

    /** Compile those names that couldn't be resolved during loading and
        inserting because they may have referred a name that was inserted later.
     */
    void CompileUnresolvedXML( sc::CompileFormulaContext& rCxt );

    /** Copy names while copying a sheet if they reference the sheet to be copied.

        Assumes that new sheet was already inserted, global names have been
        updated/adjusted, but sheet-local names on nOldTab are not, as is the
        case in ScDocument::CopyTab()

        @param  nLocalTab
                -1 when operating on global names, else sheet/tab of
                sheet-local name scope. The already adjusted tab on which to
                find the name.

        @param  nOldTab
                The original unadjusted tab position.

        @param  nNewTab
                The new tab position.
     */
    void CopyUsedNames( const SCTAB nLocalTab, const SCTAB nOldTab, const SCTAB nNewTab,
            const ScDocument& rOldDoc, ScDocument& rNewDoc, const bool bGlobalNamesToLocal ) const;

    SC_DLLPUBLIC const_iterator begin() const { return m_Data.begin(); }
    SC_DLLPUBLIC const_iterator end() const  { return m_Data.end(); }
    SC_DLLPUBLIC size_t size() const { return m_Data.size(); }
    SC_DLLPUBLIC size_t index_size() const { return maIndexToData.size(); }
    bool empty() const { return m_Data.empty(); }

    /** Insert object into set.
        @ATTENTION: The underlying ::std::map<std::unique_ptr>::insert(p) takes
        ownership of p and if it can't insert it deletes the object! So, if
        this insert here returns false the object where p pointed to is gone!

        @param  bReuseFreeIndex
                If the ScRangeData p points to has an index value of 0:
                If `TRUE` then reuse a free index slot if available.
                If `FALSE` then assign a new index slot. The Manage Names
                dialog uses this so that deleting and adding ranges in the same
                run is guaranteed to not reuse previously assigned indexes.
     */
    SC_DLLPUBLIC bool insert( ScRangeData* p, bool bReuseFreeIndex = true );

    void erase(const ScRangeData& r);
    void erase(const OUString& rName);

    /**
     * Erase by iterator position.  Note that this method doesn't check for
     * iterator's validity.  The caller must make sure that the iterator is
     * valid.
     */
    void erase(const_iterator itr);

    void clear();
    bool operator== (const ScRangeName& r) const;

    bool hasPossibleAddressConflict() const
    {
        if( mHasPossibleAddressConflictDirty )
            checkHasPossibleAddressConflict();
        return mHasPossibleAddressConflict;
    }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
