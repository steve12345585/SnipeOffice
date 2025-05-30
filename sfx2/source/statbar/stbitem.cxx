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

#include <svl/stritem.hxx>
#include <svl/voiditem.hxx>
#include <com/sun/star/util/URL.hpp>
#include <com/sun/star/util/URLTransformer.hpp>
#include <com/sun/star/util/XURLTransformer.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <com/sun/star/frame/status/ItemStatus.hpp>
#include <com/sun/star/awt/MouseButton.hpp>

#include <vcl/commandevent.hxx>
#include <vcl/event.hxx>
#include <vcl/status.hxx>
#include <vcl/svapp.hxx>
#include <vcl/unohelp.hxx>

#include <sfx2/app.hxx>
#include <sfx2/stbitem.hxx>
#include <sfx2/msg.hxx>
#include <sfx2/msgpool.hxx>
#include <sfx2/module.hxx>
#include <sfx2/dispatch.hxx>
#include <unoctitm.hxx>
#include <sfx2/objsh.hxx>

#include <comphelper/processfactory.hxx>
#include <comphelper/servicehelper.hxx>
#include <svl/eitem.hxx>
#include <svl/intitem.hxx>
#include <toolkit/helper/vclunohelper.hxx>

using namespace ::com::sun::star;


sal_uInt16 SfxStatusBarControl::convertAwtToVCLMouseButtons( sal_Int16 nAwtMouseButtons )
{
    sal_uInt16 nVCLMouseButtons( 0 );

    if ( nAwtMouseButtons & awt::MouseButton::LEFT )
        nVCLMouseButtons |= MOUSE_LEFT;
    if ( nAwtMouseButtons & awt::MouseButton::RIGHT )
        nVCLMouseButtons |= MOUSE_RIGHT;
    if ( nAwtMouseButtons & awt::MouseButton::MIDDLE )
        nVCLMouseButtons |= MOUSE_MIDDLE;

    return nVCLMouseButtons;
}


rtl::Reference<svt::StatusbarController> SfxStatusBarControllerFactory(
    const uno::Reference< frame::XFrame >& rFrame,
    StatusBar* pStatusBar,
    unsigned short nID,
    const OUString& aCommandURL )
{
    SolarMutexGuard aGuard;

    util::URL aTargetURL;
    aTargetURL.Complete = aCommandURL;
    uno::Reference < util::XURLTransformer > xTrans( util::URLTransformer::create( ::comphelper::getProcessComponentContext() ) );
    xTrans->parseStrict( aTargetURL );

    uno::Reference < frame::XController > xController;
    uno::Reference < frame::XModel > xModel;
    if ( rFrame.is() )
    {
        xController = rFrame->getController();
        if ( xController.is() )
            xModel = xController->getModel();
    }

    SfxObjectShell* pObjShell = SfxObjectShell::GetShellFromComponent(xModel);

    SfxModule*     pModule   = pObjShell ? pObjShell->GetModule() : nullptr;
    SfxSlotPool*   pSlotPool = nullptr;

    if ( pModule )
        pSlotPool = pModule->GetSlotPool();
    else
        pSlotPool = &(SfxSlotPool::GetSlotPool());

    const SfxSlot* pSlot = pSlotPool->GetUnoSlot( aTargetURL.Path );
    if ( pSlot )
    {
        sal_uInt16 nSlotId = pSlot->GetSlotId();
        if ( nSlotId > 0 )
        {
            OUString aCmd = pSlot->GetCommand();
            pStatusBar->SetHelpId( nSlotId, aCmd );
            return SfxStatusBarControl::CreateControl( nSlotId, nID, pStatusBar, pModule );
        }
    }

    return nullptr;
}


SfxStatusBarControl::SfxStatusBarControl
(
    sal_uInt16      nSlotID,            /* Slot-Id which is connected to this
                                       instance. If a Slot-Id is set to != 0 at
                                       registration it will always be set there.
                                    */


    sal_uInt16      nCtrlID,            /* ID of this controller in the status bar */

    StatusBar&  rBar                /* Reference to the StatusBar,for which
                                       this Control was created. */
)


/*  [Description]

    Constructor of the SfxStatusBarControl Class. The Subclasses are
    created at the Factory if necessary.

    Instances of this base class are created for all StatusBar-fields
    for which no specific ones have been registered.
*/

:   nSlotId( nSlotID ),
    nId( nCtrlID ),
    pBar( &rBar )
{
}


SfxStatusBarControl::~SfxStatusBarControl()

/*  [Description]

    Destructor of the SfxStatusBarControl Class. The Class and its Subclasses
    are destroyed by SFx.
*/

{}


// XInterface
void SAL_CALL SfxStatusBarControl::acquire() noexcept
{
    OWeakObject::acquire();
}

void SAL_CALL SfxStatusBarControl::release() noexcept
{
    OWeakObject::release();
}


// XStatusListener
void SAL_CALL SfxStatusBarControl::statusChanged( const frame::FeatureStateEvent& rEvent )
{
    SfxViewFrame* pViewFrame = nullptr;
    uno::Reference < frame::XController > xController;

    SolarMutexGuard aGuard;
    if ( m_xFrame.is() )
        xController = m_xFrame->getController();

    uno::Reference < frame::XDispatchProvider > xProvider( xController, uno::UNO_QUERY );
    if ( xProvider.is() )
    {
        uno::Reference < frame::XDispatch > xDisp = xProvider->queryDispatch( rEvent.FeatureURL, OUString(), 0 );
        if ( xDisp.is() )
        {
            if (auto pDisp = dynamic_cast<SfxOfficeDispatch*>(xDisp.get()))
                pViewFrame = pDisp->GetDispatcher_Impl()->GetFrame();
        }
    }

    sal_uInt16 nSlotID = 0;
    SfxSlotPool& rPool = SfxSlotPool::GetSlotPool( pViewFrame );
    const SfxSlot* pSlot = rPool.GetUnoSlot( rEvent.FeatureURL.Path );
    if ( pSlot )
        nSlotID = pSlot->GetSlotId();

    if ( nSlotID <= 0 )
        return;

    if ( rEvent.Requery )
        svt::StatusbarController::statusChanged( rEvent );
    else
    {
        SfxItemState eState = SfxItemState::DISABLED;
        std::unique_ptr<SfxPoolItem> pItem;
        if ( rEvent.IsEnabled )
        {
            eState = SfxItemState::DEFAULT;
            uno::Type aType = rEvent.State.getValueType();

            if ( aType == cppu::UnoType<void>::get() )
            {
                pItem.reset( new SfxVoidItem( nSlotID ) );
                eState = SfxItemState::UNKNOWN;
            }
            else if ( aType == cppu::UnoType<bool>::get() )
            {
                bool bTemp = false;
                rEvent.State >>= bTemp ;
                pItem.reset( new SfxBoolItem( nSlotID, bTemp ) );
            }
            else if ( aType == ::cppu::UnoType< ::cppu::UnoUnsignedShortType >::get() )
            {
                sal_uInt16 nTemp = 0;
                rEvent.State >>= nTemp ;
                pItem.reset( new SfxUInt16Item( nSlotID, nTemp ) );
            }
            else if ( aType == cppu::UnoType<sal_uInt32>::get() )
            {
                sal_uInt32 nTemp = 0;
                rEvent.State >>= nTemp ;
                pItem.reset( new SfxUInt32Item( nSlotID, nTemp ) );
            }
            else if ( aType == cppu::UnoType<OUString>::get() )
            {
                OUString sTemp ;
                rEvent.State >>= sTemp ;
                pItem.reset( new SfxStringItem( nSlotID, sTemp ) );
            }
            else if ( aType == cppu::UnoType< css::frame::status::ItemStatus>::get() )
            {
                frame::status::ItemStatus aItemStatus;
                rEvent.State >>= aItemStatus;
                eState = static_cast<SfxItemState>(aItemStatus.State);
                pItem.reset( new SfxVoidItem( nSlotID ) );
            }
            else
            {
                if ( pSlot )
                    pItem = pSlot->GetType()->CreateItem();
                if ( pItem )
                {
                    pItem->SetWhich( nSlotID );
                    pItem->PutValue( rEvent.State, 0 );
                }
                else
                    pItem.reset( new SfxVoidItem( nSlotID ) );
            }
        }

        StateChangedAtStatusBarControl( nSlotID, eState, pItem.get() );
    }
}

// XStatusbarController
sal_Bool SAL_CALL SfxStatusBarControl::mouseButtonDown(
    const awt::MouseEvent& rMouseEvent )
{
    SolarMutexGuard aGuard;
    ::Point aPos( rMouseEvent.X, rMouseEvent.Y );

    ::MouseEvent aMouseEvent( aPos,
                              static_cast<sal_uInt16>(rMouseEvent.ClickCount),
                              MouseEventModifiers::NONE,
                              convertAwtToVCLMouseButtons( rMouseEvent.Buttons ),
                              0 );

    return MouseButtonDown( aMouseEvent );
}

sal_Bool SAL_CALL SfxStatusBarControl::mouseMove(
    const awt::MouseEvent& rMouseEvent )
{
    SolarMutexGuard aGuard;
    ::Point aPos( rMouseEvent.X, rMouseEvent.Y );

    ::MouseEvent aMouseEvent( aPos,
                              static_cast<sal_uInt16>(rMouseEvent.ClickCount),
                              MouseEventModifiers::NONE,
                              convertAwtToVCLMouseButtons( rMouseEvent.Buttons ),
                              0 );
    return MouseMove( aMouseEvent );
}

sal_Bool SAL_CALL SfxStatusBarControl::mouseButtonUp(
    const ::awt::MouseEvent& rMouseEvent )
{
    SolarMutexGuard aGuard;
    ::Point aPos( rMouseEvent.X, rMouseEvent.Y );

    ::MouseEvent aMouseEvent( aPos,
                              static_cast<sal_uInt16>(rMouseEvent.ClickCount),
                              MouseEventModifiers::NONE,
                              convertAwtToVCLMouseButtons( rMouseEvent.Buttons ),
                              0 );
    return MouseButtonUp( aMouseEvent );
}

void SAL_CALL SfxStatusBarControl::command(
    const awt::Point& rPos,
    ::sal_Int32 nCommand,
    sal_Bool /*bMouseEvent*/,
    const css::uno::Any& /*aData*/ )
{
    SolarMutexGuard aGuard;
    ::Point aPos( rPos.X, rPos.Y );
    CommandEvent aCmdEvent( aPos, static_cast<CommandEventId>(nCommand), true, nullptr );

    Command( aCmdEvent );
}

void SAL_CALL SfxStatusBarControl::paint(
    const uno::Reference< awt::XGraphics >& xGraphics,
    const awt::Rectangle& rOutputRectangle,
    ::sal_Int32 /*nStyle*/ )
{
    SolarMutexGuard aGuard;

    OutputDevice* pOutDev = VCLUnoHelper::GetOutputDevice( xGraphics );
    if ( pOutDev )
    {
        ::tools::Rectangle aRect = vcl::unohelper::ConvertToVCLRect(rOutputRectangle);
        UserDrawEvent aUserDrawEvent(pOutDev, aRect, pBar->GetCurItemId());
        Paint( aUserDrawEvent );
    }
}

void SAL_CALL SfxStatusBarControl::click( const awt::Point& )
{
    SolarMutexGuard aGuard;
    Click();
}

void SAL_CALL SfxStatusBarControl::doubleClick( const awt::Point& )
{
}

// old sfx2 interface
void SfxStatusBarControl::StateChangedAtStatusBarControl
(
    sal_uInt16              nSID,
    SfxItemState        eState,
    const SfxPoolItem*  pState  /* Pointer to SfxPoolItem, is only valid
                                   within this Method call. This can be a
                                   Null-Pointer, a Pointer to SfxVoidItem
                                   or of this Type found registered by the
                                   Subclass of SfxStatusBarControl.
                                */
)

/*  [Description]

    The base implementation includes items of type SfxStringItem
    where the text is entered in the status row field and
    SfxVoidItem, where the field is emptied. The base implementation
    should not be called in overriding methods.
*/

{
    DBG_ASSERT( pBar != nullptr, "setting state to dangling StatusBar" );

    const SfxStringItem* pStr = dynamic_cast<const SfxStringItem*>( pState  );
    if ( eState == SfxItemState::DEFAULT && pStr )
        pBar->SetItemText( nSID, pStr->GetValue() );
    else
    {
        DBG_ASSERT( eState != SfxItemState::DEFAULT || IsDisabledItem(pState),
                    "wrong SfxPoolItem subclass in SfxStatusBarControl" );
        pBar->SetItemText( nSID, OUString() );
    }
}


bool SfxStatusBarControl::MouseButtonDown( const MouseEvent & )

/*  [Description]

    This virtual method forwards the Event MouseButtonDown() of the
    StatusBar if the mouse position is within the range of the items,
    or if the mouse was captured by <SfxStatusBarControl::CaptureMouse()>

    The default implementation is empty and returns FALSE.

    [Return value]

    sal_Bool                TRUE
           The event has been processed and is not intended to
           be forwarded to StatusBar

                        FALSE
           The event was not processed and is to be
           be forwarded to StatusBar
*/

{
    return false;
}


bool SfxStatusBarControl::MouseMove( const MouseEvent & )

/*  [Description]

    This virtual method forwards the Event MouseMove() of the
    StatusBar if the mouse position is within the range of the items,
    or if the mouse was captured by <SfxStatusBarControl::CaptureMouse()>

    The default implementation is empty and returns FALSE.

    [Return value]

    sal_Bool                TRUE
           The event has been processed and is not intended to
           be forwarded to StatusBar

                        FALSE
           The event was not processed and is to be
           be forwarded to StatusBar
*/

{
    return false;
}


bool SfxStatusBarControl::MouseButtonUp( const MouseEvent & )

/*  [Description]

    This virtual method forwards the Event MouseButtonUp() of the
    StatusBar if the mouse position is within the range of the items,
    or if the mouse was captured by <SfxStatusBarControl::CaptureMouse()>

    The default implementation is empty and returns FALSE.

    [Return value]

    sal_Bool                TRUE
           The event has been processed and is not intended to
           be forwarded to StatusBar

                        FALSE
           The event was not processed and is to be
           be forwarded to StatusBar
*/

{
    return false;
}


void SfxStatusBarControl::Command( const CommandEvent& )

/*  [Description]

    This virtual method is called when a CommandEvent is received by
    SfxStatusBarControl.

    The default implementation is empty.
*/

{
}


void SfxStatusBarControl::Click()

/*  [Description]

    This virtual method is called when the user clicks on the
    field in the status row that belongs to this control.
*/

{
    css::uno::Sequence< css::beans::PropertyValue > aArgs;
    execute( aArgs );
}


void SfxStatusBarControl::Paint
(
    const UserDrawEvent& /* Reference to an UserDrawEvent */
)

/*  [Description]

    This virtual method is called to paint the contents if the field
    at hand is marked with StatusBarItemBits::UserDraw. The output must be obtained
    within the Rectangle of rUDEvt.GetRect() by the OutputDevice
    given by rUDEvt.GetDevice().

    The default implementation is empty.
*/

{
}


rtl::Reference<SfxStatusBarControl> SfxStatusBarControl::CreateControl
(
    sal_uInt16     nSlotID,
    sal_uInt16     nStbId,
    StatusBar* pBar,
    SfxModule const * pMod
)
{
    SolarMutexGuard aGuard;
    SfxApplication *pApp = SfxGetpApp();

    SfxSlotPool *pSlotPool;
    if ( pMod )
        pSlotPool = pMod->GetSlotPool();
    else
        pSlotPool = &SfxSlotPool::GetSlotPool();

    const std::type_info* aSlotType = pSlotPool->GetSlotType(nSlotID);
    if ( aSlotType )
    {
        if ( pMod )
        {
            SfxStbCtrlFactory *pFact = pMod->GetStbCtrlFactory(*aSlotType, nSlotID);
            if ( pFact )
                return pFact->pCtor( nSlotID, nStbId, *pBar );
        }

        SfxStbCtrlFactory* pFact = pApp->GetStbCtrlFactory(*aSlotType, nSlotID);
        if (pFact)
            return pFact->pCtor( nSlotID, nStbId, *pBar );
    }

    return nullptr;
}


void SfxStatusBarControl::RegisterStatusBarControl(SfxModule* pMod, const SfxStbCtrlFactory& rFact)
{
    SfxGetpApp()->RegisterStatusBarControl_Impl( pMod, rFact );
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
