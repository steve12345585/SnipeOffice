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

#include <functional>

#include <string.h>
#include <osl/diagnose.h>
#include <sal/log.hxx>

#include <token.hxx>
#include <tokenarray.hxx>
#include <reftokenhelper.hxx>
#include <clipparam.hxx>
#include <compiler.hxx>
#include <interpre.hxx>
#include <formula/FormulaCompiler.hxx>
#include <formula/compiler.hxx>
#include <formula/opcode.hxx>
#include <jumpmatrix.hxx>
#include <rangeseq.hxx>
#include <rangeutl.hxx>
#include <externalrefmgr.hxx>
#include <document.hxx>
#include <refupdatecontext.hxx>
#include <tokenstringcontext.hxx>
#include <types.hxx>
#include <addincol.hxx>
#include <dbdata.hxx>
#include <reordermap.hxx>
#include <svl/sharedstring.hxx>
#include <scmatrix.hxx>

#include <com/sun/star/sheet/ComplexReference.hpp>
#include <com/sun/star/sheet/ExternalReference.hpp>
#include <com/sun/star/sheet/FormulaToken.hpp>
#include <com/sun/star/sheet/ReferenceFlags.hpp>
#include <com/sun/star/sheet/NameToken.hpp>
#include <com/sun/star/sheet/TableRefToken.hpp>
#include <utility>
#include <o3tl/safeint.hxx>
#include <o3tl/sorted_vector.hxx>

using ::std::vector;
using namespace formula;
using namespace com::sun::star;

namespace
{
    void lcl_SingleRefToCalc( ScSingleRefData& rRef, const sheet::SingleReference& rAPI )
    {
        rRef.InitFlags();

        rRef.SetColRel(     ( rAPI.Flags & sheet::ReferenceFlags::COLUMN_RELATIVE ) != 0 );
        rRef.SetRowRel(     ( rAPI.Flags & sheet::ReferenceFlags::ROW_RELATIVE    ) != 0 );
        rRef.SetTabRel(     ( rAPI.Flags & sheet::ReferenceFlags::SHEET_RELATIVE  ) != 0 );
        rRef.SetColDeleted( ( rAPI.Flags & sheet::ReferenceFlags::COLUMN_DELETED  ) != 0 );
        rRef.SetRowDeleted( ( rAPI.Flags & sheet::ReferenceFlags::ROW_DELETED     ) != 0 );
        rRef.SetTabDeleted( ( rAPI.Flags & sheet::ReferenceFlags::SHEET_DELETED   ) != 0 );
        rRef.SetFlag3D(     ( rAPI.Flags & sheet::ReferenceFlags::SHEET_3D        ) != 0 );
        rRef.SetRelName(    ( rAPI.Flags & sheet::ReferenceFlags::RELATIVE_NAME   ) != 0 );

        if (rRef.IsColRel())
            rRef.SetRelCol(static_cast<SCCOL>(rAPI.RelativeColumn));
        else
            rRef.SetAbsCol(static_cast<SCCOL>(rAPI.Column));

        if (rRef.IsRowRel())
            rRef.SetRelRow(static_cast<SCROW>(rAPI.RelativeRow));
        else
            rRef.SetAbsRow(static_cast<SCROW>(rAPI.Row));

        if (rRef.IsTabRel())
            rRef.SetRelTab(static_cast<SCTAB>(rAPI.RelativeSheet));
        else
            rRef.SetAbsTab(static_cast<SCTAB>(rAPI.Sheet));
    }

    void lcl_ExternalRefToCalc( ScSingleRefData& rRef, const sheet::SingleReference& rAPI )
    {
        rRef.InitFlags();

        rRef.SetColRel(     ( rAPI.Flags & sheet::ReferenceFlags::COLUMN_RELATIVE ) != 0 );
        rRef.SetRowRel(     ( rAPI.Flags & sheet::ReferenceFlags::ROW_RELATIVE    ) != 0 );
        rRef.SetColDeleted( ( rAPI.Flags & sheet::ReferenceFlags::COLUMN_DELETED  ) != 0 );
        rRef.SetRowDeleted( ( rAPI.Flags & sheet::ReferenceFlags::ROW_DELETED     ) != 0 );
        rRef.SetTabDeleted( false );    // sheet must not be deleted for external refs
        rRef.SetFlag3D(     ( rAPI.Flags & sheet::ReferenceFlags::SHEET_3D        ) != 0 );
        rRef.SetRelName(    false );

        if (rRef.IsColRel())
            rRef.SetRelCol(static_cast<SCCOL>(rAPI.RelativeColumn));
        else
            rRef.SetAbsCol(static_cast<SCCOL>(rAPI.Column));

        if (rRef.IsRowRel())
            rRef.SetRelRow(static_cast<SCROW>(rAPI.RelativeRow));
        else
            rRef.SetAbsRow(static_cast<SCROW>(rAPI.Row));

        // sheet index must be absolute for external refs
        rRef.SetAbsTab(0);
    }

    struct TokenPointerRange
    {
        FormulaToken**  mpStart;
        FormulaToken**  mpStop;

        TokenPointerRange() : mpStart(nullptr), mpStop(nullptr) {}
        TokenPointerRange( FormulaToken** p, sal_uInt16 n ) :
            mpStart(p), mpStop( p + static_cast<size_t>(n)) {}
    };
    struct TokenPointers
    {
        TokenPointerRange maPointerRange[2];
        bool              mbSkipRelName;

        TokenPointers( FormulaToken** pCode, sal_uInt16 nLen, FormulaToken** pRPN, sal_uInt16 nRPN,
                bool bSkipRelName = true ) :
            mbSkipRelName(bSkipRelName)
        {
            maPointerRange[0] = TokenPointerRange( pCode, nLen);
            maPointerRange[1] = TokenPointerRange( pRPN, nRPN);
        }

        bool skipToken( size_t i, const FormulaToken* const * pp )
        {
            // Handle all code tokens, and tokens in RPN only if they have a
            // reference count of 1, which means they are not referenced in the
            // code array. Doing it the other way would skip code tokens that
            // are held by flat copied token arrays and thus are shared. For
            // flat copy arrays the caller has to know what it does and should
            // discard all RPN, update only one array and regenerate all RPN.
            if (i == 1)
            {
                if ((*pp)->GetRef() > 1)
                    return true;

                if (mbSkipRelName)
                {
                    // Skip (do not adjust) relative references resulting from
                    // named expressions. Resolved expressions are only in RPN.
                    switch ((*pp)->GetType())
                    {
                        case svSingleRef:
                            return (*pp)->GetSingleRef()->IsRelName();
                        case svDoubleRef:
                            {
                                const ScComplexRefData& rRef = *(*pp)->GetDoubleRef();
                                return rRef.Ref1.IsRelName() || rRef.Ref2.IsRelName();
                            }
                        default:
                            ;   // nothing
                    }
                }
            }

            return false;
        }

        FormulaToken* getHandledToken( size_t i, FormulaToken* const * pp )
        {
            if (skipToken( i, pp))
                return nullptr;

            FormulaToken* p = *pp;
            if (p->GetOpCode() == ocTableRef)
            {
                // Return the inner reference token if it is not in RPN.
                ScTableRefToken* pTR = dynamic_cast<ScTableRefToken*>(p);
                if (!pTR)
                    return p;
                p = pTR->GetAreaRefRPN();
                if (!p)
                    return pTR;
                if (p->GetRef() > 1)
                    // Reference handled in RPN, but do not return nullptr so
                    // loops will process ocTableRef via pp instead of issuing
                    // a continue.
                    return pTR;
            }
            return p;
        }
    };

} // namespace


// --- class ScRawToken -----------------------------------------------------

void ScRawToken::SetOpCode( OpCode e )
{
    eOp   = e;
    switch (eOp)
    {
        case ocIf:
            eType = svJump;
            nJump[ 0 ] = 3; // If, Else, Behind
            break;
        case ocIfError:
        case ocIfNA:
            eType = svJump;
            nJump[ 0 ] = 2; // If, Behind
            break;
        case ocChoose:
            eType = svJump;
            nJump[ 0 ] = FORMULA_MAXJUMPCOUNT + 1;
            break;
        case ocLet:
            eType = svJump;
            nJump[ 0 ] = FORMULA_MAXPARAMS + 1;
            break;
        case ocMissing:
            eType = svMissing;
            break;
        case ocSep:
        case ocOpen:
        case ocClose:
        case ocArrayRowSep:
        case ocArrayColSep:
        case ocArrayOpen:
        case ocArrayClose:
        case ocTableRefOpen:
        case ocTableRefClose:
            eType = svSep;
            break;
        case ocWhitespace:
            eType = svByte;
            whitespace.nCount = 1;
            whitespace.cChar = 0x20;
            break;
        default:
            eType = svByte;
            sbyte.cByte = 0;
            sbyte.eInForceArray = ParamClass::Unknown;
    }
}

void ScRawToken::SetString( rtl_uString* pData, rtl_uString* pDataIgnoreCase )
{
    eOp   = ocPush;
    eType = svString;

    sharedstring.mpData = pData;
    sharedstring.mpDataIgnoreCase = pDataIgnoreCase;
}

void ScRawToken::SetStringName( rtl_uString* pData, rtl_uString* pDataIgnoreCase )
{
    eOp = ocStringName;
    eType = svString;

    sharedstring.mpData = pData;
    sharedstring.mpDataIgnoreCase = pDataIgnoreCase;
}

void ScRawToken::SetSingleReference( const ScSingleRefData& rRef )
{
    eOp       = ocPush;
    eType     = svSingleRef;
    aRef.Ref1 =
    aRef.Ref2 = rRef;
}

void ScRawToken::SetDoubleReference( const ScComplexRefData& rRef )
{
    eOp   = ocPush;
    eType = svDoubleRef;
    aRef  = rRef;
}

void ScRawToken::SetDouble(double rVal)
{
    eOp   = ocPush;
    eType = svDouble;
    nValue = rVal;
}

void ScRawToken::SetErrorConstant( FormulaError nErr )
{
    eOp   = ocPush;
    eType = svError;
    nError = nErr;
}

void ScRawToken::SetName(sal_Int16 nSheet, sal_uInt16 nIndex)
{
    eOp = ocName;
    eType = svIndex;

    name.nSheet = nSheet;
    name.nIndex = nIndex;
}

void ScRawToken::SetExternalSingleRef( sal_uInt16 nFileId, const OUString& rTabName, const ScSingleRefData& rRef )
{
    eOp = ocPush;
    eType = svExternalSingleRef;

    extref.nFileId = nFileId;
    extref.aRef.Ref1 =
    extref.aRef.Ref2 = rRef;
    maExternalName = rTabName;
}

void ScRawToken::SetExternalDoubleRef( sal_uInt16 nFileId, const OUString& rTabName, const ScComplexRefData& rRef )
{
    eOp = ocPush;
    eType = svExternalDoubleRef;

    extref.nFileId = nFileId;
    extref.aRef = rRef;
    maExternalName = rTabName;
}

void ScRawToken::SetExternalName( sal_uInt16 nFileId, const OUString& rName )
{
    eOp = ocPush;
    eType = svExternalName;

    extname.nFileId = nFileId;
    maExternalName = rName;
}

void ScRawToken::SetExternal( const OUString& rStr )
{
    eOp   = ocExternal;
    eType = svExternal;
    maExternalName = rStr;
}

bool ScRawToken::IsValidReference(const ScDocument& rDoc) const
{
    switch (eType)
    {
        case svSingleRef:
            return aRef.Ref1.Valid(rDoc);
        case svDoubleRef:
            return aRef.Valid(rDoc);
        case svExternalSingleRef:
        case svExternalDoubleRef:
            return true;
        default:
            ;   // nothing
    }
    return false;
}

FormulaToken* ScRawToken::CreateToken(ScSheetLimits& rLimits) const
{
#define IF_NOT_OPCODE_ERROR(o,c) SAL_WARN_IF((eOp!=o), "sc.core", #c "::ctor: OpCode " << static_cast<int>(eOp) << " lost, converted to " #o "; maybe inherit from FormulaToken instead!")
    switch ( GetType() )
    {
        case svByte :
            if (eOp == ocWhitespace)
                return new FormulaSpaceToken( whitespace.nCount, whitespace.cChar );
            else
                return new FormulaByteToken( eOp, sbyte.cByte, sbyte.eInForceArray );
        case svDouble :
            IF_NOT_OPCODE_ERROR( ocPush, FormulaDoubleToken);
            return new FormulaDoubleToken( nValue );
        case svString :
        {
            svl::SharedString aSS(sharedstring.mpData, sharedstring.mpDataIgnoreCase);
            if (eOp == ocPush)
                return new FormulaStringToken(std::move(aSS));
            else
                return new FormulaStringOpToken(eOp, std::move(aSS));
        }
        case svSingleRef :
            return new ScSingleRefToken(rLimits, aRef.Ref1, eOp);
        case svDoubleRef :
            return new ScDoubleRefToken(rLimits, aRef, eOp);
        case svMatrix :
            IF_NOT_OPCODE_ERROR( ocPush, ScMatrixToken);
            return new ScMatrixToken( pMat );
        case svIndex :
            if (eOp == ocTableRef)
                return new ScTableRefToken( table.nIndex, table.eItem);
            else
                return new FormulaIndexToken( eOp, name.nIndex, name.nSheet);
        case svExternalSingleRef:
            {
                svl::SharedString aTabName(maExternalName);    // string not interned
                return new ScExternalSingleRefToken(extref.nFileId, std::move(aTabName), extref.aRef.Ref1);
            }
        case svExternalDoubleRef:
            {
                svl::SharedString aTabName(maExternalName);    // string not interned
                return new ScExternalDoubleRefToken(extref.nFileId, std::move(aTabName), extref.aRef);
            }
        case svExternalName:
            {
                svl::SharedString aName(maExternalName);         // string not interned
                return new ScExternalNameToken( extname.nFileId, std::move(aName) );
            }
        case svJump :
            return new FormulaJumpToken( eOp, nJump );
        case svExternal :
            return new FormulaExternalToken( eOp, sbyte.cByte, maExternalName );
        case svFAP :
            return new FormulaFAPToken( eOp, sbyte.cByte, nullptr );
        case svMissing :
            IF_NOT_OPCODE_ERROR( ocMissing, FormulaMissingToken);
            return new FormulaMissingToken;
        case svSep :
            return new FormulaToken( svSep,eOp );
        case svError :
            return new FormulaErrorToken( nError );
        case svUnknown :
            return new FormulaUnknownToken( eOp );
        default:
            {
                SAL_WARN("sc.core",  "unknown ScRawToken::CreateToken() type " << int(GetType()));
                return new FormulaUnknownToken( ocBad );
            }
    }
#undef IF_NOT_OPCODE_ERROR
}

namespace {

//  TextEqual: if same formula entered (for optimization in sort)
bool checkTextEqual( const ScSheetLimits& rLimits, const FormulaToken& _rToken1, const FormulaToken& _rToken2 )
{
    assert(
        (_rToken1.GetType() == svSingleRef || _rToken1.GetType() == svDoubleRef)
        && _rToken1.FormulaToken::operator ==(_rToken2));

    //  in relative Refs only compare relative parts

    ScComplexRefData aTemp1;
    if ( _rToken1.GetType() == svSingleRef )
    {
        aTemp1.Ref1 = *_rToken1.GetSingleRef();
        aTemp1.Ref2 = aTemp1.Ref1;
    }
    else
        aTemp1 = *_rToken1.GetDoubleRef();

    ScComplexRefData aTemp2;
    if ( _rToken2.GetType() == svSingleRef )
    {
        aTemp2.Ref1 = *_rToken2.GetSingleRef();
        aTemp2.Ref2 = aTemp2.Ref1;
    }
    else
        aTemp2 = *_rToken2.GetDoubleRef();

    ScAddress aPos;
    ScRange aRange1 = aTemp1.toAbs(rLimits, aPos), aRange2 = aTemp2.toAbs(rLimits, aPos);

    //  memcmp doesn't work because of the alignment byte after bFlags.
    //  After SmartRelAbs only absolute parts have to be compared.
    return aRange1 == aRange2 && aTemp1.Ref1.FlagValue() == aTemp2.Ref1.FlagValue() && aTemp1.Ref2.FlagValue() == aTemp2.Ref2.FlagValue();
}

}

#if DEBUG_FORMULA_COMPILER
void DumpToken(formula::FormulaToken const & rToken)
{
    switch (rToken.GetType()) {
    case svSingleRef:
        cout << "-- ScSingleRefToken" << endl;
        rToken.GetSingleRef()->Dump(1);
        break;
    case svDoubleRef:
        cout << "-- ScDoubleRefToken" << endl;
        rToken.GetDoubleRef()->Dump(1);
        break;
    default:
        cout << "-- FormulaToken" << endl;
        cout << "  opcode: " << int(rToken.GetOpCode()) << " " <<
            formula::FormulaCompiler::GetNativeSymbol( rToken.GetOpCode()).toUtf8().getStr() << endl;
        cout << "  type: " << static_cast<int>(rToken.GetType()) << endl;
        switch (rToken.GetType())
        {
        case svDouble:
            cout << "  value: " << rToken.GetDouble() << endl;
            break;
        case svString:
            cout << "  string: "
                 << OUStringToOString(rToken.GetString().getString(), RTL_TEXTENCODING_UTF8).getStr()
                 << endl;
            break;
        default:
            ;
        }
        break;
    }
}
#endif

FormulaTokenRef extendRangeReference( ScSheetLimits& rLimits, FormulaToken & rTok1, FormulaToken & rTok2,
        const ScAddress & rPos, bool bReuseDoubleRef )
{

    StackVar sv1 = rTok1.GetType();
    // Doing a RangeOp with RefList is probably utter nonsense, but Xcl
    // supports it, so do we.
    if (sv1 != svSingleRef && sv1 != svDoubleRef && sv1 != svRefList
         && sv1 != svExternalSingleRef && sv1 != svExternalDoubleRef)
        return nullptr;
    StackVar sv2 = rTok2.GetType();
    if (sv2 != svSingleRef && sv2 != svDoubleRef && sv2 != svRefList)
        return nullptr;

    ScTokenRef xRes;
    bool bExternal = (sv1 == svExternalSingleRef);
    if ((sv1 == svSingleRef || bExternal) && sv2 == svSingleRef)
    {
        // Range references like Sheet1.A1:A2 are generalized and built by
        // first creating a DoubleRef from the first SingleRef, effectively
        // generating Sheet1.A1:A1, and then extending that with A2 as if
        // Sheet1.A1:A1:A2 was encountered, so the mechanisms to adjust the
        // references apply as well.

        /* Given the current structure of external references an external
         * reference can only be extended if the second reference does not
         * point to a different sheet. 'file'#Sheet1.A1:A2 is ok,
         * 'file'#Sheet1.A1:Sheet2.A2 is not. Since we can't determine from a
         * svSingleRef whether the sheet would be different from the one given
         * in the external reference, we have to bail out if there is any sheet
         * specified. NOTE: Xcl does handle external 3D references as in
         * '[file]Sheet1:Sheet2'!A1:A2
         *
         * FIXME: For OOo syntax be smart and remember an external singleref
         * encountered and if followed by ocRange and singleref, create an
         * external singleref for the second singleref. Both could then be
         * merged here. For Xcl syntax already parse an external range
         * reference entirely, cumbersome. */

        const ScSingleRefData& rRef2 = *rTok2.GetSingleRef();
        if (bExternal && rRef2.IsFlag3D())
            return nullptr;

        ScComplexRefData aRef;
        aRef.Ref1 = aRef.Ref2 = *rTok1.GetSingleRef();
        aRef.Ref2.SetFlag3D( false);
        aRef.Extend(rLimits, rRef2, rPos);
        if (bExternal)
            xRes = new ScExternalDoubleRefToken( rTok1.GetIndex(), rTok1.GetString(), aRef);
        else
            xRes = new ScDoubleRefToken(rLimits, aRef);
    }
    else
    {
        bExternal |= (sv1 == svExternalDoubleRef);
        const ScRefList* pRefList = nullptr;
        if (sv1 == svDoubleRef)
        {
            xRes = (bReuseDoubleRef && rTok1.GetRef() == 1 ? &rTok1 : rTok1.Clone());
            sv1 = svUnknown;    // mark as handled
        }
        else if (sv2 == svDoubleRef)
        {
            xRes = (bReuseDoubleRef && rTok2.GetRef() == 1 ? &rTok2 : rTok2.Clone());
            sv2 = svUnknown;    // mark as handled
        }
        else if (sv1 == svRefList)
            pRefList = rTok1.GetRefList();
        else if (sv2 == svRefList)
            pRefList = rTok2.GetRefList();
        if (pRefList)
        {
            if (pRefList->empty())
                return nullptr;
            if (bExternal)
                return nullptr;    // external reference list not possible
            xRes = new ScDoubleRefToken(rLimits, (*pRefList)[0] );
        }
        if (!xRes)
            return nullptr;    // shouldn't happen...
        StackVar sv[2] = { sv1, sv2 };
        formula::FormulaToken* pt[2] = { &rTok1, &rTok2 };
        ScComplexRefData& rRef = *xRes->GetDoubleRef();
        for (size_t i=0; i<2; ++i)
        {
            switch (sv[i])
            {
                case svSingleRef:
                    rRef.Extend(rLimits, *pt[i]->GetSingleRef(), rPos);
                    break;
                case svDoubleRef:
                    rRef.Extend(rLimits, *pt[i]->GetDoubleRef(), rPos);
                    break;
                case svRefList:
                    {
                        const ScRefList* p = pt[i]->GetRefList();
                        if (p->empty())
                            return nullptr;
                        for (const auto& rRefData : *p)
                        {
                            rRef.Extend(rLimits, rRefData, rPos);
                        }
                    }
                    break;
                case svExternalSingleRef:
                    if (rRef.Ref1.IsFlag3D() || rRef.Ref2.IsFlag3D())
                        return nullptr;    // no other sheets with external refs
                    else
                        rRef.Extend(rLimits, *pt[i]->GetSingleRef(), rPos);
                    break;
                case svExternalDoubleRef:
                    if (rRef.Ref1.IsFlag3D() || rRef.Ref2.IsFlag3D())
                        return nullptr;    // no other sheets with external refs
                    else
                        rRef.Extend(rLimits, *pt[i]->GetDoubleRef(), rPos);
                    break;
                default:
                    ;   // nothing, prevent compiler warning
            }
        }
    }
    return FormulaTokenRef(xRes.get());
}

// real implementations of virtual functions

const ScSingleRefData*    ScSingleRefToken::GetSingleRef() const  { return &aSingleRef; }
ScSingleRefData*          ScSingleRefToken::GetSingleRef()        { return &aSingleRef; }
bool ScSingleRefToken::TextEqual( const FormulaToken& _rToken ) const
{
    return FormulaToken::operator ==(_rToken) && checkTextEqual(mrSheetLimits, *this, _rToken);
}
bool ScSingleRefToken::operator==( const FormulaToken& r ) const
{
    return FormulaToken::operator==( r ) && aSingleRef == *r.GetSingleRef();
}

const ScSingleRefData*    ScDoubleRefToken::GetSingleRef() const  { return &aDoubleRef.Ref1; }
ScSingleRefData*          ScDoubleRefToken::GetSingleRef()        { return &aDoubleRef.Ref1; }
const ScComplexRefData*     ScDoubleRefToken::GetDoubleRef() const  { return &aDoubleRef; }
ScComplexRefData*           ScDoubleRefToken::GetDoubleRef()        { return &aDoubleRef; }
const ScSingleRefData*    ScDoubleRefToken::GetSingleRef2() const { return &aDoubleRef.Ref2; }
ScSingleRefData*          ScDoubleRefToken::GetSingleRef2()       { return &aDoubleRef.Ref2; }
bool ScDoubleRefToken::TextEqual( const FormulaToken& _rToken ) const
{
    return FormulaToken::operator ==(_rToken) && checkTextEqual(mrSheetLimits, *this, _rToken);
}
bool ScDoubleRefToken::operator==( const FormulaToken& r ) const
{
    return FormulaToken::operator==( r ) && aDoubleRef == *r.GetDoubleRef();
}

const ScRefList*        ScRefListToken::GetRefList() const  { return &aRefList; }
      ScRefList*        ScRefListToken::GetRefList()        { return &aRefList; }
      bool              ScRefListToken::IsArrayResult() const { return mbArrayResult; }
bool ScRefListToken::operator==( const FormulaToken& r ) const
{
    if (!FormulaToken::operator==( r ) || &aRefList != r.GetRefList())
        return false;
    const ScRefListToken* p = dynamic_cast<const ScRefListToken*>(&r);
    return p && mbArrayResult == p->IsArrayResult();
}

ScMatrixToken::ScMatrixToken( ScMatrixRef p ) :
    FormulaToken(formula::svMatrix), pMatrix(std::move(p)) {}

ScMatrixToken::ScMatrixToken( const ScMatrixToken& ) = default;

const ScMatrix* ScMatrixToken::GetMatrix() const        { return pMatrix.get(); }
ScMatrix*       ScMatrixToken::GetMatrix()              { return pMatrix.get(); }
bool ScMatrixToken::operator==( const FormulaToken& r ) const
{
    return FormulaToken::operator==( r ) && pMatrix == r.GetMatrix();
}

ScMatrixRangeToken::ScMatrixRangeToken( const sc::RangeMatrix& rMat ) :
    FormulaToken(formula::svMatrix), mpMatrix(rMat.mpMat)
{
    maRef.InitRange(rMat.mnCol1, rMat.mnRow1, rMat.mnTab1, rMat.mnCol2, rMat.mnRow2, rMat.mnTab2);
}

ScMatrixRangeToken::ScMatrixRangeToken( const ScMatrixRangeToken& ) = default;

sal_uInt8 ScMatrixRangeToken::GetByte() const
{
    return MATRIX_TOKEN_HAS_RANGE;
}

const ScMatrix* ScMatrixRangeToken::GetMatrix() const
{
    return mpMatrix.get();
}

ScMatrix* ScMatrixRangeToken::GetMatrix()
{
    return mpMatrix.get();
}

const ScComplexRefData* ScMatrixRangeToken::GetDoubleRef() const
{
    return &maRef;
}

ScComplexRefData* ScMatrixRangeToken::GetDoubleRef()
{
    return &maRef;
}

bool ScMatrixRangeToken::operator==( const FormulaToken& r ) const
{
    return FormulaToken::operator==(r) && mpMatrix == r.GetMatrix();
}

FormulaToken* ScMatrixRangeToken::Clone() const
{
    return new ScMatrixRangeToken(*this);
}

ScExternalSingleRefToken::ScExternalSingleRefToken( sal_uInt16 nFileId, svl::SharedString aTabName, const ScSingleRefData& r ) :
    FormulaToken( svExternalSingleRef, ocPush),
    mnFileId(nFileId),
    maTabName(std::move(aTabName)),
    maSingleRef(r)
{
}

ScExternalSingleRefToken::~ScExternalSingleRefToken()
{
}

sal_uInt16 ScExternalSingleRefToken::GetIndex() const
{
    return mnFileId;
}

const svl::SharedString & ScExternalSingleRefToken::GetString() const
{
    return maTabName;
}

const ScSingleRefData* ScExternalSingleRefToken::GetSingleRef() const
{
    return &maSingleRef;
}

ScSingleRefData* ScExternalSingleRefToken::GetSingleRef()
{
    return &maSingleRef;
}

bool ScExternalSingleRefToken::operator ==( const FormulaToken& r ) const
{
    if (!FormulaToken::operator==(r))
        return false;

    if (mnFileId != r.GetIndex())
        return false;

    if (maTabName != r.GetString())
        return false;

    return maSingleRef == *r.GetSingleRef();
}

ScExternalDoubleRefToken::ScExternalDoubleRefToken( sal_uInt16 nFileId, svl::SharedString aTabName, const ScComplexRefData& r ) :
    FormulaToken( svExternalDoubleRef, ocPush),
    mnFileId(nFileId),
    maTabName(std::move(aTabName)),
    maDoubleRef(r)
{
}

ScExternalDoubleRefToken::~ScExternalDoubleRefToken()
{
}

sal_uInt16 ScExternalDoubleRefToken::GetIndex() const
{
    return mnFileId;
}

const svl::SharedString & ScExternalDoubleRefToken::GetString() const
{
    return maTabName;
}

const ScSingleRefData* ScExternalDoubleRefToken::GetSingleRef() const
{
    return &maDoubleRef.Ref1;
}

ScSingleRefData* ScExternalDoubleRefToken::GetSingleRef()
{
    return &maDoubleRef.Ref1;
}

const ScSingleRefData* ScExternalDoubleRefToken::GetSingleRef2() const
{
    return &maDoubleRef.Ref2;
}

ScSingleRefData* ScExternalDoubleRefToken::GetSingleRef2()
{
    return &maDoubleRef.Ref2;
}

const ScComplexRefData* ScExternalDoubleRefToken::GetDoubleRef() const
{
    return &maDoubleRef;
}

ScComplexRefData* ScExternalDoubleRefToken::GetDoubleRef()
{
    return &maDoubleRef;
}

bool ScExternalDoubleRefToken::operator ==( const FormulaToken& r ) const
{
    if (!FormulaToken::operator==(r))
        return false;

    if (mnFileId != r.GetIndex())
        return false;

    if (maTabName != r.GetString())
        return false;

    return maDoubleRef == *r.GetDoubleRef();
}

ScExternalNameToken::ScExternalNameToken( sal_uInt16 nFileId, svl::SharedString aName ) :
    FormulaToken( svExternalName, ocPush),
    mnFileId(nFileId),
    maName(std::move(aName))
{
}

ScExternalNameToken::~ScExternalNameToken() {}

sal_uInt16 ScExternalNameToken::GetIndex() const
{
    return mnFileId;
}

const svl::SharedString & ScExternalNameToken::GetString() const
{
    return maName;
}

bool ScExternalNameToken::operator==( const FormulaToken& r ) const
{
    if ( !FormulaToken::operator==(r) )
        return false;

    if (mnFileId != r.GetIndex())
        return false;

    return maName == r.GetString();
}

ScTableRefToken::ScTableRefToken( sal_uInt16 nIndex, ScTableRefToken::Item eItem ) :
    FormulaToken( svIndex, ocTableRef),
    mnIndex(nIndex),
    meItem(eItem)
{
}

ScTableRefToken::ScTableRefToken( const ScTableRefToken& r ) :
    FormulaToken(r),
    mxAreaRefRPN( r.mxAreaRefRPN ?  r.mxAreaRefRPN->Clone() : nullptr),
    mnIndex(r.mnIndex),
    meItem(r.meItem)
{
}

ScTableRefToken::~ScTableRefToken() {}

sal_uInt16 ScTableRefToken::GetIndex() const
{
    return mnIndex;
}

void ScTableRefToken::SetIndex( sal_uInt16 n )
{
    mnIndex = n;
}

sal_Int16 ScTableRefToken::GetSheet() const
{
    // Code asking for this may have to be adapted as it might assume an
    // svIndex token would always be ocName or ocDBArea.
    SAL_WARN("sc.core","ScTableRefToken::GetSheet - maybe adapt caller to know about TableRef?");
    // Database range is always global.
    return -1;
}

ScTableRefToken::Item ScTableRefToken::GetItem() const
{
    return meItem;
}

void ScTableRefToken::AddItem( ScTableRefToken::Item eItem )
{
    meItem = static_cast<ScTableRefToken::Item>(meItem | eItem);
}

void ScTableRefToken::SetAreaRefRPN( formula::FormulaToken* pToken )
{
    mxAreaRefRPN = pToken;
}

formula::FormulaToken* ScTableRefToken::GetAreaRefRPN() const
{
    return mxAreaRefRPN.get();
}

bool ScTableRefToken::operator==( const FormulaToken& r ) const
{
    if ( !FormulaToken::operator==(r) )
        return false;

    if (mnIndex != r.GetIndex())
        return false;

    const ScTableRefToken* p = dynamic_cast<const ScTableRefToken*>(&r);
    if (!p)
        return false;

    if (meItem != p->GetItem())
        return false;

    if (!mxAreaRefRPN && !p->mxAreaRefRPN)
        ;   // nothing
    else if (!mxAreaRefRPN || !p->mxAreaRefRPN)
        return false;
    else if (!(*mxAreaRefRPN == *(p->mxAreaRefRPN)))
        return false;

    return true;
}

ScJumpMatrixToken::ScJumpMatrixToken(std::shared_ptr<ScJumpMatrix> p)
    : FormulaToken(formula::svJumpMatrix)
    , mpJumpMatrix(std::move(p))
{}

ScJumpMatrixToken::ScJumpMatrixToken( const ScJumpMatrixToken & ) = default;

ScJumpMatrix* ScJumpMatrixToken::GetJumpMatrix() const
{
    return mpJumpMatrix.get();
}

bool ScJumpMatrixToken::operator==( const FormulaToken& r ) const
{
    return FormulaToken::operator==( r ) && mpJumpMatrix.get() == r.GetJumpMatrix();
}

ScJumpMatrixToken::~ScJumpMatrixToken()
{
}

double          ScEmptyCellToken::GetDouble() const     { return 0.0; }

const svl::SharedString & ScEmptyCellToken::GetString() const
{
    return svl::SharedString::getEmptyString();
}

bool ScEmptyCellToken::operator==( const FormulaToken& r ) const
{
    return FormulaToken::operator==( r ) &&
        bInherited == static_cast< const ScEmptyCellToken & >(r).IsInherited() &&
        bDisplayedAsString == static_cast< const ScEmptyCellToken & >(r).IsDisplayedAsString();
}

ScMatrixCellResultToken::ScMatrixCellResultToken( ScConstMatrixRef pMat, const formula::FormulaToken* pUL ) :
    FormulaToken(formula::svMatrixCell), xMatrix(std::move(pMat)), xUpperLeft(pUL) {}

ScMatrixCellResultToken::ScMatrixCellResultToken( const ScMatrixCellResultToken& ) = default;

double          ScMatrixCellResultToken::GetDouble() const  { return xUpperLeft->GetDouble(); }

ScMatrixCellResultToken::~ScMatrixCellResultToken() {}

const svl::SharedString & ScMatrixCellResultToken::GetString() const
{
    return xUpperLeft->GetString();
}

const ScMatrix* ScMatrixCellResultToken::GetMatrix() const  { return xMatrix.get(); }
// Non-const GetMatrix() is private and unused but must be implemented to
// satisfy vtable linkage.
ScMatrix* ScMatrixCellResultToken::GetMatrix()
{
    return const_cast<ScMatrix*>(xMatrix.get());
}

bool ScMatrixCellResultToken::operator==( const FormulaToken& r ) const
{
    return FormulaToken::operator==( r ) &&
        xUpperLeft == static_cast<const ScMatrixCellResultToken &>(r).xUpperLeft &&
        xMatrix == static_cast<const ScMatrixCellResultToken &>(r).xMatrix;
}

FormulaToken* ScMatrixCellResultToken::Clone() const
{
    return new ScMatrixCellResultToken(*this);
}

void ScMatrixCellResultToken::Assign( const ScMatrixCellResultToken & r )
{
    xMatrix = r.xMatrix;
    xUpperLeft = r.xUpperLeft;
}

ScMatrixFormulaCellToken::ScMatrixFormulaCellToken(
    SCCOL nC, SCROW nR, const ScConstMatrixRef& pMat, const formula::FormulaToken* pUL ) :
    ScMatrixCellResultToken(pMat, pUL), nRows(nR), nCols(nC)
{
    CloneUpperLeftIfNecessary();
}

ScMatrixFormulaCellToken::ScMatrixFormulaCellToken( SCCOL nC, SCROW nR ) :
    ScMatrixCellResultToken(nullptr, nullptr), nRows(nR), nCols(nC) {}

ScMatrixFormulaCellToken::ScMatrixFormulaCellToken( const ScMatrixFormulaCellToken& r ) :
    ScMatrixCellResultToken(r), nRows(r.nRows), nCols(r.nCols)
{
    CloneUpperLeftIfNecessary();
}

ScMatrixFormulaCellToken::~ScMatrixFormulaCellToken() {}

bool ScMatrixFormulaCellToken::operator==( const FormulaToken& r ) const
{
    const ScMatrixFormulaCellToken* p = dynamic_cast<const ScMatrixFormulaCellToken*>(&r);
    return p && ScMatrixCellResultToken::operator==( r ) &&
        nCols == p->nCols && nRows == p->nRows;
}

void ScMatrixFormulaCellToken::CloneUpperLeftIfNecessary()
{
    if (xUpperLeft && xUpperLeft->GetType() == svDouble)
        xUpperLeft = xUpperLeft->Clone();
}

void ScMatrixFormulaCellToken::Assign( const ScMatrixCellResultToken & r )
{
    ScMatrixCellResultToken::Assign( r);

    CloneUpperLeftIfNecessary();
}

void ScMatrixFormulaCellToken::Assign( const formula::FormulaToken& r )
{
    if (this == &r)
        return;
    const ScMatrixCellResultToken* p = dynamic_cast<const ScMatrixCellResultToken*>(&r);
    if (p)
        ScMatrixCellResultToken::Assign( *p);
    else
    {
        OSL_ENSURE( r.GetType() != svMatrix, "ScMatrixFormulaCellToken::operator=: assigning ScMatrixToken to ScMatrixFormulaCellToken is not proper, use ScMatrixCellResultToken instead");
        if (r.GetType() == svMatrix)
        {
            xUpperLeft = nullptr;
            xMatrix = r.GetMatrix();
        }
        else
        {
            xUpperLeft = &r;
            xMatrix = nullptr;
            CloneUpperLeftIfNecessary();
        }
    }
}

void ScMatrixFormulaCellToken::SetUpperLeftDouble( double f )
{
    switch (GetUpperLeftType())
    {
        case svDouble:
            const_cast<FormulaToken*>(xUpperLeft.get())->SetDouble(f);
            break;
        case svString:
            xUpperLeft = new FormulaDoubleToken( f);
            break;
        case svUnknown:
            if (!xUpperLeft)
            {
                xUpperLeft = new FormulaDoubleToken( f);
                break;
            }
            [[fallthrough]];
        default:
            {
                OSL_FAIL("ScMatrixFormulaCellToken::SetUpperLeftDouble: not modifying unhandled token type");
            }
    }
}

void ScMatrixFormulaCellToken::ResetResult()
{
    xMatrix = nullptr;
    xUpperLeft = nullptr;
}

ScHybridCellToken::ScHybridCellToken(
    double f, const svl::SharedString & rStr, OUString aFormula, bool bEmptyDisplayedAsString ) :
        FormulaToken( formula::svHybridCell ),
        mfDouble( f ), maString( rStr ),
        maFormula(std::move( aFormula )),
        mbEmptyDisplayedAsString( bEmptyDisplayedAsString)
{
    // caller, make up your mind...
    assert( !bEmptyDisplayedAsString || (f == 0.0 && rStr.getString().isEmpty()));
}

double ScHybridCellToken::GetDouble() const { return mfDouble; }

const svl::SharedString & ScHybridCellToken::GetString() const
{
    return maString;
}

bool ScHybridCellToken::operator==( const FormulaToken& r ) const
{
    return FormulaToken::operator==( r ) &&
        mfDouble == r.GetDouble() && maString == r.GetString() &&
        maFormula == static_cast<const ScHybridCellToken &>(r).GetFormula();
}

bool ScTokenArray::AddFormulaToken(
    const css::sheet::FormulaToken& rToken, svl::SharedStringPool& rSPool, formula::ExternalReferenceHelper* pExtRef)
{
    bool bError = FormulaTokenArray::AddFormulaToken(rToken, rSPool, pExtRef);
    if ( bError )
    {
        bError = false;
        const OpCode eOpCode = static_cast<OpCode>(rToken.OpCode);      // assuming equal values for the moment

        const uno::TypeClass eClass = rToken.Data.getValueTypeClass();
        switch ( eClass )
        {
            case uno::TypeClass_STRUCT:
                {
                    uno::Type aType = rToken.Data.getValueType();
                    if ( aType.equals( cppu::UnoType<sheet::SingleReference>::get() ) )
                    {
                        ScSingleRefData aSingleRef;
                        sheet::SingleReference aApiRef;
                        rToken.Data >>= aApiRef;
                        lcl_SingleRefToCalc( aSingleRef, aApiRef );
                        if ( eOpCode == ocPush )
                            AddSingleReference( aSingleRef );
                        else if ( eOpCode == ocColRowName )
                            AddColRowName( aSingleRef );
                        else
                            bError = true;
                    }
                    else if ( aType.equals( cppu::UnoType<sheet::ComplexReference>::get() ) )
                    {
                        ScComplexRefData aComplRef;
                        sheet::ComplexReference aApiRef;
                        rToken.Data >>= aApiRef;
                        lcl_SingleRefToCalc( aComplRef.Ref1, aApiRef.Reference1 );
                        lcl_SingleRefToCalc( aComplRef.Ref2, aApiRef.Reference2 );

                        if ( eOpCode == ocPush )
                            AddDoubleReference( aComplRef );
                        else
                            bError = true;
                    }
                    else if ( aType.equals( cppu::UnoType<sheet::NameToken>::get() ) )
                    {
                        sheet::NameToken aTokenData;
                        rToken.Data >>= aTokenData;
                        if ( eOpCode == ocName )
                        {
                            SAL_WARN_IF( aTokenData.Sheet < -1 || std::numeric_limits<sal_Int16>::max() < aTokenData.Sheet,
                                    "sc.core",
                                    "ScTokenArray::AddFormulaToken - NameToken.Sheet out of limits: " << aTokenData.Sheet);
                            sal_Int16 nSheet = static_cast<sal_Int16>(aTokenData.Sheet);
                            AddRangeName(aTokenData.Index, nSheet);
                        }
                        else if (eOpCode == ocDBArea)
                            AddDBRange(aTokenData.Index);
                        else
                            bError = true;
                    }
                    else if ( aType.equals( cppu::UnoType<sheet::TableRefToken>::get() ) )
                    {
                        if (eOpCode == ocTableRef)
                        {
                            sheet::TableRefToken aTokenData;
                            rToken.Data >>= aTokenData;
                            ScTableRefToken* pToken = new ScTableRefToken( aTokenData.Index,
                                    static_cast<ScTableRefToken::Item>(aTokenData.Item));
                            if (Add(pToken))    // else pToken is deleted
                            {
                                if (aTokenData.Reference.Reference1 == aTokenData.Reference.Reference2)
                                {
                                    ScSingleRefData aRefData;
                                    lcl_SingleRefToCalc( aRefData, aTokenData.Reference.Reference1 );
                                    pToken->SetAreaRefRPN( new ScSingleRefToken( *mxSheetLimits, aRefData));
                                }
                                else
                                {
                                    ScComplexRefData aRefData;
                                    lcl_SingleRefToCalc( aRefData.Ref1, aTokenData.Reference.Reference1 );
                                    lcl_SingleRefToCalc( aRefData.Ref2, aTokenData.Reference.Reference2 );
                                    pToken->SetAreaRefRPN( new ScDoubleRefToken( *mxSheetLimits, aRefData));
                                }
                            }
                            else
                                bError = true;
                        }
                        else
                            bError = true;
                    }
                    else if ( aType.equals( cppu::UnoType<sheet::ExternalReference>::get() ) )
                    {
                        sheet::ExternalReference aApiExtRef;
                        if( (eOpCode == ocPush) && (rToken.Data >>= aApiExtRef) && (0 <= aApiExtRef.Index) && (aApiExtRef.Index <= SAL_MAX_UINT16) )
                        {
                            sal_uInt16 nFileId = static_cast< sal_uInt16 >( aApiExtRef.Index );
                            sheet::SingleReference aApiSRef;
                            sheet::ComplexReference aApiCRef;
                            OUString aName;
                            if( aApiExtRef.Reference >>= aApiSRef )
                            {
                                // try to resolve cache index to sheet name
                                size_t nCacheId = static_cast< size_t >( aApiSRef.Sheet );
                                OUString aTabName = pExtRef->getCacheTableName( nFileId, nCacheId );
                                if( !aTabName.isEmpty() )
                                {
                                    ScSingleRefData aSingleRef;
                                    // convert column/row settings, set sheet index to absolute
                                    lcl_ExternalRefToCalc( aSingleRef, aApiSRef );
                                    AddExternalSingleReference( nFileId, rSPool.intern( aTabName), aSingleRef );
                                }
                                else
                                    bError = true;
                            }
                            else if( aApiExtRef.Reference >>= aApiCRef )
                            {
                                // try to resolve cache index to sheet name.
                                size_t nCacheId = static_cast< size_t >( aApiCRef.Reference1.Sheet );
                                OUString aTabName = pExtRef->getCacheTableName( nFileId, nCacheId );
                                if( !aTabName.isEmpty() )
                                {
                                    ScComplexRefData aComplRef;
                                    // convert column/row settings, set sheet index to absolute
                                    lcl_ExternalRefToCalc( aComplRef.Ref1, aApiCRef.Reference1 );
                                    lcl_ExternalRefToCalc( aComplRef.Ref2, aApiCRef.Reference2 );
                                    // NOTE: This assumes that cached sheets are in consecutive order!
                                    aComplRef.Ref2.SetAbsTab(
                                        aComplRef.Ref1.Tab() + static_cast<SCTAB>(aApiCRef.Reference2.Sheet - aApiCRef.Reference1.Sheet));
                                    AddExternalDoubleReference( nFileId, rSPool.intern( aTabName), aComplRef );
                                }
                                else
                                    bError = true;
                            }
                            else if( aApiExtRef.Reference >>= aName )
                            {
                                if( !aName.isEmpty() )
                                    AddExternalName( nFileId, rSPool.intern( aName) );
                                else
                                    bError = true;
                            }
                            else
                                bError = true;
                        }
                        else
                            bError = true;
                    }
                    else
                        bError = true;      // unknown struct
                }
                break;
            case uno::TypeClass_SEQUENCE:
                {
                    if ( eOpCode != ocPush )
                        bError = true;      // not an inline array
                    else if (!rToken.Data.getValueType().equals( cppu::UnoType<
                                uno::Sequence< uno::Sequence< uno::Any >>>::get()))
                        bError = true;      // unexpected sequence type
                    else
                    {
                        ScMatrixRef xMat = ScSequenceToMatrix::CreateMixedMatrix( rToken.Data);
                        if (xMat)
                            AddMatrix( xMat);
                        else
                            bError = true;
                    }
                }
                break;
            default:
                bError = true;
        }
    }
    return bError;
}

void ScTokenArray::CheckForThreading( const FormulaToken& r )
{
#if HAVE_CPP_CONSTINIT_SORTED_VECTOR
    constinit
#endif
    static const o3tl::sorted_vector<OpCode> aThreadedCalcDenyList({
        ocIndirect,
        ocMacro,
        ocOffset,
        ocTableOp,
        ocCell,
        ocMatch,
        ocInfo,
        ocStyle,
        ocDBAverage,
        ocDBCount,
        ocDBCount2,
        ocDBGet,
        ocDBMax,
        ocDBMin,
        ocDBProduct,
        ocDBStdDev,
        ocDBStdDevP,
        ocDBSum,
        ocDBVar,
        ocDBVarP,
        ocText,
        ocSheet,
        ocExternal,
        ocDde,
        ocWebservice,
        ocGetPivotData
    });

    // Don't enable threading once we decided to disable it.
    if (!mbThreadingEnabled)
        return;

    static const bool bThreadingProhibited = std::getenv("SC_NO_THREADED_CALCULATION");

    if (bThreadingProhibited)
    {
        mbThreadingEnabled = false;
        return;
    }

    OpCode eOp = r.GetOpCode();

    if (aThreadedCalcDenyList.find(eOp) != aThreadedCalcDenyList.end())
    {
        SAL_INFO("sc.core.formulagroup", "opcode " << formula::FormulaCompiler().GetOpCodeMap(sheet::FormulaLanguage::ENGLISH)->getSymbol(eOp)
            << "(" << int(eOp) << ") disables threaded calculation of formula group");
        mbThreadingEnabled = false;
        return;
    }

    if (eOp != ocPush)
        return;

    switch (r.GetType())
    {
        case svExternalDoubleRef:
        case svExternalSingleRef:
        case svExternalName:
        case svMatrix:
            SAL_INFO("sc.core.formulagroup", "opcode ocPush: variable type " << StackVarEnumToString(r.GetType())
                << " disables threaded calculation of formula group");
            mbThreadingEnabled = false;
            return;
        default:
            break;
    }
}

void ScTokenArray::CheckToken( const FormulaToken& r )
{
    if (mbThreadingEnabled)
        CheckForThreading(r);

    if (IsFormulaVectorDisabled())
        return; // It's already disabled.  No more checking needed.

    OpCode eOp = r.GetOpCode();

    if (SC_OPCODE_START_FUNCTION <= eOp && eOp < SC_OPCODE_STOP_FUNCTION)
    {
        if (ScInterpreter::GetGlobalConfig().mbOpenCLSubsetOnly &&
            ScInterpreter::GetGlobalConfig().mpOpenCLSubsetOpCodes->find(eOp) == ScInterpreter::GetGlobalConfig().mpOpenCLSubsetOpCodes->end())
        {
            SAL_INFO("sc.opencl", "opcode " << formula::FormulaCompiler().GetOpCodeMap(sheet::FormulaLanguage::ENGLISH)->getSymbol(eOp)
                << "(" << int(eOp) << ") disables vectorisation for formula group");
            meVectorState = FormulaVectorDisabledNotInSubSet;
            mbOpenCLEnabled = false;
            return;
        }

        // We support vectorization for the following opcodes.
        switch (eOp)
        {
            case ocAverage:
            case ocMin:
            case ocMinA:
            case ocMax:
            case ocMaxA:
            case ocSum:
            case ocSumIfs:
            case ocSumProduct:
            case ocCount:
            case ocCount2:
            case ocVLookup:
            case ocXLookup:
            case ocXMatch:
            case ocFilter:
            case ocSort:
            case ocSortBy:
            case ocSLN:
            case ocIRR:
            case ocMIRR:
            case ocPMT:
            case ocRate:
            case ocRRI:
            case ocPpmt:
            case ocFisher:
            case ocFisherInv:
            case ocGamma:
            case ocGammaLn:
            case ocNotAvail:
            case ocGauss:
            case ocGeoMean:
            case ocHarMean:
            case ocSYD:
            case ocCorrel:
            case ocNegBinomVert:
            case ocPearson:
            case ocRSQ:
            case ocCos:
            case ocCosecant:
            case ocCosecantHyp:
            case ocISPMT:
            case ocPDuration:
            case ocSinHyp:
            case ocAbs:
            case ocPV:
            case ocSin:
            case ocTan:
            case ocTanHyp:
            case ocStandard:
            case ocWeibull:
            case ocMedian:
            case ocDDB:
            case ocFV:
            case ocVBD:
            case ocKurt:
            case ocNper:
            case ocNormDist:
            case ocArcCos:
            case ocSqrt:
            case ocArcCosHyp:
            case ocNPV:
            case ocStdNormDist:
            case ocNormInv:
            case ocSNormInv:
            case ocPermut:
            case ocPermutationA:
            case ocPhi:
            case ocIpmt:
            case ocConfidence:
            case ocIntercept:
            case ocDB:
            case ocLogInv:
            case ocArcCot:
            case ocCosHyp:
            case ocCritBinom:
            case ocArcCotHyp:
            case ocArcSin:
            case ocArcSinHyp:
            case ocArcTan:
            case ocArcTanHyp:
            case ocBitAnd:
            case ocForecast:
            case ocLogNormDist:
            case ocGammaDist:
            case ocLn:
            case ocRound:
            case ocCot:
            case ocCotHyp:
            case ocFDist:
            case ocVar:
            case ocChiDist:
            case ocPower:
            case ocOdd:
            case ocChiSqDist:
            case ocChiSqInv:
            case ocGammaInv:
            case ocFloor:
            case ocFInv:
            case ocFTest:
            case ocB:
            case ocBetaDist:
            case ocExp:
            case ocLog10:
            case ocExpDist:
            case ocAverageIfs:
            case ocCountIfs:
            case ocCombinA:
            case ocEven:
            case ocLog:
            case ocMod:
            case ocTrunc:
            case ocSkew:
            case ocArcTan2:
            case ocBitOr:
            case ocBitLshift:
            case ocBitRshift:
            case ocBitXor:
            case ocChiInv:
            case ocPoissonDist:
            case ocSumSQ:
            case ocSkewp:
            case ocBinomDist:
            case ocVarP:
            case ocCeil:
            case ocCombin:
            case ocDevSq:
            case ocStDev:
            case ocSlope:
            case ocSTEYX:
            case ocZTest:
            case ocPi:
            case ocRandom:
            case ocProduct:
            case ocHypGeomDist:
            case ocSumX2MY2:
            case ocSumX2DY2:
            case ocBetaInv:
            case ocTTest:
            case ocTDist:
            case ocTInv:
            case ocSumXMY2:
            case ocStDevP:
            case ocCovar:
            case ocAnd:
            case ocOr:
            case ocNot:
            case ocXor:
            case ocDBMax:
            case ocDBMin:
            case ocDBProduct:
            case ocDBAverage:
            case ocDBStdDev:
            case ocDBStdDevP:
            case ocDBSum:
            case ocDBVar:
            case ocDBVarP:
            case ocAverageIf:
            case ocDBCount:
            case ocDBCount2:
            case ocDeg:
            case ocRoundUp:
            case ocRoundDown:
            case ocInt:
            case ocRad:
            case ocCountIf:
            case ocIsEven:
            case ocIsOdd:
            case ocFact:
            case ocAverageA:
            case ocVarA:
            case ocVarPA:
            case ocStDevA:
            case ocStDevPA:
            case ocSecant:
            case ocSecantHyp:
            case ocSumIf:
            case ocNegSub:
            case ocAveDev:
            case ocMatSequence:
            case ocRandArray:
            case ocChooseCols:
            case ocChooseRows:
            case ocDrop:
            case ocExpand:
            case ocHStack:
            case ocVStack:
            case ocTake:
            case ocToCol:
            case ocToRow:
            case ocUnique:
            case ocWrapCols:
            case ocWrapRows:
            // Don't change the state.
            break;
            default:
                SAL_INFO("sc.opencl", "opcode " << formula::FormulaCompiler().GetOpCodeMap(sheet::FormulaLanguage::ENGLISH)->getSymbol(eOp)
                    << "(" << int(eOp) << ") disables vectorisation for formula group");
                meVectorState = FormulaVectorDisabledByOpCode;
                mbOpenCLEnabled = false;
                return;
        }
    }
    else if (eOp == ocPush)
    {
        // This is a stack variable.  See if this is a reference.

        switch (r.GetType())
        {
            case svByte:
            case svDouble:
            case svString:
                // Don't change the state.
            break;
            case svSingleRef:
            case svDoubleRef:
                // Depends on the reference state.
                meVectorState = FormulaVectorCheckReference;
            break;
            case svError:
            case svEmptyCell:
            case svExternal:
            case svExternalDoubleRef:
            case svExternalName:
            case svExternalSingleRef:
            case svFAP:
            case svHybridCell:
            case svIndex:
            case svJump:
            case svJumpMatrix:
            case svMatrix:
            case svMatrixCell:
            case svMissing:
            case svRefList:
            case svSep:
            case svUnknown:
                // We don't support vectorization on these.
                SAL_INFO("sc.opencl", "opcode ocPush: variable type " << StackVarEnumToString(r.GetType()) << " disables vectorisation for formula group");
                meVectorState = FormulaVectorDisabledByStackVariable;
                mbOpenCLEnabled = false;
                return;
            default:
                ;
        }
    }
    else if (SC_OPCODE_START_BIN_OP <= eOp && eOp < SC_OPCODE_STOP_UN_OP)
    {
        if (ScInterpreter::GetGlobalConfig().mbOpenCLSubsetOnly &&
            ScInterpreter::GetGlobalConfig().mpOpenCLSubsetOpCodes->find(eOp) == ScInterpreter::GetGlobalConfig().mpOpenCLSubsetOpCodes->end())
        {
            SAL_INFO("sc.opencl", "opcode " << formula::FormulaCompiler().GetOpCodeMap(sheet::FormulaLanguage::ENGLISH)->getSymbol(eOp)
                << "(" << int(eOp) << ") disables vectorisation for formula group");
            meVectorState = FormulaVectorDisabledNotInSubSet;
            mbOpenCLEnabled = false;
            return;
        }
    }
    else
    {
        // All the rest, special commands, separators, error codes, ...
        switch (eOp)
        {
            default:
                // Default is off, no vectorization.
                // Mentioning some specific values below to indicate why.

            case ocName:
                // Named expression would need "recursive" handling of its
                // token array for vector state in
                // ScFormulaCell::InterpretFormulaGroup() and below.

            case ocDBArea:
                // Certainly not a vectorization of the entire area...

            case ocTableRef:
                // May result in a single cell or range reference, depending on
                // context.

            case ocColRowName:
                // The associated reference is the name cell with which to
                // create the implicit intersection.

            case ocColRowNameAuto:
                // Auto column/row names lead to references computed in
                // interpreter.

                SAL_INFO("sc.opencl", "opcode " << formula::FormulaCompiler().GetOpCodeMap(sheet::FormulaLanguage::ENGLISH)->getSymbol(eOp)
                    << "(" << int(eOp) << ") disables vectorisation for formula group");
                meVectorState = FormulaVectorDisabledByOpCode;
                mbOpenCLEnabled = false;
                return;

            // Known good, don't change state.
            case ocStop:
            case ocExternal:
            case ocOpen:
            case ocClose:
            case ocSep:
            case ocArrayOpen:
            case ocArrayRowSep:
            case ocArrayColSep:
            case ocArrayClose:
            case ocMissing:
            case ocBad:
            case ocSpaces:
            case ocWhitespace:
            case ocSkip:
            case ocPercentSign:
            case ocErrNull:
            case ocErrDivZero:
            case ocErrValue:
            case ocErrRef:
            case ocErrName:
            case ocErrNum:
            case ocErrNA:
            break;
            case ocIf:
            case ocIfError:
            case ocIfNA:
            case ocChoose:
            case ocLet:
                // Jump commands are now supported.
            break;
        }
    }
}

bool ScTokenArray::ImplGetReference( ScRange& rRange, const ScAddress& rPos, bool bValidOnly ) const
{
    bool bIs = false;
    if ( pCode && nLen == 1 )
    {
        const FormulaToken* pToken = pCode[0];
        if ( pToken )
        {
            if ( pToken->GetType() == svSingleRef )
            {
                const ScSingleRefData& rRef = *static_cast<const ScSingleRefToken*>(pToken)->GetSingleRef();
                rRange.aStart = rRange.aEnd = rRef.toAbs(*mxSheetLimits, rPos);
                bIs = !bValidOnly || mxSheetLimits->ValidAddress(rRange.aStart);
            }
            else if ( pToken->GetType() == svDoubleRef )
            {
                const ScComplexRefData& rCompl = *static_cast<const ScDoubleRefToken*>(pToken)->GetDoubleRef();
                const ScSingleRefData& rRef1 = rCompl.Ref1;
                const ScSingleRefData& rRef2 = rCompl.Ref2;
                rRange.aStart = rRef1.toAbs(*mxSheetLimits, rPos);
                rRange.aEnd   = rRef2.toAbs(*mxSheetLimits, rPos);
                bIs = !bValidOnly || mxSheetLimits->ValidRange(rRange);
            }
        }
    }
    return bIs;
}

namespace {

// we want to compare for similar not identical formulae
// so we can't use actual row & column indices.
size_t HashSingleRef( const ScSingleRefData& rRef )
{
    size_t nVal = 0;

    nVal += size_t(rRef.IsColRel());
    nVal += (size_t(rRef.IsRowRel()) << 1);
    nVal += (size_t(rRef.IsTabRel()) << 2);

    return nVal;
}

}

void ScTokenArray::GenHash()
{
    static const OUStringHash aHasher;

    size_t nHash = 1;
    OpCode eOp;
    StackVar eType;
    const formula::FormulaToken* p;
    sal_uInt16 n = std::min<sal_uInt16>(nLen, 20);
    for (sal_uInt16 i = 0; i < n; ++i)
    {
        p = pCode[i];
        eOp = p->GetOpCode();
        if (eOp == ocPush)
        {
            // This is stack variable. Do additional differentiation.
            eType = p->GetType();
            switch (eType)
            {
                case svByte:
                {
                    // Constant value.
                    sal_uInt8 nVal = p->GetByte();
                    nHash += static_cast<size_t>(nVal);
                }
                break;
                case svDouble:
                {
                    // Constant value.
                    double fVal = p->GetDouble();
                    nHash += std::hash<double>()(fVal);
                }
                break;
                case svString:
                {
                    // Constant string.
                    OUString aStr = p->GetString().getString();
                    nHash += aHasher(aStr);
                }
                break;
                case svSingleRef:
                {
                    size_t nVal = HashSingleRef(*p->GetSingleRef());
                    nHash += nVal;
                }
                break;
                case svDoubleRef:
                {
                    const ScComplexRefData& rRef = *p->GetDoubleRef();
                    size_t nVal1 = HashSingleRef(rRef.Ref1);
                    size_t nVal2 = HashSingleRef(rRef.Ref2);
                    nHash += nVal1;
                    nHash += nVal2;
                }
                break;
                default:
                    // Use the opcode value in all the other cases.
                    nHash += static_cast<size_t>(eOp);
            }
        }
        else
            // Use the opcode value in all the other cases.
            nHash += static_cast<size_t>(eOp);

        nHash = (nHash << 4) - nHash;
    }

    mnHashValue = nHash;
}

void ScTokenArray::ResetVectorState()
{
    mbOpenCLEnabled = ScCalcConfig::isOpenCLEnabled();
    meVectorState = mbOpenCLEnabled ? FormulaVectorEnabled : FormulaVectorDisabled;
    mbThreadingEnabled = ScCalcConfig::isThreadingEnabled();
}

bool ScTokenArray::IsFormulaVectorDisabled() const
{
    switch (meVectorState)
    {
        case FormulaVectorDisabled:
        case FormulaVectorDisabledByOpCode:
        case FormulaVectorDisabledByStackVariable:
        case FormulaVectorDisabledNotInSubSet:
            return true;
        default:
            ;
    }

    return false;
}

bool ScTokenArray::IsInvariant() const
{
    FormulaToken** p = pCode.get();
    FormulaToken** pEnd = p + static_cast<size_t>(nLen);
    for (; p != pEnd; ++p)
    {
        switch ((*p)->GetType())
        {
            case svSingleRef:
            case svExternalSingleRef:
            {
                const ScSingleRefData& rRef = *(*p)->GetSingleRef();
                if (rRef.IsRowRel())
                    return false;
            }
            break;
            case svDoubleRef:
            case svExternalDoubleRef:
            {
                const ScComplexRefData& rRef = *(*p)->GetDoubleRef();
                if (rRef.Ref1.IsRowRel() || rRef.Ref2.IsRowRel())
                    return false;
            }
            break;
            case svIndex:
                return false;
            default:
                ;
        }
    }

    return true;
}

bool ScTokenArray::IsReference( ScRange& rRange, const ScAddress& rPos ) const
{
    return ImplGetReference(rRange, rPos, false);
}

bool ScTokenArray::IsValidReference( ScRange& rRange, const ScAddress& rPos ) const
{
    return ImplGetReference(rRange, rPos, true);
}

ScTokenArray::ScTokenArray(const ScDocument& rDoc) :
    mxSheetLimits(&rDoc.GetSheetLimits()),
    mnHashValue(0)
{
    ResetVectorState();
}

ScTokenArray::ScTokenArray(ScSheetLimits& rLimits) :
    mxSheetLimits(&rLimits),
    mnHashValue(0)
{
    ResetVectorState();
}

ScTokenArray::~ScTokenArray()
{
}

ScTokenArray& ScTokenArray::operator=( const ScTokenArray& rArr )
{
    Clear();
    Assign( rArr );
    mnHashValue = rArr.mnHashValue;
    meVectorState = rArr.meVectorState;
    mbOpenCLEnabled = rArr.mbOpenCLEnabled;
    mbThreadingEnabled = rArr.mbThreadingEnabled;
    return *this;
}

ScTokenArray& ScTokenArray::operator=( ScTokenArray&& rArr )
{
    mxSheetLimits = std::move(rArr.mxSheetLimits);
    mnHashValue = rArr.mnHashValue;
    meVectorState = rArr.meVectorState;
    mbOpenCLEnabled = rArr.mbOpenCLEnabled;
    mbThreadingEnabled = rArr.mbThreadingEnabled;
    Move(std::move(rArr));
    return *this;
}

bool ScTokenArray::EqualTokens( const ScTokenArray* pArr2) const
{
    // We only compare the non-RPN array
    if ( pArr2->nLen != nLen )
        return false;

    FormulaToken** ppToken1 = GetArray();
    FormulaToken** ppToken2 = pArr2->GetArray();
    for (sal_uInt16 i=0; i<nLen; i++)
    {
        if ( ppToken1[i] != ppToken2[i] &&
             !(*ppToken1[i] == *ppToken2[i]) )
            return false; // Difference
    }
    return true; // All entries are the same
}

void ScTokenArray::Clear()
{
    mnHashValue = 0;
    ResetVectorState();
    FormulaTokenArray::Clear();
}

std::unique_ptr<ScTokenArray> ScTokenArray::Clone() const
{
    std::unique_ptr<ScTokenArray> p(new ScTokenArray(*mxSheetLimits));
    p->nLen = nLen;
    p->nRPN = nRPN;
    p->nMode = nMode;
    p->nError = nError;
    p->bHyperLink = bHyperLink;
    p->mnHashValue = mnHashValue;
    p->meVectorState = meVectorState;
    p->mbOpenCLEnabled = mbOpenCLEnabled;
    p->mbThreadingEnabled = mbThreadingEnabled;
    p->mbFromRangeName = mbFromRangeName;
    p->mbShareable = mbShareable;

    FormulaToken** pp;
    if( nLen )
    {
        p->pCode.reset(new FormulaToken*[ nLen ]);
        pp = p->pCode.get();
        memcpy( pp, pCode.get(), nLen * sizeof( formula::FormulaToken* ) );
        for( sal_uInt16 i = 0; i < nLen; i++, pp++ )
        {
            *pp = (*pp)->Clone();
            (*pp)->IncRef();
        }
    }
    if( nRPN )
    {
        pp = p->pRPN = new FormulaToken*[ nRPN ];
        memcpy( pp, pRPN, nRPN * sizeof( formula::FormulaToken* ) );
        for( sal_uInt16 i = 0; i < nRPN; i++, pp++ )
        {
            FormulaToken* t = *pp;
            if( t->GetRef() > 1 )
            {
                FormulaToken** p2 = pCode.get();
                sal_uInt16 nIdx = 0xFFFF;
                for( sal_uInt16 j = 0; j < nLen; j++, p2++ )
                {
                    if( *p2 == t )
                    {
                        nIdx = j; break;
                    }
                }
                if( nIdx == 0xFFFF )
                    *pp = t->Clone();
                else
                    *pp = p->pCode[ nIdx ];
            }
            else
                *pp = t->Clone();
            (*pp)->IncRef();
        }
    }
    return p;
}

ScTokenArray ScTokenArray::CloneValue() const
{
    ScTokenArray aNew(*mxSheetLimits);
    aNew.nLen = nLen;
    aNew.nRPN = nRPN;
    aNew.nMode = nMode;
    aNew.nError = nError;
    aNew.bHyperLink = bHyperLink;
    aNew.mnHashValue = mnHashValue;
    aNew.meVectorState = meVectorState;
    aNew.mbOpenCLEnabled = mbOpenCLEnabled;
    aNew.mbThreadingEnabled = mbThreadingEnabled;
    aNew.mbFromRangeName = mbFromRangeName;
    aNew.mbShareable = mbShareable;

    FormulaToken** pp;
    if( nLen )
    {
        aNew.pCode.reset(new FormulaToken*[ nLen ]);
        pp = aNew.pCode.get();
        memcpy( pp, pCode.get(), nLen * sizeof( formula::FormulaToken* ) );
        for( sal_uInt16 i = 0; i < nLen; i++, pp++ )
        {
            *pp = (*pp)->Clone();
            (*pp)->IncRef();
        }
    }
    if( nRPN )
    {
        pp = aNew.pRPN = new FormulaToken*[ nRPN ];
        memcpy( pp, pRPN, nRPN * sizeof( formula::FormulaToken* ) );
        for( sal_uInt16 i = 0; i < nRPN; i++, pp++ )
        {
            FormulaToken* t = *pp;
            if( t->GetRef() > 1 )
            {
                FormulaToken** p2 = pCode.get();
                sal_uInt16 nIdx = 0xFFFF;
                for( sal_uInt16 j = 0; j < nLen; j++, p2++ )
                {
                    if( *p2 == t )
                    {
                        nIdx = j; break;
                    }
                }
                if( nIdx == 0xFFFF )
                    *pp = t->Clone();
                else
                    *pp = aNew.pCode[ nIdx ];
            }
            else
                *pp = t->Clone();
            (*pp)->IncRef();
        }
    }
    return aNew;
}

FormulaToken* ScTokenArray::AddRawToken( const ScRawToken& r )
{
    return Add( r.CreateToken(*mxSheetLimits) );
}

// Utility function to ensure that there is strict alternation of values and
// separators.
static bool
checkArraySep( bool & bPrevWasSep, bool bNewVal )
{
    bool bResult = (bPrevWasSep == bNewVal);
    bPrevWasSep = bNewVal;
    return bResult;
}

FormulaToken* ScTokenArray::MergeArray( )
{
    int nCol = -1, nRow = 0;
    int i, nPrevRowSep = -1, nStart = 0;
    bool bPrevWasSep = false; // top of stack is ocArrayClose
    FormulaToken* t;
    bool bNumeric = false;  // numeric value encountered in current element

    // (1) Iterate from the end to the start to find matrix dims
    // and do basic validation.
    for ( i = nLen ; i-- > nStart ; )
    {
        t = pCode[i];
        switch ( t->GetOpCode() )
        {
            case ocPush :
                if( checkArraySep( bPrevWasSep, false ) )
                {
                    return nullptr;
                }

                // no references or nested arrays
                if ( t->GetType() != svDouble  && t->GetType() != svString )
                {
                    return nullptr;
                }
                bNumeric = (t->GetType() == svDouble);
            break;

            case ocMissing :
            case ocTrue :
            case ocFalse :
                if( checkArraySep( bPrevWasSep, false ) )
                {
                    return nullptr;
                }
                bNumeric = false;
            break;

            case ocArrayColSep :
            case ocSep :
                if( checkArraySep( bPrevWasSep, true ) )
                {
                    return nullptr;
                }
                bNumeric = false;
            break;

            case ocArrayClose :
                // not possible with the , but check just in case
                // something changes in the future
                if( i != (nLen-1))
                {
                    return nullptr;
                }

                if( checkArraySep( bPrevWasSep, true ) )
                {
                    return nullptr;
                }

                nPrevRowSep = i;
                bNumeric = false;
            break;

            case ocArrayOpen :
                nStart = i; // stop iteration
                [[fallthrough]]; // to ArrayRowSep

            case ocArrayRowSep :
                if( checkArraySep( bPrevWasSep, true ) )
                {
                    return nullptr;
                }

                if( nPrevRowSep < 0 ||              // missing ocArrayClose
                    ((nPrevRowSep - i) % 2) == 1)   // no complex elements
                {
                    return nullptr;
                }

                if( nCol < 0 )
                {
                    nCol = (nPrevRowSep - i) / 2;
                }
                else if( (nPrevRowSep - i)/2 != nCol)   // irregular array
                {
                    return nullptr;
                }

                nPrevRowSep = i;
                nRow++;
                bNumeric = false;
            break;

            case ocNegSub :
            case ocAdd :
                // negation or unary plus must precede numeric value
                if( !bNumeric )
                {
                    return nullptr;
                }
                --nPrevRowSep;      // shorten this row by 1
                bNumeric = false;   // one level only, no --42
            break;

            case ocSpaces :
            case ocWhitespace :
                // ignore spaces
                --nPrevRowSep;      // shorten this row by 1
            break;

            default :
                // no functions or operators
                return nullptr;
        }
    }
    if( nCol <= 0 || nRow <= 0 )
        return nullptr;

    int nSign = 1;
    ScMatrix* pArray = new ScMatrix(nCol, nRow, 0.0);
    for ( i = nStart, nCol = 0, nRow = 0 ; i < nLen ; i++ )
    {
        t = pCode[i];

        switch ( t->GetOpCode() )
        {
            case ocPush :
                if ( t->GetType() == svDouble )
                {
                    pArray->PutDouble( t->GetDouble() * nSign, nCol, nRow );
                    nSign = 1;
                }
                else if ( t->GetType() == svString )
                {
                    pArray->PutString(t->GetString(), nCol, nRow);
                }
            break;

            case ocMissing :
                pArray->PutEmpty( nCol, nRow );
            break;

            case ocTrue :
                pArray->PutBoolean( true, nCol, nRow );
            break;

            case ocFalse :
                pArray->PutBoolean( false, nCol, nRow );
            break;

            case ocArrayColSep :
            case ocSep :
                nCol++;
            break;

            case ocArrayRowSep :
                nRow++; nCol = 0;
            break;

            case ocNegSub :
                nSign = -nSign;
            break;

            default :
                break;
        }
        pCode[i] = nullptr;
        t->DecRef();
    }
    nLen = sal_uInt16( nStart );
    return AddMatrix( pArray );
}

void ScTokenArray::MergeRangeReference( const ScAddress & rPos )
{
    if (!pCode || !nLen)
        return;
    sal_uInt16 nIdx = nLen;

    // The actual types are checked in extendRangeReference().
    FormulaToken *p3 = PeekPrev(nIdx); // ref
    if (!p3)
        return;
    FormulaToken *p2 = PeekPrev(nIdx); // ocRange
    if (!p2 || p2->GetOpCode() != ocRange)
        return;
    FormulaToken *p1 = PeekPrev(nIdx); // ref
    if (!p1)
        return;
    FormulaTokenRef p = extendRangeReference( *mxSheetLimits, *p1, *p3, rPos, true);
    if (p)
    {
        p->IncRef();
        p1->DecRef();
        p2->DecRef();
        p3->DecRef();
        nLen -= 2;
        pCode[ nLen-1 ] = p.get();
    }
}

FormulaToken* ScTokenArray::AddOpCode( OpCode e )
{
    ScRawToken t;
    t.SetOpCode( e );
    return AddRawToken( t );
}

FormulaToken* ScTokenArray::AddSingleReference( const ScSingleRefData& rRef )
{
    return Add( new ScSingleRefToken( *mxSheetLimits, rRef ) );
}

FormulaToken* ScTokenArray::AddMatrixSingleReference( const ScSingleRefData& rRef )
{
    return Add( new ScSingleRefToken(*mxSheetLimits, rRef, ocMatRef ) );
}

FormulaToken* ScTokenArray::AddDoubleReference( const ScComplexRefData& rRef )
{
    return Add( new ScDoubleRefToken(*mxSheetLimits, rRef ) );
}

FormulaToken* ScTokenArray::AddMatrix( const ScMatrixRef& p )
{
    return Add( new ScMatrixToken( p ) );
}

void ScTokenArray::AddRangeName( sal_uInt16 n, sal_Int16 nSheet )
{
    Add( new FormulaIndexToken( ocName, n, nSheet));
}

FormulaToken* ScTokenArray::AddDBRange( sal_uInt16 n )
{
    return Add( new FormulaIndexToken( ocDBArea, n));
}

FormulaToken* ScTokenArray::AddExternalName( sal_uInt16 nFileId, const svl::SharedString& rName )
{
    return Add( new ScExternalNameToken(nFileId, rName) );
}

void ScTokenArray::AddExternalSingleReference( sal_uInt16 nFileId, const svl::SharedString& rTabName,
        const ScSingleRefData& rRef )
{
    Add( new ScExternalSingleRefToken(nFileId, rTabName, rRef) );
}

FormulaToken* ScTokenArray::AddExternalDoubleReference( sal_uInt16 nFileId, const svl::SharedString& rTabName,
        const ScComplexRefData& rRef )
{
    return Add( new ScExternalDoubleRefToken(nFileId, rTabName, rRef) );
}

FormulaToken* ScTokenArray::AddColRowName( const ScSingleRefData& rRef )
{
    return Add( new ScSingleRefToken(*mxSheetLimits, rRef, ocColRowName ) );
}

void ScTokenArray::AssignXMLString( const OUString &rText, const OUString &rFormulaNmsp )
{
    sal_uInt16 nTokens = 1;
    FormulaToken *aTokens[2];

    aTokens[0] = new FormulaStringOpToken( ocStringXML, svl::SharedString( rText) );    // string not interned
    if( !rFormulaNmsp.isEmpty() )
        aTokens[ nTokens++ ] = new FormulaStringOpToken( ocStringXML,
                svl::SharedString( rFormulaNmsp) );   // string not interned

    Assign( nTokens, aTokens );
}

bool ScTokenArray::GetAdjacentExtendOfOuterFuncRefs( SCCOLROW& nExtend,
        const ScAddress& rPos, ScDirection eDir )
{
    SCCOL nCol = 0;
    SCROW nRow = 0;
    switch ( eDir )
    {
        case DIR_BOTTOM :
            if ( rPos.Row() >= mxSheetLimits->mnMaxRow )
                return false;
            nExtend = rPos.Row();
            nRow = nExtend + 1;
        break;
        case DIR_RIGHT :
            if ( rPos.Col() >= mxSheetLimits->mnMaxCol )
                return false;
            nExtend = rPos.Col();
            nCol = static_cast<SCCOL>(nExtend) + 1;
        break;
        case DIR_TOP :
            if ( rPos.Row() <= 0 )
                return false;
            nExtend = rPos.Row();
            nRow = nExtend - 1;
        break;
        case DIR_LEFT :
            if ( rPos.Col() <= 0 )
                return false;
            nExtend = rPos.Col();
            nCol = static_cast<SCCOL>(nExtend) - 1;
        break;
        default:
            OSL_FAIL( "unknown Direction" );
            return false;
    }
    if ( pRPN && nRPN )
    {
        FormulaToken* t = pRPN[nRPN-1];
        if ( t->GetType() == svByte )
        {
            sal_uInt8 nParamCount = t->GetByte();
            if ( nParamCount && nRPN > nParamCount )
            {
                bool bRet = false;
                sal_uInt16 nParam = nRPN - nParamCount - 1;
                for ( ; nParam < nRPN-1; nParam++ )
                {
                    FormulaToken* p = pRPN[nParam];
                    switch ( p->GetType() )
                    {
                        case svSingleRef :
                        {
                            ScSingleRefData& rRef = *p->GetSingleRef();
                            ScAddress aAbs = rRef.toAbs(*mxSheetLimits, rPos);
                            switch ( eDir )
                            {
                                case DIR_BOTTOM :
                                    if (aAbs.Row() == nRow && aAbs.Row() > nExtend)
                                    {
                                        nExtend = aAbs.Row();
                                        bRet = true;
                                    }
                                break;
                                case DIR_RIGHT :
                                    if (aAbs.Col() == nCol && static_cast<SCCOLROW>(aAbs.Col()) > nExtend)
                                    {
                                        nExtend = aAbs.Col();
                                        bRet = true;
                                    }
                                break;
                                case DIR_TOP :
                                    if (aAbs.Row() == nRow && aAbs.Row() < nExtend)
                                    {
                                        nExtend = aAbs.Row();
                                        bRet = true;
                                    }
                                break;
                                case DIR_LEFT :
                                    if (aAbs.Col() == nCol && static_cast<SCCOLROW>(aAbs.Col()) < nExtend)
                                    {
                                        nExtend = aAbs.Col();
                                        bRet = true;
                                    }
                                break;
                            }
                        }
                        break;
                        case svDoubleRef :
                        {
                            ScComplexRefData& rRef = *p->GetDoubleRef();
                            ScRange aAbs = rRef.toAbs(*mxSheetLimits, rPos);
                            switch ( eDir )
                            {
                                case DIR_BOTTOM :
                                    if (aAbs.aStart.Row() == nRow && aAbs.aEnd.Row() > nExtend)
                                    {
                                        nExtend = aAbs.aEnd.Row();
                                        bRet = true;
                                    }
                                break;
                                case DIR_RIGHT :
                                    if (aAbs.aStart.Col() == nCol && static_cast<SCCOLROW>(aAbs.aEnd.Col()) > nExtend)
                                    {
                                        nExtend = aAbs.aEnd.Col();
                                        bRet = true;
                                    }
                                break;
                                case DIR_TOP :
                                    if (aAbs.aEnd.Row() == nRow && aAbs.aStart.Row() < nExtend)
                                    {
                                        nExtend = aAbs.aStart.Row();
                                        bRet = true;
                                    }
                                break;
                                case DIR_LEFT :
                                    if (aAbs.aEnd.Col() == nCol && static_cast<SCCOLROW>(aAbs.aStart.Col()) < nExtend)
                                    {
                                        nExtend = aAbs.aStart.Col();
                                        bRet = true;
                                    }
                                break;
                            }
                        }
                        break;
                        default:
                        {
                            // added to avoid warnings
                        }
                    } // switch
                } // for
                return bRet;
            }
        }
    }
    return false;
}

namespace {

void GetExternalTableData(const ScDocument* pOldDoc, const ScDocument* pNewDoc, const SCTAB nTab, OUString& rTabName, sal_uInt16& rFileId)
{
    const OUString& aFileName = pOldDoc->GetFileURL();
    rFileId = pNewDoc->GetExternalRefManager()->getExternalFileId(aFileName);
    rTabName = pOldDoc->GetCopyTabName(nTab);
    if (rTabName.isEmpty())
        pOldDoc->GetName(nTab, rTabName);
}

bool IsInCopyRange( const ScRange& rRange, const ScDocument* pClipDoc )
{
    ScClipParam& rClipParam = const_cast<ScDocument*>(pClipDoc)->GetClipParam();
    return rClipParam.maRanges.Contains(rRange);
}

bool SkipReference(formula::FormulaToken* pToken, const ScAddress& rPos, const ScDocument& rOldDoc, bool bRangeName, bool bCheckCopyArea)
{
    ScRange aRange;

    if (!ScRefTokenHelper::getRangeFromToken(&rOldDoc, aRange, pToken, rPos))
        return true;

    if (bRangeName && aRange.aStart.Tab() == rPos.Tab())
    {
        switch (pToken->GetType())
        {
            case svDoubleRef:
                {
                    ScSingleRefData& rRef = *pToken->GetSingleRef2();
                    if (rRef.IsColRel() || rRef.IsRowRel())
                        return true;
                }
                [[fallthrough]];
            case svSingleRef:
                {
                    ScSingleRefData& rRef = *pToken->GetSingleRef();
                    if (rRef.IsColRel() || rRef.IsRowRel())
                        return true;
                }
                break;
            default:
                break;
        }
    }

    if (bCheckCopyArea && IsInCopyRange(aRange, &rOldDoc))
        return true;

    return false;
}

void AdjustSingleRefData( ScSingleRefData& rRef, const ScAddress& rOldPos, const ScAddress& rNewPos)
{
    SCCOL nCols = rNewPos.Col() - rOldPos.Col();
    SCROW nRows = rNewPos.Row() - rOldPos.Row();
    SCTAB nTabs = rNewPos.Tab() - rOldPos.Tab();

    if (!rRef.IsColRel())
        rRef.IncCol(nCols);

    if (!rRef.IsRowRel())
        rRef.IncRow(nRows);

    if (!rRef.IsTabRel())
        rRef.IncTab(nTabs);
}

}

void ScTokenArray::ReadjustAbsolute3DReferences( const ScDocument& rOldDoc, ScDocument& rNewDoc, const ScAddress& rPos, bool bRangeName )
{
    for ( sal_uInt16 j=0; j<nLen; ++j )
    {
        switch ( pCode[j]->GetType() )
        {
            case svDoubleRef :
            {
                if (SkipReference(pCode[j], rPos, rOldDoc, bRangeName, true))
                    continue;

                ScComplexRefData& rRef = *pCode[j]->GetDoubleRef();
                ScSingleRefData& rRef2 = rRef.Ref2;
                ScSingleRefData& rRef1 = rRef.Ref1;

                if ( (rRef2.IsFlag3D() && !rRef2.IsTabRel()) || (rRef1.IsFlag3D() && !rRef1.IsTabRel()) )
                {
                    OUString aTabName;
                    sal_uInt16 nFileId;
                    GetExternalTableData(&rOldDoc, &rNewDoc, rRef1.Tab(), aTabName, nFileId);
                    ReplaceToken( j, new ScExternalDoubleRefToken( nFileId,
                                rNewDoc.GetSharedStringPool().intern( aTabName), rRef), CODE_AND_RPN);
                    // ATTENTION: rRef can't be used after this point
                }
            }
            break;
            case svSingleRef :
            {
                if (SkipReference(pCode[j], rPos, rOldDoc, bRangeName, true))
                    continue;

                ScSingleRefData& rRef = *pCode[j]->GetSingleRef();

                if ( rRef.IsFlag3D() && !rRef.IsTabRel() )
                {
                    OUString aTabName;
                    sal_uInt16 nFileId;
                    GetExternalTableData(&rOldDoc, &rNewDoc, rRef.Tab(), aTabName, nFileId);
                    ReplaceToken( j, new ScExternalSingleRefToken( nFileId,
                                rNewDoc.GetSharedStringPool().intern( aTabName), rRef), CODE_AND_RPN);
                    // ATTENTION: rRef can't be used after this point
                }
            }
            break;
            default:
            {
                // added to avoid warnings
            }
        }
    }
}

void ScTokenArray::AdjustAbsoluteRefs( const ScDocument& rOldDoc, const ScAddress& rOldPos, const ScAddress& rNewPos,
        bool bCheckCopyRange)
{
    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN, true);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken** pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken** pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch ( p->GetType() )
            {
                case svDoubleRef :
                    {
                        if (!SkipReference(p, rOldPos, rOldDoc, false, bCheckCopyRange))
                            continue;

                        ScComplexRefData& rRef = *p->GetDoubleRef();
                        ScSingleRefData& rRef2 = rRef.Ref2;
                        ScSingleRefData& rRef1 = rRef.Ref1;

                        AdjustSingleRefData( rRef1, rOldPos, rNewPos );
                        AdjustSingleRefData( rRef2, rOldPos, rNewPos );
                    }
                    break;
                case svSingleRef :
                    {
                        if (!SkipReference(p, rOldPos, rOldDoc, false, bCheckCopyRange))
                            continue;

                        ScSingleRefData& rRef = *p->GetSingleRef();

                        AdjustSingleRefData( rRef, rOldPos, rNewPos );
                    }
                    break;
                default:
                    {
                        // added to avoid warnings
                    }
            }
        }
    }
}

void ScTokenArray::AdjustSheetLocalNameReferences( SCTAB nOldTab, SCTAB nNewTab )
{
    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN, false);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken** pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken** pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch ( p->GetType() )
            {
                case svDoubleRef :
                    {
                        ScComplexRefData& rRef = *p->GetDoubleRef();
                        ScSingleRefData& rRef2 = rRef.Ref2;
                        ScSingleRefData& rRef1 = rRef.Ref1;

                        if (!rRef1.IsTabRel() && rRef1.Tab() == nOldTab)
                            rRef1.SetAbsTab( nNewTab);
                        if (!rRef2.IsTabRel() && rRef2.Tab() == nOldTab)
                            rRef2.SetAbsTab( nNewTab);
                        if (!rRef1.IsTabRel() && !rRef2.IsTabRel() && rRef1.Tab() > rRef2.Tab())
                        {
                            SCTAB nTab = rRef1.Tab();
                            rRef1.SetAbsTab( rRef2.Tab());
                            rRef2.SetAbsTab( nTab);
                        }
                    }
                    break;
                case svSingleRef :
                    {
                        ScSingleRefData& rRef = *p->GetSingleRef();

                        if (!rRef.IsTabRel() && rRef.Tab() == nOldTab)
                            rRef.SetAbsTab( nNewTab);
                    }
                    break;
                default:
                    ;
            }
        }
    }
}

bool ScTokenArray::ReferencesSheet( SCTAB nTab, SCTAB nPosTab ) const
{
    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN, false);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken* const * pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken* const * const pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            const FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch ( p->GetType() )
            {
                case svDoubleRef :
                    {
                        const ScComplexRefData& rRef = *p->GetDoubleRef();
                        const ScSingleRefData& rRef2 = rRef.Ref2;
                        const ScSingleRefData& rRef1 = rRef.Ref1;

                        SCTAB nTab1 = (rRef1.IsTabRel() ? rRef1.Tab() + nPosTab : rRef1.Tab());
                        SCTAB nTab2 = (rRef2.IsTabRel() ? rRef2.Tab() + nPosTab : rRef2.Tab());
                        if (nTab1 <= nTab && nTab <= nTab2)
                            return true;
                    }
                    break;
                case svSingleRef :
                    {
                        const ScSingleRefData& rRef = *p->GetSingleRef();
                        if (rRef.IsTabRel())
                        {
                            if (rRef.Tab() + nPosTab == nTab)
                                return true;
                        }
                        else
                        {
                            if (rRef.Tab() == nTab)
                                return true;
                        }
                    }
                    break;
                default:
                    ;
            }
        }
    }
    return false;
}

namespace {

ScRange getSelectedRange( const sc::RefUpdateContext& rCxt )
{
    ScRange aSelectedRange(ScAddress::INITIALIZE_INVALID);
    if (rCxt.mnColDelta < 0)
    {
        // Delete and shift to left.
        aSelectedRange.aStart = ScAddress(rCxt.maRange.aStart.Col()+rCxt.mnColDelta, rCxt.maRange.aStart.Row(), rCxt.maRange.aStart.Tab());
        aSelectedRange.aEnd = ScAddress(rCxt.maRange.aStart.Col()-1, rCxt.maRange.aEnd.Row(), rCxt.maRange.aEnd.Tab());
    }
    else if (rCxt.mnRowDelta < 0)
    {
        // Delete and shift up.
        aSelectedRange.aStart = ScAddress(rCxt.maRange.aStart.Col(), rCxt.maRange.aStart.Row()+rCxt.mnRowDelta, rCxt.maRange.aStart.Tab());
        aSelectedRange.aEnd = ScAddress(rCxt.maRange.aEnd.Col(), rCxt.maRange.aStart.Row()-1, rCxt.maRange.aEnd.Tab());
    }
    else if (rCxt.mnTabDelta < 0)
    {
        // Deleting sheets.
        // TODO : Figure out what to do here.
    }
    else if (rCxt.mnColDelta > 0)
    {
        // Insert and shift to the right.
        aSelectedRange.aStart = rCxt.maRange.aStart;
        aSelectedRange.aEnd = ScAddress(rCxt.maRange.aStart.Col()+rCxt.mnColDelta-1, rCxt.maRange.aEnd.Row(), rCxt.maRange.aEnd.Tab());
    }
    else if (rCxt.mnRowDelta > 0)
    {
        // Insert and shift down.
        aSelectedRange.aStart = rCxt.maRange.aStart;
        aSelectedRange.aEnd = ScAddress(rCxt.maRange.aEnd.Col(), rCxt.maRange.aStart.Row()+rCxt.mnRowDelta-1, rCxt.maRange.aEnd.Tab());
    }
    else if (rCxt.mnTabDelta > 0)
    {
        // Inserting sheets.
        // TODO : Figure out what to do here.
    }

    return aSelectedRange;
}

void setRefDeleted( ScSingleRefData& rRef, const sc::RefUpdateContext& rCxt )
{
    if (rCxt.mnColDelta < 0)
        rRef.SetColDeleted(true);
    else if (rCxt.mnRowDelta < 0)
        rRef.SetRowDeleted(true);
    else if (rCxt.mnTabDelta < 0)
        rRef.SetTabDeleted(true);
}

void restoreDeletedRef( ScSingleRefData& rRef, const sc::RefUpdateContext& rCxt )
{
    if (rCxt.mnColDelta)
    {
        if (rRef.IsColDeleted())
            rRef.SetColDeleted(false);
    }
    else if (rCxt.mnRowDelta)
    {
        if (rRef.IsRowDeleted())
            rRef.SetRowDeleted(false);
    }
    else if (rCxt.mnTabDelta)
    {
        if (rRef.IsTabDeleted())
            rRef.SetTabDeleted(false);
    }
}

void setRefDeleted( ScComplexRefData& rRef, const sc::RefUpdateContext& rCxt )
{
    if (rCxt.mnColDelta < 0)
    {
        rRef.Ref1.SetColDeleted(true);
        rRef.Ref2.SetColDeleted(true);
    }
    else if (rCxt.mnRowDelta < 0)
    {
        rRef.Ref1.SetRowDeleted(true);
        rRef.Ref2.SetRowDeleted(true);
    }
    else if (rCxt.mnTabDelta < 0)
    {
        rRef.Ref1.SetTabDeleted(true);
        rRef.Ref2.SetTabDeleted(true);
    }
}

void restoreDeletedRef( ScComplexRefData& rRef, const sc::RefUpdateContext& rCxt )
{
    restoreDeletedRef(rRef.Ref1, rCxt);
    restoreDeletedRef(rRef.Ref2, rCxt);
}

enum ShrinkResult
{
    UNMODIFIED,
    SHRUNK,
    STICKY
};

ShrinkResult shrinkRange( const sc::RefUpdateContext& rCxt, ScRange& rRefRange, const ScRange& rDeletedRange,
        const ScComplexRefData& rRef )
{
    if (!rDeletedRange.Intersects(rRefRange))
        return UNMODIFIED;

    if (rCxt.mnColDelta < 0)
    {
        if (rRef.IsEntireRow(rCxt.mrDoc.GetSheetLimits()))
            // Entire rows are not affected, columns are anchored.
            return STICKY;

        // Shifting left.
        if (rRefRange.aStart.Row() < rDeletedRange.aStart.Row() || rDeletedRange.aEnd.Row() < rRefRange.aEnd.Row())
            // Deleted range is only partially overlapping in vertical direction. Bail out.
            return UNMODIFIED;

        if (rDeletedRange.aStart.Col() <= rRefRange.aStart.Col())
        {
            if (rRefRange.aEnd.Col() <= rDeletedRange.aEnd.Col())
            {
                // Reference is entirely deleted.
                rRefRange.SetInvalid();
            }
            else
            {
                // The reference range is truncated on the left.
                SCCOL nOffset = rDeletedRange.aStart.Col() - rRefRange.aStart.Col();
                SCCOL nDelta = rRefRange.aStart.Col() - rDeletedRange.aEnd.Col() - 1;
                rRefRange.IncEndColSticky(rCxt.mrDoc, nDelta+nOffset);
                rRefRange.aStart.IncCol(nOffset);
            }
        }
        else if (rDeletedRange.aEnd.Col() < rRefRange.aEnd.Col())
        {
            if (rRefRange.IsEndColSticky(rCxt.mrDoc))
                // Sticky end not affected.
                return STICKY;

            // Reference is deleted in the middle. Move the last column
            // position to the left.
            SCCOL nDelta = rDeletedRange.aStart.Col() - rDeletedRange.aEnd.Col() - 1;
            rRefRange.IncEndColSticky(rCxt.mrDoc, nDelta);
        }
        else
        {
            if (rRefRange.IsEndColSticky(rCxt.mrDoc))
                // Sticky end not affected.
                return STICKY;

            // The reference range is truncated on the right.
            SCCOL nDelta = rDeletedRange.aStart.Col() - rRefRange.aEnd.Col() - 1;
            rRefRange.IncEndColSticky(rCxt.mrDoc, nDelta);
        }
        return SHRUNK;
    }
    else if (rCxt.mnRowDelta < 0)
    {
        if (rRef.IsEntireCol(rCxt.mrDoc.GetSheetLimits()))
            // Entire columns are not affected, rows are anchored.
            return STICKY;

        // Shifting up.

        if (rRefRange.aStart.Col() < rDeletedRange.aStart.Col() || rDeletedRange.aEnd.Col() < rRefRange.aEnd.Col())
            // Deleted range is only partially overlapping in horizontal direction. Bail out.
            return UNMODIFIED;

        if (rDeletedRange.aStart.Row() <= rRefRange.aStart.Row())
        {
            if (rRefRange.aEnd.Row() <= rDeletedRange.aEnd.Row())
            {
                // Reference is entirely deleted.
                rRefRange.SetInvalid();
            }
            else
            {
                // The reference range is truncated on the top.
                SCROW nOffset = rDeletedRange.aStart.Row() - rRefRange.aStart.Row();
                SCROW nDelta = rRefRange.aStart.Row() - rDeletedRange.aEnd.Row() - 1;
                rRefRange.IncEndRowSticky(rCxt.mrDoc, nDelta+nOffset);
                rRefRange.aStart.IncRow(nOffset);
            }
        }
        else if (rDeletedRange.aEnd.Row() < rRefRange.aEnd.Row())
        {
            if (rRefRange.IsEndRowSticky(rCxt.mrDoc))
                // Sticky end not affected.
                return STICKY;

            // Reference is deleted in the middle. Move the last row
            // position upward.
            SCROW nDelta = rDeletedRange.aStart.Row() - rDeletedRange.aEnd.Row() - 1;
            rRefRange.IncEndRowSticky(rCxt.mrDoc, nDelta);
        }
        else
        {
            if (rRefRange.IsEndRowSticky(rCxt.mrDoc))
                // Sticky end not affected.
                return STICKY;

            // The reference range is truncated on the bottom.
            SCROW nDelta = rDeletedRange.aStart.Row() - rRefRange.aEnd.Row() - 1;
            rRefRange.IncEndRowSticky(rCxt.mrDoc, nDelta);
        }
        return SHRUNK;
    }

    return UNMODIFIED;
}

bool expandRange( const sc::RefUpdateContext& rCxt, ScRange& rRefRange, const ScRange& rSelectedRange,
        const ScComplexRefData& rRef )
{
    if (!rSelectedRange.Intersects(rRefRange))
        return false;

    if (rCxt.mnColDelta > 0)
    {
        if (rRef.IsEntireRow(rCxt.mrDoc.GetSheetLimits()))
            // Entire rows are not affected, columns are anchored.
            return false;

        // Insert and shifting right.
        if (rRefRange.aStart.Row() < rSelectedRange.aStart.Row() || rSelectedRange.aEnd.Row() < rRefRange.aEnd.Row())
            // Selected range is only partially overlapping in vertical direction. Bail out.
            return false;

        if (rCxt.mrDoc.IsExpandRefs())
        {
            if (rRefRange.aEnd.Col() - rRefRange.aStart.Col() < 1)
                // Reference must be at least two columns wide.
                return false;
        }
        else
        {
            if (rSelectedRange.aStart.Col() <= rRefRange.aStart.Col())
                // Selected range is at the left end and the edge expansion is turned off.  No expansion.
                return false;
        }

        if (rRefRange.IsEndColSticky(rCxt.mrDoc))
            // Sticky end not affected.
            return false;

        // Move the last column position to the right.
        SCCOL nDelta = rSelectedRange.aEnd.Col() - rSelectedRange.aStart.Col() + 1;
        rRefRange.IncEndColSticky(rCxt.mrDoc, nDelta);
        return true;
    }
    else if (rCxt.mnRowDelta > 0)
    {
        if (rRef.IsEntireCol(rCxt.mrDoc.GetSheetLimits()))
            // Entire columns are not affected, rows are anchored.
            return false;

        // Insert and shifting down.
        if (rRefRange.aStart.Col() < rSelectedRange.aStart.Col() || rSelectedRange.aEnd.Col() < rRefRange.aEnd.Col())
            // Selected range is only partially overlapping in horizontal direction. Bail out.
            return false;

        if (rCxt.mrDoc.IsExpandRefs())
        {
            if (rRefRange.aEnd.Row() - rRefRange.aStart.Row() < 1)
                // Reference must be at least two rows tall.
                return false;
        }
        else
        {
            if (rSelectedRange.aStart.Row() <= rRefRange.aStart.Row())
                // Selected range is at the top end and the edge expansion is turned off.  No expansion.
                return false;
        }

        if (rRefRange.IsEndRowSticky(rCxt.mrDoc))
            // Sticky end not affected.
            return false;

        // Move the last row position down.
        SCROW nDelta = rSelectedRange.aEnd.Row() - rSelectedRange.aStart.Row() + 1;
        rRefRange.IncEndRowSticky(rCxt.mrDoc, nDelta);
        return true;
    }
    return false;
}

/**
 * Check if the referenced range is expandable when the selected range is
 * not overlapping the referenced range.
 */
bool expandRangeByEdge( const sc::RefUpdateContext& rCxt, ScRange& rRefRange, const ScRange& rSelectedRange,
        const ScComplexRefData& rRef )
{
    if (!rCxt.mrDoc.IsExpandRefs())
        // Edge-expansion is turned off.
        return false;

    if (rSelectedRange.aStart.Tab() > rRefRange.aStart.Tab() || rRefRange.aEnd.Tab() > rSelectedRange.aEnd.Tab())
        // Sheet references not within selected range.
        return false;

    if (rCxt.mnColDelta > 0)
    {
        if (rRef.IsEntireRow(rCxt.mrDoc.GetSheetLimits()))
            // Entire rows are not affected, columns are anchored.
            return false;

        // Insert and shift right.

        if (rRefRange.aEnd.Col() - rRefRange.aStart.Col() < 1)
            // Reference must be at least two columns wide.
            return false;

        if (rRefRange.aStart.Row() < rSelectedRange.aStart.Row() || rSelectedRange.aEnd.Row() < rRefRange.aEnd.Row())
            // Selected range is only partially overlapping in vertical direction. Bail out.
            return false;

        if (rSelectedRange.aStart.Col() - rRefRange.aEnd.Col() != 1)
            // Selected range is not immediately adjacent. Bail out.
            return false;

        if (rRefRange.IsEndColSticky(rCxt.mrDoc))
            // Sticky end not affected.
            return false;

        // Move the last column position to the right.
        SCCOL nDelta = rSelectedRange.aEnd.Col() - rSelectedRange.aStart.Col() + 1;
        rRefRange.IncEndColSticky(rCxt.mrDoc, nDelta);
        return true;
    }
    else if (rCxt.mnRowDelta > 0)
    {
        if (rRef.IsEntireCol(rCxt.mrDoc.GetSheetLimits()))
            // Entire columns are not affected, rows are anchored.
            return false;

        if (rRefRange.aEnd.Row() - rRefRange.aStart.Row() < 1)
            // Reference must be at least two rows tall.
            return false;

        if (rRefRange.aStart.Col() < rSelectedRange.aStart.Col() || rSelectedRange.aEnd.Col() < rRefRange.aEnd.Col())
            // Selected range is only partially overlapping in horizontal direction. Bail out.
            return false;

        if (rSelectedRange.aStart.Row() - rRefRange.aEnd.Row() != 1)
            // Selected range is not immediately adjacent. Bail out.
            return false;

        if (rRefRange.IsEndRowSticky(rCxt.mrDoc))
            // Sticky end not affected.
            return false;

        // Move the last row position down.
        SCROW nDelta = rSelectedRange.aEnd.Row() - rSelectedRange.aStart.Row() + 1;
        rRefRange.IncEndRowSticky(rCxt.mrDoc, nDelta);
        return true;
    }

    return false;
}

bool isNameModified( const sc::UpdatedRangeNames& rUpdatedNames, SCTAB nOldTab, const formula::FormulaToken& rToken )
{
    SCTAB nTab = -1;
    if (rToken.GetSheet() >= 0)
        nTab = nOldTab;

    // Check if this named expression has been modified.
    return rUpdatedNames.isNameUpdated(nTab, rToken.GetIndex());
}

bool isDBDataModified( const ScDocument& rDoc, const formula::FormulaToken& rToken )
{
    // Check if this DBData has been modified.
    const ScDBData* pDBData = rDoc.GetDBCollection()->getNamedDBs().findByIndex( rToken.GetIndex());
    if (!pDBData)
        return true;

    return pDBData->IsModified();
}

}

sc::RefUpdateResult ScTokenArray::AdjustReferenceOnShift( const sc::RefUpdateContext& rCxt, const ScAddress& rOldPos )
{
    ScRange aSelectedRange = getSelectedRange(rCxt);

    sc::RefUpdateResult aRes;
    ScAddress aNewPos = rOldPos;
    bool bCellShifted = rCxt.maRange.Contains(rOldPos);
    if (bCellShifted)
    {
        ScAddress aErrorPos( ScAddress::UNINITIALIZED );
        if (!aNewPos.Move(rCxt.mnColDelta, rCxt.mnRowDelta, rCxt.mnTabDelta, aErrorPos, rCxt.mrDoc))
        {
            assert(!"can't move");
        }
    }

    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken** pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken** pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch (p->GetType())
            {
                case svSingleRef:
                    {
                        ScSingleRefData& rRef = *p->GetSingleRef();
                        ScAddress aAbs = rRef.toAbs(*mxSheetLimits, rOldPos);

                        if (rCxt.isDeleted() && aSelectedRange.Contains(aAbs))
                        {
                            // This reference is in the deleted region.
                            setRefDeleted(rRef, rCxt);
                            aRes.mbValueChanged = true;
                            break;
                        }

                        if (!rCxt.isDeleted() && rRef.IsDeleted())
                        {
                            // Check if the token has reference to previously deleted region.
                            ScAddress aCheckPos = rRef.toAbs(*mxSheetLimits, aNewPos);
                            if (rCxt.maRange.Contains(aCheckPos))
                            {
                                restoreDeletedRef(rRef, rCxt);
                                aRes.mbValueChanged = true;
                                break;
                            }
                        }

                        if (rCxt.maRange.Contains(aAbs))
                        {
                            ScAddress aErrorPos( ScAddress::UNINITIALIZED );
                            if (!aAbs.Move(rCxt.mnColDelta, rCxt.mnRowDelta, rCxt.mnTabDelta, aErrorPos, rCxt.mrDoc))
                                aAbs = aErrorPos;
                            aRes.mbReferenceModified = true;
                        }

                        rRef.SetAddress(*mxSheetLimits, aAbs, aNewPos);
                    }
                    break;
                case svDoubleRef:
                    {
                        ScComplexRefData& rRef = *p->GetDoubleRef();
                        ScRange aAbs = rRef.toAbs(*mxSheetLimits, rOldPos);

                        if (rCxt.isDeleted())
                        {
                            if (aSelectedRange.Contains(aAbs))
                            {
                                // This reference is in the deleted region.
                                setRefDeleted(rRef, rCxt);
                                aRes.mbValueChanged = true;
                                break;
                            }
                            else if (aSelectedRange.Intersects(aAbs))
                            {
                                const ShrinkResult eSR = shrinkRange(rCxt, aAbs, aSelectedRange, rRef);
                                if (eSR == SHRUNK)
                                {
                                    // The reference range has been shrunk.
                                    rRef.SetRange(*mxSheetLimits, aAbs, aNewPos);
                                    aRes.mbValueChanged = true;
                                    aRes.mbReferenceModified = true;
                                    break;
                                }
                                else if (eSR == STICKY)
                                {
                                    // The reference range stays the same but a
                                    // new (empty) cell range is shifted in and
                                    // may change the calculation result.
                                    aRes.mbValueChanged = true;
                                    // Sticky when intersecting the selected
                                    // range means also that the other
                                    // conditions below are not met,
                                    // specifically not the
                                    // if (rCxt.maRange.Contains(aAbs))
                                    // that is able to update the reference,
                                    // but aSelectedRange does not intersect
                                    // with rCxt.maRange so that can't happen
                                    // and we can bail out early without
                                    // updating the reference.
                                    break;
                                }
                            }
                        }

                        if (!rCxt.isDeleted() && rRef.IsDeleted())
                        {
                            // Check if the token has reference to previously deleted region.
                            ScRange aCheckRange = rRef.toAbs(*mxSheetLimits, aNewPos);
                            if (aSelectedRange.Contains(aCheckRange))
                            {
                                // This reference was previously in the deleted region. Restore it.
                                restoreDeletedRef(rRef, rCxt);
                                aRes.mbValueChanged = true;
                                break;
                            }
                        }

                        if (rCxt.isInserted())
                        {
                            if (expandRange(rCxt, aAbs, aSelectedRange, rRef))
                            {
                                // The reference range has been expanded.
                                rRef.SetRange(*mxSheetLimits, aAbs, aNewPos);
                                aRes.mbValueChanged = true;
                                aRes.mbReferenceModified = true;
                                break;
                            }

                            if (expandRangeByEdge(rCxt, aAbs, aSelectedRange, rRef))
                            {
                                // The reference range has been expanded on the edge.
                                rRef.SetRange(*mxSheetLimits, aAbs, aNewPos);
                                aRes.mbValueChanged = true;
                                aRes.mbReferenceModified = true;
                                break;
                            }
                        }

                        if (rCxt.maRange.Contains(aAbs))
                        {
                            // We shift either by column or by row, not both,
                            // so moving the reference has only to be done in
                            // the non-sticky case.
                            if ((rCxt.mnRowDelta && rRef.IsEntireCol(rCxt.mrDoc.GetSheetLimits()))
                                || (rCxt.mnColDelta && rRef.IsEntireRow(rCxt.mrDoc.GetSheetLimits())))
                            {
                                // In entire col/row, values are shifted within
                                // the reference, which affects all positional
                                // results like in MATCH or matrix positions.
                                aRes.mbValueChanged = true;
                            }
                            else
                            {
                                ScRange aErrorRange( ScAddress::UNINITIALIZED );
                                if (!aAbs.MoveSticky(rCxt.mrDoc, rCxt.mnColDelta, rCxt.mnRowDelta, rCxt.mnTabDelta, aErrorRange))
                                    aAbs = aErrorRange;
                                aRes.mbReferenceModified = true;
                            }
                        }
                        else if (rCxt.maRange.Intersects(aAbs))
                        {
                            // Part of the referenced range is being shifted. This
                            // will change the values of the range.
                            aRes.mbValueChanged = true;
                        }

                        rRef.SetRange(*mxSheetLimits, aAbs, aNewPos);
                    }
                    break;
                case svExternalSingleRef:
                    {
                        // For external reference, just reset the reference with
                        // respect to the new cell position.
                        ScSingleRefData& rRef = *p->GetSingleRef();
                        ScAddress aAbs = rRef.toAbs(*mxSheetLimits, rOldPos);
                        rRef.SetAddress(*mxSheetLimits, aAbs, aNewPos);
                    }
                    break;
                case svExternalDoubleRef:
                    {
                        // Same as above.
                        ScComplexRefData& rRef = *p->GetDoubleRef();
                        ScRange aAbs = rRef.toAbs(*mxSheetLimits, rOldPos);
                        rRef.SetRange(*mxSheetLimits, aAbs, aNewPos);
                    }
                    break;
                default:
                    ;
            }

            // For ocTableRef p is the inner token of *pp, so have a separate
            // condition here.
            if ((*pp)->GetType() == svIndex)
            {
                switch ((*pp)->GetOpCode())
                {
                    case ocName:
                        {
                            SCTAB nOldTab = (*pp)->GetSheet();
                            if (isNameModified(rCxt.maUpdatedNames, nOldTab, **pp))
                                aRes.mbNameModified = true;
                            if (rCxt.mnTabDelta &&
                                    rCxt.maRange.aStart.Tab() <= nOldTab && nOldTab <= rCxt.maRange.aEnd.Tab())
                            {
                                aRes.mbNameModified = true;
                                (*pp)->SetSheet( nOldTab + rCxt.mnTabDelta);
                            }
                        }
                        break;
                    case ocDBArea:
                    case ocTableRef:
                        if (isDBDataModified(rCxt.mrDoc, **pp))
                            aRes.mbNameModified = true;
                        break;
                    default:
                        ;   // nothing
                }
            }
        }
    }

    return aRes;
}

sc::RefUpdateResult ScTokenArray::AdjustReferenceOnMove(
    const sc::RefUpdateContext& rCxt, const ScAddress& rOldPos, const ScAddress& rNewPos )
{
    sc::RefUpdateResult aRes;

    if (!rCxt.mnColDelta && !rCxt.mnRowDelta && !rCxt.mnTabDelta)
        // The cell hasn't moved at all.
        return aRes;

    // When moving, the range in the context is the destination range. We need
    // to use the old range prior to the move for hit analysis.
    ScRange aOldRange = rCxt.maRange;
    ScRange aErrorMoveRange( ScAddress::UNINITIALIZED );
    if (!aOldRange.Move(-rCxt.mnColDelta, -rCxt.mnRowDelta, -rCxt.mnTabDelta, aErrorMoveRange, rCxt.mrDoc))
    {
        assert(!"can't move");
    }

    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken** pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken** pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch (p->GetType())
            {
                case svSingleRef:
                    {
                        ScSingleRefData& rRef = *p->GetSingleRef();
                        ScAddress aAbs = rRef.toAbs(*mxSheetLimits, rOldPos);

                        // Do not update the reference in transposed case (cut paste transposed).
                        // The reference will be updated in UpdateTranspose().
                        // Additionally, do not update the references from cells within the moved
                        // range as they lead to #REF! errors here. These #REF! cannot by fixed
                        // later in UpdateTranspose().
                        if (rCxt.mbTransposed && (aOldRange.Contains(rOldPos) || aOldRange.Contains(aAbs)))
                            break;

                        if (aOldRange.Contains(aAbs))
                        {
                            ScAddress aErrorPos( ScAddress::UNINITIALIZED );
                            if (!aAbs.Move(rCxt.mnColDelta, rCxt.mnRowDelta, rCxt.mnTabDelta, aErrorPos, rCxt.mrDoc))
                                aAbs = aErrorPos;
                            aRes.mbReferenceModified = true;
                        }
                        else if (rCxt.maRange.Contains(aAbs))
                        {
                            // Referenced cell has been overwritten.
                            aRes.mbValueChanged = true;
                        }

                        rRef.SetAddress(*mxSheetLimits, aAbs, rNewPos);
                        rRef.SetFlag3D(rRef.IsFlag3D() || !rRef.IsTabRel() || aAbs.Tab() != rNewPos.Tab());
                    }
                    break;
                case svDoubleRef:
                    {
                        ScComplexRefData& rRef = *p->GetDoubleRef();
                        ScRange aAbs = rRef.toAbs(*mxSheetLimits, rOldPos);

                        // Do not update the reference in transposed case (cut paste transposed).
                        // The reference will be updated in UpdateTranspose().
                        // Additionally, do not update the references from cells within the moved
                        // range as they lead to #REF! errors here. These #REF! cannot by fixed
                        // later in UpdateTranspose().
                        if (rCxt.mbTransposed && (aOldRange.Contains(rOldPos) || aOldRange.Contains(aAbs)))
                            break;

                        if (aOldRange.Contains(aAbs))
                        {
                            ScRange aErrorRange( ScAddress::UNINITIALIZED );
                            if (!aAbs.Move(rCxt.mnColDelta, rCxt.mnRowDelta, rCxt.mnTabDelta, aErrorRange, rCxt.mrDoc))
                                aAbs = aErrorRange;
                            aRes.mbReferenceModified = true;
                        }
                        else if (rCxt.maRange.Contains(aAbs))
                        {
                            // Referenced range has been entirely overwritten.
                            aRes.mbValueChanged = true;
                        }

                        rRef.SetRange(*mxSheetLimits, aAbs, rNewPos);
                        bool b1, b2;
                        if (aAbs.aStart.Tab() != aAbs.aEnd.Tab())
                        {
                            // More than one sheet referenced => has to have
                            // both 3D flags.
                            b1 = b2 = true;
                        }
                        else
                        {
                            // Keep given 3D flag even for relative sheet
                            // reference to same sheet.
                            // Absolute sheet reference => set 3D flag.
                            // Reference to another sheet => set 3D flag.
                            b1 = rRef.Ref1.IsFlag3D() || !rRef.Ref1.IsTabRel() || rNewPos.Tab() != aAbs.aStart.Tab();
                            b2 = rRef.Ref2.IsFlag3D() || !rRef.Ref2.IsTabRel() || rNewPos.Tab() != aAbs.aEnd.Tab();
                            // End part has 3D flag => start part must have it too.
                            if (b2)
                                b1 = true;
                            // End part sheet reference is identical to start
                            // part sheet reference and end part sheet
                            // reference was not explicitly given => clear end
                            // part 3D flag.
                            if (b1 && b2 && rRef.Ref1.IsTabRel() == rRef.Ref2.IsTabRel() && !rRef.Ref2.IsFlag3D())
                                b2 = false;
                        }
                        rRef.Ref1.SetFlag3D(b1);
                        rRef.Ref2.SetFlag3D(b2);
                    }
                    break;
                case svExternalSingleRef:
                    {
                        ScSingleRefData& rRef = *p->GetSingleRef();
                        ScAddress aAbs = rRef.toAbs(*mxSheetLimits, rOldPos);
                        rRef.SetAddress(*mxSheetLimits, aAbs, rNewPos);
                    }
                    break;
                case svExternalDoubleRef:
                    {
                        ScComplexRefData& rRef = *p->GetDoubleRef();
                        ScRange aAbs = rRef.toAbs(*mxSheetLimits, rOldPos);
                        rRef.SetRange(*mxSheetLimits, aAbs, rNewPos);
                    }
                    break;
                default:
                    ;
            }

            // For ocTableRef p is the inner token of *pp, so have a separate
            // condition here.
            if ((*pp)->GetType() == svIndex)
            {
                switch ((*pp)->GetOpCode())
                {
                    case ocName:
                        {
                            SCTAB nOldTab = (*pp)->GetSheet();
                            if (isNameModified(rCxt.maUpdatedNames, nOldTab, **pp))
                                aRes.mbNameModified = true;
                        }
                        break;
                    case ocDBArea:
                    case ocTableRef:
                        if (isDBDataModified(rCxt.mrDoc, **pp))
                            aRes.mbNameModified = true;
                        break;
                    default:
                        ;   // nothing
                }
            }
        }
    }

    return aRes;
}

void ScTokenArray::MoveReferenceColReorder(
    const ScAddress& rPos, SCTAB nTab, SCROW nRow1, SCROW nRow2, const sc::ColRowReorderMapType& rColMap )
{
    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken** pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken** pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch (p->GetType())
            {
                case svSingleRef:
                    {
                        ScSingleRefData& rRef = *p->GetSingleRef();
                        ScAddress aAbs = rRef.toAbs(*mxSheetLimits, rPos);

                        if (aAbs.Tab() == nTab && nRow1 <= aAbs.Row() && aAbs.Row() <= nRow2)
                        {
                            // Inside reordered row range.
                            sc::ColRowReorderMapType::const_iterator it = rColMap.find(aAbs.Col());
                            if (it != rColMap.end())
                            {
                                // This column is reordered.
                                SCCOL nNewCol = it->second;
                                aAbs.SetCol(nNewCol);
                                rRef.SetAddress(*mxSheetLimits, aAbs, rPos);
                            }
                        }
                    }
                break;
                case svDoubleRef:
                    {
                        ScComplexRefData& rRef = *p->GetDoubleRef();
                        ScRange aAbs = rRef.toAbs(*mxSheetLimits, rPos);

                        if (aAbs.aStart.Tab() != aAbs.aEnd.Tab())
                            // Must be a single-sheet reference.
                            break;

                        if (aAbs.aStart.Col() != aAbs.aEnd.Col())
                            // Whole range must fit in a single column.
                            break;

                        if (aAbs.aStart.Tab() == nTab && nRow1 <= aAbs.aStart.Row() && aAbs.aEnd.Row() <= nRow2)
                        {
                            // Inside reordered row range.
                            sc::ColRowReorderMapType::const_iterator it = rColMap.find(aAbs.aStart.Col());
                            if (it != rColMap.end())
                            {
                                // This column is reordered.
                                SCCOL nNewCol = it->second;
                                aAbs.aStart.SetCol(nNewCol);
                                aAbs.aEnd.SetCol(nNewCol);
                                rRef.SetRange(*mxSheetLimits, aAbs, rPos);
                            }
                        }
                    }
                break;
                default:
                    ;
            }
        }
    }
}

void ScTokenArray::MoveReferenceRowReorder( const ScAddress& rPos, SCTAB nTab, SCCOL nCol1, SCCOL nCol2, const sc::ColRowReorderMapType& rRowMap )
{
    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken** pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken** pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch (p->GetType())
            {
                case svSingleRef:
                    {
                        ScSingleRefData& rRef = *p->GetSingleRef();
                        ScAddress aAbs = rRef.toAbs(*mxSheetLimits, rPos);

                        if (aAbs.Tab() == nTab && nCol1 <= aAbs.Col() && aAbs.Col() <= nCol2)
                        {
                            // Inside reordered column range.
                            sc::ColRowReorderMapType::const_iterator it = rRowMap.find(aAbs.Row());
                            if (it != rRowMap.end())
                            {
                                // This column is reordered.
                                SCROW nNewRow = it->second;
                                aAbs.SetRow(nNewRow);
                                rRef.SetAddress(*mxSheetLimits, aAbs, rPos);
                            }
                        }
                    }
                break;
                case svDoubleRef:
                    {
                        ScComplexRefData& rRef = *p->GetDoubleRef();
                        ScRange aAbs = rRef.toAbs(*mxSheetLimits, rPos);

                        if (aAbs.aStart.Tab() != aAbs.aEnd.Tab())
                            // Must be a single-sheet reference.
                            break;

                        if (aAbs.aStart.Row() != aAbs.aEnd.Row())
                            // Whole range must fit in a single row.
                            break;

                        if (aAbs.aStart.Tab() == nTab && nCol1 <= aAbs.aStart.Col() && aAbs.aEnd.Col() <= nCol2)
                        {
                            // Inside reordered column range.
                            sc::ColRowReorderMapType::const_iterator it = rRowMap.find(aAbs.aStart.Row());
                            if (it != rRowMap.end())
                            {
                                // This row is reordered.
                                SCROW nNewRow = it->second;
                                aAbs.aStart.SetRow(nNewRow);
                                aAbs.aEnd.SetRow(nNewRow);
                                rRef.SetRange(*mxSheetLimits, aAbs, rPos);
                            }
                        }
                    }
                break;
                default:
                    ;
            }
        }
    }
}

namespace {

bool adjustSingleRefInName(
    ScSingleRefData& rRef, const sc::RefUpdateContext& rCxt, const ScAddress& rPos,
    ScComplexRefData* pEndOfComplex )
{
    ScAddress aAbs = rRef.toAbs(rCxt.mrDoc, rPos);

    if (aAbs.Tab() < rCxt.maRange.aStart.Tab() || rCxt.maRange.aEnd.Tab() < aAbs.Tab())
    {
        // This references a sheet that has not shifted. Don't change it.
        return false;
    }

    if (!rCxt.maRange.Contains(rRef.toAbs(rCxt.mrDoc, rPos)))
        return false;

    bool bChanged = false;

    if (rCxt.mnColDelta && !rRef.IsColRel())
    {
        // Adjust absolute column reference.
        if (rCxt.maRange.aStart.Col() <= rRef.Col() && rRef.Col() <= rCxt.maRange.aEnd.Col())
        {
            if (pEndOfComplex)
            {
                if (pEndOfComplex->IncEndColSticky(rCxt.mrDoc, rCxt.mnColDelta, rPos))
                    bChanged = true;
            }
            else
            {
                rRef.IncCol(rCxt.mnColDelta);
                bChanged = true;
            }
        }
    }

    if (rCxt.mnRowDelta && !rRef.IsRowRel())
    {
        // Adjust absolute row reference.
        if (rCxt.maRange.aStart.Row() <= rRef.Row() && rRef.Row() <= rCxt.maRange.aEnd.Row())
        {
            if (pEndOfComplex)
            {
                if (pEndOfComplex->IncEndRowSticky(rCxt.mrDoc, rCxt.mnRowDelta, rPos))
                    bChanged = true;
            }
            else
            {
                rRef.IncRow(rCxt.mnRowDelta);
                bChanged = true;
            }
        }
    }

    if (!rRef.IsTabRel() && rCxt.mnTabDelta)
    {
        // Sheet range has already been checked above.
        rRef.IncTab(rCxt.mnTabDelta);
        bChanged = true;
    }

    return bChanged;
}

bool adjustDoubleRefInName(
    ScComplexRefData& rRef, const sc::RefUpdateContext& rCxt, const ScAddress& rPos )
{
    bool bRefChanged = false;
    if (rCxt.mrDoc.IsExpandRefs())
    {
        if (rCxt.mnRowDelta > 0 && !rRef.Ref1.IsRowRel() && !rRef.Ref2.IsRowRel())
        {
            ScRange aAbs = rRef.toAbs(rCxt.mrDoc, rPos);
            // Expand only if at least two rows tall.
            if (aAbs.aStart.Row() < aAbs.aEnd.Row())
            {
                // Check and see if we should expand the range at the top.
                ScRange aSelectedRange = getSelectedRange(rCxt);
                if (aSelectedRange.Intersects(aAbs))
                {
                    // Selection intersects the referenced range. Only expand the
                    // bottom position.
                    rRef.IncEndRowSticky(rCxt.mrDoc, rCxt.mnRowDelta, rPos);
                    return true;
                }
            }
        }
        if (rCxt.mnColDelta > 0 && !rRef.Ref1.IsColRel() && !rRef.Ref2.IsColRel())
        {
            ScRange aAbs = rRef.toAbs(rCxt.mrDoc, rPos);
            // Expand only if at least two columns wide.
            if (aAbs.aStart.Col() < aAbs.aEnd.Col())
            {
                // Check and see if we should expand the range at the left.
                ScRange aSelectedRange = getSelectedRange(rCxt);
                if (aSelectedRange.Intersects(aAbs))
                {
                    // Selection intersects the referenced range. Only expand the
                    // right position.
                    rRef.IncEndColSticky(rCxt.mrDoc, rCxt.mnColDelta, rPos);
                    return true;
                }
            }
        }
    }

    if ((rCxt.mnRowDelta && rRef.IsEntireCol(rCxt.mrDoc.GetSheetLimits()))
        || (rCxt.mnColDelta && rRef.IsEntireRow(rCxt.mrDoc.GetSheetLimits())))
    {
        sc::RefUpdateContext aCxt( rCxt.mrDoc);
        // We only need a few parameters of RefUpdateContext.
        aCxt.maRange = rCxt.maRange;
        aCxt.mnColDelta = rCxt.mnColDelta;
        aCxt.mnRowDelta = rCxt.mnRowDelta;
        aCxt.mnTabDelta = rCxt.mnTabDelta;

        // References to entire col/row are not to be adjusted in the other axis.
        if (aCxt.mnRowDelta && rRef.IsEntireCol(rCxt.mrDoc.GetSheetLimits()))
            aCxt.mnRowDelta = 0;
        if (aCxt.mnColDelta && rRef.IsEntireRow(rCxt.mrDoc.GetSheetLimits()))
            aCxt.mnColDelta = 0;
        if (!aCxt.mnColDelta && !aCxt.mnRowDelta && !aCxt.mnTabDelta)
            // early bailout
            return bRefChanged;

        // Ref2 before Ref1 for sticky ends.
        if (adjustSingleRefInName(rRef.Ref2, aCxt, rPos, &rRef))
            bRefChanged = true;

        if (adjustSingleRefInName(rRef.Ref1, aCxt, rPos, nullptr))
            bRefChanged = true;
    }
    else
    {
        // Ref2 before Ref1 for sticky ends.
        if (adjustSingleRefInName(rRef.Ref2, rCxt, rPos, &rRef))
            bRefChanged = true;

        if (adjustSingleRefInName(rRef.Ref1, rCxt, rPos, nullptr))
            bRefChanged = true;
    }

    return bRefChanged;
}

}

sc::RefUpdateResult ScTokenArray::AdjustReferenceInName(
    const sc::RefUpdateContext& rCxt, const ScAddress& rPos )
{
    if (rCxt.meMode == URM_MOVE)
        return AdjustReferenceInMovedName(rCxt, rPos);

    sc::RefUpdateResult aRes;

    if (rCxt.meMode == URM_COPY)
        // Copying cells does not modify named expressions.
        return aRes;

    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken** pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken** pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch (p->GetType())
            {
                case svSingleRef:
                    {
                        ScSingleRefData& rRef = *p->GetSingleRef();
                        if (rCxt.mnRowDelta < 0)
                        {
                            // row(s) deleted.

                            if (rRef.IsRowRel())
                                // Don't modify relative references in names.
                                break;

                            ScAddress aAbs = rRef.toAbs(rCxt.mrDoc, rPos);

                            if (aAbs.Col() < rCxt.maRange.aStart.Col() || rCxt.maRange.aEnd.Col() < aAbs.Col())
                                // column of the reference is not in the deleted column range.
                                break;

                            if (aAbs.Tab() > rCxt.maRange.aEnd.Tab() || aAbs.Tab() < rCxt.maRange.aStart.Tab())
                                // wrong tables
                                break;

                            const SCROW nDelStartRow = rCxt.maRange.aStart.Row() + rCxt.mnRowDelta;
                            const SCROW nDelEndRow = nDelStartRow - rCxt.mnRowDelta - 1;

                            if (nDelStartRow <= aAbs.Row() && aAbs.Row() <= nDelEndRow)
                            {
                                // This reference is deleted.
                                rRef.SetRowDeleted(true);
                                aRes.mbReferenceModified = true;
                                break;
                            }
                        }
                        else if (rCxt.mnColDelta < 0)
                        {
                            // column(s) deleted.

                            if (rRef.IsColRel())
                                // Don't modify relative references in names.
                                break;

                            ScAddress aAbs = rRef.toAbs(rCxt.mrDoc, rPos);

                            if (aAbs.Row() < rCxt.maRange.aStart.Row() || rCxt.maRange.aEnd.Row() < aAbs.Row())
                                // row of the reference is not in the deleted row range.
                                break;

                            if (aAbs.Tab() > rCxt.maRange.aEnd.Tab() || aAbs.Tab() < rCxt.maRange.aStart.Tab())
                                // wrong tables
                                break;

                            const SCCOL nDelStartCol = rCxt.maRange.aStart.Col() + rCxt.mnColDelta;
                            const SCCOL nDelEndCol = nDelStartCol - rCxt.mnColDelta - 1;

                            if (nDelStartCol <= aAbs.Col() && aAbs.Col() <= nDelEndCol)
                            {
                                // This reference is deleted.
                                rRef.SetColDeleted(true);
                                aRes.mbReferenceModified = true;
                                break;
                            }
                        }

                        if (adjustSingleRefInName(rRef, rCxt, rPos, nullptr))
                            aRes.mbReferenceModified = true;
                    }
                    break;
                case svDoubleRef:
                    {
                        ScComplexRefData& rRef = *p->GetDoubleRef();
                        ScRange aAbs = rRef.toAbs(rCxt.mrDoc, rPos);

                        if (aAbs.aStart.Tab() > rCxt.maRange.aEnd.Tab() || aAbs.aEnd.Tab() < rCxt.maRange.aStart.Tab())
                            // Sheet references not affected.
                            break;

                        if (rCxt.maRange.Contains(aAbs))
                        {
                            // This range is entirely within the shifted region.
                            if (adjustDoubleRefInName(rRef, rCxt, rPos))
                                aRes.mbReferenceModified = true;
                        }
                        else if (rCxt.mnRowDelta < 0)
                        {
                            // row(s) deleted.

                            if (rRef.IsEntireCol(rCxt.mrDoc.GetSheetLimits()))
                                // Rows of entire columns are not affected.
                                break;

                            if (rRef.Ref1.IsRowRel() || rRef.Ref2.IsRowRel())
                                // Don't modify relative references in names.
                                break;

                            if (aAbs.aStart.Col() < rCxt.maRange.aStart.Col() || rCxt.maRange.aEnd.Col() < aAbs.aEnd.Col())
                                // column range of the reference is not entirely in the deleted column range.
                                break;

                            ScRange aDeleted = rCxt.maRange;
                            aDeleted.aStart.IncRow(rCxt.mnRowDelta);
                            aDeleted.aEnd.SetRow(aDeleted.aStart.Row()-rCxt.mnRowDelta-1);

                            if (aAbs.aEnd.Row() < aDeleted.aStart.Row() || aDeleted.aEnd.Row() < aAbs.aStart.Row())
                                // reference range doesn't intersect with the deleted range.
                                break;

                            if (aDeleted.aStart.Row() <= aAbs.aStart.Row() && aAbs.aEnd.Row() <= aDeleted.aEnd.Row())
                            {
                                // This reference is entirely deleted.
                                rRef.Ref1.SetRowDeleted(true);
                                rRef.Ref2.SetRowDeleted(true);
                                aRes.mbReferenceModified = true;
                                break;
                            }

                            if (aAbs.aStart.Row() < aDeleted.aStart.Row())
                            {
                                if (!aAbs.IsEndRowSticky(rCxt.mrDoc))
                                {
                                    if (aDeleted.aEnd.Row() < aAbs.aEnd.Row())
                                        // Deleted in the middle.  Make the reference shorter.
                                        rRef.Ref2.IncRow(rCxt.mnRowDelta);
                                    else
                                        // Deleted at tail end.  Cut off the lower part.
                                        rRef.Ref2.SetAbsRow(aDeleted.aStart.Row()-1);
                                }
                            }
                            else
                            {
                                // Deleted at the top.  Cut the top off and shift up.
                                rRef.Ref1.SetAbsRow(aDeleted.aEnd.Row()+1);
                                rRef.Ref1.IncRow(rCxt.mnRowDelta);
                                if (!aAbs.IsEndRowSticky(rCxt.mrDoc))
                                    rRef.Ref2.IncRow(rCxt.mnRowDelta);
                            }

                            aRes.mbReferenceModified = true;
                        }
                        else if (rCxt.mnColDelta < 0)
                        {
                            // column(s) deleted.

                            if (rRef.IsEntireRow(rCxt.mrDoc.GetSheetLimits()))
                                // Rows of entire rows are not affected.
                                break;

                            if (rRef.Ref1.IsColRel() || rRef.Ref2.IsColRel())
                                // Don't modify relative references in names.
                                break;

                            if (aAbs.aStart.Row() < rCxt.maRange.aStart.Row() || rCxt.maRange.aEnd.Row() < aAbs.aEnd.Row())
                                // row range of the reference is not entirely in the deleted row range.
                                break;

                            ScRange aDeleted = rCxt.maRange;
                            aDeleted.aStart.IncCol(rCxt.mnColDelta);
                            aDeleted.aEnd.SetCol(aDeleted.aStart.Col()-rCxt.mnColDelta-1);

                            if (aAbs.aEnd.Col() < aDeleted.aStart.Col() || aDeleted.aEnd.Col() < aAbs.aStart.Col())
                                // reference range doesn't intersect with the deleted range.
                                break;

                            if (aDeleted.aStart.Col() <= aAbs.aStart.Col() && aAbs.aEnd.Col() <= aDeleted.aEnd.Col())
                            {
                                // This reference is entirely deleted.
                                rRef.Ref1.SetColDeleted(true);
                                rRef.Ref2.SetColDeleted(true);
                                aRes.mbReferenceModified = true;
                                break;
                            }

                            if (aAbs.aStart.Col() < aDeleted.aStart.Col())
                            {
                                if (!aAbs.IsEndColSticky(rCxt.mrDoc))
                                {
                                    if (aDeleted.aEnd.Col() < aAbs.aEnd.Col())
                                        // Deleted in the middle.  Make the reference shorter.
                                        rRef.Ref2.IncCol(rCxt.mnColDelta);
                                    else
                                        // Deleted at tail end.  Cut off the right part.
                                        rRef.Ref2.SetAbsCol(aDeleted.aStart.Col()-1);
                                }
                            }
                            else
                            {
                                // Deleted at the left.  Cut the left off and shift left.
                                rRef.Ref1.SetAbsCol(aDeleted.aEnd.Col()+1);
                                rRef.Ref1.IncCol(rCxt.mnColDelta);
                                if (!aAbs.IsEndColSticky(rCxt.mrDoc))
                                    rRef.Ref2.IncCol(rCxt.mnColDelta);
                            }

                            aRes.mbReferenceModified = true;
                        }
                        else if (rCxt.maRange.Intersects(aAbs))
                        {
                            if (rCxt.mnColDelta && rCxt.maRange.aStart.Row() <= aAbs.aStart.Row() && aAbs.aEnd.Row() <= rCxt.maRange.aEnd.Row())
                            {
                                if (adjustDoubleRefInName(rRef, rCxt, rPos))
                                    aRes.mbReferenceModified = true;
                            }
                            if (rCxt.mnRowDelta && rCxt.maRange.aStart.Col() <= aAbs.aStart.Col() && aAbs.aEnd.Col() <= rCxt.maRange.aEnd.Col())
                            {
                                if (adjustDoubleRefInName(rRef, rCxt, rPos))
                                    aRes.mbReferenceModified = true;
                            }
                        }
                        else if (rCxt.mnRowDelta > 0 && rCxt.mrDoc.IsExpandRefs())
                        {
                            // Check if we could expand range reference by the bottom
                            // edge. For named expressions, we only expand absolute
                            // references. Reference must be at least two rows
                            // tall.
                            if (!rRef.Ref1.IsRowRel() && !rRef.Ref2.IsRowRel() &&
                                    aAbs.aStart.Row() < aAbs.aEnd.Row() &&
                                    aAbs.aEnd.Row()+1 == rCxt.maRange.aStart.Row())
                            {
                                // Expand by the bottom edge.
                                rRef.Ref2.IncRow(rCxt.mnRowDelta);
                                aRes.mbReferenceModified = true;
                            }
                        }
                        else if (rCxt.mnColDelta > 0 && rCxt.mrDoc.IsExpandRefs())
                        {
                            // Check if we could expand range reference by the right
                            // edge. For named expressions, we only expand absolute
                            // references. Reference must be at least two
                            // columns wide.
                            if (!rRef.Ref1.IsColRel() && !rRef.Ref2.IsColRel() &&
                                    aAbs.aStart.Col() < aAbs.aEnd.Col() &&
                                    aAbs.aEnd.Col()+1 == rCxt.maRange.aStart.Col())
                            {
                                // Expand by the right edge.
                                rRef.Ref2.IncCol(rCxt.mnColDelta);
                                aRes.mbReferenceModified = true;
                            }
                        }
                    }
                    break;
                default:
                    ;
            }
        }
    }

    return aRes;
}

sc::RefUpdateResult ScTokenArray::AdjustReferenceInMovedName( const sc::RefUpdateContext& rCxt, const ScAddress& rPos )
{
    // When moving, the range is the destination range.
    ScRange aOldRange = rCxt.maRange;
    ScRange aErrorMoveRange( ScAddress::UNINITIALIZED );
    if (!aOldRange.Move(-rCxt.mnColDelta, -rCxt.mnRowDelta, -rCxt.mnTabDelta, aErrorMoveRange, rCxt.mrDoc))
    {
        assert(!"can't move");
    }

    // In a named expression, we'll move the reference only when the reference
    // is entirely absolute.

    sc::RefUpdateResult aRes;

    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken** pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken** pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch (p->GetType())
            {
                case svSingleRef:
                    {
                        ScSingleRefData& rRef = *p->GetSingleRef();
                        if (rRef.IsColRel() || rRef.IsRowRel() || rRef.IsTabRel())
                            continue;

                        ScAddress aAbs = rRef.toAbs(rCxt.mrDoc, rPos);

                        // Do not update the reference in transposed case (cut paste transposed).
                        // The reference will be updated in UpdateTranspose().
                        if (rCxt.mbTransposed && aOldRange.Contains(aAbs))
                            break;

                        if (aOldRange.Contains(aAbs))
                        {
                            ScAddress aErrorPos( ScAddress::UNINITIALIZED );
                            if (!aAbs.Move(rCxt.mnColDelta, rCxt.mnRowDelta, rCxt.mnTabDelta, aErrorPos, rCxt.mrDoc))
                                aAbs = aErrorPos;
                            aRes.mbReferenceModified = true;
                        }

                        rRef.SetAddress(rCxt.mrDoc.GetSheetLimits(), aAbs, rPos);
                    }
                    break;
                case svDoubleRef:
                    {
                        ScComplexRefData& rRef = *p->GetDoubleRef();
                        if (rRef.Ref1.IsColRel() || rRef.Ref1.IsRowRel() || rRef.Ref1.IsTabRel() ||
                                rRef.Ref2.IsColRel() || rRef.Ref2.IsRowRel() || rRef.Ref2.IsTabRel())
                            continue;

                        ScRange aAbs = rRef.toAbs(rCxt.mrDoc, rPos);

                        // Do not update the reference in transposed case (cut paste transposed).
                        // The reference will be updated in UpdateTranspose().
                        if (rCxt.mbTransposed && aOldRange.Contains(aAbs))
                            break;

                        if (aOldRange.Contains(aAbs))
                        {
                            ScRange aErrorRange( ScAddress::UNINITIALIZED );
                            if (!aAbs.Move(rCxt.mnColDelta, rCxt.mnRowDelta, rCxt.mnTabDelta, aErrorRange, rCxt.mrDoc))
                                aAbs = aErrorRange;
                            aRes.mbReferenceModified = true;
                        }

                        rRef.SetRange(rCxt.mrDoc.GetSheetLimits(), aAbs, rPos);
                    }
                    break;
                default:
                    ;
            }
        }
    }

    return aRes;
}

namespace {

bool adjustSingleRefOnDeletedTab( const ScSheetLimits& rLimits, ScSingleRefData& rRef, SCTAB nDelPos, SCTAB nSheets, const ScAddress& rOldPos, const ScAddress& rNewPos )
{
    ScAddress aAbs = rRef.toAbs(rLimits, rOldPos);
    if (nDelPos <= aAbs.Tab() && aAbs.Tab() < nDelPos + nSheets)
    {
        rRef.SetTabDeleted(true);
        return true;
    }

    if (nDelPos < aAbs.Tab())
    {
        // Reference sheet needs to be adjusted.
        aAbs.IncTab(-1*nSheets);
        rRef.SetAddress(rLimits, aAbs, rNewPos);
        return true;
    }
    else if (rOldPos.Tab() != rNewPos.Tab())
    {
        // Cell itself has moved.
        rRef.SetAddress(rLimits, aAbs, rNewPos);
        return true;
    }

    return false;
}

bool adjustSingleRefOnInsertedTab( const ScSheetLimits& rLimits, ScSingleRefData& rRef, SCTAB nInsPos, SCTAB nSheets, const ScAddress& rOldPos, const ScAddress& rNewPos )
{
    ScAddress aAbs = rRef.toAbs(rLimits, rOldPos);
    if (nInsPos <= aAbs.Tab())
    {
        // Reference sheet needs to be adjusted.
        aAbs.IncTab(nSheets);
        rRef.SetAddress(rLimits, aAbs, rNewPos);
        return true;
    }
    else if (rOldPos.Tab() != rNewPos.Tab())
    {
        // Cell itself has moved.
        rRef.SetAddress(rLimits, aAbs, rNewPos);
        return true;
    }

    return false;
}

bool adjustDoubleRefOnDeleteTab(const ScSheetLimits& rLimits, ScComplexRefData& rRef, SCTAB nDelPos, SCTAB nSheets, const ScAddress& rOldPos, const ScAddress& rNewPos)
{
    ScSingleRefData& rRef1 = rRef.Ref1;
    ScSingleRefData& rRef2 = rRef.Ref2;
    ScAddress aStartPos = rRef1.toAbs(rLimits, rOldPos);
    ScAddress aEndPos = rRef2.toAbs(rLimits, rOldPos);
    bool bMoreThanOneTab = aStartPos.Tab() != aEndPos.Tab();
    bool bModified = false;
    if (bMoreThanOneTab && aStartPos.Tab() == nDelPos && nDelPos + nSheets <= aEndPos.Tab())
    {
        if (rRef1.IsTabRel() && aStartPos.Tab() < rOldPos.Tab())
        {
            rRef1.IncTab(nSheets);
            bModified = true;
        }
    }
    else
    {
        bModified = adjustSingleRefOnDeletedTab(rLimits, rRef1, nDelPos, nSheets, rOldPos, rNewPos);
    }

    if (bMoreThanOneTab && aEndPos.Tab() == nDelPos && aStartPos.Tab() <= nDelPos - nSheets)
    {
        if (!rRef2.IsTabRel() || rOldPos.Tab() < aEndPos.Tab())
        {
            rRef2.IncTab(-nSheets);
            bModified = true;
        }
    }
    else
    {
        bModified |= adjustSingleRefOnDeletedTab(rLimits, rRef2, nDelPos, nSheets, rOldPos, rNewPos);
    }
    return bModified;
}

}

sc::RefUpdateResult ScTokenArray::AdjustReferenceOnDeletedTab( const sc::RefUpdateDeleteTabContext& rCxt, const ScAddress& rOldPos )
{
    sc::RefUpdateResult aRes;
    ScAddress aNewPos = rOldPos;
    ScRangeUpdater::UpdateDeleteTab( aNewPos, rCxt);

    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken** pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken** pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch (p->GetType())
            {
                case svSingleRef:
                    {
                        ScSingleRefData& rRef = *p->GetSingleRef();
                        if (adjustSingleRefOnDeletedTab(*mxSheetLimits, rRef, rCxt.mnDeletePos, rCxt.mnSheets, rOldPos, aNewPos))
                            aRes.mbReferenceModified = true;
                    }
                    break;
                case svDoubleRef:
                    {
                        ScComplexRefData& rRef = *p->GetDoubleRef();
                        aRes.mbReferenceModified |= adjustDoubleRefOnDeleteTab(*mxSheetLimits, rRef, rCxt.mnDeletePos, rCxt.mnSheets, rOldPos, aNewPos);
                    }
                    break;
                default:
                    ;
            }

            // For ocTableRef p is the inner token of *pp, so have a separate
            // condition here.
            if ((*pp)->GetType() == svIndex)
            {
                switch ((*pp)->GetOpCode())
                {
                    case ocName:
                        {
                            SCTAB nOldTab = (*pp)->GetSheet();
                            if (isNameModified(rCxt.maUpdatedNames, nOldTab, **pp))
                                aRes.mbNameModified = true;
                            if (rCxt.mnDeletePos <= nOldTab)
                            {
                                aRes.mbNameModified = true;
                                if (rCxt.mnDeletePos + rCxt.mnSheets <= nOldTab)
                                    (*pp)->SetSheet( nOldTab - rCxt.mnSheets);
                                else
                                    // Would point to a deleted sheet. Invalidate.
                                    (*pp)->SetSheet( SCTAB_MAX);
                            }
                        }
                        break;
                    case ocDBArea:
                    case ocTableRef:
                        if (isDBDataModified(rCxt.mrDoc, **pp))
                            aRes.mbNameModified = true;
                        break;
                    default:
                        ;   // nothing
                }
            }
        }
    }
    return aRes;
}

sc::RefUpdateResult ScTokenArray::AdjustReferenceOnInsertedTab( const sc::RefUpdateInsertTabContext& rCxt, const ScAddress& rOldPos )
{
    sc::RefUpdateResult aRes;
    ScAddress aNewPos = rOldPos;
    if (rCxt.mnInsertPos <= rOldPos.Tab())
        aNewPos.IncTab(rCxt.mnSheets);

    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken** pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken** pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch (p->GetType())
            {
                case svSingleRef:
                    {
                        ScSingleRefData& rRef = *p->GetSingleRef();
                        if (adjustSingleRefOnInsertedTab(*mxSheetLimits, rRef, rCxt.mnInsertPos, rCxt.mnSheets, rOldPos, aNewPos))
                            aRes.mbReferenceModified = true;
                    }
                    break;
                case svDoubleRef:
                    {
                        ScComplexRefData& rRef = *p->GetDoubleRef();
                        if (adjustSingleRefOnInsertedTab(*mxSheetLimits, rRef.Ref1, rCxt.mnInsertPos, rCxt.mnSheets, rOldPos, aNewPos))
                            aRes.mbReferenceModified = true;
                        if (adjustSingleRefOnInsertedTab(*mxSheetLimits, rRef.Ref2, rCxt.mnInsertPos, rCxt.mnSheets, rOldPos, aNewPos))
                            aRes.mbReferenceModified = true;
                    }
                    break;
                default:
                    ;
            }

            // For ocTableRef p is the inner token of *pp, so have a separate
            // condition here.
            if ((*pp)->GetType() == svIndex)
            {
                switch ((*pp)->GetOpCode())
                {
                    case ocName:
                        {
                            SCTAB nOldTab = (*pp)->GetSheet();
                            if (isNameModified(rCxt.maUpdatedNames, nOldTab, **pp))
                                aRes.mbNameModified = true;
                            if (rCxt.mnInsertPos <= nOldTab)
                            {
                                aRes.mbNameModified = true;
                                (*pp)->SetSheet( nOldTab + rCxt.mnSheets);
                            }
                        }
                        break;
                    case ocDBArea:
                    case ocTableRef:
                        if (isDBDataModified(rCxt.mrDoc, **pp))
                            aRes.mbNameModified = true;
                        break;
                    default:
                        ;   // nothing
                }
            }
        }
    }
    return aRes;
}

namespace {

bool adjustTabOnMove( ScAddress& rPos, const sc::RefUpdateMoveTabContext& rCxt )
{
    SCTAB nNewTab = rCxt.getNewTab(rPos.Tab());
    if (nNewTab == rPos.Tab())
        return false;

    rPos.SetTab(nNewTab);
    return true;
}

}

sc::RefUpdateResult ScTokenArray::AdjustReferenceOnMovedTab( const sc::RefUpdateMoveTabContext& rCxt, const ScAddress& rOldPos )
{
    sc::RefUpdateResult aRes;
    if (rCxt.mnOldPos == rCxt.mnNewPos)
        return aRes;

    ScAddress aNewPos = rOldPos;
    if (adjustTabOnMove(aNewPos, rCxt))
    {
        aRes.mbReferenceModified = true;
        aRes.mbValueChanged = true;
        aRes.mnTab = aNewPos.Tab(); // this sets the new tab position used when deleting
    }

    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken** pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken** pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch (p->GetType())
            {
                case svSingleRef:
                    {
                        ScSingleRefData& rRef = *p->GetSingleRef();
                        ScAddress aAbs = rRef.toAbs(*mxSheetLimits, rOldPos);
                        if (adjustTabOnMove(aAbs, rCxt))
                            aRes.mbReferenceModified = true;
                        rRef.SetAddress(*mxSheetLimits, aAbs, aNewPos);
                    }
                    break;
                case svDoubleRef:
                    {
                        ScComplexRefData& rRef = *p->GetDoubleRef();
                        ScRange aAbs = rRef.toAbs(*mxSheetLimits, rOldPos);
                        if (adjustTabOnMove(aAbs.aStart, rCxt))
                            aRes.mbReferenceModified = true;
                        if (adjustTabOnMove(aAbs.aEnd, rCxt))
                            aRes.mbReferenceModified = true;
                        rRef.SetRange(*mxSheetLimits, aAbs, aNewPos);
                    }
                    break;
                default:
                    ;
            }

            // For ocTableRef p is the inner token of *pp, so have a separate
            // condition here.
            if ((*pp)->GetType() == svIndex)
            {
                switch ((*pp)->GetOpCode())
                {
                    case ocName:
                        {
                            SCTAB nOldTab = (*pp)->GetSheet();
                            if (isNameModified(rCxt.maUpdatedNames, nOldTab, **pp))
                                aRes.mbNameModified = true;
                            SCTAB nNewTab = rCxt.getNewTab( nOldTab);
                            if (nNewTab != nOldTab)
                            {
                                aRes.mbNameModified = true;
                                (*pp)->SetSheet( nNewTab);
                            }
                        }
                        break;
                    case ocDBArea:
                    case ocTableRef:
                        if (isDBDataModified(rCxt.mrDoc, **pp))
                            aRes.mbNameModified = true;
                        break;
                    default:
                        ;   // nothing
                }
            }
        }
    }

    return aRes;
}

void ScTokenArray::AdjustReferenceOnMovedOrigin( const ScAddress& rOldPos, const ScAddress& rNewPos )
{
    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken** pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken** pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch (p->GetType())
            {
                case svSingleRef:
                case svExternalSingleRef:
                    {
                        ScSingleRefData& rRef = *p->GetSingleRef();
                        ScAddress aAbs = rRef.toAbs(*mxSheetLimits, rOldPos);
                        rRef.SetAddress(*mxSheetLimits, aAbs, rNewPos);
                    }
                    break;
                case svDoubleRef:
                case svExternalDoubleRef:
                    {
                        ScComplexRefData& rRef = *p->GetDoubleRef();
                        ScRange aAbs = rRef.toAbs(*mxSheetLimits, rOldPos);
                        rRef.SetRange(*mxSheetLimits, aAbs, rNewPos);
                    }
                    break;
                default:
                    ;
            }
        }
    }
}

void ScTokenArray::AdjustReferenceOnMovedOriginIfOtherSheet( const ScAddress& rOldPos, const ScAddress& rNewPos )
{
    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken** pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken** pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            bool bAdjust = false;
            switch (p->GetType())
            {
                case svExternalSingleRef:
                    bAdjust = true;     // always
                    [[fallthrough]];
                case svSingleRef:
                    {
                        ScSingleRefData& rRef = *p->GetSingleRef();
                        ScAddress aAbs = rRef.toAbs(*mxSheetLimits, rOldPos);
                        if (!bAdjust)
                            bAdjust = (aAbs.Tab() != rOldPos.Tab());
                        if (bAdjust)
                            rRef.SetAddress(*mxSheetLimits, aAbs, rNewPos);
                    }
                    break;
                case svExternalDoubleRef:
                    bAdjust = true;     // always
                    [[fallthrough]];
                case svDoubleRef:
                    {
                        ScComplexRefData& rRef = *p->GetDoubleRef();
                        ScRange aAbs = rRef.toAbs(*mxSheetLimits, rOldPos);
                        if (!bAdjust)
                            bAdjust = (rOldPos.Tab() < aAbs.aStart.Tab() || aAbs.aEnd.Tab() < rOldPos.Tab());
                        if (bAdjust)
                            rRef.SetRange(*mxSheetLimits, aAbs, rNewPos);
                    }
                    break;
                default:
                    ;
            }
        }
    }
}

void ScTokenArray::AdjustReferenceOnCopy( const ScAddress& rNewPos )
{
    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN, false);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken** pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken** pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch (p->GetType())
            {
                case svDoubleRef:
                    {
                        ScComplexRefData& rRef = *p->GetDoubleRef();
                        rRef.PutInOrder( rNewPos);
                    }
                    break;
                default:
                    ;
            }
        }
    }
}

namespace {

void clearTabDeletedFlag( const ScSheetLimits& rLimits, ScSingleRefData& rRef, const ScAddress& rPos, SCTAB nStartTab, SCTAB nEndTab )
{
    if (!rRef.IsTabDeleted())
        return;

    ScAddress aAbs = rRef.toAbs(rLimits, rPos);
    if (nStartTab <=  aAbs.Tab() && aAbs.Tab() <= nEndTab)
        rRef.SetTabDeleted(false);
}

}

void ScTokenArray::ClearTabDeleted( const ScAddress& rPos, SCTAB nStartTab, SCTAB nEndTab )
{
    if (nEndTab < nStartTab)
        return;

    FormulaToken** p = pCode.get();
    FormulaToken** pEnd = p + static_cast<size_t>(nLen);
    for (; p != pEnd; ++p)
    {
        switch ((*p)->GetType())
        {
            case svSingleRef:
            {
                formula::FormulaToken* pToken = *p;
                ScSingleRefData& rRef = *pToken->GetSingleRef();
                clearTabDeletedFlag(*mxSheetLimits, rRef, rPos, nStartTab, nEndTab);
            }
            break;
            case svDoubleRef:
            {
                formula::FormulaToken* pToken = *p;
                ScComplexRefData& rRef = *pToken->GetDoubleRef();
                clearTabDeletedFlag(*mxSheetLimits, rRef.Ref1, rPos, nStartTab, nEndTab);
                clearTabDeletedFlag(*mxSheetLimits, rRef.Ref2, rPos, nStartTab, nEndTab);
            }
            break;
            default:
                ;
        }
    }
}

namespace {

void checkBounds(
    const ScSheetLimits& rLimits,
    const ScAddress& rPos, SCROW nGroupLen, const ScRange& rCheckRange,
    const ScSingleRefData& rRef, std::vector<SCROW>& rBounds, const ScRange* pDeletedRange )
{
    if (!rRef.IsRowRel())
        return;

    ScRange aAbs(rRef.toAbs(rLimits, rPos));
    aAbs.aEnd.IncRow(nGroupLen-1);
    if (!rCheckRange.Intersects(aAbs) && (!pDeletedRange || !pDeletedRange->Intersects(aAbs)))
        return;

    // Get the boundary row positions.
    if (aAbs.aEnd.Row() < rCheckRange.aStart.Row() && (!pDeletedRange || aAbs.aEnd.Row() < pDeletedRange->aStart.Row()))
        // No intersections.
        return;

    // rCheckRange may be a virtual non-existent row being shifted in.
    if (aAbs.aStart.Row() <= rCheckRange.aStart.Row() && rCheckRange.aStart.Row() < rLimits.GetMaxRowCount())
    {
        //    +-+ <---- top
        //    | |
        // +--+-+--+ <---- boundary row position
        // |  | |  |
        // |       |
        // +-------+

        // Add offset from the reference top to the cell position.
        SCROW nOffset = rCheckRange.aStart.Row() - aAbs.aStart.Row();
        rBounds.push_back(rPos.Row()+nOffset);
    }
    // Same for deleted range.
    if (pDeletedRange && aAbs.aStart.Row() <= pDeletedRange->aStart.Row())
    {
        SCROW nOffset = pDeletedRange->aStart.Row() - aAbs.aStart.Row();
        SCROW nRow = rPos.Row() + nOffset;
        // Unlike for rCheckRange, for pDeletedRange nRow can be anywhere>=0.
        if (rLimits.ValidRow(nRow))
            rBounds.push_back(nRow);
    }

    if (aAbs.aEnd.Row() >= rCheckRange.aEnd.Row())
    {
        // only check for end range

        // +-------+
        // |       |
        // |  | |  |
        // +--+-+--+ <---- boundary row position
        //    | |
        //    +-+

        // Ditto.
        SCROW nOffset = rCheckRange.aEnd.Row() + 1 - aAbs.aStart.Row();
        rBounds.push_back(rPos.Row()+nOffset);
    }
    // Same for deleted range.
    if (pDeletedRange && aAbs.aEnd.Row() >= pDeletedRange->aEnd.Row())
    {
        SCROW nOffset = pDeletedRange->aEnd.Row() + 1 - aAbs.aStart.Row();
        SCROW nRow = rPos.Row() + nOffset;
        // Unlike for rCheckRange, for pDeletedRange nRow can be ~anywhere.
        if (rLimits.ValidRow(nRow))
            rBounds.push_back(nRow);
    }
}

void checkBounds(
    const sc::RefUpdateContext& rCxt, const ScAddress& rPos, SCROW nGroupLen,
    const ScSingleRefData& rRef, std::vector<SCROW>& rBounds)
{
    if (!rRef.IsRowRel())
        return;

    ScRange aDeletedRange( ScAddress::UNINITIALIZED );
    const ScRange* pDeletedRange = nullptr;

    ScRange aCheckRange = rCxt.maRange;
    if (rCxt.meMode == URM_MOVE)
    {
        // Check bounds against the old range prior to the move.
        ScRange aErrorRange( ScAddress::UNINITIALIZED );
        if (!aCheckRange.Move(-rCxt.mnColDelta, -rCxt.mnRowDelta, -rCxt.mnTabDelta, aErrorRange, rCxt.mrDoc))
        {
            assert(!"can't move");
        }

        // Check bounds also against the range moved into.
        pDeletedRange = &rCxt.maRange;
    }
    else if (rCxt.meMode == URM_INSDEL &&
            ((rCxt.mnColDelta < 0 && rCxt.maRange.aStart.Col() > 0) ||
             (rCxt.mnRowDelta < 0 && rCxt.maRange.aStart.Row() > 0)))
    {
        // Check bounds also against deleted range where cells are shifted
        // into and references need to be invalidated.
        aDeletedRange = getSelectedRange( rCxt);
        pDeletedRange = &aDeletedRange;
    }

    checkBounds(rCxt.mrDoc.GetSheetLimits(), rPos, nGroupLen, aCheckRange, rRef, rBounds, pDeletedRange);
}

}

void ScTokenArray::CheckRelativeReferenceBounds(
    const sc::RefUpdateContext& rCxt, const ScAddress& rPos, SCROW nGroupLen, std::vector<SCROW>& rBounds ) const
{
    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken** pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken** pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch (p->GetType())
            {
                case svSingleRef:
                    {
                        checkBounds(rCxt, rPos, nGroupLen, *p->GetSingleRef(), rBounds);
                    }
                    break;
                case svDoubleRef:
                    {
                        const ScComplexRefData& rRef = *p->GetDoubleRef();
                        checkBounds(rCxt, rPos, nGroupLen, rRef.Ref1, rBounds);
                        checkBounds(rCxt, rPos, nGroupLen, rRef.Ref2, rBounds);
                    }
                    break;
                default:
                    ;
            }
        }
    }
}

void ScTokenArray::CheckRelativeReferenceBounds(
    const ScAddress& rPos, SCROW nGroupLen, const ScRange& rRange, std::vector<SCROW>& rBounds ) const
{
    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken** pp = aPtrs.maPointerRange[j].mpStart;
        FormulaToken** pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch (p->GetType())
            {
                case svSingleRef:
                    {
                        const ScSingleRefData& rRef = *p->GetSingleRef();
                        checkBounds(*mxSheetLimits, rPos, nGroupLen, rRange, rRef, rBounds, nullptr);
                    }
                    break;
                case svDoubleRef:
                    {
                        const ScComplexRefData& rRef = *p->GetDoubleRef();
                        checkBounds(*mxSheetLimits, rPos, nGroupLen, rRange, rRef.Ref1, rBounds, nullptr);
                        checkBounds(*mxSheetLimits, rPos, nGroupLen, rRange, rRef.Ref2, rBounds, nullptr);
                    }
                    break;
                default:
                    ;
            }
        }
    }
}

void ScTokenArray::CheckExpandReferenceBounds(
    const sc::RefUpdateContext& rCxt, const ScAddress& rPos, SCROW nGroupLen, std::vector<SCROW>& rBounds ) const
{
    const SCROW nInsRow = rCxt.maRange.aStart.Row();
    TokenPointers aPtrs( pCode.get(), nLen, pRPN, nRPN);
    for (size_t j=0; j<2; ++j)
    {
        FormulaToken* const * pp = aPtrs.maPointerRange[j].mpStart;
        const FormulaToken* const * pEnd = aPtrs.maPointerRange[j].mpStop;
        for (; pp != pEnd; ++pp)
        {
            const FormulaToken* p = aPtrs.getHandledToken(j,pp);
            if (!p)
                continue;

            switch (p->GetType())
            {
                case svDoubleRef:
                    {
                        const ScComplexRefData& rRef = *p->GetDoubleRef();
                        bool bStartRowRelative = rRef.Ref1.IsRowRel();
                        bool bEndRowRelative = rRef.Ref2.IsRowRel();

                        // For absolute references nothing needs to be done, they stay
                        // the same for all and if to be expanded the group will be
                        // adjusted later.
                        if (!bStartRowRelative && !bEndRowRelative)
                            break;  // switch

                        ScRange aAbsStart(rRef.toAbs(*mxSheetLimits, rPos));
                        ScAddress aPos(rPos);
                        aPos.IncRow(nGroupLen);
                        ScRange aAbsEnd(rRef.toAbs(*mxSheetLimits, aPos));
                        // References must be at least two rows to be expandable.
                        if ((aAbsStart.aEnd.Row() - aAbsStart.aStart.Row() < 1) &&
                                (aAbsEnd.aEnd.Row() - aAbsEnd.aStart.Row() < 1))
                            break;  // switch

                        // Only need to process if an edge may be touching the
                        // insertion row anywhere within the run of the group.
                        if (!((aAbsStart.aStart.Row() <= nInsRow && nInsRow <= aAbsEnd.aStart.Row()) ||
                                    (aAbsStart.aEnd.Row() <= nInsRow && nInsRow <= aAbsEnd.aEnd.Row())))
                            break;  // switch

                        SCROW nStartRow = aAbsStart.aStart.Row();
                        SCROW nEndRow = aAbsStart.aEnd.Row();
                        // Position on first relevant range.
                        SCROW nOffset = 0;
                        if (nEndRow + 1 < nInsRow)
                        {
                            if (bEndRowRelative)
                            {
                                nOffset = nInsRow - nEndRow - 1;
                                nEndRow += nOffset;
                                if (bStartRowRelative)
                                    nStartRow += nOffset;
                            }
                            else    // bStartRowRelative==true
                            {
                                nOffset = nInsRow - nStartRow;
                                nStartRow += nOffset;
                                // Start is overtaking End, swap.
                                bStartRowRelative = false;
                                bEndRowRelative = true;
                            }
                        }
                        for (SCROW i = nOffset; i < nGroupLen; ++i)
                        {
                            bool bSplit = (nStartRow == nInsRow || nEndRow + 1 == nInsRow);
                            if (bSplit)
                                rBounds.push_back( rPos.Row() + i);

                            if (bEndRowRelative)
                                ++nEndRow;
                            if (bStartRowRelative)
                            {
                                ++nStartRow;
                                if (!bEndRowRelative && nStartRow == nEndRow)
                                {
                                    // Start is overtaking End, swap.
                                    bStartRowRelative = false;
                                    bEndRowRelative = true;
                                }
                            }
                            if (nInsRow < nStartRow || (!bStartRowRelative && nInsRow <= nEndRow))
                            {
                                if (bSplit && (++i < nGroupLen))
                                    rBounds.push_back( rPos.Row() + i);
                                break;  // for, out of range now
                            }
                        }
                    }
                    break;
                default:
                    ;
            }
        }
    }
}

namespace {

void appendDouble( const sc::TokenStringContext& rCxt, OUStringBuffer& rBuf, double fVal )
{
    if (rCxt.mxOpCodeMap->isEnglish())
    {
        rtl::math::doubleToUStringBuffer(
            rBuf, fVal, rtl_math_StringFormat_Automatic, rtl_math_DecimalPlaces_Max, '.', true);
    }
    else
    {
        SvtSysLocale aSysLocale;
        rtl::math::doubleToUStringBuffer(
            rBuf, fVal,
            rtl_math_StringFormat_Automatic, rtl_math_DecimalPlaces_Max,
            aSysLocale.GetLocaleData().getNumDecimalSep()[0], true);
    }
}

void appendString( OUStringBuffer& rBuf, const OUString& rStr )
{
    rBuf.append('"');
    rBuf.append(rStr.replaceAll("\"", "\"\""));
    rBuf.append('"');
}

void appendTokenByType( ScSheetLimits& rLimits, sc::TokenStringContext& rCxt, OUStringBuffer& rBuf, const FormulaToken& rToken,
        const ScAddress& rPos, bool bFromRangeName )
{
    if (rToken.IsExternalRef())
    {
        size_t nFileId = rToken.GetIndex();
        OUString aTabName = rToken.GetString().getString();
        if (nFileId >= rCxt.maExternalFileNames.size())
            // out of bound
            return;

        OUString aFileName = rCxt.maExternalFileNames[nFileId];

        switch (rToken.GetType())
        {
            case svExternalName:
                rBuf.append(rCxt.mpRefConv->makeExternalNameStr(nFileId, aFileName, aTabName));
            break;
            case svExternalSingleRef:
                rCxt.mpRefConv->makeExternalRefStr(
                       rLimits, rBuf, rPos, nFileId, aFileName, aTabName, *rToken.GetSingleRef());
            break;
            case svExternalDoubleRef:
            {
                sc::TokenStringContext::IndexNamesMapType::const_iterator it =
                    rCxt.maExternalCachedTabNames.find(nFileId);

                if (it == rCxt.maExternalCachedTabNames.end())
                    return;

                rCxt.mpRefConv->makeExternalRefStr(
                        rLimits, rBuf, rPos, nFileId, aFileName, it->second, aTabName,
                        *rToken.GetDoubleRef());
            }
            break;
            default:
                // warning, not error, otherwise we may end up with a never
                // ending message box loop if this was the cursor cell to be redrawn.
                OSL_FAIL("appendTokenByType: unknown type of ocExternalRef");
        }
        return;
    }

    OpCode eOp = rToken.GetOpCode();
    switch (rToken.GetType())
    {
        case svDouble:
            appendDouble(rCxt, rBuf, rToken.GetDouble());
        break;
        case svString:
        {
            OUString aStr = rToken.GetString().getString();
            if (eOp == ocBad || eOp == ocStringXML || eOp == ocStringName)
            {
                rBuf.append(aStr);
                return;
            }

            appendString(rBuf, aStr);
        }
        break;
        case svSingleRef:
        {
            if (rCxt.mpRefConv)
            {
                const ScSingleRefData& rRef = *rToken.GetSingleRef();
                ScComplexRefData aRef;
                aRef.Ref1 = rRef;
                aRef.Ref2 = rRef;
                rCxt.mpRefConv->makeRefStr(rLimits, rBuf, rCxt.meGram, rPos, rCxt.maErrRef, rCxt.maTabNames, aRef, true,
                        bFromRangeName);
            }
            else
                rBuf.append(rCxt.maErrRef);
        }
        break;
        case svDoubleRef:
        {
            if (rCxt.mpRefConv)
            {
                const ScComplexRefData& rRef = *rToken.GetDoubleRef();
                rCxt.mpRefConv->makeRefStr(rLimits, rBuf, rCxt.meGram, rPos, rCxt.maErrRef, rCxt.maTabNames, rRef, false,
                        bFromRangeName);
            }
            else
                rBuf.append(rCxt.maErrRef);
        }
        break;
        case svMatrix:
        {
            const ScMatrix* pMat = rToken.GetMatrix();
            if (!pMat)
                return;

            size_t nC, nMaxC, nR, nMaxR;
            pMat->GetDimensions(nMaxC, nMaxR);

            rBuf.append(rCxt.mxOpCodeMap->getSymbol(ocArrayOpen));
            for (nR = 0 ; nR < nMaxR ; ++nR)
            {
                if (nR > 0)
                {
                    rBuf.append(rCxt.mxOpCodeMap->getSymbol(ocArrayRowSep));
                }

                for (nC = 0 ; nC < nMaxC ; ++nC)
                {
                    if (nC > 0)
                    {
                        rBuf.append(rCxt.mxOpCodeMap->getSymbol(ocArrayColSep));
                    }

                    if (pMat->IsValue(nC, nR))
                    {
                        if (pMat->IsBoolean(nC, nR))
                        {
                            bool bVal = pMat->GetDouble(nC, nR) != 0.0;
                            rBuf.append(rCxt.mxOpCodeMap->getSymbol(bVal ? ocTrue : ocFalse));
                        }
                        else
                        {
                            FormulaError nErr = pMat->GetError(nC, nR);
                            if (nErr != FormulaError::NONE)
                                rBuf.append(ScGlobal::GetErrorString(nErr));
                            else
                                appendDouble(rCxt, rBuf, pMat->GetDouble(nC, nR));
                        }
                    }
                    else if (pMat->IsEmpty(nC, nR))
                    {
                        // Skip it.
                    }
                    else if (pMat->IsStringOrEmpty(nC, nR))
                        appendString(rBuf, pMat->GetString(nC, nR).getString());
                }
            }
            rBuf.append(rCxt.mxOpCodeMap->getSymbol(ocArrayClose));
        }
        break;
        case svIndex:
        {
            typedef sc::TokenStringContext::IndexNameMapType NameType;

            sal_uInt16 nIndex = rToken.GetIndex();
            switch (eOp)
            {
                case ocName:
                {
                    SCTAB nTab = rToken.GetSheet();
                    if (nTab < 0)
                    {
                        // global named range
                        NameType::const_iterator it = rCxt.maGlobalRangeNames.find(nIndex);
                        if (it == rCxt.maGlobalRangeNames.end())
                        {
                            rBuf.append(ScCompiler::GetNativeSymbol(ocErrName));
                            break;
                        }

                        rBuf.append(it->second);
                    }
                    else
                    {
                        // sheet-local named range
                        if (nTab != rPos.Tab())
                        {
                            // On other sheet.
                            OUString aName;
                            if (o3tl::make_unsigned(nTab) < rCxt.maTabNames.size())
                                aName = rCxt.maTabNames[nTab];
                            if (!aName.isEmpty())
                            {
                                ScCompiler::CheckTabQuotes( aName, rCxt.mpRefConv->meConv);
                                rBuf.append( aName);
                            }
                            else
                                rBuf.append(ScCompiler::GetNativeSymbol(ocErrName));
                            rBuf.append( rCxt.mpRefConv->getSpecialSymbol( ScCompiler::Convention::SHEET_SEPARATOR));
                        }

                        sc::TokenStringContext::TabIndexMapType::const_iterator itTab = rCxt.maSheetRangeNames.find(nTab);
                        if (itTab == rCxt.maSheetRangeNames.end())
                        {
                            rBuf.append(ScCompiler::GetNativeSymbol(ocErrName));
                            break;
                        }

                        const NameType& rNames = itTab->second;
                        NameType::const_iterator it = rNames.find(nIndex);
                        if (it == rNames.end())
                        {
                            rBuf.append(ScCompiler::GetNativeSymbol(ocErrName));
                            break;
                        }

                        rBuf.append(it->second);
                    }
                }
                break;
                case ocDBArea:
                case ocTableRef:
                {
                    NameType::const_iterator it = rCxt.maNamedDBs.find(nIndex);
                    if (it != rCxt.maNamedDBs.end())
                        rBuf.append(it->second);
                }
                break;
                default:
                    rBuf.append(ScCompiler::GetNativeSymbol(ocErrName));
            }
        }
        break;
        case svExternal:
        {
            // mapped or translated name of AddIns
            OUString aAddIn = rToken.GetExternal();
            bool bMapped = rCxt.mxOpCodeMap->isPODF();     // ODF 1.1 directly uses programmatical name
            if (!bMapped && rCxt.mxOpCodeMap->hasExternals())
            {
                const ExternalHashMap& rExtMap = rCxt.mxOpCodeMap->getReverseExternalHashMap();
                ExternalHashMap::const_iterator it = rExtMap.find(aAddIn);
                if (it != rExtMap.end())
                {
                    aAddIn = it->second;
                    bMapped = true;
                }
            }

            if (!bMapped && !rCxt.mxOpCodeMap->isEnglish())
                ScGlobal::GetAddInCollection()->LocalizeString(aAddIn);

            rBuf.append(aAddIn);
        }
        break;
        case svError:
        {
            FormulaError nErr = rToken.GetError();
            OpCode eOpErr;
            switch (nErr)
            {
                case FormulaError::DivisionByZero:
                    eOpErr = ocErrDivZero;
                break;
                case FormulaError::NoValue:
                    eOpErr = ocErrValue;
                break;
                case FormulaError::NoRef:
                    eOpErr = ocErrRef;
                break;
                case FormulaError::NoName:
                    eOpErr = ocErrName;
                break;
                case FormulaError::IllegalFPOperation:
                    eOpErr = ocErrNum;
                break;
                case FormulaError::NotAvailable:
                    eOpErr = ocErrNA;
                break;
                case FormulaError::NoCode:
                default:
                    eOpErr = ocErrNull;
            }
            rBuf.append(rCxt.mxOpCodeMap->getSymbol(eOpErr));
        }
        break;
        case svByte:
        case svJump:
        case svFAP:
        case svMissing:
        case svSep:
        default:
            ;
    }
}

}

OUString ScTokenArray::CreateString( sc::TokenStringContext& rCxt, const ScAddress& rPos ) const
{
    if (!nLen)
        return OUString();

    OUStringBuffer aBuf;

    FormulaToken** p = pCode.get();
    FormulaToken** pEnd = p + static_cast<size_t>(nLen);
    for (; p != pEnd; ++p)
    {
        const FormulaToken* pToken = *p;
        OpCode eOp = pToken->GetOpCode();
        /* FIXME: why does this ignore the count of spaces? */
        if (eOp == ocSpaces)
        {
            // TODO : Handle intersection operator '!!'.
            aBuf.append(' ');
            continue;
        }
        else if (eOp == ocWhitespace)
        {
            aBuf.append( pToken->GetChar());
            continue;
        }

        if (eOp < rCxt.mxOpCodeMap->getSymbolCount())
            aBuf.append(rCxt.mxOpCodeMap->getSymbol(eOp));

        appendTokenByType(*mxSheetLimits, rCxt, aBuf, *pToken, rPos, IsFromRangeName());
    }

    return aBuf.makeStringAndClear();
}

namespace {

void wrapAddress( ScAddress& rPos, SCCOL nMaxCol, SCROW nMaxRow )
{
    if (rPos.Col() > nMaxCol)
        rPos.SetCol(rPos.Col() % (nMaxCol+1));
    if (rPos.Row() > nMaxRow)
        rPos.SetRow(rPos.Row() % (nMaxRow+1));
}

template<typename T> void wrapRange( T& n1, T& n2, T nMax )
{
    if (n2 > nMax)
    {
        if (n1 == 0)
            n2 = nMax;  // Truncate to full range instead of wrapping to a weird range.
        else
            n2 = n2 % (nMax+1);
    }
    if (n1 > nMax)
        n1 = n1 % (nMax+1);
}

void wrapColRange( ScRange& rRange, SCCOL nMaxCol )
{
    SCCOL nCol1 = rRange.aStart.Col();
    SCCOL nCol2 = rRange.aEnd.Col();
    wrapRange( nCol1, nCol2, nMaxCol);
    rRange.aStart.SetCol( nCol1);
    rRange.aEnd.SetCol( nCol2);
}

void wrapRowRange( ScRange& rRange, SCROW nMaxRow )
{
    SCROW nRow1 = rRange.aStart.Row();
    SCROW nRow2 = rRange.aEnd.Row();
    wrapRange( nRow1, nRow2, nMaxRow);
    rRange.aStart.SetRow( nRow1);
    rRange.aEnd.SetRow( nRow2);
}

}

void ScTokenArray::WrapReference( const ScAddress& rPos, SCCOL nMaxCol, SCROW nMaxRow )
{
    FormulaToken** p = pCode.get();
    FormulaToken** pEnd = p + static_cast<size_t>(nLen);
    for (; p != pEnd; ++p)
    {
        switch ((*p)->GetType())
        {
            case svSingleRef:
            {
                formula::FormulaToken* pToken = *p;
                ScSingleRefData& rRef = *pToken->GetSingleRef();
                ScAddress aAbs = rRef.toAbs(*mxSheetLimits, rPos);
                wrapAddress(aAbs, nMaxCol, nMaxRow);
                rRef.SetAddress(*mxSheetLimits, aAbs, rPos);
            }
            break;
            case svDoubleRef:
            {
                formula::FormulaToken* pToken = *p;
                ScComplexRefData& rRef = *pToken->GetDoubleRef();
                ScRange aAbs = rRef.toAbs(*mxSheetLimits, rPos);
                // Entire columns/rows are sticky.
                if (!rRef.IsEntireCol(*mxSheetLimits) && !rRef.IsEntireRow(*mxSheetLimits))
                {
                    wrapColRange( aAbs, nMaxCol);
                    wrapRowRange( aAbs, nMaxRow);
                }
                else if (rRef.IsEntireCol(*mxSheetLimits) && !rRef.IsEntireRow(*mxSheetLimits))
                    wrapColRange( aAbs, nMaxCol);
                else if (!rRef.IsEntireCol(*mxSheetLimits) && rRef.IsEntireRow(*mxSheetLimits))
                    wrapRowRange( aAbs, nMaxRow);
                // else nothing if both, column and row, are entire.
                aAbs.PutInOrder();
                rRef.SetRange(*mxSheetLimits, aAbs, rPos);
            }
            break;
            default:
                ;
        }
    }
}

sal_Int32 ScTokenArray::GetWeight() const
{
    sal_Int32 nResult = 0;
    for (auto i = 0; i < nRPN; ++i)
    {
        switch ((*pRPN[i]).GetType())
        {
        case svDoubleRef:
            {
                const auto pComplexRef = (*pRPN[i]).GetDoubleRef();

                // Number of cells referenced divided by 10.
                const double nRows = 1 + (pComplexRef->Ref2.Row() - pComplexRef->Ref1.Row());
                const double nCols = 1 + (pComplexRef->Ref2.Col() - pComplexRef->Ref1.Col());
                const double nNumCellsTerm = nRows * nCols / 10.0;

                if (nNumCellsTerm + nResult < SAL_MAX_INT32)
                    nResult += nNumCellsTerm;
                else
                    nResult = SAL_MAX_INT32;
            }
            break;
        default:
            ;
        }
    }

    if (nResult == 0)
        nResult = 1;

    return nResult;
}

#if DEBUG_FORMULA_COMPILER

void ScTokenArray::Dump() const
{
    cout << "+++ Normal Tokens +++" << endl;
    for (sal_uInt16 i = 0; i < nLen; ++i)
    {
        DumpToken(*pCode[i]);
    }

    cout << "+++ RPN Tokens +++" << endl;
    for (sal_uInt16 i = 0; i < nRPN; ++i)
    {
        DumpToken(*pRPN[i]);
    }
}
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
