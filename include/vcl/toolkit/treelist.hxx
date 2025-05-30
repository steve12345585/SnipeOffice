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

#if !defined(VCL_DLLIMPLEMENTATION) && !defined(TOOLKIT_DLLIMPLEMENTATION) && !defined(VCL_INTERNALS)
#error "don't use this in new code"
#endif

#include <config_options.h>
#include <vcl/dllapi.h>
#include <vcl/toolkit/treelistentries.hxx>

#include <tools/link.hxx>
#include <tools/contnr.hxx>

#include <memory>

enum class SvListAction
{
    INSERTED         = 1,
    REMOVING         = 2,
    REMOVED          = 3,
    MOVING           = 4,
    MOVED            = 5,
    CLEARING         = 6,
    INSERTED_TREE    = 7,
    INVALIDATE_ENTRY = 8,
    RESORTING        = 9,
    RESORTED         = 10,
    CLEARED          = 11
};

class SvTreeListEntry;
class SvListView;
class SvViewDataEntry;

enum class SvSortMode { Ascending, Descending, None };

// For the return values of Sortlink:
// See International::Compare( pLeft, pRight )
// ( Compare(a,b) ==> b.Compare(a) ==> strcmp(a,b) )
struct SvSortData
{
    const SvTreeListEntry* pLeft;
    const SvTreeListEntry* pRight;
};

class UNLESS_MERGELIBS_MORE(VCL_DLLPUBLIC) SvTreeList final
{
    friend class        SvListView;

    SvListView&         mrOwnerListView;
    sal_uInt32          nEntryCount;

    Link<SvTreeListEntry*, SvTreeListEntry*>  aCloneLink;
    Link<const SvSortData&, sal_Int32>        aCompareLink;
    SvSortMode          eSortMode;

    bool                bAbsPositionsValid;

    bool mbEnableInvalidate;

    SvTreeListEntry*        FirstVisible() const { return First(); }
    SvTreeListEntry*        NextVisible( const SvListView*,SvTreeListEntry* pEntry, sal_uInt16* pDepth=nullptr ) const;
    SvTreeListEntry*        PrevVisible( const SvListView*,SvTreeListEntry* pEntry ) const;
    SvTreeListEntry*        LastVisible( const SvListView* ) const;
    SvTreeListEntry*        NextVisible( const SvListView*,SvTreeListEntry* pEntry, sal_uInt16& rDelta ) const;
    SvTreeListEntry*        PrevVisible( const SvListView*,SvTreeListEntry* pEntry, sal_uInt16& rDelta ) const;

    bool               IsEntryVisible( const SvListView*,SvTreeListEntry* pEntry ) const;
    SvTreeListEntry*    GetEntryAtVisPos( const SvListView*, sal_uInt32 nVisPos ) const;
    sal_uInt32           GetVisiblePos( const SvListView*,SvTreeListEntry const * pEntry ) const;
    sal_uInt32           GetVisibleCount( SvListView* ) const;
    sal_uInt32           GetVisibleChildCount( const SvListView*,SvTreeListEntry* pParent ) const;

    SvTreeListEntry*        FirstSelected( const SvListView*) const;
    SvTreeListEntry*        NextSelected( const SvListView*,SvTreeListEntry* pEntry ) const;

    sal_uInt32           GetChildSelectionCount( const SvListView*,SvTreeListEntry* pParent ) const;

    SAL_DLLPRIVATE void SetAbsolutePositions();

    SAL_DLLPRIVATE void CloneChildren(
        SvTreeListEntries& rDst, sal_uInt32& rCloneCount, SvTreeListEntries& rSrc, SvTreeListEntry& rNewParent) const;

    /**
     * Invalidate the cached position data to have them re-generated before
     * the next access.
     */
    SAL_DLLPRIVATE static void SetListPositions( SvTreeListEntries& rEntries );

    // rPos is not changed for SortModeNone
    SAL_DLLPRIVATE void GetInsertionPos(
                            SvTreeListEntry const * pEntry,
                            SvTreeListEntry* pParent,
                            sal_uInt32& rPos
                        );

    SAL_DLLPRIVATE void ResortChildren( SvTreeListEntry* pParent );

    SvTreeList(const SvTreeList&) = delete;
    SvTreeList& operator= (const SvTreeList&) = delete;

    std::unique_ptr<SvTreeListEntry>  pRootItem;

public:

                        SvTreeList() = delete;
                        SvTreeList(SvListView&);
                        ~SvTreeList();

    void                Broadcast(
                            SvListAction nActionId,
                            SvTreeListEntry* pEntry1=nullptr,
                            SvTreeListEntry* pEntry2=nullptr,
                            sal_uInt32 nPos=0
                        );

    void                EnableInvalidate( bool bEnable );
    bool                IsEnableInvalidate() const { return mbEnableInvalidate; }

    // Notify all Listeners
    void                InvalidateEntry( SvTreeListEntry* );

    sal_uInt32          GetEntryCount() const { return nEntryCount; }
    SvTreeListEntry*    First() const;
    SvTreeListEntry*    Next( SvTreeListEntry* pEntry, sal_uInt16* pDepth=nullptr ) const;
    SvTreeListEntry*    Prev( SvTreeListEntry* pEntry ) const;
    SvTreeListEntry*    Last() const;

    SvTreeListEntry*    FirstChild( SvTreeListEntry* pParent ) const;

    sal_uInt32          Insert( SvTreeListEntry* pEntry,SvTreeListEntry* pPar,sal_uInt32 nPos = TREELIST_APPEND);
    sal_uInt32          Insert( SvTreeListEntry* pEntry,sal_uInt32 nRootPos = TREELIST_APPEND )
    { return Insert(pEntry, pRootItem.get(), nRootPos ); }

    void                InsertTree( SvTreeListEntry* pTree, SvTreeListEntry* pTargetParent, sal_uInt32 nListPos );

    // Entries need to be in the same Model!
    void                Move( SvTreeListEntry* pSource, SvTreeListEntry* pTarget );

    // Creates ChildList if needed
    sal_uInt32          Move( SvTreeListEntry* pSource, SvTreeListEntry* pTargetParent, sal_uInt32 nListPos);
    sal_uInt32          Copy( SvTreeListEntry* pSource, SvTreeListEntry* pTargetParent, sal_uInt32 nListPos);

    bool Remove( const SvTreeListEntry* pEntry );
    void                Clear();

    bool                IsChild(const SvTreeListEntry* pParent, const SvTreeListEntry* pChild) const;
    SvTreeListEntry*        GetEntry( SvTreeListEntry* pParent, sal_uInt32 nPos ) const;
    SvTreeListEntry*        GetEntry( sal_uInt32 nRootPos ) const;
    SvTreeListEntry*        GetEntryAtAbsPos( sal_uInt32 nAbsPos ) const;

    const SvTreeListEntry* GetParent( const SvTreeListEntry* pEntry ) const;
    SvTreeListEntry* GetParent( SvTreeListEntry* pEntry );

    const SvTreeListEntries& GetChildList( SvTreeListEntry* pParent ) const;
    SvTreeListEntries& GetChildList( SvTreeListEntry* pParent );

    sal_uInt32 GetAbsPos( const SvTreeListEntry* pEntry ) const;
    static sal_uInt32 GetRelPos( const SvTreeListEntry* pChild );

    sal_uInt32 GetChildCount( const SvTreeListEntry* pParent ) const;
    sal_uInt16 GetDepth( const SvTreeListEntry* pEntry ) const;
    bool IsAtRootDepth( const SvTreeListEntry* pEntry ) const;

    // The Model calls the Clone Link to clone Entries.
    // Thus we do not need to derive from the Model if we derive from SvTreeListEntry.
    // The Handler needs to return a SvTreeListEntry*
    SvTreeListEntry*    Clone( SvTreeListEntry* pEntry, sal_uInt32& nCloneCount ) const;
    void                SetCloneLink( const Link<SvTreeListEntry*,SvTreeListEntry*>& rLink )
    { aCloneLink=rLink; }

    const Link<SvTreeListEntry*,SvTreeListEntry*>&       GetCloneLink() const
    { return aCloneLink; }

    SvTreeListEntry*    CloneEntry( SvTreeListEntry* pSource ) const; // Calls the Clone Link

    void                SetSortMode( SvSortMode eMode ) { eSortMode = eMode; }
    SvSortMode          GetSortMode() const { return eSortMode; }
    sal_Int32           Compare(const SvTreeListEntry* pLeft, const SvTreeListEntry* pRight) const;
    void                SetCompareHdl( const Link<const SvSortData&, sal_Int32>& rLink ) { aCompareLink = rLink; }
    void                Resort();
};

class UNLESS_MERGELIBS_MORE(VCL_DLLPUBLIC) SvListView
{
    friend class SvTreeList;

    struct SAL_DLLPRIVATE Impl;
    std::unique_ptr<Impl> m_pImpl;

protected:
    std::unique_ptr<SvTreeList> pModel;

    void                ExpandListEntry( SvTreeListEntry* pParent );
    void                CollapseListEntry( SvTreeListEntry* pParent );
    bool                SelectListEntry( SvTreeListEntry* pEntry, bool bSelect );

public:
                        SvListView();   // Sets the Model to 0
    void                dispose();
    virtual             ~SvListView();
    void                Clear();
    virtual void        ModelNotification(
                            SvListAction nActionId,
                            SvTreeListEntry* pEntry1,
                            SvTreeListEntry* pEntry2,
                            sal_uInt32 nPos
                        );

    sal_uInt32          GetVisibleCount() const
    { return pModel->GetVisibleCount( const_cast<SvListView*>(this) ); }

    SvTreeListEntry*        FirstVisible() const
    { return pModel->FirstVisible(); }

    SvTreeListEntry*        NextVisible( SvTreeListEntry* pEntry ) const
    { return pModel->NextVisible(this,pEntry); }

    SvTreeListEntry*        PrevVisible( SvTreeListEntry* pEntry ) const
    { return pModel->PrevVisible(this,pEntry); }

    SvTreeListEntry*        LastVisible() const
    { return pModel->LastVisible(this); }

    SvTreeListEntry*        NextVisible( SvTreeListEntry* pEntry, sal_uInt16& rDelta ) const
    { return pModel->NextVisible(this,pEntry,rDelta); }

    SvTreeListEntry*        PrevVisible( SvTreeListEntry* pEntry, sal_uInt16& rDelta ) const
    { return pModel->PrevVisible(this,pEntry,rDelta); }

    sal_uInt32              GetSelectionCount() const;

    SvTreeListEntry* FirstSelected() const
    { return pModel->FirstSelected(this); }

    SvTreeListEntry*        NextSelected( SvTreeListEntry* pEntry ) const
    { return pModel->NextSelected(this,pEntry); }

    SvTreeListEntry*        GetEntryAtAbsPos( sal_uInt32 nAbsPos ) const
    { return pModel->GetEntryAtAbsPos(nAbsPos); }

    SvTreeListEntry*        GetEntryAtVisPos( sal_uInt32 nVisPos ) const
    { return pModel->GetEntryAtVisPos(this,nVisPos); }

    sal_uInt32              GetAbsPos( SvTreeListEntry const * pEntry ) const
    { return pModel->GetAbsPos(pEntry); }

    sal_uInt32           GetVisiblePos( SvTreeListEntry const * pEntry ) const
    { return pModel->GetVisiblePos(this,pEntry); }

    sal_uInt32           GetVisibleChildCount(SvTreeListEntry* pParent ) const
    { return pModel->GetVisibleChildCount(this,pParent); }

    bool               IsEntryVisible( SvTreeListEntry* pEntry ) const
    { return pModel->IsEntryVisible(this,pEntry); }

    bool                IsExpanded( SvTreeListEntry* pEntry ) const;
    bool                IsAllExpanded( SvTreeListEntry* pEntry) const;
    bool                IsSelected(const SvTreeListEntry* pEntry) const;
    void                SetEntryFocus( SvTreeListEntry* pEntry, bool bFocus );
    const SvViewDataEntry*         GetViewData( const SvTreeListEntry* pEntry ) const;
    SvViewDataEntry*         GetViewData( SvTreeListEntry* pEntry );
    bool                HasViewData() const;

    virtual void        InitViewData( SvViewDataEntry*, SvTreeListEntry* pEntry );

    virtual void        ModelHasCleared();
    virtual void        ModelHasInserted( SvTreeListEntry* pEntry );
    virtual void        ModelHasInsertedTree( SvTreeListEntry* pEntry );
    virtual void        ModelIsMoving( SvTreeListEntry* pSource );
    virtual void        ModelHasMoved( SvTreeListEntry* pSource );
    virtual void        ModelIsRemoving( SvTreeListEntry* pEntry );
    virtual void        ModelHasRemoved( SvTreeListEntry* pEntry );
    virtual void        ModelHasEntryInvalidated( SvTreeListEntry* pEntry );
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
