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


#ifdef __sun
#include <ctime>
#endif

#include <framework/dispatchhelper.hxx>
#include <com/sun/star/frame/DispatchResultState.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <comphelper/servicehelper.hxx>
#include <svl/eitem.hxx>
#include <svl/intitem.hxx>
#include <svl/stritem.hxx>
#include <svl/visitem.hxx>
#include <svl/voiditem.hxx>

#include <sfx2/app.hxx>
#include <statcach.hxx>
#include <sfx2/msg.hxx>
#include <sfx2/ctrlitem.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/sfxuno.hxx>
#include <unoctitm.hxx>
#include <sfx2/msgpool.hxx>
#include <sfx2/viewfrm.hxx>
#include <utility>
#include <comphelper/diagnose_ex.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::util;

BindDispatch_Impl::BindDispatch_Impl( css::uno::Reference< css::frame::XDispatch > _xDisp, css::util::URL _aURL, SfxStateCache *pStateCache, const SfxSlot* pS )
    : xDisp(std::move( _xDisp ))
    , aURL(std::move( _aURL ))
    , pCache( pStateCache )
    , pSlot( pS )
{
    DBG_ASSERT( pCache && pSlot, "Invalid BindDispatch!");
    aStatus.IsEnabled = true;
}

void SAL_CALL BindDispatch_Impl::disposing( const css::lang::EventObject& )
{
    if ( xDisp.is() )
    {
        xDisp->removeStatusListener( static_cast<css::frame::XStatusListener*>(this), aURL );
        xDisp.clear();
    }
}

void SAL_CALL  BindDispatch_Impl::statusChanged( const css::frame::FeatureStateEvent& rEvent )
{
    aStatus = rEvent;
    if ( !pCache )
        return;

    css::uno::Reference< css::frame::XStatusListener >  xKeepAlive(this);
    if ( aStatus.Requery )
        pCache->Invalidate( true );
    else
    {
        std::unique_ptr<SfxPoolItem> pItem;
        sal_uInt16 nId = pCache->GetId();
        SfxItemState eState = SfxItemState::DISABLED;
        const SfxPoolItem* pArg(nullptr);
        if ( !aStatus.IsEnabled )
        {
            // default
        }
        else if (aStatus.State.hasValue())
        {
            eState = SfxItemState::DEFAULT;
            css::uno::Any aAny = aStatus.State;

            const css::uno::Type& aType = aAny.getValueType();
            if ( aType == cppu::UnoType< bool >::get() )
            {
                bool bTemp = false;
                aAny >>= bTemp ;
                pItem.reset( new SfxBoolItem( nId, bTemp ) );
            }
            else if ( aType == ::cppu::UnoType< ::cppu::UnoUnsignedShortType >::get() )
            {
                sal_uInt16 nTemp = 0;
                aAny >>= nTemp ;
                pItem.reset( new SfxUInt16Item( nId, nTemp ) );
            }
            else if ( aType == cppu::UnoType<sal_uInt32>::get() )
            {
                sal_uInt32 nTemp = 0;
                aAny >>= nTemp ;
                pItem.reset( new SfxUInt32Item( nId, nTemp ) );
            }
            else if ( aType == cppu::UnoType<OUString>::get() )
            {
                OUString sTemp ;
                aAny >>= sTemp ;
                pItem.reset( new SfxStringItem( nId, sTemp ) );
            }
            else
            {
                if ( pSlot )
                    pItem = pSlot->GetType()->CreateItem();
                if ( pItem )
                {
                    pItem->SetWhich( nId );
                    pItem->PutValue( aAny, 0 );
                }
                else
                    // tdf#162666 nId should not be zero. This will now create
                    // a SAL_INFO in SfxVoidItem::SfxVoidItem
                    pItem.reset( new SfxVoidItem( nId ) );
            }
            pArg = pItem.get();
        }
        else
        {
            // DONTCARE status
            pArg = DISABLED_POOL_ITEM;
            eState = SfxItemState::UNKNOWN;
        }

        for ( SfxControllerItem *pCtrl = pCache->GetItemLink();
            pCtrl;
            pCtrl = pCtrl->GetItemLink() )
            pCtrl->StateChangedAtToolBoxControl( nId, eState, pArg );
    }
}

void BindDispatch_Impl::Release()
{
    if ( xDisp.is() )
    {
        try
        {
            xDisp->removeStatusListener(static_cast<css::frame::XStatusListener*>(this), aURL);
        }
        catch (const lang::DisposedException&)
        {
            TOOLS_WARN_EXCEPTION("sfx", "BindDispatch_Impl::Release: xDisp is disposed: ");
        }
        xDisp.clear();
    }
    pCache = nullptr;
}


sal_Int16 BindDispatch_Impl::Dispatch( const css::uno::Sequence < css::beans::PropertyValue >& aProps, bool bForceSynchron )
{
    sal_Int16 eRet = css::frame::DispatchResultState::DONTKNOW;

    if ( xDisp.is() && aStatus.IsEnabled )
    {
        ::rtl::Reference< ::framework::DispatchHelper > xHelper( new ::framework::DispatchHelper(nullptr));
        css::uno::Any aResult = xHelper->executeDispatch(xDisp, aURL, bForceSynchron, aProps);

        css::frame::DispatchResultEvent aEvent;
        aResult >>= aEvent;

        eRet = aEvent.State;
    }

    return eRet;
}


// This constructor for an invalid cache that is updated in the first request.

SfxStateCache::SfxStateCache( sal_uInt16 nFuncId ):
    nId(nFuncId),
    pInternalController(nullptr),
    pController(nullptr),
    pLastItem( nullptr ),
    eLastState( SfxItemState::UNKNOWN ),
    bItemVisible( true )
{
    bCtrlDirty = true;
    bSlotDirty = true;
    bItemDirty = true;
}


// The Destructor checks by assertion, even if controllers are registered.

SfxStateCache::~SfxStateCache()
{
    DBG_ASSERT( pController == nullptr && pInternalController == nullptr, "there are still Controllers registered" );

    if ( !IsInvalidItem(pLastItem) && !IsDisabledItem(pLastItem) )
    {
        // tdf#162666 only delete if it *was* cloned
        delete pLastItem;
    }

    if ( mxDispatch.is() )
        mxDispatch->Release();
}


// invalidates the cache (next request will force update)
void SfxStateCache::Invalidate( bool bWithMsg )
{
    bCtrlDirty = true;
    if ( bWithMsg )
    {
        bSlotDirty = true;
        aSlotServ.SetSlot( nullptr );
        if ( mxDispatch.is() )
            mxDispatch->Release();
        mxDispatch.clear();
    }
}


// gets the corresponding function from the dispatcher or the cache

const SfxSlotServer* SfxStateCache::GetSlotServer( SfxDispatcher &rDispat , const css::uno::Reference< css::frame::XDispatchProvider > & xProv )
{

    if ( bSlotDirty )
    {
        // get the SlotServer; we need it for internal controllers anyway, but also in most cases
        rDispat.FindServer_( nId, aSlotServ );

        DBG_ASSERT( !mxDispatch.is(), "Old Dispatch not removed!" );

        // we don't need to check the dispatch provider if we only have an internal controller
        if ( xProv.is() )
        {
            const SfxSlot* pSlot = aSlotServ.GetSlot();
            if ( !pSlot )
                // get the slot - even if it is disabled on the dispatcher
                pSlot = SfxSlotPool::GetSlotPool( rDispat.GetFrame() ).GetSlot( nId );

            if ( !pSlot || pSlot->aUnoName.isEmpty() )
            {
                bSlotDirty = false;
                bCtrlDirty = true;
                return aSlotServ.GetSlot()? &aSlotServ: nullptr;
            }

            // create the dispatch URL from the slot data
            css::util::URL aURL;
            OUString aCmd = u".uno:"_ustr;
            aURL.Protocol = aCmd;
            aURL.Path = pSlot->GetUnoName();
            aCmd += aURL.Path;
            aURL.Complete = aCmd;
            aURL.Main = aCmd;

            // try to get a dispatch object for this command
            css::uno::Reference< css::frame::XDispatch >  xDisp = xProv->queryDispatch( aURL, OUString(), 0 );
            if ( xDisp.is() )
            {
                // test the dispatch object if it is just a wrapper for a SfxDispatcher
                if (auto pDisp = dynamic_cast<SfxOfficeDispatch*>(xDisp.get()))
                {
                    // The intercepting object is an SFX component
                    // If this dispatch object does not use the wanted dispatcher or the AppDispatcher, it's treated like any other UNO component
                    // (intercepting by internal dispatches)
                    SfxDispatcher *pDispatcher = pDisp->GetDispatcher_Impl();
                    if ( pDispatcher == &rDispat || pDispatcher == SfxGetpApp()->GetAppDispatcher_Impl() )
                    {
                        // so we can use it directly
                        bSlotDirty = false;
                        bCtrlDirty = true;
                        return aSlotServ.GetSlot()? &aSlotServ: nullptr;
                    }
                }

                // so the dispatch object isn't a SfxDispatcher wrapper or it is one, but it uses another dispatcher, but not rDispat
                mxDispatch = new BindDispatch_Impl( xDisp, aURL, this, pSlot );

                // flags must be set before adding StatusListener because the dispatch object will set the state
                bSlotDirty = false;
                bCtrlDirty = true;
                xDisp->addStatusListener( mxDispatch, aURL );
            }
            else if ( rDispat.GetFrame() )
            {
                css::uno::Reference < css::frame::XDispatchProvider > xFrameProv(
                        rDispat.GetFrame()->GetFrame().GetFrameInterface(), css::uno::UNO_QUERY );
                if ( xFrameProv != xProv )
                    return GetSlotServer( rDispat, xFrameProv );
            }
        }

        bSlotDirty = false;
        bCtrlDirty = true;
    }

    // we *always* return a SlotServer (if there is one); but in case of an external dispatch we might not use it
    // for the "real" (non internal) controllers
    return aSlotServ.GetSlot()? &aSlotServ: nullptr;
}


// Set Status in all Controllers

void SfxStateCache::SetState
(
    SfxItemState        eState,  // <SfxItemState> from 'pState'
    const SfxPoolItem*  pState,  // Slot Status, 0 or -1
    bool bMaybeDirty
)

/*  [Description]

    This method distributes the status of all of this SID bound
    <SfxControllerItem>s. If the value is the same as before, and if neither
    controller was registered nor invalidated inbetween, then no value is
    passed. This way the flickering is for example avoided in ListBoxes.
*/
{
    SetState_Impl( eState, pState, bMaybeDirty );
}

void SfxStateCache::GetState
(
    boost::property_tree::ptree& rState
)
{
    if ( !mxDispatch.is() && pController )
    {
        for ( SfxControllerItem *pCtrl = pController;
              pCtrl;
              pCtrl = pCtrl->GetItemLink() )
        pCtrl->GetControlState( nId, rState );
    }
}

void SfxStateCache::SetVisibleState( bool bShow )
{
    if ( bShow == bItemVisible )
        return;

    SfxItemState eState( SfxItemState::DEFAULT );
    const SfxPoolItem*  pState( nullptr );
    bool bDeleteItem( false );

    bItemVisible = bShow;
    if ( bShow )
    {
        // tdf#164745 also need to do this when disabled item
        if ( nullptr == pLastItem || IsInvalidItem(pLastItem) || IsDisabledItem(pLastItem) )
        {
            pState = new SfxVoidItem( nId );
            bDeleteItem = true;
        }
        else
            pState = pLastItem;

        eState = eLastState;
    }
    else
    {
        pState = new SfxVisibilityItem( nId, false );
        bDeleteItem = true;
    }

    // Update Controller
    if ( !mxDispatch.is() && pController )
    {
        for ( SfxControllerItem *pCtrl = pController;
                pCtrl;
                pCtrl = pCtrl->GetItemLink() )
            pCtrl->StateChangedAtToolBoxControl( nId, eState, pState );
    }

    if ( pInternalController )
        pInternalController->StateChangedAtToolBoxControl( nId, eState, pState );

    if ( bDeleteItem )
        delete pState;
}


void SfxStateCache::SetState_Impl
(
    SfxItemState        eState,  // <SfxItemState> from 'pState'
    const SfxPoolItem*  pState,  // Slot Status, 0 or -1
    bool bMaybeDirty
)
{
    // If a hard update occurs between enter- and leave-registrations is a
    // can also intermediate Cached exist without controller.
    if ( !pController && !pInternalController )
        return;

    DBG_ASSERT( bMaybeDirty || !bSlotDirty, "setting state of dirty message" );
    DBG_ASSERT( SfxControllerItem::GetItemState(pState) == eState, "invalid SfxItemState" );

    // does the controller have to be notified at all?
    bool bNotify = bItemDirty;
    if ( !bItemDirty )
    {
        bNotify = !SfxPoolItem::areSame(pLastItem, pState) || (eState != eLastState);
    }

    if ( bNotify )
    {
        // Update Controller
        if ( !mxDispatch.is() && pController )
        {
            for ( SfxControllerItem *pCtrl = pController;
                pCtrl;
                pCtrl = pCtrl->GetItemLink() )
                pCtrl->StateChangedAtToolBoxControl( nId, eState, pState );
        }

        if ( pInternalController )
            static_cast<SfxDispatchController_Impl *>(pInternalController)->StateChanged( nId, eState, pState, &aSlotServ );

        // Remember new value
        if ( !IsInvalidItem(pLastItem) && !IsDisabledItem(pLastItem) )
        {
            delete pLastItem;
            pLastItem = nullptr;
        }

        // tdf#164745 clone when it exists and it's really an item
        if ( pState && !IsInvalidItem(pState) && !IsDisabledItem(pState) )
            pLastItem = pState->Clone();
        else
            pLastItem = nullptr;

        eLastState = eState;
        bItemDirty = false;
    }

    bCtrlDirty = false;
}


// Set old status again in all the controllers

void SfxStateCache::SetCachedState( bool bAlways )
{
    DBG_ASSERT(pController==nullptr||pController->GetId()==nId, "Cache with wrong ControllerItem" );

    // Only update if cached item exists and also able to process.
    // (If the State is sent, it must be ensured that a SlotServer is present,
    // see SfxControllerItem:: GetCoreMetric())
    if ( !(bAlways || ( !bItemDirty && !bSlotDirty )) )
        return;

    // Update Controller
    if ( !mxDispatch.is() && pController )
    {
        for ( SfxControllerItem *pCtrl = pController;
            pCtrl;
            pCtrl = pCtrl->GetItemLink() )
            pCtrl->StateChangedAtToolBoxControl( nId, eLastState, pLastItem );
    }

    if ( pInternalController )
        static_cast<SfxDispatchController_Impl *>(pInternalController)->StateChanged( nId, eLastState, pLastItem, &aSlotServ );

    // Controller is now ok
    bCtrlDirty = true;
}


css::uno::Reference< css::frame::XDispatch >  SfxStateCache::GetDispatch() const
{
    if ( mxDispatch.is() )
        return mxDispatch->xDisp;
    return css::uno::Reference< css::frame::XDispatch > ();
}

sal_Int16 SfxStateCache::Dispatch( const SfxItemSet* pSet, bool bForceSynchron )
{
    // protect pDispatch against destruction in the call
    rtl::Reference<BindDispatch_Impl> xKeepAlive( mxDispatch );
    sal_Int16 eRet = css::frame::DispatchResultState::DONTKNOW;

    if ( mxDispatch.is() )
    {
        uno::Sequence < beans::PropertyValue > aArgs;
        if (pSet)
            TransformItems( nId, *pSet, aArgs );

        eRet = mxDispatch->Dispatch( aArgs, bForceSynchron );
    }

    return eRet;
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
