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

#include <vcl/accessibility/AccessibleBrowseBoxBase.hxx>

class VCL_DLLPUBLIC AccessibleBrowseBoxHeaderCell final : public AccessibleBrowseBoxBase
{
    sal_Int32   m_nColumnRowId;
public:
    AccessibleBrowseBoxHeaderCell(sal_Int32 _nColumnRowId,
                                      const css::uno::Reference< css::accessibility::XAccessible >& rxParent,
                                      ::vcl::IAccessibleTableProvider&                  _rBrowseBox,
                                      AccessibleBrowseBoxObjType  _eObjType);
    /** @return  The count of visible children. */
    virtual sal_Int64 SAL_CALL getAccessibleChildCount() override;

    /** @return  The XAccessible interface of the specified child. */
    virtual css::uno::Reference< css::accessibility::XAccessible > SAL_CALL
        getAccessibleChild( sal_Int64 nChildIndex ) override;

    /** @return  The index of this object among the parent's children. */
    virtual sal_Int64 SAL_CALL getAccessibleIndexInParent() override;

    /** Grabs the focus to the BrowseBox. */
    virtual void SAL_CALL grabFocus() override;

    bool isRowBarCell() const
    {
        return getType() == AccessibleBrowseBoxObjType::RowHeaderCell;
    }

    /** @return
            The name of this class.
    */
    virtual OUString SAL_CALL getImplementationName() override;

    /** Return a bitset of states of the current object.
    */
    sal_Int64 implCreateStateSet() override;

private:
    virtual tools::Rectangle implGetBoundingBox() override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
