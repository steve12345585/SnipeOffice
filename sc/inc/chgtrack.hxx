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

#include <map>
#include <memory>
#include <set>
#include <stack>
#include <vector>

#include <com/sun/star/uno/Sequence.hxx>
#include <tools/color.hxx>
#include <tools/datetime.hxx>
#include <tools/link.hxx>
#include <tools/solar.h>
#include <unotools/options.hxx>
#include <optional>
#include "global.hxx"
#include "bigrange.hxx"
#include "scdllapi.h"
#include "cellvalue.hxx"

class ScDocument;
class ScFormulaCell;
class ScChangeAction;
class ScChangeTrack;
class ScAppOptions;
namespace tools { class JsonWriter; }

class ScActionColorChanger
{
private:
    const ScAppOptions&     rOpt;
    const std::set<OUString>& rUsers;
    OUString                aLastUserName;
    sal_uInt16              nLastUserIndex;
    Color                   nColor;

public:
    ScActionColorChanger( const ScChangeTrack& rTrack );
    void        Update( const ScChangeAction& rAction );
    Color       GetColor() const { return nColor; }
};

enum ScChangeActionType
{
    SC_CAT_NONE,
    SC_CAT_INSERT_COLS,
    SC_CAT_INSERT_ROWS,
    SC_CAT_INSERT_TABS,
    SC_CAT_DELETE_COLS,
    SC_CAT_DELETE_ROWS,
    SC_CAT_DELETE_TABS,
    SC_CAT_MOVE,
    SC_CAT_CONTENT,
    SC_CAT_REJECT
};

enum ScChangeActionState
{
    SC_CAS_VIRGIN,
    SC_CAS_ACCEPTED,
    SC_CAS_REJECTED
};

enum ScChangeActionClipMode
{
    SC_CACM_NONE,
    SC_CACM_CUT,
    SC_CACM_PASTE
};

/** A link/connection/dependency between change actions.

    Upon construction inserts itself as the head of a chain / linked list,
    respectively between existing link entries.

    Upon destruction removes itself from the list and connects the previous and
    next entry, if it was the first entry automatically maintaining the head
    pointer to the list.

    ppPrev == &previous->pNext or address of pointer to head of linked list,
    *ppPrev == this
 */
class ScChangeActionLinkEntry
{
    ScChangeActionLinkEntry( const ScChangeActionLinkEntry& ) = delete;
    ScChangeActionLinkEntry& operator=( const ScChangeActionLinkEntry& ) = delete;

    ScChangeActionLinkEntry*    pNext;
    ScChangeActionLinkEntry**   ppPrev;
    ScChangeAction*             pAction;
    ScChangeActionLinkEntry*    pLink;

public:

    ScChangeActionLinkEntry(
            ScChangeActionLinkEntry** ppPrevP,
            ScChangeAction* pActionP )
        :   pNext( *ppPrevP ),
            ppPrev( ppPrevP ),
            pAction( pActionP ),
            pLink( nullptr )
        {
            if ( pNext )
                pNext->ppPrev = &pNext;
            *ppPrevP = this;
        }

    virtual ~ScChangeActionLinkEntry()
    {
        ScChangeActionLinkEntry* p = pLink;
        UnLink();
        Remove();
        if ( p )
            delete p;
    }

    void SetLink( ScChangeActionLinkEntry* pLinkP )
    {
        UnLink();
        if ( pLinkP )
        {
            pLink = pLinkP;
            pLinkP->pLink = this;
        }
    }

    void UnLink()
    {
        if ( pLink )
        {
            pLink->pLink = nullptr;
            pLink = nullptr;
        }
    }

    void Remove()
    {
        if ( ppPrev )
        {
            if ( ( *ppPrev = pNext ) != nullptr )
                pNext->ppPrev = ppPrev;
            ppPrev = nullptr;  // not inserted
        }
    }

    const ScChangeActionLinkEntry*  GetNext() const     { return pNext; }
    ScChangeActionLinkEntry*        GetNext()           { return pNext; }
    const ScChangeAction*           GetAction() const   { return pAction; }
    ScChangeAction*                 GetAction()         { return pAction; }
};

// ScChangeActionCellListEntry
// this is only for the XML Export in the hxx
class ScChangeActionContent;

class SAL_DLLPUBLIC_RTTI ScChangeAction
{
    friend class ScChangeTrack;
    friend class ScChangeActionIns;
    friend class ScChangeActionDel;
    friend class ScChangeActionMove;
    friend class ScChangeActionContent;

    ScChangeAction( const ScChangeAction& ) = delete;
    ScChangeAction& operator=( const ScChangeAction& ) = delete;

protected:

    ScBigRange          aBigRange;          // Ins/Del/MoveTo/ContentPos
    DateTime            aDateTime;          //! UTC
    OUString       aUser;              // who?
    OUString       aComment;           // user comment
    ScChangeAction*     pNext;              // next in linked list
    ScChangeAction*     pPrev;              // previous in linked list
    ScChangeActionLinkEntry*    pLinkAny;   // arbitrary links
    ScChangeActionLinkEntry*    pLinkDeletedIn; // access to insert areas which were
                                            // deleted or moved or rejected
    ScChangeActionLinkEntry*    pLinkDeleted;   // links to deleted
    ScChangeActionLinkEntry*    pLinkDependent; // links to dependent
    sal_uLong               nAction;
    sal_uLong               nRejectAction;
    ScChangeActionType  eType;
    ScChangeActionState eState;

    ScChangeAction( ScChangeActionType, const ScRange& );

    // only to be used in the XML import
    ScChangeAction( ScChangeActionType,
                    ScBigRange ,
                    const sal_uLong nAction,
                    const sal_uLong nRejectAction,
                    const ScChangeActionState eState,
                    const DateTime& aDateTime,
                    OUString aUser,
                    OUString aComment );

    // only to be used in the XML import
    ScChangeAction( ScChangeActionType, ScBigRange , const sal_uLong nAction);

    OUString GetRefString(
        const ScBigRange& rRange, const ScDocument& rDoc, bool bFlag3D = false) const;

    void SetActionNumber( sal_uLong n ) { nAction = n; }
    void SetRejectAction( sal_uLong n ) { nRejectAction = n; }
    void SetUser( const OUString& r );
    void SetType( ScChangeActionType e ) { eType = e; }
    void SetState( ScChangeActionState e ) { eState = e; }
    void SetRejected();

    ScBigRange& GetBigRange() { return aBigRange; }

    void AddLink( ScChangeAction* p, ScChangeActionLinkEntry* pL )
    {
        ScChangeActionLinkEntry* pLnk =
            new ScChangeActionLinkEntry(
            &pLinkAny, p );
        pLnk->SetLink( pL );
    }

    virtual ScChangeActionLinkEntry*    GetDeletedIn() const
                                            { return pLinkDeletedIn; }
    virtual ScChangeActionLinkEntry**   GetDeletedInAddress()
                                            { return &pLinkDeletedIn; }
    bool RemoveDeletedIn( const ScChangeAction* );
    void SetDeletedIn( ScChangeAction* );

    ScChangeActionLinkEntry* AddDeleted( ScChangeAction* p )
    {
        return new ScChangeActionLinkEntry(&pLinkDeleted, p);
    }

    ScChangeActionLinkEntry* AddDependent( ScChangeAction* p )
    {
        return new ScChangeActionLinkEntry(&pLinkDependent, p);
    }

    void                RemoveAllDependent();

    void                RemoveAllLinks();

    virtual void AddContent( ScChangeActionContent* ) = 0;
    virtual void DeleteCellEntries() = 0;

    virtual void UpdateReference( const ScChangeTrack*,
                     UpdateRefMode, const ScBigRange&,
                     sal_Int32 nDx, sal_Int32 nDy, sal_Int32 nDz );

    void Accept();
    virtual bool Reject(ScDocument& rDoc) = 0;
    void RejectRestoreContents( ScChangeTrack*, SCCOL nDx, SCROW nDy );

    // used in Reject() instead of IsRejectable()
    bool IsInternalRejectable() const;

    // Derived classes that hold a pointer to the
    // ChangeTrack must return that. Otherwise NULL.
    virtual const ScChangeTrack* GetChangeTrack() const = 0;

public:
    virtual ~ScChangeAction();

    bool IsInsertType() const;
    bool IsDeleteType() const;
    bool IsVirgin() const;
    SC_DLLPUBLIC bool IsAccepted() const;
    bool IsRejected() const;

    // Action rejects another Action
    bool IsRejecting() const;

    // if action is visible in the document
    bool IsVisible() const;

    // if action if touchable
    bool IsTouchable() const;

    // if action is an entry in dialog root
    bool IsDialogRoot() const;

    // if an entry in a dialog shall be a drop down entry
    bool IsDialogParent() const;

    // if action is a delete with subdeletes (aufgeklappt = open ?)
    bool IsMasterDelete() const;

    // if action is acceptable/selectable/rejectable
    bool IsClickable() const;

    // if action is rejectable
    bool IsRejectable() const;

    const ScBigRange& GetBigRange() const { return aBigRange; }
    SC_DLLPUBLIC DateTime GetDateTime() const;        // local time
    const DateTime&     GetDateTimeUTC() const      // UTC time
                            { return aDateTime; }
    ScChangeActionType  GetType() const { return eType; }
    ScChangeActionState GetState() const { return eState; }
    sal_uLong               GetActionNumber() const { return nAction; }
    sal_uLong               GetRejectAction() const { return nRejectAction; }

    ScChangeAction*     GetNext() const { return pNext; }
    ScChangeAction*     GetPrev() const { return pPrev; }

    bool IsDeletedIn() const;
    bool IsDeletedIn( const ScChangeAction* ) const;
    bool IsDeletedInDelType( ScChangeActionType ) const;
    void RemoveAllDeletedIn();

    const ScChangeActionLinkEntry* GetFirstDeletedEntry() const
                            { return pLinkDeleted; }
    const ScChangeActionLinkEntry* GetFirstDependentEntry() const
                            { return pLinkDependent; }
    bool HasDependent() const;
    bool HasDeleted() const;
                                // description will be appended to string
                                // with bSplitRange only one column/row will be considered for delete
                                // (for a listing of entries)
    virtual OUString GetDescription(
        ScDocument& rDoc, bool bSplitRange = false, bool bWarning = true ) const;

    virtual OUString GetRefString( ScDocument& rDoc, bool bFlag3D = false ) const;

                        // for DocumentMerge set old date of the other
                        // action, fetched by GetDateTimeUTC
    void                SetDateTimeUTC( const DateTime& rDT )
                            { aDateTime = rDT; }

    const OUString& GetUser() const { return aUser;}
    const OUString& GetComment() const { return aComment;}

    // set user comment
    void SetComment( const OUString& rStr );

                        // only to be used in the XML import
    void                SetDeletedInThis( sal_uLong nActionNumber,
                                const ScChangeTrack* pTrack );
                        // only to be used in the XML import
    void                AddDependent( sal_uLong nActionNumber,
                                const ScChangeTrack* pTrack );
};

//  ScChangeActionIns
class SAL_DLLPUBLIC_RTTI ScChangeActionIns final : public ScChangeAction
{
    friend class ScChangeTrack;

    bool mbEndOfList; /// whether or not a row was auto-inserted at the bottom.

    ScChangeActionIns( const ScDocument* pDoc, const ScRange& rRange, bool bEndOfList = false );

    virtual void                AddContent( ScChangeActionContent* ) override {}
    virtual void                DeleteCellEntries() override {}

    virtual bool Reject(ScDocument& rDoc) override;

    virtual const ScChangeTrack*    GetChangeTrack() const override { return nullptr; }

public:
    virtual                     ~ScChangeActionIns() override;
    ScChangeActionIns(
        const sal_uLong nActionNumber,
        const ScChangeActionState eState,
        const sal_uLong nRejectingNumber,
        const ScBigRange& aBigRange,
        const OUString& aUser,
        const DateTime& aDateTime,
        const OUString &sComment,
        const ScChangeActionType eType,
        bool bEndOfList = false );

    virtual OUString GetDescription(
        ScDocument& rDoc, bool bSplitRange = false, bool bWarning = true) const override;

    SC_DLLPUBLIC bool IsEndOfList() const;
};

//  ScChangeActionDel
class SAL_DLLPUBLIC_RTTI ScChangeActionMove;

class ScChangeActionDelMoveEntry final : public ScChangeActionLinkEntry
{
    friend class ScChangeActionDel;
    friend class ScChangeTrack;

    short               nCutOffFrom;
    short               nCutOffTo;

    inline ScChangeActionDelMoveEntry(
        ScChangeActionDelMoveEntry** ppPrevP,
        ScChangeActionMove* pMove,
        short nFrom, short nTo );

    inline ScChangeActionMove* GetMove();

public:
    const ScChangeActionDelMoveEntry*   GetNext() const
                            {
                                return static_cast<const ScChangeActionDelMoveEntry*>(
                                    ScChangeActionLinkEntry::GetNext());
                            }
    inline const ScChangeActionMove*   GetMove() const;
    short               GetCutOffFrom() const { return nCutOffFrom; }
    short               GetCutOffTo() const { return nCutOffTo; }
};

class ScChangeActionDel final : public ScChangeAction
{
    friend class ScChangeTrack;
    friend void ScChangeAction::Accept();

    ScChangeTrack*      pTrack;
    std::vector<ScChangeActionContent*> mvCells;
    ScChangeActionIns*  pCutOff;        // cut insert
    short               nCutOff;        // +: start  -: end
    ScChangeActionDelMoveEntry* pLinkMove;
    SCCOL               nDx;
    SCROW               nDy;

    ScChangeActionDel( const ScDocument* pDoc, const ScRange& rRange, SCCOL nDx, SCROW nDy, ScChangeTrack* );

    virtual void                AddContent( ScChangeActionContent* ) override;
    virtual void                DeleteCellEntries() override;

            void                UndoCutOffMoves();
            void                UndoCutOffInsert();

    virtual void                UpdateReference( const ScChangeTrack*,
                                    UpdateRefMode, const ScBigRange&,
                                    sal_Int32 nDx, sal_Int32 nDy, sal_Int32 nDz ) override;

    virtual bool Reject(ScDocument& rDoc) override;

    virtual const ScChangeTrack*    GetChangeTrack() const override { return pTrack; }

public:
    ScChangeActionDel(
        const sal_uLong nActionNumber, const ScChangeActionState eState,
        const sal_uLong nRejectingNumber, const ScBigRange& aBigRange,
        const OUString& aUser, const DateTime& aDateTime,
        const OUString &sComment, const ScChangeActionType eType,
        const SCCOLROW nD, ScChangeTrack* pTrack); // only to use in the XML import
                                            // which of nDx and nDy is set is dependent on the type
    virtual ~ScChangeActionDel() override;

    // is the last in a row (or single)
    bool IsBaseDelete() const;

    // is the first in a row (or single)
    bool IsTopDelete() const;

    // is part of a row
    bool IsMultiDelete() const;

    // is col, belonging to a TabDelete
    bool IsTabDeleteCol() const;

    SCCOL GetDx() const { return nDx; }
    SCROW GetDy() const { return nDy; }
    ScBigRange          GetOverAllRange() const;    // BigRange + (nDx, nDy)

    const ScChangeActionDelMoveEntry* GetFirstMoveEntry() const
                            { return pLinkMove; }
    const ScChangeActionIns*    GetCutOffInsert() const { return pCutOff; }
    short               GetCutOffCount() const { return nCutOff; }

    virtual OUString GetDescription(
        ScDocument& rDoc, bool bSplitRange = false, bool bWarning = true ) const override;

    void                SetCutOffInsert( ScChangeActionIns* p, short n )
                            { pCutOff = p; nCutOff = n; }   // only to use in the XML import
                                                                    // this should be protected, but for the XML import it is public
    // only to use in the XML import
    // this should be protected, but for the XML import it is public
    ScChangeActionDelMoveEntry* AddCutOffMove(
        ScChangeActionMove* pMove, short nFrom, short nTo );
};

//  ScChangeActionMove
class ScChangeActionMove final : public ScChangeAction
{
    friend class ScChangeTrack;
    friend struct std::default_delete<ScChangeActionMove>; // for std::unique_ptr
    friend class ScChangeActionDel;

    ScBigRange          aFromRange;
    ScChangeTrack*      pTrack;
    std::vector<ScChangeActionContent*> mvCells;
    sal_uLong               nStartLastCut;  // for PasteCut undo
    sal_uLong               nEndLastCut;

    ScChangeActionMove( const ScRange& rFromRange,
        const ScRange& rToRange,
        ScChangeTrack* pTrackP )
        : ScChangeAction( SC_CAT_MOVE, rToRange ),
            aFromRange( rFromRange ),
            pTrack( pTrackP ),
            nStartLastCut(0),
            nEndLastCut(0)
        {}
    virtual ~ScChangeActionMove() override;

    virtual void                AddContent( ScChangeActionContent* ) override;
    virtual void                DeleteCellEntries() override;

            ScBigRange&         GetFromRange() { return aFromRange; }

            void                SetStartLastCut( sal_uLong nVal ) { nStartLastCut = nVal; }
            sal_uLong               GetStartLastCut() const { return nStartLastCut; }
            void                SetEndLastCut( sal_uLong nVal ) { nEndLastCut = nVal; }
            sal_uLong               GetEndLastCut() const { return nEndLastCut; }

    virtual void                UpdateReference( const ScChangeTrack*,
                                    UpdateRefMode, const ScBigRange&,
                                    sal_Int32 nDx, sal_Int32 nDy, sal_Int32 nDz ) override;

    virtual bool Reject(ScDocument& rDoc) override;

    virtual const ScChangeTrack*    GetChangeTrack() const override { return pTrack; }

protected:
    using ScChangeAction::GetRefString;

public:
    ScChangeActionMove(const sal_uLong nActionNumber,
                    const ScChangeActionState eState,
                    const sal_uLong nRejectingNumber,
                    const ScBigRange& aToBigRange,
                    const OUString& aUser,
                    const DateTime& aDateTime,
                    const OUString &sComment,
                    ScBigRange aFromBigRange,
                    ScChangeTrack* pTrack); // only to use in the XML import

    const ScBigRange&   GetFromRange() const { return aFromRange; }
    SC_DLLPUBLIC        void                GetDelta( sal_Int32& nDx, sal_Int32& nDy, sal_Int32& nDz ) const;

    virtual OUString GetDescription(
        ScDocument& rDoc, bool bSplitRange = false, bool bWarning = true ) const override;

    virtual OUString GetRefString( ScDocument& rDoc, bool bFlag3D = false ) const override;
};

ScChangeActionDelMoveEntry::ScChangeActionDelMoveEntry(
    ScChangeActionDelMoveEntry** ppPrevP,
    ScChangeActionMove* pMove,
    short nFrom, short nTo )
    :   ScChangeActionLinkEntry(
            reinterpret_cast<ScChangeActionLinkEntry**>(
                ppPrevP),
            static_cast<ScChangeAction*>(pMove) ),
        nCutOffFrom( nFrom ),
        nCutOffTo( nTo )
{}

inline ScChangeActionMove* ScChangeActionDelMoveEntry::GetMove()
                           {
                               return static_cast<ScChangeActionMove*>(
                                   ScChangeActionLinkEntry::GetAction());
                           }

inline const ScChangeActionMove* ScChangeActionDelMoveEntry::GetMove() const
                          {
                              return static_cast<const ScChangeActionMove*>(
                                  ScChangeActionLinkEntry::GetAction());
                          }
//  ScChangeActionContent
enum ScChangeActionContentCellType
{
    SC_CACCT_NONE = 0,
    SC_CACCT_NORMAL,
    SC_CACCT_MATORG,
    SC_CACCT_MATREF
};

class SAL_DLLPUBLIC_RTTI ScChangeActionContent final : public ScChangeAction
{
    friend class ScChangeTrack;

    ScCellValue maOldCell;
    ScCellValue maNewCell;

    OUString maOldValue;
    OUString maNewValue;
    ScChangeActionContent*  pNextContent;   // at the same position
    ScChangeActionContent*  pPrevContent;
    ScChangeActionContent*  pNextInSlot;    // in the same slot
    ScChangeActionContent** ppPrevInSlot;

    void InsertInSlot( ScChangeActionContent** pp )
    {
        if ( !ppPrevInSlot )
        {
            ppPrevInSlot = pp;
            if ( ( pNextInSlot = *pp ) != nullptr )
                pNextInSlot->ppPrevInSlot = &pNextInSlot;
            *pp = this;
        }
    }

    void RemoveFromSlot()
    {
        if ( ppPrevInSlot )
        {
            if ( ( *ppPrevInSlot = pNextInSlot ) != nullptr )
                pNextInSlot->ppPrevInSlot = ppPrevInSlot;
            ppPrevInSlot = nullptr;    // not inserted
        }
    }

    ScChangeActionContent*  GetNextInSlot() { return pNextInSlot; }

    void ClearTrack();

    static OUString GetStringOfCell(
        const ScCellValue& rCell, const ScDocument* pDoc, const ScAddress& rPos );

    static OUString GetStringOfCell(
        const ScCellValue& rCell, const ScDocument* pDoc, sal_uLong nFormat );

    static void SetValue( OUString& rStr, ScCellValue& rCell, const ScAddress& rPos,
                          const ScCellValue& rOrgCell, const ScDocument* pFromDoc,
                          ScDocument* pToDoc );

    static void SetValue( OUString& rStr, ScCellValue& rCell, sal_uLong nFormat,
                          const ScCellValue& rOrgCell, const ScDocument* pFromDoc,
                          ScDocument* pToDoc );

    static void SetCell( OUString& rStr, const ScCellValue& rCell, sal_uLong nFormat, const ScDocument* pDoc );

    static bool NeedsNumberFormat( const ScCellValue& rVal );

    void SetValueString( OUString& rValue, ScCellValue& rCell, const OUString& rStr, ScDocument* pDoc );

    OUString GetValueString( const OUString& rValue, const ScCellValue& rCell,
                             const ScDocument* pDoc ) const;

    OUString GetFormulaString( const ScFormulaCell* pCell ) const;

    virtual void                AddContent( ScChangeActionContent* ) override {}
    virtual void                DeleteCellEntries() override {}

    virtual void                UpdateReference( const ScChangeTrack*,
                                    UpdateRefMode, const ScBigRange&,
                                    sal_Int32 nDx, sal_Int32 nDy, sal_Int32 nDz ) override;

    virtual bool Reject(ScDocument& rDoc) override;

    virtual const ScChangeTrack*    GetChangeTrack() const override { return nullptr; }

    // pRejectActions!=NULL: reject actions get
    // stacked, no SetNewValue, no Append
    bool Select( ScDocument&, ScChangeTrack*,
                 bool bOldest, ::std::stack<ScChangeActionContent*>* pRejectActions );

    void PutValueToDoc(
        const ScCellValue& rCell, const OUString& rValue, ScDocument* pDoc, SCCOL nDx, SCROW nDy ) const;

protected:
    using ScChangeAction::GetRefString;

public:
    ScChangeActionContent( const ScRange& rRange );

    ScChangeActionContent(
        const sal_uLong nActionNumber,  const ScChangeActionState eState,
        const sal_uLong nRejectingNumber, const ScBigRange& aBigRange,
        const OUString& aUser, const DateTime& aDateTime,
        const OUString &sComment, ScCellValue aOldCell,
        const ScDocument* pDoc, const OUString& sOldValue ); // to use for XML Import

    ScChangeActionContent(
        const sal_uLong nActionNumber, ScCellValue aNewCell,
        const ScBigRange& aBigRange, const ScDocument* pDoc,
        const OUString& sNewValue ); // to use for XML Import of Generated Actions

    virtual ~ScChangeActionContent() override;

    ScChangeActionContent*  GetNextContent() const { return pNextContent; }
    ScChangeActionContent*  GetPrevContent() const { return pPrevContent; }
    ScChangeActionContent*  GetTopContent() const;
    bool IsTopContent() const { return pNextContent == nullptr; }

    virtual ScChangeActionLinkEntry*    GetDeletedIn() const override;
    virtual ScChangeActionLinkEntry**   GetDeletedInAddress() override;

    void                PutOldValueToDoc( ScDocument*,
                            SCCOL nDx, SCROW nDy ) const;
    void                PutNewValueToDoc( ScDocument*,
                            SCCOL nDx, SCROW nDy ) const;

    void SetOldValue( const ScCellValue& rCell, const ScDocument* pFromDoc, ScDocument* pToDoc, sal_uLong nFormat );

    void SetOldValue( const ScCellValue& rCell, const ScDocument* pFromDoc, ScDocument* pToDoc );

    void SetNewValue( const ScCellValue& rCell, ScDocument* pDoc );

    // Used in import filter AppendContentOnTheFly,
    void SetOldNewCells(
        const ScCellValue& rOldCell, sal_uLong nOldFormat,
        const ScCellValue& rNewCell, sal_uLong nNewFormat, const ScDocument* pDoc );

    // Use this only in the XML import,
    // takes ownership of cell.
    void SetNewCell(
        const ScCellValue& rCell, const ScDocument* pDoc, const OUString& rFormatted );

                        // These functions should be protected but for
                        // the XML import they are public.
    void                SetNextContent( ScChangeActionContent* p )
                            { pNextContent = p; }
    void                SetPrevContent( ScChangeActionContent* p )
                            { pPrevContent = p; }

    // don't use:
    // assigns string / creates formula cell
    void SetOldValue( const OUString& rOld, ScDocument* pDoc );

    OUString GetOldString( const ScDocument* pDoc ) const;
    OUString GetNewString( const ScDocument* pDoc ) const;
    const ScCellValue& GetOldCell() const { return maOldCell;}
    const ScCellValue& GetNewCell() const { return maNewCell;}
    virtual OUString GetDescription(
        ScDocument& rDoc, bool bSplitRange = false, bool bWarning = true ) const override;

    virtual OUString GetRefString( ScDocument& rDoc, bool bFlag3D = false ) const override;

    static ScChangeActionContentCellType GetContentCellType( const ScCellValue& rCell );
    static ScChangeActionContentCellType GetContentCellType( const ScRefCellValue& rIter );

    // NewCell
    bool IsMatrixOrigin() const;
    // OldCell
    bool IsOldMatrixReference() const;
};

//  ScChangeActionReject
class ScChangeActionReject final : public ScChangeAction
{
    friend class ScChangeTrack;
    friend class ScChangeActionContent;

    virtual void AddContent( ScChangeActionContent* ) override {}
    virtual void DeleteCellEntries() override {}

    virtual bool Reject(ScDocument& rDoc) override;

    virtual const ScChangeTrack* GetChangeTrack() const override { return nullptr; }

public:
    ScChangeActionReject(const sal_uLong nActionNumber,
                    const ScChangeActionState eState,
                    const sal_uLong nRejectingNumber,
                    const ScBigRange& aBigRange,
                    const OUString& aUser,
                    const DateTime& aDateTime,
                    const OUString &sComment); // only to use in the XML import
};

//  ScChangeTrack
enum class ScChangeTrackMsgType
{
    NONE,
    Append,      // Actions appended
    Remove,      // Actions removed
    Change,      // Actions changed
    Parent       // became a parent (and wasn't before)
};

struct ScChangeTrackMsgInfo
{
    ScChangeTrackMsgType    eMsgType;
    sal_uLong                   nStartAction;
    sal_uLong                   nEndAction;
};

// MsgQueue for notification via ModifiedLink
typedef std::vector<ScChangeTrackMsgInfo> ScChangeTrackMsgQueue;
typedef std::vector<ScChangeTrackMsgInfo> ScChangeTrackMsgStack;
typedef std::map<sal_uLong, ScChangeAction*> ScChangeActionMap;

enum ScChangeTrackMergeState
{
    SC_CTMS_NONE,
    SC_CTMS_PREPARE,
    SC_CTMS_OWN,
    SC_CTMS_UNDO,
    SC_CTMS_OTHER
};

// Internally generated actions start at this value (nearly all bits set)
// and are decremented, to keep values in a table separated from "normal" actions.
#define SC_CHGTRACK_GENERATED_START (sal_uInt32(0xfffffff0))

class SAL_DLLPUBLIC_RTTI ScChangeTrack final : public utl::ConfigurationListener
{
    friend void ScChangeAction::RejectRestoreContents( ScChangeTrack*, SCCOL, SCROW );
    friend bool ScChangeActionDel::Reject( ScDocument& pDoc );
    friend void ScChangeActionDel::DeleteCellEntries();
    friend void ScChangeActionMove::DeleteCellEntries();
    friend bool ScChangeActionMove::Reject( ScDocument& pDoc );

    SCROW               mnContentRowsPerSlot;
    SCSIZE              mnContentSlots;

    css::uno::Sequence< sal_Int8 >   aProtectPass;
    ScChangeActionMap   aMap;
    ScChangeActionMap   aGeneratedMap;
    ScChangeActionMap   aPasteCutMap;
    ScChangeTrackMsgQueue   aMsgQueue;
    ScChangeTrackMsgStack   aMsgStackTmp;
    ScChangeTrackMsgStack   aMsgStackFinal;
    std::set<OUString> maUserCollection;
    OUString maUser;
    Link<ScChangeTrack&,void> aModifiedLink;
    ScRange             aInDeleteRange;
    DateTime            aFixDateTime;
    ScChangeAction*     pFirst;
    ScChangeAction*     pLast;
    ScChangeActionContent*  pFirstGeneratedDelContent;
    std::unique_ptr<ScChangeActionContent*[]> ppContentSlots;
    std::unique_ptr<ScChangeActionMove> pLastCutMove;
    ScChangeActionLinkEntry*    pLinkInsertCol;
    ScChangeActionLinkEntry*    pLinkInsertRow;
    ScChangeActionLinkEntry*    pLinkInsertTab;
    ScChangeActionLinkEntry*    pLinkMove;
    std::optional<ScChangeTrackMsgInfo> xBlockModifyMsg;
    ScDocument&             rDoc;
    sal_uLong               nActionMax;
    sal_uLong               nGeneratedMin;
    sal_uLong               nMarkLastSaved;
    sal_uLong               nStartLastCut;
    sal_uLong               nEndLastCut;
    sal_uLong               nLastMerge;
    ScChangeTrackMergeState eMergeState;
    bool bInDelete:1;
    bool bInDeleteUndo:1;
    bool bInDeleteTop:1;
    bool bInPasteCut:1;
    bool bUseFixDateTime:1;
    bool bTimeNanoSeconds:1;

    ScChangeTrack( const ScChangeTrack& ) = delete;
    ScChangeTrack& operator=( const ScChangeTrack& ) = delete;

    SCROW               InitContentRowsPerSlot();

    // true if one is ScMatrixMode::Formula and the other is
    // not, or if both are and range differs
    static bool IsMatrixFormulaRangeDifferent(
        const ScCellValue& rOldCell, const ScCellValue& rNewCell );

    void                Init();
    void                DtorClear();
    void                SetInDeleteRange( const ScRange& rRange )
                            { aInDeleteRange = rRange; }
    void                SetInDelete( bool bVal )
                            { bInDelete = bVal; }
    void                SetInDeleteTop( bool bVal )
                            { bInDeleteTop = bVal; }
    void                SetInDeleteUndo( bool bVal )
                            { bInDeleteUndo = bVal; }
    void                SetInPasteCut( bool bVal )
                            { bInPasteCut = bVal; }
    void                SetMergeState( ScChangeTrackMergeState eState )
                            { eMergeState = eState; }
    ScChangeTrackMergeState GetMergeState() const { return eMergeState; }
    void                SetLastMerge( sal_uLong nVal ) { nLastMerge = nVal; }
    sal_uLong               GetLastMerge() const { return nLastMerge; }

    void                SetLastCutMoveRange( const ScRange&, ScDocument* );

                        // create block of ModifyMsg
    void                StartBlockModify( ScChangeTrackMsgType,
                            sal_uLong nStartAction );
    void                EndBlockModify( sal_uLong nEndAction );

    void                AddDependentWithNotify( ScChangeAction* pParent,
                            ScChangeAction* pDependent );

    void                Dependencies( ScChangeAction* );
    void UpdateReference( ScChangeAction*, bool bUndo );
    void UpdateReference( ScChangeAction** ppFirstAction, ScChangeAction* pAct, bool bUndo );
    void                Append( ScChangeAction* pAppend, sal_uLong nAction );
    SC_DLLPUBLIC        void                AppendDeleteRange( const ScRange&,
                                    ScDocument* pRefDoc, SCTAB nDz,
                                    sal_uLong nRejectingInsert );
    void                AppendOneDeleteRange( const ScRange& rOrgRange,
                            ScDocument* pRefDoc,
                            SCCOL nDx, SCROW nDy, SCTAB nDz,
                            sal_uLong nRejectingInsert );
    void                LookUpContents( const ScRange& rOrgRange,
                            ScDocument* pRefDoc,
                            SCCOL nDx, SCROW nDy, SCTAB nDz );
    void                Remove( ScChangeAction* );
    void                MasterLinks( ScChangeAction* );

                                // Content on top at Position
    ScChangeActionContent*  SearchContentAt( const ScBigAddress&,
                                    const ScChangeAction* pButNotThis ) const;
    void                DeleteGeneratedDelContent(
                                    ScChangeActionContent* );

    ScChangeActionContent* GenerateDelContent(
        const ScAddress& rPos, const ScCellValue& rCell, const ScDocument* pFromDoc );

    void                DeleteCellEntries(
                                    std::vector<ScChangeActionContent*>&,
                                    const ScChangeAction* pDeletor );

                                // Reject action and all dependent actions,
                                // Table stems from previous GetDependents,
                                // only needed for Insert and Move (MasterType),
                                // is NULL otherwise.
                                // bRecursion == called from reject with table
    bool Reject( ScChangeAction*, ScChangeActionMap*, bool bRecursion );

    bool IsLastAction( sal_uLong nNum ) const;

            void                ClearMsgQueue();
    virtual void                ConfigurationChanged( utl::ConfigurationBroadcaster*, ConfigurationHints ) override;

public:

    SCSIZE              ComputeContentSlot( sal_Int32 nRow ) const;

    SC_DLLPUBLIC ScChangeTrack( ScDocument& );
    ScChangeTrack(ScDocument& rDocP, std::set<OUString>&& aTempUserCollection); // only to use in the XML import
    SC_DLLPUBLIC virtual ~ScChangeTrack() override;
    void Clear();

    ScChangeActionContent*  GetFirstGenerated() const { return pFirstGeneratedDelContent; }
    ScChangeAction*     GetFirst() const { return pFirst; }
    ScChangeAction*     GetLast() const { return pLast; }
    sal_uLong               GetActionMax() const { return nActionMax; }
    bool IsGenerated( sal_uLong nAction ) const;
    SC_DLLPUBLIC ScChangeAction* GetAction( sal_uLong nAction ) const;
    ScChangeAction* GetGenerated( sal_uLong nGenerated ) const;
    ScChangeAction* GetActionOrGenerated( sal_uLong nAction ) const;
    sal_uLong GetLastSavedActionNumber() const;
    void SetLastSavedActionNumber(sal_uLong nNew);
    ScChangeAction* GetLastSaved() const;
    ScChangeActionContent** GetContentSlots() const { return ppContentSlots.get(); }

    const ScRange&      GetInDeleteRange() const
                            { return aInDeleteRange; }
    bool IsInDelete() const { return bInDelete; }
    bool IsInDeleteTop() const { return bInDeleteTop; }
    bool IsInDeleteUndo() const { return bInDeleteUndo; }
    bool IsInPasteCut() const { return bInPasteCut; }
    void CreateAuthorName();
    SC_DLLPUBLIC void SetUser( const OUString& rUser );
    const OUString& GetUser() const { return maUser;}
    const std::set<OUString>& GetUserCollection() const { return maUserCollection;}
    ScDocument&         GetDocument() const { return rDoc; }
                        // for import filter
    const DateTime&     GetFixDateTime() const { return aFixDateTime; }

                        // set this if the date/time set with
                        // SetFixDateTime...() shall be applied to
                        // appended actions
    void                SetUseFixDateTime( bool bVal )
                            { bUseFixDateTime = bVal; }
                        // for MergeDocument, apply original date/time as UTC
    void                SetFixDateTimeUTC( const DateTime& rDT )
                            { aFixDateTime = rDT; }
                        // for import filter, apply original date/time as local time
    void                SetFixDateTimeLocal( const DateTime& rDT )
                            { aFixDateTime = rDT; aFixDateTime.ConvertToUTC(); }

    void                Append( ScChangeAction* );

                                // pRefDoc may be NULL => no lookup of contents
                                // => no generation of deleted contents
    SC_DLLPUBLIC void AppendDeleteRange( const ScRange&,
                                    ScDocument* pRefDoc,
                                    sal_uLong& nStartAction, sal_uLong& nEndAction,
                                    SCTAB nDz = 0 );
                                    // nDz: multi TabDel, LookUpContent must be searched
                                    // with an offset of -nDz

                        // after new value was set in the document,
                        // old value from RefDoc/UndoDoc
    void                AppendContent( const ScAddress& rPos,
                            const ScDocument* pRefDoc );
                        // after new values were set in the document,
                        // old values from RefDoc/UndoDoc
    void                AppendContentRange( const ScRange& rRange,
                            ScDocument* pRefDoc,
                            sal_uLong& nStartAction, sal_uLong& nEndAction,
                            ScChangeActionClipMode eMode = SC_CACM_NONE );
                        // after new value was set in the document,
                        // old value from pOldCell, nOldFormat,
                        // RefDoc==NULL => Doc
    void AppendContent( const ScAddress& rPos, const ScCellValue& rOldCell,
                        sal_uLong nOldFormat, ScDocument* pRefDoc = nullptr );
                        // after new value was set in the document,
                        // old value from pOldCell, format from Doc
    SC_DLLPUBLIC void AppendContent( const ScAddress& rPos, const ScCellValue& rOldCell );
                        // after new values were set in the document,
                        // old values from RefDoc/UndoDoc.
                        // All contents with a cell in RefDoc
    void                AppendContentsIfInRefDoc( ScDocument& rRefDoc,
                            sal_uLong& nStartAction, sal_uLong& nEndAction );

                        // Meant for import filter, creates and inserts
                        // an unconditional content action of the two
                        // cells without querying the document, not
                        // even for number formats (though the number
                        // formatter of the document may be used).
                        // The action is returned and may be used to
                        // set user name, description, date/time et al.
                        // Takes ownership of the cells!
    SC_DLLPUBLIC ScChangeActionContent* AppendContentOnTheFly(
        const ScAddress& rPos, const ScCellValue& rOldCell, const ScCellValue& rNewCell,
        sal_uLong nOldFormat = 0, sal_uLong nNewFormat = 0 );

    // Only use the following two if there is no different solution! (Assign
    // string for NewValue or creation of a formula respectively)

    SC_DLLPUBLIC void AppendInsert( const ScRange& rRange, bool bEndOfList = false );

                                // pRefDoc may be NULL => no lookup of contents
                                // => no generation of deleted contents
    SC_DLLPUBLIC void AppendMove( const ScRange& rFromRange, const ScRange& rToRange,
                                  ScDocument* pRefDoc );

                                // Cut to Clipboard
    void ResetLastCut()
    {
        nStartLastCut = nEndLastCut = 0;
        pLastCutMove.reset();
    }
    bool HasLastCut() const
    {
        return nEndLastCut > 0 &&
            nStartLastCut <= nEndLastCut &&
            pLastCutMove;
    }

    SC_DLLPUBLIC void Undo( sal_uLong nStartAction, sal_uLong nEndAction, bool bMerge = false );

                        // adjust references for MergeDocument
                        //! may only be used in a temporary opened document.
                        //! the Track (?) is unclean afterwards
    void                MergePrepare( const ScChangeAction* pFirstMerge, bool bShared );
    void                MergeOwn( ScChangeAction* pAct, sal_uLong nFirstMerge, bool bShared );
    static bool MergeIgnore( const ScChangeAction&, sal_uLong nFirstMerge );

                                // This comment was already really strange in German.
                                // Tried to structure it a little. Hope no information got lost...
                                //
                                // Insert dependents into table.
                                // ScChangeAction is
                                // - "Insert": really dependents
                                // - "Move": dependent contents in FromRange /
                                //           deleted contents in ToRange
                                //      OR   inserts in FromRange or ToRange
                                // - "Delete": a list of deleted (what?)
                                //      OR     for content, different contents at the same position
                                //      OR     MatrixReferences belonging to MatrixOrigin

                                // With bListMasterDelete (==TRUE ?) all Deletes of a row belonging
                                // to a MasterDelete are listed (possibly it is
                                // "all Deletes belonging...are listed in a row?)

                                // With bAllFlat (==TRUE ?) all dependents of dependents
                                // will be inserted flatly.

    SC_DLLPUBLIC void GetDependents(
        ScChangeAction*, ScChangeActionMap&, bool bListMasterDelete = false, bool bAllFlat = false ) const;

    // Reject visible action (and dependents)
    bool Reject( ScChangeAction*, bool bShared = false );

    // Accept visible action (and dependents)
    SC_DLLPUBLIC bool Accept( ScChangeAction* );

    void                AcceptAll();    // all Virgins
    bool                RejectAll();    // all Virgins

    // Selects a content of several contents at the same
    // position and accepts this one and
    // the older ones, rejects the more recent ones.
    // If bOldest==TRUE then the first OldValue
    // of a Virgin-Content-List will be restored.
    bool SelectContent( ScChangeAction*, bool bOldest = false );

                        // If ModifiedLink is set, changes go to
                        // ScChangeTrackMsgQueue
    void                SetModifiedLink( const Link<ScChangeTrack&,void>& r )
                            { aModifiedLink = r; ClearMsgQueue(); }
    ScChangeTrackMsgQueue& GetMsgQueue();

    void                NotifyModified( ScChangeTrackMsgType eMsgType,
                            sal_uLong nStartAction, sal_uLong nEndAction );

    sal_uLong           AddLoadedGenerated( const ScCellValue& rNewCell,
                            const ScBigRange& aBigRange, const OUString& sNewValue ); // only to use in the XML import
    void                AppendLoaded( std::unique_ptr<ScChangeAction> pAppend ); // this is only for the XML import public, it should be protected
    void                SetActionMax(sal_uLong nTempActionMax)
                            { nActionMax = nTempActionMax; } // only to use in the XML import

    void                SetProtection( const css::uno::Sequence< sal_Int8 >& rPass )
                            { aProtectPass = rPass; }
    const css::uno::Sequence< sal_Int8 >& GetProtection() const
                                    { return aProtectPass; }
    bool IsProtected() const { return aProtectPass.hasElements(); }

                                // If time stamps of actions of this
                                // ChangeTrack and a second one are to be
                                // compared including nanoseconds.
    void SetTimeNanoSeconds( bool bVal ) { bTimeNanoSeconds = bVal; }
    bool IsTimeNanoSeconds() const { return bTimeNanoSeconds; }

    void AppendCloned( ScChangeAction* pAppend );
    SC_DLLPUBLIC ScChangeTrack* Clone( ScDocument* pDocument ) const;
    static void MergeActionState( ScChangeAction* pAct, const ScChangeAction* pOtherAct );
    /// Get info about all ScChangeAction elements.
    void GetChangeTrackInfo(tools::JsonWriter&);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
