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

#include <hintids.hxx>

#include <osl/diagnose.h>
#include <sal/log.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <i18nlangtag/mslangid.hxx>
#include <svtools/ctrltool.hxx>
#include <unotools/configmgr.hxx>
#include <unotools/lingucfg.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/sfxmodelfactory.hxx>
#include <sfx2/printer.hxx>
#include <svl/asiancfg.hxx>
#include <svl/intitem.hxx>
#include <editeng/adjustitem.hxx>
#include <editeng/autokernitem.hxx>
#include <com/sun/star/document/UpdateDocMode.hpp>
#include <com/sun/star/i18n/ScriptType.hpp>
#include <svx/compatflags.hxx>
#include <svx/svxids.hrc>
#include <editeng/fhgtitem.hxx>
#include <editeng/fontitem.hxx>
#include <editeng/flstitem.hxx>
#include <editeng/tstpitem.hxx>
#include <editeng/langitem.hxx>
#include <editeng/colritem.hxx>
#include <editeng/orphitem.hxx>
#include <editeng/widwitem.hxx>
#include <editeng/hyphenzoneitem.hxx>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>
#include <prtopt.hxx>
#include <fmtcol.hxx>
#include <docsh.hxx>
#include <wdocsh.hxx>
#include <swmodule.hxx>
#include <doc.hxx>
#include <IDocumentSettingAccess.hxx>
#include <IDocumentDeviceAccess.hxx>
#include <IDocumentDrawModelAccess.hxx>
#include <IDocumentStylePoolAccess.hxx>
#include <IDocumentChartDataProviderAccess.hxx>
#include <IDocumentState.hxx>
#include <docfac.hxx>
#include <docstyle.hxx>
#include <shellio.hxx>
#include <swdtflvr.hxx>
#include <usrpref.hxx>
#include <fontcfg.hxx>
#include <poolfmt.hxx>
#include <globdoc.hxx>
#include <unotxdoc.hxx>
#include <linkenum.hxx>
#include <swwait.hxx>
#include <swerror.h>
#include <unochart.hxx>
#include <drawdoc.hxx>

#include <svx/CommonStyleManager.hxx>

#include <memory>

#include <officecfg/Office/Common.hxx>

using namespace ::com::sun::star::i18n;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star;

// Load Document
bool SwDocShell::InitNew( const uno::Reference < embed::XStorage >& xStor )
{
    bool bRet = SfxObjectShell::InitNew( xStor );
    OSL_ENSURE( GetMapUnit() == MapUnit::MapTwip, "map unit is not twip!" );
    bool bHTMLTemplSet = false;
    if( bRet )
    {
        AddLink();      // create m_xDoc / pIo if applicable

        bool bWeb = dynamic_cast< const SwWebDocShell *>( this ) !=  nullptr;
        if ( bWeb )
            bHTMLTemplSet = SetHTMLTemplate( *GetDoc() );// Styles from HTML.vor
        else if( dynamic_cast< const SwGlobalDocShell *>( this ) !=  nullptr )
            GetDoc()->getIDocumentSettingAccess().set(DocumentSettingId::GLOBAL_DOCUMENT, true);       // Globaldokument

        if ( GetCreateMode() ==  SfxObjectCreateMode::EMBEDDED )
            SwTransferable::InitOle( this );

        SwModule* mod = SwModule::get();
        // set forbidden characters if necessary
        const bool bFuzzing = comphelper::IsFuzzing();
        if (!bFuzzing)
        {
            const Sequence<lang::Locale> aLocales = SvxAsianConfig::GetStartEndCharLocales();
            for(const lang::Locale& rLocale : aLocales)
            {
                ForbiddenCharacters aForbidden;
                SvxAsianConfig::GetStartEndChars( rLocale, aForbidden.beginLine, aForbidden.endLine);
                LanguageType  eLang = LanguageTag::convertToLanguageType(rLocale);
                m_xDoc->getIDocumentSettingAccess().setForbiddenCharacters( eLang, aForbidden);
            }
            m_xDoc->getIDocumentSettingAccess().set(DocumentSettingId::KERN_ASIAN_PUNCTUATION,
                  !SvxAsianConfig::IsKerningWesternTextOnly());
            m_xDoc->getIDocumentSettingAccess().setCharacterCompressionType(SvxAsianConfig::GetCharDistanceCompression());
            m_xDoc->getIDocumentDeviceAccess().setPrintData(*mod->GetPrtOptions(bWeb));
        }

        SubInitNew();

        // for all

        SwStdFontConfig* pStdFont = mod->GetStdFontConfig();
        SfxPrinter* pPrt = m_xDoc->getIDocumentDeviceAccess().getPrinter( false );

        OUString sEntry;
        static const sal_uInt16 aFontWhich[] =
        {   RES_CHRATR_FONT,
            RES_CHRATR_CJK_FONT,
            RES_CHRATR_CTL_FONT
        };
        static const sal_uInt16 aFontHeightWhich[] =
        {
            RES_CHRATR_FONTSIZE,
            RES_CHRATR_CJK_FONTSIZE,
            RES_CHRATR_CTL_FONTSIZE
        };
        static const sal_uInt16 aFontIds[] =
        {
            FONT_STANDARD,
            FONT_STANDARD_CJK,
            FONT_STANDARD_CTL
        };
        static const DefaultFontType nFontTypes[] =
        {
            DefaultFontType::LATIN_TEXT,
            DefaultFontType::CJK_TEXT,
            DefaultFontType::CTL_TEXT
        };
        static const sal_uInt16 aLangTypes[] =
        {
            RES_CHRATR_LANGUAGE,
            RES_CHRATR_CJK_LANGUAGE,
            RES_CHRATR_CTL_LANGUAGE
        };

        for(sal_uInt8 i = 0; i < 3; i++)
        {
            sal_uInt16 nFontWhich = aFontWhich[i];
            sal_uInt16 nFontId = aFontIds[i];
            std::unique_ptr<SvxFontItem> pFontItem;
            const SvxLanguageItem& rLang = static_cast<const SvxLanguageItem&>(m_xDoc->GetDefault( aLangTypes[i] ));
            LanguageType eLanguage = rLang.GetLanguage();
            if(!pStdFont->IsFontDefault(nFontId))
            {
                sEntry = pStdFont->GetFontFor(nFontId);

                vcl::Font aFont( sEntry, Size( 0, 10 ) );
                if( pPrt )
                {
                    aFont = pPrt->GetFontMetric( aFont );
                }

                pFontItem.reset(new SvxFontItem(aFont.GetFamilyTypeMaybeAskConfig(), aFont.GetFamilyName(),
                                                OUString(), aFont.GetPitchMaybeAskConfig(), aFont.GetCharSet(), nFontWhich));
            }
            else
            {
                // #107782# OJ use korean language if latin was used
                if ( i == 0 )
                {
                    LanguageType eUiLanguage = Application::GetSettings().GetUILanguageTag().getLanguageType();
                    if (MsLangId::isKorean(eUiLanguage))
                        eLanguage = eUiLanguage;
                }

                vcl::Font aLangDefFont = OutputDevice::GetDefaultFont(
                    nFontTypes[i],
                    eLanguage,
                    GetDefaultFontFlags::OnlyOne );
                pFontItem.reset(new SvxFontItem(aLangDefFont.GetFamilyTypeMaybeAskConfig(), aLangDefFont.GetFamilyName(),
                                                OUString(), aLangDefFont.GetPitchMaybeAskConfig(), aLangDefFont.GetCharSet(), nFontWhich));
            }
            m_xDoc->SetDefault(*pFontItem);
            if( !bHTMLTemplSet )
            {
                SwTextFormatColl *pColl = m_xDoc->getIDocumentStylePoolAccess().GetTextCollFromPool(RES_POOLCOLL_STANDARD);
                pColl->ResetFormatAttr(nFontWhich);
            }
            pFontItem.reset();
            sal_Int32 nFontHeight = pStdFont->GetFontHeight( FONT_STANDARD, i, eLanguage );
            if(nFontHeight <= 0)
                nFontHeight = SwStdFontConfig::GetDefaultHeightFor( nFontId, eLanguage );
            m_xDoc->SetDefault(SvxFontHeightItem( nFontHeight, 100, aFontHeightWhich[i] ));
            if( !bHTMLTemplSet )
            {
                SwTextFormatColl *pColl = m_xDoc->getIDocumentStylePoolAccess().GetTextCollFromPool(RES_POOLCOLL_STANDARD);
                pColl->ResetFormatAttr(aFontHeightWhich[i]);
            }

        }
        sal_uInt16 aFontIdPoolId[] =
        {
            FONT_OUTLINE,       RES_POOLCOLL_HEADLINE_BASE,
            FONT_LIST,          RES_POOLCOLL_NUMBER_BULLET_BASE,
            FONT_CAPTION,       RES_POOLCOLL_LABEL,
            FONT_INDEX,         RES_POOLCOLL_REGISTER_BASE,
            FONT_OUTLINE_CJK,   RES_POOLCOLL_HEADLINE_BASE,
            FONT_LIST_CJK,      RES_POOLCOLL_NUMBER_BULLET_BASE,
            FONT_CAPTION_CJK,   RES_POOLCOLL_LABEL,
            FONT_INDEX_CJK,     RES_POOLCOLL_REGISTER_BASE,
            FONT_OUTLINE_CTL,   RES_POOLCOLL_HEADLINE_BASE,
            FONT_LIST_CTL,      RES_POOLCOLL_NUMBER_BULLET_BASE,
            FONT_CAPTION_CTL,   RES_POOLCOLL_LABEL,
            FONT_INDEX_CTL,     RES_POOLCOLL_REGISTER_BASE
        };

        TypedWhichId<SvxFontItem> nFontWhich = RES_CHRATR_FONT;
        TypedWhichId<SvxFontHeightItem> nFontHeightWhich = RES_CHRATR_FONTSIZE;
        LanguageType eLanguage = m_xDoc->GetDefault( RES_CHRATR_LANGUAGE ).GetLanguage();
        bool bDisableBuiltinStyles = !bFuzzing && officecfg::Office::Common::Load::DisableBuiltinStyles::get();
        sal_uInt8 nLimit = bDisableBuiltinStyles ? 0 : 24;
        for(sal_uInt8 nIdx = 0; nIdx < nLimit; nIdx += 2)
        {
            if(nIdx == 8)
            {
                nFontWhich = RES_CHRATR_CJK_FONT;
                nFontHeightWhich = RES_CHRATR_CJK_FONTSIZE;
                eLanguage = m_xDoc->GetDefault( RES_CHRATR_CJK_LANGUAGE ).GetLanguage();
            }
            else if(nIdx == 16)
            {
                nFontWhich = RES_CHRATR_CTL_FONT;
                nFontHeightWhich = RES_CHRATR_CTL_FONTSIZE;
                eLanguage = m_xDoc->GetDefault( RES_CHRATR_CTL_LANGUAGE ).GetLanguage();
            }
            SwTextFormatColl *pColl = nullptr;
            if(!pStdFont->IsFontDefault(aFontIdPoolId[nIdx]))
            {
                sEntry = pStdFont->GetFontFor(aFontIdPoolId[nIdx]);

                vcl::Font aFont( sEntry, Size( 0, 10 ) );
                if( pPrt )
                    aFont = pPrt->GetFontMetric( aFont );

                pColl = m_xDoc->getIDocumentStylePoolAccess().GetTextCollFromPool(aFontIdPoolId[nIdx + 1]);
                assert(pColl);
                if( !bHTMLTemplSet ||
                    SfxItemState::SET != pColl->GetAttrSet().GetItemState(
                                                    nFontWhich, false ) )
                {
                    pColl->SetFormatAttr(SvxFontItem(aFont.GetFamilyTypeMaybeAskConfig(), aFont.GetFamilyName(),
                                                  OUString(), aFont.GetPitchMaybeAskConfig(), aFont.GetCharSet(), nFontWhich));
                }
            }
            sal_Int32 nFontHeight = pStdFont->GetFontHeight( static_cast< sal_Int8 >(aFontIdPoolId[nIdx]), 0, eLanguage );
            if(nFontHeight <= 0)
                nFontHeight = SwStdFontConfig::GetDefaultHeightFor( aFontIdPoolId[nIdx], eLanguage );
            if(!pColl)
                pColl = m_xDoc->getIDocumentStylePoolAccess().GetTextCollFromPool(aFontIdPoolId[nIdx + 1]);
            SvxFontHeightItem aFontHeight( pColl->GetFormatAttr( nFontHeightWhich ) );
            if(aFontHeight.GetHeight() != sal::static_int_cast<sal_uInt32, sal_Int32>(nFontHeight))
            {
                aFontHeight.SetHeight(nFontHeight);
                pColl->SetFormatAttr( aFontHeight );
            }
        }

        // the default for documents created via 'File/New' should be 'on'
        // (old documents, where this property was not yet implemented, will get the
        // value 'false' in the SwDoc c-tor)
        m_xDoc->getIDocumentSettingAccess().set( DocumentSettingId::MATH_BASELINE_ALIGNMENT,
                mod->GetUsrPref( bWeb )->IsAlignMathObjectsToBaseline() );
        m_xDoc->getIDocumentSettingAccess().set( DocumentSettingId::FOOTNOTE_IN_COLUMN_TO_PAGEEND, true);
    }

    /* #106748# If the default frame direction of a document is RTL
        the default adjustment is to the right. */
    if( !bHTMLTemplSet &&
        SvxFrameDirection::Horizontal_RL_TB == GetDefaultFrameDirection(GetAppLanguage()) )
    {
        m_xDoc->SetDefault( SvxAdjustItem(SvxAdjust::Right, RES_PARATR_ADJUST ) );
    }

// #i29550#
    m_xDoc->SetDefault( SfxBoolItem( RES_COLLAPSING_BORDERS, true ) );
// <-- collapsing

    //#i16874# AutoKerning as default for new documents
    m_xDoc->SetDefault( SvxAutoKernItem( true, RES_CHRATR_AUTOKERN ) );

    // #i42080# - Due to the several calls of method <SetDefault(..)>
    // at the document instance, the document is modified. Thus, reset this
    // status here. Note: In method <SubInitNew()> this is also done.
    m_xDoc->getIDocumentState().ResetModified();

    return bRet;
}

// Ctor with SfxCreateMode ?????
SwDocShell::SwDocShell( SfxObjectCreateMode const eMode )
    : SfxObjectShell(eMode)
    , m_IsInUpdateFontList(false)
    , m_pStyleManager(new svx::CommonStyleManager(*this))
    , m_pView(nullptr)
    , m_pWrtShell(nullptr)
    , m_nUpdateDocMode(document::UpdateDocMode::ACCORDING_TO_CONFIG)
    , m_IsATemplate(false)
    , m_IsRemovedInvisibleContent(false)
{
    Init_Impl();
}

// Ctor / Dtor
SwDocShell::SwDocShell( const SfxModelFlags i_nSfxCreationFlags )
    : SfxObjectShell ( i_nSfxCreationFlags )
    , m_IsInUpdateFontList(false)
    , m_pStyleManager(new svx::CommonStyleManager(*this))
    , m_pView(nullptr)
    , m_pWrtShell(nullptr)
    , m_nUpdateDocMode(document::UpdateDocMode::ACCORDING_TO_CONFIG)
    , m_IsATemplate(false)
    , m_IsRemovedInvisibleContent(false)
{
    Init_Impl();
}

// Ctor / Dtor
SwDocShell::SwDocShell( SwDoc& rD, SfxObjectCreateMode const eMode )
    : SfxObjectShell(eMode)
    , m_xDoc(&rD)
    , m_IsInUpdateFontList(false)
    , m_pStyleManager(new svx::CommonStyleManager(*this))
    , m_pView(nullptr)
    , m_pWrtShell(nullptr)
    , m_nUpdateDocMode(document::UpdateDocMode::ACCORDING_TO_CONFIG)
    , m_IsATemplate(false)
    , m_IsRemovedInvisibleContent(false)
{
    Init_Impl();
}

// Dtor
SwDocShell::~SwDocShell()
{
    // disable chart related objects now because in ~SwDoc it may be too late for this
    if (m_xDoc)
    {
        m_xDoc->getIDocumentChartDataProviderAccess().GetChartControllerHelper().Disconnect();
        SwChartDataProvider *pPCD = m_xDoc->getIDocumentChartDataProviderAccess().GetChartDataProvider();
        if (pPCD)
            pPCD->dispose();
    }

    RemoveLink();
    m_pFontList.reset();

    // we, as BroadCaster also become our own Listener
    // (for DocInfo/FileNames/...)
    EndListening( *this );

    m_pOLEChildList.reset();
}

void  SwDocShell::Init_Impl()
{
    SetPool(&SwModule::get()->GetPool());
    SetBaseModel(new SwXTextDocument(this));
    // we, as BroadCaster also become our own Listener
    // (for DocInfo/FileNames/...)
    StartListening( *this );
    //position of the "Automatic" style filter for the stylist (app.src)
    SetAutoStyleFilterIndex(3);

    // set map unit to twip
    SetMapUnit( MapUnit::MapTwip );
}

void SwDocShell::AddLink()
{
    if (!m_xDoc)
    {
        SwDocFac aFactory;
        m_xDoc = &aFactory.GetDoc();
        m_xDoc->getIDocumentSettingAccess().set(DocumentSettingId::HTML_MODE, dynamic_cast< const SwWebDocShell *>( this ) !=  nullptr );
    }
    m_xDoc->SetDocShell( this );      // set the DocShell-Pointer for Doc
    rtl::Reference< SwXTextDocument > xDoc(GetBaseModel());
    xDoc->Reactivate(this);

    SetPool(&m_xDoc->GetAttrPool());

    // most suitably not until a sdbcx::View is created!!!
    m_xDoc->SetOle2Link(LINK(this, SwDocShell, Ole2ModifiedHdl));
}

// create new FontList Change Printer
void SwDocShell::UpdateFontList()
{
    if (!m_IsInUpdateFontList)
    {
        m_IsInUpdateFontList = true;
        OSL_ENSURE(m_xDoc, "No Doc no FontList");
        if (m_xDoc)
        {
            m_pFontList.reset( new FontList( m_xDoc->getIDocumentDeviceAccess().getReferenceDevice(true) ) );
            PutItem( SvxFontListItem( m_pFontList.get(), SID_ATTR_CHAR_FONTLIST ) );
        }
        m_IsInUpdateFontList = false;
    }
}

void SwDocShell::RemoveLink()
{
    // disconnect Uno-Object
    rtl::Reference< SwXTextDocument > xDoc(GetBaseModel());
    xDoc->Invalidate();
    if (m_xDoc)
    {
        if (m_xBasePool.is())
        {
            m_xBasePool->dispose();
            m_xBasePool.clear();
        }
        m_xDoc->SetOle2Link(Link<bool,void>());
        m_xDoc->SetDocShell( nullptr );
        m_xDoc.clear();       // we don't have the Doc anymore!!
    }
}
void SwDocShell::InvalidateModel()
{
    // disconnect Uno-Object
    rtl::Reference< SwXTextDocument > xDoc(GetBaseModel());
    xDoc->Invalidate();
}
void SwDocShell::ReactivateModel()
{
    // disconnect Uno-Object
    rtl::Reference< SwXTextDocument > xDoc(GetBaseModel());
    xDoc->Reactivate(this);
}

// Load, Default-Format
bool  SwDocShell::Load( SfxMedium& rMedium )
{
    bool bRet = false;

    if (SfxObjectShell::Load(rMedium))
    {
        comphelper::EmbeddedObjectContainer& rEmbeddedObjectContainer = getEmbeddedObjectContainer();
        rEmbeddedObjectContainer.setUserAllowsLinkUpdate(false);

        SAL_INFO( "sw.ui", "after SfxInPlaceObject::Load" );
        if (m_xDoc) // for last version!!
            RemoveLink();       // release the existing

        AddLink();      // set Link and update Data!!

        // Define some settings for legacy ODF files that have different default values now
        // (if required, they will be overridden later when settings will be read)
        if (IsOwnStorageFormat(rMedium))
        {
            SwDrawModel* pDrawModel = m_xDoc->getIDocumentDrawModelAccess().GetDrawModel();
            if (pDrawModel)
            {
                pDrawModel->SetCompatibilityFlag(SdrCompatibilityFlag::AnchoredTextOverflowLegacy,
                                                 true); // legacy processing for tdf#99729
                pDrawModel->SetCompatibilityFlag(SdrCompatibilityFlag::LegacyFontwork,
                                                 true); // legacy processing for tdf#148000
            }
        }

        // Loading
        // for MD
        OSL_ENSURE( !m_xBasePool.is(), "who hasn't destroyed their Pool?" );
        m_xBasePool = new SwDocStyleSheetPool( *m_xDoc, SfxObjectCreateMode::ORGANIZER == GetCreateMode() );
        if(GetCreateMode() != SfxObjectCreateMode::ORGANIZER)
        {
            const SfxUInt16Item* pUpdateDocItem = rMedium.GetItemSet().GetItem(SID_UPDATEDOCMODE, false);
            m_nUpdateDocMode = pUpdateDocItem ? pUpdateDocItem->GetValue() : document::UpdateDocMode::NO_UPDATE;
        }

        SwModule* mod = SwModule::get();
        SwWait aWait( *this, true );
        ErrCodeMsg nErr = ERR_SWG_READ_ERROR;
        switch( GetCreateMode() )
        {
            case SfxObjectCreateMode::ORGANIZER:
                {
                    if( ReadXML )
                    {
                        ReadXML->SetOrganizerMode( true );
                        SwReader aRdr(rMedium, OUString(), m_xDoc.get());
                        nErr = aRdr.Read( *ReadXML );
                        ReadXML->SetOrganizerMode( false );
                    }
                }
                break;

            case SfxObjectCreateMode::INTERNAL:
            case SfxObjectCreateMode::EMBEDDED:
                {
                    SwTransferable::InitOle( this );
                }
                // suppress SfxProgress, when we are Embedded
                mod->SetEmbeddedLoadSave( true );
                [[fallthrough]];

            case SfxObjectCreateMode::STANDARD:
                {
                    Reader *pReader = ReadXML;
                    if( pReader )
                    {
                        // set Doc's DocInfo at DocShell-Medium
                        SAL_INFO( "sw.ui", "before ReadDocInfo" );
                        SwReader aRdr(rMedium, OUString(), m_xDoc.get());
                        SAL_INFO( "sw.ui", "before Read" );
                        nErr = aRdr.Read( *pReader );
                        SAL_INFO( "sw.ui", "after Read" );
                        // If a XML document is loaded, the global doc/web doc
                        // flags have to be set, because they aren't loaded
                        // by this formats.
                        if( dynamic_cast< const SwWebDocShell *>( this ) !=  nullptr )
                        {
                            if (!m_xDoc->getIDocumentSettingAccess().get(DocumentSettingId::HTML_MODE))
                                m_xDoc->getIDocumentSettingAccess().set(DocumentSettingId::HTML_MODE, true);
                        }
                        if( dynamic_cast< const SwGlobalDocShell *>( this ) !=  nullptr )
                        {
                            if (!m_xDoc->getIDocumentSettingAccess().get(DocumentSettingId::GLOBAL_DOCUMENT))
                                m_xDoc->getIDocumentSettingAccess().set(DocumentSettingId::GLOBAL_DOCUMENT, true);
                        }
                    }
                }
                break;

            default:
                OSL_ENSURE( false, "Load: new CreateMode?" );
        }

        UpdateFontList();
        InitDrawModelAndDocShell(this, m_xDoc ? m_xDoc->getIDocumentDrawModelAccess().GetDrawModel()
                                              : nullptr);

        SetError(nErr);
        bRet = !nErr.IsError();

        if (bRet && !m_xDoc->IsInLoadAsynchron() &&
            GetCreateMode() == SfxObjectCreateMode::STANDARD)
        {
            LoadingFinished();
        }

        // suppress SfxProgress, when we are Embedded
        mod->SetEmbeddedLoadSave( false );
    }

    return bRet;
}

bool  SwDocShell::LoadFrom( SfxMedium& rMedium )
{
    bool bRet = false;
    if (m_xDoc)
        RemoveLink();

    AddLink();      // set Link and update Data!!

    do {        // middle check loop
        ErrCodeMsg nErr = ERR_SWG_READ_ERROR;
        OUString aStreamName = u"styles.xml"_ustr;
        uno::Reference < container::XNameAccess > xAccess = rMedium.GetStorage();
        if ( xAccess->hasByName( aStreamName ) && rMedium.GetStorage()->isStreamElement( aStreamName ) )
        {
            // Loading
            SwWait aWait( *this, true );
            {
                OSL_ENSURE( !m_xBasePool.is(), "who hasn't destroyed their Pool?" );
                m_xBasePool = new SwDocStyleSheetPool( *m_xDoc, SfxObjectCreateMode::ORGANIZER == GetCreateMode() );
                if( ReadXML )
                {
                    ReadXML->SetOrganizerMode( true );
                    SwReader aRdr(rMedium, OUString(), m_xDoc.get());
                    nErr = aRdr.Read( *ReadXML );
                    ReadXML->SetOrganizerMode( false );
                }
            }
        }
        else
        {
            OSL_FAIL("Code removed!");
        }

        SetError(nErr);
        bRet = !nErr.IsError();

    } while( false );

    SfxObjectShell::LoadFrom( rMedium );
    m_xDoc->getIDocumentState().ResetModified();
    return bRet;
}

void SwDocShell::SubInitNew()
{
    OSL_ENSURE( !m_xBasePool.is(), "who hasn't destroyed their Pool?" );
    m_xBasePool = new SwDocStyleSheetPool( *m_xDoc, SfxObjectCreateMode::ORGANIZER == GetCreateMode() );
    UpdateFontList();
    InitDrawModelAndDocShell(this, m_xDoc ? m_xDoc->getIDocumentDrawModelAccess().GetDrawModel() : nullptr);

    m_xDoc->getIDocumentSettingAccess().setLinkUpdateMode( GLOBALSETTING );
    m_xDoc->getIDocumentSettingAccess().setFieldUpdateFlags( AUTOUPD_GLOBALSETTING );

    bool bWeb = dynamic_cast< const SwWebDocShell *>( this ) !=  nullptr;

    static const WhichRangesContainer nRange1(svl::Items<
        RES_CHRATR_COLOR, RES_CHRATR_COLOR,
        RES_CHRATR_LANGUAGE, RES_CHRATR_LANGUAGE,
        RES_CHRATR_CJK_LANGUAGE, RES_CHRATR_CJK_LANGUAGE,
        RES_CHRATR_CTL_LANGUAGE, RES_CHRATR_CTL_LANGUAGE,
        RES_PARATR_ADJUST, RES_PARATR_ADJUST
        >);
    static const WhichRangesContainer nRange2(svl::Items<
        RES_CHRATR_COLOR, RES_CHRATR_COLOR,
        RES_CHRATR_LANGUAGE, RES_CHRATR_LANGUAGE,
        RES_CHRATR_CJK_LANGUAGE, RES_CHRATR_CJK_LANGUAGE,
        RES_CHRATR_CTL_LANGUAGE, RES_CHRATR_CTL_LANGUAGE,
        RES_PARATR_ADJUST, RES_PARATR_ADJUST,
        RES_PARATR_TABSTOP, RES_PARATR_HYPHENZONE
        >);
    SfxItemSet aDfltSet( m_xDoc->GetAttrPool(), bWeb ? nRange1 : nRange2 );

    //! get lingu options without loading lingu DLL
    SvtLinguOptions aLinguOpt;

    const bool bFuzzing = comphelper::IsFuzzing();
    if (!bFuzzing)
        SvtLinguConfig().GetOptions(aLinguOpt);

    LanguageType nVal = MsLangId::resolveSystemLanguageByScriptType(aLinguOpt.nDefaultLanguage, css::i18n::ScriptType::LATIN),
                 eCJK = MsLangId::resolveSystemLanguageByScriptType(aLinguOpt.nDefaultLanguage_CJK, css::i18n::ScriptType::ASIAN),
                 eCTL = MsLangId::resolveSystemLanguageByScriptType(aLinguOpt.nDefaultLanguage_CTL, css::i18n::ScriptType::COMPLEX);
    aDfltSet.Put( SvxLanguageItem( nVal, RES_CHRATR_LANGUAGE ) );
    aDfltSet.Put( SvxLanguageItem( eCJK, RES_CHRATR_CJK_LANGUAGE ) );
    aDfltSet.Put( SvxLanguageItem( eCTL, RES_CHRATR_CTL_LANGUAGE ) );

    if(!bWeb)
    {
        SvxHyphenZoneItem aHyp( m_xDoc->GetDefault(RES_PARATR_HYPHENZONE)  );
        aHyp.GetMinLead()   = static_cast< sal_uInt8 >(aLinguOpt.nHyphMinLeading);
        aHyp.GetMinTrail()  = static_cast< sal_uInt8 >(aLinguOpt.nHyphMinTrailing);
        aHyp.GetMinWordLength()  = static_cast< sal_uInt8 >(aLinguOpt.nHyphMinWordLength);

        aDfltSet.Put( aHyp );

        sal_uInt16 nNewPos = o3tl::toTwips(SwModule::get()->GetUsrPref(false)->GetDefTabInMm100(), o3tl::Length::mm100);
        if( nNewPos )
            aDfltSet.Put( SvxTabStopItem( 1, nNewPos,
                                          SvxTabAdjust::Default, RES_PARATR_TABSTOP ) );
    }
    aDfltSet.Put( SvxColorItem( COL_AUTO, RES_CHRATR_COLOR ) );

    m_xDoc->SetDefault( aDfltSet );

    //default page mode for text grid
    if(!bWeb)
    {
        bool bSquaredPageMode = SwModule::get()->GetUsrPref(false)->IsSquaredPageMode();
        m_xDoc->SetDefaultPageMode( bSquaredPageMode );

        // only set Widow/Orphan defaults on a new, non-web document - not an opened one
        if (GetMedium() && GetMedium()->GetOrigURL().isEmpty() && !bFuzzing)
        {
            m_xDoc->SetDefault( SvxWidowsItem(  sal_uInt8(2), RES_PARATR_WIDOWS)  );
            m_xDoc->SetDefault( SvxOrphansItem( sal_uInt8(2), RES_PARATR_ORPHANS) );
        }
    }

    m_xDoc->getIDocumentState().ResetModified();
}

/*
 * Document Interface Access
 */
IDocumentDeviceAccess& SwDocShell::getIDocumentDeviceAccess()
{
    return m_xDoc->getIDocumentDeviceAccess();
}

IDocumentChartDataProviderAccess& SwDocShell::getIDocumentChartDataProviderAccess()
{
     return m_xDoc->getIDocumentChartDataProviderAccess();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
