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

#include <utility>
#include <xelink.hxx>

#include <algorithm>
#include <formula/errorcodes.hxx>
#include <oox/token/namespaces.hxx>
#include <oox/token/relationship.hxx>
#include <unotools/collatorwrapper.hxx>
#include <svl/numformat.hxx>
#include <svl/zforlist.hxx>
#include <sal/log.hxx>
#include <document.hxx>
#include <scextopt.hxx>
#include <externalrefmgr.hxx>
#include <tokenarray.hxx>
#include <xecontent.hxx>
#include <xeformula.hxx>
#include <xehelper.hxx>
#include <xllink.hxx>
#include <xltools.hxx>

#include <vector>
#include <memory>
#include <string_view>

using ::std::unique_ptr;
using ::std::vector;
using ::com::sun::star::uno::Any;

using namespace oox;

// *** Helper classes ***

// External names =============================================================

namespace {

/** This is a base class for any external name (i.e. add-in names or DDE links).
    @descr  Derived classes implement creation and export of the external names. */
class XclExpExtNameBase : public XclExpRecord, protected XclExpRoot
{
public:
    /** @param nFlags  The flags to export. */
    explicit            XclExpExtNameBase( const XclExpRoot& rRoot,
                            const OUString& rName, sal_uInt16 nFlags = 0 );

    /** Returns the name string of the external name. */
    const OUString& GetName() const { return maName; }

private:
    /** Writes the start of the record that is equal in all EXTERNNAME records and calls WriteAddData(). */
    virtual void        WriteBody( XclExpStream& rStrm ) override;
    /** Called to write additional data following the common record contents.
        @descr  Derived classes should overwrite this function to write their data. */
    virtual void        WriteAddData( XclExpStream& rStrm );

protected:
    OUString            maName;         /// Calc name (title) of the external name.
    XclExpStringRef     mxName;         /// Excel name (title) of the external name.
    sal_uInt16          mnFlags;        /// Flags for record export.
};

/** Represents an EXTERNNAME record for an add-in function name. */
class XclExpExtNameAddIn : public XclExpExtNameBase
{
public:
    explicit            XclExpExtNameAddIn( const XclExpRoot& rRoot, const OUString& rName );

private:
    /** Writes additional record contents. */
    virtual void        WriteAddData( XclExpStream& rStrm ) override;
};

/** Represents an EXTERNNAME record for a DDE link. */
class XclExpExtNameDde : public XclExpExtNameBase
{
public:
    explicit            XclExpExtNameDde( const XclExpRoot& rRoot, const OUString& rName,
                            sal_uInt16 nFlags, const ScMatrix* pResults = nullptr );

private:
    /** Writes additional record contents. */
    virtual void        WriteAddData( XclExpStream& rStrm ) override;

private:
    typedef std::shared_ptr< XclExpCachedMatrix > XclExpCachedMatRef;
    XclExpCachedMatRef  mxMatrix;       /// Cached results of the DDE link.
};

class XclExpSupbook;

class XclExpExtName : public XclExpExtNameBase
{
public:
    explicit            XclExpExtName( const XclExpRoot& rRoot, const XclExpSupbook& rSupbook, const OUString& rName,
                                       const ScExternalRefCache::TokenArrayRef& rArray );

    virtual void SaveXml(XclExpXmlStream& rStrm) override;

private:
    /** Writes additional record contents. */
    virtual void        WriteAddData( XclExpStream& rStrm ) override;

private:
    const XclExpSupbook&    mrSupbook;
    unique_ptr<ScTokenArray>  mpArray;
};

// List of external names =====================================================

/** List of all external names of a sheet. */
class XclExpExtNameBuffer : public XclExpRecordBase, protected XclExpRoot
{
public:
    explicit            XclExpExtNameBuffer( const XclExpRoot& rRoot );

    /** Inserts an add-in function name
        @return  The 1-based (Excel-like) list index of the name. */
    sal_uInt16          InsertAddIn( const OUString& rName );
    /** InsertEuroTool */
    sal_uInt16          InsertEuroTool( const OUString& rName );
    /** Inserts a DDE link.
        @return  The 1-based (Excel-like) list index of the DDE link. */
    sal_uInt16          InsertDde( std::u16string_view rApplic, std::u16string_view rTopic, const OUString& rItem );

    sal_uInt16          InsertExtName( const XclExpSupbook& rSupbook, const OUString& rName, const ScExternalRefCache::TokenArrayRef& rArray );

    /** Writes the EXTERNNAME record list. */
    virtual void        Save( XclExpStream& rStrm ) override;

    virtual void SaveXml(XclExpXmlStream& rStrm) override;

private:
    /** Returns the 1-based (Excel-like) list index of the external name or 0, if not found. */
    sal_uInt16          GetIndex( std::u16string_view rName ) const;
    /** Appends the passed newly crested external name.
        @return  The 1-based (Excel-like) list index of the appended name. */
    sal_uInt16          AppendNew( XclExpExtNameBase* pExtName );

    XclExpRecordList< XclExpExtNameBase >  maNameList;     /// The list with all EXTERNNAME records.
};

// Cached external cells ======================================================

/** Stores the contents of a consecutive row of external cells (record CRN). */
class XclExpCrn : public XclExpRecord
{
public:
    explicit            XclExpCrn( SCCOL nScCol, SCROW nScRow, const Any& rValue );

    /** Returns true, if the passed value could be appended to this record. */
    bool                InsertValue( SCCOL nScCol, SCROW nScRow, const Any& rValue );

    /** Writes the row and child elements. */
    virtual void        SaveXml( XclExpXmlStream& rStrm ) override;

private:
    virtual void        WriteBody( XclExpStream& rStrm ) override;

    static void         WriteBool( XclExpStream& rStrm, bool bValue );
    static void         WriteDouble( XclExpStream& rStrm, double fValue );
    static void         WriteString( XclExpStream& rStrm, const OUString& rValue );
    static void         WriteError( XclExpStream& rStrm, sal_uInt8 nErrCode );
    static void         WriteEmpty( XclExpStream& rStrm );

private:
    typedef ::std::vector< Any > CachedValues;

    CachedValues        maValues;   /// All cached values.
    SCCOL               mnScCol;    /// Column index of the first external cell.
    SCROW               mnScRow;    /// Row index of the external cells.
};

class XclExpCrnList;

/** Represents the record XCT which is the header record of a CRN record list.
 */
class XclExpXct : public XclExpRecordBase, protected XclExpRoot
{
public:
    explicit            XclExpXct( const XclExpRoot& rRoot,
                            const OUString& rTabName, sal_uInt16 nSBTab,
                            ScExternalRefCache::TableTypeRef xCacheTable );

    /** Returns the external sheet name. */
    const XclExpString& GetTabName() const { return maTabName; }

    /** Stores all cells in the given range in the CRN list. */
    void                StoreCellRange( const ScRange& rRange );

    void                StoreCell_( const ScAddress& rCell );
    void                StoreCellRange_( const ScRange& rRange );

    /** Writes the XCT and all CRN records. */
    virtual void        Save( XclExpStream& rStrm ) override;

    /** Writes the sheetDataSet and child elements. */
    virtual void        SaveXml( XclExpXmlStream& rStrm ) override;

private:
    ScExternalRefCache::TableTypeRef mxCacheTable;
    ScMarkData          maUsedCells;    /// Contains addresses of all stored cells.
    ScRange             maBoundRange;   /// Bounding box of maUsedCells.
    XclExpString        maTabName;      /// Sheet name of the external sheet.
    sal_uInt16          mnSBTab;        /// Referred sheet index in SUPBOOK record.

    /** Build the internal representation of records to be saved as BIFF or OOXML. */
    bool                BuildCrnList( XclExpCrnList& rCrnRecs );
};

// External documents (EXTERNSHEET/SUPBOOK), base class =======================

/** Base class for records representing external sheets/documents.

    In BIFF5/BIFF7, this record is the EXTERNSHEET record containing one sheet
    of the own or an external document. In BIFF8, this record is the SUPBOOK
    record representing the entire own or external document with all referenced
    sheets.
 */
class XclExpExternSheetBase : public XclExpRecord, protected XclExpRoot
{
public:
    explicit            XclExpExternSheetBase( const XclExpRoot& rRoot,
                            sal_uInt16 nRecId, sal_uInt32 nRecSize = 0 );

protected:
    /** Creates and returns the list of EXTERNNAME records. */
    XclExpExtNameBuffer& GetExtNameBuffer();
    /** Writes the list of EXTERNNAME records. */
    void                WriteExtNameBuffer( XclExpStream& rStrm );
    /** Writes the list of externalName elements. */
    void                WriteExtNameBufferXml( XclExpXmlStream& rStrm );

protected:
    typedef std::shared_ptr< XclExpExtNameBuffer >   XclExpExtNameBfrRef;
    XclExpExtNameBfrRef mxExtNameBfr;   /// List of EXTERNNAME records.
};

// External documents (EXTERNSHEET, BIFF5/BIFF7) ==============================

/** Represents an EXTERNSHEET record containing the URL and sheet name of a sheet.
    @descr  This class is used up to BIFF7 only, writing a BIFF8 EXTERNSHEET
        record is implemented directly in the link manager. */
class XclExpExternSheet : public XclExpExternSheetBase
{
public:
    /** Creates an EXTERNSHEET record containing a special code (i.e. own document or sheet). */
    explicit            XclExpExternSheet( const XclExpRoot& rRoot, sal_Unicode cCode );
    /** Creates an EXTERNSHEET record referring to an internal sheet. */
    explicit            XclExpExternSheet( const XclExpRoot& rRoot, std::u16string_view rTabName );

    /** Finds or inserts an EXTERNNAME record for add-ins.
        @return  The 1-based EXTERNNAME record index; or 0, if the record list is full. */
    sal_uInt16          InsertAddIn( const OUString& rName );

    /** Writes the EXTERNSHEET and all EXTERNNAME, XCT and CRN records. */
    virtual void        Save( XclExpStream& rStrm ) override;

private:
    /** Initializes the record data with the passed encoded URL. */
    void                Init( std::u16string_view rEncUrl );
    /** Writes the contents of the EXTERNSHEET  record. */
    virtual void        WriteBody( XclExpStream& rStrm ) override;

private:
    XclExpString        maTabName;      /// The name of the sheet.
};

// External documents (SUPBOOK, BIFF8) ========================================

/** The SUPBOOK record contains data for an external document (URL, sheet names, external values). */
class XclExpSupbook : public XclExpExternSheetBase
{
public:
    /** Creates a SUPBOOK record for internal references. */
    explicit            XclExpSupbook( const XclExpRoot& rRoot, sal_uInt16 nXclTabCount );
    /** Creates a SUPBOOK record for add-in functions. */
    explicit            XclExpSupbook( const XclExpRoot& rRoot );
    /** EUROTOOL SUPBOOK */
    explicit            XclExpSupbook( const XclExpRoot& rRoot, const OUString& rUrl, XclSupbookType );
    /** Creates a SUPBOOK record for an external document. */
    explicit            XclExpSupbook( const XclExpRoot& rRoot, const OUString& rUrl );
    /** Creates a SUPBOOK record for a DDE link. */
    explicit            XclExpSupbook( const XclExpRoot& rRoot, const OUString& rApplic, const OUString& rTopic );

    /** Returns true, if this SUPBOOK contains the passed URL of an external document. */
    bool                IsUrlLink( std::u16string_view rUrl ) const;
    /** Returns true, if this SUPBOOK contains the passed DDE link. */
    bool                IsDdeLink( std::u16string_view rApplic, std::u16string_view rTopic ) const;
    /** Fills the passed reference log entry with the URL and sheet names. */
    void                FillRefLogEntry( XclExpRefLogEntry& rRefLogEntry,
                            sal_uInt16 nFirstSBTab, sal_uInt16 nLastSBTab ) const;

    /** Stores all cells in the given range in the CRN list of the specified SUPBOOK sheet. */
    void                StoreCellRange( const ScRange& rRange, sal_uInt16 nSBTab );

    void                StoreCell_( sal_uInt16 nSBTab, const ScAddress& rCell );
    void                StoreCellRange_( sal_uInt16 nSBTab, const ScRange& rRange );

    sal_uInt16          GetTabIndex( const OUString& rTabName ) const;
    sal_uInt16          GetTabCount() const;

    /** Inserts a new sheet name into the SUPBOOK and returns the SUPBOOK internal sheet index. */
    sal_uInt16          InsertTabName( const OUString& rTabName, ScExternalRefCache::TableTypeRef const & xCacheTable );
    /** Finds or inserts an EXTERNNAME record for add-ins.
        @return  The 1-based EXTERNNAME record index; or 0, if the record list is full. */
    sal_uInt16          InsertAddIn( const OUString& rName );
    /** InsertEuroTool */
    sal_uInt16          InsertEuroTool( const OUString& rName );
    /** Finds or inserts an EXTERNNAME record for DDE links.
        @return  The 1-based EXTERNNAME record index; or 0, if the record list is full. */
    sal_uInt16          InsertDde( const OUString& rItem );

    sal_uInt16          InsertExtName( const OUString& rName, const ScExternalRefCache::TokenArrayRef& rArray );

    /** Get the type of record. */
    XclSupbookType      GetType() const;

    /** For references to an external document, 1-based OOXML file ID. */
    sal_uInt16          GetFileId() const;

    /** For references to an external document. */
    const OUString&     GetUrl() const;

    /** Writes the SUPBOOK and all EXTERNNAME, XCT and CRN records. */
    virtual void        Save( XclExpStream& rStrm ) override;

    /** Writes the externalBook and all child elements. */
    virtual void        SaveXml( XclExpXmlStream& rStrm ) override;

private:
    /** Returns the sheet name inside of this SUPBOOK. */
    const XclExpString* GetTabName( sal_uInt16 nSBTab ) const;

    /** Writes the SUPBOOK record contents. */
    virtual void        WriteBody( XclExpStream& rStrm ) override;

private:
    typedef XclExpRecordList< XclExpXct >   XclExpXctList;
    typedef XclExpXctList::RecordRefType    XclExpXctRef;

    XclExpXctList       maXctList;      /// List of XCT records (which contain CRN records).
    OUString            maUrl;          /// URL of the external document or application name for DDE.
    OUString            maDdeTopic;     /// Topic of a DDE link.
    XclExpString        maUrlEncoded;   /// Document name encoded for Excel.
    XclSupbookType      meType;         /// Type of this SUPBOOK record.
    sal_uInt16          mnXclTabCount;  /// Number of internal sheets.
    sal_uInt16          mnFileId;       /// 1-based external reference file ID for OOXML
};

// All SUPBOOKS in a document =================================================

/** This struct contains a sheet index range for 3D references.
    @descr  This reference consists of an index to a SUPBOOK record and indexes
    to SUPBOOK sheet names. */
struct XclExpXti
{
    sal_uInt16          mnSupbook;      /// Index to SUPBOOK record.
    sal_uInt16          mnFirstSBTab;   /// Index to the first sheet of the range in the SUPBOOK.
    sal_uInt16          mnLastSBTab;    /// Index to the last sheet of the range in the SUPBOOK.

    explicit     XclExpXti() : mnSupbook( 0 ), mnFirstSBTab( 0 ), mnLastSBTab( 0 ) {}
    explicit     XclExpXti( sal_uInt16 nSupbook, sal_uInt16 nFirstSBTab, sal_uInt16 nLastSBTab ) :
                            mnSupbook( nSupbook ), mnFirstSBTab( nFirstSBTab ), mnLastSBTab( nLastSBTab ) {}

    /** Writes this XTI structure (inside of the EXTERNSHEET record). */
    void         Save( XclExpStream& rStrm ) const
                            { rStrm << mnSupbook << mnFirstSBTab << mnLastSBTab; }
};

bool operator==( const XclExpXti& rLeft, const XclExpXti& rRight )
{
    return
        (rLeft.mnSupbook    == rRight.mnSupbook)    &&
        (rLeft.mnFirstSBTab == rRight.mnFirstSBTab) &&
        (rLeft.mnLastSBTab  == rRight.mnLastSBTab);
}

/** Contains a list of all SUPBOOK records and index arrays of external sheets. */
class XclExpSupbookBuffer : public XclExpRecordBase, protected XclExpRoot
{
public:
    explicit            XclExpSupbookBuffer( const XclExpRoot& rRoot );

    /** Finds SUPBOOK index and SUPBOOK sheet range from given Excel sheet range.
        @return  An XTI structure containing SUPBOOK and sheet indexes. */
    XclExpXti           GetXti( sal_uInt16 nFirstXclTab, sal_uInt16 nLastXclTab,
                            XclExpRefLogEntry* pRefLogEntry = nullptr ) const;

    /** Stores all cells in the given range in a CRN record list. */
    void                StoreCellRange( const ScRange& rRange );

    void                StoreCell( sal_uInt16 nFileId, const OUString& rTabName, const ScAddress& rCell );
    void                StoreCellRange( sal_uInt16 nFileId, const OUString& rTabName, const ScRange& rRange );

    /** Finds or inserts an EXTERNNAME record for an add-in function name.
     * @return an optional struct, containing [mnSupbook, mnExtName]
       rnSupbook  Returns the index of the SUPBOOK record which contains the DDE link.
       rnExtName  Returns the 1-based EXTERNNAME record index.
    */
    std::optional<XclExpSBIndex> InsertAddIn(const OUString& rName);
    /** InsertEuroTool */
    std::optional<XclExpSBIndex> InsertEuroTool(const OUString& rName);
    /** Finds or inserts an EXTERNNAME record for DDE links.
     * @return an optional struct, containing [mnSupbook, mnExtName]
     * rnSupbook  Returns the index of the SUPBOOK record which contains the DDE link.
       rnExtName  Returns the 1-based EXTERNNAME record index.
    */
    std::optional<XclExpSBIndex> InsertDde(const OUString& rApplic, const OUString& rTopic, const OUString& rItem);

    std::optional<XclExpSBIndex> InsertExtName(const OUString& rUrl, const OUString& rName,
                      const ScExternalRefCache::TokenArrayRef& rArray);

    XclExpXti           GetXti( sal_uInt16 nFileId, const OUString& rTabName, sal_uInt16 nXclTabSpan,
                                XclExpRefLogEntry* pRefLogEntry );

    /** Writes all SUPBOOK records with their sub records. */
    virtual void        Save( XclExpStream& rStrm ) override;

    /** Writes all externalBook elements with their child elements to OOXML. */
    virtual void        SaveXml( XclExpXmlStream& rStrm ) override;

    /** Whether we need to write externalReferences or not. */
    bool                HasExternalReferences() const;

private:
    typedef XclExpRecordList< XclExpSupbook >   XclExpSupbookList;
    typedef XclExpSupbookList::RecordRefType    XclExpSupbookRef;

private:
    /** Searches for the SUPBOOK record containing the passed document URL.
        @param rxSupbook  (out-param) Returns a reference to the SUPBOOK record, or 0.
        @return  List index, if the SUPBOOK record exists (out-parameters are valid). */
    std::optional<sal_uInt16> GetSupbookUrl(XclExpSupbookRef& rxSupbook,
                                            std::u16string_view rUrl) const;
    /** Searches for the SUPBOOK record containing the passed DDE link.
        @param rxSupbook  (out-param) Returns a reference to the SUPBOOK record, or 0.
        @return  List index, if the SUPBOOK record exists (out-parameters are valid). */
    std::optional<sal_uInt16> GetSupbookDde(XclExpSupbookRef& rxSupbook,
                                            std::u16string_view rApplic,
                                            std::u16string_view rTopic) const;

    /** Appends a new SUPBOOK to the list.
        @return  The list index of the SUPBOOK record. */
    sal_uInt16          Append( XclExpSupbookRef const & xSupbook );

private:
    XclExpSupbookList   maSupbookList;      /// List of all SUPBOOK records.
    std::vector< XclExpSBIndex >
                        maSBIndexVec;       /// SUPBOOK and sheet name index for each Excel sheet.
    sal_uInt16          mnOwnDocSB;         /// Index to SUPBOOK for own document.
    sal_uInt16          mnAddInSB;          /// Index to add-in SUPBOOK.
};

}

// Export link manager ========================================================

/** Abstract base class for implementation classes of the link manager. */
class XclExpLinkManagerImpl : protected XclExpRoot
{
public:
    /** Derived classes search for an EXTSHEET structure for the given Calc sheet range. */
    virtual void        FindExtSheet( sal_uInt16& rnExtSheet,
                            sal_uInt16& rnFirstXclTab, sal_uInt16& rnLastXclTab,
                            SCTAB nFirstScTab, SCTAB nLastScTab,
                            XclExpRefLogEntry* pRefLogEntry ) = 0;
    /** Derived classes search for a special EXTERNSHEET index for the own document. */
    virtual sal_uInt16  FindExtSheet( sal_Unicode cCode ) = 0;

    virtual void FindExtSheet( sal_uInt16 nFileId, const OUString& rTabName, sal_uInt16 nXclTabSpan,
                               sal_uInt16& rnExtSheet, sal_uInt16& rnFirstSBTab, sal_uInt16& rnLastSBTab,
                               XclExpRefLogEntry* pRefLogEntry ) = 0;

    /** Derived classes store all cells in the given range in a CRN record list. */
    virtual void StoreCellRange( const ScSingleRefData& rRef1, const ScSingleRefData& rRef2, const ScAddress& rPos ) = 0;

    virtual void StoreCell( sal_uInt16 nFileId, const OUString& rTabName, const ScAddress& rPos ) = 0;
    virtual void StoreCellRange( sal_uInt16 nFileId, const OUString& rTabName, const ScRange& rRange ) = 0;

    /** Derived classes find or insert an EXTERNNAME record for an add-in function name. */
    virtual std::optional<XclExpSBIndex> InsertAddIn(const OUString& rName) = 0;
    /** InsertEuroTool */
    virtual std::optional<XclExpSBIndex> InsertEuroTool(const OUString& rName)
        = 0;

    /** Derived classes find or insert an EXTERNNAME record for DDE links. */
    virtual std::optional<XclExpSBIndex> InsertDde(const OUString& rApplic, const OUString& rTopic, const OUString& rItem) = 0;

    virtual std::optional<XclExpSBIndex> InsertExtName(const OUString& rUrl, const OUString& rName,
                      const ScExternalRefCache::TokenArrayRef& rArray)
        = 0;

    /** Derived classes write the entire link table to the passed stream. */
    virtual void        Save( XclExpStream& rStrm ) = 0;

    /** Derived classes write the entire link table to the passed OOXML stream. */
    virtual void        SaveXml( XclExpXmlStream& rStrm ) = 0;

protected:
    explicit            XclExpLinkManagerImpl( const XclExpRoot& rRoot );
};

namespace {

/** Implementation of the link manager for BIFF5/BIFF7. */
class XclExpLinkManagerImpl5 : public XclExpLinkManagerImpl
{
public:
    explicit            XclExpLinkManagerImpl5( const XclExpRoot& rRoot );

    virtual void        FindExtSheet( sal_uInt16& rnExtSheet,
                            sal_uInt16& rnFirstXclTab, sal_uInt16& rnLastXclTab,
                            SCTAB nFirstScTab, SCTAB nLastScTab,
                            XclExpRefLogEntry* pRefLogEntry ) override;
    virtual sal_uInt16  FindExtSheet( sal_Unicode cCode ) override;

    virtual void FindExtSheet( sal_uInt16 nFileId, const OUString& rTabName, sal_uInt16 nXclTabSpan,
                               sal_uInt16& rnExtSheet, sal_uInt16& rnFirstSBTab, sal_uInt16& rnLastSBTab,
                               XclExpRefLogEntry* pRefLogEntry ) override;

    virtual void StoreCellRange( const ScSingleRefData& rRef1, const ScSingleRefData& rRef2, const ScAddress& rPos ) override;

    virtual void StoreCell( sal_uInt16 nFileId, const OUString& rTabName, const ScAddress& rPos ) override;
    virtual void StoreCellRange( sal_uInt16 nFileId, const OUString& rTabName, const ScRange& rRange ) override;

    virtual std::optional<XclExpSBIndex> InsertAddIn(const OUString& rName) override;

    /** InsertEuroTool */
    virtual std::optional<XclExpSBIndex> InsertEuroTool(const OUString& rName) override;

    virtual std::optional<XclExpSBIndex> InsertDde(const OUString& rApplic, const OUString& rTopic, const OUString& rItem) override;

    virtual std::optional<XclExpSBIndex> InsertExtName(const OUString& rUrl, const OUString& rName,
                      const ScExternalRefCache::TokenArrayRef& rArray) override;

    virtual void        Save( XclExpStream& rStrm ) override;

    virtual void        SaveXml( XclExpXmlStream& rStrm ) override;

private:
    typedef XclExpRecordList< XclExpExternSheet >   XclExpExtSheetList;
    typedef XclExpExtSheetList::RecordRefType       XclExpExtSheetRef;
    typedef ::std::map< SCTAB, sal_uInt16 >         XclExpIntTabMap;
    typedef ::std::map< sal_Unicode, sal_uInt16 >   XclExpCodeMap;

private:
    /** Returns the number of EXTERNSHEET records. */
    sal_uInt16          GetExtSheetCount() const;

    /** Appends an internal EXTERNSHEET record and returns the one-based index. */
    sal_uInt16          AppendInternal( XclExpExtSheetRef const & xExtSheet );
    /** Creates all EXTERNSHEET records for internal sheets on first call. */
    void                CreateInternal();

    /** Returns the specified internal EXTERNSHEET record. */
    XclExpExtSheetRef   GetInternal( sal_uInt16 nExtSheet );
    /** Returns the EXTERNSHEET index of an internal Calc sheet, or a deleted reference. */
    XclExpExtSheetRef   FindInternal( sal_uInt16& rnExtSheet, sal_uInt16& rnXclTab, SCTAB nScTab );
    /** Finds or creates the EXTERNSHEET index of an internal special EXTERNSHEET. */
    XclExpExtSheetRef   FindInternal( sal_uInt16& rnExtSheet, sal_Unicode cCode );

private:
    XclExpExtSheetList  maExtSheetList;     /// List with EXTERNSHEET records.
    XclExpIntTabMap     maIntTabMap;        /// Maps internal Calc sheets to EXTERNSHEET records.
    XclExpCodeMap       maCodeMap;          /// Maps special external codes to EXTERNSHEET records.
};

/** Implementation of the link manager for BIFF8 and OOXML. */
class XclExpLinkManagerImpl8 : public XclExpLinkManagerImpl
{
public:
    explicit            XclExpLinkManagerImpl8( const XclExpRoot& rRoot );

    virtual void        FindExtSheet( sal_uInt16& rnExtSheet,
                            sal_uInt16& rnFirstXclTab, sal_uInt16& rnLastXclTab,
                            SCTAB nFirstScTab, SCTAB nLastScTab,
                            XclExpRefLogEntry* pRefLogEntry ) override;
    virtual sal_uInt16  FindExtSheet( sal_Unicode cCode ) override;

    virtual void FindExtSheet( sal_uInt16 nFileId, const OUString& rTabName, sal_uInt16 nXclTabSpan,
                               sal_uInt16& rnExtSheet, sal_uInt16& rnFirstSBTab, sal_uInt16& rnLastSBTab,
                               XclExpRefLogEntry* pRefLogEntry ) override;

    virtual void StoreCellRange( const ScSingleRefData& rRef1, const ScSingleRefData& rRef2, const ScAddress& rPos ) override;

    virtual void StoreCell( sal_uInt16 nFileId, const OUString& rTabName, const ScAddress& rPos ) override;
    virtual void StoreCellRange( sal_uInt16 nFileId, const OUString& rTabName, const ScRange& rRange ) override;

    virtual std::optional<XclExpSBIndex> InsertAddIn(const OUString& rName) override;
    /** InsertEuroTool */
    virtual std::optional<XclExpSBIndex> InsertEuroTool(const OUString& rName) override;

    virtual std::optional<XclExpSBIndex> InsertDde(const OUString& rApplic, const OUString& rTopic, const OUString& rItem) override;

    virtual std::optional<XclExpSBIndex> InsertExtName(const OUString& rUrl, const OUString& rName,
                      const ScExternalRefCache::TokenArrayRef& rArray) override;

    virtual void        Save( XclExpStream& rStrm ) override;

    virtual void        SaveXml( XclExpXmlStream& rStrm ) override;

private:
    /** Searches for or inserts a new XTI structure.
        @return  The 0-based list index of the XTI structure. */
    sal_uInt16          InsertXti( const XclExpXti& rXti );

private:

    XclExpSupbookBuffer       maSBBuffer;     /// List of all SUPBOOK records.
    std::vector< XclExpXti >  maXtiVec;       /// List of XTI structures for the EXTERNSHEET record.
};

}

// *** Implementation ***

// Excel sheet indexes ========================================================


XclExpTabInfo::XclExpTabInfo( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot ),
    mnScCnt( 0 ),
    mnXclCnt( 0 ),
    mnXclExtCnt( 0 ),
    mnXclSelCnt( 0 ),
    mnDisplXclTab( 0 ),
    mnFirstVisXclTab( 0 )
{
    ScDocument& rDoc = GetDoc();
    ScExtDocOptions& rDocOpt = GetExtDocOptions();

    mnScCnt = rDoc.GetTableCount();

    SCTAB nScTab;
    SCTAB nFirstVisScTab = SCTAB_INVALID;   // first visible sheet
    SCTAB nFirstExpScTab = SCTAB_INVALID;   // first exported sheet

    // --- initialize the flags in the index buffer ---

    maTabInfoVec.resize( mnScCnt );
    for( nScTab = 0; nScTab < mnScCnt; ++nScTab )
    {
        // ignored sheets (skipped by export, with invalid Excel sheet index)
        if( rDoc.IsScenario( nScTab ) )
        {
            SetFlag( nScTab, ExcTabBufFlags::Ignore );
        }

        // external sheets (skipped, but with valid Excel sheet index for ref's)
        else if( rDoc.GetLinkMode( nScTab ) == ScLinkMode::VALUE )
        {
            SetFlag( nScTab, ExcTabBufFlags::Extern );
        }

        // exported sheets
        else
        {
            // sheet name
            rDoc.GetName( nScTab, maTabInfoVec[ nScTab ].maScName );

            // remember first exported sheet
            if( nFirstExpScTab == SCTAB_INVALID )
               nFirstExpScTab = nScTab;
            // remember first visible exported sheet
            if( (nFirstVisScTab == SCTAB_INVALID) && rDoc.IsVisible( nScTab ) )
               nFirstVisScTab = nScTab;

            // sheet visible (only exported sheets)
            SetFlag( nScTab, ExcTabBufFlags::Visible, rDoc.IsVisible( nScTab ) );

            // sheet selected (only exported sheets)
            if( const ScExtTabSettings* pTabSett = rDocOpt.GetTabSettings( nScTab ) )
                SetFlag( nScTab, ExcTabBufFlags::Selected, pTabSett->mbSelected );

            // sheet mirrored (only exported sheets)
            SetFlag( nScTab, ExcTabBufFlags::Mirrored, rDoc.IsLayoutRTL( nScTab ) );
        }
    }

    // --- visible/selected sheets ---

    SCTAB nDisplScTab = rDocOpt.GetDocSettings().mnDisplTab;

    // missing viewdata at embedded XLSX OLE objects
    if (nDisplScTab == -1 )
        nDisplScTab = rDoc.GetVisibleTab();

    // find first visible exported sheet
    if( (nFirstVisScTab == SCTAB_INVALID) || !IsExportTab( nFirstVisScTab ) )
    {
        // no exportable visible sheet -> use first exportable sheet
        nFirstVisScTab = nFirstExpScTab;
        if( (nFirstVisScTab == SCTAB_INVALID) || !IsExportTab( nFirstVisScTab ) )
        {
            // no exportable sheet at all -> use active sheet and export it
            nFirstVisScTab = nDisplScTab;
            SetFlag( nFirstVisScTab, ExcTabBufFlags::SkipMask, false ); // clear skip flags
        }
        SetFlag( nFirstVisScTab, ExcTabBufFlags::Visible ); // must be visible, even if originally hidden
    }

    // find currently displayed sheet
    if( !IsExportTab( nDisplScTab ) )   // selected sheet not exported (i.e. scenario) -> use first visible
        nDisplScTab = nFirstVisScTab;
    SetFlag( nDisplScTab, ExcTabBufFlags::Visible | ExcTabBufFlags::Selected );

    // number of selected sheets
    for( nScTab = 0; nScTab < mnScCnt; ++nScTab )
        if( IsSelectedTab( nScTab ) )
            ++mnXclSelCnt;

    // --- calculate resulting Excel sheet indexes ---

    CalcXclIndexes();
    mnFirstVisXclTab = GetXclTab( nFirstVisScTab );
    mnDisplXclTab = GetXclTab( nDisplScTab );

    // --- sorted vectors for index lookup ---

    CalcSortedIndexes();
}

bool XclExpTabInfo::IsExportTab( SCTAB nScTab ) const
{
    /*  Check sheet index before to avoid assertion in GetFlag(). */
    return (nScTab < mnScCnt && nScTab >= 0) && !GetFlag( nScTab, ExcTabBufFlags::SkipMask );
}

bool XclExpTabInfo::IsExternalTab( SCTAB nScTab ) const
{
    /*  Check sheet index before to avoid assertion (called from formula
        compiler also for deleted references). */
    return (nScTab < mnScCnt && nScTab >= 0) && GetFlag( nScTab, ExcTabBufFlags::Extern );
}

bool XclExpTabInfo::IsVisibleTab( SCTAB nScTab ) const
{
    return GetFlag( nScTab, ExcTabBufFlags::Visible );
}

bool XclExpTabInfo::IsSelectedTab( SCTAB nScTab ) const
{
    return GetFlag( nScTab, ExcTabBufFlags::Selected );
}

bool XclExpTabInfo::IsDisplayedTab( SCTAB nScTab ) const
{
    OSL_ENSURE( nScTab < mnScCnt && nScTab >= 0, "XclExpTabInfo::IsActiveTab - sheet out of range" );
    return GetXclTab( nScTab ) == mnDisplXclTab;
}

bool XclExpTabInfo::IsMirroredTab( SCTAB nScTab ) const
{
    return GetFlag( nScTab, ExcTabBufFlags::Mirrored );
}

OUString XclExpTabInfo::GetScTabName( SCTAB nScTab ) const
{
    OSL_ENSURE( nScTab < mnScCnt && nScTab >= 0, "XclExpTabInfo::IsActiveTab - sheet out of range" );
    return (nScTab < mnScCnt && nScTab >= 0) ? maTabInfoVec[ nScTab ].maScName : OUString();
}

sal_uInt16 XclExpTabInfo::GetXclTab( SCTAB nScTab ) const
{
    return (nScTab < mnScCnt && nScTab >= 0) ? maTabInfoVec[ nScTab ].mnXclTab : EXC_TAB_DELETED;
}

SCTAB XclExpTabInfo::GetRealScTab( SCTAB nSortedScTab ) const
{
    OSL_ENSURE( nSortedScTab < mnScCnt && nSortedScTab >= 0, "XclExpTabInfo::GetRealScTab - sheet out of range" );
    return (nSortedScTab < mnScCnt && nSortedScTab >= 0) ? maFromSortedVec[ nSortedScTab ] : SCTAB_INVALID;
}

bool XclExpTabInfo::GetFlag( SCTAB nScTab, ExcTabBufFlags nFlags ) const
{
    OSL_ENSURE( nScTab < mnScCnt && nScTab >= 0, "XclExpTabInfo::GetFlag - sheet out of range" );
    return (nScTab < mnScCnt && nScTab >= 0) && (maTabInfoVec[ nScTab ].mnFlags & nFlags);
}

void XclExpTabInfo::SetFlag( SCTAB nScTab, ExcTabBufFlags nFlags, bool bSet )
{
    OSL_ENSURE( nScTab < mnScCnt && nScTab >= 0, "XclExpTabInfo::SetFlag - sheet out of range" );
    if( nScTab < mnScCnt && nScTab >= 0 )
    {
        if (bSet)
            maTabInfoVec[ nScTab ].mnFlags |= nFlags;
        else
            maTabInfoVec[ nScTab ].mnFlags &= ~nFlags;
    }
}

void XclExpTabInfo::CalcXclIndexes()
{
    sal_uInt16 nXclTab = 0;
    SCTAB nScTab = 0;

    // --- pass 1: process regular sheets ---
    for( nScTab = 0; nScTab < mnScCnt; ++nScTab )
    {
        if( IsExportTab( nScTab ) )
        {
            maTabInfoVec[ nScTab ].mnXclTab = nXclTab;
            ++nXclTab;
        }
        else
            maTabInfoVec[ nScTab ].mnXclTab = EXC_TAB_DELETED;
    }
    mnXclCnt = nXclTab;

    // --- pass 2: process external sheets (nXclTab continues) ---
    for( nScTab = 0; nScTab < mnScCnt; ++nScTab )
    {
        if( IsExternalTab( nScTab ) )
        {
            maTabInfoVec[ nScTab ].mnXclTab = nXclTab;
            ++nXclTab;
            ++mnXclExtCnt;
        }
    }

    // result: first occur all exported sheets, followed by all external sheets
}

typedef ::std::pair< OUString, SCTAB > XclExpTabName;

namespace {

struct XclExpTabNameSort {
    bool operator ()( const XclExpTabName& rArg1, const XclExpTabName& rArg2 )
    {
        // compare the sheet names only
        return ScGlobal::GetCollator().compareString( rArg1.first, rArg2.first ) < 0;
    }
};

}

void XclExpTabInfo::CalcSortedIndexes()
{
    ScDocument& rDoc = GetDoc();
    ::std::vector< XclExpTabName > aVec( mnScCnt );
    SCTAB nScTab;

    // fill with sheet names
    for( nScTab = 0; nScTab < mnScCnt; ++nScTab )
    {
        rDoc.GetName( nScTab, aVec[ nScTab ].first );
        aVec[ nScTab ].second = nScTab;
    }
    ::std::sort( aVec.begin(), aVec.end(), XclExpTabNameSort() );

    // fill index vectors from sorted sheet name vector
    maFromSortedVec.resize( mnScCnt );
    maToSortedVec.resize( mnScCnt );
    for( nScTab = 0; nScTab < mnScCnt; ++nScTab )
    {
        maFromSortedVec[ nScTab ] = aVec[ nScTab ].second;
        maToSortedVec[ aVec[ nScTab ].second ] = nScTab;
    }
}

// External names =============================================================

XclExpExtNameBase::XclExpExtNameBase(
        const XclExpRoot& rRoot, const OUString& rName, sal_uInt16 nFlags ) :
    XclExpRecord( EXC_ID_EXTERNNAME ),
    XclExpRoot( rRoot ),
    maName( rName ),
    mxName( XclExpStringHelper::CreateString( rRoot, rName, XclStrFlags::EightBitLength ) ),
    mnFlags( nFlags )
{
    OSL_ENSURE( maName.getLength() <= 255, "XclExpExtNameBase::XclExpExtNameBase - string too long" );
    SetRecSize( 6 + mxName->GetSize() );
}

void XclExpExtNameBase::WriteBody( XclExpStream& rStrm )
{
    rStrm   << mnFlags
            << sal_uInt32( 0 )
            << *mxName;
    WriteAddData( rStrm );
}

void XclExpExtNameBase::WriteAddData( XclExpStream& /*rStrm*/ )
{
}

XclExpExtNameAddIn::XclExpExtNameAddIn( const XclExpRoot& rRoot, const OUString& rName ) :
    XclExpExtNameBase( rRoot, rName )
{
    AddRecSize( 4 );
}

void XclExpExtNameAddIn::WriteAddData( XclExpStream& rStrm )
{
    // write a #REF! error formula
    rStrm << sal_uInt16( 2 ) << EXC_TOKID_ERR << EXC_ERR_REF;
}

XclExpExtNameDde::XclExpExtNameDde( const XclExpRoot& rRoot,
        const OUString& rName, sal_uInt16 nFlags, const ScMatrix* pResults ) :
    XclExpExtNameBase( rRoot, rName, nFlags )
{
    if( pResults )
    {
        mxMatrix = std::make_shared<XclExpCachedMatrix>( *pResults );
        AddRecSize( mxMatrix->GetSize() );
    }
}

void XclExpExtNameDde::WriteAddData( XclExpStream& rStrm )
{
    if( mxMatrix )
        mxMatrix->Save( rStrm );
}

XclExpExtName::XclExpExtName( const XclExpRoot& rRoot, const XclExpSupbook& rSupbook,
        const OUString& rName, const ScExternalRefCache::TokenArrayRef& rArray ) :
    XclExpExtNameBase( rRoot, rName ),
    mrSupbook(rSupbook),
    mpArray(rArray->Clone())
{
}

void XclExpExtName::WriteAddData( XclExpStream& rStrm )
{
    // Write only if it only has a single token that is either a cell or cell
    // range address.  Excel just writes '02 00 1C 17' for all the other types
    // of external names.

    using namespace ::formula;
    do
    {
        if (mpArray->GetLen() != 1)
            break;

        const formula::FormulaToken* p = mpArray->FirstToken();
        if (!p->IsExternalRef())
            break;

        switch (p->GetType())
        {
            case svExternalSingleRef:
            {
                const ScSingleRefData& rRef = *p->GetSingleRef();
                if (rRef.IsTabRel())
                    break;

                bool bColRel = rRef.IsColRel();
                bool bRowRel = rRef.IsRowRel();
                sal_uInt16 nCol = static_cast<sal_uInt16>(rRef.Col());
                sal_uInt16 nRow = static_cast<sal_uInt16>(rRef.Row());
                if (bColRel) nCol |= 0x4000;
                if (bRowRel) nCol |= 0x8000;

                OUString aTabName = p->GetString().getString();
                sal_uInt16 nSBTab = mrSupbook.GetTabIndex(aTabName);

                // size is always 9
                rStrm << static_cast<sal_uInt16>(9);
                // operator token (3A for cell reference)
                rStrm << static_cast<sal_uInt8>(0x3A);
                // cell address (Excel's address has 2 sheet IDs.)
                rStrm << nSBTab << nSBTab << nRow << nCol;
                return;
            }
            case svExternalDoubleRef:
            {
                const ScComplexRefData& rRef = *p->GetDoubleRef();
                const ScSingleRefData& r1 = rRef.Ref1;
                const ScSingleRefData& r2 = rRef.Ref2;
                if (r1.IsTabRel() || r2.IsTabRel())
                    break;

                sal_uInt16 nTab1 = r1.Tab();
                sal_uInt16 nTab2 = r2.Tab();
                bool bCol1Rel = r1.IsColRel();
                bool bRow1Rel = r1.IsRowRel();
                bool bCol2Rel = r2.IsColRel();
                bool bRow2Rel = r2.IsRowRel();

                sal_uInt16 nCol1 = static_cast<sal_uInt16>(r1.Col());
                sal_uInt16 nCol2 = static_cast<sal_uInt16>(r2.Col());
                sal_uInt16 nRow1 = static_cast<sal_uInt16>(r1.Row());
                sal_uInt16 nRow2 = static_cast<sal_uInt16>(r2.Row());
                if (bCol1Rel) nCol1 |= 0x4000;
                if (bRow1Rel) nCol1 |= 0x8000;
                if (bCol2Rel) nCol2 |= 0x4000;
                if (bRow2Rel) nCol2 |= 0x8000;

                OUString aTabName = p->GetString().getString();
                sal_uInt16 nSBTab = mrSupbook.GetTabIndex(aTabName);

                // size is always 13 (0x0D)
                rStrm << static_cast<sal_uInt16>(13);
                // operator token (3B for area reference)
                rStrm << static_cast<sal_uInt8>(0x3B);
                // range (area) address
                sal_uInt16 nSBTab2 = nSBTab + nTab2 - nTab1;
                rStrm << nSBTab << nSBTab2 << nRow1 << nRow2 << nCol1 << nCol2;
                return;
            }
            default:
                ;   // nothing
        }
    }
    while (false);

    // special value for #REF! (02 00 1C 17)
    rStrm << static_cast<sal_uInt16>(2) << EXC_TOKID_ERR << EXC_ERR_REF;
}

void XclExpExtName::SaveXml(XclExpXmlStream& rStrm)
{
    sax_fastparser::FSHelperPtr pExternalLink = rStrm.GetCurrentStream();

    /* TODO: mpArray contains external references. It doesn't cause any problems, but it's enough
    to export it without the external document identifier. */
    if (mpArray->GetLen())
    {
        const OUString aFormula = XclXmlUtils::ToOUString(GetCompileFormulaContext(), ScAddress(0, 0, 0), mpArray.get());
        pExternalLink->startElement(XML_definedName,
            XML_name, maName.toUtf8(),
            XML_refersTo, aFormula.toUtf8(),
            XML_sheetId, nullptr);
    }
    else
    {
        pExternalLink->startElement(XML_definedName,
            XML_name, maName.toUtf8(),
            XML_refersTo, nullptr,
            XML_sheetId, nullptr);
    }

    pExternalLink->endElement(XML_definedName);
}

// List of external names =====================================================

XclExpExtNameBuffer::XclExpExtNameBuffer( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot )
{
}

sal_uInt16 XclExpExtNameBuffer::InsertAddIn( const OUString& rName )
{
    sal_uInt16 nIndex = GetIndex( rName );
    return nIndex ? nIndex : AppendNew( new XclExpExtNameAddIn( GetRoot(), rName ) );
}

sal_uInt16 XclExpExtNameBuffer::InsertEuroTool( const OUString& rName )
{
    sal_uInt16 nIndex = GetIndex( rName );
    return nIndex ? nIndex : AppendNew( new XclExpExtNameBase( GetRoot(), rName ) );
}

sal_uInt16 XclExpExtNameBuffer::InsertDde(
        std::u16string_view rApplic, std::u16string_view rTopic, const OUString& rItem )
{
    sal_uInt16 nIndex = GetIndex( rItem );
    if( nIndex == 0 )
    {
        size_t nPos;
        if( GetDoc().FindDdeLink( rApplic, rTopic, rItem, SC_DDE_IGNOREMODE, nPos ) )
        {
            // create the leading 'StdDocumentName' EXTERNNAME record
            if( maNameList.IsEmpty() )
                AppendNew( new XclExpExtNameDde(
                    GetRoot(), u"StdDocumentName"_ustr, EXC_EXTN_EXPDDE_STDDOC ) );

            // try to find DDE result array, but create EXTERNNAME record without them too
            const ScMatrix* pScMatrix = GetDoc().GetDdeLinkResultMatrix( nPos );
            nIndex = AppendNew( new XclExpExtNameDde( GetRoot(), rItem, EXC_EXTN_EXPDDE, pScMatrix ) );
        }
    }
    return nIndex;
}

sal_uInt16 XclExpExtNameBuffer::InsertExtName( const XclExpSupbook& rSupbook,
        const OUString& rName, const ScExternalRefCache::TokenArrayRef& rArray )
{
    sal_uInt16 nIndex = GetIndex( rName );
    return nIndex ? nIndex : AppendNew( new XclExpExtName( GetRoot(), rSupbook, rName, rArray ) );
}

void XclExpExtNameBuffer::Save( XclExpStream& rStrm )
{
    maNameList.Save( rStrm );
}

void XclExpExtNameBuffer::SaveXml(XclExpXmlStream& rStrm)
{
    maNameList.SaveXml(rStrm);
}

sal_uInt16 XclExpExtNameBuffer::GetIndex( std::u16string_view rName ) const
{
    for( size_t nPos = 0, nSize = maNameList.GetSize(); nPos < nSize; ++nPos )
        if( maNameList.GetRecord( nPos )->GetName() == rName )
            return static_cast< sal_uInt16 >( nPos + 1 );
    return 0;
}

sal_uInt16 XclExpExtNameBuffer::AppendNew( XclExpExtNameBase* pExtName )
{
    size_t nSize = maNameList.GetSize();
    if( nSize < 0x7FFF )
    {
        maNameList.AppendRecord( pExtName );
        return static_cast< sal_uInt16 >( nSize + 1 );
    }
    return 0;
}

// Cached external cells ======================================================

XclExpCrn::XclExpCrn( SCCOL nScCol, SCROW nScRow, const Any& rValue ) :
    XclExpRecord( EXC_ID_CRN, 4 ),
    mnScCol( nScCol ),
    mnScRow( nScRow )
{
    maValues.push_back( rValue );
}

bool XclExpCrn::InsertValue( SCCOL nScCol, SCROW nScRow, const Any& rValue )
{
    if( (nScRow != mnScRow) || (nScCol != static_cast< SCCOL >( mnScCol + maValues.size() )) )
        return false;
    maValues.push_back( rValue );
    return true;
}

void XclExpCrn::WriteBody( XclExpStream& rStrm )
{
    rStrm   << static_cast< sal_uInt8 >( mnScCol + maValues.size() - 1 )
            << static_cast< sal_uInt8 >( mnScCol )
            << static_cast< sal_uInt16 >( mnScRow );
    for( const auto& rValue : maValues )
    {
        if( rValue.has< bool >() )
            WriteBool( rStrm, rValue.get< bool >() );
        else if( rValue.has< double >() )
            WriteDouble( rStrm, rValue.get< double >() );
        else if( rValue.has< OUString >() )
            WriteString( rStrm, rValue.get< OUString >() );
        else
            WriteEmpty( rStrm );
    }
}

void XclExpCrn::WriteBool( XclExpStream& rStrm, bool bValue )
{
    rStrm << EXC_CACHEDVAL_BOOL << sal_uInt8( bValue ? 1 : 0);
    rStrm.WriteZeroBytes( 7 );
}

void XclExpCrn::WriteDouble( XclExpStream& rStrm, double fValue )
{
    if( !std::isfinite( fValue ) )
    {
        FormulaError nScError = GetDoubleErrorValue(fValue);
        WriteError( rStrm, XclTools::GetXclErrorCode( nScError ) );
    }
    else
    {
        rStrm << EXC_CACHEDVAL_DOUBLE << fValue;
    }
}

void XclExpCrn::WriteString( XclExpStream& rStrm, const OUString& rValue )
{
    rStrm << EXC_CACHEDVAL_STRING << XclExpString( rValue );
}

void XclExpCrn::WriteError( XclExpStream& rStrm, sal_uInt8 nErrCode )
{
    rStrm << EXC_CACHEDVAL_ERROR << nErrCode;
    rStrm.WriteZeroBytes( 7 );
}

void XclExpCrn::WriteEmpty( XclExpStream& rStrm )
{
    rStrm << EXC_CACHEDVAL_EMPTY;
    rStrm.WriteZeroBytes( 8 );
}

void XclExpCrn::SaveXml( XclExpXmlStream& rStrm )
{
    sax_fastparser::FSHelperPtr pFS = rStrm.GetCurrentStream();

    pFS->startElement(XML_row, XML_r, OString::number(mnScRow + 1));

    ScAddress aAdr( mnScCol, mnScRow, 0);   // Tab number doesn't matter
    for( const auto& rValue : maValues )
    {
        bool bCloseCell = true;
        if( rValue.has< double >() )
        {
            double fVal = rValue.get< double >();
            if (std::isfinite( fVal))
            {
                // t='n' is omitted
                pFS->startElement(XML_cell, XML_r, XclXmlUtils::ToOString(rStrm.GetRoot().GetDoc(), ScRange(aAdr)));
                pFS->startElement(XML_v);
                pFS->write( fVal );
            }
            else
            {
                pFS->startElement(XML_cell, XML_r, XclXmlUtils::ToOString(rStrm.GetRoot().GetDoc(), ScRange(aAdr)), XML_t, "e");
                pFS->startElement(XML_v);
                pFS->write( "#VALUE!" );    // OOXTODO: support other error values
            }
        }
        else if( rValue.has< OUString >() )
        {
            pFS->startElement(XML_cell, XML_r, XclXmlUtils::ToOString(rStrm.GetRoot().GetDoc(), ScRange(aAdr)), XML_t, "str");
            pFS->startElement(XML_v);
            pFS->writeEscaped( rValue.get< OUString >() );
        }
        else if( rValue.has< bool >() )
        {
            pFS->startElement(XML_cell, XML_r, XclXmlUtils::ToOString(rStrm.GetRoot().GetDoc(), ScRange(aAdr)), XML_t, "b");
            pFS->startElement(XML_v);
            pFS->write( rValue.get< bool >() ? "1" : "0" );
        }
        // OOXTODO: error type cell t='e'
        else
        {
            // Empty/blank cell not stored, only aAdr is incremented.
            bCloseCell = false;
        }
        if (bCloseCell)
        {
            pFS->endElement(XML_v);
            pFS->endElement(XML_cell);
        }
        aAdr.IncCol();
    }

    pFS->endElement( XML_row);
}

// Cached cells of a sheet ====================================================

XclExpXct::XclExpXct( const XclExpRoot& rRoot, const OUString& rTabName,
        sal_uInt16 nSBTab, ScExternalRefCache::TableTypeRef xCacheTable ) :
    XclExpRoot( rRoot ),
    mxCacheTable(std::move( xCacheTable )),
    maUsedCells( rRoot.GetDoc().GetSheetLimits() ),
    maBoundRange( ScAddress::INITIALIZE_INVALID ),
    maTabName( rTabName ),
    mnSBTab( nSBTab )
{
}

void XclExpXct::StoreCellRange( const ScRange& rRange )
{
    // #i70418# restrict size of external range to prevent memory overflow
    if( (rRange.aEnd.Col() - rRange.aStart.Col()) * (rRange.aEnd.Row() - rRange.aStart.Row()) > 1024 )
        return;

    maUsedCells.SetMultiMarkArea( rRange );
    maBoundRange.ExtendTo( rRange );
}

void XclExpXct::StoreCell_( const ScAddress& rCell )
{
    maUsedCells.SetMultiMarkArea( ScRange( rCell ) );
    maBoundRange.ExtendTo( ScRange( rCell ) );
}

void XclExpXct::StoreCellRange_( const ScRange& rRange )
{
    maUsedCells.SetMultiMarkArea( rRange );
    maBoundRange.ExtendTo( rRange );
}

namespace {

class XclExpCrnList : public XclExpRecordList< XclExpCrn >
{
public:
    /** Inserts the passed value into an existing or new CRN record.
        @return  True = value inserted successfully, false = CRN list is full. */
    bool                InsertValue( SCCOL nScCol, SCROW nScRow, const Any& rValue );
};

bool XclExpCrnList::InsertValue( SCCOL nScCol, SCROW nScRow, const Any& rValue )
{
    RecordRefType xLastRec = GetLastRecord();
    if( xLastRec && xLastRec->InsertValue( nScCol, nScRow, rValue ) )
        return true;
    if( GetSize() == SAL_MAX_UINT16 )
        return false;
    AppendNewRecord( new XclExpCrn( nScCol, nScRow, rValue ) );
    return true;
}

} // namespace

bool XclExpXct::BuildCrnList( XclExpCrnList& rCrnRecs )
{
    if( !mxCacheTable )
        return false;

    /*  Get the range of used rows in the cache table. This may help to
        optimize building the CRN record list if the cache table does not
        contain all referred cells, e.g. if big empty ranges are used in the
        formulas. */
    ::std::pair< SCROW, SCROW > aRowRange = mxCacheTable->getRowRange();
    if( aRowRange.first >= aRowRange.second )
        return false;

    /*  Crop the bounding range of used cells in this table to Excel limits.
        Return if there is no external cell inside these limits. */
    if( !GetAddressConverter().ValidateRange( maBoundRange, false ) )
        return false;

    /*  Find the resulting row range that needs to be processed. */
    SCROW nScRow1 = ::std::max( aRowRange.first, maBoundRange.aStart.Row() );
    SCROW nScRow2 = ::std::min( aRowRange.second - 1, maBoundRange.aEnd.Row() );
    if( nScRow1 > nScRow2 )
        return false;

    /*  Build and collect all CRN records before writing the XCT record. This
        is needed to determine the total number of CRN records which must be
        known when writing the XCT record (possibly encrypted, so seeking the
        output stream back after writing the CRN records is not an option). */
    SvNumberFormatter& rFormatter = GetFormatter();
    bool bValid = true;
    for( SCROW nScRow = nScRow1; bValid && (nScRow <= nScRow2); ++nScRow )
    {
        ::std::pair< SCCOL, SCCOL > aColRange = mxCacheTable->getColRange( nScRow );
        const SCCOL nScEnd = ::std::min( aColRange.second, GetDoc().GetSheetLimits().GetMaxColCount() );
        for( SCCOL nScCol = aColRange.first; bValid && (nScCol < nScEnd); ++nScCol )
        {
            if( maUsedCells.IsCellMarked( nScCol, nScRow, true ) )
            {
                sal_uInt32 nScNumFmt = 0;
                ScExternalRefCache::TokenRef xToken = mxCacheTable->getCell( nScCol, nScRow, &nScNumFmt );
                using namespace ::formula;
                if( xToken )
                    switch( xToken->GetType() )
                    {
                        case svDouble:
                            bValid = (rFormatter.GetType( nScNumFmt ) == SvNumFormatType::LOGICAL) ?
                                rCrnRecs.InsertValue( nScCol, nScRow, Any( xToken->GetDouble() != 0 ) ) :
                                rCrnRecs.InsertValue( nScCol, nScRow, Any( xToken->GetDouble() ) );
                        break;
                        case svString:
                            // do not save empty strings (empty cells) to cache
                            if( !xToken->GetString().isEmpty() )
                                bValid = rCrnRecs.InsertValue( nScCol, nScRow, Any( xToken->GetString().getString() ) );
                        break;
                        default:
                        break;
                    }
            }
        }
    }
    return true;
}

void XclExpXct::Save( XclExpStream& rStrm )
{
    XclExpCrnList aCrnRecs;
    if (!BuildCrnList( aCrnRecs))
        return;

    // write the XCT record and the list of CRN records
    rStrm.StartRecord( EXC_ID_XCT, 4 );
    rStrm << static_cast< sal_uInt16 >( aCrnRecs.GetSize() ) << mnSBTab;
    rStrm.EndRecord();
    aCrnRecs.Save( rStrm );
}

void XclExpXct::SaveXml( XclExpXmlStream& rStrm )
{
    XclExpCrnList aCrnRecs;

    sax_fastparser::FSHelperPtr pFS = rStrm.GetCurrentStream();

    bool bValid = BuildCrnList( aCrnRecs);
    pFS->startElement(XML_sheetData, XML_sheetId, OString::number(mnSBTab));
    if (bValid)
    {
        // row elements
        aCrnRecs.SaveXml( rStrm );
    }
    pFS->endElement( XML_sheetData);
}

// External documents (EXTERNSHEET/SUPBOOK), base class =======================

XclExpExternSheetBase::XclExpExternSheetBase( const XclExpRoot& rRoot, sal_uInt16 nRecId, sal_uInt32 nRecSize ) :
    XclExpRecord( nRecId, nRecSize ),
    XclExpRoot( rRoot )
{
}

XclExpExtNameBuffer& XclExpExternSheetBase::GetExtNameBuffer()
{
    if( !mxExtNameBfr )
        mxExtNameBfr = std::make_shared<XclExpExtNameBuffer>( GetRoot() );
    return *mxExtNameBfr;
}

void XclExpExternSheetBase::WriteExtNameBuffer( XclExpStream& rStrm )
{
    if( mxExtNameBfr )
        mxExtNameBfr->Save( rStrm );
}

void XclExpExternSheetBase::WriteExtNameBufferXml( XclExpXmlStream& rStrm )
{
    if( mxExtNameBfr )
        mxExtNameBfr->SaveXml( rStrm );
}

// External documents (EXTERNSHEET, BIFF5/BIFF7) ==============================

XclExpExternSheet::XclExpExternSheet( const XclExpRoot& rRoot, sal_Unicode cCode ) :
    XclExpExternSheetBase( rRoot, EXC_ID_EXTERNSHEET )
{
    Init( OUStringChar(cCode) );
}

XclExpExternSheet::XclExpExternSheet( const XclExpRoot& rRoot, std::u16string_view rTabName ) :
    XclExpExternSheetBase( rRoot, EXC_ID_EXTERNSHEET )
{
    // reference to own sheet: \03<sheetname>
    Init(Concat2View(OUStringChar(EXC_EXTSH_TABNAME) + rTabName));
}

void XclExpExternSheet::Save( XclExpStream& rStrm )
{
    // EXTERNSHEET record
    XclExpRecord::Save( rStrm );
    // EXTERNNAME records
    WriteExtNameBuffer( rStrm );
}

void XclExpExternSheet::Init( std::u16string_view rEncUrl )
{
    OSL_ENSURE_BIFF( GetBiff() <= EXC_BIFF5 );
    maTabName.AssignByte( rEncUrl, GetTextEncoding(), XclStrFlags::EightBitLength );
    SetRecSize( maTabName.GetSize() );
}

sal_uInt16 XclExpExternSheet::InsertAddIn( const OUString& rName )
{
    return GetExtNameBuffer().InsertAddIn( rName );
}

void XclExpExternSheet::WriteBody( XclExpStream& rStrm )
{
    sal_uInt8 nNameSize = static_cast< sal_uInt8 >( maTabName.Len() );
    // special case: reference to own sheet (starting with '\03') needs wrong string length
    if( maTabName.GetChar( 0 ) == EXC_EXTSH_TABNAME )
        --nNameSize;
    rStrm << nNameSize;
    maTabName.WriteBuffer( rStrm );
}

// External document (SUPBOOK, BIFF8) =========================================

XclExpSupbook::XclExpSupbook( const XclExpRoot& rRoot, sal_uInt16 nXclTabCount ) :
    XclExpExternSheetBase( rRoot, EXC_ID_SUPBOOK, 4 ),
    meType( XclSupbookType::Self ),
    mnXclTabCount( nXclTabCount ),
    mnFileId( 0 )
{
}

XclExpSupbook::XclExpSupbook( const XclExpRoot& rRoot ) :
    XclExpExternSheetBase( rRoot, EXC_ID_SUPBOOK, 4 ),
    meType( XclSupbookType::Addin ),
    mnXclTabCount( 1 ),
    mnFileId( 0 )
{
}

XclExpSupbook::XclExpSupbook( const XclExpRoot& rRoot, const OUString& rUrl, XclSupbookType ) :
    XclExpExternSheetBase( rRoot, EXC_ID_SUPBOOK ),
    maUrl( rUrl ),
    maUrlEncoded( rUrl ),
    meType( XclSupbookType::Eurotool ),
    mnXclTabCount( 0 ),
    mnFileId( 0 )
{
    SetRecSize( 2 + maUrlEncoded.GetSize() );
}

XclExpSupbook::XclExpSupbook( const XclExpRoot& rRoot, const OUString& rUrl ) :
    XclExpExternSheetBase( rRoot, EXC_ID_SUPBOOK ),
    maUrl( rUrl ),
    maUrlEncoded( XclExpUrlHelper::EncodeUrl( rRoot, rUrl ) ),
    meType( XclSupbookType::Extern ),
    mnXclTabCount( 0 ),
    mnFileId( 0 )
{
    SetRecSize( 2 + maUrlEncoded.GetSize() );

    // We need to create all tables up front to ensure the correct table order.
    ScExternalRefManager* pRefMgr = rRoot.GetDoc().GetExternalRefManager();
    sal_uInt16 nFileId = pRefMgr->getExternalFileId( rUrl );
    mnFileId = nFileId + 1;
    ScfStringVec aTabNames;
    pRefMgr->getAllCachedTableNames( nFileId, aTabNames );
    size_t nTabIndex = 0;
    for( const auto& rTabName : aTabNames )
    {
        InsertTabName( rTabName, pRefMgr->getCacheTable( nFileId, nTabIndex ) );
        ++nTabIndex;
    }
}

XclExpSupbook::XclExpSupbook( const XclExpRoot& rRoot, const OUString& rApplic, const OUString& rTopic ) :
    XclExpExternSheetBase( rRoot, EXC_ID_SUPBOOK, 4 ),
    maUrl( rApplic ),
    maDdeTopic( rTopic ),
    maUrlEncoded( XclExpUrlHelper::EncodeDde( rApplic, rTopic ) ),
    meType( XclSupbookType::Special ),
    mnXclTabCount( 0 ),
    mnFileId( 0 )
{
    SetRecSize( 2 + maUrlEncoded.GetSize() );
}

bool XclExpSupbook::IsUrlLink( std::u16string_view rUrl ) const
{
    return (meType == XclSupbookType::Extern || meType == XclSupbookType::Eurotool) && (maUrl == rUrl);
}

bool XclExpSupbook::IsDdeLink( std::u16string_view rApplic, std::u16string_view rTopic ) const
{
    return (meType == XclSupbookType::Special) && (maUrl == rApplic) && (maDdeTopic == rTopic);
}

void XclExpSupbook::FillRefLogEntry( XclExpRefLogEntry& rRefLogEntry,
        sal_uInt16 nFirstSBTab, sal_uInt16 nLastSBTab ) const
{
    rRefLogEntry.mpUrl = maUrlEncoded.IsEmpty() ? nullptr : &maUrlEncoded;
    rRefLogEntry.mpFirstTab = GetTabName( nFirstSBTab );
    rRefLogEntry.mpLastTab = GetTabName( nLastSBTab );
}

void XclExpSupbook::StoreCellRange( const ScRange& rRange, sal_uInt16 nSBTab )
{
    if( XclExpXct* pXct = maXctList.GetRecord( nSBTab ) )
        pXct->StoreCellRange( rRange );
}

void XclExpSupbook::StoreCell_( sal_uInt16 nSBTab, const ScAddress& rCell )
{
    if( XclExpXct* pXct = maXctList.GetRecord( nSBTab ) )
        pXct->StoreCell_( rCell );
}

void XclExpSupbook::StoreCellRange_( sal_uInt16 nSBTab, const ScRange& rRange )
{
    // multi-table range is not allowed!
    if( rRange.aStart.Tab() == rRange.aEnd.Tab() )
        if( XclExpXct* pXct = maXctList.GetRecord( nSBTab ) )
            pXct->StoreCellRange_( rRange );
}

sal_uInt16 XclExpSupbook::GetTabIndex( const OUString& rTabName ) const
{
    XclExpString aXclName(rTabName);
    size_t nSize = maXctList.GetSize();
    for (size_t i = 0; i < nSize; ++i)
    {
        XclExpXctRef aRec = maXctList.GetRecord(i);
        if (aXclName == aRec->GetTabName())
            return ulimit_cast<sal_uInt16>(i);
    }
    return EXC_NOTAB;
}

sal_uInt16 XclExpSupbook::GetTabCount() const
{
    return ulimit_cast<sal_uInt16>(maXctList.GetSize());
}

sal_uInt16 XclExpSupbook::InsertTabName( const OUString& rTabName, ScExternalRefCache::TableTypeRef const & xCacheTable )
{
    SAL_WARN_IF( meType != XclSupbookType::Extern, "sc.filter", "Don't insert sheet names here" );
    sal_uInt16 nSBTab = ulimit_cast< sal_uInt16 >( maXctList.GetSize() );
    XclExpXctRef xXct = new XclExpXct( GetRoot(), rTabName, nSBTab, xCacheTable );
    AddRecSize( xXct->GetTabName().GetSize() );
    maXctList.AppendRecord( xXct );
    return nSBTab;
}

sal_uInt16 XclExpSupbook::InsertAddIn( const OUString& rName )
{
    return GetExtNameBuffer().InsertAddIn( rName );
}

sal_uInt16 XclExpSupbook::InsertEuroTool( const OUString& rName )
{
    return GetExtNameBuffer().InsertEuroTool( rName );
}

sal_uInt16 XclExpSupbook::InsertDde( const OUString& rItem )
{
    return GetExtNameBuffer().InsertDde( maUrl, maDdeTopic, rItem );
}

sal_uInt16 XclExpSupbook::InsertExtName( const OUString& rName, const ScExternalRefCache::TokenArrayRef& rArray )
{
    return GetExtNameBuffer().InsertExtName(*this, rName, rArray);
}

XclSupbookType XclExpSupbook::GetType() const
{
    return meType;
}

sal_uInt16 XclExpSupbook::GetFileId() const
{
    return mnFileId;
}

const OUString& XclExpSupbook::GetUrl() const
{
    return maUrl;
}

void XclExpSupbook::Save( XclExpStream& rStrm )
{
    // SUPBOOK record
    XclExpRecord::Save( rStrm );
    // XCT record, CRN records
    maXctList.Save( rStrm );
    // EXTERNNAME records
    WriteExtNameBuffer( rStrm );
}

void XclExpSupbook::SaveXml( XclExpXmlStream& rStrm )
{
    sax_fastparser::FSHelperPtr pExternalLink = rStrm.GetCurrentStream();

    // Add relation for this stream, e.g. xl/externalLinks/_rels/externalLink1.xml.rels
    sal_uInt16 nLevel = 0;
    bool bRel = true;

    // BuildFileName delete ../ and convert them to nLevel
    // but addrelation needs ../ instead of nLevel, so we have to convert it back
    OUString sFile = XclExpHyperlink::BuildFileName(nLevel, bRel, maUrl, GetRoot(), true);
    while (nLevel > 0)
    {
        sFile = "../" + sFile;
        --nLevel;
    }

    OUString sId = rStrm.addRelation( pExternalLink->getOutputStream(),
            oox::getRelationship(Relationship::EXTERNALLINKPATH), sFile, true );

    pExternalLink->startElement( XML_externalLink,
            XML_xmlns, rStrm.getNamespaceURL(OOX_NS(xls)).toUtf8());

    pExternalLink->startElement( XML_externalBook,
            FSNS(XML_xmlns, XML_r), rStrm.getNamespaceURL(OOX_NS(officeRel)).toUtf8(),
            FSNS(XML_r, XML_id),    sId.toUtf8());

    if (!maXctList.IsEmpty())
    {
        pExternalLink->startElement(XML_sheetNames);
        for (size_t nPos = 0, nSize = maXctList.GetSize(); nPos < nSize; ++nPos)
        {
            pExternalLink->singleElement(XML_sheetName,
                XML_val, XclXmlUtils::ToOString(maXctList.GetRecord(nPos)->GetTabName()));
        }
        pExternalLink->endElement( XML_sheetNames);

    }

    if (mxExtNameBfr)
    {
        pExternalLink->startElement(XML_definedNames);
        // externalName elements
        WriteExtNameBufferXml( rStrm );
        pExternalLink->endElement(XML_definedNames);
    }

    if (!maXctList.IsEmpty())
    {
        pExternalLink->startElement(XML_sheetDataSet);

        // sheetData elements
        maXctList.SaveXml( rStrm );

        pExternalLink->endElement( XML_sheetDataSet);

    }
    pExternalLink->endElement( XML_externalBook);
    pExternalLink->endElement( XML_externalLink);
}

const XclExpString* XclExpSupbook::GetTabName( sal_uInt16 nSBTab ) const
{
    XclExpXctRef xXct = maXctList.GetRecord( nSBTab );
    return xXct ? &xXct->GetTabName() : nullptr;
}

void XclExpSupbook::WriteBody( XclExpStream& rStrm )
{
    switch( meType )
    {
        case XclSupbookType::Self:
            rStrm << mnXclTabCount << EXC_SUPB_SELF;
        break;
        case XclSupbookType::Extern:
        case XclSupbookType::Special:
        case XclSupbookType::Eurotool:
        {
            sal_uInt16 nCount = ulimit_cast< sal_uInt16 >( maXctList.GetSize() );
            rStrm << nCount << maUrlEncoded;

            for( size_t nPos = 0, nSize = maXctList.GetSize(); nPos < nSize; ++nPos )
                rStrm << maXctList.GetRecord( nPos )->GetTabName();
        }
        break;
        case XclSupbookType::Addin:
            rStrm << mnXclTabCount << EXC_SUPB_ADDIN;
        break;
        default:
            SAL_WARN( "sc.filter", "Unhandled SUPBOOK type " << meType);
    }
}

// All SUPBOOKS in a document =================================================

XclExpSupbookBuffer::XclExpSupbookBuffer( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot ),
    mnOwnDocSB( SAL_MAX_UINT16 ),
    mnAddInSB( SAL_MAX_UINT16 )
{
    XclExpTabInfo& rTabInfo = GetTabInfo();
    sal_uInt16 nXclCnt = rTabInfo.GetXclTabCount();
    sal_uInt16 nCodeCnt = static_cast< sal_uInt16 >( GetExtDocOptions().GetCodeNameCount() );
    size_t nCount = nXclCnt + rTabInfo.GetXclExtTabCount();

    OSL_ENSURE( nCount > 0, "XclExpSupbookBuffer::XclExpSupbookBuffer - no sheets to export" );
    if( nCount )
    {
        maSBIndexVec.resize( nCount );

        // self-ref SUPBOOK first of list
        XclExpSupbookRef xSupbook = new XclExpSupbook( GetRoot(), ::std::max( nXclCnt, nCodeCnt ) );
        mnOwnDocSB = Append( xSupbook );
        for( sal_uInt16 nXclTab = 0; nXclTab < nXclCnt; ++nXclTab )
            maSBIndexVec[ nXclTab ].Set( mnOwnDocSB, nXclTab );
    }
}

XclExpXti XclExpSupbookBuffer::GetXti( sal_uInt16 nFirstXclTab, sal_uInt16 nLastXclTab,
        XclExpRefLogEntry* pRefLogEntry ) const
{
    XclExpXti aXti;
    size_t nSize = maSBIndexVec.size();
    if( (nFirstXclTab < nSize) && (nLastXclTab < nSize) )
    {
        // index of the SUPBOOK record
        aXti.mnSupbook = maSBIndexVec[ nFirstXclTab ].mnSupbook;

        // all sheets in the same supbook?
        bool bSameSB = true;
        for( sal_uInt16 nXclTab = nFirstXclTab + 1; bSameSB && (nXclTab <= nLastXclTab); ++nXclTab )
        {
            bSameSB = maSBIndexVec[ nXclTab ].mnSupbook == aXti.mnSupbook;
            if( !bSameSB )
                nLastXclTab = nXclTab - 1;
        }
        aXti.mnFirstSBTab = maSBIndexVec[ nFirstXclTab ].mnSBTab;
        aXti.mnLastSBTab = maSBIndexVec[ nLastXclTab ].mnSBTab;

        // fill external reference log entry (for change tracking)
        if( pRefLogEntry )
        {
            pRefLogEntry->mnFirstXclTab = nFirstXclTab;
            pRefLogEntry->mnLastXclTab = nLastXclTab;
            XclExpSupbookRef xSupbook = maSupbookList.GetRecord( aXti.mnSupbook );
            if( xSupbook )
                xSupbook->FillRefLogEntry( *pRefLogEntry, aXti.mnFirstSBTab, aXti.mnLastSBTab );
        }
    }
    else
    {
        // special range, i.e. for deleted sheets or add-ins
        aXti.mnSupbook = mnOwnDocSB;
        aXti.mnFirstSBTab = nFirstXclTab;
        aXti.mnLastSBTab = nLastXclTab;
    }

    return aXti;
}

void XclExpSupbookBuffer::StoreCellRange( const ScRange& rRange )
{
    sal_uInt16 nXclTab = GetTabInfo().GetXclTab( rRange.aStart.Tab() );
    if( nXclTab < maSBIndexVec.size() )
    {
        const XclExpSBIndex& rSBIndex = maSBIndexVec[ nXclTab ];
        XclExpSupbookRef xSupbook = maSupbookList.GetRecord( rSBIndex.mnSupbook );
        OSL_ENSURE( xSupbook , "XclExpSupbookBuffer::StoreCellRange - missing SUPBOOK record" );
        if( xSupbook )
            xSupbook->StoreCellRange( rRange, rSBIndex.mnSBTab );
    }
}

namespace {

class FindSBIndexEntry
{
public:
    explicit FindSBIndexEntry(sal_uInt16 nSupbookId, sal_uInt16 nTabId) :
        mnSupbookId(nSupbookId), mnTabId(nTabId) {}

    bool operator()(const XclExpSBIndex& r) const
    {
        return mnSupbookId == r.mnSupbook && mnTabId == r.mnSBTab;
    }

private:
    sal_uInt16 mnSupbookId;
    sal_uInt16 mnTabId;
};

}

void XclExpSupbookBuffer::StoreCell( sal_uInt16 nFileId, const OUString& rTabName, const ScAddress& rCell )
{
    ScExternalRefManager* pRefMgr = GetDoc().GetExternalRefManager();
    const OUString* pUrl = pRefMgr->getExternalFileName(nFileId);
    if (!pUrl)
        return;

    XclExpSupbookRef xSupbook;
    auto nSupbookId = GetSupbookUrl(xSupbook, *pUrl);
    if (!nSupbookId)
    {
        xSupbook = new XclExpSupbook(GetRoot(), *pUrl);
        nSupbookId = Append(xSupbook);
    }

    sal_uInt16 nSheetId = xSupbook->GetTabIndex(rTabName);
    if (nSheetId == EXC_NOTAB)
        // specified table name not found in this SUPBOOK.
        return;

    FindSBIndexEntry f(*nSupbookId, nSheetId);
    if (::std::none_of(maSBIndexVec.begin(), maSBIndexVec.end(), f))
    {
        maSBIndexVec.emplace_back();
        XclExpSBIndex& r = maSBIndexVec.back();
        r.mnSupbook = *nSupbookId;
        r.mnSBTab   = nSheetId;
    }

    xSupbook->StoreCell_(nSheetId, rCell);
}

void XclExpSupbookBuffer::StoreCellRange( sal_uInt16 nFileId, const OUString& rTabName, const ScRange& rRange )
{
    ScExternalRefManager* pRefMgr = GetDoc().GetExternalRefManager();
    const OUString* pUrl = pRefMgr->getExternalFileName(nFileId);
    if (!pUrl)
        return;

    XclExpSupbookRef xSupbook;
    auto nSupbookId = GetSupbookUrl(xSupbook, *pUrl);
    if (!nSupbookId)
    {
        xSupbook = new XclExpSupbook(GetRoot(), *pUrl);
        nSupbookId = Append(xSupbook);
    }

    SCTAB nTabCount = rRange.aEnd.Tab() - rRange.aStart.Tab() + 1;

    // If this is a multi-table range, get token for each table.
    using namespace ::formula;
    SCTAB aMatrixListSize = 0;

    // This is a new'ed instance, so we must manage its life cycle here.
    ScExternalRefCache::TokenArrayRef pArray = pRefMgr->getDoubleRefTokens(nFileId, rTabName, rRange, nullptr);
    if (!pArray)
        return;

    FormulaTokenArrayPlainIterator aIter(*pArray);
    for (FormulaToken* p = aIter.First(); p; p = aIter.Next())
    {
        if (p->GetType() == svMatrix)
            ++aMatrixListSize;
        else if (p->GetOpCode() != ocSep)
        {
            // This is supposed to be ocSep!!!
            return;
        }
    }

    if (aMatrixListSize != nTabCount)
    {
        // matrix size mismatch!
        return;
    }

    sal_uInt16 nFirstSheetId = xSupbook->GetTabIndex(rTabName);

    ScRange aRange(rRange);
    aRange.aStart.SetTab(0);
    aRange.aEnd.SetTab(0);
    for (SCTAB nTab = 0; nTab < nTabCount; ++nTab)
    {
        sal_uInt16 nSheetId = nFirstSheetId + static_cast<sal_uInt16>(nTab);
        FindSBIndexEntry f(*nSupbookId, nSheetId);
        if (::std::none_of(maSBIndexVec.begin(), maSBIndexVec.end(), f))
        {
            maSBIndexVec.emplace_back();
            XclExpSBIndex& r = maSBIndexVec.back();
            r.mnSupbook = *nSupbookId;
            r.mnSBTab   = nSheetId;
        }

        xSupbook->StoreCellRange_(nSheetId, aRange);
    }
}

std::optional<XclExpSBIndex> XclExpSupbookBuffer::InsertAddIn(const OUString& rName )
{
    XclExpSupbookRef xSupbook;
    if( mnAddInSB == SAL_MAX_UINT16 )
    {
        xSupbook = new XclExpSupbook( GetRoot() );
        mnAddInSB = Append( xSupbook );
    }
    else
        xSupbook = maSupbookList.GetRecord( mnAddInSB );
    OSL_ENSURE( xSupbook, "XclExpSupbookBuffer::InsertAddin - missing add-in supbook" );

    sal_uInt16 nExtName = xSupbook->InsertAddIn( rName );
    if( nExtName > 0)
    {
        return XclExpSBIndex( mnAddInSB, nExtName );
    }
    return {};
}

std::optional<XclExpSBIndex> XclExpSupbookBuffer::InsertEuroTool( const OUString& rName )
{
    XclExpSupbookRef xSupbook;
    OUString aUrl( u"\001\010EUROTOOL.XLA"_ustr );
    auto nSupbookId = GetSupbookUrl(xSupbook, aUrl);
    if ( !nSupbookId )
    {
        xSupbook = new XclExpSupbook( GetRoot(), aUrl, XclSupbookType::Eurotool );
        nSupbookId = Append( xSupbook );
    }

    auto nExtName = xSupbook->InsertEuroTool( rName );
    if( nExtName > 0)
    {
        return XclExpSBIndex( *nSupbookId, nExtName );
    }
    return {};
}

std::optional<XclExpSBIndex> XclExpSupbookBuffer::InsertDde(
        const OUString& rApplic, const OUString& rTopic, const OUString& rItem )
{
    XclExpSupbookRef xSupbook;
    auto nSupbook = GetSupbookDde( xSupbook, rApplic, rTopic );
    if( !nSupbook )
    {
        xSupbook = new XclExpSupbook( GetRoot(), rApplic, rTopic );
        nSupbook = Append( xSupbook );
    }
    auto nExtName = xSupbook->InsertDde( rItem );
    if (nExtName > 0)
    {
        return XclExpSBIndex(*nSupbook, nExtName);
    }
    return {};
}

std::optional<XclExpSBIndex> XclExpSupbookBuffer::InsertExtName( const OUString& rUrl,
        const OUString& rName, const ScExternalRefCache::TokenArrayRef& rArray )
{
    XclExpSupbookRef xSupbook;
    auto nSupbookId = GetSupbookUrl(xSupbook, rUrl);
    if ( !nSupbookId )
    {
        xSupbook = new XclExpSupbook(GetRoot(), rUrl);
        nSupbookId = Append(xSupbook);
    }

    auto nExtName = xSupbook->InsertExtName(rName, rArray);
    if (nExtName > 0)
    {
        return XclExpSBIndex( *nSupbookId, nExtName );
    }
    return {};
}

XclExpXti XclExpSupbookBuffer::GetXti( sal_uInt16 nFileId, const OUString& rTabName, sal_uInt16 nXclTabSpan,
                                       XclExpRefLogEntry* pRefLogEntry )
{
    XclExpXti aXti(0, EXC_NOTAB, EXC_NOTAB);
    ScExternalRefManager* pRefMgr = GetDoc().GetExternalRefManager();
    const OUString* pUrl = pRefMgr->getExternalFileName(nFileId);
    if (!pUrl)
        return aXti;

    XclExpSupbookRef xSupbook;
    auto nSupbookId = GetSupbookUrl(xSupbook, *pUrl);
    if (!nSupbookId)
    {
        xSupbook = new XclExpSupbook(GetRoot(), *pUrl);
        nSupbookId = Append(xSupbook);
    }
    aXti.mnSupbook = *nSupbookId;

    sal_uInt16 nFirstSheetId = xSupbook->GetTabIndex(rTabName);
    if (nFirstSheetId == EXC_NOTAB)
    {
        // first sheet not found in SUPBOOK.
        return aXti;
    }
    sal_uInt16 nSheetCount = xSupbook->GetTabCount();
    for (sal_uInt16 i = 0; i < nXclTabSpan; ++i)
    {
        sal_uInt16 nSheetId = nFirstSheetId + i;
        if (nSheetId >= nSheetCount)
            return aXti;

        FindSBIndexEntry f(*nSupbookId, nSheetId);
        if (::std::none_of(maSBIndexVec.begin(), maSBIndexVec.end(), f))
        {
            maSBIndexVec.emplace_back();
            XclExpSBIndex& r = maSBIndexVec.back();
            r.mnSupbook = *nSupbookId;
            r.mnSBTab   = nSheetId;
        }
        if (i == 0)
            aXti.mnFirstSBTab = nSheetId;
        if (i == nXclTabSpan - 1)
            aXti.mnLastSBTab = nSheetId;
    }

    if (pRefLogEntry)
    {
        pRefLogEntry->mnFirstXclTab = 0;
        pRefLogEntry->mnLastXclTab  = 0;
        if (xSupbook)
            xSupbook->FillRefLogEntry(*pRefLogEntry, aXti.mnFirstSBTab, aXti.mnLastSBTab);
    }

    return aXti;
}

void XclExpSupbookBuffer::Save( XclExpStream& rStrm )
{
    maSupbookList.Save( rStrm );
}

void XclExpSupbookBuffer::SaveXml( XclExpXmlStream& rStrm )
{
    // Unused external references are not saved, only kept in memory.
    // Those that are saved must be indexed from 1, so indexes must be reordered
    ScExternalRefManager* pRefMgr = GetDoc().GetExternalRefManager();
    vector<sal_uInt16> aExternFileIds;
    for (size_t nPos = 0, nSize = maSupbookList.GetSize(); nPos < nSize; ++nPos)
    {
        XclExpSupbookRef xRef(maSupbookList.GetRecord(nPos));
        // fileIDs are indexed from 1 in xlsx, and from 0 in ScExternalRefManager
        // converting between them require a -1 or +1
        if (xRef->GetType() == XclSupbookType::Extern)
            aExternFileIds.push_back(xRef->GetFileId() - 1);
    }
    if (aExternFileIds.size() > 0)
        pRefMgr->setSkipUnusedFileIds(aExternFileIds);

    ::std::map< sal_uInt16, OUString > aMap;
    for (size_t nPos = 0, nSize = maSupbookList.GetSize(); nPos < nSize; ++nPos)
    {
        XclExpSupbookRef xRef( maSupbookList.GetRecord( nPos));
        if (xRef->GetType() != XclSupbookType::Extern)
            continue;   // handle only external reference (for now?)

        sal_uInt16 nId = xRef->GetFileId();
        sal_uInt16 nUsedId = pRefMgr->convertFileIdToUsedFileId(nId - 1) + 1;
        const OUString& rUrl = xRef->GetUrl();
        ::std::pair< ::std::map< sal_uInt16, OUString >::iterator, bool > aInsert(
                aMap.insert( ::std::make_pair( nId, rUrl)));
        if (!aInsert.second)
        {
            SAL_WARN( "sc.filter", "XclExpSupbookBuffer::SaveXml: file ID already used: " << nId <<
                    " wanted for " << rUrl << " and is " << (*aInsert.first).second <<
                    (rUrl == (*aInsert.first).second ? " multiple Supbook not supported" : ""));
            continue;
        }
        OUString sId;
        sax_fastparser::FSHelperPtr pExternalLink = rStrm.CreateOutputStream(
                XclXmlUtils::GetStreamName( "xl/", "externalLinks/externalLink", nUsedId),
                XclXmlUtils::GetStreamName( nullptr, "externalLinks/externalLink", nUsedId),
                rStrm.GetCurrentStream()->getOutputStream(),
                "application/vnd.openxmlformats-officedocument.spreadsheetml.externalLink+xml",
                CREATE_OFFICEDOC_RELATION_TYPE("externalLink"),
                &sId );

        // externalReference entry in workbook externalReferences
        rStrm.GetCurrentStream()->singleElement( XML_externalReference,
                FSNS(XML_r, XML_id), sId.toUtf8() );

        // Each externalBook in a separate stream.
        rStrm.PushStream( pExternalLink );
        xRef->SaveXml( rStrm );
        rStrm.PopStream();
    }
}

bool XclExpSupbookBuffer::HasExternalReferences() const
{
    for (size_t nPos = 0, nSize = maSupbookList.GetSize(); nPos < nSize; ++nPos)
    {
        if (maSupbookList.GetRecord( nPos)->GetType() == XclSupbookType::Extern)
            return true;
    }
    return false;
}

std::optional<sal_uInt16> XclExpSupbookBuffer::GetSupbookUrl(
        XclExpSupbookRef& rxSupbook, std::u16string_view rUrl ) const
{
    for( size_t nPos = 0, nSize = maSupbookList.GetSize(); nPos < nSize; ++nPos )
    {
        rxSupbook = maSupbookList.GetRecord( nPos );
        if( rxSupbook->IsUrlLink( rUrl ) )
        {
            return ulimit_cast< sal_uInt16 >( nPos );
        }
    }
    return {};
}

std::optional<sal_uInt16> XclExpSupbookBuffer::GetSupbookDde( XclExpSupbookRef& rxSupbook,
        std::u16string_view rApplic, std::u16string_view rTopic ) const
{
    for( size_t nPos = 0, nSize = maSupbookList.GetSize(); nPos < nSize; ++nPos )
    {
        rxSupbook = maSupbookList.GetRecord( nPos );
        if( rxSupbook->IsDdeLink( rApplic, rTopic ) )
        {
            return ulimit_cast< sal_uInt16 >( nPos );
        }
    }
    return {};
}

sal_uInt16 XclExpSupbookBuffer::Append( XclExpSupbookRef const & xSupbook )
{
    maSupbookList.AppendRecord( xSupbook );
    return ulimit_cast< sal_uInt16 >( maSupbookList.GetSize() - 1 );
}

// Export link manager ========================================================

XclExpLinkManagerImpl::XclExpLinkManagerImpl( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot )
{
}

XclExpLinkManagerImpl5::XclExpLinkManagerImpl5( const XclExpRoot& rRoot ) :
    XclExpLinkManagerImpl( rRoot )
{
}

void XclExpLinkManagerImpl5::FindExtSheet(
        sal_uInt16& rnExtSheet, sal_uInt16& rnFirstXclTab, sal_uInt16& rnLastXclTab,
        SCTAB nFirstScTab, SCTAB nLastScTab, XclExpRefLogEntry* pRefLogEntry )
{
    FindInternal( rnExtSheet, rnFirstXclTab, nFirstScTab );
    if( (rnFirstXclTab == EXC_TAB_DELETED) || (nFirstScTab == nLastScTab) )
    {
        rnLastXclTab = rnFirstXclTab;
    }
    else
    {
        sal_uInt16 nDummyExtSheet;
        FindInternal( nDummyExtSheet, rnLastXclTab, nLastScTab );
    }

    OSL_ENSURE( !pRefLogEntry, "XclExpLinkManagerImpl5::FindExtSheet - fill reflog entry not implemented" );
}

sal_uInt16 XclExpLinkManagerImpl5::FindExtSheet( sal_Unicode cCode )
{
    sal_uInt16 nExtSheet;
    FindInternal( nExtSheet, cCode );
    return nExtSheet;
}

void XclExpLinkManagerImpl5::FindExtSheet(
    sal_uInt16 /*nFileId*/, const OUString& /*rTabName*/, sal_uInt16 /*nXclTabSpan*/,
    sal_uInt16& /*rnExtSheet*/, sal_uInt16& /*rnFirstSBTab*/, sal_uInt16& /*rnLastSBTab*/,
    XclExpRefLogEntry* /*pRefLogEntry*/ )
{
    // not implemented
}

void XclExpLinkManagerImpl5::StoreCellRange( const ScSingleRefData& /*rRef1*/, const ScSingleRefData& /*rRef2*/, const ScAddress& /*rPos*/ )
{
    // not implemented
}

void XclExpLinkManagerImpl5::StoreCell( sal_uInt16 /*nFileId*/, const OUString& /*rTabName*/, const ScAddress& /*rPos*/ )
{
    // not implemented
}

void XclExpLinkManagerImpl5::StoreCellRange( sal_uInt16 /*nFileId*/, const OUString& /*rTabName*/, const ScRange& /*rRange*/ )
{
    // not implemented
}

std::optional<XclExpSBIndex> XclExpLinkManagerImpl5::InsertAddIn( const OUString& rName )
{
    sal_uInt16 nExtSheet, nExtName;
    XclExpExtSheetRef xExtSheet = FindInternal( nExtSheet, EXC_EXTSH_ADDIN );
    if( xExtSheet )
    {
        nExtName = xExtSheet->InsertAddIn( rName );
        if(nExtName > 0)
        {
            return XclExpSBIndex( nExtSheet, nExtName );
        }
    }
    return {};
}

std::optional<XclExpSBIndex> XclExpLinkManagerImpl5::InsertEuroTool( const OUString& /*rName*/ )
{
     return {};
}

std::optional<XclExpSBIndex> XclExpLinkManagerImpl5::InsertDde(
        const OUString& /*rApplic*/, const OUString& /*rTopic*/, const OUString& /*rItem*/ )
{
    // not implemented
    return {};
}

std::optional<XclExpSBIndex> XclExpLinkManagerImpl5::InsertExtName( const OUString& /*rUrl*/,
        const OUString& /*rName*/, const ScExternalRefCache::TokenArrayRef& /*rArray*/ )
{
    // not implemented
    return {};
}

void XclExpLinkManagerImpl5::Save( XclExpStream& rStrm )
{
    if( sal_uInt16 nExtSheetCount = GetExtSheetCount() )
    {
        // EXTERNCOUNT record
        XclExpUInt16Record( EXC_ID_EXTERNCOUNT, nExtSheetCount ).Save( rStrm );
        // list of EXTERNSHEET records with EXTERNNAME, XCT, CRN records
        maExtSheetList.Save( rStrm );
    }
}

void XclExpLinkManagerImpl5::SaveXml( XclExpXmlStream& /*rStrm*/ )
{
    // not applicable
}

sal_uInt16 XclExpLinkManagerImpl5::GetExtSheetCount() const
{
    return static_cast< sal_uInt16 >( maExtSheetList.GetSize() );
}

sal_uInt16 XclExpLinkManagerImpl5::AppendInternal( XclExpExtSheetRef const & xExtSheet )
{
    if( GetExtSheetCount() < 0x7FFF )
    {
        maExtSheetList.AppendRecord( xExtSheet );
        // return negated one-based EXTERNSHEET index (i.e. 0xFFFD for 3rd record)
        return static_cast< sal_uInt16 >( -GetExtSheetCount() );
    }
    return 0;
}

void XclExpLinkManagerImpl5::CreateInternal()
{
    if( !maIntTabMap.empty() )
        return;

    // create EXTERNSHEET records for all internal exported sheets
    XclExpTabInfo& rTabInfo = GetTabInfo();
    for( SCTAB nScTab = 0, nScCnt = rTabInfo.GetScTabCount(); nScTab < nScCnt; ++nScTab )
    {
        if( rTabInfo.IsExportTab( nScTab ) )
        {
            XclExpExtSheetRef xRec;
            if( nScTab == GetCurrScTab() )
                xRec = new XclExpExternSheet( GetRoot(), EXC_EXTSH_OWNTAB );
            else
                xRec = new XclExpExternSheet( GetRoot(), rTabInfo.GetScTabName( nScTab ) );
            maIntTabMap[ nScTab ] = AppendInternal( xRec );
        }
    }
}

XclExpLinkManagerImpl5::XclExpExtSheetRef XclExpLinkManagerImpl5::GetInternal( sal_uInt16 nExtSheet )
{
    return maExtSheetList.GetRecord( static_cast< sal_uInt16 >( -nExtSheet - 1 ) );
}

XclExpLinkManagerImpl5::XclExpExtSheetRef XclExpLinkManagerImpl5::FindInternal(
        sal_uInt16& rnExtSheet, sal_uInt16& rnXclTab, SCTAB nScTab )
{
    // create internal EXTERNSHEET records on demand
    CreateInternal();

    // try to find an EXTERNSHEET record - if not, return a "deleted sheet" reference
    XclExpExtSheetRef xExtSheet;
    XclExpIntTabMap::const_iterator aIt = maIntTabMap.find( nScTab );
    if( aIt == maIntTabMap.end() )
    {
        xExtSheet = FindInternal( rnExtSheet, EXC_EXTSH_OWNDOC );
        rnXclTab = EXC_TAB_DELETED;
    }
    else
    {
        rnExtSheet = aIt->second;
        xExtSheet = GetInternal( rnExtSheet );
        rnXclTab = GetTabInfo().GetXclTab( nScTab );
    }
    return xExtSheet;
}

XclExpLinkManagerImpl5::XclExpExtSheetRef XclExpLinkManagerImpl5::FindInternal(
    sal_uInt16& rnExtSheet, sal_Unicode cCode )
{
    XclExpExtSheetRef xExtSheet;
    XclExpCodeMap::const_iterator aIt = maCodeMap.find( cCode );
    if( aIt == maCodeMap.end() )
    {
        xExtSheet = new XclExpExternSheet( GetRoot(), cCode );
        rnExtSheet = maCodeMap[ cCode ] = AppendInternal( xExtSheet );
    }
    else
    {
        rnExtSheet = aIt->second;
        xExtSheet = GetInternal( rnExtSheet );
    }
    return xExtSheet;
}

XclExpLinkManagerImpl8::XclExpLinkManagerImpl8( const XclExpRoot& rRoot ) :
    XclExpLinkManagerImpl( rRoot ),
    maSBBuffer( rRoot )
{
}

void XclExpLinkManagerImpl8::FindExtSheet(
        sal_uInt16& rnExtSheet, sal_uInt16& rnFirstXclTab, sal_uInt16& rnLastXclTab,
        SCTAB nFirstScTab, SCTAB nLastScTab, XclExpRefLogEntry* pRefLogEntry )
{
    XclExpTabInfo& rTabInfo = GetTabInfo();
    rnFirstXclTab = rTabInfo.GetXclTab( nFirstScTab );
    rnLastXclTab = rTabInfo.GetXclTab( nLastScTab );
    rnExtSheet = InsertXti( maSBBuffer.GetXti( rnFirstXclTab, rnLastXclTab, pRefLogEntry ) );
}

sal_uInt16 XclExpLinkManagerImpl8::FindExtSheet( sal_Unicode cCode )
{
    OSL_ENSURE( (cCode == EXC_EXTSH_OWNDOC) || (cCode == EXC_EXTSH_ADDIN),
        "XclExpLinkManagerImpl8::FindExtSheet - unknown externsheet code" );
    return InsertXti( maSBBuffer.GetXti( EXC_TAB_EXTERNAL, EXC_TAB_EXTERNAL ) );
}

void XclExpLinkManagerImpl8::FindExtSheet(
    sal_uInt16 nFileId, const OUString& rTabName, sal_uInt16 nXclTabSpan,
    sal_uInt16& rnExtSheet, sal_uInt16& rnFirstSBTab, sal_uInt16& rnLastSBTab,
    XclExpRefLogEntry* pRefLogEntry )
{
    XclExpXti aXti = maSBBuffer.GetXti(nFileId, rTabName, nXclTabSpan, pRefLogEntry);
    rnExtSheet = InsertXti(aXti);
    rnFirstSBTab = aXti.mnFirstSBTab;
    rnLastSBTab  = aXti.mnLastSBTab;
}

void XclExpLinkManagerImpl8::StoreCellRange( const ScSingleRefData& rRef1, const ScSingleRefData& rRef2, const ScAddress& rPos )
{
    ScAddress aAbs1 = rRef1.toAbs(GetRoot().GetDoc(), rPos);
    ScAddress aAbs2 = rRef2.toAbs(GetRoot().GetDoc(), rPos);
    if (!(!rRef1.IsDeleted() && !rRef2.IsDeleted() && (aAbs1.Tab() >= 0) && (aAbs2.Tab() >= 0)))
        return;

    const XclExpTabInfo& rTabInfo = GetTabInfo();
    SCTAB nFirstScTab = aAbs1.Tab();
    SCTAB nLastScTab = aAbs2.Tab();
    ScRange aRange(aAbs1.Col(), aAbs1.Row(), 0, aAbs2.Col(), aAbs2.Row(), 0);
    for (SCTAB nScTab = nFirstScTab; nScTab <= nLastScTab; ++nScTab)
    {
        if( rTabInfo.IsExternalTab( nScTab ) )
        {
            aRange.aStart.SetTab( nScTab );
            aRange.aEnd.SetTab( nScTab );
            maSBBuffer.StoreCellRange( aRange );
        }
    }
}

void XclExpLinkManagerImpl8::StoreCell( sal_uInt16 nFileId, const OUString& rTabName, const ScAddress& rPos )
{
    maSBBuffer.StoreCell(nFileId, rTabName, rPos);
}

void XclExpLinkManagerImpl8::StoreCellRange( sal_uInt16 nFileId, const OUString& rTabName, const ScRange& rRange )
{
    maSBBuffer.StoreCellRange(nFileId, rTabName, rRange);
}

std::optional<XclExpSBIndex> XclExpLinkManagerImpl8::InsertAddIn( const OUString& rName )
{
    const auto pResult = maSBBuffer.InsertAddIn( rName );
    if( pResult )
    {
        return XclExpSBIndex(InsertXti( XclExpXti( pResult->mnSupbook, EXC_TAB_EXTERNAL, EXC_TAB_EXTERNAL ) ), pResult->mnSBTab);
    }
    return {};
}

std::optional<XclExpSBIndex> XclExpLinkManagerImpl8::InsertEuroTool( const OUString& rName )
{
    const auto pResult = maSBBuffer.InsertEuroTool( rName );
    if( pResult )
    {
        return XclExpSBIndex(InsertXti( XclExpXti( pResult->mnSupbook, EXC_TAB_EXTERNAL, EXC_TAB_EXTERNAL ) ),
                             pResult->mnSBTab);
    }
    return {};
}

std::optional<XclExpSBIndex> XclExpLinkManagerImpl8::InsertDde(
     const OUString& rApplic, const OUString& rTopic, const OUString& rItem )
{
    const auto pResult = maSBBuffer.InsertDde( rApplic, rTopic, rItem );
    if( pResult )
    {
        return XclExpSBIndex(InsertXti( XclExpXti( pResult->mnSupbook, EXC_TAB_EXTERNAL, EXC_TAB_EXTERNAL ) ),
                             pResult->mnSBTab);
    }
    return {};
}

std::optional<XclExpSBIndex> XclExpLinkManagerImpl8::InsertExtName( const OUString& rUrl, const OUString& rName,
const ScExternalRefCache::TokenArrayRef& rArray )
{
    const auto pResult = maSBBuffer.InsertExtName( rUrl, rName, rArray );
    if( pResult )
    {
        return XclExpSBIndex(InsertXti( XclExpXti( pResult->mnSupbook, EXC_TAB_EXTERNAL, EXC_TAB_EXTERNAL ) ),
                             pResult->mnSBTab);
    }
    return {};
}

void XclExpLinkManagerImpl8::Save( XclExpStream& rStrm )
{
    if( maXtiVec.empty() )
        return;

    // SUPBOOKs, XCTs, CRNs, EXTERNNAMEs
    maSBBuffer.Save( rStrm );

    // EXTERNSHEET
    sal_uInt16 nCount = ulimit_cast< sal_uInt16 >( maXtiVec.size() );
    rStrm.StartRecord( EXC_ID_EXTERNSHEET, 2 + 6 * nCount );
    rStrm << nCount;
    rStrm.SetSliceSize( 6 );
    for( const auto& rXti : maXtiVec )
        rXti.Save( rStrm );
    rStrm.EndRecord();
}

void XclExpLinkManagerImpl8::SaveXml( XclExpXmlStream& rStrm )
{
    if (maSBBuffer.HasExternalReferences())
    {
        sax_fastparser::FSHelperPtr pWorkbook = rStrm.GetCurrentStream();
        pWorkbook->startElement(XML_externalReferences);

        // externalLink, externalBook, sheetNames, sheetDataSet, externalName
        maSBBuffer.SaveXml( rStrm );

        pWorkbook->endElement( XML_externalReferences);
    }

    // TODO: equivalent for EXTERNSHEET in OOXML?
#if 0
    if( !maXtiVec.empty() )
    {
        for( const auto& rXti : maXtiVec )
            rXti.SaveXml( rStrm );
    }
#endif
}

sal_uInt16 XclExpLinkManagerImpl8::InsertXti( const XclExpXti& rXti )
{
    auto aIt = std::find(maXtiVec.begin(), maXtiVec.end(), rXti);
    if (aIt != maXtiVec.end())
        return ulimit_cast< sal_uInt16 >( std::distance(maXtiVec.begin(), aIt) );
    maXtiVec.push_back( rXti );
    return ulimit_cast< sal_uInt16 >( maXtiVec.size() - 1 );
}

XclExpLinkManager::XclExpLinkManager( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot )
{
    switch( GetBiff() )
    {
        case EXC_BIFF5:
            mxImpl = std::make_shared<XclExpLinkManagerImpl5>( rRoot );
        break;
        case EXC_BIFF8:
            mxImpl = std::make_shared<XclExpLinkManagerImpl8>( rRoot );
        break;
        default:
            DBG_ERROR_BIFF();
    }
}

XclExpLinkManager::~XclExpLinkManager()
{
}

void XclExpLinkManager::FindExtSheet(
        sal_uInt16& rnExtSheet, sal_uInt16& rnXclTab,
        SCTAB nScTab, XclExpRefLogEntry* pRefLogEntry )
{
    mxImpl->FindExtSheet( rnExtSheet, rnXclTab, rnXclTab, nScTab, nScTab, pRefLogEntry );
}

void XclExpLinkManager::FindExtSheet(
        sal_uInt16& rnExtSheet, sal_uInt16& rnFirstXclTab, sal_uInt16& rnLastXclTab,
        SCTAB nFirstScTab, SCTAB nLastScTab, XclExpRefLogEntry* pRefLogEntry )
{
    mxImpl->FindExtSheet( rnExtSheet, rnFirstXclTab, rnLastXclTab, nFirstScTab, nLastScTab, pRefLogEntry );
}

sal_uInt16 XclExpLinkManager::FindExtSheet( sal_Unicode cCode )
{
    return mxImpl->FindExtSheet( cCode );
}

void XclExpLinkManager::FindExtSheet( sal_uInt16 nFileId, const OUString& rTabName, sal_uInt16 nXclTabSpan,
                                      sal_uInt16& rnExtSheet, sal_uInt16& rnFirstSBTab, sal_uInt16& rnLastSBTab,
                                      XclExpRefLogEntry* pRefLogEntry )
{
    mxImpl->FindExtSheet( nFileId, rTabName, nXclTabSpan, rnExtSheet, rnFirstSBTab, rnLastSBTab, pRefLogEntry );
}

void XclExpLinkManager::StoreCell( const ScSingleRefData& rRef, const ScAddress& rPos )
{
    mxImpl->StoreCellRange(rRef, rRef, rPos);
}

void XclExpLinkManager::StoreCellRange( const ScComplexRefData& rRef, const ScAddress& rPos )
{
    mxImpl->StoreCellRange(rRef.Ref1, rRef.Ref2, rPos);
}

void XclExpLinkManager::StoreCell( sal_uInt16 nFileId, const OUString& rTabName, const ScAddress& rPos )
{
    mxImpl->StoreCell(nFileId, rTabName, rPos);
}

void XclExpLinkManager::StoreCellRange( sal_uInt16 nFileId, const OUString& rTabName, const ScRange& rRange )
{
    mxImpl->StoreCellRange(nFileId, rTabName, rRange);
}

std::optional<XclExpSBIndex> XclExpLinkManager::InsertAddIn( const OUString& rName )
{
    return mxImpl->InsertAddIn( rName );
}

std::optional<XclExpSBIndex> XclExpLinkManager::InsertEuroTool( const OUString& rName )
{
    return mxImpl->InsertEuroTool( rName );
}

std::optional<XclExpSBIndex> XclExpLinkManager::InsertDde(
    const OUString& rApplic, const OUString& rTopic, const OUString& rItem )
{
    return mxImpl->InsertDde( rApplic, rTopic, rItem );
}

std::optional<XclExpSBIndex> XclExpLinkManager::InsertExtName( const OUString& rUrl, const OUString& rName,
    const ScExternalRefCache::TokenArrayRef& rArray )
{
    return mxImpl->InsertExtName( rUrl, rName, rArray);
}

void XclExpLinkManager::Save( XclExpStream& rStrm )
{
    mxImpl->Save( rStrm );
}

void XclExpLinkManager::SaveXml( XclExpXmlStream& rStrm )
{
    mxImpl->SaveXml( rStrm );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
