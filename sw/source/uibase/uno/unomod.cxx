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

#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/view/DocumentZoomType.hpp>
#include <comphelper/ChainablePropertySetInfo.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <o3tl/any.hxx>
#include <osl/diagnose.h>
#include <svl/itemprop.hxx>
#include <tools/urlobj.hxx>
#include <tools/UnitConversion.hxx>
#include <vcl/svapp.hxx>

#include <unomod.hxx>
#include <usrpref.hxx>
#include <prtopt.hxx>
#include <swmodule.hxx>
#include <view.hxx>
#include <docsh.hxx>
#include <wrtsh.hxx>
#include <viewopt.hxx>
#include <doc.hxx>
#include <IDocumentDeviceAccess.hxx>
#include <edtwin.hxx>
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::lang;
using namespace ::comphelper;

namespace {

enum SwViewSettingsPropertyHandles
{
    HANDLE_VIEWSET_ANNOTATIONS,
    HANDLE_VIEWSET_BREAKS,
    HANDLE_VIEWSET_DRAWINGS,
    HANDLE_VIEWSET_FIELD_COMMANDS,
    HANDLE_VIEWSET_FOOTNOTE_BACKGROUND,
    HANDLE_VIEWSET_GRAPHICS,
    HANDLE_VIEWSET_HIDDEN_CHARACTERS,
    HANDLE_VIEWSET_HIDDEN_PARAGRAPHS,
    HANDLE_VIEWSET_HIDDEN_TEXT,
    HANDLE_VIEWSET_HRULER,
    HANDLE_VIEWSET_HSCROLL,
    HANDLE_VIEWSET_INDEX_MARK_BACKGROUND,
    HANDLE_VIEWSET_NONPRINTING_CHARACTERS,
    HANDLE_VIEWSET_ONLINE_LAYOUT,
    HANDLE_VIEWSET_PARA_BREAKS,
    HANDLE_VIEWSET_PROTECTED_SPACES,
    HANDLE_VIEWSET_SOFT_HYPHENS,
    HANDLE_VIEWSET_SPACES,
    HANDLE_VIEWSET_TABLE_BOUNDARIES,
    HANDLE_VIEWSET_TABLES,
    HANDLE_VIEWSET_TABSTOPS,
    HANDLE_VIEWSET_TEXT_BOUNDARIES,
    HANDLE_VIEWSET_TEXT_FIELD_BACKGROUND,
    HANDLE_VIEWSET_VRULER,
    HANDLE_VIEWSET_VSCROLL,
    HANDLE_VIEWSET_SMOOTH_SCROLLING,
    HANDLE_VIEWSET_ZOOM_TYPE,
    HANDLE_VIEWSET_ZOOM,
    HANDLE_VIEWSET_SHOW_CONTENT_TIPS,
    HANDLE_VIEWSET_HELP_URL,
    HANDLE_VIEWSET_VRULER_RIGHT,
    HANDLE_VIEWSET_SHOW_RULER,
    HANDLE_VIEWSET_IS_RASTER_VISIBLE,
    HANDLE_VIEWSET_IS_SNAP_TO_RASTER,
    HANDLE_VIEWSET_RASTER_RESOLUTION_X,
    HANDLE_VIEWSET_RASTER_RESOLUTION_Y,
    HANDLE_VIEWSET_RASTER_SUBDIVISION_X,
    HANDLE_VIEWSET_RASTER_SUBDIVISION_Y,
    HANDLE_VIEWSET_HORI_RULER_METRIC,
    HANDLE_VIEWSET_VERT_RULER_METRIC,
    HANDLE_VIEWSET_SCROLLBAR_TIPS,
    HANDLE_VIEWSET_INLINECHANGES_TIPS,
    HANDLE_VIEWSET_HIDE_WHITESPACE,
    HANDLE_VIEWSET_USE_HEADERFOOTERMENU,
    HANDLE_VIEWSET_BOOKMARKS,
    HANDLE_VIEWSET_SHOW_OUTLINECONTENTVISIBILITYBUTTON,
    HANDLE_VIEWSET_TREAT_SUB_OUTLINE_LEVELS_AS_CONTENT,
    HANDLE_VIEWSET_CHANGES_IN_MARGIN,
    HANDLE_VIEWSET_TEXTBOUNDARIES
};

enum SwPrintSettingsPropertyHandles
{
    HANDLE_PRINTSET_ANNOTATION_MODE,
    HANDLE_PRINTSET_BLACK_FONTS,
    HANDLE_PRINTSET_CONTROLS,
    HANDLE_PRINTSET_DRAWINGS,
    HANDLE_PRINTSET_GRAPHICS,
    HANDLE_PRINTSET_LEFT_PAGES,
    HANDLE_PRINTSET_PAGE_BACKGROUND,
    HANDLE_PRINTSET_PROSPECT,
    HANDLE_PRINTSET_REVERSED,
    HANDLE_PRINTSET_RIGHT_PAGES,
    HANDLE_PRINTSET_FAX_NAME,
    HANDLE_PRINTSET_PAPER_FROM_SETUP,
    HANDLE_PRINTSET_TABLES,
    HANDLE_PRINTSET_EMPTY_PAGES,
    HANDLE_PRINTSET_PROSPECT_RTL,
    HANDLE_PRINTSET_PLACEHOLDER,
    HANDLE_PRINTSET_HIDDEN_TEXT
};

}

static rtl::Reference<ChainablePropertySetInfo> lcl_createViewSettingsInfo()
{
    static PropertyInfo const aViewSettingsMap_Impl[] =
    {
        { u"HelpURL"_ustr,             HANDLE_VIEWSET_HELP_URL             , cppu::UnoType<OUString>::get(),    PROPERTY_NONE},
        { u"HorizontalRulerMetric"_ustr,HANDLE_VIEWSET_HORI_RULER_METRIC   , cppu::UnoType<sal_Int32>::get(), PROPERTY_NONE},
        { u"IsRasterVisible"_ustr,      HANDLE_VIEWSET_IS_RASTER_VISIBLE,       cppu::UnoType<bool>::get(),   PROPERTY_NONE},
        { u"IsSnapToRaster"_ustr,       HANDLE_VIEWSET_IS_SNAP_TO_RASTER,       cppu::UnoType<bool>::get(),   PROPERTY_NONE},
        { u"IsVertRulerRightAligned"_ustr,HANDLE_VIEWSET_VRULER_RIGHT         , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowContentTips"_ustr,     HANDLE_VIEWSET_SHOW_CONTENT_TIPS      , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowInlineTooltips"_ustr,  HANDLE_VIEWSET_INLINECHANGES_TIPS      , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"UseHeaderFooterMenu"_ustr, HANDLE_VIEWSET_USE_HEADERFOOTERMENU , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowOutlineContentVisibilityButton"_ustr, HANDLE_VIEWSET_SHOW_OUTLINECONTENTVISIBILITYBUTTON , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"TreatSubOutlineLevelsAsContent"_ustr, HANDLE_VIEWSET_TREAT_SUB_OUTLINE_LEVELS_AS_CONTENT , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowChangesInMargin"_ustr, HANDLE_VIEWSET_CHANGES_IN_MARGIN,       cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"RasterResolutionX"_ustr,    HANDLE_VIEWSET_RASTER_RESOLUTION_X,     cppu::UnoType<sal_Int32>::get(),     PROPERTY_NONE},
        { u"RasterResolutionY"_ustr,    HANDLE_VIEWSET_RASTER_RESOLUTION_Y,     cppu::UnoType<sal_Int32>::get(),     PROPERTY_NONE},
        { u"RasterSubdivisionX"_ustr,   HANDLE_VIEWSET_RASTER_SUBDIVISION_X,    cppu::UnoType<sal_Int32>::get(),     PROPERTY_NONE},
        { u"RasterSubdivisionY"_ustr,   HANDLE_VIEWSET_RASTER_SUBDIVISION_Y,    cppu::UnoType<sal_Int32>::get(),     PROPERTY_NONE},
        { u"ShowAnnotations"_ustr,     HANDLE_VIEWSET_ANNOTATIONS          , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowBookmarks"_ustr, HANDLE_VIEWSET_BOOKMARKS, cppu::UnoType<bool>::get(), PROPERTY_NONE },
        { u"ShowBreaks"_ustr,           HANDLE_VIEWSET_BREAKS               , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowDrawings"_ustr,         HANDLE_VIEWSET_DRAWINGS             , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowFieldCommands"_ustr,    HANDLE_VIEWSET_FIELD_COMMANDS       , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowFootnoteBackground"_ustr,HANDLE_VIEWSET_FOOTNOTE_BACKGROUND , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowGraphics"_ustr,         HANDLE_VIEWSET_GRAPHICS             , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowHiddenCharacters"_ustr, HANDLE_VIEWSET_HIDDEN_CHARACTERS    , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"HideWhitespace"_ustr,       HANDLE_VIEWSET_HIDE_WHITESPACE      , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowHiddenParagraphs"_ustr, HANDLE_VIEWSET_HIDDEN_PARAGRAPHS    , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowHiddenText"_ustr,       HANDLE_VIEWSET_HIDDEN_TEXT          , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowHoriRuler"_ustr,        HANDLE_VIEWSET_HRULER               , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowHoriScrollBar"_ustr,    HANDLE_VIEWSET_HSCROLL              , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowIndexMarkBackground"_ustr,HANDLE_VIEWSET_INDEX_MARK_BACKGROUND, cppu::UnoType<bool>::get(),PROPERTY_NONE},
        { u"ShowNonprintingCharacters"_ustr,HANDLE_VIEWSET_NONPRINTING_CHARACTERS, cppu::UnoType<bool>::get(),PROPERTY_NONE},
        { u"ShowOnlineLayout"_ustr,     HANDLE_VIEWSET_ONLINE_LAYOUT        , cppu::UnoType<bool>::get(), PropertyAttribute::MAYBEVOID},
        { u"ShowParaBreaks"_ustr,       HANDLE_VIEWSET_PARA_BREAKS          , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowProtectedSpaces"_ustr,  HANDLE_VIEWSET_PROTECTED_SPACES     , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowRulers"_ustr,           HANDLE_VIEWSET_SHOW_RULER           , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowScrollBarTips"_ustr,    HANDLE_VIEWSET_SCROLLBAR_TIPS       , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowSoftHyphens"_ustr,      HANDLE_VIEWSET_SOFT_HYPHENS         , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowSpaces"_ustr,           HANDLE_VIEWSET_SPACES               , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowTableBoundaries"_ustr,  HANDLE_VIEWSET_TABLE_BOUNDARIES     , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowTables"_ustr,           HANDLE_VIEWSET_TABLES               , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowTabstops"_ustr,         HANDLE_VIEWSET_TABSTOPS             , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowTextBoundaries"_ustr,   HANDLE_VIEWSET_TEXT_BOUNDARIES      , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowTextFieldBackground"_ustr,HANDLE_VIEWSET_TEXT_FIELD_BACKGROUND, cppu::UnoType<bool>::get(),PROPERTY_NONE},
        { u"ShowVertRuler"_ustr,        HANDLE_VIEWSET_VRULER               , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"ShowVertScrollBar"_ustr,    HANDLE_VIEWSET_VSCROLL              , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"SmoothScrolling"_ustr,      HANDLE_VIEWSET_SMOOTH_SCROLLING     , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"VerticalRulerMetric"_ustr,  HANDLE_VIEWSET_VERT_RULER_METRIC   , cppu::UnoType<sal_Int32>::get(), PROPERTY_NONE},
        { u"ZoomType"_ustr,             HANDLE_VIEWSET_ZOOM_TYPE            , cppu::UnoType<sal_Int16>::get(),   PROPERTY_NONE},
        { u"ZoomValue"_ustr,            HANDLE_VIEWSET_ZOOM                 , cppu::UnoType<sal_Int16>::get(),   PROPERTY_NONE},
        { u"TextBoundaries"_ustr,       HANDLE_VIEWSET_TEXTBOUNDARIES       , cppu::UnoType<sal_Int16>::get(),   PROPERTY_NONE},
        { OUString(), 0, css::uno::Type(), 0 }
    };
    return new ChainablePropertySetInfo ( aViewSettingsMap_Impl );
}

static rtl::Reference<ChainablePropertySetInfo> lcl_createPrintSettingsInfo()
{
    static PropertyInfo const aPrintSettingsMap_Impl[] =
    {
        { u"PrintAnnotationMode"_ustr, HANDLE_PRINTSET_ANNOTATION_MODE , cppu::UnoType<sal_Int16>::get(),   PROPERTY_NONE},
        { u"PrintBlackFonts"_ustr,      HANDLE_PRINTSET_BLACK_FONTS        , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"PrintControls"_ustr,        HANDLE_PRINTSET_CONTROLS           , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"PrintDrawings"_ustr,        HANDLE_PRINTSET_DRAWINGS           , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"PrintGraphics"_ustr,        HANDLE_PRINTSET_GRAPHICS           , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"PrintHiddenText"_ustr,       HANDLE_PRINTSET_HIDDEN_TEXT        , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"PrintLeftPages"_ustr,       HANDLE_PRINTSET_LEFT_PAGES         , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"PrintPageBackground"_ustr,  HANDLE_PRINTSET_PAGE_BACKGROUND    , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"PrintProspect"_ustr,        HANDLE_PRINTSET_PROSPECT           , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"PrintProspectRTL"_ustr,     HANDLE_PRINTSET_PROSPECT_RTL       , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"PrintReversed"_ustr,        HANDLE_PRINTSET_REVERSED           , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"PrintRightPages"_ustr,      HANDLE_PRINTSET_RIGHT_PAGES        , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"PrintFaxName"_ustr,         HANDLE_PRINTSET_FAX_NAME           , cppu::UnoType<OUString>::get(), PROPERTY_NONE},
        { u"PrintPaperFromSetup"_ustr,  HANDLE_PRINTSET_PAPER_FROM_SETUP   , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"PrintTables"_ustr,          HANDLE_PRINTSET_TABLES             , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"PrintTextPlaceholder"_ustr,  HANDLE_PRINTSET_PLACEHOLDER        , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { u"PrintEmptyPages"_ustr,      HANDLE_PRINTSET_EMPTY_PAGES        , cppu::UnoType<bool>::get(), PROPERTY_NONE},
        { OUString(), 0, css::uno::Type(), 0}
    };
    return new ChainablePropertySetInfo ( aPrintSettingsMap_Impl );
}

SwXModule::SwXModule()
{
}

SwXModule::~SwXModule()
{
}

Reference< XPropertySet >  SwXModule::getViewSettings()
{
    SolarMutexGuard aGuard;
    if(!mxViewSettings.is())
    {
        OSL_FAIL("Web or Text?");
        mxViewSettings = new SwXViewSettings( nullptr );
    }
    return mxViewSettings;
}

Reference< XPropertySet >  SwXModule::getPrintSettings()
{
    SolarMutexGuard aGuard;
    if(!mxPrintSettings.is())
    {
        OSL_FAIL("Web or Text?");
        mxPrintSettings = new SwXPrintSettings( SwXPrintSettingsType::Module );
    }
    return mxPrintSettings;
}

OUString SwXModule::getImplementationName()
{
    return u"SwXModule"_ustr;
}

sal_Bool SwXModule::supportsService(const OUString& rServiceName)
{
    return cppu::supportsService(this, rServiceName);
}

Sequence< OUString > SwXModule::getSupportedServiceNames()
{
    return { u"com.sun.star.text.GlobalSettings"_ustr };
}

SwXPrintSettings::SwXPrintSettings(SwXPrintSettingsType eType, SwDoc* pDoc)
: ChainablePropertySet ( lcl_createPrintSettingsInfo().get(), &Application::GetSolarMutex() )
, meType(eType)
, mpPrtOpt ( nullptr )
, mpDoc ( pDoc )
{
}

SwXPrintSettings::~SwXPrintSettings()
    noexcept
{
}

void SwXPrintSettings::_preSetValues ()
{
    switch (meType)
    {
        case SwXPrintSettingsType::Module:
            mpPrtOpt = SwModule::get()->GetPrtOptions(false);
        break;
        case SwXPrintSettingsType::Document:
        {
            if (!mpDoc)
                throw IllegalArgumentException ();
            mpPrtOpt = const_cast< SwPrintData * >(&mpDoc->getIDocumentDeviceAccess().getPrintData());
        }
        break;
    }
}

namespace
{
    bool tryBoolAccess(std::u16string_view rName, const uno::Any &rValue)
    {
        const std::optional<const bool> xPrSet = o3tl::tryAccess<bool>(rValue);
        if (!xPrSet.has_value())
            throw lang::IllegalArgumentException(OUString(OUString::Concat(rName) + " no value"), nullptr, 0);
        return *xPrSet;
    }
}

void SwXPrintSettings::_setSingleValue( const comphelper::PropertyInfo & rInfo, const uno::Any &rValue )
{
    switch( rInfo.mnHandle )
    {
        case HANDLE_PRINTSET_LEFT_PAGES:
        {
            mpPrtOpt->SetPrintLeftPage(tryBoolAccess(rInfo.maName, rValue));
        }
        break;
        case HANDLE_PRINTSET_RIGHT_PAGES:
        {
            mpPrtOpt->SetPrintRightPage(tryBoolAccess(rInfo.maName, rValue));
        }
        break;
        case HANDLE_PRINTSET_REVERSED:
        {
            // no action
        }
        break;
        case HANDLE_PRINTSET_PROSPECT:
        {
            mpPrtOpt->SetPrintProspect(tryBoolAccess(rInfo.maName, rValue));
        }
        break;
        case HANDLE_PRINTSET_GRAPHICS:
        {
            mpPrtOpt->SetPrintGraphic(tryBoolAccess(rInfo.maName, rValue));
        }
        break;
        case HANDLE_PRINTSET_TABLES:
        {
            // no action
        }
        break;
        case HANDLE_PRINTSET_DRAWINGS:
        {
            mpPrtOpt->SetPrintGraphic(tryBoolAccess(rInfo.maName, rValue));
        }
        break;
        case HANDLE_PRINTSET_CONTROLS:
        {
            mpPrtOpt->SetPrintControl(tryBoolAccess(rInfo.maName, rValue));
        }
        break;
        case HANDLE_PRINTSET_PAGE_BACKGROUND:
        {
            mpPrtOpt->SetPrintPageBackground(tryBoolAccess(rInfo.maName, rValue));
        }
        break;
        case HANDLE_PRINTSET_BLACK_FONTS:
        {
            mpPrtOpt->SetPrintBlackFont(tryBoolAccess(rInfo.maName, rValue));
        }
        break;
        case HANDLE_PRINTSET_PAPER_FROM_SETUP:
        {
            mpPrtOpt->SetPaperFromSetup(tryBoolAccess(rInfo.maName, rValue));
        }
        break;
        case HANDLE_PRINTSET_ANNOTATION_MODE:
        {
            sal_Int16 nTmp = 0;
            rValue >>= nTmp;
            SwPostItMode nVal = static_cast<SwPostItMode>(nTmp);
            if(nVal > SwPostItMode::EndPage)
                throw lang::IllegalArgumentException(OUString::number(nTmp) + " > SwPostItMode::EndPage", nullptr, 0);

            mpPrtOpt->SetPrintPostIts(nVal);
        }
        break;
        case HANDLE_PRINTSET_EMPTY_PAGES:
        {
            mpPrtOpt->SetPrintEmptyPages(tryBoolAccess(rInfo.maName, rValue));
        }
        break;
        case HANDLE_PRINTSET_FAX_NAME:
        {
            OUString sString;
            if ( !(rValue >>= sString))
                throw lang::IllegalArgumentException();

            mpPrtOpt->SetFaxName(sString);
        }
        break;
        case HANDLE_PRINTSET_PROSPECT_RTL:
        {
            mpPrtOpt->SetPrintProspect_RTL(tryBoolAccess(rInfo.maName, rValue));
        }
        break;
        case HANDLE_PRINTSET_PLACEHOLDER:
        {
            mpPrtOpt->SetPrintTextPlaceholder(tryBoolAccess(rInfo.maName, rValue));
        }
        break;
        case HANDLE_PRINTSET_HIDDEN_TEXT:
        {
            mpPrtOpt->SetPrintHiddenText(tryBoolAccess(rInfo.maName, rValue));
        }
        break;
        default:
            throw UnknownPropertyException(OUString::number(rInfo.mnHandle));
    }
}

void SwXPrintSettings::_postSetValues()
{
    mpPrtOpt = nullptr;
}

void SwXPrintSettings::_preGetValues()
{
    switch (meType)
    {
        case SwXPrintSettingsType::Module:
            mpPrtOpt = SwModule::get()->GetPrtOptions(false);
        break;
        case SwXPrintSettingsType::Document:
        {
            if (!mpDoc)
                throw IllegalArgumentException ();
            mpPrtOpt = const_cast< SwPrintData * >(&mpDoc->getIDocumentDeviceAccess().getPrintData());
        }
        break;
    }
}

void SwXPrintSettings::_getSingleValue( const comphelper::PropertyInfo & rInfo, uno::Any & rValue )
{
    switch( rInfo.mnHandle )
    {
        case HANDLE_PRINTSET_LEFT_PAGES:
            rValue <<= mpPrtOpt->IsPrintLeftPage();
        break;
        case HANDLE_PRINTSET_RIGHT_PAGES:
            rValue <<= mpPrtOpt->IsPrintRightPage();
        break;
        case HANDLE_PRINTSET_REVERSED:
            rValue <<= false;
        break;
        case HANDLE_PRINTSET_PROSPECT:
            rValue <<= mpPrtOpt->IsPrintProspect();
        break;
        case HANDLE_PRINTSET_GRAPHICS:
            rValue <<= mpPrtOpt->IsPrintGraphic();
        break;
        case HANDLE_PRINTSET_TABLES:
            rValue <<= true;
        break;
        case HANDLE_PRINTSET_DRAWINGS:
            rValue <<= mpPrtOpt->IsPrintGraphic();
        break;
        case HANDLE_PRINTSET_CONTROLS:
            rValue <<= mpPrtOpt->IsPrintControl();
        break;
        case HANDLE_PRINTSET_PAGE_BACKGROUND:
            rValue <<= mpPrtOpt->IsPrintPageBackground();
        break;
        case HANDLE_PRINTSET_BLACK_FONTS:
            rValue <<= mpPrtOpt->IsPrintBlackFont();
        break;
        case HANDLE_PRINTSET_EMPTY_PAGES:
            rValue <<= mpPrtOpt->IsPrintEmptyPages();
        break;
        case HANDLE_PRINTSET_PAPER_FROM_SETUP:
            rValue <<= mpPrtOpt->IsPaperFromSetup();
        break;
        case HANDLE_PRINTSET_ANNOTATION_MODE:
        {
            rValue <<= static_cast < sal_Int16 > ( mpPrtOpt->GetPrintPostIts() );
        }
        break;
        case HANDLE_PRINTSET_FAX_NAME :
        {
            rValue <<= mpPrtOpt->GetFaxName();
        }
        break;
        case HANDLE_PRINTSET_PROSPECT_RTL:
        {
            rValue <<= mpPrtOpt->IsPrintProspectRTL();
        }
        break;
        case HANDLE_PRINTSET_PLACEHOLDER:
        {
            rValue <<= mpPrtOpt->IsPrintTextPlaceholder();
        }
        break;
        case HANDLE_PRINTSET_HIDDEN_TEXT:
        {
            rValue <<= mpPrtOpt->IsPrintHiddenText();
        }
        break;
        default:
            throw UnknownPropertyException(OUString::number(rInfo.mnHandle));
    }
}

void SwXPrintSettings::_postGetValues ()
{
    mpPrtOpt = nullptr;
}

OUString SwXPrintSettings::getImplementationName()
{
    return u"SwXPrintSettings"_ustr;
}

sal_Bool SwXPrintSettings::supportsService(const OUString& rServiceName)
{
    return cppu::supportsService(this, rServiceName);
}

Sequence< OUString > SwXPrintSettings::getSupportedServiceNames()
{
    Sequence<OUString> aRet { u"com.sun.star.text.PrintSettings"_ustr };
    return aRet;
}

SwXViewSettings::SwXViewSettings(SwView* pVw)
    : ChainablePropertySet( lcl_createViewSettingsInfo().get(), &Application::GetSolarMutex() )
    , m_pView(pVw)
    , mpConstViewOption(nullptr)
    , m_bObjectValid(true)
    , mbApplyZoom(false)
    , m_eHRulerUnit(FieldUnit::CM)
    , mbApplyHRulerMetric(false)
    , m_eVRulerUnit(FieldUnit::CM)
    , mbApplyVRulerMetric(false)
{
    // This property only exists if we have a view (ie, not at the module )
    if ( !m_pView )
        mxInfo->remove ( u"HelpURL"_ustr );

}

SwXViewSettings::~SwXViewSettings()
    noexcept
{

}

void SwXViewSettings::_preSetValues ()
{
    const SwViewOption* pVOpt = nullptr;
    if(m_pView)
    {
        if(!IsValid())
            return;
        pVOpt = m_pView->GetWrtShell().GetViewOptions();
    }
    else
        pVOpt = SwModule::get()->GetViewOption(false);

    mpViewOption.reset( new SwViewOption (*pVOpt) );
    mbApplyZoom = false;
    if(m_pView)
        mpViewOption->SetStarOneSetting(true);
}

void SwXViewSettings::_setSingleValue( const comphelper::PropertyInfo & rInfo, const uno::Any &rValue )
{
    // the API flag should not be set to the application's view settings
    switch( rInfo.mnHandle )
    {
        case  HANDLE_VIEWSET_SHOW_RULER            :   mpViewOption->SetViewAnyRuler(*o3tl::doAccess<bool>(rValue)); break;
        case  HANDLE_VIEWSET_HRULER                :   mpViewOption->SetViewHRuler(*o3tl::doAccess<bool>(rValue));   break;
        case  HANDLE_VIEWSET_VRULER                :   mpViewOption->SetViewVRuler(*o3tl::doAccess<bool>(rValue));break;
        case  HANDLE_VIEWSET_VRULER_RIGHT          :   mpViewOption->SetVRulerRight(*o3tl::doAccess<bool>(rValue));break;
        case  HANDLE_VIEWSET_HSCROLL               :   mpViewOption->SetViewHScrollBar(*o3tl::doAccess<bool>(rValue));break;
        case  HANDLE_VIEWSET_VSCROLL               :   mpViewOption->SetViewVScrollBar(*o3tl::doAccess<bool>(rValue));break;
        case  HANDLE_VIEWSET_GRAPHICS              :   mpViewOption->SetGraphic(*o3tl::doAccess<bool>(rValue));break;
        case  HANDLE_VIEWSET_TABLES                :   mpViewOption->SetTable(*o3tl::doAccess<bool>(rValue));    break;
        case  HANDLE_VIEWSET_DRAWINGS              :   mpViewOption->SetDraw(*o3tl::doAccess<bool>(rValue)); break;
        case  HANDLE_VIEWSET_FIELD_COMMANDS        :   mpViewOption->SetFieldName(*o3tl::doAccess<bool>(rValue));  break;
        case  HANDLE_VIEWSET_ANNOTATIONS           :   mpViewOption->SetPostIts(*o3tl::doAccess<bool>(rValue));  break;
        case  HANDLE_VIEWSET_INDEX_MARK_BACKGROUND :   mpViewOption->SetAppearanceFlag(ViewOptFlags::FieldShadings, *o3tl::doAccess<bool>(rValue), true);  break;
        case  HANDLE_VIEWSET_NONPRINTING_CHARACTERS:   mpViewOption->SetViewMetaChars( *o3tl::doAccess<bool>(rValue) ); break;
        case  HANDLE_VIEWSET_FOOTNOTE_BACKGROUND   :   mpViewOption->SetAppearanceFlag(ViewOptFlags::FieldShadings, *o3tl::doAccess<bool>(rValue), true); break;
        case  HANDLE_VIEWSET_TEXT_FIELD_BACKGROUND :   mpViewOption->SetAppearanceFlag(ViewOptFlags::FieldShadings, *o3tl::doAccess<bool>(rValue), true);    break;
        case  HANDLE_VIEWSET_PARA_BREAKS           :   mpViewOption->SetParagraph(*o3tl::doAccess<bool>(rValue));    break;
        case  HANDLE_VIEWSET_SOFT_HYPHENS          :   mpViewOption->SetSoftHyph(*o3tl::doAccess<bool>(rValue)); break;
        case  HANDLE_VIEWSET_SPACES                :   mpViewOption->SetBlank(*o3tl::doAccess<bool>(rValue));    break;
        case  HANDLE_VIEWSET_PROTECTED_SPACES      :   mpViewOption->SetHardBlank(*o3tl::doAccess<bool>(rValue));    break;
        case  HANDLE_VIEWSET_TABSTOPS              :   mpViewOption->SetTab(*o3tl::doAccess<bool>(rValue));  break;
        case  HANDLE_VIEWSET_BREAKS                :   mpViewOption->SetLineBreak(*o3tl::doAccess<bool>(rValue)); break;
        case  HANDLE_VIEWSET_BOOKMARKS             :   mpViewOption->SetShowBookmarks(*o3tl::doAccess<bool>(rValue)); break;
        case  HANDLE_VIEWSET_HIDDEN_TEXT           :   mpViewOption->SetShowHiddenField(*o3tl::doAccess<bool>(rValue));  break;
        case  HANDLE_VIEWSET_HIDDEN_CHARACTERS     :   mpViewOption->SetShowHiddenChar(*o3tl::doAccess<bool>(rValue)); break;
        case  HANDLE_VIEWSET_HIDDEN_PARAGRAPHS     :   mpViewOption->SetShowHiddenPara(*o3tl::doAccess<bool>(rValue));   break;
        case  HANDLE_VIEWSET_TABLE_BOUNDARIES      :   mpViewOption->SetTableBoundaries(*o3tl::doAccess<bool>(rValue));    break;
        case  HANDLE_VIEWSET_TEXT_BOUNDARIES       :   mpViewOption->SetTextBoundaries(*o3tl::doAccess<bool>(rValue));    break;
        case  HANDLE_VIEWSET_SMOOTH_SCROLLING      :   mpViewOption->SetSmoothScroll(*o3tl::doAccess<bool>(rValue)); break;
        case  HANDLE_VIEWSET_SHOW_CONTENT_TIPS     :   mpViewOption->SetShowContentTips(*o3tl::doAccess<bool>(rValue)); break;
        case  HANDLE_VIEWSET_IS_RASTER_VISIBLE     : mpViewOption->SetGridVisible(*o3tl::doAccess<bool>(rValue)); break;
        case  HANDLE_VIEWSET_IS_SNAP_TO_RASTER     : mpViewOption->SetSnap(*o3tl::doAccess<bool>(rValue)); break;
        case  HANDLE_VIEWSET_SCROLLBAR_TIPS        : mpViewOption->SetShowScrollBarTips(*o3tl::doAccess<bool>(rValue)); break;
        case  HANDLE_VIEWSET_INLINECHANGES_TIPS    : mpViewOption->SetShowInlineTooltips(*o3tl::doAccess<bool>(rValue)); break;
        case  HANDLE_VIEWSET_USE_HEADERFOOTERMENU  : mpViewOption->SetUseHeaderFooterMenu(*o3tl::doAccess<bool>(rValue)); break;
        case  HANDLE_VIEWSET_SHOW_OUTLINECONTENTVISIBILITYBUTTON : mpViewOption->SetShowOutlineContentVisibilityButton(*o3tl::doAccess<bool>(rValue)); break;
        case  HANDLE_VIEWSET_TREAT_SUB_OUTLINE_LEVELS_AS_CONTENT : mpViewOption->SetTreatSubOutlineLevelsAsContent(*o3tl::doAccess<bool>(rValue)); break;
        case  HANDLE_VIEWSET_CHANGES_IN_MARGIN     : mpViewOption->SetShowChangesInMargin(*o3tl::doAccess<bool>(rValue)); break;
        case  HANDLE_VIEWSET_RASTER_RESOLUTION_X   :
        {
            sal_Int32 nTmp = 0;
            if(!(rValue >>= nTmp)  ||  nTmp < 10)
                throw IllegalArgumentException();
            Size aSize( mpViewOption->GetSnapSize() );
            aSize.setWidth( o3tl::toTwips(nTmp, o3tl::Length::mm100) );
            mpViewOption->SetSnapSize( aSize );
        }
        break;
        case  HANDLE_VIEWSET_RASTER_RESOLUTION_Y   :
        {
            sal_Int32 nTmp = 0;
            if(!(rValue >>= nTmp)  ||  nTmp < 10)
                throw IllegalArgumentException();
            Size aSize( mpViewOption->GetSnapSize() );
            aSize.setHeight( o3tl::toTwips(nTmp, o3tl::Length::mm100) );
            mpViewOption->SetSnapSize( aSize );
        }
        break;
        case  HANDLE_VIEWSET_RASTER_SUBDIVISION_X  :
        {
            sal_Int32 nTmp = 0;
            if(!(rValue >>= nTmp)  ||  (0 > nTmp  ||  nTmp >= 100))
                throw IllegalArgumentException();
            mpViewOption->SetDivisionX( static_cast<short>(nTmp) );
        }
        break;
        case  HANDLE_VIEWSET_RASTER_SUBDIVISION_Y  :
        {
            sal_Int32 nTmp = 0;
            if(!(rValue >>= nTmp)  ||  (0 > nTmp  ||  nTmp >= 100))
                throw IllegalArgumentException();
            mpViewOption->SetDivisionY( static_cast<short>(nTmp) );
        }
        break;
        case  HANDLE_VIEWSET_ZOOM                   :
        {
            sal_Int16 nZoom = 0;
            if(!(rValue >>= nZoom) || nZoom > MAXZOOM || nZoom < MINZOOM)
                throw lang::IllegalArgumentException();
            mpViewOption->SetZoom(o3tl::narrowing<sal_uInt16>(nZoom));
            mbApplyZoom = true;
        }
        break;
        case HANDLE_VIEWSET_ZOOM_TYPE:
        {
            sal_Int16 nZoom = 0;
            if(!(rValue >>= nZoom))
                throw IllegalArgumentException();
            SvxZoomType eZoom;
            switch (nZoom)
            {
                case view::DocumentZoomType::OPTIMAL:
                    eZoom = SvxZoomType::OPTIMAL;
                break;
                case view::DocumentZoomType::PAGE_WIDTH:
                    eZoom = SvxZoomType::PAGEWIDTH;
                break;
                case view::DocumentZoomType::ENTIRE_PAGE:
                    eZoom = SvxZoomType::WHOLEPAGE;
                break;
                case view::DocumentZoomType::BY_VALUE:
                    eZoom = SvxZoomType::PERCENT;
                break;
                case view::DocumentZoomType::PAGE_WIDTH_EXACT:
                    eZoom = SvxZoomType::PAGEWIDTH_NOBORDER;
                break;
                default:
                    throw IllegalArgumentException(
                        u"SwXViewSettings: invalid zoom type"_ustr, nullptr, 0);
            }
            mpViewOption->SetZoomType( eZoom );
            mbApplyZoom = true;
        }
        break;
        case HANDLE_VIEWSET_ONLINE_LAYOUT :
        {
            if ( m_pView )
            {
                bool bVal = *o3tl::doAccess<bool>(rValue);
                SwViewOption aOpt(*m_pView->GetWrtShell().GetViewOptions());
                if (!bVal != !aOpt.getBrowseMode())
                {
                    aOpt.setBrowseMode( bVal );
                    m_pView->GetWrtShell().ApplyViewOptions( aOpt );

                    // must be set in mpViewOption as this will overwrite settings in _post!
                    if(mpViewOption)
                        mpViewOption->setBrowseMode(bVal);

                    m_pView->GetDocShell()->ToggleLayoutMode(m_pView);
                }
            }
        }
        break;
        case HANDLE_VIEWSET_HIDE_WHITESPACE:
        {
            if ( m_pView )
            {
                bool bVal = *o3tl::doAccess<bool>(rValue);
                SwViewOption aOpt(*m_pView->GetWrtShell().GetViewOptions());
                if (!bVal != !aOpt.IsHideWhitespaceMode())
                {
                    aOpt.SetHideWhitespaceMode( bVal );
                    m_pView->GetWrtShell().ApplyViewOptions( aOpt );

                    // must be set in mpViewOption as this will overwrite settings in _post!
                    if(mpViewOption)
                        mpViewOption->SetHideWhitespaceMode(bVal);
                }
            }
        }
        break;
        case HANDLE_VIEWSET_HELP_URL:
        {
            if ( !m_pView )
                throw UnknownPropertyException();

            OUString sHelpURL;
            if ( ! ( rValue >>= sHelpURL ) )
                throw IllegalArgumentException();

            INetURLObject aHID( sHelpURL );
            if ( aHID.GetProtocol() != INetProtocol::Hid )
                throw IllegalArgumentException ();

            m_pView->GetEditWin().SetHelpId( aHID.GetURLPath() );
        }
        break;
        case HANDLE_VIEWSET_HORI_RULER_METRIC:
        case HANDLE_VIEWSET_VERT_RULER_METRIC:
        {
            sal_uInt16 nUnit;
            if( rValue >>= nUnit )
                switch (static_cast<FieldUnit>(nUnit))
                {
                case FieldUnit::MM:
                case FieldUnit::CM:
                case FieldUnit::POINT:
                case FieldUnit::PICA:
                case FieldUnit::INCH:
                    if( rInfo.mnHandle == HANDLE_VIEWSET_HORI_RULER_METRIC )
                    {
                        m_eHRulerUnit = static_cast<FieldUnit>(nUnit);
                        mbApplyHRulerMetric = true;
                    }
                    else
                    {
                        m_eVRulerUnit = static_cast<FieldUnit>(nUnit);
                        mbApplyVRulerMetric = true;
                    }
                    break;
                default:
                    throw IllegalArgumentException();
                }
        }
        break;
        default:
            throw UnknownPropertyException(OUString::number(rInfo.mnHandle));
    }
}

void SwXViewSettings::_postSetValues()
{
    SwModule* mod = SwModule::get();
    if( m_pView )
    {
        if(mbApplyZoom )
            m_pView->SetZoom( mpViewOption->GetZoomType(),
                            mpViewOption->GetZoom(), true );
        if(mbApplyHRulerMetric)
            m_pView->ChangeTabMetric(m_eHRulerUnit);
        if(mbApplyVRulerMetric)
            m_pView->ChangeVRulerMetric(m_eVRulerUnit);

    }
    else
    {
        if(mbApplyHRulerMetric)
            mod->ApplyRulerMetric(m_eHRulerUnit, true, false);
        if(mbApplyVRulerMetric)
            mod->ApplyRulerMetric(m_eVRulerUnit, false, false);
    }

    mod->ApplyUsrPref( *mpViewOption, m_pView, m_pView ? SvViewOpt::DestViewOnly
                                                  : SvViewOpt::DestText );

    mpViewOption.reset();
}

void SwXViewSettings::_preGetValues ()
{
    if(m_pView)
    {
        if(!IsValid())
            return;
        mpConstViewOption = m_pView->GetWrtShell().GetViewOptions();
    }
    else
        mpConstViewOption = SwModule::get()->GetViewOption(false);
}

void SwXViewSettings::_getSingleValue( const comphelper::PropertyInfo & rInfo, uno::Any & rValue )
{
    bool bBool = true;
    bool bBoolVal = false;
    switch( rInfo.mnHandle )
    {
        case  HANDLE_VIEWSET_SHOW_RULER:                bBoolVal = mpConstViewOption->IsViewAnyRuler();   break;
        case  HANDLE_VIEWSET_HRULER :                   bBoolVal = mpConstViewOption->IsViewHRuler(true);   break;
        case  HANDLE_VIEWSET_VRULER :                   bBoolVal = mpConstViewOption->IsViewVRuler(true);break;
        case  HANDLE_VIEWSET_VRULER_RIGHT          :   bBoolVal = mpConstViewOption->IsVRulerRight();break;
        case  HANDLE_VIEWSET_HSCROLL:                   bBoolVal = mpConstViewOption->IsViewHScrollBar();break;
        case  HANDLE_VIEWSET_VSCROLL:                   bBoolVal = mpConstViewOption->IsViewVScrollBar();break;
        case  HANDLE_VIEWSET_GRAPHICS              :    bBoolVal = mpConstViewOption->IsGraphic();break;
        case  HANDLE_VIEWSET_TABLES                :   bBoolVal = mpConstViewOption->IsTable(); break;
        case  HANDLE_VIEWSET_DRAWINGS              :   bBoolVal = mpConstViewOption->IsDraw();  break;
        case  HANDLE_VIEWSET_FIELD_COMMANDS        :   bBoolVal = mpConstViewOption->IsFieldName();   break;
        case  HANDLE_VIEWSET_ANNOTATIONS           :   bBoolVal = mpConstViewOption->IsPostIts();   break;
        case  HANDLE_VIEWSET_INDEX_MARK_BACKGROUND :   bBoolVal = mpConstViewOption->IsFieldShadings();   break;
        case  HANDLE_VIEWSET_NONPRINTING_CHARACTERS:   bBoolVal = mpConstViewOption->IsViewMetaChars(); break;
        case  HANDLE_VIEWSET_FOOTNOTE_BACKGROUND   :   bBoolVal = mpConstViewOption->IsFieldShadings();  break;
        case  HANDLE_VIEWSET_TEXT_FIELD_BACKGROUND :   bBoolVal = mpConstViewOption->IsFieldShadings(); break;
        case  HANDLE_VIEWSET_PARA_BREAKS           :   bBoolVal = mpConstViewOption->IsParagraph(true); break;
        case  HANDLE_VIEWSET_SOFT_HYPHENS          :   bBoolVal = mpConstViewOption->IsSoftHyph();  break;
        case  HANDLE_VIEWSET_SPACES                :   bBoolVal = mpConstViewOption->IsBlank(true); break;
        case  HANDLE_VIEWSET_PROTECTED_SPACES      :   bBoolVal = mpConstViewOption->IsHardBlank(); break;
        case  HANDLE_VIEWSET_TABSTOPS              :   bBoolVal = mpConstViewOption->IsTab(true);   break;
        case  HANDLE_VIEWSET_BREAKS                :   bBoolVal = mpConstViewOption->IsLineBreak(true); break;
        case  HANDLE_VIEWSET_BOOKMARKS             :   bBoolVal = mpConstViewOption->IsShowBookmarks(true); break;
        case  HANDLE_VIEWSET_HIDDEN_TEXT           :   bBoolVal = mpConstViewOption->IsShowHiddenField();   break;
        case  HANDLE_VIEWSET_HIDDEN_CHARACTERS     :   bBoolVal = mpConstViewOption->IsShowHiddenChar(true); break;
        case  HANDLE_VIEWSET_HIDE_WHITESPACE       :   bBoolVal = mpConstViewOption->IsHideWhitespaceMode(); break;
        case  HANDLE_VIEWSET_HIDDEN_PARAGRAPHS     :   bBoolVal = mpConstViewOption->IsShowHiddenPara();    break;
        case  HANDLE_VIEWSET_TABLE_BOUNDARIES      :   bBoolVal = mpConstViewOption->IsTableBoundaries(); break;
        case  HANDLE_VIEWSET_TEXT_BOUNDARIES       :   bBoolVal = mpConstViewOption->IsTextBoundaries(); break;
        case  HANDLE_VIEWSET_SMOOTH_SCROLLING      :   bBoolVal = mpConstViewOption->IsSmoothScroll();  break;
        case  HANDLE_VIEWSET_SHOW_CONTENT_TIPS     :   bBoolVal = mpConstViewOption->IsShowContentTips(); break;
        case  HANDLE_VIEWSET_INLINECHANGES_TIPS    :   bBoolVal = mpConstViewOption->IsShowInlineTooltips(); break;
        case  HANDLE_VIEWSET_CHANGES_IN_MARGIN     :   bBoolVal = mpConstViewOption->IsShowChangesInMargin(); break;
        case  HANDLE_VIEWSET_IS_RASTER_VISIBLE     : bBoolVal = mpConstViewOption->IsGridVisible(); break;
        case  HANDLE_VIEWSET_IS_SNAP_TO_RASTER     : bBoolVal = mpConstViewOption->IsSnap(); break;
        case  HANDLE_VIEWSET_SCROLLBAR_TIPS        : bBoolVal = mpConstViewOption->IsShowScrollBarTips(); break;
        case  HANDLE_VIEWSET_RASTER_RESOLUTION_X   :
            bBool = false;
            rValue <<= static_cast<sal_Int32>(convertTwipToMm100(mpConstViewOption->GetSnapSize().Width()));
        break;
        case  HANDLE_VIEWSET_RASTER_RESOLUTION_Y   :
            bBool = false;
            rValue <<= static_cast<sal_Int32>(convertTwipToMm100(mpConstViewOption->GetSnapSize().Height()));
        break;
        case  HANDLE_VIEWSET_RASTER_SUBDIVISION_X  :
            bBool = false;
            rValue <<= static_cast<sal_Int32>(mpConstViewOption->GetDivisionX());
        break;
        case  HANDLE_VIEWSET_RASTER_SUBDIVISION_Y  :
            bBool = false;
            rValue <<= static_cast<sal_Int32>(mpConstViewOption->GetDivisionY());
        break;
        case  HANDLE_VIEWSET_ZOOM                   :
                bBool = false;
                rValue <<= static_cast<sal_Int16>(mpConstViewOption->GetZoom());
        break;
        case HANDLE_VIEWSET_ZOOM_TYPE:
        {
            bBool = false;
            sal_Int16 nRet(0);
            switch (mpConstViewOption->GetZoomType())
            {
                case SvxZoomType::OPTIMAL:
                    nRet = view::DocumentZoomType::OPTIMAL;
                break;
                case SvxZoomType::PAGEWIDTH:
                    nRet = view::DocumentZoomType::PAGE_WIDTH;
                break;
                case SvxZoomType::WHOLEPAGE:
                    nRet = view::DocumentZoomType::ENTIRE_PAGE;
                break;
                case SvxZoomType::PERCENT:
                    nRet = view::DocumentZoomType::BY_VALUE;
                break;
                case SvxZoomType::PAGEWIDTH_NOBORDER:
                    nRet = view::DocumentZoomType::PAGE_WIDTH_EXACT;
                break;
                default:
                    OSL_FAIL("SwXViewSettings: invalid zoom type");
                break;
            }
            rValue <<= nRet;
        }
        break;
        case HANDLE_VIEWSET_ONLINE_LAYOUT:
            if(m_pView)
                bBoolVal = m_pView->GetWrtShell().GetViewOptions()->getBrowseMode();
        break;
        case HANDLE_VIEWSET_HELP_URL :
        {
            if ( !m_pView )
                throw UnknownPropertyException();

            bBool = false;
            SwEditWin &rEditWin = m_pView->GetEditWin();
            OUString sHelpURL = INET_HID_SCHEME + rEditWin.GetHelpId();
            rValue <<= sHelpURL;
        }
        break;
        case HANDLE_VIEWSET_HORI_RULER_METRIC:
        {
            if ( m_pView )
            {
                FieldUnit eUnit;
                m_pView->GetHRulerMetric( eUnit );
                rValue <<= static_cast<sal_Int32>(eUnit);
            }
            else
            {
                const SwMasterUsrPref* pUsrPref = SwModule::get()->GetUsrPref(false);
                rValue <<= static_cast<sal_Int32>(pUsrPref->GetHScrollMetric());
            }
            bBool = false;
        }
        break;
        case HANDLE_VIEWSET_VERT_RULER_METRIC:
        {
            if ( m_pView )
            {
                FieldUnit eUnit;
                m_pView->GetVRulerMetric( eUnit );
                rValue <<= static_cast<sal_Int32>(eUnit);
            }
            else
            {
                const SwMasterUsrPref* pUsrPref = SwModule::get()->GetUsrPref(false);
                rValue <<= static_cast<sal_Int32>(pUsrPref->GetVScrollMetric());
            }
            bBool = false;
        }
        break;
        default: OSL_FAIL("there is no such ID!");
    }
    if( bBool )
        rValue <<= bBoolVal;
}

void SwXViewSettings::_postGetValues ()
{
    mpConstViewOption = nullptr;
}

OUString SwXViewSettings::getImplementationName()
{
    return u"SwXViewSettings"_ustr;
}

sal_Bool SwXViewSettings::supportsService(const OUString& rServiceName)
{
    return cppu::supportsService(this, rServiceName);
}

Sequence< OUString > SwXViewSettings::getSupportedServiceNames()
{
    Sequence<OUString> aRet { u"com.sun.star.text.ViewSettings"_ustr };
    return aRet;
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
SwXModule_get_implementation(css::uno::XComponentContext*,
        css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new SwXModule());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
