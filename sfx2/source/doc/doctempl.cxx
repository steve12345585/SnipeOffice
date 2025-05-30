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


#include <limits.h>
#include <mutex>
#include <string_view>

#include <com/sun/star/uno/Any.h>
#include <sal/log.hxx>

#include <unotools/pathoptions.hxx>
#include <tools/urlobj.hxx>
#include <tools/debug.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/propertyvalue.hxx>
#include <ucbhelper/content.hxx>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XPropertySetInfo.hpp>
#include <com/sun/star/document/XTypeDetection.hpp>
#include <com/sun/star/document/DocumentProperties.hpp>
#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>
#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/frame/DocumentTemplates.hpp>
#include <com/sun/star/frame/XDocumentTemplates.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/io/XPersist.hpp>
#include <com/sun/star/lang/XLocalizable.hpp>
#include <com/sun/star/sdbc/XResultSet.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/ucb/ContentCreationException.hpp>
#include <com/sun/star/ucb/NameClash.hpp>
#include <com/sun/star/ucb/TransferInfo.hpp>
#include <com/sun/star/ucb/XContent.hpp>
#include <com/sun/star/ucb/XContentAccess.hpp>
#include <com/sun/star/ucb/AnyCompareFactory.hpp>
#include <com/sun/star/ucb/NumberedSortingInfo.hpp>

#include "doctemplateslocal.hxx"
#include <sfxurlrelocator.hxx>

#include <sfx2/doctempl.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/sfxresid.hxx>
#include <sfx2/strings.hrc>
#include <strings.hxx>
#include <svtools/templatefoldercache.hxx>

#include <memory>
#include <utility>
#include <vector>


using namespace ::com::sun::star;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::ucb;
using namespace ::com::sun::star::document;
using namespace ::rtl;
using namespace ::ucbhelper;

constexpr OUString TITLE = u"Title"_ustr;
constexpr OUString TARGET_URL = u"TargetURL"_ustr;

constexpr OUStringLiteral COMMAND_TRANSFER = u"transfer";

namespace {

class RegionData_Impl;

}

namespace DocTempl {

namespace {

class DocTempl_EntryData_Impl
{
    // the following member must be SfxObjectShellLock since it controls that SfxObjectShell lifetime by design
    // and users of this class expect it to be so.
    SfxObjectShellLock  mxObjShell;

    OUString            maTitle;
    OUString            maOwnURL;
    OUString            maTargetURL;

public:
                        DocTempl_EntryData_Impl(const OUString& rTitle);

    const OUString&     GetTitle() const { return maTitle; }
    const OUString&     GetTargetURL(const INetURLObject& rRootURL);
    const OUString&     GetHierarchyURL(const INetURLObject& rRootURL);

    void                SetTitle( const OUString& rTitle ) { maTitle = rTitle; }
    void                SetTargetURL( const OUString& rURL ) { maTargetURL = rURL; }
    void                SetHierarchyURL( const OUString& rURL) { maOwnURL = rURL; }

    int                 Compare( std::u16string_view rTitle ) const;
};

}

}

using namespace ::DocTempl;

namespace {

class RegionData_Impl
{
    std::vector<std::unique_ptr<DocTempl_EntryData_Impl>> maEntries;
    OUString                    maTitle;
    OUString                    maOwnURL;

private:
    size_t                      GetEntryPos( std::u16string_view rTitle,
                                             bool& rFound ) const;

public:
                        RegionData_Impl(OUString aTitle);

    void                SetHierarchyURL( const OUString& rURL) { maOwnURL = rURL; }

    DocTempl_EntryData_Impl*     GetEntry( size_t nIndex ) const;
    DocTempl_EntryData_Impl*     GetEntry( std::u16string_view rName ) const;

    const OUString&     GetTitle() const { return maTitle; }
    const OUString&     GetHierarchyURL(const INetURLObject& rRootURL);

    size_t              GetCount() const;

    void                SetTitle( const OUString& rTitle ) { maTitle = rTitle; }

    void                AddEntry(const INetURLObject& rRootURL,
                                 const OUString& rTitle,
                                 const OUString& rTargetURL,
                                 const size_t *pPos);
    void                DeleteEntry( size_t nIndex );

    int                 Compare( RegionData_Impl const * pCompareWith ) const;
};

}

class SfxDocTemplate_Impl : public SvRefBase
{
    uno::Reference< XPersist >               mxInfo;
    uno::Reference< XDocumentTemplates >     mxTemplates;

    mutable std::mutex  maMutex;
    OUString            maRootURL;
    OUString            maStandardGroup;
    std::vector<std::unique_ptr<RegionData_Impl>> maRegions;
    bool            mbConstructed;

    uno::Reference< XAnyCompareFactory > m_rCompareFactory;

    // the following member is intended to prevent clearing of the global data when it is in use
    // TODO/LATER: it still does not make the implementation complete thread-safe
    sal_Int32           mnLockCounter;

private:
    void                Clear();

public:
                        SfxDocTemplate_Impl();
                        virtual ~SfxDocTemplate_Impl() override;

    void                IncrementLock();
    void                DecrementLock();

    bool            Construct( );
    void                CreateFromHierarchy( std::unique_lock<std::mutex>& rGuard, Content &rTemplRoot );
    void                ReInitFromComponent();
    void                AddRegion( std::unique_lock<std::mutex>& rGuard,
                                   const OUString& rTitle,
                                   Content& rContent );

    void                Rescan();

    void                DeleteRegion( size_t nIndex );

    size_t              GetRegionCount() const
                            { return maRegions.size(); }
    RegionData_Impl*    GetRegion( std::u16string_view rName ) const;
    RegionData_Impl*    GetRegion( size_t nIndex ) const;

    bool            GetTitleFromURL( const OUString& rURL, OUString& aTitle );
    bool            InsertRegion( std::unique_ptr<RegionData_Impl> pData, size_t nPos );

    INetURLObject   GetRootURL() const
    {
        std::unique_lock aGuard(maMutex);
        return INetURLObject(maRootURL);
    }

    uno::Reference<XDocumentTemplates> getDocTemplates() const
    {
        std::unique_lock aGuard(maMutex);
        return mxTemplates;
    }
};

namespace {

class DocTemplLocker_Impl
{
    SfxDocTemplate_Impl& m_aDocTempl;
public:
    explicit DocTemplLocker_Impl( SfxDocTemplate_Impl& aDocTempl )
    : m_aDocTempl( aDocTempl )
    {
        m_aDocTempl.IncrementLock();
    }

    ~DocTemplLocker_Impl()
    {
        m_aDocTempl.DecrementLock();
    }
};

}

static SfxDocTemplate_Impl *gpTemplateData = nullptr;


static bool getTextProperty_Impl( Content& rContent,
                                      const OUString& rPropName,
                                      OUString& rPropValue );


OUString SfxDocumentTemplates::GetFullRegionName
(
    sal_uInt16 nIdx                     // Region Index
)   const

/*  [Description]

    Returns the logical name of a region and its path

    [Return value]                 Reference to the Region name

*/

{
    // First: find the RegionData for the index

    DocTemplLocker_Impl aLocker( *pImp );

    if ( pImp->Construct() )
    {
        RegionData_Impl *pData1 = pImp->GetRegion( nIdx );

        if ( pData1 )
            return pData1->GetTitle();

        // --**-- here was some code which appended the path to the
        //      group if there was more than one with the same name.
        //      this should not happen anymore
    }

    return OUString();
}


OUString SfxDocumentTemplates::GetRegionName
(
    sal_uInt16 nIdx                 // Region Index
)   const

/*  [Description]

    Returns the logical name of a region

    [Return value]

    const String&                   Reference to the Region name

*/
{
    DocTemplLocker_Impl aLocker( *pImp );

    if ( pImp->Construct() )
    {
        RegionData_Impl *pData = pImp->GetRegion( nIdx );

        if ( pData )
            return pData->GetTitle();
    }

    return OUString();
}


sal_uInt16 SfxDocumentTemplates::GetRegionCount() const

/*  [Description]

    Returns the number of Regions

    [Return value]

    sal_uInt16                  Number of Regions
*/
{
    DocTemplLocker_Impl aLocker( *pImp );

    if ( !pImp->Construct() )
        return 0;

    return pImp->GetRegionCount();
}


sal_uInt16 SfxDocumentTemplates::GetCount
(
    sal_uInt16 nRegion              /* Region index whose number is
                                   to be determined */

)   const

/*  [Description]

    Number of entries in Region

    [Return value]                 Number of entries
*/

{
    DocTemplLocker_Impl aLocker( *pImp );

    if ( !pImp->Construct() )
        return 0;

    RegionData_Impl *pData = pImp->GetRegion( nRegion );

    if ( !pData )
        return 0;

    return pData->GetCount();
}


OUString SfxDocumentTemplates::GetName
(
    sal_uInt16 nRegion,     //  Region Index, in which the entry lies
    sal_uInt16 nIdx         //  Index of the entry
)   const

/*  [Description]

    Returns the logical name of an entry in Region

    [Return value]

    const String&           Entry Name
*/

{
    DocTemplLocker_Impl aLocker( *pImp );

    if ( pImp->Construct() )
    {
        RegionData_Impl *pRegion = pImp->GetRegion( nRegion );

        if ( pRegion )
        {
            DocTempl_EntryData_Impl *pEntry = pRegion->GetEntry( nIdx );
            if ( pEntry )
                return pEntry->GetTitle();
        }
    }

    return OUString();
}


OUString SfxDocumentTemplates::GetPath
(
    sal_uInt16  nRegion,    //  Region Index, in which the entry lies
    sal_uInt16  nIdx        //  Index of the entry
)   const

/*  [Description]

    Returns the file name with full path to the file assigned to an entry

    [Return value]

    String                  File name with full path
*/
{
    DocTemplLocker_Impl aLocker( *pImp );

    if ( !pImp->Construct() )
        return OUString();

    RegionData_Impl *pRegion = pImp->GetRegion( nRegion );

    if ( pRegion )
    {
        DocTempl_EntryData_Impl *pEntry = pRegion->GetEntry( nIdx );
        if ( pEntry )
            return pEntry->GetTargetURL(pImp->GetRootURL());
    }

    return OUString();
}

OUString SfxDocumentTemplates::GetTemplateTargetURLFromComponent( std::u16string_view aGroupName,
                                                                    std::u16string_view aTitle )
{
    DocTemplLocker_Impl aLocker( *pImp );

    INetURLObject aTemplateObj( pImp->GetRootURL() );

    aTemplateObj.insertName( aGroupName, false,
                        INetURLObject::LAST_SEGMENT,
                        INetURLObject::EncodeMechanism::All );

    aTemplateObj.insertName( aTitle, false,
                        INetURLObject::LAST_SEGMENT,
                        INetURLObject::EncodeMechanism::All );


    Content aTemplate;
    uno::Reference< XCommandEnvironment > aCmdEnv;
    if ( Content::create( aTemplateObj.GetMainURL( INetURLObject::DecodeMechanism::NONE ), aCmdEnv, comphelper::getProcessComponentContext(), aTemplate ) )
    {
        OUString aResult;
        getTextProperty_Impl( aTemplate, TARGET_URL, aResult );
        return SvtPathOptions().SubstituteVariable( aResult );
    }

    return OUString();
}


/** Convert a template name to its localised pair if it exists.
    @param rString
        Name to be translated.
    @return
        The localised pair of rString or rString if the former does not exist.
*/
OUString SfxDocumentTemplates::ConvertResourceString(const OUString& rString)
{
    static constexpr OUString aTemplateNames[] =
    {
        STR_TEMPLATE_NAME1_DEF,
        STR_TEMPLATE_NAME2_DEF,
        STR_TEMPLATE_NAME3_DEF,
        STR_TEMPLATE_NAME4_DEF,
        STR_TEMPLATE_NAME5_DEF,
        STR_TEMPLATE_NAME6_DEF,
        STR_TEMPLATE_NAME7_DEF,
        STR_TEMPLATE_NAME8_DEF,
        STR_TEMPLATE_NAME9_DEF,
        STR_TEMPLATE_NAME10_DEF,
        STR_TEMPLATE_NAME11_DEF,
        STR_TEMPLATE_NAME12_DEF,
        STR_TEMPLATE_NAME13_DEF,
        STR_TEMPLATE_NAME14_DEF,
        STR_TEMPLATE_NAME15_DEF,
        STR_TEMPLATE_NAME16_DEF,
        STR_TEMPLATE_NAME17_DEF,
        STR_TEMPLATE_NAME18_DEF,
        STR_TEMPLATE_NAME19_DEF,
        STR_TEMPLATE_NAME20_DEF,
        STR_TEMPLATE_NAME21_DEF,
        STR_TEMPLATE_NAME22_DEF,
        STR_TEMPLATE_NAME23_DEF,
        STR_TEMPLATE_NAME24_DEF,
        STR_TEMPLATE_NAME25_DEF,
        STR_TEMPLATE_NAME26_DEF,
        STR_TEMPLATE_NAME27_DEF,
        STR_TEMPLATE_NAME28_DEF,
        STR_TEMPLATE_NAME29_DEF,
        STR_TEMPLATE_NAME30_DEF,
        STR_TEMPLATE_NAME31_DEF,
        STR_TEMPLATE_NAME32_DEF,
        STR_TEMPLATE_NAME33_DEF,
        STR_TEMPLATE_NAME34_DEF
    };

    TranslateId STR_TEMPLATE_NAME[] =
    {
        STR_TEMPLATE_NAME1,
        STR_TEMPLATE_NAME2,
        STR_TEMPLATE_NAME3,
        STR_TEMPLATE_NAME4,
        STR_TEMPLATE_NAME5,
        STR_TEMPLATE_NAME6,
        STR_TEMPLATE_NAME7,
        STR_TEMPLATE_NAME8,
        STR_TEMPLATE_NAME9,
        STR_TEMPLATE_NAME10,
        STR_TEMPLATE_NAME11,
        STR_TEMPLATE_NAME12,
        STR_TEMPLATE_NAME13,
        STR_TEMPLATE_NAME14,
        STR_TEMPLATE_NAME15,
        STR_TEMPLATE_NAME16,
        STR_TEMPLATE_NAME17,
        STR_TEMPLATE_NAME18,
        STR_TEMPLATE_NAME19,
        STR_TEMPLATE_NAME20,
        STR_TEMPLATE_NAME21,
        STR_TEMPLATE_NAME22,
        STR_TEMPLATE_NAME23,
        STR_TEMPLATE_NAME24,
        STR_TEMPLATE_NAME25,
        STR_TEMPLATE_NAME26,
        STR_TEMPLATE_NAME27,
        STR_TEMPLATE_NAME28,
        STR_TEMPLATE_NAME29,
        STR_TEMPLATE_NAME30,
        STR_TEMPLATE_NAME31,
        STR_TEMPLATE_NAME32,
        STR_TEMPLATE_NAME33,
        STR_TEMPLATE_NAME34
    };

    static_assert(SAL_N_ELEMENTS(aTemplateNames) == SAL_N_ELEMENTS(STR_TEMPLATE_NAME));

    for (size_t i = 0; i < SAL_N_ELEMENTS(STR_TEMPLATE_NAME); ++i)
    {
        if (rString == aTemplateNames[i])
            return SfxResId(STR_TEMPLATE_NAME[i]);
    }
    return rString;
}


bool SfxDocumentTemplates::CopyOrMove
(
    sal_uInt16  nTargetRegion,      //  Target Region Index
    sal_uInt16  nTargetIdx,         //  Target position Index
    sal_uInt16  nSourceRegion,      //  Source Region Index
    sal_uInt16  nSourceIdx,         /*  Index to be copied / to moved template */
    bool        bMove               //  Copy / Move
)

/*  [Description]

    Copy or move a document template

    [Return value]

    sal_Bool            sal_True,   Action could be performed
                        sal_False,  Action could not be performed

    [Cross-references]

    <SfxDocumentTemplates::Move(sal_uInt16,sal_uInt16,sal_uInt16,sal_uInt16)>
    <SfxDocumentTemplates::Copy(sal_uInt16,sal_uInt16,sal_uInt16,sal_uInt16)>
*/

{
    /* to perform a copy or move, we need to send a transfer command to
       the destination folder with the URL of the source as parameter.
       ( If the destination content doesn't support the transfer command,
       we could try a copy ( and delete ) instead. )
       We need two transfers ( one for the real template and one for its
       representation in the hierarchy )
       ...
    */

    DocTemplLocker_Impl aLocker( *pImp );

    if ( !pImp->Construct() )
        return false;

    // Don't copy or move any folders
    if( nSourceIdx == USHRT_MAX )
        return false ;

    if ( nSourceRegion == nTargetRegion )
    {
        SAL_WARN( "sfx.doc", "Don't know, what to do!" );
        return false;
    }

    RegionData_Impl *pSourceRgn = pImp->GetRegion( nSourceRegion );
    if ( !pSourceRgn )
        return false;

    DocTempl_EntryData_Impl *pSource = pSourceRgn->GetEntry( nSourceIdx );
    if ( !pSource )
        return false;

    RegionData_Impl *pTargetRgn = pImp->GetRegion( nTargetRegion );
    if ( !pTargetRgn )
        return false;

    const OUString aTitle = pSource->GetTitle();

    uno::Reference< XDocumentTemplates > xTemplates = pImp->getDocTemplates();

    if ( xTemplates->addTemplate( pTargetRgn->GetTitle(),
                                  aTitle,
                                  pSource->GetTargetURL(pImp->GetRootURL()) ) )
    {
        const OUString aNewTargetURL = GetTemplateTargetURLFromComponent( pTargetRgn->GetTitle(), aTitle );
        if ( aNewTargetURL.isEmpty() )
            return false;

        if ( bMove )
        {
            // --**-- delete the original file
            bool bDeleted = xTemplates->removeTemplate( pSourceRgn->GetTitle(),
                                                            pSource->GetTitle() );
            if ( bDeleted )
                pSourceRgn->DeleteEntry( nSourceIdx );
            else
            {
                if ( xTemplates->removeTemplate( pTargetRgn->GetTitle(), aTitle ) )
                    return false; // will trigger retry with copy instead of move

                // if it is not possible to remove just created template ( must be possible! )
                // it is better to report success here, since at least the copy has succeeded
                // TODO/LATER: solve it more gracefully in future
            }
        }

        // todo: fix SfxDocumentTemplates to handle size_t instead of sal_uInt16
        size_t temp_nTargetIdx = nTargetIdx;
        pTargetRgn->AddEntry(pImp->GetRootURL(), aTitle, aNewTargetURL, &temp_nTargetIdx);

        return true;
    }

    // --**-- if the current file is opened,
    // it must be re-opened afterwards.

    return false;
}

bool SfxDocumentTemplates::Move
(
    sal_uInt16 nTargetRegion,       //  Target Region Index
    sal_uInt16 nTargetIdx,          //  Target position Index
    sal_uInt16 nSourceRegion,       //  Source Region Index
    sal_uInt16 nSourceIdx           /*  Index to be copied / to moved template */
)

/*  [Description]

    Moving a template

    [Return value]

    sal_Bool            sal_True,   Action could be performed
                        sal_False,  Action could not be performed

    [Cross-references]

    <SfxDocumentTemplates::CopyOrMove(sal_uInt16,sal_uInt16,sal_uInt16,sal_uInt16,sal_Bool)>
*/
{
    DocTemplLocker_Impl aLocker( *pImp );

    return CopyOrMove( nTargetRegion, nTargetIdx,
                       nSourceRegion, nSourceIdx, true );
}


bool SfxDocumentTemplates::Copy
(
    sal_uInt16 nTargetRegion,       //  Target Region Index
    sal_uInt16 nTargetIdx,          //  Target position Index
    sal_uInt16 nSourceRegion,       //  Source Region Index
    sal_uInt16 nSourceIdx           /*  Index to be copied / to moved template */
)

/*  [Description]

    Copying a template

    [Return value]

    sal_Bool            sal_True,   Action could be performed
                        sal_False,  Action could not be performed

    [Cross-references]

    <SfxDocumentTemplates::CopyOrMove(sal_uInt16,sal_uInt16,sal_uInt16,sal_uInt16,sal_Bool)>
*/

{
    DocTemplLocker_Impl aLocker( *pImp );

    return CopyOrMove( nTargetRegion, nTargetIdx,
                       nSourceRegion, nSourceIdx, false );
}


bool SfxDocumentTemplates::CopyTo
(
    sal_uInt16          nRegion,    //  Region of the template to be exported
    sal_uInt16          nIdx,       //  Index of the template to be exported
    std::u16string_view rName       /*  File name under which the template is to
                                    be created */
)   const

/*  [Description]

    Exporting a template into the file system

    [Return value]

    sal_Bool            sal_True,   Action could be performed
                        sal_False,  Action could not be performed

    [Cross-references]

    <SfxDocumentTemplates::CopyFrom(sal_uInt16,sal_uInt16,String&)>
*/

{
    DocTemplLocker_Impl aLocker( *pImp );

    if ( ! pImp->Construct() )
        return false;

    RegionData_Impl *pSourceRgn = pImp->GetRegion( nRegion );
    if ( !pSourceRgn )
        return false;

    DocTempl_EntryData_Impl *pSource = pSourceRgn->GetEntry( nIdx );
    if ( !pSource )
        return false;

    INetURLObject aTargetURL( rName );

    const OUString aTitle( aTargetURL.getName( INetURLObject::LAST_SEGMENT, true,
                                         INetURLObject::DecodeMechanism::WithCharset ) );
    aTargetURL.removeSegment();

    const OUString aParentURL = aTargetURL.GetMainURL( INetURLObject::DecodeMechanism::NONE );

    uno::Reference< XCommandEnvironment > aCmdEnv;
    Content aTarget;

    try
    {
        aTarget = Content( aParentURL, aCmdEnv, comphelper::getProcessComponentContext() );

        TransferInfo aTransferInfo;
        aTransferInfo.MoveData = false;
        aTransferInfo.SourceURL = pSource->GetTargetURL(pImp->GetRootURL());
        aTransferInfo.NewTitle = aTitle;
        aTransferInfo.NameClash = NameClash::RENAME;

        Any aArg( aTransferInfo );
        aTarget.executeCommand( COMMAND_TRANSFER, aArg );
    }
    catch ( ContentCreationException& )
    { return false; }
    catch ( Exception& )
    { return false; }

    return true;
}


bool SfxDocumentTemplates::CopyFrom
(
    sal_uInt16      nRegion,        /*  Region in which the template is to be
                                    imported */
    sal_uInt16      nIdx,           //  Index of the new template in this Region
    OUString&       rName           /*  File name of the template to be imported
                                    as an out parameter of the (automatically
                                    generated from the file name) logical name
                                    of the template */
)

/*  [Description]

    Import a template from the file system

    [Return value]                 Success (sal_True) or serfpTargetDirectory->GetContent());

    sal_Bool            sal_True,   Action could be performed
                        sal_False,  Action could not be performed

    [Cross-references]

    <SfxDocumentTemplates::CopyTo(sal_uInt16,sal_uInt16,const String&)>
*/

{
    DocTemplLocker_Impl aLocker( *pImp );

    if ( ! pImp->Construct() )
        return false;

    RegionData_Impl *pTargetRgn = pImp->GetRegion( nRegion );

    if ( !pTargetRgn )
        return false;

    uno::Reference< XDocumentTemplates > xTemplates = pImp->getDocTemplates();
    if ( !xTemplates.is() )
        return false;

    OUString aTitle;
    bool bTemplateAdded = false;

    if( pImp->GetTitleFromURL( rName, aTitle ) )
    {
        bTemplateAdded = xTemplates->addTemplate( pTargetRgn->GetTitle(), aTitle, rName );
    }
    else
    {
        uno::Reference< XDesktop2 > xDesktop = Desktop::create( ::comphelper::getProcessComponentContext() );

        Sequence< PropertyValue > aArgs{ comphelper::makePropertyValue(u"Hidden"_ustr, true) };

        INetURLObject   aTemplURL( rName );
        uno::Reference< XDocumentPropertiesSupplier > xDocPropsSupplier;
        uno::Reference< XStorable > xStorable;
        try
        {
            xStorable.set(
                xDesktop->loadComponentFromURL( aTemplURL.GetMainURL(INetURLObject::DecodeMechanism::NONE),
                                                u"_blank"_ustr,
                                                0,
                                                aArgs ),
                UNO_QUERY );

            xDocPropsSupplier.set( xStorable, UNO_QUERY );
        }
        catch( Exception& )
        {
        }

        if( xStorable.is() )
        {
            // get Title from XDocumentPropertiesSupplier
            if( xDocPropsSupplier.is() )
            {
                uno::Reference< XDocumentProperties > xDocProps
                    = xDocPropsSupplier->getDocumentProperties();
                if (xDocProps.is() ) {
                    aTitle = xDocProps->getTitle();
                }
            }

            if( aTitle.isEmpty() )
            {
                INetURLObject aURL(std::move(aTemplURL));
                aURL.CutExtension();
                aTitle = aURL.getName( INetURLObject::LAST_SEGMENT, true,
                                        INetURLObject::DecodeMechanism::WithCharset );
            }

            // write a template using XStorable interface
            bTemplateAdded = xTemplates->storeTemplate( pTargetRgn->GetTitle(), aTitle, xStorable );
        }
    }


    if( bTemplateAdded )
    {
        INetURLObject aTemplObj(pTargetRgn->GetHierarchyURL(pImp->GetRootURL()));
        aTemplObj.insertName( aTitle, false,
                              INetURLObject::LAST_SEGMENT,
                              INetURLObject::EncodeMechanism::All );
        const OUString aTemplURL = aTemplObj.GetMainURL( INetURLObject::DecodeMechanism::NONE );

        uno::Reference< XCommandEnvironment > aCmdEnv;
        Content aTemplCont;

        if( Content::create( aTemplURL, aCmdEnv, comphelper::getProcessComponentContext(), aTemplCont ) )
        {
            OUString aTemplName;
            if( getTextProperty_Impl( aTemplCont, TARGET_URL, aTemplName ) )
            {
                if ( nIdx == USHRT_MAX )
                    nIdx = 0;
                else
                    ++nIdx;

                // todo: fix SfxDocumentTemplates to handle size_t instead of sal_uInt16
                size_t temp_nIdx = nIdx;
                pTargetRgn->AddEntry(pImp->GetRootURL(), aTitle, aTemplName, &temp_nIdx);
                rName = aTitle;
                return true;
            }
            else
            {
                SAL_WARN( "sfx.doc", "CopyFrom(): The content should contain target URL!" );
            }
        }
        else
        {
            SAL_WARN( "sfx.doc", "CopyFrom(): The content just was created!" );
        }
    }

    return false;
}


bool SfxDocumentTemplates::Delete
(
    sal_uInt16 nRegion,             //  Region Index
    sal_uInt16 nIdx                 /*  Index of the entry or USHRT_MAX,
                                    if a directory is meant. */
)

/*  [Description]

    Deleting an entry or a directory

    [Return value]

    sal_Bool            sal_True,   Action could be performed
                        sal_False,  Action could not be performed

    [Cross-references]

    <SfxDocumentTemplates::InsertDir(const String&,sal_uInt16)>
    <SfxDocumentTemplates::KillDir(SfxTemplateDir&)>
*/

{
    DocTemplLocker_Impl aLocker( *pImp );

    /* delete the template or folder in the hierarchy and in the
       template folder by sending a delete command to the content.
       Then remove the data from the lists
    */
    if ( ! pImp->Construct() )
        return false;

    RegionData_Impl *pRegion = pImp->GetRegion( nRegion );

    if ( !pRegion )
        return false;

    bool bRet;
    uno::Reference< XDocumentTemplates > xTemplates = pImp->getDocTemplates();

    if ( nIdx == USHRT_MAX )
    {
        bRet = xTemplates->removeGroup( pRegion->GetTitle() );
        if ( bRet )
            pImp->DeleteRegion( nRegion );
    }
    else
    {
        DocTempl_EntryData_Impl *pEntry = pRegion->GetEntry( nIdx );

        if ( !pEntry )
            return false;

        bRet = xTemplates->removeTemplate( pRegion->GetTitle(),
                                           pEntry->GetTitle() );
        if( bRet )
            pRegion->DeleteEntry( nIdx );
    }

    return bRet;
}


bool SfxDocumentTemplates::InsertDir
(
    const OUString&     rText,      //  the logical name of the new Region
    sal_uInt16          nRegion     //  Region Index
)

/*  [Description]

    Insert an index

    [Return value]

    sal_Bool            sal_True,   Action could be performed
                        sal_False,  Action could not be performed

    [Cross-references]

    <SfxDocumentTemplates::KillDir(SfxTemplateDir&)>
*/
{
    DocTemplLocker_Impl aLocker( *pImp );

    if ( ! pImp->Construct() )
        return false;

    RegionData_Impl *pRegion = pImp->GetRegion( rText );

    if ( pRegion )
        return false;

    uno::Reference< XDocumentTemplates > xTemplates = pImp->getDocTemplates();

    if (xTemplates->addGroup(rText))
        return pImp->InsertRegion(std::make_unique<RegionData_Impl>(rText), nRegion);

    return false;
}

bool SfxDocumentTemplates::InsertTemplate(sal_uInt16 nSourceRegion, sal_uInt16 nIdx, const OUString &rName, const OUString &rPath)
{
    DocTemplLocker_Impl aLocker( *pImp );

    if ( ! pImp->Construct() )
        return false;

    RegionData_Impl *pRegion = pImp->GetRegion( nSourceRegion );

    if ( !pRegion )
        return false;

    size_t pos = nIdx;
    pRegion->AddEntry(pImp->GetRootURL(), rName, rPath, &pos);

    return true;
}

bool SfxDocumentTemplates::SetName( const OUString& rName, sal_uInt16 nRegion, sal_uInt16 nIdx )

{
    DocTemplLocker_Impl aLocker( *pImp );

    if ( ! pImp->Construct() )
        return false;

    RegionData_Impl *pRegion = pImp->GetRegion( nRegion );

    if ( !pRegion )
        return false;

    uno::Reference< XDocumentTemplates > xTemplates = pImp->getDocTemplates();

    if ( nIdx == USHRT_MAX )
    {
        if ( pRegion->GetTitle() == rName )
            return true;

        // we have to rename a region
        if ( xTemplates->renameGroup( pRegion->GetTitle(), rName ) )
        {
            pRegion->SetTitle( rName );
            pRegion->SetHierarchyURL( u""_ustr );
            return true;
        }
    }
    else
    {
        DocTempl_EntryData_Impl *pEntry = pRegion->GetEntry( nIdx );

        if ( !pEntry )
            return false;

        if ( pEntry->GetTitle() == rName )
            return true;

        if ( xTemplates->renameTemplate( pRegion->GetTitle(),
                                         pEntry->GetTitle(),
                                         rName ) )
        {
            pEntry->SetTitle( rName );
            pEntry->SetTargetURL( u""_ustr );
            pEntry->SetHierarchyURL( u""_ustr );
            return true;
        }
    }

    return false;
}


bool SfxDocumentTemplates::GetFull
(
    std::u16string_view rRegion,      // Region Name
    std::u16string_view rName,    // Template Name
    OUString &rPath               // Out: Path + File name
)

/*  [Description]

    Returns Path + File name of the template specified by rRegion and rName.

    [Return value]

    sal_Bool            sal_True,   Action could be performed
                        sal_False,  Action could not be performed

    [Cross-references]

    <SfxDocumentTemplates::GetLogicNames(const String&,String&,String&)>
*/

{
    DocTemplLocker_Impl aLocker( *pImp );

    // We don't search for empty names!
    if ( rName.empty() )
        return false;

    if ( ! pImp->Construct() )
        return false;

    DocTempl_EntryData_Impl* pEntry = nullptr;
    const sal_uInt16 nCount = GetRegionCount();

    for ( sal_uInt16 i = 0; i < nCount; ++i )
    {
        RegionData_Impl *pRegion = pImp->GetRegion( i );

        if( pRegion &&
            ( rRegion.empty() || ( rRegion == pRegion->GetTitle() ) ) )
        {
            pEntry = pRegion->GetEntry( rName );

            if ( pEntry )
            {
                rPath = pEntry->GetTargetURL(pImp->GetRootURL());
                break;
            }
        }
    }

    return ( pEntry != nullptr );
}


bool SfxDocumentTemplates::GetLogicNames
(
    std::u16string_view rPath,        // Full Path to the template
    OUString &rRegion,                // Out: Region name
    OUString &rName                   // Out: Template name
) const

/*  [Description]

    Returns and logical path name to the template specified by rPath

    [Return value]

    sal_Bool            sal_True,   Action could be performed
                        sal_False,  Action could not be performed

    [Cross-references]

    <SfxDocumentTemplates::GetFull(const String&,const String&,DirEntry&)>
*/

{
    DocTemplLocker_Impl aLocker( *pImp );

    if ( ! pImp->Construct() )
        return false;

    INetURLObject aFullPath;

    aFullPath.SetSmartProtocol( INetProtocol::File );
    aFullPath.SetURL( rPath );
    const OUString aPath( aFullPath.GetMainURL( INetURLObject::DecodeMechanism::NONE ) );

    const sal_uInt16 nCount = GetRegionCount();

    for ( sal_uInt16 i=0; i<nCount; ++i )
    {
        RegionData_Impl *pData = pImp->GetRegion( i );
        if ( pData )
        {
            const sal_uInt16 nChildCount = pData->GetCount();

            for ( sal_uInt16 j=0; j<nChildCount; ++j )
            {
                DocTempl_EntryData_Impl *pEntry = pData->GetEntry( j );
                if ( pEntry && pEntry->GetTargetURL(pImp->GetRootURL()) == aPath )
                {
                    rRegion = pData->GetTitle();
                    rName = pEntry->GetTitle();
                    return true;
                }
            }
        }
    }

    return false;
}


SfxDocumentTemplates::SfxDocumentTemplates()

/*  [Description]

    Constructor
*/
{
    if ( !gpTemplateData )
        gpTemplateData = new SfxDocTemplate_Impl;

    pImp = gpTemplateData;
}


SfxDocumentTemplates::~SfxDocumentTemplates()

/*  [Description]

    Destructor
    Release of administrative data
*/

{
    pImp = nullptr;
}

void SfxDocumentTemplates::Update( )
{
    if ( ::svt::TemplateFolderCache( true ).needsUpdate() )   // update is really necessary
    {
        if ( pImp->Construct() )
            pImp->Rescan();
    }
}

void SfxDocumentTemplates::ReInitFromComponent()
{
    pImp->ReInitFromComponent();
}

DocTempl_EntryData_Impl::DocTempl_EntryData_Impl(const OUString& rTitle)
    : maTitle(SfxDocumentTemplates::ConvertResourceString(rTitle))
{
}

int DocTempl_EntryData_Impl::Compare( std::u16string_view rTitle ) const
{
    return maTitle.compareTo( rTitle );
}

const OUString& DocTempl_EntryData_Impl::GetHierarchyURL(const INetURLObject& rRootURL)
{
    if ( maOwnURL.isEmpty() )
    {
        INetURLObject aTemplateObj(rRootURL);

        aTemplateObj.insertName( GetTitle(), false,
                     INetURLObject::LAST_SEGMENT,
                     INetURLObject::EncodeMechanism::All );

        maOwnURL = aTemplateObj.GetMainURL( INetURLObject::DecodeMechanism::NONE );
        DBG_ASSERT( !maOwnURL.isEmpty(), "GetHierarchyURL(): Could not create URL!" );
    }

    return maOwnURL;
}

const OUString& DocTempl_EntryData_Impl::GetTargetURL(const INetURLObject& rRootURL)
{
    if ( maTargetURL.isEmpty() )
    {
        uno::Reference< XCommandEnvironment > aCmdEnv;
        Content aRegion;

        if ( Content::create( GetHierarchyURL(rRootURL), aCmdEnv, comphelper::getProcessComponentContext(), aRegion ) )
        {
            getTextProperty_Impl( aRegion, TARGET_URL, maTargetURL );
        }
        else
        {
            SAL_WARN( "sfx.doc", "GetTargetURL(): Could not create hierarchy content!" );
        }
    }

    return maTargetURL;
}

RegionData_Impl::RegionData_Impl(OUString aTitle)
    : maTitle(std::move(aTitle))
{
}


size_t RegionData_Impl::GetEntryPos( std::u16string_view rTitle, bool& rFound ) const
{
    const size_t nCount = maEntries.size();

    for ( size_t i=0; i<nCount; ++i )
    {
        auto &pData = maEntries[ i ];

        if ( pData->Compare( rTitle ) == 0 )
        {
            rFound = true;
            return i;
        }
    }

    rFound = false;
    return nCount;
}

void RegionData_Impl::AddEntry(const INetURLObject& rRootURL,
                               const OUString& rTitle,
                               const OUString& rTargetURL,
                               const size_t *pPos)
{
    INetURLObject aLinkObj( GetHierarchyURL(rRootURL) );
    aLinkObj.insertName( rTitle, false,
                      INetURLObject::LAST_SEGMENT,
                      INetURLObject::EncodeMechanism::All );
    const OUString aLinkURL = aLinkObj.GetMainURL( INetURLObject::DecodeMechanism::NONE );

    bool        bFound = false;
    size_t          nPos = GetEntryPos( rTitle, bFound );

    if ( bFound )
        return;

    if ( pPos )
        nPos = *pPos;

    auto pEntry = std::make_unique<DocTempl_EntryData_Impl>(rTitle);
    pEntry->SetTargetURL( rTargetURL );
    pEntry->SetHierarchyURL( aLinkURL );
    if ( nPos < maEntries.size() ) {
        auto it = maEntries.begin();
        std::advance( it, nPos );
        maEntries.insert( it, std::move(pEntry) );
    }
    else
        maEntries.push_back( std::move(pEntry) );
}

size_t RegionData_Impl::GetCount() const
{
    return maEntries.size();
}

const OUString& RegionData_Impl::GetHierarchyURL(const INetURLObject& rRootURL)
{
    if ( maOwnURL.isEmpty() )
    {
        INetURLObject aRegionObj(rRootURL);

        aRegionObj.insertName( GetTitle(), false,
                     INetURLObject::LAST_SEGMENT,
                     INetURLObject::EncodeMechanism::All );

        maOwnURL = aRegionObj.GetMainURL( INetURLObject::DecodeMechanism::NONE );
        DBG_ASSERT( !maOwnURL.isEmpty(), "GetHierarchyURL(): Could not create URL!" );
    }

    return maOwnURL;
}

DocTempl_EntryData_Impl* RegionData_Impl::GetEntry( std::u16string_view rName ) const
{
    bool    bFound = false;
    tools::Long        nPos = GetEntryPos( rName, bFound );

    if ( bFound )
        return maEntries[ nPos ].get();
    return nullptr;
}


DocTempl_EntryData_Impl* RegionData_Impl::GetEntry( size_t nIndex ) const
{
    if ( nIndex < maEntries.size() )
        return maEntries[ nIndex ].get();
    return nullptr;
}

void RegionData_Impl::DeleteEntry( size_t nIndex )
{
    if ( nIndex < maEntries.size() )
    {
        auto it = maEntries.begin();
        std::advance( it, nIndex );
        maEntries.erase( it );
    }
}

int RegionData_Impl::Compare( RegionData_Impl const * pCompare ) const
{
    return maTitle.compareTo( pCompare->maTitle );
}

SfxDocTemplate_Impl::SfxDocTemplate_Impl()
    : maStandardGroup(DocTemplLocaleHelper::GetStandardGroupString())
    , mbConstructed(false)
    , mnLockCounter(0)
{
}

SfxDocTemplate_Impl::~SfxDocTemplate_Impl()
{
    gpTemplateData = nullptr;
}

void SfxDocTemplate_Impl::IncrementLock()
{
    std::unique_lock aGuard( maMutex );
    mnLockCounter++;
}

void SfxDocTemplate_Impl::DecrementLock()
{
    std::unique_lock aGuard( maMutex );
    if ( mnLockCounter )
        mnLockCounter--;
}

RegionData_Impl* SfxDocTemplate_Impl::GetRegion( size_t nIndex ) const
{
    if ( nIndex < maRegions.size() )
        return maRegions[ nIndex ].get();
    return nullptr;
}

RegionData_Impl* SfxDocTemplate_Impl::GetRegion( std::u16string_view rName )
    const
{
    for (auto& pData : maRegions)
    {
        if( pData->GetTitle() == rName )
            return pData.get();
    }
    return nullptr;
}


void SfxDocTemplate_Impl::DeleteRegion( size_t nIndex )
{
    if ( nIndex < maRegions.size() )
    {
        auto it = maRegions.begin();
        std::advance( it, nIndex );
        maRegions.erase( it );
    }
}


/*  AddRegion adds a Region to the RegionList
*/
void SfxDocTemplate_Impl::AddRegion( std::unique_lock<std::mutex>& /*rGuard*/,
                                     const OUString& rTitle,
                                     Content& rContent )
{
    auto pRegion = std::make_unique<RegionData_Impl>(rTitle);
    auto pRegionTmp = pRegion.get();

    if ( ! InsertRegion( std::move(pRegion), size_t(-1) ) )
    {
        return;
    }

    // now get the content of the region
    uno::Reference< XResultSet > xResultSet;

    try
    {
        xResultSet = rContent.createSortedCursor( { TITLE, TARGET_URL }, { { 1, true } }, m_rCompareFactory, INCLUDE_DOCUMENTS_ONLY );
    }
    catch ( Exception& ) {}

    if ( !xResultSet.is() )
        return;

    uno::Reference< XRow > xRow( xResultSet, UNO_QUERY );

    try
    {
        while ( xResultSet->next() )
        {
            pRegionTmp->AddEntry(INetURLObject(maRootURL), xRow->getString( 1 ), xRow->getString( 2 ), nullptr);
        }
    }
    catch ( Exception& ) {}
}

void SfxDocTemplate_Impl::CreateFromHierarchy( std::unique_lock<std::mutex>& rGuard, Content &rTemplRoot )
{
    uno::Reference< XResultSet > xResultSet;
    Sequence< OUString > aProps { TITLE };

    try
    {
        xResultSet = rTemplRoot.createSortedCursor(
                         aProps,
                         { // Sequence
                              { // NumberedSortingInfo
                                  /* ColumnIndex */ 1, /* Ascending */ true
                              }
                         },
                         m_rCompareFactory,
                         INCLUDE_FOLDERS_ONLY
                     );
    }
    catch ( Exception& ) {}

    if ( !xResultSet.is() )
        return;

    uno::Reference< XCommandEnvironment > aCmdEnv;
    uno::Reference< XContentAccess > xContentAccess( xResultSet, UNO_QUERY );
    uno::Reference< XRow > xRow( xResultSet, UNO_QUERY );

    try
    {
        while ( xResultSet->next() )
        {
            const OUString aId = xContentAccess->queryContentIdentifierString();
            Content  aContent( aId, aCmdEnv, comphelper::getProcessComponentContext() );

            AddRegion( rGuard, xRow->getString( 1 ), aContent );
        }
    }
    catch ( Exception& ) {}
}


bool SfxDocTemplate_Impl::Construct( )
{
    std::unique_lock aGuard( maMutex );

    if ( mbConstructed )
        return true;

    const uno::Reference< XComponentContext >& xContext = ::comphelper::getProcessComponentContext();

    mxInfo.set(document::DocumentProperties::create(xContext), UNO_QUERY);

    mxTemplates = frame::DocumentTemplates::create(xContext);

    uno::Reference< XLocalizable > xLocalizable( mxTemplates, UNO_QUERY );

    m_rCompareFactory = AnyCompareFactory::createWithLocale(xContext, xLocalizable->getLocale());

    uno::Reference < XContent > aRootContent = mxTemplates->getContent();
    uno::Reference < XCommandEnvironment > aCmdEnv;

    if ( ! aRootContent.is() )
        return false;

    mbConstructed = true;
    maRootURL = aRootContent->getIdentifier()->getContentIdentifier();

    Content aTemplRoot( aRootContent, aCmdEnv, xContext );
    CreateFromHierarchy( aGuard, aTemplRoot );

    return true;
}


void SfxDocTemplate_Impl::ReInitFromComponent()
{
    uno::Reference< XDocumentTemplates > xTemplates = getDocTemplates();
    if ( xTemplates.is() )
    {
        uno::Reference < XContent > aRootContent = xTemplates->getContent();
        uno::Reference < XCommandEnvironment > aCmdEnv;
        Content aTemplRoot( aRootContent, aCmdEnv, comphelper::getProcessComponentContext() );
        Clear();
        std::unique_lock aGuard(maMutex);
        CreateFromHierarchy( aGuard, aTemplRoot );
    }
}


bool SfxDocTemplate_Impl::InsertRegion( std::unique_ptr<RegionData_Impl> pNew, size_t nPos )
{
    // return false (not inserted) if the entry already exists
    for (auto const& pRegion : maRegions)
        if ( pRegion->Compare( pNew.get() ) == 0 )
            return false;

    size_t newPos = nPos;
    if ( pNew->GetTitle() == maStandardGroup )
        newPos = 0;

    if ( newPos < maRegions.size() )
    {
        auto it = maRegions.begin();
        std::advance( it, newPos );
        maRegions.emplace( it, std::move(pNew) );
    }
    else
        maRegions.emplace_back( std::move(pNew) );

    return true;
}


void SfxDocTemplate_Impl::Rescan()
{
    Clear();

    try
    {
        uno::Reference< XDocumentTemplates > xTemplates = getDocTemplates();
        DBG_ASSERT( xTemplates.is(), "SfxDocTemplate_Impl::Rescan:invalid template instance!" );
        if ( xTemplates.is() )
        {
            xTemplates->update();

            uno::Reference < XContent > aRootContent = xTemplates->getContent();
            uno::Reference < XCommandEnvironment > aCmdEnv;

            Content aTemplRoot( aRootContent, aCmdEnv, comphelper::getProcessComponentContext() );
            std::unique_lock aGuard(maMutex);
            CreateFromHierarchy( aGuard, aTemplRoot );
        }
    }
    catch( const Exception& )
    {
        TOOLS_WARN_EXCEPTION( "sfx.doc", "SfxDocTemplate_Impl::Rescan: caught an exception while doing the update" );
    }
}


bool SfxDocTemplate_Impl::GetTitleFromURL( const OUString& rURL,
                                           OUString& aTitle )
{
    if ( mxInfo.is() )
    {
        try
        {
            mxInfo->read( rURL );
        }
        catch ( Exception& )
        {
            // the document is not a StarOffice document
            return false;
        }


        try
        {
            uno::Reference< XPropertySet > aPropSet( mxInfo, UNO_QUERY );
            if ( aPropSet.is() )
            {
                Any aValue = aPropSet->getPropertyValue( TITLE );
                aValue >>= aTitle;
            }
        }
        catch ( IOException& ) {}
        catch ( UnknownPropertyException& ) {}
        catch ( Exception& ) {}
    }

    if ( aTitle.isEmpty() )
    {
        INetURLObject aURL( rURL );
        aURL.CutExtension();
        aTitle = aURL.getName( INetURLObject::LAST_SEGMENT, true,
                               INetURLObject::DecodeMechanism::WithCharset );
    }

    return true;
}


void SfxDocTemplate_Impl::Clear()
{
    std::unique_lock aGuard( maMutex );
    if ( mnLockCounter )
        return;
    maRegions.clear();
}


bool getTextProperty_Impl( Content& rContent,
                               const OUString& rPropName,
                               OUString& rPropValue )
{
    bool bGotProperty = false;

    // Get the property
    try
    {
        uno::Reference< XPropertySetInfo > aPropInfo = rContent.getProperties();

        // check, whether or not the property exists
        if ( !aPropInfo.is() || !aPropInfo->hasPropertyByName( rPropName ) )
        {
            return false;
        }

        // now get the property
        Any aAnyValue = rContent.getPropertyValue( rPropName );
        aAnyValue >>= rPropValue;

        if ( SfxURLRelocator_Impl::propertyCanContainOfficeDir( rPropName ) )
        {
            SfxURLRelocator_Impl aRelocImpl( ::comphelper::getProcessComponentContext() );
            aRelocImpl.makeAbsoluteURL( rPropValue );
        }

        bGotProperty = true;
    }
    catch ( RuntimeException& ) {}
    catch ( Exception& ) {}

    return bGotProperty;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
