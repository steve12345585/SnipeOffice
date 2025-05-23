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

#pragma once

#include <config_options.h>
#include <svtools/svtdllapi.h>

#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/frame/XDispatch.hpp>
#include <com/sun/star/frame/XStatusListener.hpp>
#include <com/sun/star/frame/XPopupMenuController.hpp>

#include <tools/link.hxx>
#include <comphelper/compbase.hxx>
#include <rtl/ref.hxx>
#include <rtl/ustring.hxx>

namespace com :: sun :: star :: frame { class XFrame; }
namespace com :: sun :: star :: uno { class XComponentContext; }
namespace com :: sun :: star :: util { class XURLTransformer; }
class VCLXPopupMenu;

namespace svt
{

    typedef comphelper::WeakComponentImplHelper<
                        css::lang::XServiceInfo            ,
                        css::frame::XPopupMenuController ,
                        css::lang::XInitialization         ,
                        css::frame::XStatusListener        ,
                        css::awt::XMenuListener            ,
                        css::frame::XDispatchProvider      ,
                        css::frame::XDispatch > PopupMenuControllerBaseType;

    class UNLESS_MERGELIBS(SVT_DLLPUBLIC) PopupMenuControllerBase : public PopupMenuControllerBaseType
    {
        public:
            PopupMenuControllerBase( const css::uno::Reference< css::uno::XComponentContext >& xContext );
            virtual ~PopupMenuControllerBase() override;

            // XServiceInfo
            virtual OUString SAL_CALL getImplementationName(  ) override = 0;
            virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
            virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override = 0;

            // XPopupMenuController
            virtual void SAL_CALL setPopupMenu( const css::uno::Reference< css::awt::XPopupMenu >& PopupMenu ) override;
            virtual void SAL_CALL updatePopupMenu() override;

            // XInitialization
            virtual void SAL_CALL initialize( const css::uno::Sequence< css::uno::Any >& aArguments ) override final;

            // XStatusListener
            virtual void SAL_CALL statusChanged( const css::frame::FeatureStateEvent& Event ) override = 0;

            // XMenuListener
            virtual void SAL_CALL itemHighlighted( const css::awt::MenuEvent& rEvent ) override;
            virtual void SAL_CALL itemSelected( const css::awt::MenuEvent& rEvent ) override;
            virtual void SAL_CALL itemActivated( const css::awt::MenuEvent& rEvent ) override;
            virtual void SAL_CALL itemDeactivated( const css::awt::MenuEvent& rEvent ) override;

            // XDispatchProvider
            virtual css::uno::Reference< css::frame::XDispatch > SAL_CALL queryDispatch( const css::util::URL& aURL, const OUString& sTarget, sal_Int32 nFlags ) override;
            virtual css::uno::Sequence< css::uno::Reference< css::frame::XDispatch > > SAL_CALL queryDispatches( const css::uno::Sequence< css::frame::DispatchDescriptor >& lDescriptor ) override;

            // XDispatch
            virtual void SAL_CALL dispatch( const css::util::URL& aURL, const css::uno::Sequence< css::beans::PropertyValue >& seqProperties ) override;
            virtual void SAL_CALL addStatusListener( const css::uno::Reference< css::frame::XStatusListener >& xControl, const css::util::URL& aURL ) override;
            virtual void SAL_CALL removeStatusListener( const css::uno::Reference< css::frame::XStatusListener >& xControl, const css::util::URL& aURL ) override;

            // XEventListener
            virtual void SAL_CALL disposing( const css::lang::EventObject& Source ) override;

            void dispatchCommand( const OUString& sCommandURL, const css::uno::Sequence< css::beans::PropertyValue >& rArgs, const OUString& sTarget = OUString() );

    protected:
            virtual void initializeImpl( std::unique_lock<std::mutex>& rGuard, const css::uno::Sequence< css::uno::Any >& aArguments );

            void dispatchCommandImpl( std::unique_lock<std::mutex>& rGuard, const OUString& sCommandURL, const css::uno::Sequence< css::beans::PropertyValue >& rArgs, const OUString& sTarget );

            /** helper method to cause statusChanged is called once for the given command url */
            void updateCommand( const OUString& rCommandURL );

            /** this function is called upon disposing the component
            */
            virtual void disposing(std::unique_lock<std::mutex>& rGuard) override;

            static void resetPopupMenu( css::uno::Reference< css::awt::XPopupMenu > const & rPopupMenu );
            virtual void impl_setPopupMenu(std::unique_lock<std::mutex>& rGuard);
            static OUString determineBaseURL( std::u16string_view aURL );

            DECL_DLLPRIVATE_STATIC_LINK( PopupMenuControllerBase, ExecuteHdl_Impl, void*, void );


            bool                                                   m_bInitialized;
            OUString                                               m_aCommandURL;
            OUString                                               m_aBaseURL;
            OUString                                               m_aModuleName;
            css::uno::Reference< css::frame::XDispatch >           m_xDispatch;
            css::uno::Reference< css::frame::XFrame >              m_xFrame;
            css::uno::Reference< css::util::XURLTransformer >      m_xURLTransformer;
            rtl::Reference< VCLXPopupMenu >                        m_xPopupMenu;
            comphelper::OInterfaceContainerHelper4<XStatusListener> maStatusListeners;
    };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
