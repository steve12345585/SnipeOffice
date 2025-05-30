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

#include <sal/config.h>

#include <string_view>

#include <localizationmgr.hxx>

#include <basidesh.hxx>
#include <baside3.hxx>
#include <basobj.hxx>
#include <iderdll.hxx>
#include <dlged.hxx>
#include <managelang.hxx>

#include <com/sun/star/frame/XLayoutManager.hpp>
#include <com/sun/star/resource/MissingResourceException.hpp>
#include <com/sun/star/resource/XStringResourceSupplier.hpp>
#include <sfx2/bindings.hxx>
#include <sfx2/sfxsids.hrc>
#include <sfx2/viewfrm.hxx>
#include <tools/debug.hxx>
#include <utility>
#include <osl/diagnose.h>
#include <o3tl/string_view.hxx>

namespace basctl
{

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::resource;

namespace
{

constexpr OUString aDot(u"."_ustr);
constexpr OUString aEsc(u"&"_ustr);
constexpr OUString aSemi(u";"_ustr);

} // namespace

LocalizationMgr::LocalizationMgr(
    Shell* pShell,
    ScriptDocument aDocument,
    OUString aLibName,
    Reference<XStringResourceManager> const& xStringResourceManager
) :
    m_xStringResourceManager(xStringResourceManager),
    m_pShell(pShell),
    m_aDocument(std::move(aDocument)),
    m_aLibName(std::move(aLibName))
{ }

bool LocalizationMgr::isLibraryLocalized ()
{
    if (m_xStringResourceManager.is())
        return m_xStringResourceManager->getLocales().hasElements();
    return false;
}

void LocalizationMgr::handleTranslationbar ()
{
    static constexpr OUString aToolBarResName = u"private:resource/toolbar/translationbar"_ustr;

    Reference< beans::XPropertySet > xFrameProps
        ( m_pShell->GetViewFrame().GetFrame().GetFrameInterface(), uno::UNO_QUERY );
    if ( !xFrameProps.is() )
        return;

    Reference< css::frame::XLayoutManager > xLayoutManager;
    uno::Any a = xFrameProps->getPropertyValue( u"LayoutManager"_ustr );
    a >>= xLayoutManager;
    if ( xLayoutManager.is() )
    {
        if ( !isLibraryLocalized() )
        {
            xLayoutManager->destroyElement( aToolBarResName );
        }
        else
        {
            xLayoutManager->createElement( aToolBarResName );
            xLayoutManager->requestElement( aToolBarResName );
        }
    }
}


// TODO: -> export from toolkit


static bool isLanguageDependentProperty( std::u16string_view aName )
{
    static struct Prop
    {
        const char* sName;
        sal_Int32 nNameLength;
    }
    const vProp[] =
    {
        { "Text",            4 },
        { "Label",           5 },
        { "Title",           5 },
        { "HelpText",        8 },
        { "CurrencySymbol", 14 },
        { "StringItemList", 14 },
        { nullptr, 0                 }
    };

    for (Prop const* pProp = vProp; pProp->sName; ++pProp)
        if (o3tl::equalsAscii(aName, std::string_view(pProp->sName, pProp->nNameLength)))
            return true;
    return false;
}


void LocalizationMgr::implEnableDisableResourceForAllLibraryDialogs( HandleResourceMode eMode )
{
    Reference< XStringResourceResolver > xDummyStringResolver;
    for (auto& aDlgName : m_aDocument.getObjectNames(E_DIALOGS, m_aLibName))
    {
        if (VclPtr<DialogWindow> pWin = m_pShell->FindDlgWin(m_aDocument, m_aLibName, aDlgName))
        {
            Reference< container::XNameContainer > xDialog = pWin->GetDialog();
            if( xDialog.is() )
            {
                // Handle dialog itself as control
                Any aDialogCtrl;
                aDialogCtrl <<= xDialog;
                implHandleControlResourceProperties( aDialogCtrl, aDlgName,
                    std::u16string_view(), m_xStringResourceManager, xDummyStringResolver, eMode );

                // Handle all controls
                for (auto& aCtrlName : xDialog->getElementNames())
                {
                    Any aCtrl = xDialog->getByName( aCtrlName );
                    implHandleControlResourceProperties( aCtrl, aDlgName,
                        aCtrlName, m_xStringResourceManager, xDummyStringResolver, eMode );
                }
            }
        }
    }
}


static OUString implCreatePureResourceId
    ( std::u16string_view aDialogName, std::u16string_view aCtrlName,
      std::u16string_view aPropName,
      const Reference< XStringResourceManager >& xStringResourceManager )
{
    sal_Int32 nUniqueId = xStringResourceManager->getUniqueNumericId();
    OUString aPureIdStr = OUString::number( nUniqueId )
                        + aDot
                        + aDialogName
                        + aDot;
    if( !aCtrlName.empty() )
    {
        aPureIdStr += aCtrlName + aDot;
    }
    aPureIdStr += aPropName;
    return aPureIdStr;
}

// Works on xStringResourceManager's current language for SET_IDS/RESET_IDS,
// anyway only one language should exist when calling this method then,
// either the first one for mode SET_IDS or the last one for mode RESET_IDS
sal_Int32 LocalizationMgr::implHandleControlResourceProperties
    (const Any& rControlAny, std::u16string_view aDialogName, std::u16string_view aCtrlName,
        const Reference< XStringResourceManager >& xStringResourceManager,
        const Reference< XStringResourceResolver >& xSourceStringResolver, HandleResourceMode eMode )
{
    sal_Int32 nChangedCount = 0;

    Reference< XPropertySet > xPropertySet;
    rControlAny >>= xPropertySet;
    if( xPropertySet.is() && xStringResourceManager.is())
    {
        Sequence< Locale > aLocaleSeq = xStringResourceManager->getLocales();
        if (!aLocaleSeq.hasElements())
            return 0;

        Reference< XPropertySetInfo > xPropertySetInfo = xPropertySet->getPropertySetInfo();
        if( xPropertySetInfo.is() )
        {
            // get sequence of control properties
            // create a map of tab indices and control names, sorted by tab index
            for (auto& rProp : xPropertySetInfo->getProperties())
            {
                OUString aPropName = rProp.Name;
                TypeClass eType = rProp.Type.getTypeClass();
                bool bLanguageDependentProperty =
                    (eType == TypeClass_STRING || eType == TypeClass_SEQUENCE)
                    && isLanguageDependentProperty( aPropName );
                if( !bLanguageDependentProperty )
                    continue;

                if( eType == TypeClass_STRING )
                {
                    Any aPropAny = xPropertySet->getPropertyValue( aPropName );
                    OUString aPropStr;
                    aPropAny >>= aPropStr;

                    // Replace string by id, add id+string to StringResource
                    if( eMode == SET_IDS )
                    {
                        bool bEscAlreadyExisting = aPropStr.startsWith("&");
                        if( bEscAlreadyExisting )
                            continue;

                        OUString aPureIdStr = implCreatePureResourceId
                            ( aDialogName, aCtrlName, aPropName, xStringResourceManager );

                        // Set Id for all locales
                        for (auto& rLocale : aLocaleSeq)
                        {
                            xStringResourceManager->setStringForLocale( aPureIdStr, aPropStr, rLocale );
                        }

                        OUString aPropIdStr = aEsc + aPureIdStr;
                        // TODO?: Change here and in toolkit
                        (void)aSemi;
                        xPropertySet->setPropertyValue( aPropName, Any(aPropIdStr) );
                    }
                    // Replace id by string from StringResource
                    else if( eMode == RESET_IDS )
                    {
                        if( aPropStr.getLength() > 1 )
                        {
                            OUString aPureIdStr = aPropStr.copy( 1 );
                            OUString aNewPropStr = aPropStr;
                            try
                            {
                                aNewPropStr = xStringResourceManager->resolveString( aPureIdStr );
                            }
                            catch(const MissingResourceException&)
                            {
                            }
                            xPropertySet->setPropertyValue( aPropName, Any(aNewPropStr) );
                        }
                    }
                    // Remove Id for all locales
                    else if( eMode == REMOVE_IDS_FROM_RESOURCE )
                    {
                        if( aPropStr.getLength() > 1 )
                        {
                            OUString aPureIdStr = aPropStr.copy( 1 );

                            for (auto& rLocale : aLocaleSeq)
                            {
                                try
                                {
                                    xStringResourceManager->removeIdForLocale( aPureIdStr, rLocale );
                                }
                                catch(const MissingResourceException&)
                                {
                                }
                            }
                        }
                    }
                    // Rename resource id
                    else if( eMode == RENAME_DIALOG_IDS || eMode == RENAME_CONTROL_IDS )
                    {
                        OUString aPureSourceIdStr = aPropStr.copy( 1 );

                        OUString aPureIdStr = implCreatePureResourceId
                            ( aDialogName, aCtrlName, aPropName, xStringResourceManager );

                        // Set new Id and remove old one for all locales
                        for (auto& rLocale : aLocaleSeq)
                        {
                            try
                            {
                                OUString aResStr = xStringResourceManager->resolveStringForLocale
                                    ( aPureSourceIdStr, rLocale );
                                xStringResourceManager->removeIdForLocale( aPureSourceIdStr, rLocale );
                                xStringResourceManager->setStringForLocale( aPureIdStr, aResStr, rLocale );
                            }
                            catch(const MissingResourceException&)
                            {}
                        }

                        OUString aPropIdStr = aEsc + aPureIdStr;
                        // TODO?: Change here and in toolkit
                        (void)aSemi;
                        xPropertySet->setPropertyValue( aPropName, Any(aPropIdStr) );
                    }
                    // Replace string by string from source StringResourceResolver
                    else if( eMode == MOVE_RESOURCES && xSourceStringResolver.is() )
                    {
                        OUString aPureSourceIdStr = aPropStr.copy( 1 );

                        OUString aPureIdStr = implCreatePureResourceId
                            ( aDialogName, aCtrlName, aPropName, xStringResourceManager );

                        const Locale aDefaultLocale = xSourceStringResolver->getDefaultLocale();

                        // Set Id for all locales
                        for (auto& rLocale : aLocaleSeq)
                        {
                            OUString aResStr;
                            try
                            {
                                aResStr = xSourceStringResolver->resolveStringForLocale
                                    ( aPureSourceIdStr, rLocale );
                            }
                            catch(const MissingResourceException&)
                            {
                                aResStr = xSourceStringResolver->resolveStringForLocale
                                    ( aPureSourceIdStr, aDefaultLocale );
                            }
                            xStringResourceManager->setStringForLocale( aPureIdStr, aResStr, rLocale );
                        }

                        OUString aPropIdStr = aEsc + aPureIdStr;
                        // TODO?: Change here and in toolkit
                        (void)aSemi;
                        xPropertySet->setPropertyValue( aPropName, Any(aPropIdStr) );
                    }
                    // Copy string from source to target resource
                    else if( eMode == COPY_RESOURCES && xSourceStringResolver.is() )
                    {
                        OUString aPureSourceIdStr = aPropStr.copy( 1 );

                        const Locale aDefaultLocale = xSourceStringResolver->getDefaultLocale();

                        // Copy Id for all locales
                        for (auto& rLocale : aLocaleSeq)
                        {
                            OUString aResStr;
                            try
                            {
                                aResStr = xSourceStringResolver->resolveStringForLocale
                                    ( aPureSourceIdStr, rLocale );
                            }
                            catch(const MissingResourceException&)
                            {
                                aResStr = xSourceStringResolver->resolveStringForLocale
                                    ( aPureSourceIdStr, aDefaultLocale );
                            }
                            xStringResourceManager->setStringForLocale( aPureSourceIdStr, aResStr, rLocale );
                        }
                    }
                    nChangedCount++;
                }

                // Listbox / Combobox
                else if( eType == TypeClass_SEQUENCE )
                {
                    Any aPropAny = xPropertySet->getPropertyValue( aPropName );
                    Sequence< OUString > aPropStrings;
                    aPropAny >>= aPropStrings;

                    sal_Int32 nPropStringCount = aPropStrings.getLength();
                    if( nPropStringCount == 0 )
                        continue;

                    // Replace string by id, add id+string to StringResource
                    if( eMode == SET_IDS )
                    {
                        Sequence< OUString > aIdStrings(nPropStringCount);
                        OUString* pIdStrings = aIdStrings.getArray();

                        OUString aIdStrBase = aDot
                                            + aCtrlName
                                            + aDot
                                            + aPropName;

                        sal_Int32 i;
                        for ( i = 0; i < nPropStringCount; ++i )
                        {
                            const OUString& aPropStr = aPropStrings[i];
                            bool bEscAlreadyExisting = aPropStr.startsWith("&");
                            if( bEscAlreadyExisting )
                            {
                                pIdStrings[i] = aPropStr;
                                continue;
                            }

                            sal_Int32 nUniqueId = xStringResourceManager->getUniqueNumericId();
                            OUString aPureIdStr = OUString::number( nUniqueId )
                                                + aIdStrBase;

                            // Set Id for all locales
                            for (auto& rLocale : aLocaleSeq)
                            {
                                xStringResourceManager->setStringForLocale( aPureIdStr, aPropStr, rLocale );
                            }

                            pIdStrings[i] = aEsc + aPureIdStr;
                        }
                        xPropertySet->setPropertyValue( aPropName, Any(aIdStrings) );
                    }
                    // Replace id by string from StringResource
                    else if( eMode == RESET_IDS )
                    {
                        Sequence<OUString> aNewPropStrings(nPropStringCount);
                        OUString* pNewPropStrings = aNewPropStrings.getArray();

                        for (sal_Int32 i = 0; i < nPropStringCount; ++i)
                        {
                            const OUString& aIdStr = aPropStrings[i];
                            OUString aNewPropStr = aIdStr;
                            if( aIdStr.getLength() > 1 )
                            {
                                OUString aPureIdStr = aIdStr.copy( 1 );
                                try
                                {
                                    aNewPropStr = xStringResourceManager->resolveString( aPureIdStr );
                                }
                                catch(const MissingResourceException&)
                                {
                                }
                            }
                            pNewPropStrings[i] = aNewPropStr;
                        }
                        xPropertySet->setPropertyValue( aPropName, Any(aNewPropStrings) );
                    }
                    // Remove Id for all locales
                    else if( eMode == REMOVE_IDS_FROM_RESOURCE )
                    {
                        for (auto& aIdStr : aPropStrings)
                        {
                            if( aIdStr.getLength() > 1 )
                            {
                                OUString aPureIdStr = aIdStr.copy( 1 );

                                for (auto& rLocale : aLocaleSeq)
                                {
                                    try
                                    {
                                        xStringResourceManager->removeIdForLocale( aPureIdStr, rLocale );
                                    }
                                    catch(const MissingResourceException&)
                                    {
                                    }
                                }
                            }
                        }
                    }
                    // Rename resource id
                    else if( eMode == RENAME_CONTROL_IDS )
                    {
                        Sequence<OUString> aIdStrings(nPropStringCount);
                        OUString* pIdStrings = aIdStrings.getArray();

                        OUString aIdStrBase = aDot
                                            + aCtrlName
                                            + aDot
                                            + aPropName;

                        for (sal_Int32 i = 0; i < nPropStringCount; ++i)
                        {
                            OUString aPureSourceIdStr = aPropStrings[i].copy( 1 );

                            sal_Int32 nUniqueId = xStringResourceManager->getUniqueNumericId();
                            OUString aPureIdStr = OUString::number( nUniqueId )
                                                + aIdStrBase;

                            // Set Id for all locales
                            for (auto& rLocale : aLocaleSeq)
                            {
                                try
                                {
                                    OUString aResStr = xStringResourceManager->resolveStringForLocale
                                        ( aPureSourceIdStr, rLocale );
                                    xStringResourceManager->removeIdForLocale( aPureSourceIdStr, rLocale );
                                    xStringResourceManager->setStringForLocale( aPureIdStr, aResStr, rLocale );
                                }
                                catch(const MissingResourceException&)
                                {}
                            }

                            pIdStrings[i] = aEsc + aPureIdStr;
                        }
                        xPropertySet->setPropertyValue( aPropName, Any(aIdStrings) );
                    }
                    // Replace string by string from source StringResourceResolver
                    else if( eMode == MOVE_RESOURCES && xSourceStringResolver.is() )
                    {
                        Sequence<OUString> aIdStrings(nPropStringCount);
                        OUString* pIdStrings = aIdStrings.getArray();

                        OUString aIdStrBase = aDot
                                            + aCtrlName
                                            + aDot
                                            + aPropName;

                        const Locale aDefaultLocale = xSourceStringResolver->getDefaultLocale();

                        for (sal_Int32 i = 0; i < nPropStringCount; ++i)
                        {
                            OUString aPureSourceIdStr = aPropStrings[i].copy( 1 );

                            sal_Int32 nUniqueId = xStringResourceManager->getUniqueNumericId();
                            OUString aPureIdStr = OUString::number( nUniqueId )
                                                + aIdStrBase;

                            // Set Id for all locales
                            for (auto& rLocale : aLocaleSeq)
                            {
                                OUString aResStr;
                                try
                                {
                                    aResStr = xSourceStringResolver->resolveStringForLocale
                                        ( aPureSourceIdStr, rLocale );
                                }
                                catch(const MissingResourceException&)
                                {
                                    aResStr = xSourceStringResolver->resolveStringForLocale
                                        ( aPureSourceIdStr, aDefaultLocale );
                                }
                                xStringResourceManager->setStringForLocale( aPureIdStr, aResStr, rLocale );
                            }

                            pIdStrings[i] = aEsc + aPureIdStr;
                        }
                        xPropertySet->setPropertyValue( aPropName, Any(aIdStrings) );
                    }
                    // Copy string from source to target resource
                    else if( eMode == COPY_RESOURCES && xSourceStringResolver.is() )
                    {
                        const Locale aDefaultLocale = xSourceStringResolver->getDefaultLocale();

                        for (auto& aSourceIdStr : aPropStrings)
                        {
                            OUString aPureSourceIdStr = aSourceIdStr.copy( 1 );

                            // Set Id for all locales
                            for (auto& rLocale : aLocaleSeq)
                            {
                                OUString aResStr;
                                try
                                {
                                    aResStr = xSourceStringResolver->resolveStringForLocale
                                        ( aPureSourceIdStr, rLocale );
                                }
                                catch(const MissingResourceException&)
                                {
                                    aResStr = xSourceStringResolver->resolveStringForLocale
                                        ( aPureSourceIdStr, aDefaultLocale );
                                }
                                xStringResourceManager->setStringForLocale( aPureSourceIdStr, aResStr, rLocale );
                            }
                        }
                    }
                    nChangedCount++;
                }
            }
        }
    }
    return nChangedCount;
}


void LocalizationMgr::handleAddLocales( const Sequence< Locale >& aLocaleSeq )
{
    if( isLibraryLocalized() )
    {
        for (auto& rLocale : aLocaleSeq)
        {
            m_xStringResourceManager->newLocale( rLocale );
        }
    }
    else
    {
        DBG_ASSERT( aLocaleSeq.getLength()==1, "LocalizationMgr::handleAddLocales(): Only one first locale allowed" );

        const Locale& rLocale = aLocaleSeq[0];
        m_xStringResourceManager->newLocale( rLocale );
        enableResourceForAllLibraryDialogs();
    }

    MarkDocumentModified( m_aDocument );

    // update locale toolbar
    if (SfxBindings* pBindings = GetBindingsPtr())
        pBindings->Invalidate( SID_BASICIDE_CURRENT_LANG );

    handleTranslationbar();
}


void LocalizationMgr::handleRemoveLocales( const Sequence< Locale >& aLocaleSeq )
{
    bool bConsistent = true;
    bool bModified = false;

    for (auto& rLocale : aLocaleSeq)
    {
        bool bRemove = true;

        // Check if last locale
        Sequence< Locale > aResLocaleSeq = m_xStringResourceManager->getLocales();
        if( aResLocaleSeq.getLength() == 1 )
        {
            const Locale& rLastResLocale = aResLocaleSeq[0];
            if( localesAreEqual( rLocale, rLastResLocale ) )
            {
                disableResourceForAllLibraryDialogs();
            }
            else
            {
                // Inconsistency, keep last locale
                bConsistent = false;
                bRemove = false;
            }
        }

        if( bRemove )
        {
            try
            {
                m_xStringResourceManager->removeLocale( rLocale );
                bModified = true;
            }
            catch(const IllegalArgumentException&)
            {
                bConsistent = false;
            }
        }
    }
    if( bModified )
    {
        MarkDocumentModified( m_aDocument );

        // update slots
        if (SfxBindings* pBindings = GetBindingsPtr())
        {
            pBindings->Invalidate( SID_BASICIDE_CURRENT_LANG );
            pBindings->Invalidate( SID_BASICIDE_MANAGE_LANG );
        }

        handleTranslationbar();
    }

    DBG_ASSERT( bConsistent,
        "LocalizationMgr::handleRemoveLocales(): sequence contains unsupported locales" );
}

void LocalizationMgr::handleSetDefaultLocale(const Locale& rLocale)
{
    if( !m_xStringResourceManager.is() )
        return;

    try
    {
        m_xStringResourceManager->setDefaultLocale(rLocale);
    }
    catch(const IllegalArgumentException&)
    {
        OSL_FAIL( "LocalizationMgr::handleSetDefaultLocale: Invalid locale" );
    }

    // update locale toolbar
    if (SfxBindings* pBindings = GetBindingsPtr())
        pBindings->Invalidate( SID_BASICIDE_CURRENT_LANG );
}

void LocalizationMgr::handleSetCurrentLocale(const css::lang::Locale& rLocale)
{
    if( !m_xStringResourceManager.is() )
        return;

    try
    {
        m_xStringResourceManager->setCurrentLocale(rLocale, false);
    }
    catch(const IllegalArgumentException&)
    {
        OSL_FAIL( "LocalizationMgr::handleSetCurrentLocale: Invalid locale" );
    }

    // update locale toolbar
    if (SfxBindings* pBindings = GetBindingsPtr())
        pBindings->Invalidate( SID_BASICIDE_CURRENT_LANG );

    if (DialogWindow* pDlgWin = dynamic_cast<DialogWindow*>(m_pShell->GetCurWindow()))
        if (!pDlgWin->IsSuspended())
            pDlgWin->GetEditor().UpdatePropertyBrowserDelayed();
}

void LocalizationMgr::handleBasicStarted()
{
    if( m_xStringResourceManager.is() )
        m_aLocaleBeforeBasicStart = m_xStringResourceManager->getCurrentLocale();
}

void LocalizationMgr::handleBasicStopped()
{
    try
    {
        if( m_xStringResourceManager.is() )
            m_xStringResourceManager->setCurrentLocale( m_aLocaleBeforeBasicStart, true );
    }
    catch(const IllegalArgumentException&)
    {
    }
}


static DialogWindow* FindDialogWindowForEditor( DlgEditor const * pEditor )
{
    Shell::WindowTable const& aWindowTable = GetShell()->GetWindowTable();
    for (auto const& window : aWindowTable)
    {
        BaseWindow* pWin = window.second;
        if (!pWin->IsSuspended())
            if (DialogWindow* pDlgWin = dynamic_cast<DialogWindow*>(pWin))
            {
                if (&pDlgWin->GetEditor() == pEditor)
                    return pDlgWin;
            }
    }
    return nullptr;
}


void LocalizationMgr::setControlResourceIDsForNewEditorObject( DlgEditor const * pEditor,
    const Any& rControlAny, std::u16string_view aCtrlName )
{
    // Get library for DlgEditor
    DialogWindow* pDlgWin = FindDialogWindowForEditor( pEditor );
    if( !pDlgWin )
        return;
    ScriptDocument aDocument( pDlgWin->GetDocument() );
    DBG_ASSERT( aDocument.isValid(), "LocalizationMgr::setControlResourceIDsForNewEditorObject: invalid document!" );
    if ( !aDocument.isValid() )
        return;
    const OUString& rLibName = pDlgWin->GetLibName();
    Reference< container::XNameContainer > xDialogLib( aDocument.getLibrary( E_DIALOGS, rLibName, true ) );
    Reference< XStringResourceManager > xStringResourceManager =
        LocalizationMgr::getStringResourceFromDialogLibrary( xDialogLib );

    // Set resource property
    if( !xStringResourceManager.is() || !xStringResourceManager->getLocales().hasElements() )
        return;

    OUString aDialogName = pDlgWin->GetName();
    Reference< XStringResourceResolver > xDummyStringResolver;
    sal_Int32 nChangedCount = implHandleControlResourceProperties
        ( rControlAny, aDialogName, aCtrlName, xStringResourceManager,
          xDummyStringResolver, SET_IDS );

    if( nChangedCount )
        MarkDocumentModified( aDocument );
}

void LocalizationMgr::renameControlResourceIDsForEditorObject( DlgEditor const * pEditor,
    const css::uno::Any& rControlAny, std::u16string_view aNewCtrlName )
{
    // Get library for DlgEditor
    DialogWindow* pDlgWin = FindDialogWindowForEditor( pEditor );
    if( !pDlgWin )
        return;
    ScriptDocument aDocument( pDlgWin->GetDocument() );
    DBG_ASSERT( aDocument.isValid(), "LocalizationMgr::renameControlResourceIDsForEditorObject: invalid document!" );
    if ( !aDocument.isValid() )
        return;
    const OUString& rLibName = pDlgWin->GetLibName();
    Reference< container::XNameContainer > xDialogLib( aDocument.getLibrary( E_DIALOGS, rLibName, true ) );
    Reference< XStringResourceManager > xStringResourceManager =
        LocalizationMgr::getStringResourceFromDialogLibrary( xDialogLib );

    // Set resource property
    if( !xStringResourceManager.is() || !xStringResourceManager->getLocales().hasElements() )
        return;

    OUString aDialogName = pDlgWin->GetName();
    Reference< XStringResourceResolver > xDummyStringResolver;
    implHandleControlResourceProperties
        ( rControlAny, aDialogName, aNewCtrlName, xStringResourceManager,
          xDummyStringResolver, RENAME_CONTROL_IDS );
}


void LocalizationMgr::deleteControlResourceIDsForDeletedEditorObject( DlgEditor const * pEditor,
    const Any& rControlAny, std::u16string_view aCtrlName )
{
    // Get library for DlgEditor
    DialogWindow* pDlgWin = FindDialogWindowForEditor( pEditor );
    if( !pDlgWin )
        return;
    ScriptDocument aDocument( pDlgWin->GetDocument() );
    DBG_ASSERT( aDocument.isValid(), "LocalizationMgr::deleteControlResourceIDsForDeletedEditorObject: invalid document!" );
    if ( !aDocument.isValid() )
        return;
    const OUString& rLibName = pDlgWin->GetLibName();
    Reference< container::XNameContainer > xDialogLib( aDocument.getLibrary( E_DIALOGS, rLibName, true ) );
    Reference< XStringResourceManager > xStringResourceManager =
        LocalizationMgr::getStringResourceFromDialogLibrary( xDialogLib );

    OUString aDialogName = pDlgWin->GetName();
    Reference< XStringResourceResolver > xDummyStringResolver;
    sal_Int32 nChangedCount = implHandleControlResourceProperties
        ( rControlAny, aDialogName, aCtrlName, xStringResourceManager,
          xDummyStringResolver, REMOVE_IDS_FROM_RESOURCE );

    if( nChangedCount )
        MarkDocumentModified( aDocument );
}

void LocalizationMgr::setStringResourceAtDialog( const ScriptDocument& rDocument, const OUString& aLibName,
    std::u16string_view aDlgName, const Reference< container::XNameContainer >& xDialogModel )
{
    // Get library
    Reference< container::XNameContainer > xDialogLib( rDocument.getLibrary( E_DIALOGS, aLibName, true ) );
    Reference< XStringResourceManager > xStringResourceManager =
        LocalizationMgr::getStringResourceFromDialogLibrary( xDialogLib );

    // Set resource property
    if( !xStringResourceManager.is() )
        return;

    // Not very elegant as dialog may or may not be localized yet
    // TODO: Find better place, where dialog is created
    if( xStringResourceManager->getLocales().hasElements() )
    {
        Any aDialogCtrl;
        aDialogCtrl <<= xDialogModel;
        Reference< XStringResourceResolver > xDummyStringResolver;
        implHandleControlResourceProperties( aDialogCtrl, aDlgName,
            std::u16string_view(), xStringResourceManager,
            xDummyStringResolver, SET_IDS );
    }

    Reference< beans::XPropertySet > xDlgPSet( xDialogModel, UNO_QUERY );
    xDlgPSet->setPropertyValue( u"ResourceResolver"_ustr, Any(xStringResourceManager) );
}

void LocalizationMgr::renameStringResourceIDs( const ScriptDocument& rDocument, const OUString& aLibName,
    std::u16string_view aDlgName, const Reference< container::XNameContainer >& xDialogModel )
{
    // Get library
    Reference< container::XNameContainer > xDialogLib( rDocument.getLibrary( E_DIALOGS, aLibName, true ) );
    Reference< XStringResourceManager > xStringResourceManager =
        LocalizationMgr::getStringResourceFromDialogLibrary( xDialogLib );
    if( !xStringResourceManager.is() )
        return;

    Any aDialogCtrl;
    aDialogCtrl <<= xDialogModel;
    Reference< XStringResourceResolver > xDummyStringResolver;
    implHandleControlResourceProperties( aDialogCtrl, aDlgName,
        std::u16string_view(), xStringResourceManager,
        xDummyStringResolver, RENAME_DIALOG_IDS );

    // Handle all controls
    for(const auto& rCtrlName : xDialogModel->getElementNames()) {
        Any aCtrl = xDialogModel->getByName( rCtrlName );
        implHandleControlResourceProperties( aCtrl, aDlgName,
            rCtrlName, xStringResourceManager,
            xDummyStringResolver, RENAME_DIALOG_IDS );
    }
}

void LocalizationMgr::removeResourceForDialog( const ScriptDocument& rDocument, const OUString& aLibName,
    std::u16string_view aDlgName, const Reference< container::XNameContainer >& xDialogModel )
{
    // Get library
    Reference< container::XNameContainer > xDialogLib( rDocument.getLibrary( E_DIALOGS, aLibName, true ) );
    Reference< XStringResourceManager > xStringResourceManager =
        LocalizationMgr::getStringResourceFromDialogLibrary( xDialogLib );
    if( !xStringResourceManager.is() )
        return;

    Any aDialogCtrl;
    aDialogCtrl <<= xDialogModel;
    Reference< XStringResourceResolver > xDummyStringResolver;
    implHandleControlResourceProperties( aDialogCtrl, aDlgName,
        std::u16string_view(), xStringResourceManager,
        xDummyStringResolver, REMOVE_IDS_FROM_RESOURCE );

    // Handle all controls
    for(const auto& rCtrlName : xDialogModel->getElementNames()) {
        Any aCtrl = xDialogModel->getByName( rCtrlName );
        implHandleControlResourceProperties( aCtrl, aDlgName,
            rCtrlName, xStringResourceManager,
            xDummyStringResolver, REMOVE_IDS_FROM_RESOURCE );
    }
}

void LocalizationMgr::resetResourceForDialog( const Reference< container::XNameContainer >& xDialogModel,
    const Reference< XStringResourceManager >& xStringResourceManager )
{
    if( !xStringResourceManager.is() )
        return;

    // Dialog as control
    std::u16string_view aDummyName;
    Any aDialogCtrl;
    aDialogCtrl <<= xDialogModel;
    Reference< XStringResourceResolver > xDummyStringResolver;
    implHandleControlResourceProperties( aDialogCtrl, aDummyName,
        aDummyName, xStringResourceManager, xDummyStringResolver, RESET_IDS );

    // Handle all controls
    for(const auto& rCtrlName : xDialogModel->getElementNames()){
        Any aCtrl = xDialogModel->getByName( rCtrlName );
        implHandleControlResourceProperties( aCtrl, aDummyName,
            rCtrlName, xStringResourceManager, xDummyStringResolver, RESET_IDS );
    }
}

void LocalizationMgr::setResourceIDsForDialog( const Reference< container::XNameContainer >& xDialogModel,
    const Reference< XStringResourceManager >& xStringResourceManager )
{
    if( !xStringResourceManager.is() )
        return;

    // Dialog as control
    std::u16string_view aDummyName;
    Any aDialogCtrl;
    aDialogCtrl <<= xDialogModel;
    Reference< XStringResourceResolver > xDummyStringResolver;
    implHandleControlResourceProperties( aDialogCtrl, aDummyName,
        aDummyName, xStringResourceManager, xDummyStringResolver, SET_IDS );

    // Handle all controls
    for(const auto& rCtrlName : xDialogModel->getElementNames()) {
        Any aCtrl = xDialogModel->getByName( rCtrlName );
        implHandleControlResourceProperties( aCtrl, aDummyName,
            rCtrlName, xStringResourceManager, xDummyStringResolver, SET_IDS );
    }
}

void LocalizationMgr::copyResourcesForPastedEditorObject( DlgEditor const * pEditor,
    const Any& rControlAny, std::u16string_view aCtrlName,
    const Reference< XStringResourceResolver >& xSourceStringResolver )
{
    // Get library for DlgEditor
    DialogWindow* pDlgWin = FindDialogWindowForEditor( pEditor );
    if( !pDlgWin )
        return;
    ScriptDocument aDocument( pDlgWin->GetDocument() );
    DBG_ASSERT( aDocument.isValid(), "LocalizationMgr::copyResourcesForPastedEditorObject: invalid document!" );
    if ( !aDocument.isValid() )
        return;
    const OUString& rLibName = pDlgWin->GetLibName();
    Reference< container::XNameContainer > xDialogLib( aDocument.getLibrary( E_DIALOGS, rLibName, true ) );
    Reference< XStringResourceManager > xStringResourceManager =
        LocalizationMgr::getStringResourceFromDialogLibrary( xDialogLib );

    // Set resource property
    if( !xStringResourceManager.is() || !xStringResourceManager->getLocales().hasElements() )
        return;

    OUString aDialogName = pDlgWin->GetName();
    implHandleControlResourceProperties
        ( rControlAny, aDialogName, aCtrlName, xStringResourceManager,
          xSourceStringResolver, MOVE_RESOURCES );
}

void LocalizationMgr::copyResourceForDroppedDialog( const Reference< container::XNameContainer >& xDialogModel,
    std::u16string_view aDialogName,
    const Reference< XStringResourceManager >& xStringResourceManager,
    const Reference< XStringResourceResolver >& xSourceStringResolver )
{
    if( !xStringResourceManager.is() )
        return;

    // Dialog as control
    Any aDialogCtrl;
    aDialogCtrl <<= xDialogModel;
    implHandleControlResourceProperties( aDialogCtrl, aDialogName,
        std::u16string_view(), xStringResourceManager, xSourceStringResolver, MOVE_RESOURCES );

    // Handle all controls
    for(const auto& rCtrlName : xDialogModel->getElementNames()) {
        Any aCtrl = xDialogModel->getByName( rCtrlName );
        implHandleControlResourceProperties( aCtrl, aDialogName,
            rCtrlName, xStringResourceManager, xSourceStringResolver, MOVE_RESOURCES );
    }
}

void LocalizationMgr::copyResourceForDialog(
    const Reference< container::XNameContainer >& xDialogModel,
    const Reference< XStringResourceResolver >& xSourceStringResolver,
    const Reference< XStringResourceManager >& xTargetStringResourceManager )
{
    if( !xDialogModel.is() || !xSourceStringResolver.is() || !xTargetStringResourceManager.is() )
        return;

    std::u16string_view aDummyName;
    Any aDialogCtrl;
    aDialogCtrl <<= xDialogModel;
    implHandleControlResourceProperties
        ( aDialogCtrl, aDummyName, aDummyName, xTargetStringResourceManager,
          xSourceStringResolver, COPY_RESOURCES );

    // Handle all controls
    for(const auto& rCtrlName : xDialogModel->getElementNames()) {
        Any aCtrl = xDialogModel->getByName( rCtrlName );
        implHandleControlResourceProperties( aCtrl, aDummyName, aDummyName,
            xTargetStringResourceManager, xSourceStringResolver, COPY_RESOURCES );
    }
}

Reference< XStringResourceManager > LocalizationMgr::getStringResourceFromDialogLibrary
    ( const Reference< container::XNameContainer >& xDialogLib )
{
    Reference< XStringResourceManager > xStringResourceManager;
    if( xDialogLib.is() )
    {
        Reference< resource::XStringResourceSupplier > xStringResourceSupplier( xDialogLib, UNO_QUERY );
        if( xStringResourceSupplier.is() )
        {
            Reference< resource::XStringResourceResolver >
                xStringResourceResolver = xStringResourceSupplier->getStringResource();

            xStringResourceManager =
                Reference< resource::XStringResourceManager >( xStringResourceResolver, UNO_QUERY );
        }
    }
    return xStringResourceManager;
}

} // namespace basctl

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
