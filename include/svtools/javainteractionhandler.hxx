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

#include <svtools/svtdllapi.h>
#include <sal/types.h>

#include <com/sun/star/task/XInteractionHandler.hpp>

inline constexpr OUString JAVA_INTERACTION_HANDLER_NAME = u"java-vm.interaction-handler"_ustr;

namespace svt
{
// We cannot derive from  cppu::WeakImplHelper because we would export the inline
//generated class. This conflicts with other libraries if they use the same inline
//class.

class SVT_DLLPUBLIC JavaInteractionHandler final :
        public css::task::XInteractionHandler
{
public:
    JavaInteractionHandler();

    // XInterface
    virtual css::uno::Any SAL_CALL queryInterface(
        const css::uno::Type& aType ) override;

    virtual void SAL_CALL acquire() noexcept override;

    virtual void SAL_CALL release() noexcept override;

    // XCurrentContext
    virtual void SAL_CALL handle( const css::uno::Reference< css::task::XInteractionRequest >& Request ) override;

private:
    oslInterlockedCount m_aRefCount;
    JavaInteractionHandler(JavaInteractionHandler const&) = delete;
    JavaInteractionHandler& operator = (JavaInteractionHandler const &) = delete;
    SVT_DLLPRIVATE virtual ~JavaInteractionHandler();
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
