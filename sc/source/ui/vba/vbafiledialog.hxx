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

#include <ooo/vba/excel/XFileDialog.hpp>
#include <vbahelper/vbahelperinterface.hxx>

namespace com::sun::star::uno { class XComponentContext; }
namespace ooo::vba { class XHelperInterface; }
namespace ooo::vba::excel { class XFileDialogSelectedItems; }

typedef InheritedHelperInterfaceWeakImpl< ov::excel::XFileDialog > ScVbaFileDialog_BASE;

class ScVbaFileDialog : public ScVbaFileDialog_BASE
{
private:
    sal_Int32 m_nType;
    OUString m_sTitle;
    OUString m_sInitialFileName;
    bool m_bMultiSelectMode;
    css::uno::Reference< ov::excel::XFileDialogSelectedItems> m_xItems;
public:
    ScVbaFileDialog( const css::uno::Reference< ov::XHelperInterface >& xParent,  const css::uno::Reference< css::uno::XComponentContext >& xContext, const sal_Int32 nType);

    virtual css::uno::Any SAL_CALL getInitialFileName() override;
    virtual void SAL_CALL setInitialFileName( const css::uno::Any& rName ) override;
    virtual css::uno::Any SAL_CALL getTitle() override;
    virtual void SAL_CALL setTitle( const css::uno::Any& rTitle ) override;
    virtual css::uno::Any SAL_CALL getAllowMultiSelect() override;
    virtual void SAL_CALL setAllowMultiSelect(const css::uno::Any& rAllowMultiSelect) override;

    virtual css::uno::Reference< ov::excel::XFileDialogSelectedItems > SAL_CALL getSelectedItems() override;

    virtual sal_Int32 SAL_CALL Show() override;

    //XHelperInterface
    virtual OUString getServiceImplName() override;
    virtual css::uno::Sequence<OUString> getServiceNames() override;
};


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
