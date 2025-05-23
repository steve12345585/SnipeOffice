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

#include <accessibility/vclxaccessiblemenuitem.hxx>
#include <comphelper/accessiblecontexthelper.hxx>

#include <comphelper/accessiblekeybindinghelper.hxx>
#include <com/sun/star/awt/KeyModifier.hpp>

#include <com/sun/star/accessibility/AccessibleRole.hpp>
#include <com/sun/star/accessibility/AccessibleStateType.hpp>
#include <com/sun/star/datatransfer/clipboard/XClipboard.hpp>
#include <com/sun/star/datatransfer/clipboard/XFlushableClipboard.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <comphelper/sequence.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <vcl/accessibility/characterattributeshelper.hxx>
#include <vcl/accessibility/strings.hxx>
#include <vcl/event.hxx>
#include <vcl/svapp.hxx>
#include <vcl/window.hxx>
#include <vcl/menu.hxx>
#include <vcl/unohelp.hxx>
#include <vcl/unohelp2.hxx>
#include <vcl/settings.hxx>

using namespace ::com::sun::star::accessibility;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star;
using namespace ::comphelper;




VCLXAccessibleMenuItem::VCLXAccessibleMenuItem( Menu* pParent, sal_uInt16 nItemPos, Menu* pMenu )
    :ImplInheritanceHelper( pParent, nItemPos, pMenu )
{
}


bool VCLXAccessibleMenuItem::IsFocused()
{
    return IsHighlighted();
}


bool VCLXAccessibleMenuItem::IsSelected()
{
    return IsHighlighted();
}

bool VCLXAccessibleMenuItem::IsCheckable()
{
    if (!m_pParent)
        return false;

    const sal_uInt16 nItemId = m_pParent->GetItemId(m_nItemPos);
    return m_pParent->IsItemCheckable(nItemId);
}

bool VCLXAccessibleMenuItem::IsChecked()
{
    bool bChecked = false;

    if ( m_pParent )
    {
        sal_uInt16 nItemId = m_pParent->GetItemId( m_nItemPos );
        if ( m_pParent->IsItemChecked( nItemId ) )
            bChecked = true;
    }

    return bChecked;
}


bool VCLXAccessibleMenuItem::IsHighlighted()
{
    bool bHighlighted = false;

    if ( m_pParent && m_pParent->IsHighlighted( m_nItemPos ) )
        bHighlighted = true;

    return bHighlighted;
}


void VCLXAccessibleMenuItem::FillAccessibleStateSet( sal_Int64& rStateSet )
{
    OAccessibleMenuItemComponent::FillAccessibleStateSet( rStateSet );

    rStateSet |= AccessibleStateType::FOCUSABLE;

    if ( IsFocused() )
        rStateSet |= AccessibleStateType::FOCUSED;

    rStateSet |= AccessibleStateType::SELECTABLE;

    if ( IsSelected() )
        rStateSet |= AccessibleStateType::SELECTED;

    if (IsCheckable())
        rStateSet |= AccessibleStateType::CHECKABLE;
    if ( IsChecked() )
        rStateSet |= AccessibleStateType::CHECKED;
}


// OCommonAccessibleText


OUString VCLXAccessibleMenuItem::implGetText()
{
    return m_sItemText;
}


Locale VCLXAccessibleMenuItem::implGetLocale()
{
    return Application::GetSettings().GetLanguageTag().getLocale();
}


void VCLXAccessibleMenuItem::implGetSelection( sal_Int32& nStartIndex, sal_Int32& nEndIndex )
{
    nStartIndex = 0;
    nEndIndex = 0;
}


// XServiceInfo


OUString VCLXAccessibleMenuItem::getImplementationName()
{
    return u"com.sun.star.comp.toolkit.AccessibleMenuItem"_ustr;
}


Sequence< OUString > VCLXAccessibleMenuItem::getSupportedServiceNames()
{
    return { u"com.sun.star.awt.AccessibleMenuItem"_ustr };
}


// XAccessibleContext


sal_Int16 VCLXAccessibleMenuItem::getAccessibleRole(  )
{
    OExternalLockGuard aGuard( this );
    // IA2 CWS. MT: We had the additional roles in UAA for ever, but never used them anywhere.
    // Looks reasonable, but need to verify in Orca and VoiceOver.
    sal_Int16 nRole = AccessibleRole::MENU_ITEM;
    if ( m_pParent )
    {
        sal_uInt16 nItemId = m_pParent->GetItemId( m_nItemPos );
        MenuItemBits nItemBits = m_pParent->GetItemBits(nItemId);
        if(  nItemBits & MenuItemBits::RADIOCHECK)
            nRole = AccessibleRole::RADIO_MENU_ITEM;
        else if( nItemBits & MenuItemBits::CHECKABLE)
            nRole = AccessibleRole::CHECK_MENU_ITEM;
    }
    return nRole;
}


// XAccessibleText


sal_Int32 VCLXAccessibleMenuItem::getCaretPosition()
{
    return -1;
}


sal_Bool VCLXAccessibleMenuItem::setCaretPosition( sal_Int32 nIndex )
{

    OExternalLockGuard aGuard( this );

    if ( !implIsValidRange( nIndex, nIndex, m_sItemText.getLength() ) )
        throw IndexOutOfBoundsException();

    return false;
}


sal_Unicode VCLXAccessibleMenuItem::getCharacter( sal_Int32 nIndex )
{
    OExternalLockGuard aGuard( this );

    return OCommonAccessibleText::implGetCharacter( implGetText(), nIndex );
}


Sequence< PropertyValue > VCLXAccessibleMenuItem::getCharacterAttributes( sal_Int32 nIndex, const Sequence< OUString >& aRequestedAttributes )
{
    OExternalLockGuard aGuard( this );

    if ( !implIsValidIndex( nIndex, m_sItemText.getLength() ) )
        throw IndexOutOfBoundsException();

    vcl::Font aFont = Application::GetSettings().GetStyleSettings().GetMenuFont();
    sal_Int32 nBackColor = getBackground();
    sal_Int32 nColor = getForeground();
    return CharacterAttributesHelper( aFont, nBackColor, nColor )
        .GetCharacterAttributes( aRequestedAttributes );
}


awt::Rectangle VCLXAccessibleMenuItem::getCharacterBounds( sal_Int32 nIndex )
{
    OExternalLockGuard aGuard( this );

    if ( !implIsValidIndex( nIndex, m_sItemText.getLength() ) )
        throw IndexOutOfBoundsException();

    awt::Rectangle aBounds( 0, 0, 0, 0 );
    if ( m_pParent )
    {
        sal_uInt16 nItemId = m_pParent->GetItemId( m_nItemPos );
        tools::Rectangle aItemRect = m_pParent->GetBoundingRectangle( m_nItemPos );
        tools::Rectangle aCharRect = m_pParent->GetCharacterBounds( nItemId, nIndex );
        aCharRect.Move( -aItemRect.Left(), -aItemRect.Top() );
        aBounds = vcl::unohelper::ConvertToAWTRect(aCharRect);
    }

    return aBounds;
}


sal_Int32 VCLXAccessibleMenuItem::getCharacterCount()
{
    OExternalLockGuard aGuard( this );

    return m_sItemText.getLength();
}


sal_Int32 VCLXAccessibleMenuItem::getIndexAtPoint( const awt::Point& aPoint )
{
    OExternalLockGuard aGuard( this );

    sal_Int32 nIndex = -1;
    if ( m_pParent )
    {
        sal_uInt16 nItemId = 0;
        tools::Rectangle aItemRect = m_pParent->GetBoundingRectangle( m_nItemPos );
        Point aPnt(vcl::unohelper::ConvertToVCLPoint(aPoint));
        aPnt += aItemRect.TopLeft();
        sal_Int32 nI = m_pParent->GetIndexForPoint( aPnt, nItemId );
        if ( nI != -1 && m_pParent->GetItemId( m_nItemPos ) == nItemId )
            nIndex = nI;
    }

    return nIndex;
}


OUString VCLXAccessibleMenuItem::getSelectedText()
{
    OExternalLockGuard aGuard( this );

    return OUString();
}


sal_Int32 VCLXAccessibleMenuItem::getSelectionStart()
{
    OExternalLockGuard aGuard( this );

    return 0;
}


sal_Int32 VCLXAccessibleMenuItem::getSelectionEnd()
{
    OExternalLockGuard aGuard( this );

    return 0;
}


sal_Bool VCLXAccessibleMenuItem::setSelection( sal_Int32 nStartIndex, sal_Int32 nEndIndex )
{
    OExternalLockGuard aGuard( this );

    if ( !implIsValidRange( nStartIndex, nEndIndex, m_sItemText.getLength() ) )
        throw IndexOutOfBoundsException();

    return false;
}


OUString VCLXAccessibleMenuItem::getText()
{
    OExternalLockGuard aGuard( this );

    return m_sItemText;
}


OUString VCLXAccessibleMenuItem::getTextRange( sal_Int32 nStartIndex, sal_Int32 nEndIndex )
{
    OExternalLockGuard aGuard( this );

    return OCommonAccessibleText::implGetTextRange( implGetText(), nStartIndex, nEndIndex );
}


css::accessibility::TextSegment VCLXAccessibleMenuItem::getTextAtIndex( sal_Int32 nIndex, sal_Int16 aTextType )
{
    OExternalLockGuard aGuard( this );

    return OCommonAccessibleText::getTextAtIndex( nIndex, aTextType );
}


css::accessibility::TextSegment VCLXAccessibleMenuItem::getTextBeforeIndex( sal_Int32 nIndex, sal_Int16 aTextType )
{
    OExternalLockGuard aGuard( this );

    return OCommonAccessibleText::getTextBeforeIndex( nIndex, aTextType );
}


css::accessibility::TextSegment VCLXAccessibleMenuItem::getTextBehindIndex( sal_Int32 nIndex, sal_Int16 aTextType )
{
    OExternalLockGuard aGuard( this );

    return OCommonAccessibleText::getTextBehindIndex( nIndex, aTextType );
}


sal_Bool VCLXAccessibleMenuItem::copyText( sal_Int32 nStartIndex, sal_Int32 nEndIndex )
{
    OExternalLockGuard aGuard( this );

    bool bReturn = false;

    if ( m_pParent )
    {
        vcl::Window* pWindow = m_pParent->GetWindow();
        if ( pWindow )
        {
            Reference< datatransfer::clipboard::XClipboard > xClipboard = pWindow->GetClipboard();
            if ( xClipboard.is() )
            {
                OUString sText( getTextRange( nStartIndex, nEndIndex ) );

                rtl::Reference<vcl::unohelper::TextDataObject> pDataObj = new vcl::unohelper::TextDataObject( sText );

                SolarMutexReleaser aReleaser;
                xClipboard->setContents( pDataObj, nullptr );
                Reference< datatransfer::clipboard::XFlushableClipboard > xFlushableClipboard( xClipboard, uno::UNO_QUERY );
                if( xFlushableClipboard.is() )
                    xFlushableClipboard->flushClipboard();

                bReturn = true;
            }
        }
    }

    return bReturn;
}

sal_Bool VCLXAccessibleMenuItem::scrollSubstringTo( sal_Int32, sal_Int32, AccessibleScrollType )
{
    return false;
}


// XAccessibleAction


sal_Int32 VCLXAccessibleMenuItem::getAccessibleActionCount( )
{
    return 1;
}


sal_Bool VCLXAccessibleMenuItem::doAccessibleAction ( sal_Int32 nIndex )
{
    OExternalLockGuard aGuard( this );

    if ( nIndex != 0 )
        throw IndexOutOfBoundsException();

    Click();

    return true;
}


OUString VCLXAccessibleMenuItem::getAccessibleActionDescription ( sal_Int32 nIndex )
{
    OExternalLockGuard aGuard( this );

    if ( nIndex != 0 )
        throw IndexOutOfBoundsException();

    return RID_STR_ACC_ACTION_SELECT;
}


Reference< XAccessibleKeyBinding > VCLXAccessibleMenuItem::getAccessibleActionKeyBinding( sal_Int32 nIndex )
{
    OExternalLockGuard aGuard( this );

    if ( nIndex != 0 )
        throw IndexOutOfBoundsException();

    rtl::Reference<OAccessibleKeyBindingHelper> pKeyBindingHelper = new OAccessibleKeyBindingHelper();

    if ( m_pParent )
    {
        // create auto mnemonics
        if (!(m_pParent->GetMenuFlags() & MenuFlags::NoAutoMnemonics))
            m_pParent->CreateAutoMnemonics();

        // activation key
        KeyEvent aKeyEvent = m_pParent->GetActivationKey( m_pParent->GetItemId( m_nItemPos ) );
        vcl::KeyCode aKeyCode = aKeyEvent.GetKeyCode();
        Sequence< awt::KeyStroke > aSeq1
        {
            {
                0, // Modifiers
                static_cast< sal_Int16 >(aKeyCode.GetCode()),
                aKeyEvent.GetCharCode(),
                static_cast< sal_Int16 >( aKeyCode.GetFunction())
            }
        };
        Reference< XAccessible > xParent( getAccessibleParent() );
        if ( xParent.is() )
        {
            Reference< XAccessibleContext > xParentContext( xParent->getAccessibleContext() );
            if ( xParentContext.is() && xParentContext->getAccessibleRole() == AccessibleRole::MENU_BAR )
                aSeq1.getArray()[0].Modifiers |= awt::KeyModifier::MOD2;
        }
        pKeyBindingHelper->AddKeyBinding( aSeq1 );

        // complete menu activation key sequence
        Sequence< awt::KeyStroke > aSeq;
        if ( xParent.is() )
        {
            Reference< XAccessibleContext > xParentContext( xParent->getAccessibleContext() );
            if ( xParentContext.is() && xParentContext->getAccessibleRole() == AccessibleRole::MENU )
            {
                Reference< XAccessibleAction > xAction( xParentContext, UNO_QUERY );
                if ( xAction.is() && xAction->getAccessibleActionCount() > 0 )
                {
                    Reference< XAccessibleKeyBinding > xKeyB( xAction->getAccessibleActionKeyBinding( 0 ) );
                    if ( xKeyB.is() && xKeyB->getAccessibleKeyBindingCount() > 1 )
                        aSeq = xKeyB->getAccessibleKeyBinding( 1 );
                }
            }
        }
        Sequence< awt::KeyStroke > aSeq2 = ::comphelper::concatSequences( aSeq, aSeq1 );
        pKeyBindingHelper->AddKeyBinding( aSeq2 );

        // accelerator key
        vcl::KeyCode aAccelKeyCode = m_pParent->GetAccelKey( m_pParent->GetItemId( m_nItemPos ) );
        if ( aAccelKeyCode.GetCode() != 0 )
        {
            Sequence< awt::KeyStroke > aSeq3
            {
                {
                    0, // Modifiers
                    static_cast< sal_Int16 >(aAccelKeyCode.GetCode()),
                    aKeyEvent.GetCharCode(),
                    static_cast< sal_Int16 >(aAccelKeyCode.GetFunction())
                }
            };
            if (aAccelKeyCode.GetModifier() != 0)
            {
                auto pSeq3 = aSeq3.getArray();
                if ( aAccelKeyCode.IsShift() )
                    pSeq3[0].Modifiers |= awt::KeyModifier::SHIFT;
                if ( aAccelKeyCode.IsMod1() )
                    pSeq3[0].Modifiers |= awt::KeyModifier::MOD1;
                if ( aAccelKeyCode.IsMod2() )
                    pSeq3[0].Modifiers |= awt::KeyModifier::MOD2;
                if ( aAccelKeyCode.IsMod3() )
                    pSeq3[0].Modifiers |= awt::KeyModifier::MOD3;
            }
            pKeyBindingHelper->AddKeyBinding( aSeq3 );
        }
    }

    return pKeyBindingHelper;
}


// XAccessibleValue


Any VCLXAccessibleMenuItem::getCurrentValue(  )
{
    OExternalLockGuard aGuard( this );

    Any aValue;
    if ( IsSelected() )
        aValue <<= sal_Int32(1);
    else
        aValue <<= sal_Int32(0);

    return aValue;
}


sal_Bool VCLXAccessibleMenuItem::setCurrentValue( const Any& aNumber )
{
    OExternalLockGuard aGuard( this );

    bool bReturn = false;
    sal_Int32 nValue = 0;
    OSL_VERIFY( aNumber >>= nValue );

    if ( nValue <= 0 )
    {
        DeSelect();
        bReturn = true;
    }
    else if ( nValue >= 1 )
    {
        Select();
        bReturn = true;
    }

    return bReturn;
}


Any VCLXAccessibleMenuItem::getMaximumValue(  )
{
    Any aValue;
    aValue <<= sal_Int32(1);

    return aValue;
}


Any VCLXAccessibleMenuItem::getMinimumValue(  )
{
    Any aValue;
    aValue <<= sal_Int32(0);

    return aValue;
}

Any VCLXAccessibleMenuItem::getMinimumIncrement( )
{
    Any aValue;
    aValue <<= sal_Int32(1);

    return aValue;
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
