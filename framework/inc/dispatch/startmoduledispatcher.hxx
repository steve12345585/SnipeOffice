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

#include <com/sun/star/frame/XStatusListener.hpp>
#include <com/sun/star/frame/XNotifyingDispatch.hpp>
#include <com/sun/star/frame/XDispatchInformationProvider.hpp>
#include <com/sun/star/util/URL.hpp>
#include <com/sun/star/frame/XDispatchResultListener.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <cppuhelper/implbase.hxx>

namespace framework{

/**
    @short          helper to handle all URLs related to the StartModule
 */
class StartModuleDispatcher final : public  ::cppu::WeakImplHelper<
                                         css::frame::XNotifyingDispatch,             // => XDispatch
                                         css::frame::XDispatchInformationProvider >
{

    // member

    private:

        /** @short reference to a uno service manager,
                   which can be used to create own needed
                   uno resources. */
        css::uno::Reference< css::uno::XComponentContext > m_xContext;

    // native interface

    public:

        /** @short  connect a new StartModuleDispatcher instance to its "owner frame".

            @descr  Such "owner frame" is used as context for all related operations.

            @param  xContext
                    a UNO service manager, which is needed to create UNO resource
                    internally.

            @param  xFrame
                    the frame where the corresponding dispatch was started.
         */
        StartModuleDispatcher(css::uno::Reference< css::uno::XComponentContext > xContext);

        /** @short  does nothing real. */
        virtual ~StartModuleDispatcher() override;

    // uno interface

    public:

        // XNotifyingDispatch
        virtual void SAL_CALL dispatchWithNotification( const css::util::URL&                                             aURL      ,
                                                        const css::uno::Sequence< css::beans::PropertyValue >&            lArguments,
                                                        const css::uno::Reference< css::frame::XDispatchResultListener >& xListener ) override;

        // XDispatch
        virtual void SAL_CALL dispatch            ( const css::util::URL&                                     aURL      ,
                                                    const css::uno::Sequence< css::beans::PropertyValue >&    lArguments) override;
        virtual void SAL_CALL addStatusListener   ( const css::uno::Reference< css::frame::XStatusListener >& xListener ,
                                                    const css::util::URL&                                     aURL      ) override;
        virtual void SAL_CALL removeStatusListener( const css::uno::Reference< css::frame::XStatusListener >& xListener ,
                                                    const css::util::URL&                                     aURL      ) override;

        // XDispatchInformationProvider
        virtual css::uno::Sequence< sal_Int16 >                       SAL_CALL getSupportedCommandGroups         (                         ) override;
        virtual css::uno::Sequence< css::frame::DispatchInformation > SAL_CALL getConfigurableDispatchInformation( sal_Int16 nCommandGroup ) override;

    // internal helper

    private:

        /** @short  check if StartModule can be shown.
         */
        bool implts_isBackingModePossible();

        /** @short  open the special BackingComponent (now StartModule)
         */
        void implts_establishBackingMode();

        /** @short  notify a DispatchResultListener.

            @descr  We check the listener reference before we use it.
                    So this method can be called every time!

            @parama xListener
                    the listener, which should be notified.
                    Can be null!

            @param  nState
                    directly used as css::frame::DispatchResultState value.

            @param  aResult
                    not used yet really ...
         */
        void implts_notifyResultListener(const css::uno::Reference< css::frame::XDispatchResultListener >& xListener,
                                               sal_Int16                                                   nState   ,
                                         const css::uno::Any&                                              aResult  );

}; // class StartModuleDispatcher

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
