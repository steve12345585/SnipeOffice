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

#ifndef INCLUDED_SVTOOLS_ITEMHOLDER2_HXX_
#define INCLUDED_SVTOOLS_ITEMHOLDER2_HXX_

#include <unotools/itemholderbase.hxx>
#include <cppuhelper/implbase.hxx>
#include <com/sun/star/lang/XEventListener.hpp>
#include <mutex>

class ItemHolder2 : public  ::cppu::WeakImplHelper< css::lang::XEventListener >
{
    // member
private:

        std::mutex m_aLock;
        std::vector<TItemInfo> m_lItems;

    // c++ interface
    public:

        ItemHolder2();
        virtual ~ItemHolder2() override;
        static void holdConfigItem(EItem eItem);

    // uno interface
    public:

        virtual void SAL_CALL disposing(const css::lang::EventObject& aEvent) override;

    // helper
    private:

        void impl_addItem(EItem eItem);
        void impl_releaseAllItems();
        static void impl_newItem(TItemInfo& rItem);
};

#endif // INCLUDED_SVTOOLS_ITEMHOLDER2_HXX_

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
