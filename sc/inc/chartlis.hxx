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

#include <vcl/idle.hxx>
#include <svl/listener.hxx>
#include "rangelst.hxx"
#include "externalrefmgr.hxx"

#include <memory>
#include <map>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace com::sun::star::chart { class XChartData; }
namespace com::sun::star::chart { class XChartDataChangeEventListener; }

class Timer;
class ScDocument;
class ScChartUnoData;

class SAL_DLLPUBLIC_RTTI ScChartListener final : public SvtListener
{
public:
    class ExternalRefListener final : public ScExternalRefManager::LinkListener
    {
    public:
        ExternalRefListener(ScChartListener& rParent, ScDocument& rDoc);
        virtual ~ExternalRefListener() override;
        virtual void notify(sal_uInt16 nFileId, ScExternalRefManager::LinkUpdateType eType) override;
        void addFileId(sal_uInt16 nFileId);
        void removeFileId(sal_uInt16 nFileId);
        std::unordered_set<sal_uInt16>& getAllFileIds() { return maFileIds;}

    private:
        ExternalRefListener(const ExternalRefListener& r) = delete;

        ScChartListener& mrParent;
        std::unordered_set<sal_uInt16> maFileIds;
        ScDocument* m_pDoc;
    };

private:

    std::unique_ptr<ExternalRefListener> mpExtRefListener;
    std::vector<ScTokenRef> maTokens;

    OUString maName;
    std::unique_ptr<ScChartUnoData> pUnoData;
    ScDocument&     mrDoc;
    bool            bUsed:1;  // for ScChartListenerCollection::FreeUnused
    bool            bDirty:1;

    ScChartListener& operator=( const ScChartListener& ) = delete;

public:
    ScChartListener( OUString aName, ScDocument& rDoc,
                     const ScRangeListRef& rRangeListRef );
    SC_DLLPUBLIC ScChartListener( OUString aName, ScDocument& rDoc,
                     ::std::vector<ScTokenRef> aTokens );
    ScChartListener( const ScChartListener& ) = delete;
    SC_DLLPUBLIC virtual ~ScChartListener() override;

    const OUString& GetName() const { return maName;}

    void            SetUno( const css::uno::Reference< css::chart::XChartDataChangeEventListener >& rListener,
                            const css::uno::Reference< css::chart::XChartData >& rSource );
    css::uno::Reference< css::chart::XChartDataChangeEventListener >  GetUnoListener() const;
    css::uno::Reference< css::chart::XChartData >                     GetUnoSource() const;

    bool            IsUno() const   { return (pUnoData != nullptr); }

    virtual void Notify( const SfxHint& rHint ) override;
    SC_DLLPUBLIC void StartListeningTo();
    void            EndListeningTo();
    void            ChangeListening( const ScRangeListRef& rRangeListRef,
                                    bool bDirty );
    void            Update();
    ScRangeListRef  GetRangeList() const;
    void            SetRangeList( const ScRangeListRef& rNew );
    bool            IsUsed() const { return bUsed; }
    void            SetUsed( bool bFlg ) { bUsed = bFlg; }
    bool            IsDirty() const { return bDirty; }
    void            SetDirty( bool bFlg ) { bDirty = bFlg; }

    void            UpdateChartIntersecting( const ScRange& rRange );

    ExternalRefListener* GetExtRefListener();
    void            SetUpdateQueue();

    bool operator==( const ScChartListener& ) const;
    bool operator!=( const ScChartListener& r ) const;
};

class SC_DLLPUBLIC ScChartHiddenRangeListener
{
public:
    ScChartHiddenRangeListener();
    virtual ~ScChartHiddenRangeListener();
    virtual void notify() = 0;
};

class ScChartListenerCollection final
{
public:
    typedef std::map<OUString, std::unique_ptr<ScChartListener>> ListenersType;
    typedef std::unordered_set<OUString> StringSetType;
private:
    ListenersType m_Listeners;
    enum UpdateStatus
    {
        SC_CLCUPDATE_NONE,
        SC_CLCUPDATE_RUNNING,
        SC_CLCUPDATE_MODIFIED
    } meModifiedDuringUpdate;

    std::unordered_multimap<ScChartHiddenRangeListener*, ScRange> maHiddenListeners;

    StringSetType maNonOleObjectNames;

    Idle            aIdle;
    ScDocument&     rDoc;

    DECL_LINK(TimerHdl, Timer *, void);

    ScChartListenerCollection& operator=( const ScChartListenerCollection& ) = delete;

    void Init();

public:
    SC_DLLPUBLIC ScChartListenerCollection( ScDocument& rDoc );
    ScChartListenerCollection( const ScChartListenerCollection& );
    SC_DLLPUBLIC ~ScChartListenerCollection();

                    // only needed after copy-ctor, if newly added to doc
    void            StartAllListeners();

    SC_DLLPUBLIC bool insert(ScChartListener* pListener);
    ScChartListener* findByName(const OUString& rName);
    const ScChartListener* findByName(const OUString& rName) const;
    bool hasListeners() const;

    void removeByName(const OUString& rName);

    const ListenersType& getListeners() const { return m_Listeners; }
    ListenersType& getListeners() { return m_Listeners; }
    StringSetType& getNonOleObjectNames() { return maNonOleObjectNames;}

    /**
     * Create a unique name that's not taken by any existing chart listener
     * objects.  The name consists of a prefix given followed by a number.
     */
    OUString getUniqueName(std::u16string_view rPrefix) const;

    void            ChangeListening( const OUString& rName,
                                    const ScRangeListRef& rRangeListRef );
    // use FreeUnused only the way it's used in ScDocument::UpdateChartListenerCollection
    void            FreeUnused();
    void            FreeUno( const css::uno::Reference< css::chart::XChartDataChangeEventListener >& rListener,
                             const css::uno::Reference< css::chart::XChartData >& rSource );
    void            StartTimer();
    void            UpdateDirtyCharts();
    void            SetDirty();
    void            SetDiffDirty( const ScChartListenerCollection&,
                        bool bSetChartRangeLists );

    SC_DLLPUBLIC void SetRangeDirty( const ScRange& rRange );     // for example rows/columns

    void            UpdateChartsContainingTab( SCTAB nTab );

    bool operator==( const ScChartListenerCollection& r ) const;

    /**
     * Start listening on hide/show change within specified cell range.  A
     * single listener may listen on multiple ranges when the caller passes
     * the same pointer multiple times with different ranges.
     *
     * Note that the caller is responsible for managing the life-cycle of the
     * listener instance.
     */
    SC_DLLPUBLIC void StartListeningHiddenRange( const ScRange& rRange,
                                               ScChartHiddenRangeListener* pListener );

    /**
     * Remove all ranges associated with passed listener instance from the
     * list of hidden range listeners.  This does not delete the passed
     * listener instance.
     */
    SC_DLLPUBLIC void EndListeningHiddenRange( ScChartHiddenRangeListener* pListener );
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
