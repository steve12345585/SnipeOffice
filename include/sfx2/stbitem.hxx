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
#ifndef INCLUDED_SFX2_STBITEM_HXX
#define INCLUDED_SFX2_STBITEM_HXX

#include <sal/config.h>
#include <sfx2/dllapi.h>
#include <svl/poolitem.hxx>
#include <svtools/statusbarcontroller.hxx>
#include <vcl/vclptr.hxx>

class SfxModule;
class SfxStatusBarControl;
class StatusBar;

rtl::Reference<svt::StatusbarController> SfxStatusBarControllerFactory(
    const css::uno::Reference< css::frame::XFrame >& rFrame,
    StatusBar* pStatusBar,
    unsigned short nID,
    const OUString& aCommandURL );
typedef rtl::Reference<SfxStatusBarControl> (*SfxStatusBarControlCtor)( sal_uInt16 nSlotId, sal_uInt16 nId, StatusBar &rStb );

struct SfxStbCtrlFactory
{
    SfxStatusBarControlCtor pCtor;
    const std::type_info&   nTypeId;
    sal_uInt16              nSlotId;

    SfxStbCtrlFactory( SfxStatusBarControlCtor pTheCtor,
            const std::type_info& nTheTypeId, sal_uInt16 nTheSlotId ):
        pCtor(pTheCtor),
        nTypeId(nTheTypeId),
        nSlotId(nTheSlotId)
    {}
};


class CommandEvent;
class MouseEvent;
class UserDrawEvent;

class SFX2_DLLPUBLIC SfxStatusBarControl: public svt::StatusbarController
{
    sal_uInt16         nSlotId;
    sal_uInt16         nId;
    VclPtr<StatusBar>  pBar;

public:
    // new controller API
    // XInterface
    virtual void               SAL_CALL acquire() noexcept override;
    virtual void               SAL_CALL release() noexcept override;

protected:
    // XEventListener
    using svt::StatusbarController::disposing;

    // XComponent
    using svt::StatusbarController::dispose;

    // XStatusListener
    virtual void SAL_CALL statusChanged( const css::frame::FeatureStateEvent& Event ) override;

    // XStatusbarController
    virtual sal_Bool SAL_CALL mouseButtonDown( const css::awt::MouseEvent& aMouseEvent ) override;
    virtual sal_Bool SAL_CALL mouseMove( const css::awt::MouseEvent& aMouseEvent ) override;
    virtual sal_Bool SAL_CALL mouseButtonUp( const css::awt::MouseEvent& aMouseEvent ) override;
    virtual void SAL_CALL command( const css::awt::Point& aPos,
                                    ::sal_Int32 nCommand,
                                    sal_Bool bMouseEvent,
                                    const css::uno::Any& aData ) override;
    virtual void SAL_CALL paint( const css::uno::Reference< css::awt::XGraphics >& xGraphics,
                                 const css::awt::Rectangle& rOutputRectangle,
                                 ::sal_Int32 nStyle ) override;
    virtual void SAL_CALL click( const css::awt::Point& aPos ) override;
    virtual void SAL_CALL doubleClick( const css::awt::Point& aPos ) override;

    // Old sfx2 interface
    virtual void    StateChangedAtStatusBarControl( sal_uInt16 nSID, SfxItemState eState,
                                  const SfxPoolItem* pState );
    virtual void    Click();
    virtual void    Command( const CommandEvent& rCEvt );
    virtual bool    MouseButtonDown( const MouseEvent & );
    virtual bool    MouseMove( const MouseEvent & );
    virtual bool    MouseButtonUp( const MouseEvent & );
    virtual void    Paint( const UserDrawEvent &rUDEvt );

    static sal_uInt16   convertAwtToVCLMouseButtons( sal_Int16 nAwtMouseButtons );

public:
                    SfxStatusBarControl( sal_uInt16 nSlotID, sal_uInt16 nId, StatusBar& rBar );
    virtual         ~SfxStatusBarControl() override;

    sal_uInt16      GetSlotId() const { return nSlotId; }
    sal_uInt16      GetId() const { return nId; }
    StatusBar&      GetStatusBar() const { return *pBar; }

    static rtl::Reference<SfxStatusBarControl> CreateControl( sal_uInt16 nSlotID, sal_uInt16 nId, StatusBar *pBar, SfxModule const * );
    static void RegisterStatusBarControl(SfxModule*, const SfxStbCtrlFactory&);

};


#define SFX_DECL_STATUSBAR_CONTROL() \
        static rtl::Reference<SfxStatusBarControl> CreateImpl( sal_uInt16 nSlotId, sal_uInt16 nId, StatusBar &rStb ); \
        static void RegisterControl(sal_uInt16 nSlotId = 0, SfxModule *pMod=nullptr)

#define SFX_IMPL_STATUSBAR_CONTROL(Class, nItemClass) \
        rtl::Reference<SfxStatusBarControl> Class::CreateImpl( sal_uInt16 nSlotId, sal_uInt16 nId, StatusBar &rStb ) \
               { return new Class( nSlotId, nId, rStb ); } \
        void Class::RegisterControl(sal_uInt16 nSlotId, SfxModule *pMod) \
               { SfxStatusBarControl::RegisterStatusBarControl( pMod, SfxStbCtrlFactory( \
                    Class::CreateImpl, typeid(nItemClass), nSlotId ) ); }


#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
