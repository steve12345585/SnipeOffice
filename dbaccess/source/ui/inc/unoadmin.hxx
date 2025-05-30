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

#include <svtools/genericunodialog.hxx>
#include <com/sun/star/sdbc/XConnection.hpp>
#include <dsntypes.hxx>

class SfxItemSet;
class SfxItemPool;

namespace dbaui
{

// ODatabaseAdministrationDialog
typedef ::svt::OGenericUnoDialog ODatabaseAdministrationDialogBase;
class ODatabaseAdministrationDialog
        :public ODatabaseAdministrationDialogBase
{
protected:
    std::unique_ptr<SfxItemSet> m_pDatasourceItems; // item set for the dialog
    rtl::Reference<SfxItemPool> m_pItemPool;            // item pool for the item set for the dialog
    std::unique_ptr<::dbaccess::ODsnTypeCollection>
                            m_pCollection;          // datasource type collection

    css::uno::Any           m_aInitialSelection;
    css::uno::Reference< css::sdbc::XConnection > m_xActiveConnection;

protected:
    ODatabaseAdministrationDialog(const css::uno::Reference< css::uno::XComponentContext >& _rxORB);
    virtual ~ODatabaseAdministrationDialog() override;
protected:
// OGenericUnoDialog overridables
    virtual void implInitialize(const css::uno::Any& _rValue) override;
};

}   // namespace dbaui

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
