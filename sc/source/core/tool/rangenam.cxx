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

#include <string.h>
#include <memory>
#include <unotools/collatorwrapper.hxx>
#include <unotools/charclass.hxx>
#include <com/sun/star/sheet/NamedRangeFlag.hpp>
#include <osl/diagnose.h>

#include <token.hxx>
#include <tokenarray.hxx>
#include <rangenam.hxx>
#include <rangeutl.hxx>
#include <global.hxx>
#include <compiler.hxx>
#include <refupdat.hxx>
#include <document.hxx>
#include <refupdatecontext.hxx>
#include <tokenstringcontext.hxx>

#include <formula/errorcodes.hxx>

using namespace formula;
using ::std::pair;

// ScRangeData

ScRangeData::ScRangeData( ScDocument& rDok,
                          const OUString& rName,
                          const OUString& rSymbol,
                          const ScAddress& rAddress,
                          Type nType,
                          const FormulaGrammar::Grammar eGrammar ) :
                aName       ( rName ),
                aUpperName  ( ScGlobal::getCharClass().uppercase( rName ) ),
                aPos        ( rAddress ),
                eType       ( nType ),
                rDoc        ( rDok ),
                eTempGrammar( eGrammar ),
                nIndex      ( 0 ),
                bModified   ( false )
{
    if (!rSymbol.isEmpty())
    {
        // Let the compiler set an error on unknown names for a subsequent
        // CompileUnresolvedXML().
        const bool bImporting = rDoc.IsImportingXML();
        CompileRangeData( rSymbol, bImporting);
        if (bImporting)
            rDoc.CheckLinkFormulaNeedingCheck( *pCode);
    }
    else
    {
        // #i63513#/#i65690# don't leave pCode as NULL.
        // Copy ctor default-constructs pCode if it was NULL, so it's initialized here, too,
        // to ensure same behavior if unnecessary copying is left out.

        pCode.reset( new ScTokenArray(rDoc) );
        pCode->SetFromRangeName(true);
    }
}

ScRangeData::ScRangeData( ScDocument& rDok,
                          const OUString& rName,
                          const ScTokenArray& rArr,
                          const ScAddress& rAddress,
                          Type nType ) :
                aName       ( rName ),
                aUpperName  ( ScGlobal::getCharClass().uppercase( rName ) ),
                pCode       ( new ScTokenArray( rArr ) ),
                aPos        ( rAddress ),
                eType       ( nType ),
                rDoc        ( rDok ),
                eTempGrammar( FormulaGrammar::GRAM_UNSPECIFIED ),
                nIndex      ( 0 ),
                bModified   ( false )
{
    pCode->SetFromRangeName(true);
    InitCode();
}

ScRangeData::ScRangeData( ScDocument& rDok,
                          const OUString& rName,
                          const ScAddress& rTarget ) :
                aName       ( rName ),
                aUpperName  ( ScGlobal::getCharClass().uppercase( rName ) ),
                pCode       ( new ScTokenArray(rDok) ),
                aPos        ( rTarget ),
                eType       ( Type::Name ),
                rDoc        ( rDok ),
                eTempGrammar( FormulaGrammar::GRAM_UNSPECIFIED ),
                nIndex      ( 0 ),
                bModified   ( false )
{
    ScSingleRefData aRefData;
    aRefData.InitAddress( rTarget );
    aRefData.SetFlag3D( true );
    pCode->AddSingleReference( aRefData );
    pCode->SetFromRangeName(true);
    ScCompiler aComp( rDoc, aPos, *pCode, rDoc.GetGrammar() );
    aComp.CompileTokenArray();
    if ( pCode->GetCodeError() == FormulaError::NONE )
        eType |= Type::AbsPos;
}

ScRangeData::ScRangeData(const ScRangeData& rScRangeData, ScDocument* pDocument, const ScAddress* pPos) :
    aName   (rScRangeData.aName),
    aUpperName  (rScRangeData.aUpperName),
    pCode       (rScRangeData.pCode ? rScRangeData.pCode->Clone().release() : new ScTokenArray(*pDocument)),   // make real copy (not copy-ctor)
    aPos        (pPos ? *pPos : rScRangeData.aPos),
    eType       (rScRangeData.eType),
    rDoc        (pDocument ? *pDocument : rScRangeData.rDoc),
    eTempGrammar(rScRangeData.eTempGrammar),
    nIndex      (rScRangeData.nIndex),
    bModified   (rScRangeData.bModified)
{
    pCode->SetFromRangeName(true);
}

ScRangeData::~ScRangeData()
{
}

void ScRangeData::CompileRangeData( const OUString& rSymbol, bool bSetError )
{
    if (eTempGrammar == FormulaGrammar::GRAM_UNSPECIFIED)
    {
        OSL_FAIL( "ScRangeData::CompileRangeData: unspecified grammar");
        // Anything is almost as bad as this, but we might have the best choice
        // if not loading documents.
        eTempGrammar = FormulaGrammar::GRAM_NATIVE;
    }

    ScCompiler aComp( rDoc, aPos, eTempGrammar );
    if (bSetError)
        aComp.SetExtendedErrorDetection( ScCompiler::EXTENDED_ERROR_DETECTION_NAME_NO_BREAK);
    pCode = aComp.CompileString( rSymbol );
    pCode->SetFromRangeName(true);
    if( pCode->GetCodeError() != FormulaError::NONE )
        return;

    FormulaTokenArrayPlainIterator aIter(*pCode);
    FormulaToken* p = aIter.GetNextReference();
    if( p )
    {
        // first token is a reference
        /* FIXME: wouldn't that need a check if it's exactly one reference? */
        if( p->GetType() == svSingleRef )
            eType = eType | Type::AbsPos;
        else
            eType = eType | Type::AbsArea;
    }
    // For manual input set an error for an incomplete formula.
    if (!rDoc.IsImportingXML())
    {
        aComp.CompileTokenArray();
        pCode->DelRPN();
    }
}

void ScRangeData::CompileUnresolvedXML( sc::CompileFormulaContext& rCxt )
{
    if (pCode->GetCodeError() == FormulaError::NoName)
    {
        // Reconstruct the symbol/formula and then recompile.
        OUString aSymbol;
        rCxt.setGrammar(eTempGrammar);
        ScCompiler aComp(rCxt, aPos, *pCode);
        aComp.CreateStringFromTokenArray( aSymbol);
        // Don't let the compiler set an error for unknown names on final
        // compile, errors are handled by the interpreter thereafter.
        CompileRangeData( aSymbol, false);
        rCxt.getDoc().CheckLinkFormulaNeedingCheck( *pCode);
    }
}

#if DEBUG_FORMULA_COMPILER
void ScRangeData::Dump() const
{
    cout << "-- ScRangeData" << endl;
    cout << "  name: " << aName << endl;
    cout << "  ref position: (col=" << aPos.Col() << ", row=" << aPos.Row() << ", sheet=" << aPos.Tab() << ")" << endl;

    if (pCode)
        pCode->Dump();
}
#endif

void ScRangeData::GuessPosition()
{
    // set a position that allows "absoluting" of all relative references
    // in CalcAbsIfRel without errors

    OSL_ENSURE(aPos == ScAddress(), "position will go lost now");

    SCCOL nMinCol = 0;
    SCROW nMinRow = 0;
    SCTAB nMinTab = 0;

    formula::FormulaToken* t;
    formula::FormulaTokenArrayPlainIterator aIter(*pCode);
    while ( ( t = aIter.GetNextReference() ) != nullptr )
    {
        ScSingleRefData& rRef1 = *t->GetSingleRef();
        if ( rRef1.IsColRel() && rRef1.Col() < nMinCol )
            nMinCol = rRef1.Col();
        if ( rRef1.IsRowRel() && rRef1.Row() < nMinRow )
            nMinRow = rRef1.Row();
        if ( rRef1.IsTabRel() && rRef1.Tab() < nMinTab )
            nMinTab = rRef1.Tab();

        if ( t->GetType() == svDoubleRef )
        {
            ScSingleRefData& rRef2 = t->GetDoubleRef()->Ref2;
            if ( rRef2.IsColRel() && rRef2.Col() < nMinCol )
                nMinCol = rRef2.Col();
            if ( rRef2.IsRowRel() && rRef2.Row() < nMinRow )
                nMinRow = rRef2.Row();
            if ( rRef2.IsTabRel() && rRef2.Tab() < nMinTab )
                nMinTab = rRef2.Tab();
        }
    }

    aPos = ScAddress( static_cast<SCCOL>(-nMinCol), static_cast<SCROW>(-nMinRow), static_cast<SCTAB>(-nMinTab) );
}

OUString ScRangeData::GetSymbol( const FormulaGrammar::Grammar eGrammar ) const
{
    ScCompiler aComp(rDoc, aPos, *pCode, eGrammar);
    OUString symbol;
    aComp.CreateStringFromTokenArray( symbol );
    return symbol;
}

OUString ScRangeData::GetSymbol( const ScAddress& rPos, const FormulaGrammar::Grammar eGrammar ) const
{
    OUString aStr;
    ScCompiler aComp(rDoc, rPos, *pCode, eGrammar);
    aComp.CreateStringFromTokenArray( aStr );
    return aStr;
}

void ScRangeData::UpdateSymbol( OUStringBuffer& rBuffer, const ScAddress& rPos )
{
    ScTokenArray aTemp( pCode->CloneValue() );
    ScCompiler aComp(rDoc, rPos, aTemp, formula::FormulaGrammar::GRAM_DEFAULT);
    aComp.MoveRelWrap();
    aComp.CreateStringFromTokenArray( rBuffer );
}

void ScRangeData::UpdateReference( sc::RefUpdateContext& rCxt, SCTAB nLocalTab )
{
    sc::RefUpdateResult aRes = pCode->AdjustReferenceInName(rCxt, aPos);
    bModified = aRes.mbReferenceModified;
    if (aRes.mbReferenceModified)
        rCxt.maUpdatedNames.setUpdatedName(nLocalTab, nIndex);
}

void ScRangeData::UpdateTranspose( const ScRange& rSource, const ScAddress& rDest )
{
    bool bChanged = false;

    formula::FormulaToken* t;
    formula::FormulaTokenArrayPlainIterator aIter(*pCode);

    while ( ( t = aIter.GetNextReference() ) != nullptr )
    {
        if( t->GetType() != svIndex )
        {
            SingleDoubleRefModifier aMod( *t );
            ScComplexRefData& rRef = aMod.Ref();
            // Update only absolute references
            if (!rRef.Ref1.IsColRel() && !rRef.Ref1.IsRowRel() &&
                    (!rRef.Ref1.IsFlag3D() || !rRef.Ref1.IsTabRel()) &&
                ( t->GetType() == svSingleRef ||
                (!rRef.Ref2.IsColRel() && !rRef.Ref2.IsRowRel() &&
                    (!rRef.Ref2.IsFlag3D() || !rRef.Ref2.IsTabRel()))))
            {
                ScRange aAbs = rRef.toAbs(rDoc, aPos);
                // Check if the absolute reference of this range is pointing to the transposed source
                if (ScRefUpdate::UpdateTranspose(rDoc, rSource, rDest, aAbs) != UR_NOTHING)
                {
                    rRef.SetRange(rDoc.GetSheetLimits(), aAbs, aPos);
                    bChanged = true;
                }
            }
        }
    }

    bModified = bChanged;
}

void ScRangeData::UpdateGrow( const ScRange& rArea, SCCOL nGrowX, SCROW nGrowY )
{
    bool bChanged = false;

    formula::FormulaToken* t;
    formula::FormulaTokenArrayPlainIterator aIter(*pCode);

    while ( ( t = aIter.GetNextReference() ) != nullptr )
    {
        if( t->GetType() != svIndex )
        {
            SingleDoubleRefModifier aMod( *t );
            ScComplexRefData& rRef = aMod.Ref();
            if (!rRef.Ref1.IsColRel() && !rRef.Ref1.IsRowRel() &&
                    (!rRef.Ref1.IsFlag3D() || !rRef.Ref1.IsTabRel()) &&
                ( t->GetType() == svSingleRef ||
                (!rRef.Ref2.IsColRel() && !rRef.Ref2.IsRowRel() &&
                    (!rRef.Ref2.IsFlag3D() || !rRef.Ref2.IsTabRel()))))
            {
                ScRange aAbs = rRef.toAbs(rDoc, aPos);
                if (ScRefUpdate::UpdateGrow(rArea, nGrowX, nGrowY, aAbs) != UR_NOTHING)
                {
                    rRef.SetRange(rDoc.GetSheetLimits(), aAbs, aPos);
                    bChanged = true;
                }
            }
        }
    }

    bModified = bChanged;           // has to be evaluated immediately afterwards
}

bool ScRangeData::operator== (const ScRangeData& rData) const       // for Undo
{
    if ( nIndex != rData.nIndex ||
         aName  != rData.aName  ||
         aPos   != rData.aPos   ||
         eType  != rData.eType     ) return false;

    sal_uInt16 nLen = pCode->GetLen();
    if ( nLen != rData.pCode->GetLen() ) return false;

    FormulaToken** ppThis = pCode->GetArray();
    FormulaToken** ppOther = rData.pCode->GetArray();

    for ( sal_uInt16 i=0; i<nLen; i++ )
        if ( ppThis[i] != ppOther[i] && !(*ppThis[i] == *ppOther[i]) )
            return false;

    return true;
}

bool ScRangeData::IsRangeAtBlock( const ScRange& rBlock ) const
{
    bool bRet = false;
    ScRange aRange;
    if ( IsReference(aRange) )
        bRet = ( rBlock == aRange );
    return bRet;
}

bool ScRangeData::IsReference( ScRange& rRange ) const
{
    if ( (eType & ( Type::AbsArea | Type::RefArea | Type::AbsPos )) && pCode )
        return pCode->IsReference(rRange, aPos);

    return false;
}

bool ScRangeData::IsReference( ScRange& rRange, const ScAddress& rPos ) const
{
    if ( (eType & ( Type::AbsArea | Type::RefArea | Type::AbsPos ) ) && pCode )
        return pCode->IsReference(rRange, rPos);

    return false;
}

bool ScRangeData::IsValidReference( ScRange& rRange ) const
{
    if ( (eType & ( Type::AbsArea | Type::RefArea | Type::AbsPos ) ) && pCode )
        return pCode->IsValidReference(rRange, aPos);

    return false;
}

void ScRangeData::UpdateInsertTab( sc::RefUpdateInsertTabContext& rCxt, SCTAB nLocalTab )
{
    sc::RefUpdateResult aRes = pCode->AdjustReferenceOnInsertedTab(rCxt, aPos);
    if (aRes.mbReferenceModified)
        rCxt.maUpdatedNames.setUpdatedName(nLocalTab, nIndex);

    if (rCxt.mnInsertPos <= aPos.Tab())
        aPos.IncTab(rCxt.mnSheets);
}

void ScRangeData::UpdateDeleteTab( sc::RefUpdateDeleteTabContext& rCxt, SCTAB nLocalTab )
{
    sc::RefUpdateResult aRes = pCode->AdjustReferenceOnDeletedTab(rCxt, aPos);
    if (aRes.mbReferenceModified)
        rCxt.maUpdatedNames.setUpdatedName(nLocalTab, nIndex);

    ScRangeUpdater::UpdateDeleteTab( aPos, rCxt);
}

void ScRangeData::UpdateMoveTab( sc::RefUpdateMoveTabContext& rCxt, SCTAB nLocalTab )
{
    sc::RefUpdateResult aRes = pCode->AdjustReferenceOnMovedTab(rCxt, aPos);
    if (aRes.mbReferenceModified)
        rCxt.maUpdatedNames.setUpdatedName(nLocalTab, nIndex);

    aPos.SetTab(rCxt.getNewTab(aPos.Tab()));
}

void ScRangeData::MakeValidName( const ScDocument& rDoc, OUString& rName )
{

    // strip leading invalid characters
    sal_Int32 nPos = 0;
    sal_Int32 nLen = rName.getLength();
    while ( nPos < nLen && !ScCompiler::IsCharFlagAllConventions( rName, nPos, ScCharFlags::Name) )
        ++nPos;
    if ( nPos>0 )
        rName = rName.copy(nPos);

    // if the first character is an invalid start character, precede with '_'
    if ( !rName.isEmpty() && !ScCompiler::IsCharFlagAllConventions( rName, 0, ScCharFlags::CharName ) )
        rName = "_" + rName;

    // replace invalid with '_'
    nLen = rName.getLength();
    for (nPos=0; nPos<nLen; nPos++)
    {
        if ( !ScCompiler::IsCharFlagAllConventions( rName, nPos, ScCharFlags::Name) )
            rName = rName.replaceAt( nPos, 1, u"_" );
    }

    // Ensure that the proposed name is not a reference under any convention,
    // same as in IsNameValid()
    ScAddress aAddr;
    ScRange aRange;
    for (int nConv = FormulaGrammar::CONV_UNSPECIFIED; ++nConv < FormulaGrammar::CONV_LAST; )
    {
        ScAddress::Details details( static_cast<FormulaGrammar::AddressConvention>( nConv ) );
        // Don't check Parse on VALID, any partial only VALID may result in
        // #REF! during compile later!
        while (aRange.Parse(rName, rDoc, details) != ScRefFlags::ZERO ||
                aAddr.Parse(rName, rDoc, details) != ScRefFlags::ZERO)
        {
            // Range Parse is partially valid also with invalid sheet name,
            // Address Parse ditto, during compile name would generate a #REF!
            if ( rName.indexOf( '.' ) != -1 )
                rName = rName.replaceFirst( ".", "_" );
            else
                rName = "_" + rName;
        }
    }
}

ScRangeData::IsNameValidType ScRangeData::IsNameValid( const OUString& rName, const ScDocument& rDoc )
{
    /* XXX If changed, sc/source/filter/ftools/ftools.cxx
     * ScfTools::ConvertToScDefinedName needs to be changed too. */
    char const a('.');
    if (rName.indexOf(a) != -1)
        return IsNameValidType::NAME_INVALID_BAD_STRING;
    sal_Int32 nPos = 0;
    sal_Int32 nLen = rName.getLength();
    if ( !nLen || !ScCompiler::IsCharFlagAllConventions( rName, nPos++, ScCharFlags::CharName ) )
        return IsNameValidType::NAME_INVALID_BAD_STRING;
    while ( nPos < nLen )
    {
        if ( !ScCompiler::IsCharFlagAllConventions( rName, nPos++, ScCharFlags::Name ) )
            return IsNameValidType::NAME_INVALID_BAD_STRING;
    }
    ScAddress aAddr;
    ScRange aRange;
    for (int nConv = FormulaGrammar::CONV_UNSPECIFIED; ++nConv < FormulaGrammar::CONV_LAST; )
    {
        ScAddress::Details details( static_cast<FormulaGrammar::AddressConvention>( nConv ) );
        // Don't check Parse on VALID, any partial only VALID may result in
        // #REF! during compile later!
        if (aRange.Parse(rName, rDoc, details) != ScRefFlags::ZERO ||
             aAddr.Parse(rName, rDoc, details) != ScRefFlags::ZERO )
        {
            return IsNameValidType::NAME_INVALID_CELL_REF;
        }
    }
    return IsNameValidType::NAME_VALID;
}

bool ScRangeData::HasPossibleAddressConflict() const
{
    // Similar to part of IsNameValid(), but only check if the name is a valid address.
    ScAddress aAddr;
    for (int nConv = FormulaGrammar::CONV_UNSPECIFIED; ++nConv < FormulaGrammar::CONV_LAST; )
    {
        ScAddress::Details details( static_cast<FormulaGrammar::AddressConvention>( nConv ) );
        // Don't check Parse on VALID, any partial only VALID may result in
        // #REF! during compile later!
        if(aAddr.Parse(aUpperName, rDoc, details) != ScRefFlags::ZERO)
            return true;
    }
    return false;
}

FormulaError ScRangeData::GetErrCode() const
{
    return pCode ? pCode->GetCodeError() : FormulaError::NONE;
}

bool ScRangeData::HasReferences() const
{
    return pCode->HasReferences();
}

sal_uInt32 ScRangeData::GetUnoType() const
{
    sal_uInt32 nUnoType = 0;
    if ( HasType(Type::Criteria) )  nUnoType |= css::sheet::NamedRangeFlag::FILTER_CRITERIA;
    if ( HasType(Type::PrintArea) ) nUnoType |= css::sheet::NamedRangeFlag::PRINT_AREA;
    if ( HasType(Type::ColHeader) ) nUnoType |= css::sheet::NamedRangeFlag::COLUMN_HEADER;
    if ( HasType(Type::RowHeader) ) nUnoType |= css::sheet::NamedRangeFlag::ROW_HEADER;
    if ( HasType(Type::Hidden) )    nUnoType |= css::sheet::NamedRangeFlag::HIDDEN;
    return nUnoType;
}

void ScRangeData::ValidateTabRefs()
{
    //  try to make sure all relative references and the reference position
    //  are within existing tables, so they can be represented as text
    //  (if the range of used tables is more than the existing tables,
    //  the result may still contain invalid tables, because the relative
    //  references aren't changed so formulas stay the same)

    //  find range of used tables

    SCTAB nMinTab = aPos.Tab();
    SCTAB nMaxTab = nMinTab;
    formula::FormulaToken* t;
    formula::FormulaTokenArrayPlainIterator aIter(*pCode);
    while ( ( t = aIter.GetNextReference() ) != nullptr )
    {
        ScSingleRefData& rRef1 = *t->GetSingleRef();
        ScAddress aAbs = rRef1.toAbs(rDoc, aPos);
        if ( rRef1.IsTabRel() && !rRef1.IsTabDeleted() )
        {
            if (aAbs.Tab() < nMinTab)
                nMinTab = aAbs.Tab();
            if (aAbs.Tab() > nMaxTab)
                nMaxTab = aAbs.Tab();
        }
        if ( t->GetType() == svDoubleRef )
        {
            ScSingleRefData& rRef2 = t->GetDoubleRef()->Ref2;
            aAbs = rRef2.toAbs(rDoc, aPos);
            if ( rRef2.IsTabRel() && !rRef2.IsTabDeleted() )
            {
                if (aAbs.Tab() < nMinTab)
                    nMinTab = aAbs.Tab();
                if (aAbs.Tab() > nMaxTab)
                    nMaxTab = aAbs.Tab();
            }
        }
    }

    SCTAB nTabCount = rDoc.GetTableCount();
    if ( nMaxTab < nTabCount || nMinTab <= 0 )
        return;

    //  move position and relative tab refs
    //  The formulas that use the name are not changed by this

    SCTAB nMove = nMinTab;
    ScAddress aOldPos = aPos;
    aPos.SetTab( aPos.Tab() - nMove );

    aIter.Reset();
    while ( ( t = aIter.GetNextReference() ) != nullptr )
    {
        switch (t->GetType())
        {
            case svSingleRef:
            {
                ScSingleRefData& rRef = *t->GetSingleRef();
                if (!rRef.IsTabDeleted())
                {
                    ScAddress aAbs = rRef.toAbs(rDoc, aOldPos);
                    rRef.SetAddress(rDoc.GetSheetLimits(), aAbs, aPos);
                }
            }
            break;
            case svDoubleRef:
            {
                ScComplexRefData& rRef = *t->GetDoubleRef();
                if (!rRef.Ref1.IsTabDeleted())
                {
                    ScAddress aAbs = rRef.Ref1.toAbs(rDoc, aOldPos);
                    rRef.Ref1.SetAddress(rDoc.GetSheetLimits(), aAbs, aPos);
                }
                if (!rRef.Ref2.IsTabDeleted())
                {
                    ScAddress aAbs = rRef.Ref2.toAbs(rDoc, aOldPos);
                    rRef.Ref2.SetAddress(rDoc.GetSheetLimits(), aAbs, aPos);
                }
            }
            break;
            default:
                ;
        }
    }
}

void ScRangeData::SetCode( const ScTokenArray& rArr )
{
    pCode.reset(new ScTokenArray( rArr ));
    pCode->SetFromRangeName(true);
    InitCode();
}

void ScRangeData::InitCode()
{
    if( pCode->GetCodeError() == FormulaError::NONE )
    {
        FormulaToken* p = FormulaTokenArrayPlainIterator(*pCode).GetNextReference();
        if( p )   // exact one reference at first
        {
            if( p->GetType() == svSingleRef )
                eType = eType | Type::AbsPos;
            else
                eType = eType | Type::AbsArea;
        }
    }
}

extern "C"
int ScRangeData_QsortNameCompare( const void* p1, const void* p2 )
{
    return static_cast<int>(ScGlobal::GetCollator().compareString(
            (*static_cast<const ScRangeData* const *>(p1))->GetName(),
            (*static_cast<const ScRangeData* const *>(p2))->GetName() ));
}

namespace {

/**
 * Predicate to check if the name references the specified range.
 */
class MatchByRange
{
    const ScRange& mrRange;
public:
    explicit MatchByRange(const ScRange& rRange) : mrRange(rRange) {}
    bool operator() (std::pair<OUString const, std::unique_ptr<ScRangeData>> const& r) const
    {
        return r.second->IsRangeAtBlock(mrRange);
    }
};

}

ScRangeName::ScRangeName()
    : mHasPossibleAddressConflict(false)
    , mHasPossibleAddressConflictDirty(false)
{
}

ScRangeName::ScRangeName(const ScRangeName& r)
    : mHasPossibleAddressConflict( r.mHasPossibleAddressConflict )
    , mHasPossibleAddressConflictDirty( r.mHasPossibleAddressConflictDirty )
{
    for (auto const& it : r.m_Data)
    {
        m_Data.insert(std::make_pair(it.first, std::make_unique<ScRangeData>(*it.second)));
    }
    // std::map was cloned, so each collection needs its own index to data.
    maIndexToData.resize( r.maIndexToData.size(), nullptr);
    for (auto const& itr : m_Data)
    {
        size_t nPos = itr.second->GetIndex() - 1;
        if (nPos >= maIndexToData.size())
        {
            OSL_FAIL( "ScRangeName copy-ctor: maIndexToData size doesn't fit");
            maIndexToData.resize(nPos+1, nullptr);
        }
        maIndexToData[nPos] = itr.second.get();
    }
}

const ScRangeData* ScRangeName::findByRange(const ScRange& rRange) const
{
    DataType::const_iterator itr = std::find_if(
        m_Data.begin(), m_Data.end(), MatchByRange(rRange));
    return itr == m_Data.end() ? nullptr : itr->second.get();
}

ScRangeData* ScRangeName::findByUpperName(const OUString& rName)
{
    DataType::iterator itr = m_Data.find(rName);
    return itr == m_Data.end() ? nullptr : itr->second.get();
}

const ScRangeData* ScRangeName::findByUpperName(const OUString& rName) const
{
    DataType::const_iterator itr = m_Data.find(rName);
    return itr == m_Data.end() ? nullptr : itr->second.get();
}

ScRangeData* ScRangeName::findByIndex(sal_uInt16 i) const
{
    if (!i)
        // index should never be zero.
        return nullptr;

    size_t nPos = i - 1;
    return nPos < maIndexToData.size() ? maIndexToData[nPos] : nullptr;
}

void ScRangeName::UpdateReference(sc::RefUpdateContext& rCxt, SCTAB nLocalTab )
{
    if (rCxt.meMode == URM_COPY)
        // Copying cells does not modify named expressions.
        return;

    for (auto const& itr : m_Data)
    {
        itr.second->UpdateReference(rCxt, nLocalTab);
    }
}

void ScRangeName::UpdateInsertTab( sc::RefUpdateInsertTabContext& rCxt, SCTAB nLocalTab )
{
    for (auto const& itr : m_Data)
    {
        itr.second->UpdateInsertTab(rCxt, nLocalTab);
    }
}

void ScRangeName::UpdateDeleteTab( sc::RefUpdateDeleteTabContext& rCxt, SCTAB nLocalTab )
{
    for (auto const& itr : m_Data)
    {
        itr.second->UpdateDeleteTab(rCxt, nLocalTab);
    }
}

void ScRangeName::UpdateMoveTab( sc::RefUpdateMoveTabContext& rCxt, SCTAB nLocalTab )
{
    for (auto const& itr : m_Data)
    {
        itr.second->UpdateMoveTab(rCxt, nLocalTab);
    }
}

void ScRangeName::UpdateTranspose(const ScRange& rSource, const ScAddress& rDest)
{
    for (auto const& itr : m_Data)
    {
        itr.second->UpdateTranspose(rSource, rDest);
    }
}

void ScRangeName::UpdateGrow(const ScRange& rArea, SCCOL nGrowX, SCROW nGrowY)
{
    for (auto const& itr : m_Data)
    {
        itr.second->UpdateGrow(rArea, nGrowX, nGrowY);
    }
}

void ScRangeName::CompileUnresolvedXML( sc::CompileFormulaContext& rCxt )
{
    for (auto const& itr : m_Data)
    {
        itr.second->CompileUnresolvedXML(rCxt);
    }
}

void ScRangeName::CopyUsedNames( const SCTAB nLocalTab, const SCTAB nOldTab, const SCTAB nNewTab,
        const ScDocument& rOldDoc, ScDocument& rNewDoc, const bool bGlobalNamesToLocal ) const
{
    for (auto const& itr : m_Data)
    {
        SCTAB nSheet = (nLocalTab < 0) ? nLocalTab : nOldTab;
        sal_uInt16 nIndex = itr.second->GetIndex();
        ScAddress aOldPos( itr.second->GetPos());
        aOldPos.SetTab( nOldTab);
        ScAddress aNewPos( aOldPos);
        aNewPos.SetTab( nNewTab);
        ScRangeData* pRangeData = nullptr;
        rOldDoc.CopyAdjustRangeName( nSheet, nIndex, pRangeData, rNewDoc, aNewPos, aOldPos, bGlobalNamesToLocal, false);
    }
}

bool ScRangeName::insert( ScRangeData* p, bool bReuseFreeIndex )
{
    if (!p)
        return false;

    if (!p->GetIndex())
    {
        // Assign a new index.  An index must be unique and is never 0.
        if (bReuseFreeIndex)
        {
            IndexDataType::iterator itr = std::find(
                    maIndexToData.begin(), maIndexToData.end(), static_cast<ScRangeData*>(nullptr));
            if (itr != maIndexToData.end())
            {
                // Empty slot exists.  Re-use it.
                size_t nPos = std::distance(maIndexToData.begin(), itr);
                p->SetIndex(nPos + 1);
            }
            else
                // No empty slot.  Append it to the end.
                p->SetIndex(maIndexToData.size() + 1);
        }
        else
        {
            p->SetIndex(maIndexToData.size() + 1);
        }
    }

    OUString aName(p->GetUpperName());
    erase(aName); // ptr_map won't insert it if a duplicate name exists.
    pair<DataType::iterator, bool> r =
        m_Data.insert(std::make_pair(aName, std::unique_ptr<ScRangeData>(p)));
    if (r.second)
    {
        // Data inserted.  Store its index for mapping.
        size_t nPos = p->GetIndex() - 1;
        if (nPos >= maIndexToData.size())
            maIndexToData.resize(nPos+1, nullptr);
        maIndexToData[nPos] = p;
        mHasPossibleAddressConflictDirty = true;
    }
    return r.second;
}

void ScRangeName::erase(const ScRangeData& r)
{
    erase(r.GetUpperName());
}

void ScRangeName::erase(const OUString& rName)
{
    DataType::const_iterator itr = m_Data.find(rName);
    if (itr != m_Data.end())
        erase(itr);
}

void ScRangeName::erase(const_iterator itr)
{
    sal_uInt16 nIndex = itr->second->GetIndex();
    m_Data.erase(itr);
    OSL_ENSURE( 0 < nIndex && nIndex <= maIndexToData.size(), "ScRangeName::erase: bad index");
    if (0 < nIndex && nIndex <= maIndexToData.size())
        maIndexToData[nIndex-1] = nullptr;
    if(mHasPossibleAddressConflict)
        mHasPossibleAddressConflictDirty = true;
}

void ScRangeName::clear()
{
    m_Data.clear();
    maIndexToData.clear();
    mHasPossibleAddressConflict = false;
    mHasPossibleAddressConflictDirty = false;
}

void ScRangeName::checkHasPossibleAddressConflict() const
{
    mHasPossibleAddressConflict = false;
    mHasPossibleAddressConflictDirty = false;
    for (auto const& itr : m_Data)
    {
        if( itr.second->HasPossibleAddressConflict())
        {
            mHasPossibleAddressConflict = true;
            return;
        }
    }
}

bool ScRangeName::operator== (const ScRangeName& r) const
{
    return std::equal(m_Data.begin(), m_Data.end(), r.m_Data.begin(), r.m_Data.end(),
        [](const DataType::value_type& lhs, const DataType::value_type& rhs) {
            return (lhs.first == rhs.first) && (*lhs.second == *rhs.second);
        });
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
