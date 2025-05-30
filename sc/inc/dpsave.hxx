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

#include <memory>
#include <vector>

#include <com/sun/star/sheet/DataPilotFieldOrientation.hpp>
#include <rtl/ustring.hxx>
#include <sal/types.h>
#include <tools/long.hxx>

#include "scdllapi.h"
#include "calcmacros.hxx"

#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace com::sun::star::sheet {
    class XDimensionsSupplier;
    struct DataPilotFieldReference;
    struct DataPilotFieldSortInfo;
    struct DataPilotFieldAutoShowInfo;
    struct DataPilotFieldLayoutInfo;
}

class ScDPDimensionSaveData;
class ScDPTableData;
enum class ScGeneralFunction;
namespace sc { class PivotTableFormats; }

// classes to save Data Pilot settings

class ScDPSaveMember
{
private:
    OUString aName;
    std::optional<OUString> mpLayoutName; // custom name to be displayed in the table.
    sal_uInt16 nVisibleMode;
    sal_uInt16 nShowDetailsMode;

public:
    ScDPSaveMember(OUString aName);
    ScDPSaveMember(const ScDPSaveMember& r);
    ~ScDPSaveMember();

    bool operator== ( const ScDPSaveMember& r ) const;

    const OUString& GetName() const
        { return aName; }

    SC_DLLPUBLIC bool HasIsVisible() const;
    SC_DLLPUBLIC void SetIsVisible(bool bSet);
    bool GetIsVisible() const
        { return bool(nVisibleMode); }

    SC_DLLPUBLIC bool HasShowDetails() const;
    SC_DLLPUBLIC void SetShowDetails(bool bSet);
    bool GetShowDetails() const
        { return bool(nShowDetailsMode); }

    void SetName( const OUString& rNew ); // used if the source member was renamed (groups)

    SC_DLLPUBLIC void SetLayoutName( const OUString& rName );
    SC_DLLPUBLIC const std::optional<OUString> & GetLayoutName() const;
    void RemoveLayoutName();

    void WriteToSource( const css::uno::Reference<css::uno::XInterface>& xMember,
                            sal_Int32 nPosition );

#if DUMP_PIVOT_TABLE
    void Dump(int nIndent = 0) const;
#endif
};

class ScDPSaveDimension
{
private:
    OUString aName;
    std::optional<OUString> mpLayoutName;
    std::optional<OUString> mpSubtotalName;
    bool bIsDataLayout;
    bool bDupFlag;
    css::sheet::DataPilotFieldOrientation nOrientation;
    ScGeneralFunction nFunction; // for data dimensions
    tools::Long nUsedHierarchy;
    sal_uInt16 nShowEmptyMode; //! at level
    bool bRepeatItemLabels; //! at level
    bool bSubTotalDefault; //! at level
    std::vector<ScGeneralFunction> maSubTotalFuncs;
    std::unique_ptr<css::sheet::DataPilotFieldReference> pReferenceValue;
    std::unique_ptr<css::sheet::DataPilotFieldSortInfo> pSortInfo; // (level)
    std::unique_ptr<css::sheet::DataPilotFieldAutoShowInfo> pAutoShowInfo; // (level)
    std::unique_ptr<css::sheet::DataPilotFieldLayoutInfo> pLayoutInfo; // (level)

public:
    typedef std::unordered_set<OUString> MemberSetType;
    typedef std::vector<ScDPSaveMember*> MemberList;

private:
    std::unordered_map<OUString, std::unique_ptr<ScDPSaveMember>> maMemberHash;
    MemberList maMemberList;

public:
    ScDPSaveDimension(OUString aName, bool bDataLayout);
    ScDPSaveDimension(const ScDPSaveDimension& r);
    ~ScDPSaveDimension();

    bool operator== ( const ScDPSaveDimension& r ) const;

    const MemberList& GetMembers() const
        { return maMemberList; }

    void AddMember(std::unique_ptr<ScDPSaveMember> pMember);

    void SetDupFlag(bool bSet)
        { bDupFlag = bSet; }

    bool GetDupFlag() const
        { return bDupFlag; }

    const OUString& GetName() const
        { return aName; }

    bool IsDataLayout() const
        { return bIsDataLayout; }

    void SetName( const OUString& rNew ); // used if the source dim was renamed (groups)

    SC_DLLPUBLIC void SetOrientation(css::sheet::DataPilotFieldOrientation nNew);
    SC_DLLPUBLIC void SetSubTotals(std::vector<ScGeneralFunction> && rFuncs);
    tools::Long GetSubTotalsCount() const
        { return maSubTotalFuncs.size(); }

    ScGeneralFunction GetSubTotalFunc(tools::Long nIndex) const
        { return maSubTotalFuncs[nIndex]; }

    SC_DLLPUBLIC bool HasShowEmpty() const;
    SC_DLLPUBLIC void SetShowEmpty(bool bSet);
    bool GetShowEmpty() const
        { return bool(nShowEmptyMode); }

    void SetRepeatItemLabels(bool bSet);
    bool GetRepeatItemLabels() const
        { return bRepeatItemLabels; }

    SC_DLLPUBLIC void SetFunction(ScGeneralFunction nNew);
    ScGeneralFunction GetFunction() const
        { return nFunction; }

    void SetUsedHierarchy(tools::Long nNew);
    tools::Long GetUsedHierarchy() const
        { return nUsedHierarchy; }

    SC_DLLPUBLIC void SetLayoutName(const OUString& rName);
    SC_DLLPUBLIC const std::optional<OUString> & GetLayoutName() const;
    void RemoveLayoutName();
    SC_DLLPUBLIC void SetSubtotalName(const OUString& rName);
    SC_DLLPUBLIC const std::optional<OUString> & GetSubtotalName() const;
    void RemoveSubtotalName();

    bool IsMemberNameInUse(const OUString& rName) const;

    const css::sheet::DataPilotFieldReference* GetReferenceValue() const
        { return pReferenceValue.get(); }

    SC_DLLPUBLIC void SetReferenceValue(const css::sheet::DataPilotFieldReference* pNew);

    const css::sheet::DataPilotFieldSortInfo* GetSortInfo() const
        { return pSortInfo.get(); }

    SC_DLLPUBLIC void SetSortInfo(const css::sheet::DataPilotFieldSortInfo* pNew);
    const css::sheet::DataPilotFieldAutoShowInfo* GetAutoShowInfo() const
        { return pAutoShowInfo.get(); }

    SC_DLLPUBLIC void SetAutoShowInfo(const css::sheet::DataPilotFieldAutoShowInfo* pNew);
    const css::sheet::DataPilotFieldLayoutInfo* GetLayoutInfo() const
        { return pLayoutInfo.get(); }

    SC_DLLPUBLIC void SetLayoutInfo(const css::sheet::DataPilotFieldLayoutInfo* pNew);

    SC_DLLPUBLIC void SetCurrentPage( const OUString* pPage ); // NULL = no selection (all)
    OUString GetCurrentPage() const; // only for ODF compatibility

    css::sheet::DataPilotFieldOrientation GetOrientation() const
        { return nOrientation; }

    SC_DLLPUBLIC ScDPSaveMember* GetExistingMemberByName(const OUString& rName);

    /**
     * Get a member object by its name.  If one doesn't exist, create a new
     * object and return it.  This class manages the life cycle of all member
     * objects belonging to it, so <i>don't delete the returned instance.</i>
     *
     * @param rName member name
     *
     * @return pointer to the member object.
     */
    SC_DLLPUBLIC ScDPSaveMember* GetMemberByName(const OUString& rName);

    void SetMemberPosition( const OUString& rName, sal_Int32 nNewPos );

    void WriteToSource( const css::uno::Reference<css::uno::XInterface>& xDim );

    void UpdateMemberVisibility(const std::unordered_map< OUString, bool>& rData);

    SC_DLLPUBLIC bool HasInvisibleMember() const;

    void RemoveObsoleteMembers(const MemberSetType& rMembers);

#if DUMP_PIVOT_TABLE
    void Dump(int nIndent = 0) const;
#endif
};

class ScDPSaveData
{
    typedef std::unordered_map<OUString, size_t> DupNameCountType;
public:
    typedef std::unordered_map<OUString, size_t> DimOrderType;
    typedef std::vector<std::unique_ptr<ScDPSaveDimension>> DimsType;

private:
    DimsType m_DimList;
    DupNameCountType maDupNameCounts; /// keep track of number of duplicates in each name.
    std::unique_ptr<ScDPDimensionSaveData> mpDimensionData; // settings that create new dimensions
    sal_uInt16 mnColumnGrandMode;
    sal_uInt16 mnRowGrandMode;
    sal_uInt16 mnIgnoreEmptyMode;
    sal_uInt16 mnRepeatEmptyMode;
    bool mbFilterButton; // not passed to DataPilotSource
    bool mbDrillDown; // not passed to DataPilotSource
    bool mbExpandCollapse; // not passed to DataPilotSource

    /** if true, all dimensions already have all of their member instances
     *  created. */
    bool mbDimensionMembersBuilt;

    std::unique_ptr<sc::PivotTableFormats> mpFormats;
    std::optional<OUString> mpGrandTotalName;
    mutable std::unique_ptr<DimOrderType> mpDimOrder; // dimension order for row and column dimensions, to traverse result tree.

public:
    SC_DLLPUBLIC ScDPSaveData();
    ScDPSaveData(const ScDPSaveData& r);
    SC_DLLPUBLIC ~ScDPSaveData();

    ScDPSaveData& operator= ( const ScDPSaveData& r );

    bool operator== ( const ScDPSaveData& r ) const;

    SC_DLLPUBLIC bool hasFormats();
    SC_DLLPUBLIC sc::PivotTableFormats const& getFormats();
    SC_DLLPUBLIC void setFormats(sc::PivotTableFormats const& rPivotTableFormats);

    SC_DLLPUBLIC void SetGrandTotalName(const OUString& rName);
    SC_DLLPUBLIC const std::optional<OUString> & GetGrandTotalName() const;

    const DimsType& GetDimensions() const { return m_DimList; }

    /**
     * Get sort order map to sort row and column dimensions in order of
     * appearance. Row dimensions get sorted before column dimensions.  This
     * is used to traverse result tree, which is structured following this
     * order.
     */
    const DimOrderType& GetDimensionSortOrder() const;

    /**
     * Get all dimensions in a given orientation.  The order represents the
     * actual order of occurrence.  The returned list also includes data
     * layout dimension.
     *
     * @param eOrientation orientation
     * @param rDims (out) list of dimensions for specified orientation
     */
    SC_DLLPUBLIC void GetAllDimensionsByOrientation(
        css::sheet::DataPilotFieldOrientation eOrientation,
        std::vector<const ScDPSaveDimension*>& rDims) const;

    void AddDimension(ScDPSaveDimension* pDim);

    /**
     * Get a dimension object by its name.  <i>If one doesn't exist for the
     * given name, it creates a new one.</i>
     *
     * @param rName dimension name
     *
     * @return pointer to the dimension object.  The ScDPSaveData instance
     *         manages its life cycle; hence the caller must
     *         <i>not</i> delete this object.
     */
    SC_DLLPUBLIC ScDPSaveDimension* GetDimensionByName(const OUString& rName);
    SC_DLLPUBLIC ScDPSaveDimension* GetDataLayoutDimension();
    SC_DLLPUBLIC ScDPSaveDimension* GetExistingDataLayoutDimension() const;

    ScDPSaveDimension* DuplicateDimension(std::u16string_view rName);
    SC_DLLPUBLIC ScDPSaveDimension& DuplicateDimension(const ScDPSaveDimension& rDim);

    SC_DLLPUBLIC ScDPSaveDimension* GetExistingDimensionByName(std::u16string_view rName) const;
    SC_DLLPUBLIC ScDPSaveDimension* GetNewDimensionByName(const OUString& rName);

    void RemoveDimensionByName(const OUString& rName);

    ScDPSaveDimension* GetInnermostDimension(css::sheet::DataPilotFieldOrientation nOrientation);
    ScDPSaveDimension* GetFirstDimension(css::sheet::DataPilotFieldOrientation eOrientation);
    SC_DLLPUBLIC tools::Long GetDataDimensionCount() const;

    void SetPosition( ScDPSaveDimension* pDim, tools::Long nNew );
    SC_DLLPUBLIC void SetColumnGrand( bool bSet );
    bool GetColumnGrand() const { return bool(mnColumnGrandMode); }

    SC_DLLPUBLIC void SetRowGrand( bool bSet );
    bool GetRowGrand() const { return bool(mnRowGrandMode); }

    SC_DLLPUBLIC void SetIgnoreEmptyRows( bool bSet );
    bool GetIgnoreEmptyRows() const { return bool(mnIgnoreEmptyMode); }

    SC_DLLPUBLIC void SetRepeatIfEmpty( bool bSet );
    bool GetRepeatIfEmpty() const { return bool(mnRepeatEmptyMode); }

    SC_DLLPUBLIC void SetFilterButton( bool bSet );
    bool GetFilterButton() const { return mbFilterButton; }

    SC_DLLPUBLIC void SetDrillDown( bool bSet );
    bool GetDrillDown() const { return mbDrillDown; }

    SC_DLLPUBLIC void SetExpandCollapse( bool bSet );
    bool GetExpandCollapse() const { return mbExpandCollapse; }

    void WriteToSource( const css::uno::Reference<css::sheet::XDimensionsSupplier>& xSource );
    bool IsEmpty() const;

    const ScDPDimensionSaveData* GetExistingDimensionData() const { return mpDimensionData.get(); }

    void RemoveAllGroupDimensions( const OUString& rSrcDimName, std::vector<OUString>* pDeletedNames = nullptr );

    SC_DLLPUBLIC ScDPDimensionSaveData* GetDimensionData(); // create if not there
    SC_DLLPUBLIC void SetDimensionData( const ScDPDimensionSaveData* pNew ); // copied
    void BuildAllDimensionMembers(ScDPTableData* pData);
    void SyncAllDimensionMembers(ScDPTableData* pData);

    /**
     * Check whether a dimension has one or more invisible members.
     *
     * @param rDimName dimension name
     */
    SC_DLLPUBLIC bool HasInvisibleMember(std::u16string_view rDimName) const;

#if DUMP_PIVOT_TABLE
    void Dump() const;
#endif

private:
    void CheckDuplicateName(ScDPSaveDimension& rDim);
    void RemoveDuplicateNameCount(const OUString& rName);

    /**
     * Append a new original dimension. Not to be called to insert a duplicate
     * dimension.
     *
     * @param rName Dimension name. The name must be the original dimension
     *              name; not a duplicate dimension name.
     * @param bDataLayout true if this is a data layout dimension, false
     *                    otherwise.
     *
     * @return pointer to the new dimension just inserted.
     */
    ScDPSaveDimension* AppendNewDimension(const OUString& rName, bool bDataLayout);

    void DimensionsChanged();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
