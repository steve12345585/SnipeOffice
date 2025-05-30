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

#include <uielement/langselectionmenucontroller.hxx>

#include <services.h>

#include <com/sun/star/awt/MenuItemStyle.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/util/XURLTransformer.hpp>

#include <vcl/svapp.hxx>

#include <svl/languageoptions.hxx>
#include <svtools/langtab.hxx>
#include <toolkit/awt/vclxmenu.hxx>
#include <classes/fwkresid.hxx>

#include <strings.hrc>

#include <helper/mischelper.hxx>
#include <osl/mutex.hxx>
#include <cppuhelper/supportsservice.hxx>

#include <map>
#include <set>

//  Defines

using namespace ::com::sun::star;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::frame;
using namespace com::sun::star::util;

namespace framework
{

// XInterface, XTypeProvider, XServiceInfo

OUString SAL_CALL LanguageSelectionMenuController::getImplementationName()
{
    return u"com.sun.star.comp.framework.LanguageSelectionMenuController"_ustr;
}

sal_Bool SAL_CALL LanguageSelectionMenuController::supportsService( const OUString& sServiceName )
{
    return cppu::supportsService(this, sServiceName);
}

css::uno::Sequence< OUString > SAL_CALL LanguageSelectionMenuController::getSupportedServiceNames()
{
    return { SERVICENAME_POPUPMENUCONTROLLER };
}


LanguageSelectionMenuController::LanguageSelectionMenuController( const css::uno::Reference< css::uno::XComponentContext >& xContext )
    : svt::PopupMenuControllerBase(xContext)
    , m_bShowMenu(true)
    , m_nScriptType(SvtScriptType::LATIN | SvtScriptType::ASIAN | SvtScriptType::COMPLEX)
    , m_aLangGuessHelper(xContext)
{
}

LanguageSelectionMenuController::~LanguageSelectionMenuController()
{
}

// XEventListener
void SAL_CALL LanguageSelectionMenuController::disposing( const EventObject& )
{
    Reference< css::awt::XMenuListener > xHolder(this);

    std::unique_lock aLock( m_aMutex );
    m_xFrame.clear();
    m_xDispatch.clear();
    m_xLanguageDispatch.clear();

    if ( m_xPopupMenu.is() )
        m_xPopupMenu->removeMenuListener( Reference< css::awt::XMenuListener >(this) );
    m_xPopupMenu.clear();
}

// XStatusListener
void SAL_CALL LanguageSelectionMenuController::statusChanged( const FeatureStateEvent& Event )
{
    SolarMutexGuard aSolarMutexGuard;

    if (m_bDisposed)
        return;

    m_bShowMenu = true;
    m_nScriptType = SvtScriptType::LATIN | SvtScriptType::ASIAN | SvtScriptType::COMPLEX;  //set the default value

    Sequence< OUString > aSeq;

    if ( Event.State >>= aSeq )
    {
        if ( aSeq.getLength() == 4 )
        {
            // Retrieve all other values from the sequence and
            // store it members!
            m_aCurLang          = aSeq[0];
            m_nScriptType       = static_cast< SvtScriptType >(aSeq[1].toInt32());
            m_aKeyboardLang     = aSeq[2];
            m_aGuessedTextLang  = aSeq[3];
        }
    }
    else if ( !Event.State.hasValue() )
    {
        m_bShowMenu = false;    // no language -> no sub-menu entries -> disable menu
    }
}

// XPopupMenuController
void LanguageSelectionMenuController::impl_setPopupMenu(std::unique_lock<std::mutex>& /*rGuard*/)
{
    Reference< XDispatchProvider > xDispatchProvider( m_xFrame, UNO_QUERY );

    css::util::URL aTargetURL;

    // Register for language updates
    aTargetURL.Complete = m_aLangStatusCommandURL;
    m_xURLTransformer->parseStrict( aTargetURL );
    m_xLanguageDispatch = xDispatchProvider->queryDispatch( aTargetURL, OUString(), 0 );

    // Register for setting languages and opening language dialog
    aTargetURL.Complete = m_aMenuCommandURL_Lang;
    m_xURLTransformer->parseStrict( aTargetURL );
    m_xMenuDispatch_Lang = xDispatchProvider->queryDispatch( aTargetURL, OUString(), 0 );

    // Register for opening character dialog
    aTargetURL.Complete = m_aMenuCommandURL_Font;
    m_xURLTransformer->parseStrict( aTargetURL );
    m_xMenuDispatch_Font = xDispatchProvider->queryDispatch( aTargetURL, OUString(), 0 );

    // Register for opening character dialog with preselected paragraph
    aTargetURL.Complete = m_aMenuCommandURL_CharDlgForParagraph;
    m_xURLTransformer->parseStrict( aTargetURL );
    m_xMenuDispatch_CharDlgForParagraph = xDispatchProvider->queryDispatch( aTargetURL, OUString(), 0 );
}

void LanguageSelectionMenuController::fillPopupMenu( Reference< css::awt::XPopupMenu > const & rPopupMenu , const Mode eMode )
{
    SolarMutexGuard aSolarMutexGuard;

    resetPopupMenu( rPopupMenu );
    if (!m_bShowMenu)
        return;

    OUString aCmd_Dialog;
    OUString aCmd_Language;
    if( eMode == MODE_SetLanguageSelectionMenu )
    {
        aCmd_Dialog += ".uno:FontDialog?Page:string=font";
        aCmd_Language += ".uno:LanguageStatus?Language:string=Current_";
    }
    else if ( eMode == MODE_SetLanguageParagraphMenu )
    {
        aCmd_Dialog += ".uno:FontDialogForParagraph";
        aCmd_Language += ".uno:LanguageStatus?Language:string=Paragraph_";
    }
    else if ( eMode == MODE_SetLanguageAllTextMenu )
    {
        aCmd_Dialog += ".uno:LanguageStatus?Language:string=*";
        aCmd_Language += ".uno:LanguageStatus?Language:string=Default_";
    }

    // get languages to be displayed in the menu
    std::set< OUString > aLangItems;
    FillLangItems( aLangItems, m_xFrame, m_aLangGuessHelper,
            m_nScriptType, m_aCurLang, m_aKeyboardLang, m_aGuessedTextLang );

    // now add menu entries
    // the different menus purpose will be handled by the different string
    // for aCmd_Dialog and aCmd_Language
    sal_Int16 nItemId = 0;  // in this control the item id is not important for executing the command
    static constexpr OUStringLiteral sAsterisk(u"*");  // multiple languages in current selection
    const OUString sNone( SvtLanguageTable::GetLanguageString( LANGUAGE_NONE ));
    for (auto const& langItem : aLangItems)
    {
        if (langItem != sNone &&
            langItem != sAsterisk &&
            !langItem.isEmpty()) // 'no language found' from language guessing
        {
            ++nItemId;
            rPopupMenu->insertItem(nItemId, langItem, css::awt::MenuItemStyle::CHECKABLE, nItemId - 1);
            OUString aCmd = aCmd_Language + langItem;
            rPopupMenu->setCommand(nItemId, aCmd);
            bool bChecked = langItem == m_aCurLang && eMode == MODE_SetLanguageSelectionMenu;
            //make a sign for the current language
            rPopupMenu->checkItem(nItemId, bChecked);
        }
    }

    // entry for LANGUAGE_NONE
    ++nItemId;
    rPopupMenu->insertItem(nItemId, FwkResId(STR_LANGSTATUS_NONE), 0, nItemId - 1);
    OUString aCmd = aCmd_Language + "LANGUAGE_NONE";
    rPopupMenu->setCommand(nItemId, aCmd);

    // entry for 'Reset to default language'
    ++nItemId;
    rPopupMenu->insertItem(nItemId, FwkResId(STR_RESET_TO_DEFAULT_LANGUAGE), 0, nItemId - 1);
    aCmd = aCmd_Language + "RESET_LANGUAGES";
    rPopupMenu->setCommand(nItemId, aCmd);

    // entry for opening the Format/Character dialog
    ++nItemId;
    rPopupMenu->insertItem(nItemId, FwkResId(STR_LANGSTATUS_MORE), 0, nItemId - 1);
    rPopupMenu->setCommand(nItemId, aCmd_Dialog);
}

void SAL_CALL LanguageSelectionMenuController::updatePopupMenu()
{
    svt::PopupMenuControllerBase::updatePopupMenu();

    // Force status update to get information about the current languages
    std::unique_lock aLock( m_aMutex );
    Reference< XDispatch > xDispatch( m_xLanguageDispatch );
    css::util::URL aTargetURL;
    aTargetURL.Complete = m_aLangStatusCommandURL;
    m_xURLTransformer->parseStrict( aTargetURL );
    aLock.unlock();

    if ( xDispatch.is() )
    {
        xDispatch->addStatusListener( static_cast< XStatusListener* >(this), aTargetURL );
        xDispatch->removeStatusListener( static_cast< XStatusListener* >(this), aTargetURL );
    }

    // TODO: Fill menu with the information retrieved by the status update

    if ( m_aCommandURL == ".uno:SetLanguageSelectionMenu" )
    {
        fillPopupMenu(m_xPopupMenu, MODE_SetLanguageSelectionMenu );
    }
    else if ( m_aCommandURL == ".uno:SetLanguageParagraphMenu" )
    {
        fillPopupMenu(m_xPopupMenu, MODE_SetLanguageParagraphMenu );
    }
    else if ( m_aCommandURL == ".uno:SetLanguageAllTextMenu" )
    {
        fillPopupMenu(m_xPopupMenu, MODE_SetLanguageAllTextMenu );
    }
}

// XInitialization
void LanguageSelectionMenuController::initializeImpl( std::unique_lock<std::mutex>& rGuard, const Sequence< Any >& aArguments )
{
    bool bInitialized( m_bInitialized );
    if ( !bInitialized )
    {
        svt::PopupMenuControllerBase::initializeImpl(rGuard, aArguments);

        if ( m_bInitialized )
        {
            m_aLangStatusCommandURL               = ".uno:LanguageStatus";
            m_aMenuCommandURL_Lang                = m_aLangStatusCommandURL;
            m_aMenuCommandURL_Font                = ".uno:FontDialog";
            m_aMenuCommandURL_CharDlgForParagraph = ".uno:FontDialogForParagraph";
        }
    }
}

}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
framework_LanguageSelectionMenuController_get_implementation(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const& )
{
    return cppu::acquire(new framework::LanguageSelectionMenuController(context));
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
