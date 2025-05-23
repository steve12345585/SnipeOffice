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

#include <uielement/togglebuttontoolbarcontroller.hxx>

#include <comphelper/propertyvalue.hxx>
#include <vcl/svapp.hxx>
#include <vcl/toolbox.hxx>
#include <vcl/menu.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::awt;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::frame;

namespace framework
{

ToggleButtonToolbarController::ToggleButtonToolbarController(
    const Reference< XComponentContext >&    rxContext,
    const Reference< XFrame >&               rFrame,
    ToolBox*                                 pToolbar,
    ToolBoxItemId                            nID,
    Style                                    eStyle,
    const OUString&                          aCommand ) :
    ComplexToolbarController( rxContext, rFrame, pToolbar, nID, aCommand )
{
    if ( eStyle == Style::DropDownButton )
        m_xToolbar->SetItemBits( m_nID, ToolBoxItemBits::DROPDOWNONLY | m_xToolbar->GetItemBits( m_nID ) );
    else // Style::ToggleDropDownButton
        m_xToolbar->SetItemBits( m_nID, ToolBoxItemBits::DROPDOWN | m_xToolbar->GetItemBits( m_nID ) );
}

ToggleButtonToolbarController::~ToggleButtonToolbarController()
{
}

void SAL_CALL ToggleButtonToolbarController::dispose()
{
    SolarMutexGuard aSolarMutexGuard;
    ComplexToolbarController::dispose();
}

Sequence<PropertyValue> ToggleButtonToolbarController::getExecuteArgs(sal_Int16 KeyModifier) const
{
    Sequence<PropertyValue> aArgs{ // Add key modifier to argument list
                                   comphelper::makePropertyValue(u"KeyModifier"_ustr, KeyModifier),
                                   comphelper::makePropertyValue(u"Text"_ustr, m_aCurrentSelection) };
    return aArgs;
}

uno::Reference< awt::XWindow > SAL_CALL ToggleButtonToolbarController::createPopupWindow()
{
    uno::Reference< awt::XWindow > xWindow;

    SolarMutexGuard aSolarMutexGuard;

    // create popup menu
    ScopedVclPtrInstance<::PopupMenu> aPopup;
    const sal_uInt32 nCount = m_aDropdownMenuList.size();
    for ( sal_uInt32 i = 0; i < nCount; i++ )
    {
        const OUString & rLabel = m_aDropdownMenuList[i].mLabel;
        aPopup->InsertItem( sal_uInt16( i+1 ), rLabel );
        if ( rLabel == m_aCurrentSelection )
            aPopup->CheckItem( sal_uInt16( i+1 ) );
        else
            aPopup->CheckItem( sal_uInt16( i+1 ), false );

        if ( !m_aDropdownMenuList[i].mTipHelpText.isEmpty() )
            aPopup->SetTipHelpText( sal_uInt16( i+1 ), m_aDropdownMenuList[i].mTipHelpText );
    }

    m_xToolbar->SetItemDown( m_nID, true );
    aPopup->SetSelectHdl( LINK( this, ToggleButtonToolbarController, MenuSelectHdl ));
    aPopup->Execute( m_xToolbar, m_xToolbar->GetItemRect( m_nID ));
    m_xToolbar->SetItemDown( m_nID, false );

    return xWindow;
}

void ToggleButtonToolbarController::executeControlCommand( const css::frame::ControlCommand& rControlCommand )
{
    SolarMutexGuard aSolarMutexGuard;

    if ( rControlCommand.Command == "SetList" )
    {
        for ( auto const & arg : rControlCommand.Arguments )
        {
            if ( arg.Name == "List" )
            {
                Sequence< OUString > aList;
                m_aDropdownMenuList.clear();
                m_aCurrentSelection.clear();

                arg.Value >>= aList;
                for (OUString const& label : aList)
                {
                    m_aDropdownMenuList.push_back( DropdownMenuItem() );
                    m_aDropdownMenuList.back().mLabel = label;
                }

                // send notification
                uno::Sequence< beans::NamedValue > aInfo { { u"List"_ustr, css::uno::Any(aList) } };
                addNotifyInfo( u"ListChanged"_ustr,
                            getDispatchFromCommand( m_aCommandURL ),
                            aInfo );

                break;
            }
        }
    }
    else if ( rControlCommand.Command == "CheckItemPos" )
    {
        for ( auto const & arg : rControlCommand.Arguments )
        {
            if ( arg.Name == "Pos" )
            {
                sal_Int32 nPos( -1 );

                arg.Value >>= nPos;
                if ( nPos >= 0 &&
                     ( sal::static_int_cast< sal_uInt32 >(nPos)
                       < m_aDropdownMenuList.size() ) )
                {
                    m_aCurrentSelection = m_aDropdownMenuList[nPos].mLabel;

                    // send notification
                    uno::Sequence< beans::NamedValue > aInfo { { u"ItemChecked"_ustr, css::uno::Any(nPos) } };
                    addNotifyInfo( u"Pos"_ustr,
                                getDispatchFromCommand( m_aCommandURL ),
                                aInfo );
                }
                break;
            }
        }
    }
    else if ( rControlCommand.Command == "AddEntry" )
    {
        OUString   aText;
        OUString   aTipHelpText;

        for ( auto const & arg : rControlCommand.Arguments )
        {
            if ( arg.Name == "Text" )
            {
                arg.Value >>= aText;
            }
            else if ( arg.Name == "TipHelpText" )
            {
                arg.Value >>= aTipHelpText;
            }
        }

        if (!aText.isEmpty())
        {
            m_aDropdownMenuList.push_back( DropdownMenuItem() );
            m_aDropdownMenuList.back().mLabel = aText;
            m_aDropdownMenuList.back().mTipHelpText = aTipHelpText;
        }
    }
    else if ( rControlCommand.Command == "InsertEntry" )
    {
        sal_Int32 nPos(0);
        sal_Int32 nSize = sal_Int32( m_aDropdownMenuList.size() );
        OUString  aText;
        for ( auto const & arg : rControlCommand.Arguments )
        {
            if ( arg.Name == "Pos" )
            {
                sal_Int32 nTmpPos = 0;
                if ( arg.Value >>= nTmpPos )
                {
                    if (( nTmpPos >= 0 ) && ( nTmpPos < nSize ))
                        nPos = nTmpPos;
                }
            }
            else if ( arg.Name == "Text" )
                arg.Value >>= aText;
        }

        std::vector< DropdownMenuItem >::iterator aIter = m_aDropdownMenuList.begin();
        aIter += nPos;
        aIter = m_aDropdownMenuList.insert(aIter, DropdownMenuItem());
        if (aIter != m_aDropdownMenuList.end())
            aIter->mLabel = aText;
    }
    else if ( rControlCommand.Command == "RemoveEntryPos" )
    {
        for ( auto const & arg : rControlCommand.Arguments )
        {
            if ( arg.Name == "Pos" )
            {
                sal_Int32 nPos( -1 );
                if ( arg.Value >>= nPos )
                {
                    if ( nPos < sal_Int32( m_aDropdownMenuList.size() ))
                    {
                        m_aDropdownMenuList.erase(m_aDropdownMenuList.begin() + nPos);
                    }
                }
                break;
            }
        }
    }
    else if ( rControlCommand.Command == "RemoveEntryText" )
    {
        for ( auto const & arg : rControlCommand.Arguments )
        {
            if ( arg.Name == "Text" )
            {
                OUString aText;
                if ( arg.Value >>= aText )
                {
                    sal_Int32 nSize = sal_Int32( m_aDropdownMenuList.size() );
                    for ( sal_Int32 j = 0; j < nSize; j++ )
                    {
                        if ( m_aDropdownMenuList[j].mLabel == aText )
                        {
                            m_aDropdownMenuList.erase(m_aDropdownMenuList.begin() + j);
                            break;
                        }
                    }
                }
                break;
            }
        }
    }
    else if ( rControlCommand.Command == "createPopupMenu" )
    {
        createPopupWindow();
    }
}

IMPL_LINK( ToggleButtonToolbarController, MenuSelectHdl, Menu *, pMenu, bool )
{
    SolarMutexGuard aGuard;

    sal_uInt16 nItemId = pMenu->GetCurItemId();
    if ( nItemId > 0 && nItemId <= m_aDropdownMenuList.size() )
    {
        m_aCurrentSelection = m_aDropdownMenuList[nItemId-1].mLabel;

        execute( 0 );
    }
    return false;
}

} // namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
