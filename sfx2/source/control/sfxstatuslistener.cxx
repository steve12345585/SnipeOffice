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

#include <sfx2/sfxstatuslistener.hxx>
#include <svl/poolitem.hxx>
#include <svl/eitem.hxx>
#include <svl/stritem.hxx>
#include <svl/intitem.hxx>
#include <svl/visitem.hxx>
#include <svl/voiditem.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/servicehelper.hxx>
#include <vcl/svapp.hxx>
#include <com/sun/star/util/URLTransformer.hpp>
#include <com/sun/star/util/XURLTransformer.hpp>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <com/sun/star/frame/status/ItemStatus.hpp>
#include <com/sun/star/frame/status/Visibility.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>

#include <sfx2/viewfrm.hxx>
#include <sfx2/dispatch.hxx>
#include <unoctitm.hxx>
#include <sfx2/msgpool.hxx>
#include <sfx2/msg.hxx>

using namespace ::cppu;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::frame::status;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::util;

SfxStatusListener::SfxStatusListener( const Reference< XDispatchProvider >& rDispatchProvider, sal_uInt16 nSlotId, const OUString& rCommand ) :
    m_nSlotID( nSlotId ),
    m_xDispatchProvider( rDispatchProvider )
{
    m_aCommand.Complete = rCommand;
    Reference< XURLTransformer > xTrans( URLTransformer::create( ::comphelper::getProcessComponentContext() ) );
    xTrans->parseStrict( m_aCommand );
    if ( rDispatchProvider.is() )
        m_xDispatch = rDispatchProvider->queryDispatch( m_aCommand, OUString(), 0 );
}

SfxStatusListener::~SfxStatusListener()
{
}

// old sfx controller item C++ API
void SfxStatusListener::StateChangedAtStatusListener( SfxItemState, const SfxPoolItem* )
{
    // must be implemented by sub class
}

void SfxStatusListener::UnBind()
{
    if ( m_xDispatch.is() )
    {
        Reference< XStatusListener > aStatusListener(this);
        m_xDispatch->removeStatusListener( aStatusListener, m_aCommand );
        m_xDispatch.clear();
    }
}

void SfxStatusListener::ReBind()
{
    Reference< XStatusListener > aStatusListener(this);
    if ( m_xDispatch.is() )
        m_xDispatch->removeStatusListener( aStatusListener, m_aCommand );
    if ( m_xDispatchProvider.is() )
    {
        try
        {
            m_xDispatch = m_xDispatchProvider->queryDispatch( m_aCommand, OUString(), 0 );
            if ( m_xDispatch.is() )
                m_xDispatch->addStatusListener( aStatusListener, m_aCommand );
        }
        catch( Exception& )
        {
        }
    }
}

// new UNO API
void SAL_CALL SfxStatusListener::dispose()
{
    if ( m_xDispatch.is() && !m_aCommand.Complete.isEmpty() )
    {
        try
        {
            Reference< XStatusListener > aStatusListener(this);
            m_xDispatch->removeStatusListener( aStatusListener, m_aCommand );
        }
        catch ( Exception& )
        {
        }
    }

    m_xDispatch.clear();
    m_xDispatchProvider.clear();
}

void SAL_CALL SfxStatusListener::addEventListener( const Reference< XEventListener >& )
{
    // do nothing - this is a wrapper class which does not support listeners
}

void SAL_CALL SfxStatusListener::removeEventListener( const Reference< XEventListener >& )
{
    // do nothing - this is a wrapper class which does not support listeners
}

void SAL_CALL SfxStatusListener::disposing( const EventObject& Source )
{
    SolarMutexGuard aGuard;

    if ( Source.Source == Reference< XInterface >( m_xDispatch, UNO_QUERY ))
        m_xDispatch.clear();
    else if ( Source.Source == Reference< XInterface >( m_xDispatchProvider, UNO_QUERY ))
        m_xDispatchProvider.clear();
}

void SAL_CALL SfxStatusListener::statusChanged( const FeatureStateEvent& rEvent)
{
    SolarMutexGuard aGuard;

    SfxViewFrame* pViewFrame = nullptr;
    if ( m_xDispatch.is() )
    {
        if (auto pDisp = dynamic_cast<SfxOfficeDispatch*>(m_xDispatch.get()))
            pViewFrame = pDisp->GetDispatcher_Impl()->GetFrame();
    }

    SfxSlotPool& rPool = SfxSlotPool::GetSlotPool( pViewFrame );
    const SfxSlot* pSlot = rPool.GetSlot( m_nSlotID );

    SfxItemState eState = SfxItemState::DISABLED;
    std::unique_ptr<SfxPoolItem> pItem;
    if ( rEvent.IsEnabled )
    {
        eState = SfxItemState::DEFAULT;
        css::uno::Type aType = rEvent.State.getValueType();

        if ( aType == ::cppu::UnoType<void>::get() )
        {
            pItem.reset(new SfxVoidItem( m_nSlotID ));
            eState = SfxItemState::UNKNOWN;
        }
        else if ( aType == cppu::UnoType< bool >::get() )
        {
            bool bTemp = false;
            rEvent.State >>= bTemp ;
            pItem.reset(new SfxBoolItem( m_nSlotID, bTemp ));
        }
        else if ( aType == cppu::UnoType< ::cppu::UnoUnsignedShortType >::get() )
        {
            sal_uInt16 nTemp = 0;
            rEvent.State >>= nTemp ;
            pItem.reset(new SfxUInt16Item( m_nSlotID, nTemp ));
        }
        else if ( aType == cppu::UnoType<sal_uInt32>::get() )
        {
            sal_uInt32 nTemp = 0;
            rEvent.State >>= nTemp ;
            pItem.reset(new SfxUInt32Item( m_nSlotID, nTemp ));
        }
        else if ( aType == cppu::UnoType<OUString>::get() )
        {
            OUString sTemp ;
            rEvent.State >>= sTemp ;
            pItem.reset(new SfxStringItem( m_nSlotID, sTemp ));
        }
        else if ( aType == cppu::UnoType< css::frame::status::ItemStatus >::get() )
        {
            ItemStatus aItemStatus;
            rEvent.State >>= aItemStatus;
            eState = static_cast<SfxItemState>(aItemStatus.State);
            pItem.reset(new SfxVoidItem( m_nSlotID ));
        }
        else if ( aType == cppu::UnoType< css::frame::status::Visibility >::get() )
        {
            Visibility aVisibilityStatus;
            rEvent.State >>= aVisibilityStatus;
            pItem.reset(new SfxVisibilityItem( m_nSlotID, aVisibilityStatus.bVisible ));
        }
        else
        {
            if ( pSlot )
                pItem = pSlot->GetType()->CreateItem();
            if ( pItem )
            {
                pItem->SetWhich( m_nSlotID );
                pItem->PutValue( rEvent.State, 0 );
            }
            else
                pItem.reset(new SfxVoidItem( m_nSlotID ));
        }
    }

    StateChangedAtStatusListener( eState, pItem.get() );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
